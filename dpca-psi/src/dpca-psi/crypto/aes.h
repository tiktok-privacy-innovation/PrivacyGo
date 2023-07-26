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

#include <wmmintrin.h>

#include <array>
#include <cstdint>
#include <cstring>

namespace privacy_go {
namespace dpca_psi {

using block = __m128i;

class AES {
public:
    AES() = default;

    AES(const AES&) = default;

    explicit AES(const block& user_key);

    void set_key(const block& user_key);

    void ecb_encrypt_block(const block& plaintext, block& ciphertext) const;

    block ecb_encrypt_block(const block& plaintext) const;

    void ecb_encrypt_counter_mode(std::uint64_t base_idx, std::uint64_t length, block* ciphertext) const {
        ecb_encrypt_counter_mode(_mm_set_epi64x(0, base_idx), length, ciphertext);
    }

    void ecb_encrypt_counter_mode(block base_idx, std::uint64_t length, block* ciphertext) const;

    const block& get_key() const {
        return round_key_[0];
    }

    std::array<block, 11> round_key_;

private:
    static block round_encrypt(block state, const block& round_key) {
        return _mm_aesenc_si128(state, round_key);
    }

    static block final_encrypt(block state, const block& round_key) {
        return _mm_aesenclast_si128(state, round_key);
    }
};

}  // namespace dpca_psi
}  // namespace privacy_go
