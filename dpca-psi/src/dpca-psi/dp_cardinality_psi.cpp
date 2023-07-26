// Copyright 2023 TikTok Pte. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dpca-psi/dp_cardinality_psi.h"

#include <omp.h>

#include <algorithm>
#include <map>
#include <set>
#include <thread>
#include <utility>

#include "glog/logging.h"

#include "dpca-psi/common/defines.h"
#include "dpca-psi/common/parameter_check.h"
#include "dpca-psi/common/utils.h"
#include "dpca-psi/crypto/ipcl_utils.h"

namespace privacy_go {
namespace dpca_psi {

DPCardinalityPSI::DPCardinalityPSI() {
}

void DPCardinalityPSI::init(const json& params, std::shared_ptr<IOBase> net) {
    auto defalut_config = R"({
        "common": {
            "address": "127.0.0.1",
            "remote_port": 30330,
            "local_port": 30331,
            "timeout": 90,
            "input_file": "example/data/sender_input_file.csv",
            "has_header": false,
            "output_file": "example/data/sender_output_file.csv",
            "ids_num": 3,
            "is_sender": true,
            "verbose": false
        },
        "paillier_params": {
            "paillier_n_len": 2048,
            "enable_djn": true,
            "apply_packing": true,
            "statistical_security_bits": 40
        },
        "ecc_params": {
            "curve_id": 415
        },
        "dp_params": {
            "epsilon": 2.0,
            "maximum_queries": 10,
            "use_precomputed_tau": true,
            "precomputed_tau": 1440,
            "input_dp": true,
            "has_zero_column": false,
            "zero_column_index": -1
        }
    })"_json;

    defalut_config.merge_patch(params);
    params_ = defalut_config;
    io_ = net;

    verbose_ = params_["common"]["verbose"];
    is_sender_ = params_["common"]["is_sender"];

    check_params();

    key_size_ = params_["common"]["ids_num"];
    apply_packing_ = params_["paillier_params"]["apply_packing"];
    if (apply_packing_) {
        statistical_security_bits_ = params_["paillier_params"]["statistical_security_bits"];
        slot_bits_ = kValueBits + statistical_security_bits_ + 1;
    }
    LOG_IF(INFO, verbose_) << "\nDPCA PSI parameters: \n" << params_.dump(4);

    std::size_t curve_id = params_["ecc_params"]["curve_id"];
    ecc_cipher_ = std::make_unique<EccCipher>(curve_id, key_size_);
    LOG_IF(INFO, verbose_) << "ecc curve id is " << curve_id;

    num_threads_ = omp_get_max_threads();

    std::size_t paillier_n_len = params_["paillier_params"]["paillier_n_len"];
    LOG_IF(INFO, verbose_) << "paillier n len is " << paillier_n_len;

    bool enable_djn = params_["paillier_params"]["enable_djn"];

    if (is_sender_) {
        sender_paillier_.keygen(paillier_n_len, enable_djn);

        io_->send_value<bool>(enable_djn);
        io_->send_bytes(sender_paillier_.export_pk());
        LOG_IF(INFO, verbose_) << "sender sent paillier pk";

        bool receiver_enable_djn = io_->recv_value<bool>();
        ByteVector receiver_pk;
        io_->recv_bytes(receiver_pk);
        LOG_IF(INFO, verbose_) << "sender received paillier pk";
        receiver_paillier_.import_pk(receiver_pk, receiver_enable_djn);
    } else {
        receiver_paillier_.keygen(paillier_n_len, enable_djn);

        bool sender_enable_djn = io_->recv_value<bool>();
        ByteVector sender_pk;
        io_->recv_bytes(sender_pk);
        LOG_IF(INFO, verbose_) << "receiver received paillier pk";

        io_->send_value<bool>(enable_djn);
        io_->send_bytes(receiver_paillier_.export_pk());
        LOG_IF(INFO, verbose_) << "receiver sent paillier pk";
        sender_paillier_.import_pk(sender_pk, sender_enable_djn);
    }
}

