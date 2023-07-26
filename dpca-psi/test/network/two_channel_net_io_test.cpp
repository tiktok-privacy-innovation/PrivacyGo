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

#include "dpca-psi/network/two_channel_net_io.h"

#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace privacy_go {
namespace dpca_psi {

class TwoChannelNetIOTest : public ::testing::Test {
public:
    void SetUp() {
    }
    std::thread t_[2];
    std::vector<std::uint64_t> send_bytes_count_ = {0, 0};
    std::vector<std::uint64_t> recv_bytes_count_ = {0, 0};
    std::vector<std::uint64_t> expected_send_bytes_count_ = {0, 0};
    std::vector<std::uint64_t> expected_recv_bytes_count_ = {0, 0};
};

TEST_F(TwoChannelNetIOTest, send_data) {
    std::vector<std::size_t> send_data = {100, 200};
    std::vector<std::size_t> recv_data = {0, 0};

    t_[0] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30330, 30331);
        net->send_data(&send_data[0], sizeof(std::size_t));
        net->recv_data(&recv_data[1], sizeof(std::size_t));
        send_bytes_count_[0] = net->get_bytes_sent();
        recv_bytes_count_[0] = net->get_bytes_received();
    });
    t_[1] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30331, 30330);
        net->send_data(&send_data[1], sizeof(std::size_t));
        net->recv_data(&recv_data[0], sizeof(std::size_t));
        send_bytes_count_[1] = net->get_bytes_sent();
        recv_bytes_count_[1] = net->get_bytes_received();
    });

    t_[0].join();
    t_[1].join();

    expected_send_bytes_count_ = {sizeof(std::size_t), sizeof(std::size_t)};
    expected_recv_bytes_count_ = {sizeof(std::size_t), sizeof(std::size_t)};

    ASSERT_EQ(send_data, recv_data);
    ASSERT_EQ(send_bytes_count_, expected_send_bytes_count_);
    ASSERT_EQ(recv_bytes_count_, expected_recv_bytes_count_);
}

TEST_F(TwoChannelNetIOTest, send_block) {
    std::vector<block> send_data = {_mm_set_epi64x(0, 0), _mm_set_epi64x(0, 1)};
    std::vector<block> recv_data = {_mm_set_epi64x(0, 0), _mm_set_epi64x(0, 0)};

    t_[0] = std::thread([this, &send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30330, 30331);
        net->send_block(&send_data[0], 1);
        net->recv_block(&recv_data[1], 1);
        send_bytes_count_[0] = net->get_bytes_sent();
        recv_bytes_count_[0] = net->get_bytes_received();
    });
    t_[1] = std::thread([this, &send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30331, 30330);
        net->send_block(&send_data[1], 1);
        net->recv_block(&recv_data[0], 1);
        send_bytes_count_[1] = net->get_bytes_sent();
        recv_bytes_count_[1] = net->get_bytes_received();
    });

    t_[0].join();
    t_[1].join();

    expected_send_bytes_count_ = {sizeof(block), sizeof(block)};
    expected_recv_bytes_count_ = {sizeof(block), sizeof(block)};

    ASSERT_EQ(send_data[0][0], recv_data[0][0]);
    ASSERT_EQ(send_data[0][1], recv_data[0][1]);
    ASSERT_EQ(send_data[1][0], recv_data[1][0]);
    ASSERT_EQ(send_data[1][1], recv_data[1][1]);
    ASSERT_EQ(send_bytes_count_, expected_send_bytes_count_);
    ASSERT_EQ(recv_bytes_count_, expected_recv_bytes_count_);
}

TEST_F(TwoChannelNetIOTest, send_value) {
    std::vector<std::size_t> send_data = {100, 200};
    std::vector<std::size_t> recv_data = {0, 0};

    t_[0] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30330, 30331);
        net->send_value<std::size_t>(send_data[0]);
        recv_data[1] = net->recv_value<std::size_t>();
        send_bytes_count_[0] = net->get_bytes_sent();
        recv_bytes_count_[0] = net->get_bytes_received();
    });
    t_[1] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30331, 30330);
        net->send_value<std::size_t>(send_data[1]);
        recv_data[0] = net->recv_value<std::size_t>();
        send_bytes_count_[1] = net->get_bytes_sent();
        recv_bytes_count_[1] = net->get_bytes_received();
    });

    t_[0].join();
    t_[1].join();

    expected_send_bytes_count_ = {sizeof(std::size_t), sizeof(std::size_t)};
    expected_recv_bytes_count_ = {sizeof(std::size_t), sizeof(std::size_t)};

    ASSERT_EQ(send_data, recv_data);
    ASSERT_EQ(send_bytes_count_, expected_send_bytes_count_);
    ASSERT_EQ(recv_bytes_count_, expected_recv_bytes_count_);
}

