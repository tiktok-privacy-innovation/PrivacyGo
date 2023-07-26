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
#include <cstring>
#include <vector>

#include "dpca-psi/crypto/aes.h"

namespace privacy_go {
namespace dpca_psi {

class PRNG {
public:
    PRNG();

    explicit PRNG(const block& seed, std::uint64_t buffer_size = 256);

    PRNG(PRNG&& s);

    PRNG(const PRNG&) = delete;

    void operator=(PRNG&&);

    void set_seed(const block& b, std::uint64_t buffer_size = 256);

    const block get_seed() const;

    template <typename T>
    T get() {
        T ret;
        get(&ret, 1);
        return ret;
    }

    template <typename T>
    void get(T* dest, std::uint64_t length) {
        std::uint64_t length_u8 = length * sizeof(T);
        std::uint8_t* dest_u8 = reinterpret_cast<std::uint8_t*>(dest);

        impl_get(dest_u8, length_u8);
    }

    std::uint8_t get_bit();

    typedef std::uint64_t result_type;
    static constexpr result_type min() {
        return 0;
    }
    static constexpr result_type max() {
        return (result_type)-1;
    }
    result_type operator()() {
        return get<result_type>();
    }

private:
    void refill_buffer();

    void impl_get(std::uint8_t* data_u8, std::uint64_t length_u8);

    std::vector<block> buffer_;

    AES aes_;

    std::uint64_t bytes_idx_;
    std::uint64_t block_idx_;
    std::uint64_t buffer_byte_capacity_;
};

template <>
inline void PRNG::get<bool>(bool* dest, std::uint64_t length) {
    get(reinterpret_cast<std::uint8_t*>(dest), length);
    for (std::uint64_t i = 0; i < length; ++i) {
        dest[i] = (reinterpret_cast<std::uint8_t*>(dest)[i]) & 1;
    }
}

template <>
inline bool PRNG::get<bool>() {
    std::uint8_t ret;
    get(reinterpret_cast<std::uint8_t*>(&ret), 1);
    return ret & 1;
}

}  // namespace dpca_psi
}  // namespace privacy_go
