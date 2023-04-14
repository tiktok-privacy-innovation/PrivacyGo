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

#include <stdexcept>

#include "gtest/gtest.h"
#include "dpca-psi/common/utils.h"
#include "dpca-psi/crypto/smart_pointer.h"

namespace dpca_psi {

class EccCipherTest : public ::testing::Test {
public:
    const std::size_t bench_iter_num_ = 10000;
    const std::size_t test_iter_num_ = 10;
    static const std::size_t curve_id_ = NID_X9_62_prime256v1;
    static BnCtxPtr bn_ctx_;
    static ECGroupPtr group_;

    static void SetUpTestCase() {
    }
};

BnCtxPtr EccCipherTest::bn_ctx_(BN_CTX_new());
ECGroupPtr EccCipherTest::group_(EC_GROUP_new_by_curve_name(EccCipherTest::curve_id_));

TEST_F(EccCipherTest, strong_randomness) {
    BignumPtr private_key(BN_new());
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        BignumPtr order(BN_dup(EC_GROUP_get0_order(group_.get())));
        BN_sub_word(order.get(), 1);
        BN_rand_range(private_key.get(), order.get());

        while (BN_num_bits(private_key.get()) != kEccKeyBitsLen) {
            BN_rand_range(private_key.get(), order.get());
        }
        BN_add_word(private_key.get(), 1);
        ASSERT_EQ(BN_num_bits(private_key.get()), kEccKeyBitsLen);
    }
}

TEST_F(EccCipherTest, serialization) {
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        ECPointPtr point(EC_POINT_dup(EC_GROUP_get0_generator(group_.get()), group_.get()));
        std::array<std::uint8_t, kEccPointLen> point_str;
        EC_POINT_point2oct(
                group_.get(), point.get(), POINT_CONVERSION_COMPRESSED, point_str.data(), kEccPointLen, bn_ctx_.get());
        std::string serialized_point(point_str.begin(), point_str.end());

        ECPointPtr point2(EC_POINT_new(group_.get()));
        EC_POINT_oct2point(group_.get(), point2.get(), reinterpret_cast<const std::uint8_t*>(serialized_point.data()),
                kEccPointLen, bn_ctx_.get());
        ASSERT_EQ(EC_POINT_cmp(group_.get(), point.get(), point2.get(), bn_ctx_.get()), 0);
    }
}

TEST_F(EccCipherTest, serialization_length) {
    BignumPtr bn(BN_new());
    ECPointPtr point(EC_POINT_new(group_.get()));
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        BN_rand_range(bn.get(), EC_GROUP_get0_order(group_.get()));
        EC_POINT_mul(group_.get(), point.get(), bn.get(), NULL, NULL, bn_ctx_.get());
        std::array<std::uint8_t, kEccPointLen> point_str;
        auto ret = EC_POINT_point2oct(
                group_.get(), point.get(), POINT_CONVERSION_COMPRESSED, point_str.data(), kEccPointLen, bn_ctx_.get());
        std::string serialized_point(point_str.begin(), point_str.end());
        ASSERT_EQ(ret, kEccPointLen);
        ASSERT_EQ(serialized_point.size(), kEccPointLen);
    }
}

TEST_F(EccCipherTest, bench_sha3_hash_reused) {
    EvpMdCtxPtr evp_md_ctx(EVP_MD_CTX_new());
    std::string bytes("123");
    std::array<std::uint8_t, kHashDigestLen> hash;
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        EVP_DigestInit_ex(evp_md_ctx.get(), EVP_sha3_256(), nullptr);
        EVP_DigestUpdate(evp_md_ctx.get(), bytes.data(), bytes.size());
        unsigned int md_len;
        EVP_DigestFinal_ex(evp_md_ctx.get(), hash.data(), &md_len);
    }
}

