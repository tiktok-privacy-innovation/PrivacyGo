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

#include <random>
#include <string>
#include <utility>
#include <vector>

#include "dpca-psi/crypto/prng.h"

namespace privacy_go {
namespace dpca_psi {

const std::size_t kIdentifierLen = 32;

// Returns ramdom keys using prng.
// The identifier is filled alternately with numbers and alphabets.
inline std::vector<std::string> random_keys(PRNG& prng, std::size_t n, const std::string& suffix) {
    const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const std::string number = "0123456789";
    std::vector<std::string> result;
    result.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        std::string identifier;
        for (std::size_t idx = 0; idx < kIdentifierLen; ++idx) {
            if (idx & 1) {
                identifier += alphabet[prng.get<std::uint8_t>() % alphabet.size()];
            } else {
                identifier += number[prng.get<std::uint8_t>() % number.size()];
            }
        }
        identifier += suffix;
        result.emplace_back(identifier);
    }
    return std::move(result);
}

// Returns ramdom features using prng.
// @param is_zero: indiciates whether the random feature should be set to zero.
inline std::vector<std::uint64_t> random_features(PRNG& prng, std::size_t n, bool is_zero) {
    std::vector<std::uint64_t> result(n, 0);
    if (!is_zero) {
        for (std::size_t i = 0; i < n; ++i) {
            result[i] = prng.get<std::uint64_t>();
        }
    }
    return std::move(result);
}

}  // namespace dpca_psi
}  // namespace privacy_go
