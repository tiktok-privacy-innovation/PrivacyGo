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

#include "dpca-psi/crypto/aes.h"

namespace dpca_psi {

block prf(const block& b, std::uint64_t i) {
    return AES(b).ecb_encrypt_block(_mm_set_epi64x(0, i));
}

block key_gen_helper(block key, block key_rcon) {
    key_rcon = _mm_shuffle_epi32(key_rcon, _MM_SHUFFLE(3, 3, 3, 3));
    key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
    key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
    key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
    return _mm_xor_si128(key, key_rcon);
}

void AES::set_key(const block& user_key) {
    round_key_[0] = user_key;
    round_key_[1] = key_gen_helper(round_key_[0], _mm_aeskeygenassist_si128(round_key_[0], 0x01));
    round_key_[2] = key_gen_helper(round_key_[1], _mm_aeskeygenassist_si128(round_key_[1], 0x02));
    round_key_[3] = key_gen_helper(round_key_[2], _mm_aeskeygenassist_si128(round_key_[2], 0x04));
    round_key_[4] = key_gen_helper(round_key_[3], _mm_aeskeygenassist_si128(round_key_[3], 0x08));
    round_key_[5] = key_gen_helper(round_key_[4], _mm_aeskeygenassist_si128(round_key_[4], 0x10));
    round_key_[6] = key_gen_helper(round_key_[5], _mm_aeskeygenassist_si128(round_key_[5], 0x20));
    round_key_[7] = key_gen_helper(round_key_[6], _mm_aeskeygenassist_si128(round_key_[6], 0x40));
    round_key_[8] = key_gen_helper(round_key_[7], _mm_aeskeygenassist_si128(round_key_[7], 0x80));
    round_key_[9] = key_gen_helper(round_key_[8], _mm_aeskeygenassist_si128(round_key_[8], 0x1B));
    round_key_[10] = key_gen_helper(round_key_[9], _mm_aeskeygenassist_si128(round_key_[9], 0x36));
}

AES::AES(const block& user_key) {
    set_key(user_key);
}

void AES::ecb_encrypt_block(const block& plaintext, block& ciphertext) const {
    ciphertext = plaintext ^ round_key_[0];
    for (std::uint64_t i = 1; i < 10; ++i) {
        ciphertext = round_encrypt(ciphertext, round_key_[i]);
    }
    ciphertext = final_encrypt(ciphertext, round_key_[10]);
}

block AES::ecb_encrypt_block(const block& plaintext) const {
    block ret;
    ecb_encrypt_block(plaintext, ret);
    return ret;
}

void AES::ecb_encrypt_counter_mode(block base_idx, std::uint64_t block_length, block* ciphertext) const {
    std::int32_t idx = 0;
    const std::int32_t step = 8;
    std::int32_t length = std::int32_t(block_length - block_length % step);
    const auto b0 = _mm_set_epi64x(0, 0);
    const auto b1 = _mm_set_epi64x(0, 1ull);
    const auto b2 = _mm_set_epi64x(0, 2ull);
    const auto b3 = _mm_set_epi64x(0, 3ull);
    const auto b4 = _mm_set_epi64x(0, 4ull);
    const auto b5 = _mm_set_epi64x(0, 5ull);
    const auto b6 = _mm_set_epi64x(0, 6ull);
    const auto b7 = _mm_set_epi64x(0, 7ull);

    block temp[8];
    for (; idx < length; idx += step) {
        temp[0] = (base_idx + b0) ^ round_key_[0];
        temp[1] = (base_idx + b1) ^ round_key_[0];
        temp[2] = (base_idx + b2) ^ round_key_[0];
        temp[3] = (base_idx + b3) ^ round_key_[0];
        temp[4] = (base_idx + b4) ^ round_key_[0];
        temp[5] = (base_idx + b5) ^ round_key_[0];
        temp[6] = (base_idx + b6) ^ round_key_[0];
        temp[7] = (base_idx + b7) ^ round_key_[0];
        base_idx = base_idx + _mm_set_epi64x(0, step);

        temp[0] = round_encrypt(temp[0], round_key_[1]);
        temp[1] = round_encrypt(temp[1], round_key_[1]);
        temp[2] = round_encrypt(temp[2], round_key_[1]);
        temp[3] = round_encrypt(temp[3], round_key_[1]);
        temp[4] = round_encrypt(temp[4], round_key_[1]);
        temp[5] = round_encrypt(temp[5], round_key_[1]);
        temp[6] = round_encrypt(temp[6], round_key_[1]);
        temp[7] = round_encrypt(temp[7], round_key_[1]);

        temp[0] = round_encrypt(temp[0], round_key_[2]);
        temp[1] = round_encrypt(temp[1], round_key_[2]);
        temp[2] = round_encrypt(temp[2], round_key_[2]);
        temp[3] = round_encrypt(temp[3], round_key_[2]);
        temp[4] = round_encrypt(temp[4], round_key_[2]);
        temp[5] = round_encrypt(temp[5], round_key_[2]);
        temp[6] = round_encrypt(temp[6], round_key_[2]);
        temp[7] = round_encrypt(temp[7], round_key_[2]);

        temp[0] = round_encrypt(temp[0], round_key_[3]);
        temp[1] = round_encrypt(temp[1], round_key_[3]);
        temp[2] = round_encrypt(temp[2], round_key_[3]);
        temp[3] = round_encrypt(temp[3], round_key_[3]);
        temp[4] = round_encrypt(temp[4], round_key_[3]);
        temp[5] = round_encrypt(temp[5], round_key_[3]);
        temp[6] = round_encrypt(temp[6], round_key_[3]);
        temp[7] = round_encrypt(temp[7], round_key_[3]);

        temp[0] = round_encrypt(temp[0], round_key_[4]);
        temp[1] = round_encrypt(temp[1], round_key_[4]);
        temp[2] = round_encrypt(temp[2], round_key_[4]);
        temp[3] = round_encrypt(temp[3], round_key_[4]);
        temp[4] = round_encrypt(temp[4], round_key_[4]);
        temp[5] = round_encrypt(temp[5], round_key_[4]);
        temp[6] = round_encrypt(temp[6], round_key_[4]);
        temp[7] = round_encrypt(temp[7], round_key_[4]);

        temp[0] = round_encrypt(temp[0], round_key_[5]);
        temp[1] = round_encrypt(temp[1], round_key_[5]);
        temp[2] = round_encrypt(temp[2], round_key_[5]);
        temp[3] = round_encrypt(temp[3], round_key_[5]);
        temp[4] = round_encrypt(temp[4], round_key_[5]);
        temp[5] = round_encrypt(temp[5], round_key_[5]);
        temp[6] = round_encrypt(temp[6], round_key_[5]);
        temp[7] = round_encrypt(temp[7], round_key_[5]);

        temp[0] = round_encrypt(temp[0], round_key_[6]);
        temp[1] = round_encrypt(temp[1], round_key_[6]);
        temp[2] = round_encrypt(temp[2], round_key_[6]);
        temp[3] = round_encrypt(temp[3], round_key_[6]);
        temp[4] = round_encrypt(temp[4], round_key_[6]);
        temp[5] = round_encrypt(temp[5], round_key_[6]);
        temp[6] = round_encrypt(temp[6], round_key_[6]);
        temp[7] = round_encrypt(temp[7], round_key_[6]);

        temp[0] = round_encrypt(temp[0], round_key_[7]);
        temp[1] = round_encrypt(temp[1], round_key_[7]);
        temp[2] = round_encrypt(temp[2], round_key_[7]);
        temp[3] = round_encrypt(temp[3], round_key_[7]);
        temp[4] = round_encrypt(temp[4], round_key_[7]);
        temp[5] = round_encrypt(temp[5], round_key_[7]);
        temp[6] = round_encrypt(temp[6], round_key_[7]);
        temp[7] = round_encrypt(temp[7], round_key_[7]);

        temp[0] = round_encrypt(temp[0], round_key_[8]);
        temp[1] = round_encrypt(temp[1], round_key_[8]);
        temp[2] = round_encrypt(temp[2], round_key_[8]);
        temp[3] = round_encrypt(temp[3], round_key_[8]);
        temp[4] = round_encrypt(temp[4], round_key_[8]);
        temp[5] = round_encrypt(temp[5], round_key_[8]);
        temp[6] = round_encrypt(temp[6], round_key_[8]);
        temp[7] = round_encrypt(temp[7], round_key_[8]);

        temp[0] = round_encrypt(temp[0], round_key_[9]);
        temp[1] = round_encrypt(temp[1], round_key_[9]);
        temp[2] = round_encrypt(temp[2], round_key_[9]);
        temp[3] = round_encrypt(temp[3], round_key_[9]);
        temp[4] = round_encrypt(temp[4], round_key_[9]);
        temp[5] = round_encrypt(temp[5], round_key_[9]);
        temp[6] = round_encrypt(temp[6], round_key_[9]);
        temp[7] = round_encrypt(temp[7], round_key_[9]);

        temp[0] = final_encrypt(temp[0], round_key_[10]);
        temp[1] = final_encrypt(temp[1], round_key_[10]);
        temp[2] = final_encrypt(temp[2], round_key_[10]);
        temp[3] = final_encrypt(temp[3], round_key_[10]);
        temp[4] = final_encrypt(temp[4], round_key_[10]);
        temp[5] = final_encrypt(temp[5], round_key_[10]);
        temp[6] = final_encrypt(temp[6], round_key_[10]);
        temp[7] = final_encrypt(temp[7], round_key_[10]);

        memcpy(reinterpret_cast<std::uint8_t*>(ciphertext + idx), temp, sizeof(temp));
    }

    for (; idx < static_cast<std::int32_t>(block_length); ++idx) {
        auto temp_block = base_idx ^ round_key_[0];
        base_idx = base_idx + _mm_set_epi64x(0, 1);
        temp_block = round_encrypt(temp_block, round_key_[1]);
        temp_block = round_encrypt(temp_block, round_key_[2]);
        temp_block = round_encrypt(temp_block, round_key_[3]);
        temp_block = round_encrypt(temp_block, round_key_[4]);
        temp_block = round_encrypt(temp_block, round_key_[5]);
        temp_block = round_encrypt(temp_block, round_key_[6]);
        temp_block = round_encrypt(temp_block, round_key_[7]);
        temp_block = round_encrypt(temp_block, round_key_[8]);
        temp_block = round_encrypt(temp_block, round_key_[9]);
        temp_block = final_encrypt(temp_block, round_key_[10]);

        memcpy(reinterpret_cast<std::uint8_t*>(ciphertext + idx), &temp_block, sizeof(temp_block));
    }
}
}  // namespace dpca_psi