void DPCardinalityPSI::data_sampling(
        const std::vector<std::vector<std::string>>& keys, const std::vector<std::vector<std::uint64_t>>& features) {
    if (is_sender_) {
        sender_data_size_ = keys[0].size();
        sender_feature_size_ = features.size();

        // sync data size and feature size.
        io_->send_value<std::size_t>(sender_data_size_);
        io_->send_value<std::size_t>(sender_feature_size_);
        receiver_data_size_ = io_->recv_value<std::size_t>();
        receiver_feature_size_ = io_->recv_value<std::size_t>();
    } else {
        receiver_data_size_ = keys[0].size();
        receiver_feature_size_ = features.size();

        // sync data size and feature size.
        sender_data_size_ = io_->recv_value<std::size_t>();
        sender_feature_size_ = io_->recv_value<std::size_t>();
        io_->send_value<std::size_t>(receiver_data_size_);
        io_->send_value<std::size_t>(receiver_feature_size_);
    }

    LOG_IF(INFO, verbose_) << "sender data size is " << sender_data_size_;
    LOG_IF(INFO, verbose_) << "sender feature size is " << sender_feature_size_;
    LOG_IF(INFO, verbose_) << "receiver data size is  " << receiver_data_size_;
    LOG_IF(INFO, verbose_) << "receiver feature size is " << receiver_feature_size_;

    plaintext_keys_.assign(keys.begin(), keys.end());
    plaintext_features_.assign(features.begin(), features.end());

    bool input_dp = params_["dp_params"]["input_dp"];
    LOG_IF(INFO, verbose_) << "apply input dp " << input_dp;

    if (input_dp) {
        std::size_t max_data_size = std::max(sender_data_size_, receiver_data_size_);
        double epsilon = params_["dp_params"]["epsilon"];
        std::size_t maximum_queries = params_["dp_params"]["maximum_queries"];
        bool use_precomputed_tau = params_["dp_params"]["use_precomputed_tau"];
        std::size_t precomputed_tau = 0;
        if (use_precomputed_tau) {
            precomputed_tau = params_["dp_params"]["precomputed_tau"];
            if (is_sender_) {
                io_->send_value<std::size_t>(precomputed_tau);
                std::size_t remote_tau = io_->recv_value<std::size_t>();
                precomputed_tau = std::max(remote_tau, precomputed_tau);
            } else {
                std::size_t remote_tau = io_->recv_value<std::size_t>();
                io_->send_value<std::size_t>(precomputed_tau);
                precomputed_tau = std::max(remote_tau, precomputed_tau);
            }
        }
        bool has_zero_column = params_["dp_params"]["has_zero_column"];
        int zero_column_index = params_["dp_params"]["zero_column_index"];
        std::size_t feature_size = is_sender_ ? sender_feature_size_ : receiver_feature_size_;
        if (feature_size == 0) {
            zero_column_index = -1;
        } else {
            int temp_idx = (zero_column_index + static_cast<int>(feature_size)) % static_cast<int>(feature_size);
            zero_column_index = has_zero_column ? temp_idx : -1;
        }
        LOG_IF(INFO, verbose_) << "\nDP parameters: "
                               << "\ndata size: " << max_data_size << "\nepsilon: " << epsilon
                               << "\nmaximum queries: " << maximum_queries
                               << "\nuse_precomputed_tau: " << use_precomputed_tau
                               << "\nprecomputed_tau: " << precomputed_tau << "\nhas zero column: " << has_zero_column
                               << "\nzero column index: " << zero_column_index << "\nfeature size: " << feature_size;

        DPSampling dp_sampling;
        if (is_sender_) {
            block common_seed = read_block_from_dev_urandom();
            io_->send_value<block>(common_seed);
            dp_sampling.set_common_prng_seed(common_seed);
        } else {
            block common_seed = io_->recv_value<block>();
            dp_sampling.set_common_prng_seed(common_seed);
        }

        LOG_IF(INFO, verbose_) << "dp sample start.";
        auto sampled_dummies = dp_sampling.multi_key_sampling(
                key_size_, feature_size, zero_column_index, is_sender_, use_precomputed_tau, precomputed_tau);
        LOG_IF(INFO, verbose_) << "dp sample end. dummy data size is " << sampled_dummies.first[0].size();

        std::size_t dummied_data_size =
                sampled_dummies.first[0].size() + (is_sender_ ? sender_data_size_ : receiver_data_size_);
        for (std::size_t key_idx = 0; key_idx < plaintext_keys_.size(); ++key_idx) {
            plaintext_keys_[key_idx].insert(plaintext_keys_[key_idx].end(), sampled_dummies.first[key_idx].begin(),
                    sampled_dummies.first[key_idx].end());
            LOG_IF(INFO, verbose_) << "total data size of key " << key_idx << " is " << plaintext_keys_[key_idx].size();
        }
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            plaintext_features_[feat_idx].insert(plaintext_features_[feat_idx].end(),
                    sampled_dummies.second[feat_idx].begin(), sampled_dummies.second[feat_idx].end());
            LOG_IF(INFO, verbose_) << "total data size of feature " << feat_idx << " is "
                                   << plaintext_features_[feat_idx].size();
        }

        if (is_sender_) {
            sender_data_size_ = dummied_data_size;
            io_->send_value<std::size_t>(sender_data_size_);
            receiver_data_size_ = io_->recv_value<std::size_t>();
        } else {
            sender_data_size_ = io_->recv_value<std::size_t>();
            receiver_data_size_ = dummied_data_size;
            io_->send_value<std::size_t>(receiver_data_size_);
        }
        LOG_IF(INFO, verbose_) << "updated sender data size is " << sender_data_size_;
        LOG_IF(INFO, verbose_) << "updated receiver data size is " << receiver_data_size_;
    }

    sender_permutation_ = generate_permutation(sender_data_size_);
    receiver_permutation_ = generate_permutation(receiver_data_size_);
    LOG_IF(INFO, verbose_) << "generate permutation done.";
}

void DPCardinalityPSI::process(std::vector<std::vector<std::uint64_t>>& shares) {
    std::vector<std::vector<ByteVector>> encrypted_keys;
    shuffle_and_encrypt_keys_round_one(encrypted_keys);
    LOG_IF(INFO, verbose_) << "shuffle and encrypt keys round one done.";

    auto received_data_size = is_sender_ ? receiver_data_size_ : sender_data_size_;
    exchange_encrypted_keys(encrypted_keys, key_size_, received_data_size, exchanged_keys_, kEccPointLen);
    for (std::size_t key_idx = 0; key_idx < encrypted_keys.size(); ++key_idx) {
        encrypted_keys[key_idx].clear();
    }
    encrypted_keys.clear();
    LOG_IF(INFO, verbose_) << "send and receive encryptd keys round one done.";

    std::vector<ByteVector> reshuffled_keys;
    reshuffle_and_encrypt_exchanged_keys_round_one(reshuffled_keys);
    LOG_IF(INFO, verbose_) << "reshuffle and double encrypt keys round one done.";
    received_data_size = is_sender_ ? sender_data_size_ : receiver_data_size_;
    std::vector<ByteVector> single_encrypted_keys;
    exchange_single_encrypted_keys(reshuffled_keys, received_data_size, single_encrypted_keys, kECCCompareBytesLen);
    reshuffled_keys.clear();
    LOG_IF(INFO, verbose_) << "send and receive double encryptd keys round one done.";

    auto intersection_size_round_one = calculate_intersection_round_one(single_encrypted_keys, exchanged_keys_[0]);
    single_encrypted_keys.clear();
    LOG_IF(INFO, verbose_) << "intersection size round 1 is " << intersection_size_round_one;

    LOG_IF(INFO, verbose_) << "repeatedly match begin.";
    auto intersection_size = repeatedly_match(intersection_size_round_one);
    LOG_IF(INFO, verbose_) << "repeatedly match end.";

    LOG_IF(INFO, verbose_) << "calculates intersection and saves intersection indices done.";
    LOG_IF(INFO, verbose_) << "intersection size is " << intersection_size;

    std::vector<std::vector<ByteVector>> encrypted_features;
    shuffle_and_encrypt_features(encrypted_features);
    LOG_IF(INFO, verbose_) << "shuffle and encrypt features done.";

    std::vector<std::vector<ByteVector>> exchanged_encrypted_features;
    auto self_pailler_len = is_sender_ ? sender_paillier_.get_bytes_len(1) : receiver_paillier_.get_bytes_len(1);
    auto remote_paillier_len = is_sender_ ? receiver_paillier_.get_bytes_len(1) : sender_paillier_.get_bytes_len(1);
    auto received_feature_size = is_sender_ ? receiver_feature_size_ : sender_feature_size_;
    if (apply_packing_) {
        std::size_t packing_capacity = remote_paillier_len * 4 / slot_bits_;
        received_feature_size = (received_feature_size + packing_capacity - 1) / packing_capacity;
    }
    received_data_size = is_sender_ ? receiver_data_size_ : sender_data_size_;
    exchange_encrypted_features(encrypted_features, self_pailler_len, remote_paillier_len, received_feature_size,
            received_data_size, exchanged_encrypted_features);
    for (std::size_t feat_idx = 0; feat_idx < encrypted_features.size(); ++feat_idx) {
        encrypted_features[feat_idx].clear();
    }
    encrypted_features.clear();
    LOG_IF(INFO, verbose_) << "send and receive encrypted features done.";

    std::vector<std::vector<ByteVector>> intersection_features;
    filter_intersection_features(exchanged_encrypted_features, intersection_size, intersection_features);
    for (std::size_t feat_idx = 0; feat_idx < exchanged_encrypted_features.size(); ++feat_idx) {
        exchanged_encrypted_features[feat_idx].clear();
    }
    exchanged_encrypted_features.clear();
    LOG_IF(INFO, verbose_) << "filter intersection features done.";

    std::vector<std::vector<BigNumber>> random_r;
    generate_additive_shares((is_sender_ ? receiver_paillier_ : sender_paillier_), intersection_features, random_r);
    LOG_IF(INFO, verbose_) << "generate additive shares done.";

    std::vector<std::vector<ByteVector>> exchanged_shares;
    received_feature_size = is_sender_ ? sender_feature_size_ : receiver_feature_size_;
    if (apply_packing_) {
        std::size_t packing_capacity = self_pailler_len * 4 / slot_bits_;
        received_feature_size = (received_feature_size + packing_capacity - 1) / packing_capacity;
    }
    exchange_encrypted_features(intersection_features, remote_paillier_len, self_pailler_len, received_feature_size,
            intersection_size, exchanged_shares);
    for (std::size_t feat_idx = 0; feat_idx < intersection_features.size(); ++feat_idx) {
        intersection_features[feat_idx].clear();
    }
    intersection_features.clear();
    LOG_IF(INFO, verbose_) << "send and receive encrypted additive shares done.";

    decrypt_and_reveal_shares(exchanged_shares, random_r, intersection_size, shares);
    LOG_IF(INFO, verbose_) << "decrypt and reveal shares done.";

    for (std::size_t feat_idx = 0; feat_idx < random_r.size(); ++feat_idx) {
        random_r[feat_idx].clear();
    }
    random_r.clear();
    for (std::size_t feat_idx = 0; feat_idx < exchanged_shares.size(); ++feat_idx) {
        exchanged_shares[feat_idx].clear();
    }
    exchanged_shares.clear();
    reset_data();
}

