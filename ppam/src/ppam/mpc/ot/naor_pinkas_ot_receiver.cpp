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

#include "ppam/mpc/ot/naor_pinkas_ot_receiver.h"

#include <openssl/err.h>
#include <openssl/sha.h>

#include <cstring>
#include <stdexcept>
#include <string>

namespace privacy_go {
namespace ppam {

NaorPinkasOtReceiver::NaorPinkasOtReceiver(std::size_t base_ot_size, const block& choices) : choices_(choices) {
    msgs_.resize(base_ot_size);

    int ret = 0;

    group_ = EC_GROUP_new_by_curve_name(kCurveID);
    if (group_ == NULL) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    for (std::size_t idx = 0; idx < base_ot_size; ++idx) {
        auto k_sigma = EC_KEY_new();
        if (k_sigma == NULL) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        ret = EC_KEY_set_group(k_sigma, group_);
        if (ret != 1) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        auto pk0 = EC_POINT_new(group_);
        if (pk0 == NULL) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        auto c = EC_POINT_new(group_);
        if (c == NULL) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        auto gr = EC_POINT_new(group_);
        if (gr == NULL) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }
        k_sigma_.emplace_back(k_sigma);
        pk0_.emplace_back(pk0);
        c_.emplace_back(c);
        gr_.emplace_back(gr);
    }
}

NaorPinkasOtReceiver::~NaorPinkasOtReceiver() {
    for (auto& item : gr_) {
        EC_POINT_free(item);
    }
    for (auto& item : c_) {
        EC_POINT_free(item);
    }
    for (auto& item : pk0_) {
        EC_POINT_free(item);
    }
    for (auto& item : k_sigma_) {
        EC_KEY_free(item);
    }
    EC_GROUP_free(group_);
}

std::array<std::uint8_t, kEccPointLen> NaorPinkasOtReceiver::recv(
        const std::size_t idx, const std::array<std::array<std::uint8_t, kEccPointLen>, 2>& input) {
    int ret = 0;
    std::array<std::uint8_t, kEccPointLen> out_put_pk0;

    auto choices_bit_view = [this, idx]() {
        const std::uint8_t* bit_view = reinterpret_cast<const std::uint8_t*>(&choices_);
        int bit = bit_view[idx / 8] >> (idx % 8);
        return bit & 1;
    };

    int sigma = choices_bit_view();

    ret = EC_KEY_generate_key(k_sigma_[idx]);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // PK_sigma = g^k
    ret = EC_POINT_copy(pk0_[idx], EC_KEY_get0_public_key(k_sigma_[idx]));
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // receive C
    ret = EC_POINT_oct2point(group_, c_[idx], input[0].data(), input[0].size(), NULL);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // receive g^r
    ret = EC_POINT_oct2point(group_, gr_[idx], input[1].data(), input[1].size(), NULL);

    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // PK_0 = C_sigma/PK_sigma
    if (sigma) {
        ret = EC_POINT_invert(group_, pk0_[idx], NULL);
        if (ret != 1) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        ret = EC_POINT_add(group_, pk0_[idx], c_[idx], pk0_[idx], NULL);
        if (ret != 1) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }
    }

    // PK_0 to send
    std::size_t ret1 = 0;
    ret1 = EC_POINT_point2oct(group_, pk0_[idx], POINT_CONVERSION_COMPRESSED, out_put_pk0.data(), kEccPointLen, NULL);
    if (ret1 == 0) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // (g^r)^k = (PK_sigma)^r
    ret = EC_POINT_mul(group_, pk0_[idx], NULL, gr_[idx], EC_KEY_get0_private_key(k_sigma_[idx]), NULL);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // (g^r)^k = (PK_sigma)^r to send
    std::uint8_t msg[kEccPointLen];
    ret1 = EC_POINT_point2oct(group_, pk0_[idx], POINT_CONVERSION_COMPRESSED, msg, kEccPointLen, NULL);
    if (ret1 == 0) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    msg[0] = static_cast<std::uint8_t>(sigma);

    // hash(PK_sigma^r)
    std::array<std::uint8_t, kHashDigestLen> md;
    SHA256(reinterpret_cast<const std::uint8_t*>(msg), kEccPointLen, md.data());
    std::memcpy(&msgs_[idx], md.data(), sizeof(block));

    return out_put_pk0;
}

}  // namespace ppam
}  // namespace privacy_go
