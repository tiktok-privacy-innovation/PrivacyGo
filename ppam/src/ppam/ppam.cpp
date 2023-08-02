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

#include "ppam/ppam.h"

#include <string>

namespace privacy_go {
namespace ppam {

void PrivacyMeasurement::initialize(const json& params, const std::shared_ptr<IOBase>& net) {
    std::string config;
    bool is_sender_ = params["common"]["is_sender"];

    aby_op_->initialize(is_sender_, net);
    dpcpsi_op_->init(params, net);
}

double PrivacyMeasurement::measurement(const double& tf, const std::vector<std::vector<std::string>>& keys,
        const std::vector<std::vector<double>>& features) {
    std::vector<std::vector<std::uint64_t>> fixed_features(
            features.size(), std::vector<std::uint64_t>(features[0].size()));
    for (std::size_t i = 0; i < features.size(); i++) {
        for (std::size_t j = 0; j < features[i].size(); j++) {
            fixed_features[i][j] = static_cast<std::uint64_t>(features[i][j] * ((std::uint64_t)1 << 16));
        }
    }

    dpcpsi_op_->data_sampling(keys, fixed_features);
    std::vector<std::vector<std::uint64_t>> shares;
    dpcpsi_op_->process(shares);

    ppam::CryptoMatrix input(shares[0].size(), shares.size());
    for (std::size_t i = 0; i < input.rows(); i++) {
        for (std::size_t j = 0; j < input.cols(); j++) {
            input.shares(i, j) = (std::int64_t)shares[j][i];
        }
    }

    ppam::CryptoMatrix output;
    aby_op_->attribution(tf, input, output);

    ppam::eMatrix<double> plain;
    aby_op_->reveal(0, output, plain);
    aby_op_->reveal(1, output, plain);

    return plain(0, 0);
}

double PrivacyMeasurement::plain_measurement(const double& tf, const std::vector<std::vector<std::string>>& keys,
        const std::vector<std::vector<double>>& features) {
    std::vector<std::vector<std::uint64_t>> fixed_features(
            features.size(), std::vector<std::uint64_t>(features[0].size()));
    for (std::size_t i = 0; i < features.size(); i++) {
        for (std::size_t j = 0; j < features[i].size(); j++) {
            fixed_features[i][j] = static_cast<std::uint64_t>(features[i][j] * ((std::uint64_t)1 << 16));
        }
    }

    dpcpsi_op_->data_sampling(keys, fixed_features);
    std::vector<std::vector<std::uint64_t>> shares;
    dpcpsi_op_->process(shares);

    ppam::CryptoMatrix input(shares[0].size(), shares.size());
    for (std::size_t i = 0; i < input.rows(); i++) {
        for (std::size_t j = 0; j < input.cols(); j++) {
            input.shares(i, j) = (std::int64_t)shares[j][i];
        }
    }

    ppam::eMatrix<double> input_plain;
    aby_op_->reveal(0, input, input_plain);
    aby_op_->reveal(1, input, input_plain);

    double plain = 0.0;
    for (int i = 0; i < input_plain.rows(); i++) {
        auto t = input_plain(i, 1) - input_plain(i, 0);
        if ((t > 0) && (t < tf)) {
            plain += input_plain(i, 2);
        }
    }

    return plain;
}

}  // namespace ppam
}  // namespace privacy_go