void DPCardinalityPSI::check_params() {
    std::size_t curve_id = params_["ecc_params"]["curve_id"];
    check_consistency(is_sender_, io_, "ecc_curve_id", curve_id);
    check_equal<std::size_t>("curve_id", curve_id, 415);

    std::size_t ids_num = params_["common"]["ids_num"];
    check_consistency(is_sender_, io_, "ids_num", ids_num);
    check_in_range<std::size_t>("ids_num", ids_num, 1, 100);

    bool input_dp = params_["dp_params"]["input_dp"];
    check_consistency(is_sender_, io_, "input_dp", input_dp);
    std::size_t paillier_n_len = params_["paillier_params"]["paillier_n_len"];
    check_equal<std::size_t>("paillier_n_len", paillier_n_len, {1024, 2048, 3072});

    bool apply_packing = params_["paillier_params"]["apply_packing"];
    check_consistency(is_sender_, io_, "apply_packing", apply_packing);
    if (apply_packing) {
        std::size_t statistical_security_bits = params_["paillier_params"]["statistical_security_bits"];
        check_consistency(is_sender_, io_, "statistical_security_bits", statistical_security_bits);
        check_in_range<std::size_t>("statistical_security_bits", statistical_security_bits, 40, 80);
    }
    if (input_dp) {
        bool use_precomputed_tau = params_["dp_params"]["use_precomputed_tau"];
        check_consistency(is_sender_, io_, "use_precomputed_tau", use_precomputed_tau);
        if (!use_precomputed_tau) {
            double epsilon = params_["dp_params"]["epsilon"];
            std::size_t maximum_queries = params_["dp_params"]["maximum_queries"];
            check_consistency(is_sender_, io_, "dp_epsilon", epsilon);
            check_consistency(is_sender_, io_, "dp_maximum_queries", maximum_queries);
        } else {
            std::size_t precomputed_tau = params_["dp_params"]["precomputed_tau"];
            check_in_range<std::size_t>("precomputed_tau", precomputed_tau, 0, 1ull << 20);
        }
    }
}

void DPCardinalityPSI::shuffle_and_encrypt_keys_round_one(std::vector<std::vector<ByteVector>>& encrypted_keys) {
    encrypted_keys.reserve(plaintext_keys_.size());
    for (std::size_t key_idx = 0; key_idx < key_size_; ++key_idx) {
        std::vector<ByteVector> encrypted_keys_i;
        encrypted_keys_i.resize(plaintext_keys_[key_idx].size());
        if (is_sender_) {
            permute_and_undo(sender_permutation_, true, plaintext_keys_[key_idx]);
        } else {
            permute_and_undo(receiver_permutation_, true, plaintext_keys_[key_idx]);
        }
#pragma omp parallel for num_threads(num_threads_)
        for (std::size_t item_idx = 0; item_idx < plaintext_keys_[key_idx].size(); ++item_idx) {
            encrypted_keys_i[item_idx] = ecc_cipher_->hash_encrypt(plaintext_keys_[key_idx][item_idx], 0);
        }
        encrypted_keys.emplace_back(encrypted_keys_i);
    }
}

void DPCardinalityPSI::reshuffle_and_encrypt_exchanged_keys_round_one(
        std::vector<ByteVector>& reshuffled_encrypted_keys) {
#pragma omp parallel for num_threads(num_threads_)
    for (std::size_t item_idx = 0; item_idx < exchanged_keys_[0].size(); ++item_idx) {
        auto double_encrypted_key = ecc_cipher_->encrypt(exchanged_keys_[0][item_idx], 0);
        exchanged_keys_[0][item_idx].resize(kECCCompareBytesLen);
        std::copy_n(double_encrypted_key.rbegin(), kECCCompareBytesLen, exchanged_keys_[0][item_idx].rbegin());
    }

    reshuffled_encrypted_keys.assign(exchanged_keys_[0].begin(), exchanged_keys_[0].end());
    if (is_sender_) {
        permute_and_undo(receiver_permutation_, true, reshuffled_encrypted_keys);
    } else {
        permute_and_undo(sender_permutation_, true, reshuffled_encrypted_keys);
    }
}

