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
#include <vector>

#include "dpca-psi/network/io_base.h"
#include "ppam/ppam.h"

namespace privacy_go {
namespace mpc_dualdp {

using IOBase = dpca_psi::IOBase;

class MPCDualDP {
public:
    MPCDualDP() {
        aby_op_ = std::make_shared<ppam::AbyProtocol>();
    }

    MPCDualDP(const MPCDualDP& other) = delete;

    MPCDualDP& operator=(const MPCDualDP& other) = delete;

    ~MPCDualDP() = default;

    /**
     * @brief Initialize psi and mpc protocol.
     * @param[in] party_id The id of party.
     * @param[in] net Net io channel.
     * @return None.
     */
    void initialize(const std::size_t party_id, const std::shared_ptr<IOBase>& net);

    /**
     * @brief MPC-DualDP protocol.
     * @param[in] n Amount of noise to be generated.
     * @param[in] epsilon DP parameter $epsilon$.
     * @param[in] delta DP parameter $delta$.
     * @param[in] sensitivity DP parameter $sensitivity$.
     * @param[out] noise DP noise secret shares.
     * @return none
     */
    void binomial_sampling(const std::size_t n, const double epsilon, const double delta, const double sensitivity,
            std::vector<int64_t>& noise);

private:
    std::size_t party_id_ = 0;

    std::shared_ptr<IOBase> net_ = nullptr;

    std::shared_ptr<ppam::AbyProtocol> aby_op_ = nullptr;

    std::shared_ptr<ppam::PseudoRandGenerator> rand_generator_ = nullptr;

    std::shared_ptr<ppam::ObliviousTransfer> oblivious_transfer_ = nullptr;
};

}  // namespace mpc_dualdp
}  // namespace privacy_go
