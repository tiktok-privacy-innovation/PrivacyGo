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

#include "mpc-dualdp/mpc_dualdp.h"

#include "gtest/gtest.h"

#include "ppam/mpc/aby/aby_protocol.h"
#include "ppam/mpc/common/defines.h"

namespace privacy_go {
namespace mpc_dualdp {

class MPCDualDPTest : public ::testing::Test {
public:
    void SetUp() {
        mpc_party0_ = 0;
        mpc_party1_ = 1;
    }

    void gen_noise_test(std::size_t& party) {
        std::uint16_t remote_port = (party == mpc_party0_) ? 9989 : 9990;
        std::uint16_t local_port = (party == mpc_party0_) ? 9990 : 9989;
        auto net = std::make_shared<dpca_psi::TwoChannelNetIO>("127.0.0.1", remote_port, local_port);

        auto aby_test = ppam::AbyProtocol::Instance();
        aby_test->initialize(party, net);

        auto dual_dp_test = std::make_shared<MPCDualDP>();
        dual_dp_test->initialize(party, net);

        std::size_t batch_n = 16;
        double epsilon = 1.0;
        double delta = 0.00001;
        double sensitivity = 1.0;
        std::vector<int64_t> noise;

        auto binomial_n = ceil(8 * sensitivity * sensitivity * log(2 / delta) / (epsilon * epsilon));
        sigma_4 = 4 * sqrt(static_cast<double>(binomial_n) / 4);

        dual_dp_test->binomial_sampling(batch_n, epsilon, delta, sensitivity, noise);

        ppam::CryptoMatrix cipher_share(noise.size(), 1);
        cipher_share.shares = Eigen::Map<ppam::eMatrix<int64_t>>(noise.data(), noise.size(), 1);

        plain_.resize(noise.size(), 1);
        aby_test->reveal(0, cipher_share, plain_);

        return;
    }

public:
    std::size_t mpc_party0_ = 0;
    std::size_t mpc_party1_ = 1;
    double sigma_4 = 0;
    ppam::eMatrix<double> plain_{};
};

TEST_F(MPCDualDPTest, gen_noise_test) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        status = -1;
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        gen_noise_test(mpc_party1_);
        exit(EXIT_SUCCESS);
    } else {
        gen_noise_test(mpc_party0_);
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
        for (std::size_t i = 0; i < plain_.size(); i++) {
            ASSERT_LT(plain_(i, 0), sigma_4);
        }

        return;
    }
}

}  // namespace mpc_dualdp
}  // namespace privacy_go