std::size_t DPCardinalityPSI::repeatedly_match(std::size_t intersection_round_one) {
    auto intersection_size = intersection_round_one;
    for (std::size_t key_idx = 1; key_idx < key_size_; ++key_idx) {
        std::vector<ByteVector> filtered_exchanged_keys_i;
        std::vector<std::size_t> filtered_exchanged_keys_i_mapping;

        // remove the rows that have been matched in (i-1)'s mathcing.
        for (std::size_t item_idx = 0; item_idx < intersection_indices_.size(); ++item_idx) {
            if (!intersection_indices_[item_idx].first) {
                filtered_exchanged_keys_i.push_back(exchanged_keys_[key_idx][item_idx]);
                filtered_exchanged_keys_i_mapping.push_back(item_idx);
            }
        }

#pragma omp parallel for num_threads(num_threads_)
        // encrypt the i-th column's keys.
        for (std::size_t item_idx = 0; item_idx < filtered_exchanged_keys_i.size(); ++item_idx) {
            filtered_exchanged_keys_i[item_idx] = ecc_cipher_->encrypt(filtered_exchanged_keys_i[item_idx], key_idx);
        }

        // shuffle the i-th column's encrypted keys.
        auto permutation_i = generate_permutation(filtered_exchanged_keys_i.size());
        permute_and_undo(permutation_i, true, filtered_exchanged_keys_i);

        // exchange the i-th column's encrypted keys.
        std::vector<ByteVector> single_encrypted_keys;
        auto received_data_size =
                is_sender_ ? sender_data_size_ - intersection_size : receiver_data_size_ - intersection_size;
        exchange_single_encrypted_keys(
                filtered_exchanged_keys_i, received_data_size, single_encrypted_keys, kEccPointLen);
        LOG_IF(INFO, verbose_) << "send and receive encryptd keys round " << key_idx + 1 << " done.";

#pragma omp parallel for num_threads(num_threads_)
        // double encrypt the i-th column's exchanged encrypted keys.
        for (std::size_t item_idx = 0; item_idx < single_encrypted_keys.size(); ++item_idx) {
            auto double_encrypted_key = ecc_cipher_->encrypt_and_div(single_encrypted_keys[item_idx], key_idx, 0);
            single_encrypted_keys[item_idx].resize(kECCCompareBytesLen);
            std::copy_n(double_encrypted_key.rbegin(), kECCCompareBytesLen, single_encrypted_keys[item_idx].rbegin());
        }

        // exchange the i-th column's double encrypted keys.
        received_data_size = filtered_exchanged_keys_i.size();
        filtered_exchanged_keys_i.clear();
        exchange_single_encrypted_keys(
                single_encrypted_keys, received_data_size, filtered_exchanged_keys_i, kECCCompareBytesLen);
        LOG_IF(INFO, verbose_) << "send and receive double encryptd keys round " << key_idx + 1 << " done.";

        permute_and_undo(permutation_i, false, filtered_exchanged_keys_i);

        auto intersection_size_round_i = calculate_intersection_round_i(
                single_encrypted_keys, filtered_exchanged_keys_i, filtered_exchanged_keys_i_mapping);
        LOG_IF(INFO, verbose_) << "intersection size round " << key_idx + 1 << " is " << intersection_size_round_i;
        intersection_size += intersection_size_round_i;
    }
    return intersection_size;
}

std::size_t DPCardinalityPSI::calculate_intersection_round_one(
        const std::vector<ByteVector>& encrypted_keys, const std::vector<ByteVector>& exchanged_keys) {
    intersection_indices_.resize(exchanged_keys.size());
    for (auto flag : intersection_indices_) {
        flag.first = false;
        flag.second = ByteVector();
    }

    std::size_t count = 0;
    std::vector<ByteVector> truncted_encrypted_keys;
    truncted_encrypted_keys.assign(encrypted_keys.begin(), encrypted_keys.end());
    std::sort(truncted_encrypted_keys.begin(), truncted_encrypted_keys.end());

    for (std::size_t item_idx = 0; item_idx < exchanged_keys.size(); ++item_idx) {
        if (!intersection_indices_[item_idx].first) {
            if (std::binary_search(
                        truncted_encrypted_keys.begin(), truncted_encrypted_keys.end(), exchanged_keys[item_idx])) {
                intersection_indices_[item_idx].first = true;
                intersection_indices_[item_idx].second = exchanged_keys[item_idx];
                ++count;
            }
        }
    }
    return count;
}

// Calculates i-th column's intersection and saves intersection indices.
// Intersections of the previous column does not participate in the calculation of the next column.
std::size_t DPCardinalityPSI::calculate_intersection_round_i(const std::vector<ByteVector>& encrypted_keys,
        const std::vector<ByteVector>& exchanged_keys, const std::vector<std::size_t>& mapping) {
    std::size_t count = 0;
    std::vector<ByteVector> truncted_encrypted_keys;
    truncted_encrypted_keys.assign(encrypted_keys.begin(), encrypted_keys.end());
    std::sort(truncted_encrypted_keys.begin(), truncted_encrypted_keys.end());

    for (std::size_t item_idx = 0; item_idx < exchanged_keys.size(); ++item_idx) {
        auto original_idx = mapping[item_idx];
        if (!intersection_indices_[original_idx].first) {
            if (std::binary_search(
                        truncted_encrypted_keys.begin(), truncted_encrypted_keys.end(), exchanged_keys[item_idx])) {
                intersection_indices_[original_idx].first = true;
                intersection_indices_[original_idx].second = exchanged_keys[item_idx];
                ++count;
            }
        }
    }
    return count;
}

