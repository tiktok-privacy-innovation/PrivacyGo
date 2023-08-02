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

#include <memory>

#include "ppam/mpc/beaver/bool_triplet_generator.h"
#include "ppam/mpc/common/defines.h"
#include "ppam/mpc/common/pseudo_rand_generator.h"

namespace privacy_go {
namespace ppam {

class AbyProtocol {
public:
    AbyProtocol() = default;

    AbyProtocol(const AbyProtocol& other) = delete;

    AbyProtocol& operator=(const AbyProtocol& other) = delete;

    ~AbyProtocol() = default;

    static AbyProtocol* Instance() {
        static AbyProtocol aby_protocol;
        return &aby_protocol;
    }

    int set_seed();

    int initialize(const std::size_t party_id, const std::shared_ptr<IOBase>& net_io);

    int release();

    int share(const std::size_t party, const eMatrix<double>& in, CryptoMatrix& out);

    int reveal(const std::size_t party, const CryptoMatrix& in, eMatrix<double>& out);

    int add(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z);

    int sub(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z);

    int elementwise_bool_mul(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z);

    int kogge_stone_ppa(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z);

    int a2b(const CryptoMatrix& x, CryptoMatrix& z);

    int greater(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z);

    int greater(const CryptoMatrix& x, const eMatrix<double>& y, CryptoMatrix& z);

    int less(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z);

    int less(const CryptoMatrix& x, const eMatrix<double>& y, CryptoMatrix& z);

    int sum(const CryptoMatrix& in, CryptoMatrix& out);

    int multiplexer(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z);

    int attribution(const double& tf, const CryptoMatrix& in, CryptoMatrix& out);

    std::shared_ptr<IOBase> get_io_channel() {
        return net_io_;
    }

private:
    inline int64_t float_to_fixed(const double input) {
        return static_cast<int64_t>(input * ((int64_t)1 << 16));
    }

    inline double fixed_to_float(const int64_t input) {
        return static_cast<double>(input) / ((int64_t)1 << 16);
    }

    std::shared_ptr<IOBase> net_io_ = nullptr;
    std::shared_ptr<PseudoRandGenerator> rand_generator_ = nullptr;
    std::shared_ptr<BoolTripletGenerator> rand_bool_triplet_generator_ = nullptr;
    std::shared_ptr<ObliviousTransfer> oblivious_transfer_ = nullptr;
    std::size_t party_id_ = 0;
};

}  // namespace ppam
}  // namespace privacy_go
