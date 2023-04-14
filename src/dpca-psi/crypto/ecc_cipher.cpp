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

#include "dpca-psi/crypto/ecc_cipher.h"

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "dpca-psi/common/utils.h"

namespace dpca_psi {

inline void throw_openssl_error() {
    throw std::runtime_error("openssl error: " + std::to_string(ERR_get_error()));
}

EccCipher::EccCipher(std::size_t curve_id, std::size_t private_keys_num)
        : bn_ctx_(BN_CTX_new()),
          private_keys_(std::make_unique<BignumPtr[]>(private_keys_num)),
          private_keys_num_(private_keys_num),
          group_(EC_GROUP_new_by_curve_name(static_cast<int>(curve_id))),
          evp_md_ctx_(EVP_MD_CTX_new()),
          p_(BN_new()),
          a_(BN_new()),
          b_(BN_new()),
          three_(BN_new()),
          p_minus_one_over_two_(BN_new()) {
    if (bn_ctx_ == nullptr || group_ == nullptr || private_keys_ == nullptr || evp_md_ctx_ == nullptr ||
            p_ == nullptr || a_ == nullptr || b_ == nullptr || three_ == nullptr || p_minus_one_over_two_ == nullptr) {
        throw_openssl_error();
    }
    for (std::size_t i = 0; i < private_keys_num_; ++i) {
        private_keys_.get()[i] = BignumPtr(BN_new());
    }
    generate_private_key();

    auto ret = EC_GROUP_get_curve(group_.get(), p_.get(), a_.get(), b_.get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }

    ret = BN_set_word(three_.get(), 3);
    if (ret != 1) {
        throw_openssl_error();
    }

    auto ret_ptr = BN_copy(p_minus_one_over_two_.get(), p_.get());
    if (ret_ptr == nullptr) {
        throw_openssl_error();
    }

    ret = BN_sub_word(p_minus_one_over_two_.get(), 1);
    if (ret != 1) {
        throw_openssl_error();
    }

    ret = BN_rshift1(p_minus_one_over_two_.get(), p_minus_one_over_two_.get());
    if (ret != 1) {
        throw_openssl_error();
    }
}

std::string EccCipher::hash_encrypt(const std::string& plaintext, std::size_t key_index) {
    ECPointPtr point = hash_to_curve(plaintext);
    auto ret = EC_POINT_mul(
            group_.get(), point.get(), NULL, point.get(), private_keys_.get()[key_index].get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }
    return export_to_string(point);
}

std::string EccCipher::encrypt(const std::string& point, std::size_t key_index) {
    ECPointPtr deserialized_point = import_from_string(point);
    auto ret = EC_POINT_mul(group_.get(), deserialized_point.get(), NULL, deserialized_point.get(),
            private_keys_.get()[key_index].get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }
    return export_to_string(deserialized_point);
}

std::string EccCipher::encrypt_and_div(
        const std::string& point, std::size_t key_index_first, std::size_t key_index_second) {
    ECPointPtr deserialized_point = import_from_string(point);
    BignumPtr exponent = BignumPtr(BN_new());
    BN_mod_inverse(exponent.get(), private_keys_.get()[key_index_second].get(), EC_GROUP_get0_order(group_.get()),
            bn_ctx_.get());
    auto ret = BN_mod_mul(exponent.get(), private_keys_.get()[key_index_first].get(), exponent.get(),
            EC_GROUP_get0_order(group_.get()), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }
    ret = EC_POINT_mul(
            group_.get(), deserialized_point.get(), NULL, deserialized_point.get(), exponent.get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }
    return export_to_string(deserialized_point);
}

void EccCipher::generate_private_key() {
    BignumPtr order(BN_dup(EC_GROUP_get0_order(group_.get())));
    if (order == nullptr) {
        throw_openssl_error();
    }

    auto ret = BN_sub_word(order.get(), 1);
    if (ret != 1) {
        throw_openssl_error();
    }

    for (std::size_t i = 0; i < private_keys_num_; ++i) {
        ret = BN_rand_range(private_keys_.get()[i].get(), order.get());
        if (ret != 1) {
            throw_openssl_error();
        }

        // Checks bit length of private key to ensure strong radomness.
        while (BN_num_bits(private_keys_.get()[i].get()) != kEccKeyBitsLen) {
            ret = BN_rand_range(private_keys_.get()[i].get(), order.get());
            if (ret != 1) {
                throw_openssl_error();
            }
        }
        ret = BN_add_word(private_keys_.get()[i].get(), 1);
        if (ret != 1) {
            throw_openssl_error();
        }
    }
}

ECPointPtr EccCipher::hash_to_curve(const std::string& plaintext) {
    BignumPtr x = ranom_oracle(plaintext, p_);
    ECPointPtr point(EC_POINT_new(group_.get()));
    if (point == nullptr) {
        throw_openssl_error();
    }

    while (true) {
        // Checks whether y^2 is quadratic residue.
        BignumPtr y_square = compute_y_square(x);
        if (is_square(y_square)) {
            BignumPtr sqrt(BN_new());
            if (sqrt == nullptr) {
                throw_openssl_error();
            }

            // Computes the square root y.
            auto ret = BN_mod_sqrt(sqrt.get(), y_square.get(), p_.get(), bn_ctx_.get());
            if (ret == nullptr) {
                throw_openssl_error();
            }

            // Always set the sqrt be the even one.
            // For example, 4^2 = 9^2 = 3 mod 13, we will choose 4.
            if (BN_is_bit_set(sqrt.get(), 0)) {
                auto ret1 = BN_nnmod(sqrt.get(), sqrt.get(), p_.get(), bn_ctx_.get());
                if (ret1 != 1) {
                    throw_openssl_error();
                }
                ret1 = BN_sub(sqrt.get(), p_.get(), sqrt.get());
                if (ret1 != 1) {
                    throw_openssl_error();
                }
            }

            auto ret2 =
                    EC_POINT_set_affine_coordinates_GFp(group_.get(), point.get(), x.get(), sqrt.get(), bn_ctx_.get());
            if (ret2 != 1) {
                throw_openssl_error();
            }

            // Checks point is valid.
            bool is_on_curve = (1 == EC_POINT_is_on_curve(group_.get(), point.get(), bn_ctx_.get()));
            bool is_at_infinity = (1 == EC_POINT_is_at_infinity(group_.get(), point.get()));
            if (!is_on_curve || is_at_infinity) {
                x = ranom_oracle(bn_to_string(x), p_);
            } else {
                break;
            }
        } else {
            x = ranom_oracle(bn_to_string(x), p_);
        }
    }

    return std::move(point);
}

std::string EccCipher::export_to_string(const ECPointPtr& point) const {
    std::array<std::uint8_t, kEccPointLen> point_str;
    auto ret = EC_POINT_point2oct(
            group_.get(), point.get(), POINT_CONVERSION_COMPRESSED, point_str.data(), kEccPointLen, bn_ctx_.get());
    if (ret == 0) {
        throw_openssl_error();
    }
    return std::string(point_str.begin(), point_str.end());
}

ECPointPtr EccCipher::import_from_string(const std::string& plaintext) const {
    ECPointPtr point(EC_POINT_new(group_.get()));
    if (point == nullptr) {
        throw_openssl_error();
    }
    auto ret = EC_POINT_oct2point(group_.get(), point.get(), reinterpret_cast<const std::uint8_t*>(plaintext.data()),
            kEccPointLen, bn_ctx_.get());
    if (ret == 0) {
        throw_openssl_error();
    }
    return std::move(point);
}

std::string EccCipher::bn_to_string(const BignumPtr& bn) const {
    std::size_t length = (BN_num_bits(bn.get()) + 7) / 8;
    std::vector<std::uint8_t> tmp;
    tmp.resize(length);
    BN_bn2bin(bn.get(), tmp.data());
    return std::string(reinterpret_cast<char*>(tmp.data()), tmp.size());
}

std::string EccCipher::sha3_256_hash(const std::string& plaintext) {
    std::array<std::uint8_t, kHashDigestLen> md;
    auto ret = EVP_DigestInit_ex(evp_md_ctx_.get(), EVP_sha3_256(), nullptr);
    if (ret != 1) {
        throw_openssl_error();
    }

    ret = EVP_DigestUpdate(evp_md_ctx_.get(), plaintext.data(), plaintext.size());
    if (ret != 1) {
        throw_openssl_error();
    }

    unsigned int len;
    ret = EVP_DigestFinal_ex(evp_md_ctx_.get(), md.data(), &len);
    if (ret != 1) {
        throw_openssl_error();
    }
    return std::string(md.begin(), md.end());
}

BignumPtr EccCipher::ranom_oracle(const std::string& plaintext, const BignumPtr& max_value) {
    std::size_t output_length = BN_num_bits(max_value.get()) + kHashDigestBitsLen;
    std::size_t iter_num = (output_length + kHashDigestBitsLen - 1) / kHashDigestBitsLen;

    // We use secp256r1 and sha3_256, so the bit length of p is 256.
    // There is no need to truncate output.
    // std::size_t excess_bit_count = (iter_num * kHashDigestBitsLen) - output_length;

    BignumPtr hashed_output(BN_new());
    if (hashed_output == nullptr) {
        throw_openssl_error();
    }
    auto ret = BN_set_word(hashed_output.get(), 0);
    if (ret != 1) {
        throw_openssl_error();
    }

    BignumPtr tmp(BN_new());
    if (tmp == nullptr) {
        throw_openssl_error();
    }

    for (std::size_t i = 1; i < iter_num + 1; ++i) {
        ret = BN_lshift(hashed_output.get(), hashed_output.get(), kHashDigestBitsLen);
        if (ret != 1) {
            throw_openssl_error();
        }

        // i will no bigger than 256 in our implementation.
        std::string hash_input = std::string(1, std::uint8_t(i)) + plaintext;
        // hash(i||x)
        std::string hashed_string = sha3_256_hash(hash_input);

        auto ret2 = BN_bin2bn(reinterpret_cast<const std::uint8_t*>(hashed_string.data()),
                static_cast<int>(hashed_string.size()), tmp.get());
        if (ret2 == nullptr) {
            throw_openssl_error();
        }

        ret = BN_add(hashed_output.get(), hashed_output.get(), tmp.get());
        if (ret != 1) {
            throw_openssl_error();
        }
    }

    ret = BN_nnmod(hashed_output.get(), hashed_output.get(), max_value.get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }
    return std::move(hashed_output);
}

BignumPtr EccCipher::compute_y_square(const BignumPtr& x) {
    BignumPtr y_square(BN_new());
    BignumPtr tmp(BN_new());
    if (y_square == nullptr || tmp == nullptr) {
        throw_openssl_error();
    }

    auto ret = BN_mod_exp(y_square.get(), x.get(), three_.get(), p_.get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }

    ret = BN_mod_mul(tmp.get(), a_.get(), x.get(), p_.get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }

    ret = BN_mod_add(y_square.get(), y_square.get(), tmp.get(), p_.get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }

    ret = BN_mod_add(y_square.get(), y_square.get(), b_.get(), p_.get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }

    return std::move(y_square);
}

// gcd(m, p) = 1, x^2 = m mod p has a solution if m^((p-1)/2) = 1 mod p.
bool EccCipher::is_square(const BignumPtr& m) {
    BignumPtr residue(BN_new());
    if (residue == nullptr) {
        throw_openssl_error();
    }
    auto ret = BN_mod_exp(residue.get(), m.get(), p_minus_one_over_two_.get(), p_.get(), bn_ctx_.get());
    if (ret != 1) {
        throw_openssl_error();
    }
    return BN_is_one(residue.get());
}

}  // namespace dpca_psi
