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
//
// This file defines smart pointers used for openssl classes.

#pragma once

#include <openssl/ec.h>
#include <openssl/evp.h>

#include <memory>

namespace dpca_psi {

// Deletes a BIGNUM.
class BnDeleter {
public:
    void operator()(BIGNUM* bn) {
        BN_clear_free(bn);
    }
};

// Deletes a EC_GROUP.
class ECGroupDeleter {
public:
    void operator()(EC_GROUP* group) {
        EC_GROUP_free(group);
    }
};

// Deletes a BN_CTX.
class BnCtxDeleter {
public:
    void operator()(BN_CTX* ctx) {
        BN_CTX_free(ctx);
    }
};

// Deletes a EC_POINT.
class ECPointDeleter {
public:
    void operator()(EC_POINT* point) {
        EC_POINT_clear_free(point);
    }
};

// Deletes a EC_KEY.
class ECKeyDeleter {
public:
    void operator()(EC_KEY* key) {
        EC_KEY_free(key);
    }
};

// Deletes an EVP_MD_CTX.
class EvpMdCtxDeleter {
public:
    void operator()(EVP_MD_CTX* ctx) {
        EVP_MD_CTX_free(ctx);
    }
};

using BignumPtr = std::unique_ptr<BIGNUM, BnDeleter>;
using BignumArrayPtr = std::unique_ptr<BignumPtr[]>;
using ECGroupPtr = std::unique_ptr<EC_GROUP, ECGroupDeleter>;
using BnCtxPtr = std::unique_ptr<BN_CTX, BnCtxDeleter>;
using ECPointPtr = std::unique_ptr<EC_POINT, ECPointDeleter>;
using ECPointArrayPtr = std::unique_ptr<ECPointPtr[]>;
using ECKeyPtr = std::unique_ptr<EC_KEY, ECKeyDeleter>;
using EvpMdCtxPtr = std::unique_ptr<EVP_MD_CTX, EvpMdCtxDeleter>;

}  // namespace dpca_psi