TEST_F(EccCipherTest, bench_sha3_hash) {
    EvpMdCtxPtr evp_md_ctx(EVP_MD_CTX_new());
    std::string bytes("123");
    std::array<std::uint8_t, kHashDigestLen> hash;
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        unsigned int md_len;
        EVP_Digest(bytes.data(), bytes.size(), hash.data(), &md_len, EVP_sha3_256(), NULL);
    }
}

TEST_F(EccCipherTest, compute_y_square) {
    BignumPtr p(BN_new());
    BignumPtr a(BN_new());
    BignumPtr b(BN_new());
    EC_GROUP_get_curve_GFp(group_.get(), p.get(), a.get(), b.get(), bn_ctx_.get());

    BignumPtr three(BN_new());
    BN_set_word(three.get(), 3);

    BignumPtr y_square(BN_new());
    BignumPtr tmp(BN_new());
    BignumPtr x(BN_new());
    BN_set_word(x.get(), 0);

    BN_mod_exp(y_square.get(), x.get(), three.get(), p.get(), bn_ctx_.get());
    BN_mod_mul(tmp.get(), a.get(), x.get(), p.get(), bn_ctx_.get());
    BN_mod_add(y_square.get(), y_square.get(), tmp.get(), p.get(), bn_ctx_.get());
    BN_mod_add(y_square.get(), y_square.get(), b.get(), p.get(), bn_ctx_.get());

    ASSERT_EQ(BN_cmp(y_square.get(), b.get()), 0);
}

TEST_F(EccCipherTest, hash_encrypt) {
    std::string email1 = "test1@google.com";
    std::string phone1 = "18818881888";

    std::string email2 = "test2@google.com";
    std::string phone2 = "18818882888";

    EccCipher cipher(curve_id_, 2);
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        std::string encrypted_email0 = cipher.hash_encrypt(email1, 0);
        std::string encrypted_email1 = cipher.hash_encrypt(email1, 0);
        std::string encrypted_email2 = cipher.hash_encrypt(email2, 0);
        std::string encrypted_email3 = cipher.hash_encrypt(email1, 1);

        std::string encrypted_phone0 = cipher.hash_encrypt(phone1, 0);
        std::string encrypted_phone1 = cipher.hash_encrypt(phone1, 0);
        std::string encrypted_phone2 = cipher.hash_encrypt(phone2, 0);
        std::string encrypted_phone3 = cipher.hash_encrypt(phone1, 1);

        ASSERT_EQ(encrypted_email0, encrypted_email1);
        ASSERT_NE(encrypted_email1, encrypted_email2);
        ASSERT_NE(encrypted_email0, encrypted_email3);

        ASSERT_EQ(encrypted_phone0, encrypted_phone1);
        ASSERT_NE(encrypted_phone1, encrypted_phone2);
        ASSERT_NE(encrypted_phone0, encrypted_phone3);
    }
}

TEST_F(EccCipherTest, encrypt) {
    std::string email1 = "test1@google.com";
    std::string phone1 = "18818881888";

    std::string email2 = "test2@google.com";
    std::string phone2 = "18818882888";

    EccCipher cipher1(curve_id_, 2);
    EccCipher cipher2(curve_id_, 2);

    std::string encrypted_email0 = cipher1.hash_encrypt(email1, 0);
    std::string encrypted_email1 = cipher1.hash_encrypt(email1, 0);
    std::string encrypted_email2 = cipher1.hash_encrypt(email2, 0);
    std::string encrypted_email3 = cipher1.hash_encrypt(email1, 1);

    std::string encrypted_phone0 = cipher1.hash_encrypt(phone1, 0);
    std::string encrypted_phone1 = cipher1.hash_encrypt(phone1, 0);
    std::string encrypted_phone2 = cipher1.hash_encrypt(phone2, 0);
    std::string encrypted_phone3 = cipher1.hash_encrypt(phone1, 1);

    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        std::string re_encrypted_email0 = cipher2.encrypt(encrypted_email0, 0);
        std::string re_encrypted_email1 = cipher2.encrypt(encrypted_email1, 0);
        std::string re_encrypted_email2 = cipher2.encrypt(encrypted_email2, 0);
        std::string re_encrypted_email3 = cipher2.encrypt(encrypted_email0, 1);

        std::string re_encrypted_phone0 = cipher2.encrypt(encrypted_phone0, 0);
        std::string re_encrypted_phone1 = cipher2.encrypt(encrypted_phone1, 0);
        std::string re_encrypted_phone2 = cipher2.encrypt(encrypted_phone2, 0);
        std::string re_encrypted_phone3 = cipher2.encrypt(encrypted_phone0, 1);

        ASSERT_EQ(re_encrypted_email0, re_encrypted_email1);
        ASSERT_NE(re_encrypted_email1, re_encrypted_email2);
        ASSERT_NE(re_encrypted_email0, re_encrypted_email3);

        ASSERT_EQ(re_encrypted_phone0, re_encrypted_phone1);
        ASSERT_NE(re_encrypted_phone1, re_encrypted_phone2);
        ASSERT_NE(re_encrypted_phone0, re_encrypted_phone3);
    }
}

