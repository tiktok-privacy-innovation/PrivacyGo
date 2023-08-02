// Copyright 2023 TikTok Ltd.
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

#include "ppam/mpc/ot/iknp_ot_ext_sender.h"

#include "ppam/mpc/common/utils.h"

namespace privacy_go {
namespace ppam {

void IknpOtExtSender::initialize(std::vector<block>& msg) {
    for (std::size_t i = 0; i < kBaseOTSize; i++) {
        prng_[i].set_seed(msg[i]);
    }
    return;
}

void IknpOtExtSender::send(block& base_ot_choice, const std::vector<std::vector<block>>& rcv_matrix,
        std::vector<block>& message0, std::vector<block>& message1) {
    if (ex_ot_num_ % (sizeof(block) * 8) != 0) {
        return;
    }
    auto block_num = ex_ot_num_ / (sizeof(block) * 8);

    std::vector<block> rows_mat(ex_ot_num_);
    std::vector<std::vector<block>> cols_mat(kBaseOTSize, std::vector<block>(block_num));
    std::vector<block> matrix_input(kBaseOTSize);
    std::vector<block> matrix_output(kBaseOTSize);

    for (std::size_t i = 0; i < kBaseOTSize; i++) {
        const std::uint8_t* bits = reinterpret_cast<const std::uint8_t*>(&base_ot_choice);
        int one_bit = (bits[i / 8] >> (i % 8)) & 0x1;
        prng_[i].get(&(cols_mat[i][0]), block_num);

        if (one_bit) {
            for (std::size_t j = 0; j < block_num; j++) {
                cols_mat[i][j] = cols_mat[i][j] ^ rcv_matrix[i][j];
            }
        }
    }

    for (std::size_t i = 0; i < block_num; i++) {
        for (std::size_t j = 0; j < kBaseOTSize; j++) {
            matrix_input[j] = cols_mat[j][i];
        }
        matrix_transpose(matrix_input, kBaseOTSize, kBaseOTSize, matrix_output);
        for (std::size_t j = 0; j < kBaseOTSize; j++) {
            rows_mat[i * kBaseOTSize + j] = matrix_output[j];
        }
    }

    AES fixed_key_aes;
    fixed_key_aes.set_key(aes_key_);
    for (std::size_t i = 0; i < ex_ot_num_; i++) {
        block input = rows_mat[i] ^ _mm_set_epi64x(0, i);
        fixed_key_aes.ecb_encrypt_block(input, message0[i]);
        input = input ^ base_ot_choice;
        fixed_key_aes.ecb_encrypt_block(input, message1[i]);
    }
}

}  // namespace ppam
}  // namespace privacy_go