void DPCardinalityPSI::shuffle_and_encrypt_features(std::vector<std::vector<ByteVector>>& encrypted_features) {
    auto feature_size = is_sender_ ? sender_feature_size_ : receiver_feature_size_;
    auto data_size = is_sender_ ? sender_data_size_ : receiver_data_size_;

    std::size_t packing_capacity = 1;
    std::size_t raw_feature_size = feature_size;
    if (apply_packing_) {
        packing_capacity = is_sender_ ? (sender_paillier_.get_bytes_len(0) * 8 / slot_bits_)
                                      : (receiver_paillier_.get_bytes_len(0) * 8 / slot_bits_);
        feature_size = (feature_size + packing_capacity - 1) / packing_capacity;
    }

    encrypted_features.resize(feature_size);
    for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
        encrypted_features[feat_idx].resize(data_size);
    }
    auto compute_paillier_cipher = [this, &encrypted_features, feature_size, data_size](const IpclPaillier& pai) {
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            std::vector<BigNumber> plaintexts_bn;
            plaintexts_bn.reserve(data_size);
            for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
                plaintexts_bn.emplace_back(ipcl_u64_2_bn(plaintext_features_[feat_idx][item_idx]));
            }
            ipcl::PlainText plaintexts(plaintexts_bn);
            auto ciphertexts = pai.encrypt(plaintexts);
            for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
                encrypted_features[feat_idx][item_idx] = pai.encode(ciphertexts.getElement(item_idx), true);
            }
        }
    };

    // shifting-and-adding.
    // [x_0||x_1].
    // support the case when feature size is bigger than single cipher's packing capacity.
    auto compute_paillier_cipher_with_packing = [this, &encrypted_features, feature_size, data_size, packing_capacity,
                                                        raw_feature_size](const IpclPaillier& pai) {
        BigNumber bn_slot(BigNumber::One());
        ipcl_bn_lshift(bn_slot, slot_bits_);
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            std::vector<BigNumber> plaintexts_bn;
            plaintexts_bn.reserve(data_size);
            std::size_t cur_packed_num = std::min(packing_capacity, raw_feature_size - feat_idx * packing_capacity);
            for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
                BigNumber packed_value(0);
                packed_value += ipcl_u64_2_bn(plaintext_features_[feat_idx * packing_capacity][item_idx]);
                for (std::size_t pack_idx = 1; pack_idx < cur_packed_num; ++pack_idx) {
                    std::size_t raw_feat_idx = feat_idx * packing_capacity + pack_idx;
                    packed_value *= bn_slot;
                    packed_value += ipcl_u64_2_bn(plaintext_features_[raw_feat_idx][item_idx]);
                }
                plaintexts_bn.emplace_back(packed_value);
            }
            ipcl::PlainText plaintexts(plaintexts_bn);
            auto ciphertexts = pai.encrypt(plaintexts);
            for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
                encrypted_features[feat_idx][item_idx] = pai.encode(ciphertexts.getElement(item_idx), true);
            }
        }
    };

    if (apply_packing_) {
        compute_paillier_cipher_with_packing(is_sender_ ? sender_paillier_ : receiver_paillier_);
    } else {
        compute_paillier_cipher(is_sender_ ? sender_paillier_ : receiver_paillier_);
    }
    LOG_IF(INFO, verbose_) << "encrypt features done.";

    for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
        permute_and_undo(
                (is_sender_ ? sender_permutation_ : receiver_permutation_), true, encrypted_features[feat_idx]);
    }
}

void DPCardinalityPSI::filter_intersection_features(const std::vector<std::vector<ByteVector>>& encrypted_features,
        std::size_t intersection_size, std::vector<std::vector<ByteVector>>& intersection_features) {
    if (encrypted_features.empty()) {
        return;
    }
    std::vector<std::pair<ByteVector, std::size_t>> intersection_keys;
    std::size_t feature_size = encrypted_features.size();
    std::size_t data_size = encrypted_features.empty() ? 0 : encrypted_features[0].size();
    intersection_keys.reserve(intersection_size);
    intersection_features.reserve(feature_size);

    // select intersection keys and intersection features.
    // intersection keys from different columns will be merged into one column as the final intersection set.
    std::size_t index_counter = 0;
    for (std::size_t item_idx = 0; item_idx < intersection_indices_.size(); ++item_idx) {
        if (intersection_indices_[item_idx].first) {
            intersection_keys.emplace_back(intersection_indices_[item_idx].second, index_counter++);
        }
    }

    std::vector<ByteVector> intersection_features_buffer;
    intersection_features_buffer.reserve(intersection_size);
    for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
        for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
            if (intersection_indices_[item_idx].first) {
                intersection_features_buffer.emplace_back(encrypted_features[feat_idx][item_idx]);
            }
        }
        intersection_features.emplace_back(intersection_features_buffer);
        intersection_features_buffer.clear();
    }

    // sort intersection keys and intersection features.
    std::sort(intersection_keys.begin(), intersection_keys.end());
    std::vector<std::size_t> sort_permutation;
    sort_permutation.reserve(intersection_size);
    for (std::size_t item_idx = 0; item_idx < intersection_size; ++item_idx) {
        sort_permutation.emplace_back(intersection_keys[item_idx].second);
    }

    for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
        permute_and_undo(sort_permutation, false, intersection_features[feat_idx]);
    }
}

