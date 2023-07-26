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

#include <cmath>

#include <random>

#include "glog/logging.h"

#include "dpca-psi/common/defines.h"
#include "dpca-psi/common/dummy_data_utils.h"
#include "dpca-psi/common/utils.h"

namespace privacy_go {
namespace dpca_psi {

DPSampling::DPSampling() : prng_(read_block_from_dev_urandom()), common_prng_(kZeroBlock) {
}

std::pair<std::vector<std::vector<std::string>>, std::vector<std::vector<std::uint64_t>>>
DPSampling::multi_key_sampling(std::size_t key_size, std::size_t feature_size, int zero_column, bool is_sender,
        bool use_precomputed_tau, std::size_t precomputed_tau) {
    std::size_t tau = use_precomputed_tau ? precomputed_tau : 0;

    std::size_t dummy_data_size = key_size * tau;
    auto common_keys = random_keys(common_prng_, 2 * tau, "");
    std::string unique_suffix = is_sender ? "DA" : "DB";
    auto unique_keys = random_keys(prng_, (key_size - 1) * tau, unique_suffix);
    std::vector<std::vector<std::uint64_t>> dummied_features;
    dummied_features.reserve(feature_size);
    for (std::size_t feat_idx = 0; feat_idx < feature_size; ++feat_idx) {
        auto feature = random_features(prng_, dummy_data_size, feat_idx == static_cast<std::size_t>(zero_column));
        dummied_features.emplace_back(feature);
    }

    std::vector<std::vector<std::string>> dummied_keys;
    dummied_keys.reserve(key_size);
    for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
        std::vector<std::string> unique_keys_i;
        unique_keys_i.reserve(unique_keys.size());
        for (std::size_t item_idx = 0; item_idx < unique_keys.size(); ++item_idx) {
            unique_keys_i.emplace_back(unique_keys[item_idx] + std::to_string(key_idx));
        }
        std::vector<std::string> common_keys_i;
        common_keys_i.assign(common_keys.begin(), common_keys.end());
        for (std::size_t item_idx = 0; item_idx < common_keys_i.size(); ++item_idx) {
            common_keys_i[item_idx] += std::to_string(key_idx);
        }
        std::shuffle(common_keys_i.begin(), common_keys_i.end(), prng_);

        unique_keys_i.insert(unique_keys_i.begin() + key_idx * tau, common_keys_i.begin(), common_keys_i.begin() + tau);
        dummied_keys.emplace_back(std::move(unique_keys_i));
    }
    return std::make_pair(std::move(dummied_keys), std::move(dummied_features));
}

void DPSampling::set_common_prng_seed(const block& seed) {
    common_prng_.set_seed(seed);
}

}  // namespace dpca_psi
}  // namespace privacy_go