TEST_F(TwoChannelNetIOTest, send_string) {
    std::vector<std::string> send_data = {"100", "200"};
    std::vector<std::string> recv_data = {"", ""};

    t_[0] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30330, 30331);
        net->send_string(send_data[0]);
        recv_data[1] = net->recv_string();
        send_bytes_count_[0] = net->get_bytes_sent();
        recv_bytes_count_[0] = net->get_bytes_received();
    });
    t_[1] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30331, 30330);
        net->send_string(send_data[1]);
        recv_data[0] = net->recv_string();
        send_bytes_count_[1] = net->get_bytes_sent();
        recv_bytes_count_[1] = net->get_bytes_received();
    });

    t_[0].join();
    t_[1].join();

    expected_send_bytes_count_ = {3 + sizeof(std::size_t), 3 + sizeof(std::size_t)};
    expected_recv_bytes_count_ = {3 + sizeof(std::size_t), 3 + sizeof(std::size_t)};

    ASSERT_EQ(send_data, recv_data);
    ASSERT_EQ(send_bytes_count_, expected_send_bytes_count_);
    ASSERT_EQ(recv_bytes_count_, expected_recv_bytes_count_);
}

TEST_F(TwoChannelNetIOTest, send_bytes) {
    std::vector<ByteVector> send_data = {{Byte(0), Byte(1)}, {Byte(0), Byte(1)}};
    std::vector<ByteVector> recv_data(2, ByteVector(2));

    t_[0] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30330, 30331);
        net->send_bytes(send_data[0]);
        net->recv_bytes(recv_data[1]);
        send_bytes_count_[0] = net->get_bytes_sent();
        recv_bytes_count_[0] = net->get_bytes_received();
    });
    t_[1] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30331, 30330);
        net->send_bytes(send_data[1]);
        net->recv_bytes(recv_data[0]);
        send_bytes_count_[1] = net->get_bytes_sent();
        recv_bytes_count_[1] = net->get_bytes_received();
    });

    t_[0].join();
    t_[1].join();

    expected_send_bytes_count_ = {2 + sizeof(std::size_t), 2 + sizeof(std::size_t)};
    expected_recv_bytes_count_ = {2 + sizeof(std::size_t), 2 + sizeof(std::size_t)};

    ASSERT_EQ(send_data, recv_data);
    ASSERT_EQ(send_bytes_count_, expected_send_bytes_count_);
    ASSERT_EQ(recv_bytes_count_, expected_recv_bytes_count_);
}

TEST_F(TwoChannelNetIOTest, send_bool) {
    bool send_data[2][2] = {{true, false}, {false, true}};
    bool recv_data[2][2] = {{false, false}, {false, false}};

    t_[0] = std::thread([this, &send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30330, 30331);
        net->send_bool(send_data[0], 2);
        net->recv_bool(recv_data[1], 2);
        send_bytes_count_[0] = net->get_bytes_sent();
        recv_bytes_count_[0] = net->get_bytes_received();
    });
    t_[1] = std::thread([this, &send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", 30331, 30330);
        net->send_bool(send_data[1], 2);
        net->recv_bool(recv_data[0], 2);
        send_bytes_count_[1] = net->get_bytes_sent();
        recv_bytes_count_[1] = net->get_bytes_received();
    });

    t_[0].join();
    t_[1].join();
    expected_send_bytes_count_ = {2, 2};
    expected_recv_bytes_count_ = {2, 2};

    ASSERT_EQ(send_data[0][0], recv_data[0][0]);
    ASSERT_EQ(send_bytes_count_, expected_send_bytes_count_);
    ASSERT_EQ(recv_bytes_count_, expected_recv_bytes_count_);
}

TEST_F(TwoChannelNetIOTest, ipv6) {
    std::vector<std::size_t> send_data = {100, 200};
    std::vector<std::size_t> recv_data = {0, 0};

    t_[0] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("::1", 30330, 30331);
        net->send_data(&send_data[0], sizeof(std::size_t));
        net->recv_data(&recv_data[1], sizeof(std::size_t));
        send_bytes_count_[0] = net->get_bytes_sent();
        recv_bytes_count_[0] = net->get_bytes_received();
    });
    t_[1] = std::thread([this, send_data, &recv_data]() {
        auto net = std::make_shared<TwoChannelNetIO>("::1", 30331, 30330);
        net->send_data(&send_data[1], sizeof(std::size_t));
        net->recv_data(&recv_data[0], sizeof(std::size_t));
        send_bytes_count_[1] = net->get_bytes_sent();
        recv_bytes_count_[1] = net->get_bytes_received();
    });

    t_[0].join();
    t_[1].join();

    expected_send_bytes_count_ = {sizeof(std::size_t), sizeof(std::size_t)};
    expected_recv_bytes_count_ = {sizeof(std::size_t), sizeof(std::size_t)};

    ASSERT_EQ(send_data, recv_data);
    ASSERT_EQ(send_bytes_count_, expected_send_bytes_count_);
    ASSERT_EQ(recv_bytes_count_, expected_recv_bytes_count_);
}

}  // namespace dpca_psi
}  // namespace privacy_go
