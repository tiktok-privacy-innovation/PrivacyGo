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

#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"
#include "dpca-psi/common/dummy_data_utils.h"
#include "dpca-psi/crypto/prng.h"
#include "dpca-psi/network/net_io_channel.h"

namespace dpca_psi {
using json = nlohmann::json;

class DPCAPSITest : public ::testing::Test {
public:
    void SetUp() {
        sender_params_ = R"({
            "common": {
                "address": "127.0.0.1",
                "port": 30331,
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
                "input_file": "example/receiver_input_file.csv",
                "output_file": "example/receiver_output_file.csv",
                "is_sender": false,
                "port": 30331
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

    void dpca_psi_default(const json& params, int idx) {
        bool is_sender = params["common"]["is_sender"];
        std::string address = params["common"]["address"];
        std::size_t port = params["common"]["port"];
        auto net = std::make_shared<NetIO>(is_sender ? nullptr : address.c_str(), port);
        DPCardinalityPSI psi;
        psi.init(params, net);
        if (is_sender) {
            psi.data_sampling(default_sender_keys_, default_sender_features_);
        } else {
            psi.data_sampling(default_receiver_keys_, default_receiver_features_);
        }
        if (idx == 0) {
            shares_0_ = psi.process();
        }
        if (idx == 1) {
            shares_1_ = psi.process();
        }
    }

    std::uint64_t dpca_psi_random(const json& params, std::size_t intersection_size, std::size_t feature_size,
            std::vector<std::vector<std::uint64_t>>& shares) {
        std::size_t data_size = 10 * intersection_size;
        std::size_t key_size = params["common"]["ids_num"];

        dpca_psi::PRNG common_prng;
        common_prng.set_seed(kZeroBlock);
        dpca_psi::PRNG unique_prng;
        unique_prng.set_seed(read_block_from_dev_urandom());

        std::vector<std::vector<std::string>> keys;
        keys.reserve(key_size);

        std::size_t column_intersection_size = (intersection_size + key_size - 1) / key_size;
        for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
            std::size_t cur_intersection_size =
                    std::min(column_intersection_size, intersection_size - key_idx * column_intersection_size);
            auto common_keys = random_keys(common_prng, cur_intersection_size, std::to_string(key_idx));
            auto unique_keys = random_keys(unique_prng, data_size - cur_intersection_size, std::to_string(key_idx));
            unique_keys.insert(
                    unique_keys.begin() + key_idx * column_intersection_size, common_keys.begin(), common_keys.end());
            keys.emplace_back(unique_keys);
        }
        std::vector<std::vector<std::uint64_t>> features;
        features.reserve(feature_size);
        for (std::size_t i = 0; i < feature_size; ++i) {
            features.emplace_back(random_features(unique_prng, data_size, false));
        }

        bool is_sender = params["common"]["is_sender"];
        std::string address = params["common"]["address"];
        std::size_t port = params["common"]["port"];
        auto net = std::make_shared<NetIO>(is_sender ? nullptr : address.c_str(), port);
        DPCardinalityPSI psi;
        psi.init(params, net);
        psi.data_sampling(keys, features);
        shares = psi.process();

        std::uint64_t sum = 0;
        if (!is_sender) {
            auto idx = features.size() - 1;
            for (std::size_t item_idx = 0; item_idx < intersection_size; ++item_idx) {
                sum += features[idx][item_idx];
            }
        }
        return sum;
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

    std::vector<std::vector<std::uint64_t>> shares_0_;
    std::vector<std::vector<std::uint64_t>> shares_1_;

    std::vector<std::vector<std::string>> default_sender_keys_ = {
            {"c", "h", "e", "g", "y", "z"}, {"*", "#", "&", "@", "%", "!"}};
    std::vector<std::vector<std::uint64_t>> default_sender_features_ = {{1, 2, 3, 4, 5, 6}};
    std::vector<std::vector<std::string>> default_receiver_keys_ = {{"b", "c", "e", "g"}, {"#", "*", "&", "!"}};
    std::vector<std::vector<std::uint64_t>> default_receiver_features_ = {{1, 2, 3, 4}, {1, 2, 3, 4}};
    std::uint64_t default_expected_sum_ = 10;
    std::vector<std::vector<std::uint64_t>> default_expected_results_ = {{1, 2, 3, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}};
};

TEST_F(DPCAPSITest, default_test) {
    t_[0] = std::thread([this]() { dpca_psi_default(sender_params_, 0); });
    t_[1] = std::thread([this]() { dpca_psi_default(receiver_params_, 1); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0_.size(), shares_1_.size());
    EXPECT_EQ(shares_0_[0].size(), shares_1_[0].size());
    std::size_t idx = shares_0_.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0_[idx].size(); ++j) {
        actual_result += shares_0_[idx][j] + shares_1_[idx][j];
    }
    EXPECT_EQ(actual_result, default_expected_sum_);
}

TEST_F(DPCAPSITest, default_without_dp) {
    t_[0] = std::thread([this]() { dpca_psi_default(sender_params_without_dp_, 0); });
    t_[1] = std::thread([this]() { dpca_psi_default(receiver_params_without_dp_, 1); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0_.size(), shares_1_.size());
    EXPECT_EQ(shares_0_[0].size(), shares_1_[0].size());

    std::size_t idx = shares_0_.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0_[idx].size(); ++j) {
        actual_result += shares_0_[idx][j] + shares_1_[idx][j];
    }
    EXPECT_EQ(actual_result, default_expected_sum_);
}

TEST_F(DPCAPSITest, default_with_verbose) {
    t_[0] = std::thread([this]() { dpca_psi_default(sender_params_with_verbose_, 0); });
    t_[1] = std::thread([this]() { dpca_psi_default(receiver_params_with_verbose_, 1); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0_.size(), shares_1_.size());
    EXPECT_EQ(shares_0_[0].size(), shares_1_[0].size());

    std::size_t idx = shares_0_.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0_[idx].size(); ++j) {
        actual_result += shares_0_[idx][j] + shares_1_[idx][j];
    }
    EXPECT_EQ(actual_result, default_expected_sum_);
}

TEST_F(DPCAPSITest, default_without_packing) {
    t_[0] = std::thread([this]() { dpca_psi_default(sender_params_without_packing_, 0); });
    t_[1] = std::thread([this]() { dpca_psi_default(receiver_params_without_packing_, 1); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0_.size(), shares_1_.size());
    EXPECT_EQ(shares_0_[0].size(), shares_1_[0].size());

    std::size_t idx = shares_0_.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0_[idx].size(); ++j) {
        actual_result += shares_0_[idx][j] + shares_1_[idx][j];
    }
    EXPECT_EQ(actual_result, default_expected_sum_);
}

TEST_F(DPCAPSITest, default_without_djn) {
    t_[0] = std::thread([this]() { dpca_psi_default(sender_params_without_djn_, 0); });
    t_[1] = std::thread([this]() { dpca_psi_default(receiver_params_without_djn_, 1); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0_.size(), shares_1_.size());
    EXPECT_EQ(shares_0_[0].size(), shares_1_[0].size());
    std::size_t idx = shares_0_.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0_[idx].size(); ++j) {
        actual_result += shares_0_[idx][j] + shares_1_[idx][j];
    }
    EXPECT_EQ(actual_result, default_expected_sum_);
}

TEST_F(DPCAPSITest, random_test) {
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    std::uint64_t expected_sum_0 = 0;
    std::uint64_t expected_sum_1 = 0;
    t_[0] = std::thread(
            [this, &shares_0, &expected_sum_0]() { expected_sum_0 = dpca_psi_random(sender_params_, 5, 1, shares_0); });
    t_[1] = std::thread([this, &shares_1, &expected_sum_1]() {
        expected_sum_1 = dpca_psi_random(receiver_params_, 5, 2, shares_1);
    });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0.size(), shares_1.size());
    EXPECT_EQ(shares_0[0].size(), shares_1[0].size());

    std::size_t idx = shares_0.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0[idx].size(); ++j) {
        actual_result += shares_0[idx][j] + shares_1[idx][j];
    }
    EXPECT_EQ(actual_result, expected_sum_1);
}

TEST_F(DPCAPSITest, random_without_dp) {
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    std::uint64_t expected_sum_0 = 0;
    std::uint64_t expected_sum_1 = 0;
    t_[0] = std::thread([this, &shares_0, &expected_sum_0]() {
        expected_sum_0 = dpca_psi_random(sender_params_without_dp_, 5, 1, shares_0);
    });
    t_[1] = std::thread([this, &shares_1, &expected_sum_1]() {
        expected_sum_1 = dpca_psi_random(receiver_params_without_dp_, 5, 2, shares_1);
    });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0.size(), shares_1.size());
    EXPECT_EQ(shares_0[0].size(), shares_1[0].size());
    EXPECT_EQ(shares_0[0].size(), 5);

