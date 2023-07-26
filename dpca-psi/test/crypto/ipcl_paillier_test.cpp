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

#include "dpca-psi/crypto/ipcl_paillier.h"

#include <vector>

#include "gtest/gtest.h"
#include "ipcl/utils/common.hpp"

#include "dpca-psi/crypto/ipcl_utils.h"

namespace privacy_go {
namespace dpca_psi {

class IpclPaillierTest : public ::testing::Test {
public:
    static const std::size_t test_iter_num_ = 10;
    static const std::size_t bench_iter_num_ = 1e3;
    static std::size_t n_len_;
    static IpclPaillier pai_;
    static IpclPaillier pai_without_djn_;
    static const std::size_t bench_keygen_ = 1e1;
    static const std::vector<int> bits_vec_;
    static const std::size_t bench_vec_num_values_ = 8;

    static void SetUpTestCase() {
        pai_.keygen(n_len_, true);
        pai_without_djn_.keygen(n_len_, false);
    }
};

std::size_t IpclPaillierTest::n_len_(2048);
IpclPaillier IpclPaillierTest::pai_;
IpclPaillier IpclPaillierTest::pai_without_djn_;
const std::vector<int> IpclPaillierTest::bits_vec_ = {2, 31, 32, 482, 511, 512, 994, 1023, 1024, 2018, 2047, 2048};

TEST_F(IpclPaillierTest, test_enc_dec) {
    std::vector<BigNumber> bn;
    for (std::size_t i = 0; i < bits_vec_.size(); ++i) {
        bn.push_back(ipcl::getRandomBN(bits_vec_[i]));
    }
    ipcl::PlainText plain(bn);
    auto ct = pai_.encrypt(plain);
    auto pt = pai_.decrypt(ct);
    for (std::size_t i = 0; i < pt.getSize(); ++i) {
        EXPECT_EQ(plain.getElement(i) % pai_.n(), pt.getElement(i));
    }
}

TEST_F(IpclPaillierTest, test_add) {
    std::vector<BigNumber> bn0;
    std::vector<BigNumber> bn1;
    for (std::size_t i = 0; i < bits_vec_.size(); ++i) {
        bn0.push_back(ipcl::getRandomBN(bits_vec_[i]));
        bn1.push_back(ipcl::getRandomBN(bits_vec_[i]));
    }
    ipcl::PlainText pt0(bn0);
    ipcl::PlainText pt1(bn1);
    auto ct0 = pai_.encrypt(pt0);
    auto ct1 = pai_.encrypt(pt1);
    auto ct = pai_.add(ct0, ct1);
    auto pt = pai_.decrypt(ct);
    for (std::size_t i = 0; i < pt.getSize(); ++i) {
        EXPECT_EQ((bn0[i] + bn1[i]) % pai_.n(), pt.getElement(i));
    }
}

TEST_F(IpclPaillierTest, test_mult) {
    std::vector<BigNumber> bn0;
    std::vector<BigNumber> bn1;
    for (std::size_t i = 0; i < bits_vec_.size(); ++i) {
        bn0.push_back(ipcl::getRandomBN(bits_vec_[i]));
        bn1.push_back(ipcl::getRandomBN(bits_vec_[i]));
    }
    ipcl::PlainText pt0(bn0);
    ipcl::PlainText pt1(bn1);
    auto ct0 = pai_.encrypt(pt0);
    auto ct = pai_.mult(ct0, pt1);
    auto pt = pai_.decrypt(ct);
    for (std::size_t i = 0; i < pt.getSize(); ++i) {
        EXPECT_EQ((bn0[i] * bn1[i]) % pai_.n(), pt.getElement(i));
    }
}

TEST_F(IpclPaillierTest, test_pk) {
    auto serilized_pk = pai_.export_pk();
    EXPECT_EQ(serilized_pk.size(), IpclPaillier::pubkey_bytes(n_len_, true));
    IpclPaillier pai;
    pai.import_pk(serilized_pk, true);
    BigNumber bn = ipcl::getRandomBN(32);
    auto ct = pai.encrypt(ipcl::PlainText(bn));
    auto pt = pai_.decrypt(ct);
    EXPECT_EQ(bn, pt.getElement(0));
}

TEST_F(IpclPaillierTest, test_pk_without_djn) {
    auto serilized_pk = pai_without_djn_.export_pk();
    EXPECT_EQ(serilized_pk.size(), IpclPaillier::pubkey_bytes(n_len_, false));
    IpclPaillier pai;
    pai.import_pk(serilized_pk, false);
    BigNumber bn = ipcl::getRandomBN(32);
    auto ct = pai.encrypt(ipcl::PlainText(bn));
    auto pt = pai_without_djn_.decrypt(ct);
    EXPECT_EQ(bn, pt.getElement(0));
}

TEST_F(IpclPaillierTest, test_sk) {
    auto serilized_sk = pai_.export_sk();
    EXPECT_EQ(serilized_sk.size(), IpclPaillier::privkey_bytes(n_len_));
    IpclPaillier pai;
    pai.import_sk(serilized_sk);
    BigNumber bn = ipcl::getRandomBN(32);
    auto ct = pai_.encrypt(ipcl::PlainText(bn));
    auto pt = pai.decrypt(ct);
    EXPECT_EQ(bn, pt.getElement(0));
}

TEST_F(IpclPaillierTest, test_pk_sk_bytes) {
    EXPECT_EQ(384u, IpclPaillier::pubkey_bytes(1024, true));
    EXPECT_EQ(128u, IpclPaillier::pubkey_bytes(1024, false));
    EXPECT_EQ(256u, IpclPaillier::privkey_bytes(1024));
    EXPECT_EQ(387u, IpclPaillier::pubkey_bytes(1025, true));
    EXPECT_EQ(129u, IpclPaillier::pubkey_bytes(1025, false));
    EXPECT_EQ(258u, IpclPaillier::privkey_bytes(1025));
    EXPECT_EQ(0u, IpclPaillier::pubkey_bytes(0, true));
    EXPECT_EQ(0u, IpclPaillier::pubkey_bytes(0, false));
    EXPECT_EQ(0u, IpclPaillier::privkey_bytes(0));
}

TEST_F(IpclPaillierTest, test_copy) {
    IpclPaillier pai(pai_);
    BigNumber bn = ipcl::getRandomBN(32);
    auto ct = pai_.encrypt(ipcl::PlainText(bn));
    auto pt = pai.decrypt(ct);
    EXPECT_EQ(bn, pt.getElement(0));

    ct = pai.encrypt(ipcl::PlainText(bn));
    pt = pai_.decrypt(ct);
    EXPECT_EQ(bn, pt.getElement(0));
}

TEST_F(IpclPaillierTest, test_assign) {
    IpclPaillier pai;
    pai = pai_;
    BigNumber bn = ipcl::getRandomBN(32);
    auto ct = pai_.encrypt(ipcl::PlainText(bn));
    auto pt = pai.decrypt(ct);
    EXPECT_EQ(bn, pt.getElement(0));

    ct = pai.encrypt(ipcl::PlainText(bn));
    pt = pai_.decrypt(ct);
    EXPECT_EQ(bn, pt.getElement(0));
}

TEST_F(IpclPaillierTest, test_invalid_sk) {
    IpclPaillier pai;
    EXPECT_THROW(pai.export_sk(), std::logic_error);
    EXPECT_THROW(pai.import_sk(ByteVector{}), std::logic_error);
    EXPECT_THROW(pai_.import_sk(ByteVector{Byte(1), Byte(2), Byte(3)}), std::logic_error);
    EXPECT_THROW(pai_.import_sk(ByteVector{Byte('\x08'), Byte('\x02'), Byte('\x0f')}), std::logic_error);
}

TEST_F(IpclPaillierTest, test_invalid_pk) {
    IpclPaillier pai;
    EXPECT_THROW(pai.export_pk(), std::logic_error);
    EXPECT_THROW(pai.import_pk(ByteVector{}, true), std::logic_error);
    EXPECT_THROW(pai_.import_pk(ByteVector{Byte('\x03'), Byte('\x01')}, true), std::logic_error);
}

TEST_F(IpclPaillierTest, bn_string_conversion) {
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        for (std::size_t j = 0; j < bits_vec_.size(); ++j) {
            BigNumber bn = ipcl::getRandomBN(bits_vec_[j]);
            ByteVector serialized_bytes;
            ipcl_bn_2_bytes(bn, serialized_bytes);
            BigNumber deserialized_bn;
            ipcl_bytes_2_bn(serialized_bytes, deserialized_bn);
            EXPECT_TRUE(bn == deserialized_bn);
        }
    }
}

TEST_F(IpclPaillierTest, bn_lshift) {
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        for (std::size_t j = 0; j < bits_vec_.size(); ++j) {
            BigNumber bn(0x1);
            ipcl_bn_lshift(bn, bits_vec_[j]);
        }
    }
}

