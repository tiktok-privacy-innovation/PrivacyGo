// Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
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
//
// This file may have been modified by TikTok Ltd. All TikTok's Modifications
// are Copyright (2023) TikTok Ltd.

#pragma once

#include <emmintrin.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <array>
#include <vector>

#include "ppam/mpc/common/defines.h"

namespace privacy_go {
namespace ppam {

class NaorPinkasOtReceiver {
public:
    NaorPinkasOtReceiver() = delete;

    NaorPinkasOtReceiver(const NaorPinkasOtReceiver& other) = delete;

    NaorPinkasOtReceiver& operator=(const NaorPinkasOtReceiver& other) = delete;

    NaorPinkasOtReceiver(std::size_t base_ot_size, const block& choices);

    ~NaorPinkasOtReceiver();

    std::array<std::uint8_t, kEccPointLen> recv(
            const std::size_t idx, const std::array<std::array<std::uint8_t, kEccPointLen>, 2>& input);

    std::vector<block> msgs_{};

private:
    const block choices_;
    EC_GROUP* group_;
    std::vector<EC_KEY*> k_sigma_;
    std::vector<EC_POINT*> pk0_;
    std::vector<EC_POINT*> c_;
    std::vector<EC_POINT*> gr_;
};

}  // namespace ppam
}  // namespace privacy_go
