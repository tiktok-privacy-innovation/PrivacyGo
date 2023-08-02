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

#include <vector>

#include "ppam/mpc/common/defines.h"

namespace privacy_go {
namespace ppam {

class IknpOtExtSender {
public:
    IknpOtExtSender() = delete;

    IknpOtExtSender(int ot_length, block key, std::vector<block>& msg0, std::vector<block>& msg1)
            : ex_ot_num_(ot_length), aes_key_(key) {
        x0_ = std::vector<block>(msg0);
        x1_ = std::vector<block>(msg1);
    }

    IknpOtExtSender(int ot_length, block key) : ex_ot_num_(ot_length), aes_key_(key) {
    }

    ~IknpOtExtSender() = default;

    void initialize(std::vector<block>& msg);

    void send(block& base_ot_choice, const std::vector<std::vector<block>>& rcv_matrix, std::vector<block>& message0,
            std::vector<block>& message1);

private:
    std::size_t ex_ot_num_ = 0;
    block aes_key_;
    std::vector<block> x0_{};
    std::vector<block> x1_{};
    PRNG prng_[kBaseOTSize];
};

}  // namespace ppam
}  // namespace privacy_go