TEST_F(IpclPaillierTest, bn_2_u64) {
    for (std::size_t i = 0; i < test_iter_num_; ++i) {
        BigNumber bn = ipcl::getRandomBN(64);
        std::uint64_t value_u64 = ipcl_bn_2_u64(bn);
        EXPECT_EQ(ipcl_u64_2_bn(value_u64), bn);
    }
}

TEST_F(IpclPaillierTest, bench_enc_small) {
    std::vector<BigNumber> bn;
    for (std::size_t i = 0; i < bench_vec_num_values_; ++i) {
        bn.push_back(ipcl::getRandomBN(32));
    }
    ipcl::PlainText pt(bn);
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        pai_.encrypt(pt);
    }
}

TEST_F(IpclPaillierTest, bench_enc_mid) {
    std::vector<BigNumber> bn;
    for (std::size_t i = 0; i < bench_vec_num_values_; ++i) {
        bn.push_back(ipcl::getRandomBN(static_cast<int>(n_len_ / 2)));
    }
    ipcl::PlainText pt(bn);
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        pai_.encrypt(pt);
    }
}

TEST_F(IpclPaillierTest, bench_enc_large) {
    std::vector<BigNumber> bn;
    for (std::size_t i = 0; i < bench_vec_num_values_; ++i) {
        bn.push_back(ipcl::getRandomBN(static_cast<int>(n_len_)));
    }
    ipcl::PlainText pt(bn);
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        pai_.encrypt(pt);
    }
}

