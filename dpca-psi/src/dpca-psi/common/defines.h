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

#include <emmintrin.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <vector>

namespace privacy_go {
namespace dpca_psi {

using block = __m128i;
const std::size_t kHashDigestLen = SHA256_DIGEST_LENGTH;
const std::size_t kHashDigestBitsLen = SHA256_DIGEST_LENGTH * 8;
const std::size_t kEccPointLen = 33;
const std::size_t kEccKeyBitsLen = 256;
const std::size_t kECCCompareBytesLen = 12;
const std::size_t kCurveID = NID_X9_62_prime256v1;
const std::size_t kValueBits = 64;
const block kZeroBlock = _mm_set_epi64x(0, 0);
enum class Byte : unsigned char {};
using ByteVector = std::vector<Byte>;

}  // namespace dpca_psi
}  // namespace privacy_go
