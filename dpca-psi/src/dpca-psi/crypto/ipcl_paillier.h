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

#include <memory>
#include <string>

#include "ipcl/bignum.h"
#include "ipcl/ciphertext.hpp"
#include "ipcl/ipcl.hpp"
#include "ipcl/plaintext.hpp"
#include "ipcl/pri_key.hpp"
#include "ipcl/pub_key.hpp"

#include "dpca-psi/common/defines.h"

namespace privacy_go {
namespace dpca_psi {

// Wrapper class for Intel Paillier Cryptosystem Library(IPCL).
// IPCL is certified for ISO compliance.
// Basic implementation details refers to https://github.com/intel/pailliercryptolib.
class IpclPaillier {
public:
    IpclPaillier();

    explicit IpclPaillier(const IpclPaillier& other);

    IpclPaillier& operator=(const IpclPaillier& rhs);

    // Generates pk and sk given the bits length of n.
    // More details refer to https://github.com/intel/pailliercryptolib/blob/development/ipcl/keygen.cpp.
    void keygen(std::size_t n_len, bool enable_djn);

    // If DJN optimiztion is enabled, c = (1 + n * m) * (hs) ^ r mod n^2.
    // Otherwise, c = (1 + n * m) * (r ^ n) mod n^2.
    // Returns encrypted cipherText.
    ipcl::CipherText encrypt(const ipcl::PlainText& plain) const;

    // CRT optimization is used by default.
    // Returns decrypted plaintext.
    ipcl::PlainText decrypt(const ipcl::CipherText& cipher) const;

    // Homomorphic addition, cipher0 + cipher1.
    ipcl::CipherText add(const ipcl::CipherText& cipher0, const ipcl::CipherText& cipher1) const;

    // Homomorphic addition, cipher + plain
    ipcl::CipherText add(const ipcl::CipherText& cipher, const ipcl::PlainText& plain) const;

    // Homomorphic multiplication, cipher * plain.
    ipcl::CipherText mult(const ipcl::CipherText& cipher, const ipcl::PlainText& plain) const;

    // If DJN optimiztion is enabled, the public key is (n, hs), otherwise, the public key is (n).
    // Returns serialized pk.
    ByteVector export_pk() const;

    // Deserializes pk.
    void import_pk(const ByteVector& in, bool enable_djn);

    // Returns the bytes length of pubkey given the bits length of key(n) .
    static std::size_t pubkey_bytes(std::size_t key_bits, bool enable_djn);

    // Returns serialized sk in the form of (n, p, q).
    ByteVector export_sk() const;

    // Deserializes sk.
    void import_sk(const ByteVector& in);

    // Returns the bytes length of privkey given the bits length of key(n).
    static std::size_t privkey_bytes(std::size_t key_bits);

    // Encodes the BigNumber bn into bytes and padding zero if the length of bn is not same with n or n^2.
    // Returns serialized cipher.
    ByteVector encode(const BigNumber& bn, bool is_n_square) const;

    // Returns deserialized BigNumber.
    static BigNumber decode(const ByteVector& in);

    // Returns bytes length of n^(1+n_square).
    std::size_t get_bytes_len(bool is_n_square) const;

    BigNumber n() const {
        return BigNumber(*(pk_->getN()));
    }

    std::shared_ptr<ipcl::PublicKey> get_pk() const {
        return pk_;
    }

    ~IpclPaillier() {
    }

private:
    // Padding zero to the end of input bytes.
    void padding_zero(ByteVector& in, bool is_n_square) const;

    // Sets public key. If DJN optimiztion is enabled, will also set hs.
    void set_pk(const ipcl::PublicKey& pk, bool enable_djn);

    // Sets private key.
    void set_sk(const ipcl::PrivateKey& sk);

    std::shared_ptr<ipcl::PublicKey> pk_ = nullptr;

    std::shared_ptr<ipcl::PrivateKey> sk_ = nullptr;

    std::size_t n_len_;
    bool pk_set_;
    bool sk_set_;
    bool enable_djn_;
};

}  // namespace dpca_psi
}  // namespace privacy_go