TEST_F(EccCipherTest, encrypt_and_div) {
    std::string email1 = "test1@google.com";

    EccCipher cipher1(curve_id_, 2);

    std::string encrypted_email0 = cipher1.hash_encrypt(email1, 0);
    std::string encrypted_email1 = cipher1.hash_encrypt(email1, 1);

    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        std::string re_encrypted_email1 = cipher1.encrypt_and_div(encrypted_email0, 1, 0);
        ASSERT_EQ(encrypted_email1, re_encrypted_email1);
    }
}

TEST_F(EccCipherTest, diffie_hellman) {
    std::string sender_email1 = "test1@google.com";
    std::string sender_phone1 = "18818881888";

    std::string sender_email2 = "test2@google.com";
    std::string sender_phone2 = "18818882888";

    std::string receiver_email1 = "test1@google.com";
    std::string receiver_phone1 = "18818881888";

    EccCipher sender(curve_id_, 2);
    EccCipher receiver(curve_id_, 2);

    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        std::string sender_encrypted_email1 = sender.hash_encrypt(sender_email1, 0);
        std::string sender_encrypted_email2 = sender.hash_encrypt(sender_email2, 0);
        std::string sender_encrypted_phone1 = sender.hash_encrypt(sender_phone1, 0);
        std::string sender_encrypted_phone2 = sender.hash_encrypt(sender_phone2, 0);

        std::string exchanged_sender_encrypted_email1 = receiver.encrypt(sender_encrypted_email1, 0);
        std::string exchanged_sender_encrypted_email2 = receiver.encrypt(sender_encrypted_email2, 0);
        std::string exchanged_sender_encrypted_phone1 = receiver.encrypt(sender_encrypted_phone1, 0);
        std::string exchanged_sender_encrypted_phone2 = receiver.encrypt(sender_encrypted_phone2, 0);

        std::string receiver_encrypted_email1 = receiver.hash_encrypt(receiver_email1, 0);
        std::string receiver_encrypted_phone1 = receiver.hash_encrypt(receiver_phone1, 0);

        std::string exchanged_receiver_encrypted_email1 = sender.encrypt(receiver_encrypted_email1, 0);
        std::string exchanged_receiver_encrypted_phone1 = sender.encrypt(receiver_encrypted_phone1, 0);

        ASSERT_EQ(exchanged_sender_encrypted_email1, exchanged_receiver_encrypted_email1);
        ASSERT_NE(exchanged_sender_encrypted_email1, exchanged_sender_encrypted_email2);

        ASSERT_EQ(exchanged_sender_encrypted_phone1, exchanged_receiver_encrypted_phone1);
        ASSERT_NE(exchanged_sender_encrypted_phone1, exchanged_sender_encrypted_phone2);
    }
}

}  // namespace dpca_psi
