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

#include "dpca-psi/dp_cardinality_psi.h"
#include "dpca-psi/network/io_base.h"
#include "ppam/mpc/aby/aby_protocol.h"

namespace privacy_go {
namespace ppam {

using json = nlohmann::json;
using IOBase = dpca_psi::IOBase;

class PrivacyMeasurement {
public:
    PrivacyMeasurement() {
        dpcpsi_op_ = std::make_shared<dpca_psi::DPCardinalityPSI>();
        aby_op_ = std::make_shared<ppam::AbyProtocol>();
    }

    PrivacyMeasurement(const PrivacyMeasurement& other) = delete;

    PrivacyMeasurement& operator=(const PrivacyMeasurement& other) = delete;

    ~PrivacyMeasurement() = default;

    /**
     * @brief initialize psi and mpc protocol
     * @param params: parameter configuration for psi and mpc
     * @param net: net io channel
     * @return none
     */
    void initialize(const json& params, const std::shared_ptr<IOBase>& net);

    /**
     * @brief privacy ad measurement
     * @param tf: threshold value for measurement
     * @param keys: ID, such as phone number or email
     * @param features: feature, such as age, time or personal consumption
     * @return measurement result
     */
    double measurement(const double& tf, const std::vector<std::vector<std::string>>& keys,
            const std::vector<std::vector<double>>& features);

    /**
     * @brief plain ad measurement, used for correctness verification
     * @param tf: threshold value for measurement
     * @param keys: ID, such as phone number or email
     * @param features: feature, such as age, time or personal consumption
     * @return measurement result
     */
    double plain_measurement(const double& tf, const std::vector<std::vector<std::string>>& keys,
            const std::vector<std::vector<double>>& features);

private:
    std::shared_ptr<dpca_psi::DPCardinalityPSI> dpcpsi_op_ = nullptr;
    std::shared_ptr<ppam::AbyProtocol> aby_op_ = nullptr;
};

}  // namespace ppam
}  // namespace privacy_go
