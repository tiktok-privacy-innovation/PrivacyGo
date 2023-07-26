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

#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "dpca-psi/crypto/prng.h"

namespace privacy_go {
namespace dpca_psi {

inline std::string string_2_hex(const std::string& data) {
    const std::string hex = "0123456789ABCDEF";
    std::stringstream ss;
    for (const auto& item : data) {
        ss << hex[(unsigned char)item >> 4] << hex[(unsigned char)item & 0xf];
    }
    return std::move(ss.str());
}

inline std::string hex_2_string(const std::string& data) {
    std::string result;
    for (std::size_t i = 0; i < data.length(); i += 2) {
        std::string byte = data.substr(i, 2);
        char chr = static_cast<char>(strtol(byte.c_str(), NULL, 16));
        result.push_back(chr);
    }
    return result;
}

inline block read_block_from_dev_urandom() {
    block ret;
    std::ifstream in("/dev/urandom");
    in.read(reinterpret_cast<char*>(&ret), sizeof(ret));
    in.close();
    return ret;
}

template <typename T>
inline T read_data_from_dev_urandom() {
    T ret;
    std::ifstream in("/dev/urandom");
    in.read(reinterpret_cast<char*>(&ret), sizeof(ret));
    in.close();
    return ret;
}

inline std::chrono::time_point<std::chrono::high_resolution_clock> clock_start() {
    return std::chrono::high_resolution_clock::now();
}

inline std::int64_t time_from(const std::chrono::time_point<std::chrono::high_resolution_clock>& start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start)
            .count();
}

// Generates random swap permutation.
inline std::vector<std::size_t> generate_permutation(std::size_t n) {
    std::vector<std::size_t> permutation(n);
    for (std::size_t i = 0; i < n; ++i) {
        permutation[i] = i;
    }
    auto seed = read_block_from_dev_urandom();
    PRNG prng(seed);
    std::shuffle(permutation.begin(), permutation.end(), prng);
    return permutation;
}

// Applies or un-applies permutation given data, permutation and is_permute flag.
template <typename T>
inline void permute_and_undo(const std::vector<std::size_t>& permutation, bool is_permute, std::vector<T>& data) {
    std::vector<T> output;
    output.resize(data.size());
    if (is_permute) {
        for (std::size_t i = 0; i < permutation.size(); ++i) {
            output[permutation[i]] = data[i];
        }
    } else {
        for (std::size_t i = 0; i < permutation.size(); ++i) {
            output[i] = data[permutation[i]];
        }
    }
    data.clear();
    std::swap(output, data);
}

}  // namespace dpca_psi
}  // namespace privacy_go