// Generates additive shares for encrypted features through the Paillier encryption.
// Without packing:
//     Fileds: x in Z_{2^l}, r in Z_n
//     PartyA:
//         shares: -r mod n
//        decrypt and reveal: -r mod 2^l
//     PartyB:
//        shares: (x + r) mod n
//        decrypt and reveal: (x+r)mod 2^l
//
// With packing:
//     Fileds: x in Z_{2^l}
//     Constraint: r_0, r_1 in  Z_{2^(l+delta)},  r_1 || r_0 < n
//     PartyA:
//         shares: r_1 || r_0 mod n
//         decrypt and reveal: -r mod 2^l
//             x_0: - r_0 mod 2^l
//             x_1: -r_1 mod 2^l
//     PartyB:
//         shares: (x_1||x_0) + (r_1||r_0) = (x_1+r_1)||(x_0 + r_0) mod n
//         decrypt and reveal:
//             res = (((x_1||x_0) + (r_1||r_0)) mod n
//             x_0: res mod 2^l
//             x_1: (res >> (l+delta+1) mod 2^l.
void DPCardinalityPSI::generate_additive_shares(IpclPaillier& paillier,
        std::vector<std::vector<ByteVector>>& encrypted_features, std::vector<std::vector<BigNumber>>& random_r) {
    auto feature_size = encrypted_features.size();
    auto data_size = encrypted_features.empty() ? 0 : encrypted_features[0].size();
    BigNumber two_power_l(BigNumber::One());
    ipcl_bn_lshift(two_power_l, kValueBits);
    BigNumber n_minus_l = paillier.n() - two_power_l;
    std::size_t n_len = paillier.get_bytes_len(0);
    std::size_t raw_feature_size = is_sender_ ? receiver_feature_size_ : sender_feature_size_;

    std::size_t packing_capacity = apply_packing_ ? (n_len * 8 / slot_bits_) : 1;

    BigNumber mask_minus_l;
    if (apply_packing_) {
        BigNumber two_power_k_minus_one(BigNumber::One());
        ipcl_bn_lshift(two_power_k_minus_one, (slot_bits_ - 1));
        mask_minus_l = two_power_k_minus_one - two_power_l;
    } else {
        mask_minus_l = n_minus_l;
    }

    random_r.reserve(feature_size);
    std::vector<BigNumber> random_r_buffer;
    random_r_buffer.reserve(data_size);
    std::vector<BigNumber> encrypted_features_buffer;
    encrypted_features_buffer.reserve(data_size);

    // shifting-and-adding.
    if (apply_packing_) {
        std::size_t mask_bits = kValueBits + statistical_security_bits_;
        BigNumber bn_slot(BigNumber::One());
        ipcl_bn_lshift(bn_slot, slot_bits_);
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            std::size_t cur_packed_num = std::min(packing_capacity, raw_feature_size - feat_idx * packing_capacity);
            for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
                BigNumber r = two_power_l + (ipcl::getRandomBN(static_cast<int>(mask_bits)) % mask_minus_l);
                for (std::size_t pack_idx = 1; pack_idx < cur_packed_num; ++pack_idx) {
                    r *= bn_slot;
                    r += two_power_l + (ipcl::getRandomBN(static_cast<int>(mask_bits)) % mask_minus_l);
                }
                random_r_buffer.emplace_back(r);
                encrypted_features_buffer.emplace_back(paillier.decode(encrypted_features[feat_idx][item_idx]));
            }
            ipcl::PlainText plaintexts_r(random_r_buffer);
            ipcl::CipherText ciphertexts_encrypted_features(*paillier.get_pk(), encrypted_features_buffer);
            auto additive_share = paillier.add(ciphertexts_encrypted_features, plaintexts_r);
            for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
                encrypted_features[feat_idx][item_idx] = paillier.encode(additive_share.getElement(item_idx), true);
            }
            random_r.emplace_back(random_r_buffer);
            random_r_buffer.clear();
            encrypted_features_buffer.clear();
        }
    } else {
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
                BigNumber r = two_power_l + (ipcl::getRandomBN(static_cast<int>(n_len)) % n_minus_l);
                random_r_buffer.emplace_back(r);
                encrypted_features_buffer.emplace_back(paillier.decode(encrypted_features[feat_idx][item_idx]));
            }
            ipcl::PlainText plaintexts_r(random_r_buffer);
            ipcl::CipherText ciphertexts_encrypted_features(*paillier.get_pk(), encrypted_features_buffer);
            auto additive_share = paillier.add(ciphertexts_encrypted_features, plaintexts_r);
            for (std::size_t item_idx = 0; item_idx < data_size; ++item_idx) {
                encrypted_features[feat_idx][item_idx] = paillier.encode(additive_share.getElement(item_idx), true);
            }
            random_r.emplace_back(random_r_buffer);
            random_r_buffer.clear();
            encrypted_features_buffer.clear();
        }
    }
}

