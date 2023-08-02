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

class IknpOtExtReceiver {
public:
    IknpOtExtReceiver() = delete;

    IknpOtExtReceiver(int ot_length, block key) : ex_ot_num_(ot_length), aes_key_(key) {
    }

    ~IknpOtExtReceiver() = default;

    void initialize(std::vector<block>& msg0, std::vector<block>& msg1);

    void receive(std::vector<block>& choices, std::vector<std::vector<block>>& col_matrix, std::vector<block>& message);

private:
    std::size_t ex_ot_num_ = 0;
    block aes_key_;
    PRNG prng_0_[kBaseOTSize];
    PRNG prng_1_[kBaseOTSize];
};

}  // namespace ppam
}  // namespace privacy_go
