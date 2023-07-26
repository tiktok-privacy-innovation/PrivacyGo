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

#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "dpca-psi/common/defines.h"
#include "dpca-psi/common/utils.h"

namespace privacy_go {
namespace dpca_psi {

class AesTest : public ::testing::Test {
public:
    std::size_t test_iter_num_ = 10;
    std::size_t bench_iter_num_ = 0x10000;
};

TEST_F(AesTest, encrypt) {
    block key = read_block_from_dev_urandom();
    AES aes(key);
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        block random_block = read_block_from_dev_urandom();
        aes.ecb_encrypt_block(random_block);
    }
}

TEST_F(AesTest, bench) {
    block key = read_block_from_dev_urandom();
    AES aes(key);
    std::vector<block> random_blocks(bench_iter_num_, kZeroBlock);
    auto start = clock_start();
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        aes.ecb_encrypt_counter_mode(0x100, bench_iter_num_, random_blocks.data());
    }
    auto duration = time_from(start);

    std::cout << duration * 1000 / static_cast<int64_t>(test_iter_num_ * bench_iter_num_) << "ns per op.\n";
}

}  // namespace dpca_psi
}  // namespace privacy_go
