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

#include "dpca-psi/crypto/dp_sampling.h"

#include <algorithm>

#include "gtest/gtest.h"

#include "dpca-psi/common/utils.h"

namespace privacy_go {
namespace dpca_psi {

class DPSamplingTest : public ::testing::Test {
public:
    const std::size_t single_ids_num_ = 1;
    const std::size_t multiple_ids_num_ = 3;
    static void SetUpTestCase() {
    }
};

TEST_F(DPSamplingTest, sample_with_zero) {
    DPSampling sample1;
    DPSampling sample2;
    bool use_precomputed_tau = true;
    std::size_t precomputed_tau = 722;
    auto sampled_result1 =
            sample1.multi_key_sampling(single_ids_num_, 2, 1, true, use_precomputed_tau, precomputed_tau);
    auto sampled_result2 =
            sample2.multi_key_sampling(single_ids_num_, 2, 1, false, use_precomputed_tau, precomputed_tau);
    auto zero_num = std::count_if(
            sampled_result1.second[1].begin(), sampled_result1.second[1].end(), [](int i) { return i == 0; });

    ASSERT_NE(sampled_result1.first, sampled_result2.first);
    ASSERT_EQ(sampled_result1.first.size(), single_ids_num_);
    ASSERT_EQ(sampled_result2.first.size(), single_ids_num_);
    ASSERT_EQ(sampled_result1.second[1].size(), zero_num);
    ASSERT_EQ(sampled_result1.second[0].size(), sampled_result1.second[1].size());
    ASSERT_EQ(sampled_result2.second[0].size(), sampled_result2.second[1].size());
}

TEST_F(DPSamplingTest, sample_without_zero) {
    DPSampling sample1;
    DPSampling sample2;
    bool use_precomputed_tau = true;
    std::size_t precomputed_tau = 722;
    auto sampled_result1 =
            sample1.multi_key_sampling(single_ids_num_, 2, -1, true, use_precomputed_tau, precomputed_tau);
    auto sampled_result2 =
            sample2.multi_key_sampling(single_ids_num_, 2, -1, false, use_precomputed_tau, precomputed_tau);

    auto zero_num1 = std::count_if(
            sampled_result1.second[1].begin(), sampled_result1.second[1].end(), [](int i) { return i == 0; });
    auto zero_num2 = std::count_if(
            sampled_result1.second[1].begin(), sampled_result1.second[1].end(), [](int i) { return i == 0; });

    ASSERT_NE(sampled_result1.first, sampled_result2.first);
    ASSERT_NE(sampled_result1.second[1].size(), zero_num1);
    ASSERT_NE(sampled_result1.second[1].size(), zero_num2);
    ASSERT_EQ(sampled_result1.second[0].size(), sampled_result1.second[1].size());
    ASSERT_EQ(sampled_result2.second[0].size(), sampled_result2.second[1].size());
}

TEST_F(DPSamplingTest, sample_with_multi_id) {
    DPSampling sample1;
    DPSampling sample2;
    bool use_precomputed_tau = true;
    std::size_t precomputed_tau = 722;
    auto sampled_result1 =
            sample1.multi_key_sampling(multiple_ids_num_, 1, 0, true, use_precomputed_tau, precomputed_tau);
    auto sampled_result2 =
            sample2.multi_key_sampling(multiple_ids_num_, 1, 0, false, use_precomputed_tau, precomputed_tau);

    ASSERT_EQ(sampled_result1.first.size(), multiple_ids_num_);
    ASSERT_EQ(sampled_result2.first.size(), multiple_ids_num_);
    ASSERT_EQ(sampled_result1.first.size(), sampled_result2.first.size());
    ASSERT_EQ(sampled_result1.first[0].size(), sampled_result2.first[0].size());
    ASSERT_NE(sampled_result1.first, sampled_result2.first);
}

TEST_F(DPSamplingTest, permute) {
    auto permutation1 = generate_permutation(1000);
    auto permutation2 = generate_permutation(1000);

    std::vector<std::size_t> input(1000);
    for (std::size_t i = 0; i < 10; ++i) {
        input[i] = i * 100;
    }
    std::vector<std::size_t> input_raw(input);
    permute_and_undo(permutation1, true, input);
    permute_and_undo(permutation1, false, input);

    ASSERT_NE(permutation1, permutation2);
    ASSERT_EQ(input, input_raw);
}

}  // namespace dpca_psi
}  // namespace privacy_go
