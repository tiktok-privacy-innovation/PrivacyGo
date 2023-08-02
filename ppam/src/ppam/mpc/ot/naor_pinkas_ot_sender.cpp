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

#include "ppam/mpc/ot/naor_pinkas_ot_sender.h"

#include <openssl/err.h>
#include <openssl/sha.h>

#include <cstring>
#include <stdexcept>
#include <string>

namespace privacy_go {
namespace ppam {

NaorPinkasOtSender::NaorPinkasOtSender(std::size_t base_ot_size) {
    msgs_.resize(base_ot_size);

    int ret = 0;
    group_ = EC_GROUP_new_by_curve_name(kCurveID);
    if (group_ == NULL) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    for (std::size_t idx = 0; idx < base_ot_size; ++idx) {
        auto c = EC_KEY_new();
        if (c == NULL) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        ret = EC_KEY_set_group(c, group_);
        if (ret != 1) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        auto gr = EC_KEY_new();
        if (gr == NULL) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        ret = EC_KEY_set_group(gr, group_);
        if (ret != 1) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        auto cr = EC_POINT_new(group_);
        if (cr == NULL) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }

        auto pk0r = EC_POINT_new(group_);
        if (pk0r == NULL) {
            throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
        }
        c_.emplace_back(c);
        gr_.emplace_back(gr);
        pk0_r_.emplace_back(pk0r);
        cr_.emplace_back(cr);
    }
}

NaorPinkasOtSender::~NaorPinkasOtSender() {
    for (auto& item : cr_) {
        EC_POINT_free(item);
    }
    for (auto& item : pk0_r_) {
        EC_POINT_free(item);
    }
    for (auto& item : gr_) {
        EC_KEY_free(item);
    }
    for (auto& item : c_) {
        EC_KEY_free(item);
    }
    EC_GROUP_free(group_);
}

std::array<std::array<std::uint8_t, kEccPointLen>, 2> NaorPinkasOtSender::send_pre(const std::size_t idx) {
    std::array<std::array<std::uint8_t, kEccPointLen>, 2> output;

    int ret = EC_KEY_generate_key(c_[idx]);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // C
    const EC_POINT* C_point = EC_KEY_get0_public_key(c_[idx]);
    if (C_point == NULL) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    std::size_t ret1 = 0;
    ret1 = EC_POINT_point2oct(group_, C_point, POINT_CONVERSION_COMPRESSED, output[0].data(), kEccPointLen, NULL);
    if (ret1 == 0) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    ret = EC_KEY_generate_key(gr_[idx]);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // g^r
    const EC_POINT* gr_point = EC_KEY_get0_public_key(gr_[idx]);
    if (C_point == NULL) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    ret1 = EC_POINT_point2oct(group_, gr_point, POINT_CONVERSION_COMPRESSED, output[1].data(), kEccPointLen, NULL);
    if (ret1 == 0) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    return output;
}

void NaorPinkasOtSender::send_post(const std::size_t idx, const std::array<std::uint8_t, kEccPointLen>& input) {
    int ret = 0;

    // r
    const BIGNUM* r = EC_KEY_get0_private_key(gr_[idx]);
    if (r == NULL) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // C
    const EC_POINT* C_point = EC_KEY_get0_public_key(c_[idx]);
    if (C_point == NULL) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // C^r
    ret = EC_POINT_mul(group_, cr_[idx], NULL, C_point, r, NULL);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    ret = EC_POINT_oct2point(group_, pk0_r_[idx], input.data(), input.size(), NULL);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // PK_0^r
    ret = EC_POINT_mul(group_, pk0_r_[idx], NULL, pk0_r_[idx], r, NULL);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    std::uint8_t msg0[kEccPointLen];
    std::uint8_t msg1[kEccPointLen];

    std::size_t ret1 = 0;
    ret1 = EC_POINT_point2oct(group_, pk0_r_[idx], POINT_CONVERSION_COMPRESSED, msg0, kEccPointLen, NULL);
    if (ret1 == 0) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    ret = EC_POINT_invert(group_, pk0_r_[idx], NULL);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    // pk_1^r = c^r - pk0^r
    ret = EC_POINT_add(group_, pk0_r_[idx], cr_[idx], pk0_r_[idx], NULL);
    if (ret != 1) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    ret1 = EC_POINT_point2oct(group_, pk0_r_[idx], POINT_CONVERSION_COMPRESSED, msg1, kEccPointLen, NULL);
    if (ret1 == 0) {
        throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
    }

    msg0[0] = 0;
    msg1[0] = 1;
    std::array<std::uint8_t, kHashDigestLen> md;
    SHA256(msg0, kEccPointLen, md.data());
    std::memcpy(&msgs_[idx][0], md.data(), sizeof(block));
    SHA256(msg1, kEccPointLen, md.data());
    std::memcpy(&msgs_[idx][1], md.data(), sizeof(block));
}

}  // namespace ppam
}  // namespace privacy_go
