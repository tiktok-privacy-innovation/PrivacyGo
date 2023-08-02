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

#include "ppam/mpc/common/pseudo_rand_generator.h"

#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "ppam/mpc/common/utils.h"

namespace privacy_go {
namespace ppam {

class MPCPRNGTest : public ::testing::Test {
public:
    std::size_t test_iter_num_ = 10;
};

TEST_F(MPCPRNGTest, common) {
    block seed = read_block_from_dev_urandom();
    PseudoRandGenerator prng(seed);
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        prng.get_common_rand();
    }
}

TEST_F(MPCPRNGTest, unique) {
    block seed = read_block_from_dev_urandom();
    PseudoRandGenerator prng(seed);
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        prng.get_unique_rand();
    }
}

}  // namespace ppam
}  // namespace privacy_go
