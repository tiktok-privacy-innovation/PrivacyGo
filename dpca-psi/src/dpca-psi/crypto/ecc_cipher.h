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

#include <string>

#include "dpca-psi/common/defines.h"
#include "dpca-psi/crypto/smart_pointer.h"

namespace privacy_go {
namespace dpca_psi {

class EccCipher {
public:
    EccCipher() = delete;

    // Constructor with curve_id and keys_num.
    EccCipher(std::size_t curve_id, std::size_t keys_num);

    EccCipher(const EccCipher& other) = delete;

    EccCipher& operator=(const EccCipher& other) = delete;

    // Maps plaintext to a point on elliptic curve and exponentiates the point to `private_keys_[key_index]` power.
    // Returns a compressed form of the result point.
    ByteVector hash_encrypt(const std::string& plaintext, std::size_t key_index);

    // Deserializes the points and exponentiates the point to `private_keys_[key_index]` power.
    // Returns a compressed form of the result point.
    ByteVector encrypt(const ByteVector& point, std::size_t key_index);

    // Deserializes the points and exponentiates the point to `private_key_[key_index_first] /
    // private_key_[key_index_second] ` power.
    // Returns a compressed form of the result point.
    ByteVector encrypt_and_div(const ByteVector& point, std::size_t key_index_first, std::size_t key_index_second);

    ~EccCipher() {
    }

private:
    // Hashes a string to a point on elliptic curve using SHA3-256 with "try-and-increment" method.
    // The method can be illustrated as a general implementation of HashToX:
    // 1. Applies a secure hash function, such as SHA3-256, to hash the data and map the hash result to the x
    // coordinates of the elliptic curve.
    // Actually, we use SHA3-256  based ranom oracle in consistent with google's implementation.
    // 2. Calculates y coordinates from the definition of elliptic curves and x coordinates.
    //   a. If this fails, hash x again and repeat step 2.
    //   b. If successful, return the elliptic curve point (x,y).
    ECPointPtr hash_to_curve(const std::string& plaintext);

    // Serializes a point to a byte vector in compressed form.
    ByteVector export_to_bytes(const ECPointPtr& point) const;

    // Deserializes a byte vector to a point.
    ECPointPtr import_from_bytes(const ByteVector& plaintext) const;

    // Serializes a bignum to a string.
    std::string bn_to_string(const BignumPtr& bn) const;

    // SHA3-256 implementation based on openssl.
    ByteVector sha3_256_hash(const std::string& plaintext);

    // A random oracle function mapping x deterministically into a large domain.
    // Refers to
    // https://github.com/google/private-join-and-compute/blob/master/private_join_and_compute/crypto/context.h.
    BignumPtr ranom_oracle(const std::string& plaintext, const BignumPtr& max_value, BnCtxPtr& bn_ctx);

    // Generates strong random private key.
    void generate_private_key();

    // Computes y^2 = x^3 + a*x + b.
    BignumPtr compute_y_square(const BignumPtr& x, BnCtxPtr& bn_ctx);

    // Checks whether m is a quadratic residue modulo p.
    // Gcd(m, p) = 1, x^2 = m mod p has a solution if m^((p-1)/2) = 1 mod p.
    bool is_square(const BignumPtr& m, BnCtxPtr& bn_ctx);

    // Private keys for exponentiating.
    const BignumArrayPtr private_keys_;
    const std::size_t private_keys_num_;

    // Ec group for elliptic curve.
    const ECGroupPtr group_;

    // Stores ec curve param p, a, b, three and (p-1)/2 for hash_to_curve.
    const BignumPtr p_;
    const BignumPtr a_;
    const BignumPtr b_;
    const BignumPtr three_;
    const BignumPtr p_minus_one_over_two_;
};

}  // namespace dpca_psi
}  // namespace privacy_go
