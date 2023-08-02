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

#include <emmintrin.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <wmmintrin.h>

#include <memory>

#include "Eigen/Dense"

#include "dpca-psi/crypto/aes.h"
#include "dpca-psi/crypto/prng.h"
#include "dpca-psi/network/io_base.h"
#include "dpca-psi/network/two_channel_net_io.h"

namespace privacy_go {
namespace ppam {

using IOBase = dpca_psi::IOBase;
using TwoChannelNetIO = dpca_psi::TwoChannelNetIO;
using AES = dpca_psi::AES;
using PRNG = dpca_psi::PRNG;

using block = __m128i;
const std::size_t kHashDigestLen = SHA256_DIGEST_LENGTH;
const std::size_t kEccPointLen = 33;
const std::size_t kCurveID = NID_X9_62_prime256v1;
const std::size_t kBaseOTSize = 128;
const std::size_t kOTSize = 1024;

template <typename T>
using eMatrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

class CryptoMatrix {
public:
    CryptoMatrix() {
    }
    ~CryptoMatrix() {
    }

    eMatrix<std::int64_t> shares;

    std::size_t rows() const {
        return shares.rows();
    }
    std::size_t cols() const {
        return shares.cols();
    }
    std::size_t size() const {
        return shares.size();
    }

    CryptoMatrix(std::int64_t xSize, std::int64_t ySize) {
        resize(xSize, ySize);
    }

    CryptoMatrix operator+(const CryptoMatrix& b) const {
        CryptoMatrix c;
        c.shares.resize(b.rows(), b.cols());
        c.shares = shares + b.shares;
        return c;
    }

    CryptoMatrix operator-(const CryptoMatrix& b) const {
        CryptoMatrix c;
        c.shares.resize(b.rows(), b.cols());
        c.shares = shares - b.shares;
        return c;
    }

    void resize(std::int64_t xSize, std::int64_t ySize) {
        shares.resize(xSize, ySize);
    }
};

}  // namespace ppam
}  // namespace privacy_go
