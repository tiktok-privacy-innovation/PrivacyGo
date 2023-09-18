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

#include <memory>
#include <string>

#include "mpc-dualdp/mpc_dualdp.h"

#include "dpca-psi/dp_cardinality_psi.h"
#include "dpca-psi/network/two_channel_net_io.h"

void mpc_dualdp_example(std::size_t party, std::string local_addr, std::size_t local_port, std::string remote_addr,
        std::size_t remote_port) {
    auto net = std::make_shared<privacy_go::dpca_psi::TwoChannelNetIO>(remote_addr, remote_port, local_port);

    auto aby_test = privacy_go::ppam::AbyProtocol::Instance();
    aby_test->initialize(party, net);

    auto dual_dp_test = std::make_shared<privacy_go::mpc_dualdp::MPCDualDP>();
    dual_dp_test->initialize(party, net);

    std::size_t batch_n = 16;
    double epsilon = 1.0;
    double delta = 0.00001;
    double sensitivity = 1.0;
    std::vector<int64_t> noise;

    dual_dp_test->binomial_sampling(batch_n, epsilon, delta, sensitivity, noise);

    privacy_go::ppam::CryptoMatrix cipher_share(noise.size(), 1);
    cipher_share.shares = Eigen::Map<privacy_go::ppam::eMatrix<int64_t>>(noise.data(), noise.size(), 1);

    privacy_go::ppam::eMatrix<double> plain(noise.size(), 1);

    aby_test->reveal(0, cipher_share, plain);
    aby_test->reveal(1, cipher_share, plain);

    for (std::size_t i = 0; i < plain.size(); i++) {
        std::cout << plain(i, 0) << " ";
    }
    std::cout << std::endl;

    return;
}

// ./build/bin/mpc_dualdp_example  127.0.0.1 8899 127.0.0.1 8890 0
// ./build/bin/mpc_dualdp_example  127.0.0.1 8890 127.0.0.1 8899 1
int main(int argc, char* argv[]) {
    mpc_dualdp_example(atoi(argv[5]), std::string(argv[1]), atoi(argv[2]), std::string(argv[3]), atoi(argv[4]));
    return 0;
}
