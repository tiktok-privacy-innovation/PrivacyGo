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

#include <algorithm>
#include <vector>

#include "dpca-psi/crypto/ipcl_utils.h"

namespace privacy_go {
namespace dpca_psi {

IpclPaillier::IpclPaillier() : n_len_(0), pk_set_(false), sk_set_(false), enable_djn_(false) {
}

IpclPaillier::IpclPaillier(const IpclPaillier& other) : pk_set_(false), sk_set_(false) {
    if (other.sk_set_) {
        set_sk(*(other.sk_));
        set_pk(*(other.pk_), other.enable_djn_);
    } else if (other.pk_set_) {
        set_pk(*(other.pk_), other.enable_djn_);
    }
}

IpclPaillier& IpclPaillier::operator=(const IpclPaillier& other) {
    sk_set_ = false;
    pk_set_ = false;
    if (other.sk_set_) {
        set_sk(*(other.sk_));
        set_pk(*(other.pk_), other.enable_djn_);
    } else if (other.pk_set_) {
        set_pk(*(other.pk_), other.enable_djn_);
    }
    return *this;
}

void IpclPaillier::keygen(std::size_t n_len, bool enable_djn) {
    if (n_len < 1024) {
        throw std::logic_error("Paillier key length is too short");
    }
    n_len_ = n_len;
    enable_djn_ = enable_djn;
    ipcl::KeyPair key_pair = ipcl::generateKeypair(n_len_, enable_djn);
    set_pk(key_pair.pub_key, enable_djn);
    set_sk(key_pair.priv_key);
}

void IpclPaillier::set_pk(const ipcl::PublicKey& pk, bool enable_djn) {
    pk_ = std::make_shared<ipcl::PublicKey>();
    if (enable_djn) {
        pk_->create(*(pk.getN()), pk.getBits(), pk.getHS(), pk.getRandBits());
    } else {
        pk_->create(*(pk.getN()), pk.getBits(), false);
    }
    n_len_ = pk.getBits();
    enable_djn_ = enable_djn;
    pk_set_ = true;
}

void IpclPaillier::set_sk(const ipcl::PrivateKey& sk) {
    sk_ = std::make_shared<ipcl::PrivateKey>(*(sk.getN()), *(sk.getP()), *(sk.getQ()));
    sk_set_ = true;
}

ipcl::CipherText IpclPaillier::encrypt(const ipcl::PlainText& plain) const {
    ipcl::setHybridMode(ipcl::HybridMode::IPP);
    ipcl::CipherText cipher = pk_->encrypt(plain, true);
    ipcl::setHybridOff();
    return cipher;
}

ipcl::PlainText IpclPaillier::decrypt(const ipcl::CipherText& cipher) const {
    ipcl::setHybridMode(ipcl::HybridMode::IPP);
    ipcl::PlainText plain = sk_->decrypt(cipher);
    ipcl::setHybridOff();
    return plain;
}

ipcl::CipherText IpclPaillier::add(const ipcl::CipherText& cipher0, const ipcl::CipherText& cipher1) const {
    return cipher0 + cipher1;
}

ipcl::CipherText IpclPaillier::add(const ipcl::CipherText& cipher, const ipcl::PlainText& plain) const {
    return cipher + plain;
}

ipcl::CipherText IpclPaillier::mult(const ipcl::CipherText& cipher, const ipcl::PlainText& plain) const {
    ipcl::setHybridMode(ipcl::HybridMode::IPP);
    auto result = cipher * plain;
    ipcl::setHybridOff();
    return result;
}

ByteVector IpclPaillier::export_pk() const {
    if (!pk_set_) {
        throw std::logic_error("pk not set.");
    }
    ByteVector serialized_pk;
    ipcl_bn_2_bytes(*(pk_->getN()), serialized_pk);
    padding_zero(serialized_pk, false);
    if (enable_djn_) {
        ByteVector serialized_hs;
        ipcl_bn_2_bytes(pk_->getHS(), serialized_hs);
        padding_zero(serialized_hs, true);
        serialized_pk.insert(serialized_pk.end(), serialized_hs.begin(), serialized_hs.end());
    }
    return serialized_pk;
}

void IpclPaillier::import_pk(const ByteVector& in, bool enable_djn) {
    ipcl::PublicKey pk;
    if (enable_djn) {
        if (in.size() % 3 || in.empty()) {
            throw std::logic_error("enable djn, invalid pk.");
        }
        std::size_t n_bytes = in.size() / 3;
        ByteVector n(n_bytes);
        std::copy_n(in.begin(), n_bytes, n.begin());
        ByteVector hs(2 * n_bytes);
        std::copy_n(in.begin() + n_bytes, 2 * n_bytes, hs.begin());
        pk.create(decode(n), static_cast<int>(n_bytes * 8), decode(hs), static_cast<int>(n_bytes * 4));
    } else {
        if (in.empty()) {
            throw std::logic_error("invalid pk.");
        }
        pk.create(decode(in), static_cast<int>(in.size() * 8), false);
    }
    set_pk(pk, enable_djn);
}

std::size_t IpclPaillier::pubkey_bytes(std::size_t key_bits, bool enable_djn) {
    std::size_t n_bytes = (key_bits + 7) / 8;
    return n_bytes * (1 + 2 * enable_djn);
}

ByteVector IpclPaillier::export_sk() const {
    if (!sk_set_) {
        throw std::logic_error("sk not set.");
    }
    ByteVector encoded_n = encode(*(sk_->getN()), false);

    std::size_t pq_bytes = get_bytes_len(false) / 2;
    ByteVector encoded_p;
    ipcl_bn_2_bytes(*(sk_->getP()), encoded_p);
    ByteVector encoded_q;
    ipcl_bn_2_bytes(*(sk_->getQ()), encoded_q);
    std::fill_n(std::back_inserter(encoded_p), pq_bytes - encoded_p.size(), Byte('\x00'));
    std::fill_n(std::back_inserter(encoded_q), pq_bytes - encoded_q.size(), Byte('\x00'));
    encoded_n.insert(encoded_n.end(), encoded_p.begin(), encoded_p.end());
    encoded_n.insert(encoded_n.end(), encoded_q.begin(), encoded_q.end());

    return encoded_n;
}

void IpclPaillier::import_sk(const ByteVector& in) {
    if (in.size() % 2 || in.empty()) {
        throw std::logic_error("invalid sk");
    }
    std::size_t half_n_bytes = in.size() / 4;

    ByteVector n(2 * half_n_bytes);
    ByteVector p(half_n_bytes);
    ByteVector q(half_n_bytes);
    std::copy_n(in.begin(), 2 * half_n_bytes, n.begin());
    std::copy_n(in.begin() + 2 * half_n_bytes, half_n_bytes, p.begin());
    std::copy_n(in.begin() + 3 * half_n_bytes, half_n_bytes, q.begin());

    ipcl::PrivateKey sk(decode(n), decode(p), decode(q));
    set_sk(sk);
}

std::size_t IpclPaillier::privkey_bytes(std::size_t key_bits) {
    std::size_t n_bytes = (key_bits + 7) / 8;
    return n_bytes * 2;
}

ByteVector IpclPaillier::encode(const BigNumber& bn, bool is_n_square) const {
    ByteVector out;
    ipcl_bn_2_bytes(bn, out);
    padding_zero(out, is_n_square);
    return out;
}

BigNumber IpclPaillier::decode(const ByteVector& in) {
    BigNumber out;
    ipcl_bytes_2_bn(in, out);
    return out;
}

std::size_t IpclPaillier::get_bytes_len(bool is_n_square) const {
    std::size_t bytes_len = (n_len_ + 7) / 8;
    return bytes_len * (1 + is_n_square);
}

void IpclPaillier::padding_zero(ByteVector& in, bool is_n_square) const {
    std::size_t zero_num = get_bytes_len(is_n_square) - in.size();
    for (std::size_t idx = 0; idx < zero_num; ++idx) {
        in.push_back(Byte('\x00'));
    }
}

}  // namespace dpca_psi
}  // namespace privacy_go
