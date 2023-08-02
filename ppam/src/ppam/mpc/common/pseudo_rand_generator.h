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

#pragma once

#include <random>
#include <vector>

#include "ppam/mpc/common/defines.h"
#include "ppam/mpc/common/utils.h"

namespace privacy_go {
namespace ppam {

class PseudoRandGenerator {
public:
    PseudoRandGenerator() = delete;

    explicit PseudoRandGenerator(const block& common_seed, std::int64_t buff_size = 256) {
        common_rand_gen_idx = 0;
        common_rand_buff.resize(buff_size);
        common_rand_gen.set_key(common_seed);

        unique_rand_gen_idx = 0;
        unique_rand_buff.resize(buff_size);
        unique_rand_gen.set_key(read_block_from_dev_urandom());

        refill_common_buffer();
        refill_unique_buffer();
    }

    ~PseudoRandGenerator() = default;

    std::int64_t get_common_rand() {
        if (common_rand_idx + sizeof(std::int64_t) > common_rand_buff.size() * sizeof(block)) {
            refill_common_buffer();
        }
        std::int64_t ret = *(std::int64_t*)((std::uint8_t*)common_rand_buff.data() + common_rand_idx);
        common_rand_idx += sizeof(std::int64_t);
        return ret;
    }

    std::int64_t get_unique_rand() {
        if (unique_rand_idx + sizeof(std::int64_t) > unique_rand_buff.size() * sizeof(block)) {
            refill_unique_buffer();
        }

        std::int64_t ret = *(std::int64_t*)((std::uint8_t*)unique_rand_buff.data() + unique_rand_idx);
        unique_rand_idx += sizeof(std::int64_t);
        return ret;
    }

private:
    std::int64_t common_rand_idx = 0;
    std::int64_t common_rand_gen_idx = 0;
    std::int64_t unique_rand_idx = 0;
    std::int64_t unique_rand_gen_idx = 0;
    AES common_rand_gen;
    AES unique_rand_gen;
    std::vector<block> common_rand_buff{};
    std::vector<block> unique_rand_buff{};

    void refill_common_buffer() {
        common_rand_gen.ecb_encrypt_counter_mode(common_rand_gen_idx, common_rand_buff.size(), common_rand_buff.data());
        common_rand_gen_idx += common_rand_buff.size();
        common_rand_idx = 0;
    }

    void refill_unique_buffer() {
        unique_rand_gen.ecb_encrypt_counter_mode(unique_rand_gen_idx, unique_rand_buff.size(), unique_rand_buff.data());
        unique_rand_gen_idx += unique_rand_buff.size();
        unique_rand_idx = 0;
    }
};

}  // namespace ppam
}  // namespace privacy_go