TEST_F(IpclPaillierTest, bench_dec) {
    std::vector<BigNumber> bn;
    for (std::size_t i = 0; i < bench_vec_num_values_; ++i) {
        bn.push_back(ipcl::getRandomBN(32));
    }
    ipcl::PlainText pt(bn);
    auto ct = pai_.encrypt(pt);
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        pai_.decrypt(ct);
    }
}

TEST_F(IpclPaillierTest, bench_add) {
    std::vector<BigNumber> bn0;
    std::vector<BigNumber> bn1;
    for (std::size_t i = 0; i < bench_vec_num_values_; ++i) {
        bn0.push_back(ipcl::getRandomBN(32));
        bn1.push_back(ipcl::getRandomBN(32));
    }
    ipcl::PlainText pt0(bn0);
    ipcl::PlainText pt1(bn1);
    auto ct0 = pai_.encrypt(pt0);
    auto ct1 = pai_.encrypt(pt1);
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        pai_.add(ct0, ct1);
    }
}

TEST_F(IpclPaillierTest, bench_mul) {
    std::vector<BigNumber> bn0;
    std::vector<BigNumber> bn1;
    for (std::size_t i = 0; i < bench_vec_num_values_; ++i) {
        bn0.push_back(ipcl::getRandomBN(32));
        bn1.push_back(ipcl::getRandomBN(32));
    }
    ipcl::PlainText pt0(bn0);
    ipcl::PlainText pt1(bn1);
    auto ct0 = pai_.encrypt(pt0);
    for (std::size_t i = 0; i < bench_iter_num_; ++i) {
        pai_.mult(ct0, pt1);
    }
}

TEST_F(IpclPaillierTest, bench_keygen) {
    IpclPaillier pai;
    for (std::size_t i = 0; i < bench_keygen_; ++i) {
        pai.keygen(n_len_, false);
    }
}

TEST_F(IpclPaillierTest, bench_keygen_djn) {
    IpclPaillier pai;
    for (std::size_t i = 0; i < bench_keygen_; ++i) {
        pai.keygen(n_len_, true);
    }
}

}  // namespace dpca_psi
}  // namespace privacy_go
