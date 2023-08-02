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

#include "ppam/ppam.h"

#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "dpca-psi/common/defines.h"
#include "dpca-psi/common/dummy_data_utils.h"
#include "dpca-psi/common/utils.h"
#include "dpca-psi/crypto/prng.h"
#include "dpca-psi/network/two_channel_net_io.h"

namespace privacy_go {
namespace ppam {

using json = nlohmann::json;

class PPAMTest : public ::testing::Test {
public:
    void SetUp() {
        sender_params_ = R"({
            "common": {
                "address": "127.0.0.1",
                "remote_port": 30330,
                "local_port": 30331,
                "timeout": 90,
                "input_file": "example/data/sender_input_file.csv",
                "has_header": false,
                "output_file": "example/data/sender_output_file.csv",
                "ids_num": 2,
                "is_sender": true,
                "verbose": false
            },
            "paillier_params": {
                "paillier_n_len": 2048,
                "enable_djn": false,
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

        auto receiver_params = R"({
            "common": {
                "address": "127.0.0.1",
                "remote_port": 30331,
                "local_port": 30330,
                "input_file": "example/receiver_input_file.csv",
                "output_file": "example/receiver_output_file.csv",
                "is_sender": false
            },
            "dp_params": {
                "has_zero_column": true,
                "zero_column_index": -1
            }
        })"_json;
        receiver_params_ = sender_params_;
        receiver_params_.merge_patch(receiver_params);

        sender_params_with_verbose_ = sender_params_;
        receiver_params_with_verbose_ = receiver_params_;
        sender_params_with_verbose_["common"]["verbose"] = true;
        receiver_params_with_verbose_["common"]["verbose"] = true;
        sender_params_without_dp_ = sender_params_;
        receiver_params_without_dp_ = receiver_params_;
        sender_params_without_dp_["dp_params"]["input_dp"] = false;
        receiver_params_without_dp_["dp_params"]["input_dp"] = false;
        sender_params_without_packing_ = sender_params_;
        receiver_params_without_packing_ = receiver_params_;
        sender_params_without_packing_["paillier_params"]["apply_packing"] = false;
        receiver_params_without_packing_["paillier_params"]["apply_packing"] = false;
        sender_params_without_precomputed_tau_ = sender_params_;
        receiver_params_without_precomputed_tau_ = receiver_params_;
        sender_params_without_precomputed_tau_["dp_params"]["use_precomputed_tau"] = false;
        receiver_params_without_precomputed_tau_["dp_params"]["use_precomputed_tau"] = false;
        sender_params_without_djn_ = sender_params_;
        receiver_params_without_djn_ = receiver_params_;
        sender_params_without_djn_["paillier_params"]["enable_djn"] = false;
        receiver_params_without_djn_["paillier_params"]["enable_djn"] = false;
    }

    std::vector<double> random_features(std::size_t n, std::size_t min, std::size_t max, bool is_zero) {
        std::vector<double> result(n, 0.0);
        std::random_device rd("/dev/urandom");
        std::default_random_engine eng(rd());
        std::uniform_real_distribution<double> distr(static_cast<double>(min), static_cast<double>(max));
        if (!is_zero) {
            for (std::size_t i = 0; i < n; ++i) {
                result[i] = distr(eng) - (static_cast<double>(max) - static_cast<double>(min)) / 2.0;
            }
        }
        return result;
    }

    void ppam_default(const json& params) {
        bool is_sender = params["common"]["is_sender"];
        std::string address = params["common"]["address"];
        std::uint16_t remote_port = params["common"]["remote_port"];
        std::uint16_t local_port = params["common"]["local_port"];
        auto net = std::make_shared<dpca_psi::TwoChannelNetIO>(address, remote_port, local_port);

        PrivacyMeasurement ads_measure;
        ads_measure.initialize(params, net);
        if (is_sender) {
            mpc_result = ads_measure.measurement(5.0, default_sender_keys_, default_sender_features_);
            plain_result = ads_measure.plain_measurement(5.0, default_sender_keys_, default_sender_features_);
        } else {
            mpc_result = ads_measure.measurement(5.0, default_receiver_keys_, default_receiver_features_);
            plain_result = ads_measure.plain_measurement(5.0, default_receiver_keys_, default_receiver_features_);
        }
    }

