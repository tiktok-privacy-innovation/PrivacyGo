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

namespace privacy_go {
namespace ppam {

void ObliviousTransfer::initialize() {
    auto np_ot_send0 = [&]() {
        std::array<std::array<std::array<std::uint8_t, kEccPointLen>, 2>, kBaseOTSize> send_buffer;
        for (std::uint64_t idx = 0; idx < kBaseOTSize; idx += 1) {
            send_buffer[idx] = np_ot_sender_->send_pre(idx);
        }
        net_->send_data(&send_buffer[0][0][0], kBaseOTSize * 2 * kEccPointLen);
    };

    auto np_ot_send1 = [&]() {
        std::array<std::array<std::uint8_t, kEccPointLen>, kBaseOTSize> recv_buffer;
        net_->recv_data(&recv_buffer[0][0], kBaseOTSize * kEccPointLen);
        for (std::uint64_t idx = 0; idx < kBaseOTSize; idx += 1) {
            np_ot_sender_->send_post(idx, recv_buffer[idx]);
        }
    };

    auto np_ot_receive = [&]() {
        std::array<std::array<std::array<std::uint8_t, kEccPointLen>, 2>, kBaseOTSize> recv_buffer;
        std::array<std::array<std::uint8_t, kEccPointLen>, kBaseOTSize> send_buffer;
        net_->recv_data(&recv_buffer[0][0][0], 2 * kBaseOTSize * kEccPointLen);

        for (std::uint64_t idx = 0; idx < kBaseOTSize; idx += 1) {
            send_buffer[idx] = np_ot_recver_->recv(idx, recv_buffer[idx]);
        }
        net_->send_data(&send_buffer[0][0], kBaseOTSize * kEccPointLen);
    };

    if (party_ == 0) {
        np_ot_receive();
        np_ot_send0();
        np_ot_send1();
    } else {
        np_ot_send0();
        np_ot_send1();
        np_ot_receive();
    }

    std::vector<block> msgs_0(kBaseOTSize), msgs_1(kBaseOTSize);
    for (std::size_t i = 0; i < kBaseOTSize; i++) {
        msgs_0[i] = np_ot_sender_->msgs_[i][0];
        msgs_1[i] = np_ot_sender_->msgs_[i][1];
    }
    ex_ot_recver_->initialize(msgs_0, msgs_1);

    std::vector<block> msgs(kBaseOTSize);
    for (std::size_t i = 0; i < kBaseOTSize; i++) {
        msgs[i] = np_ot_recver_->msgs_[i];
    }
    ex_ot_sender_->initialize(msgs);

    fill_ot_buffer(0);
    now_idx_[0] = 0;
    fill_ot_buffer(1);
    now_idx_[1] = 0;
}

void ObliviousTransfer::fill_ot_buffer(const std::size_t sender_party) {
    auto iknp_send = [&]() {
        std::vector<std::vector<block>> t0(kBaseOTSize, std::vector<block>(kOTSize / kBaseOTSize));
        std::vector<block> buf(kOTSize);

        net_->recv_block(buf.data(), kOTSize);

        for (std::size_t i = 0; i < kBaseOTSize; i++) {
            for (std::size_t j = 0; j < kOTSize / kBaseOTSize; j++) {
                t0[i][j] = buf[i * (kOTSize / kBaseOTSize) + j];
            }
        }

        ex_ot_sender_->send(base_ot_choices_, t0, send_msgs_0_, send_msgs_1_);
    };
    auto iknp_receive = [&]() {
        std::vector<std::vector<block>> t0(kBaseOTSize, std::vector<block>(kOTSize / kBaseOTSize));
        std::vector<block> buf(kOTSize);

        for (std::size_t i = 0; i < kOTSize / kBaseOTSize; i++) {
            ex_choices_[i] = prng_.get<block>();
        }
        ex_ot_recver_->receive(ex_choices_, t0, recv_msgs_);

        for (std::size_t i = 0; i < kBaseOTSize; i++) {
            for (std::size_t j = 0; j < kOTSize / kBaseOTSize; j++) {
                buf[i * (kOTSize / kBaseOTSize) + j] = t0[i][j];
            }
        }

        net_->send_block(buf.data(), kOTSize);
    };

    if (party_ == sender_party) {  // sender
        iknp_send();
    } else {  // receiver
        iknp_receive();
    }
}

std::vector<std::int64_t> ObliviousTransfer::get_ot_instance(const std::size_t sender_party) {
    std::vector<std::int64_t> ret(2);
    if (now_idx_[sender_party] == kOTSize) {
        fill_ot_buffer(sender_party);
        now_idx_[sender_party] = 0;
    }

    if (party_ == sender_party) {
        ret[0] = send_msgs_0_[now_idx_[sender_party]][0];
        ret[1] = send_msgs_1_[now_idx_[sender_party]][0];
    } else {
        ret[0] = recv_msgs_[now_idx_[sender_party]][0];
        std::int64_t index_quo = now_idx_[sender_party] / 128;
        std::int64_t index_rem = now_idx_[sender_party] % 128;
        ret[1] = (ex_choices_[index_quo][index_rem / 64] >> (index_rem % 64)) & 0x1;
    }
    now_idx_[sender_party]++;

    return ret;
}

}  // namespace ppam
}  // namespace privacy_go
