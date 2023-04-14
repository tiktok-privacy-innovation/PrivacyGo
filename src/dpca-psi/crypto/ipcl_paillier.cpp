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

#include "dpca-psi/crypto/ipcl_utils.h"

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

std::string IpclPaillier::export_pk() const {
    if (!pk_set_) {
        throw std::logic_error("pk not set.");
    }
    std::string serialized_pk = padding_zero(ipcl_bn_2_string(*(pk_->getN())), false);
    if (enable_djn_) {
        serialized_pk += padding_zero(ipcl_bn_2_string(pk_->getHS()), true);
    }
    return serialized_pk;
}

void IpclPaillier::import_pk(const std::string& in, bool enable_djn) {
    ipcl::PublicKey pk;
    if (enable_djn) {
        if (in.size() % 3 || in.empty()) {
            throw std::logic_error("enable djn, invalid pk.");
        }
        std::size_t n_bytes = in.size() / 3;
        pk.create(decode(std::string(in, 0, n_bytes)), static_cast<int>(n_bytes * 8), decode(std::string(in, n_bytes)),
                static_cast<int>(n_bytes * 4));
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

std::string IpclPaillier::export_sk() const {
    if (!sk_set_) {
        throw std::logic_error("sk not set.");
    }
    std::string encoded_n = encode(*(sk_->getN()), false);

    std::size_t pq_bytes = get_bytes_len(false) / 2;
    std::string encoded_p = ipcl_bn_2_string(*(sk_->getP()));
    std::string encoded_q = ipcl_bn_2_string(*(sk_->getQ()));
    encoded_p += std::string(pq_bytes - encoded_p.size(), '\x00');
    encoded_q += std::string(pq_bytes - encoded_q.size(), '\x00');
    return encoded_n + encoded_p + encoded_q;
}

void IpclPaillier::import_sk(const std::string& in) {
    if (in.size() % 2 || in.empty()) {
        throw std::logic_error("invalid sk");
    }
    std::size_t half_n_bytes = in.size() / 4;
    ipcl::PrivateKey sk(decode(std::string(in, 0, 2 * half_n_bytes)),
            decode(std::string(in, 2 * half_n_bytes, half_n_bytes)), decode(std::string(in, 3 * half_n_bytes)));
    set_sk(sk);
}

std::size_t IpclPaillier::privkey_bytes(std::size_t key_bits) {
    std::size_t n_bytes = (key_bits + 7) / 8;
    return n_bytes * 2;
}

std::string IpclPaillier::encode(const BigNumber& bn, bool is_n_square) const {
    return padding_zero(ipcl_bn_2_string(bn), is_n_square);
}

BigNumber IpclPaillier::decode(const std::string& in) {
    return ipcl_string_2_bn(in);
}

std::size_t IpclPaillier::get_bytes_len(bool is_n_square) const {
    std::size_t bytes_len = (n_len_ + 7) / 8;
    return bytes_len * (1 + is_n_square);
}

std::string IpclPaillier::padding_zero(const std::string& in, bool is_n_square) const {
    std::size_t zero_num = get_bytes_len(is_n_square) - in.size();
    return in + std::string(zero_num, '\x00');
}

}  // namespace dpca_psi