    void ppam_random(const json& params, std::size_t intersection_size, std::size_t feature_size) {
        std::size_t data_size = 10 * intersection_size;
        std::size_t key_size = params["common"]["ids_num"];

        dpca_psi::PRNG common_prng;
        common_prng.set_seed(dpca_psi::kZeroBlock);
        dpca_psi::PRNG unique_prng;
        unique_prng.set_seed(dpca_psi::read_block_from_dev_urandom());

        std::vector<std::vector<std::string>> keys;
        keys.reserve(key_size);

        std::size_t column_intersection_size = (intersection_size + key_size - 1) / key_size;
        for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
            std::size_t cur_intersection_size =
                    std::min(column_intersection_size, intersection_size - key_idx * column_intersection_size);
            auto common_keys = dpca_psi::random_keys(common_prng, cur_intersection_size, std::to_string(key_idx));
            auto unique_keys =
                    dpca_psi::random_keys(unique_prng, data_size - cur_intersection_size, std::to_string(key_idx));
            unique_keys.insert(
                    unique_keys.begin() + key_idx * column_intersection_size, common_keys.begin(), common_keys.end());
            keys.emplace_back(unique_keys);
        }
        std::vector<std::vector<double>> features;
        features.reserve(feature_size);
        for (std::size_t i = 0; i < feature_size; ++i) {
            features.emplace_back(random_features(data_size, 5, 1, false));
        }

        std::string address = params["common"]["address"];
        std::uint16_t remote_port = params["common"]["remote_port"];
        std::uint16_t local_port = params["common"]["local_port"];
        auto net = std::make_shared<dpca_psi::TwoChannelNetIO>(address, remote_port, local_port);

        PrivacyMeasurement ads_measure;
        ads_measure.initialize(params, net);
        mpc_result = ads_measure.measurement(5.0, keys, features);
        plain_result = ads_measure.plain_measurement(5.0, keys, features);
    }

public:
    json sender_params_;
    json receiver_params_;
    json sender_params_with_verbose_;
    json receiver_params_with_verbose_;
    json sender_params_without_dp_;
    json receiver_params_without_dp_;
    json sender_params_without_packing_;
    json receiver_params_without_packing_;
    json sender_params_without_precomputed_tau_;
    json receiver_params_without_precomputed_tau_;
    json sender_params_without_djn_;
    json receiver_params_without_djn_;
    std::thread t_[2];

    double plain_result;
    double mpc_result;

    std::vector<std::vector<std::string>> default_sender_keys_ = {
            {"c", "h", "e", "g", "y", "z"}, {"*", "#", "&", "@", "%", "!"}};
    std::vector<std::vector<double>> default_sender_features_ = {{0.1, 2.0, 0.03, 4, 0.5, 0.6}};
    std::vector<std::vector<std::string>> default_receiver_keys_ = {{"b", "c", "e", "g"}, {"#", "*", "&", "!"}};
    std::vector<std::vector<double>> default_receiver_features_ = {{0.1, 2, 0.3, 4}, {0.1, 2, 0.3, 4}};
};

TEST_F(PPAMTest, default_test) {
    t_[0] = std::thread([this]() { ppam_default(sender_params_); });
    t_[1] = std::thread([this]() { ppam_default(receiver_params_); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, default_without_dp) {
    t_[0] = std::thread([this]() { ppam_default(sender_params_without_dp_); });
    t_[1] = std::thread([this]() { ppam_default(receiver_params_without_dp_); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, default_with_verbose) {
    t_[0] = std::thread([this]() { ppam_default(sender_params_with_verbose_); });
    t_[1] = std::thread([this]() { ppam_default(receiver_params_with_verbose_); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, default_without_packing) {
    t_[0] = std::thread([this]() { ppam_default(sender_params_without_packing_); });
    t_[1] = std::thread([this]() { ppam_default(receiver_params_without_packing_); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, default_without_djn) {
    t_[0] = std::thread([this]() { ppam_default(sender_params_without_djn_); });
    t_[1] = std::thread([this]() { ppam_default(receiver_params_without_djn_); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, random_test) {
    t_[0] = std::thread([this]() { ppam_random(sender_params_, 5, 1); });
    t_[1] = std::thread([this]() { ppam_random(receiver_params_, 5, 2); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, random_without_dp) {
    t_[0] = std::thread([this]() { ppam_random(sender_params_without_dp_, 5, 1); });
    t_[1] = std::thread([this]() { ppam_random(receiver_params_without_dp_, 5, 2); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, random_with_verbose) {
    t_[0] = std::thread([this]() { ppam_random(sender_params_with_verbose_, 5, 1); });
    t_[1] = std::thread([this]() { ppam_random(receiver_params_with_verbose_, 5, 2); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, random_without_packing) {
    t_[0] = std::thread([this]() { ppam_random(sender_params_without_packing_, 5, 1); });
    t_[1] = std::thread([this]() { ppam_random(receiver_params_without_packing_, 5, 2); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

TEST_F(PPAMTest, random_without_djn) {
    t_[0] = std::thread([this]() { ppam_random(sender_params_without_djn_, 5, 1); });
    t_[1] = std::thread([this]() { ppam_random(receiver_params_without_djn_, 5, 2); });

    t_[0].join();
    t_[1].join();

    ASSERT_TRUE(fabs(mpc_result - plain_result) < 0.001);
}

}  // namespace ppam
}  // namespace privacy_go
