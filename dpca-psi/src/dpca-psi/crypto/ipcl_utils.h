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

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ipcl/bignum.h"

#include "dpca-psi/common/defines.h"

namespace privacy_go {
namespace dpca_psi {

// Converts BigNumber to bytes.
inline void ipcl_bn_2_bytes(const BigNumber& in, ByteVector& out) {
    std::size_t length = (in.BitSize() + 7) / 8;
    std::vector<std::uint8_t> temp(length);
    in.num2char(temp);
    out.resize(length);
    for (std::size_t idx = 0; idx < length; ++idx) {
        out[idx] = Byte(temp[idx]);
    }
}

// Converts bytes to BigNumber.
inline void ipcl_bytes_2_bn(const ByteVector& in, BigNumber& out) {
    std::size_t bytes = in.size();
    std::size_t length = (bytes + 3) / 4;
    std::vector<std::uint32_t> vec_u32(length, 0);
    for (std::size_t i = 0; i < length; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            std::size_t cur_idx = i * 4 + j;
            if (cur_idx < bytes) {
                vec_u32[i] += reinterpret_cast<const uint8_t*>(in.data())[cur_idx] << (8 * j);
            }
        }
    }
    out = BigNumber(vec_u32.data(), static_cast<int>(length));
}

// Left shift bn bits. bn << bits.
inline void ipcl_bn_lshift(BigNumber& in, const std::size_t bits) {
    std::size_t length = (bits + 1 + 31) / 32;
    std::size_t remainder = bits % 32;
    std::vector<std::uint32_t> temp(length, 0);
    temp[length - 1] = 1 << remainder;
    BigNumber shift_bn(temp.data(), static_cast<int>(length));
    in *= shift_bn;
}

// Converts BigNumber to std::uint64_t.
inline std::uint64_t ipcl_bn_2_u64(const BigNumber& in) {
    std::size_t length = in.DwordSize();
    if (length == 0) {
        return 0;
    }
    std::vector<std::uint32_t> data;
    in.num2vec(data);
    std::uint64_t value = data[0];
    if (length > 1) {
        value += static_cast<std::uint64_t>(data[1]) << 32;
    }
    return value;
}

// Converts std::uint64_t to BigNumber.
inline BigNumber ipcl_u64_2_bn(std::uint64_t value) {
    std::vector<std::uint32_t> vec_u32 = {static_cast<std::uint32_t>(value), static_cast<std::uint32_t>(value >> 32)};
    return BigNumber(vec_u32.data(), 2);
}

}  // namespace dpca_psi
}  // namespace privacy_go