void DPCardinalityPSI::decrypt_and_reveal_shares(const std::vector<std::vector<ByteVector>>& encrypetd_shares,
        const std::vector<std::vector<BigNumber>>& random_r, std::size_t intersection_size,
        std::vector<std::vector<std::uint64_t>>& shares) {
    std::size_t total_feature_size = sender_feature_size_ + receiver_feature_size_;
    shares.reserve(total_feature_size);
    BigNumber modulus(BigNumber::One());
    ipcl_bn_lshift(modulus, kValueBits);

    auto compute_a = [&shares, &intersection_size, &random_r, &modulus](
                             const IpclPaillier& paillier, std::size_t feature_size) {
        std::vector<std::uint64_t> shares_buffer;
        shares_buffer.reserve(intersection_size);
        BigNumber n = paillier.n();
        BigNumber n_mod_modulus = n % modulus;
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            for (std::size_t item_idx = 0; item_idx < intersection_size; ++item_idx) {
                BigNumber a = (n - random_r[feat_idx][item_idx]) % modulus;
                a = (a + modulus - n_mod_modulus) % modulus;
                shares_buffer.emplace_back(ipcl_bn_2_u64(a));
            }
            shares.emplace_back(shares_buffer);
            shares_buffer.clear();
        }
    };

    auto compute_a_with_packing = [&shares, &intersection_size, &random_r, &modulus](std::size_t feature_size,
                                          std::size_t raw_feature_size, std::size_t packing_capacity,
                                          std::size_t slot_bits) {
        std::vector<std::vector<std::uint64_t>> shares_buffer;
        shares_buffer.resize(packing_capacity);
        for (std::size_t pack_idx = 0; pack_idx < packing_capacity; ++pack_idx) {
            shares_buffer[pack_idx].reserve(intersection_size);
        }
        BigNumber slot_modulus(BigNumber::One());
        ipcl_bn_lshift(slot_modulus, slot_bits);
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            std::size_t cur_packed_num = std::min(packing_capacity, raw_feature_size - feat_idx * packing_capacity);
            for (std::size_t item_idx = 0; item_idx < intersection_size; ++item_idx) {
                BigNumber r = random_r[feat_idx][item_idx];
                BigNumber a = (slot_modulus - (r % slot_modulus)) % modulus;
                // slot_modulus % modulus == 0
                shares_buffer[cur_packed_num - 1].emplace_back(ipcl_bn_2_u64(a));
                for (std::size_t pack_idx = 1; pack_idx < cur_packed_num; ++pack_idx) {
                    r /= slot_modulus;
                    a = (slot_modulus - (r % slot_modulus)) % modulus;
                    shares_buffer[cur_packed_num - 1 - pack_idx].emplace_back(ipcl_bn_2_u64(a));
                }
            }
            for (std::size_t pack_idx = 0; pack_idx < cur_packed_num; ++pack_idx) {
                shares.emplace_back(shares_buffer[pack_idx]);
                shares_buffer[pack_idx].clear();
            }
        }
    };

    auto compute_b = [&shares, &intersection_size, &encrypetd_shares, &modulus](
                             const IpclPaillier& paillier, std::size_t feature_size) {
        std::vector<std::uint64_t> shares_buffer;
        shares_buffer.reserve(intersection_size);
        std::vector<BigNumber> encrypetd_shares_buffer;
        encrypetd_shares_buffer.reserve(intersection_size);
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            for (std::size_t item_idx = 0; item_idx < intersection_size; ++item_idx) {
                encrypetd_shares_buffer.emplace_back(paillier.decode(encrypetd_shares[feat_idx][item_idx]));
            }
            ipcl::CipherText ciphertexts_shares(*paillier.get_pk(), encrypetd_shares_buffer);
            auto plaintexts_shares = paillier.decrypt(ciphertexts_shares);
            for (std::size_t item_idx = 0; item_idx < intersection_size; ++item_idx) {
                BigNumber b = plaintexts_shares.getElement(item_idx) % modulus;
                shares_buffer.emplace_back(ipcl_bn_2_u64(b));
            }
            shares.emplace_back(shares_buffer);
            shares_buffer.clear();
            encrypetd_shares_buffer.clear();
        }
    };

    auto compute_b_with_packing = [&shares, &intersection_size, &encrypetd_shares, &modulus](
                                          const IpclPaillier& paillier, std::size_t feature_size,
                                          std::size_t raw_feature_size, std::size_t packing_capacity,
                                          std::size_t slot_bits) {
        std::vector<std::vector<std::uint64_t>> shares_buffer;
        shares_buffer.resize(packing_capacity);
        for (std::size_t pack_idx = 0; pack_idx < packing_capacity; ++pack_idx) {
            shares_buffer[pack_idx].reserve(intersection_size);
        }
        std::vector<BigNumber> encrypetd_shares_buffer;
        encrypetd_shares_buffer.reserve(intersection_size);
        BigNumber slot_modulus(BigNumber::One());
        ipcl_bn_lshift(slot_modulus, slot_bits);
        for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
            std::size_t cur_packed_num = std::min(packing_capacity, raw_feature_size - feat_idx * packing_capacity);
            for (std::size_t item_idx = 0; item_idx < intersection_size; ++item_idx) {
                encrypetd_shares_buffer.emplace_back(paillier.decode(encrypetd_shares[feat_idx][item_idx]));
            }
            ipcl::CipherText ciphertexts_shares(*paillier.get_pk(), encrypetd_shares_buffer);
            auto plaintexts_shares = paillier.decrypt(ciphertexts_shares);

            for (std::size_t item_idx = 0; item_idx < intersection_size; ++item_idx) {
                BigNumber x_plus_r = plaintexts_shares.getElement(item_idx);
                BigNumber b = (x_plus_r % slot_modulus) % modulus;
                shares_buffer[cur_packed_num - 1].emplace_back(ipcl_bn_2_u64(b));
                for (std::size_t pack_idx = 1; pack_idx < cur_packed_num; ++pack_idx) {
                    x_plus_r /= slot_modulus;
                    b = (x_plus_r % slot_modulus) % modulus;
                    shares_buffer[cur_packed_num - 1 - pack_idx].emplace_back(ipcl_bn_2_u64(b));
                }
            }
            for (std::size_t pack_idx = 0; pack_idx < cur_packed_num; ++pack_idx) {
                shares.emplace_back(shares_buffer[pack_idx]);
                shares_buffer[pack_idx].clear();
            }
        }
    };

    if (apply_packing_) {
        std::size_t sender_packing_capacity = sender_paillier_.get_bytes_len(0) * 8 / slot_bits_;
        std::size_t receiver_packing_capacity = receiver_paillier_.get_bytes_len(0) * 8 / slot_bits_;
        if (is_sender_) {
            compute_b_with_packing(sender_paillier_, encrypetd_shares.size(), sender_feature_size_,
                    sender_packing_capacity, slot_bits_);
            compute_a_with_packing(random_r.size(), receiver_feature_size_, receiver_packing_capacity, slot_bits_);
        } else {
            compute_a_with_packing(random_r.size(), sender_feature_size_, sender_packing_capacity, slot_bits_);
            compute_b_with_packing(receiver_paillier_, encrypetd_shares.size(), receiver_feature_size_,
                    receiver_packing_capacity, slot_bits_);
        }
    } else {
        if (is_sender_) {
            compute_b(sender_paillier_, sender_feature_size_);
            compute_a(receiver_paillier_, receiver_feature_size_);
        } else {
            compute_a(sender_paillier_, sender_feature_size_);
            compute_b(receiver_paillier_, receiver_feature_size_);
        }
    }
}

void DPCardinalityPSI::exchange_encrypted_keys(const std::vector<std::vector<ByteVector>>& encrypted_keys,
        std::size_t key_size, std::size_t received_data_size, std::vector<std::vector<ByteVector>>& received_keys,
        std::size_t point_len) {
    std::size_t self_data_size = encrypted_keys[0].size();
    if (is_sender_) {
        ByteVector encrypted_keys_buffer;
        encrypted_keys_buffer.reserve(self_data_size * point_len);
        for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
            for (const auto& key : encrypted_keys[key_idx]) {
                encrypted_keys_buffer.insert(encrypted_keys_buffer.end(), key.begin(), key.end());
            }
            io_->send_bytes(encrypted_keys_buffer);
            encrypted_keys_buffer.clear();
        }
        LOG_IF(INFO, verbose_) << "sender sent encryptd keys.";

        received_keys.reserve(key_size);

        ByteVector received_keys_buffer;
        received_keys_buffer.reserve(received_data_size * point_len);

        std::vector<ByteVector> received_keys_i;
        received_keys_i.reserve(received_data_size);

        for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
            io_->recv_bytes(received_keys_buffer);
            for (std::size_t item_idx = 0; item_idx < received_data_size; ++item_idx) {
                received_keys_i.emplace_back(received_keys_buffer.begin() + item_idx * point_len,
                        received_keys_buffer.begin() + (item_idx + 1) * point_len);
            }
            received_keys_buffer.clear();
            received_keys.emplace_back(received_keys_i);
            received_keys_i.clear();
        }
        LOG_IF(INFO, verbose_) << "sender received encryptd keys.";
    } else {
        received_keys.reserve(key_size);

        ByteVector received_keys_buffer;
        received_keys_buffer.reserve(received_data_size * point_len);

        std::vector<ByteVector> received_keys_i;
        received_keys_i.reserve(received_data_size);

        for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
            io_->recv_bytes(received_keys_buffer);
            for (std::size_t item_idx = 0; item_idx < received_data_size; ++item_idx) {
                received_keys_i.emplace_back(received_keys_buffer.begin() + item_idx * point_len,
                        received_keys_buffer.begin() + (item_idx + 1) * point_len);
            }
            received_keys_buffer.clear();
            received_keys.emplace_back(received_keys_i);
            received_keys_i.clear();
        }
        LOG_IF(INFO, verbose_) << "receiver received encryptd keys.";

        ByteVector encrypted_keys_buffer;
        encrypted_keys_buffer.reserve(self_data_size * point_len);
        for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
            for (const auto& key : encrypted_keys[key_idx]) {
                encrypted_keys_buffer.insert(encrypted_keys_buffer.end(), key.begin(), key.end());
            }
            io_->send_bytes(encrypted_keys_buffer);
            encrypted_keys_buffer.clear();
        }
        LOG_IF(INFO, verbose_) << "receiver sent encryptd keys.";
    }
}

