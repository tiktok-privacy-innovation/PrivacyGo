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

#include "ppam/mpc/ot/oblivious_transfer.h"

#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "ppam/mpc/common/defines.h"
#include "ppam/mpc/common/utils.h"

namespace privacy_go {
namespace ppam {

class ObliviousTransferTest : public ::testing::Test {
public:
    void random_ot(bool idx) {
        std::uint16_t remote_port = idx ? 7790 : 7791;
        std::uint16_t local_port = idx ? 7791 : 7790;
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", remote_port, local_port);
        ObliviousTransfer ot(idx, net, _mm_set_epi64x(0, 0xcccccccc), read_block_from_dev_urandom());
        ot.initialize();
        ot.fill_ot_buffer();
        if (idx == 0) {
            send_msg_ = ot.get_ot_instance();
        } else {
            recv_msg_ = ot.get_ot_instance();
        }
    }

public:
    std::thread t_[2];
    std::vector<std::int64_t> send_msg_;
    std::vector<std::int64_t> recv_msg_;
};

TEST_F(ObliviousTransferTest, random_ot) {
    t_[0] = std::thread([this]() { random_ot(0); });
    t_[1] = std::thread([this]() { random_ot(1); });

    t_[0].join();
    t_[1].join();

    EXPECT_EQ(send_msg_.size(), recv_msg_.size());
    EXPECT_EQ(send_msg_[recv_msg_[1]], recv_msg_[0]);
}

}  // namespace ppam
}  // namespace privacy_go
