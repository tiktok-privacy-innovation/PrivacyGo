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

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "dpca-psi/crypto/prng.h"

namespace dpca_psi {

class DPSampling {
public:
    DPSampling();

    DPSampling(const DPSampling& other) = delete;

    DPSampling& operator=(const DPSampling& other) = delete;

    // Generates dummy data set for both parties in multi-key setting.
    // Returns sampled dummies(key, features) for sender or receiver.
    // @param key_size: the number of keys(ids).
    // @param feature_size: the number of feature columns.
    // @param zero_column: the index indicates which column's dummy should be set to zero.
    // @param is_sender: whether sender or receiver.
    // @param use_precomputed_tau: whether to use precomputed tau to avoid online caculation.
    // @param precomputed_tau: precomputed tau corresponding to specific(data_size, epsilon, maximum_queries, key_size).
    std::pair<std::vector<std::vector<std::string>>, std::vector<std::vector<std::uint64_t>>> multi_key_sampling(
            std::size_t key_size, std::size_t feature_size, int zero_column, bool is_sender, bool use_precomputed_tau,
            std::size_t precomputed_tau);

    // Sets the prng seed for common dummy data generation.
    void set_common_prng_seed(const block& seed);

    ~DPSampling() {
    }

private:
    // Self prng.
    PRNG prng_;

    // Agreed  prng.
    PRNG common_prng_;
};
}  // namespace dpca_psi
