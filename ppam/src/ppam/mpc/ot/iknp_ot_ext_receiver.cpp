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

#include "ppam/mpc/ot/iknp_ot_ext_receiver.h"

#include "ppam/mpc/common/utils.h"
#include "ppam/mpc/ot/naor_pinkas_ot_receiver.h"

namespace privacy_go {
namespace ppam {

void IknpOtExtReceiver::initialize(std::vector<block>& msg0, std::vector<block>& msg1) {
    for (std::size_t i = 0; i < kBaseOTSize; i++) {
        prng_0_[i].set_seed(msg0[i]);
        prng_1_[i].set_seed(msg1[i]);
    }
    return;
}

void IknpOtExtReceiver::receive(
        std::vector<block>& choices, std::vector<std::vector<block>>& col_matrix, std::vector<block>& message) {
    if (ex_ot_num_ % (sizeof(block) * 8) != 0) {
        return;
    }
    auto block_num = ex_ot_num_ / (sizeof(block) * 8);
    std::vector<std::vector<block>> t0(kBaseOTSize, std::vector<block>(block_num));
    std::vector<std::vector<block>> t1(kBaseOTSize, std::vector<block>(block_num));
    std::vector<block> matrix_input(kBaseOTSize);
    std::vector<block> matrix_output(kBaseOTSize);
    std::vector<block> row_matrix(ex_ot_num_);

    for (std::size_t i = 0; i < kBaseOTSize; i++) {
        prng_0_[i].get(&(t0[i][0]), block_num);
        prng_1_[i].get(&(t1[i][0]), block_num);

        for (std::size_t j = 0; j < block_num; j++) {
            col_matrix[i][j] = t0[i][j] ^ t1[i][j] ^ choices[j];
        }
    }

    for (std::size_t i = 0; i < block_num; i++) {
        for (std::size_t j = 0; j < kBaseOTSize; j++) {
            matrix_input[j] = t0[j][i];
        }
        matrix_transpose(matrix_input, kBaseOTSize, kBaseOTSize, matrix_output);
        for (std::size_t j = 0; j < kBaseOTSize; j++) {
            row_matrix[i * kBaseOTSize + j] = matrix_output[j];
        }
    }

    AES fixed_key_aes;
    fixed_key_aes.set_key(aes_key_);
    for (std::size_t i = 0; i < ex_ot_num_; i++) {
        block input = row_matrix[i] ^ _mm_set_epi64x(0, i);
        fixed_key_aes.ecb_encrypt_block(input, message[i]);
    }
}

}  // namespace ppam
}  // namespace privacy_go