    std::size_t idx = shares_0.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0[idx].size(); ++j) {
        actual_result += shares_0[idx][j] + shares_1[idx][j];
    }
    EXPECT_EQ(actual_result, expected_sum_1);
}

TEST_F(DPCAPSITest, random_with_verbose) {
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    std::uint64_t expected_sum_0 = 0;
    std::uint64_t expected_sum_1 = 0;
    t_[0] = std::thread([this, &shares_0, &expected_sum_0]() {
        expected_sum_0 = dpca_psi_random(sender_params_with_verbose_, 5, 1, shares_0);
    });
    t_[1] = std::thread([this, &shares_1, &expected_sum_1]() {
        expected_sum_1 = dpca_psi_random(receiver_params_with_verbose_, 5, 2, shares_1);
    });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0.size(), shares_1.size());
    EXPECT_EQ(shares_0[0].size(), shares_1[0].size());

    std::size_t idx = shares_0.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0[idx].size(); ++j) {
        actual_result += shares_0[idx][j] + shares_1[idx][j];
    }
    EXPECT_EQ(actual_result, expected_sum_1);
}

TEST_F(DPCAPSITest, random_without_packing) {
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    std::uint64_t expected_sum_0 = 0;
    std::uint64_t expected_sum_1 = 0;
    t_[0] = std::thread([this, &shares_0, &expected_sum_0]() {
        expected_sum_0 = dpca_psi_random(sender_params_without_packing_, 5, 1, shares_0);
    });
    t_[1] = std::thread([this, &shares_1, &expected_sum_1]() {
        expected_sum_1 = dpca_psi_random(receiver_params_without_packing_, 5, 2, shares_1);
    });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0.size(), shares_1.size());
    EXPECT_EQ(shares_0[0].size(), shares_1[0].size());

    std::size_t idx = shares_0.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0[idx].size(); ++j) {
        actual_result += shares_0[idx][j] + shares_1[idx][j];
    }
    EXPECT_EQ(actual_result, expected_sum_1);
}

