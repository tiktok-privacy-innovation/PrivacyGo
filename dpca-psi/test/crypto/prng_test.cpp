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

#include "dpca-psi/crypto/prng.h"

#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "dpca-psi/common/utils.h"

namespace privacy_go {
namespace dpca_psi {

class PRNGTest : public ::testing::Test {
public:
    std::size_t test_iter_num_ = 10;
};

TEST_F(PRNGTest, move) {
    block seed = read_block_from_dev_urandom();
    PRNG prng(seed);
    PRNG prng_ = std::move(prng);
}

TEST_F(PRNGTest, get) {
    block seed = read_block_from_dev_urandom();
    PRNG prng(seed);
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        prng.get<std::uint8_t>();
        prng.get<std::uint64_t>();
        prng.get<bool>();
        prng.get<block>();
        prng.get_bit();
        prng();
    }
}

TEST_F(PRNGTest, get_array) {
    block seed = read_block_from_dev_urandom();
    PRNG prng(seed);
    std::size_t length = 100;
    std::vector<std::uint8_t> random_u8(length);
    std::vector<std::uint64_t> random_u64(length);
    std::vector<block> random_block(length);
    bool random_bool[100];
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        prng.get<std::uint8_t>(random_u8.data(), length);
        prng.get<std::uint64_t>(random_u64.data(), length);
        prng.get<bool>(random_bool, 100);
        prng.get<block>(random_block.data(), length);
    }
}

}  // namespace dpca_psi
}  // namespace privacy_go