void DPCardinalityPSI::exchange_single_encrypted_keys(const std::vector<ByteVector>& encrypted_keys,
        std::size_t received_data_size, std::vector<ByteVector>& received_keys, std::size_t point_len) {
    std::size_t self_data_size = encrypted_keys[0].size();
    if (is_sender_) {
        ByteVector encrypted_keys_buffer;
        encrypted_keys_buffer.reserve(self_data_size * point_len);
        for (const auto& key : encrypted_keys) {
            encrypted_keys_buffer.insert(encrypted_keys_buffer.end(), key.begin(), key.end());
        }
        io_->send_bytes(encrypted_keys_buffer);
        encrypted_keys_buffer.clear();
        LOG_IF(INFO, verbose_) << "sender sent single column's encryptd keys.";

        ByteVector received_keys_buffer;
        received_keys_buffer.reserve(received_data_size * point_len);

        received_keys.reserve(received_data_size);

        io_->recv_bytes(received_keys_buffer);
        for (std::size_t item_idx = 0; item_idx < received_data_size; ++item_idx) {
            received_keys.emplace_back(received_keys_buffer.begin() + item_idx * point_len,
                    received_keys_buffer.begin() + (item_idx + 1) * point_len);
        }
        received_keys_buffer.clear();
        LOG_IF(INFO, verbose_) << "sender received single column's encryptd keys.";
    } else {
        ByteVector received_keys_buffer;
        received_keys_buffer.reserve(received_data_size * point_len);

        received_keys.reserve(received_data_size);

        io_->recv_bytes(received_keys_buffer);
        for (std::size_t item_idx = 0; item_idx < received_data_size; ++item_idx) {
            received_keys.emplace_back(received_keys_buffer.begin() + item_idx * point_len,
                    received_keys_buffer.begin() + (item_idx + 1) * point_len);
        }
        received_keys_buffer.clear();
        LOG_IF(INFO, verbose_) << "receiver received single column's encryptd keys.";

        ByteVector encrypted_keys_buffer;
        encrypted_keys_buffer.reserve(self_data_size * point_len);
        for (const auto& key : encrypted_keys) {
            encrypted_keys_buffer.insert(encrypted_keys_buffer.end(), key.begin(), key.end());
        }
        io_->send_bytes(encrypted_keys_buffer);
        encrypted_keys_buffer.clear();
        LOG_IF(INFO, verbose_) << "receiver sent single column's encryptd keys.";
    }
}

void DPCardinalityPSI::exchange_encrypted_features(const std::vector<std::vector<ByteVector>>& encrypted_features,
        std::size_t self_paillier_len, std::size_t remote_paillier_len, std::size_t received_feature_size,
        std::size_t received_data_size, std::vector<std::vector<ByteVector>>& received_features) {
    std::size_t self_feature_size = encrypted_features.size();
    std::size_t self_data_size = encrypted_features.empty() ? 0 : encrypted_features[0].size();
    if (is_sender_) {
        ByteVector encrypted_features_buffer;
        encrypted_features_buffer.reserve(self_data_size * self_paillier_len);
        for (std::size_t feat_idx = 0; feat_idx < self_feature_size; ++feat_idx) {
            for (const auto& feature : encrypted_features[feat_idx]) {
                encrypted_features_buffer.insert(encrypted_features_buffer.end(), feature.begin(), feature.end());
            }
            io_->send_bytes(encrypted_features_buffer);
            encrypted_features_buffer.clear();
        }

        received_features.reserve(received_feature_size);

        ByteVector received_features_buffer;
        received_features_buffer.reserve(received_data_size * remote_paillier_len);

        std::vector<ByteVector> received_features_i;
        received_features_i.reserve(received_data_size);

        for (std::size_t feat_idx = 0; feat_idx < received_feature_size; ++feat_idx) {
            io_->recv_bytes(received_features_buffer);
            for (std::size_t item_idx = 0; item_idx < received_data_size; ++item_idx) {
                received_features_i.emplace_back(received_features_buffer.begin() + item_idx * remote_paillier_len,
                        received_features_buffer.begin() + (item_idx + 1) * remote_paillier_len);
            }
            received_features_buffer.clear();
            received_features.emplace_back(received_features_i);
            received_features_i.clear();
        }
    } else {
        received_features.reserve(received_data_size);

        ByteVector received_features_buffer;
        received_features_buffer.reserve(received_data_size * remote_paillier_len);

        std::vector<ByteVector> received_features_i;
        received_features_i.reserve(received_data_size);

        for (std::size_t feat_idx = 0; feat_idx < received_feature_size; ++feat_idx) {
            io_->recv_bytes(received_features_buffer);
            for (std::size_t item_idx = 0; item_idx < received_data_size; ++item_idx) {
                received_features_i.emplace_back(received_features_buffer.begin() + item_idx * remote_paillier_len,
                        received_features_buffer.begin() + (item_idx + 1) * remote_paillier_len);
            }
            received_features_buffer.clear();
            received_features.emplace_back(received_features_i);
            received_features_i.clear();
        }

        ByteVector encrypted_features_buffer;
        encrypted_features_buffer.reserve(self_data_size * self_paillier_len);
        for (std::size_t feat_idx = 0; feat_idx < self_feature_size; ++feat_idx) {
            for (const auto& feature : encrypted_features[feat_idx]) {
                encrypted_features_buffer.insert(encrypted_features_buffer.end(), feature.begin(), feature.end());
            }
            io_->send_bytes(encrypted_features_buffer);
            encrypted_features_buffer.clear();
        }
    }
}

void DPCardinalityPSI::reset_data() {
    sender_data_size_ = 0;
    sender_feature_size_ = 0;
    receiver_data_size_ = 0;
    receiver_feature_size_ = 0;
    for (auto& keys : plaintext_keys_) {
        keys.clear();
    }
    plaintext_keys_.clear();
    for (auto& features : plaintext_features_) {
        features.clear();
    }
    plaintext_features_.clear();
    sender_permutation_.clear();
    receiver_permutation_.clear();
    exchanged_keys_.clear();
    intersection_indices_.resize(0);
}

}  // namespace dpca_psi
}  // namespace privacy_go