TEST_F(DPCAPSITest, random_without_djn) {
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    std::uint64_t expected_sum_0 = 0;
    std::uint64_t expected_sum_1 = 0;
    t_[0] = std::thread([this, &shares_0, &expected_sum_0]() {
        expected_sum_0 = dpca_psi_random(sender_params_without_djn_, 5, 1, shares_0);
    });
    t_[1] = std::thread([this, &shares_1, &expected_sum_1]() {
        expected_sum_1 = dpca_psi_random(receiver_params_without_djn_, 5, 2, shares_1);
    });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(shares_0.size(), shares_1.size());
    EXPECT_EQ(shares_0[0].size(), shares_1[0].size());

    std::size_t idx = shares_0.size() - 1;
    std::uint64_t actual_result = 0;
    for (std::size_t j = 0; j < shares_0[idx].size(); ++j) {
        actual_result += shares_0[idx][j] + shares_1[idx][j];
    }
    EXPECT_EQ(actual_result, expected_sum_1);
}

TEST_F(DPCAPSITest, inconsistent_curve_id) {
    json receiver_invalid_params = receiver_params_without_dp_;
    receiver_invalid_params["ecc_params"]["curve_id"] = 414;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0]() {
        EXPECT_THROW(dpca_psi_random(sender_params_without_dp_, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, unexpected_ecurve_id) {
    json receiver_invalid_params = receiver_params_without_dp_;
    json sender_invalid_params = sender_params_without_dp_;
    receiver_invalid_params["ecc_params"]["curve_id"] = 414;
    sender_invalid_params["ecc_params"]["curve_id"] = 416;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0, &sender_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(sender_invalid_params, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, inconsistent_input_dp) {
    json receiver_invalid_params = receiver_params_without_dp_;
    receiver_invalid_params["dp_params"]["input_dp"] = true;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0]() {
        EXPECT_THROW(dpca_psi_random(sender_params_without_dp_, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, inconsistent_epsilon) {
    json receiver_invalid_params = receiver_params_without_precomputed_tau_;
    receiver_invalid_params["dp_params"]["epsilon"] = 3.0;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0]() {
        EXPECT_THROW(dpca_psi_random(sender_params_without_precomputed_tau_, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, inconsistent_maximum_queries) {
    json receiver_invalid_params = receiver_params_without_precomputed_tau_;
    receiver_invalid_params["dp_params"]["maximum_queries"] = 40;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0]() {
        EXPECT_THROW(dpca_psi_random(sender_params_without_precomputed_tau_, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, inconsistent_ids_num) {
    json receiver_invalid_params = receiver_params_;
    receiver_invalid_params["common"]["ids_num"] = 4;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0]() {
        EXPECT_THROW(dpca_psi_random(sender_params_, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, unexpected_ids_num) {
    json receiver_invalid_params = receiver_params_;
    json sender_invalid_params = sender_params_;
    receiver_invalid_params["common"]["ids_num"] = 0;
    sender_invalid_params["common"]["ids_num"] = 120;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0, &sender_invalid_params]() {
        EXPECT_THROW(dpca_psi_default(sender_invalid_params, 0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_default(receiver_invalid_params, 0), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}


TEST_F(DPCAPSITest, inconsistent_apply_packing) {
    json receiver_invalid_params = receiver_params_;
    receiver_invalid_params["paillier_params"]["apply_packing"] = false;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0]() {
        EXPECT_THROW(dpca_psi_random(sender_params_, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, inconsistent_statistical_security) {
    json receiver_invalid_params = receiver_params_;
    receiver_invalid_params["paillier_params"]["statistical_security_bits"] = 80;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0]() {
        EXPECT_THROW(dpca_psi_random(sender_params_, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, unexpected_statistical_security) {
    json receiver_invalid_params = receiver_params_;
    json sender_invalid_params = sender_params_;
    receiver_invalid_params["paillier_params"]["statistical_security_bits"] = 0;
    sender_invalid_params["paillier_params"]["statistical_security_bits"] = 65536;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0, &sender_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(sender_invalid_params, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, unexpected_paillier_n_len) {
    json receiver_invalid_params = receiver_params_;
    json sender_invalid_params = sender_params_;
    receiver_invalid_params["paillier_params"]["paillier_n_len"] = 100;
    sender_invalid_params["paillier_params"]["paillier_n_len"] = 1525;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0, &sender_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(sender_invalid_params, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, inconsistent_use_precomputed_tau) {
    json receiver_invalid_params = receiver_params_;
    receiver_invalid_params["dp_params"]["use_precomputed_tau"] = false;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0]() {
        EXPECT_THROW(dpca_psi_random(sender_params_, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

TEST_F(DPCAPSITest, unexpected_precomputed_tau) {
    json receiver_invalid_params = receiver_params_;
    json sender_invalid_params = sender_params_;
    receiver_invalid_params["dp_params"]["precomputed_tau"] = 1ull << 26;
    sender_invalid_params["dp_params"]["precomputed_tau"] = 1ull << 24;
    std::vector<std::vector<std::uint64_t>> shares_0;
    std::vector<std::vector<std::uint64_t>> shares_1;

    t_[0] = std::thread([this, &shares_0, &sender_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(sender_invalid_params, 1, 1, shares_0), std::invalid_argument);
    });
    t_[1] = std::thread([this, &shares_1, &receiver_invalid_params]() {
        EXPECT_THROW(dpca_psi_random(receiver_invalid_params, 1, 2, shares_1), std::invalid_argument);
    });

    t_[0].join();
    t_[1].join();
}

}  // namespace dpca_psi
