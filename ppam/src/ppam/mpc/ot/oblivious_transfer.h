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

#include <array>
#include <memory>
#include <vector>

#include "ppam/mpc/common/defines.h"
#include "ppam/mpc/ot/iknp_ot_ext_receiver.h"
#include "ppam/mpc/ot/iknp_ot_ext_sender.h"
#include "ppam/mpc/ot/naor_pinkas_ot_receiver.h"
#include "ppam/mpc/ot/naor_pinkas_ot_sender.h"

namespace privacy_go {
namespace ppam {

class ObliviousTransfer {
public:
    ObliviousTransfer() = delete;

    ObliviousTransfer(
            std::size_t party, const std::shared_ptr<IOBase>& net, const block& common_seed, const block& unique_seed)
            : party_(party), net_(net), now_idx_(0) {
        prng_.set_seed(unique_seed);
        base_ot_choices_ = prng_.get<block>();
        np_ot_sender_ = std::make_shared<NaorPinkasOtSender>(kBaseOTSize);
        np_ot_recver_ = std::make_shared<NaorPinkasOtReceiver>(kBaseOTSize, base_ot_choices_);
        ex_ot_sender_ = std::make_shared<IknpOtExtSender>(kOTSize, common_seed);
        ex_ot_recver_ = std::make_shared<IknpOtExtReceiver>(kOTSize, common_seed);
        send_msgs_0_.resize(kOTSize);
        send_msgs_1_.resize(kOTSize);
        recv_msgs_.resize(kOTSize);
        now_idx_.resize(2);
        ex_choices_.resize((kOTSize) / (sizeof(block) * 8));
    }

    ~ObliviousTransfer() = default;

    void initialize();

    void fill_ot_buffer(const std::size_t sender_party = 0);

    std::vector<std::int64_t> get_ot_instance(const std::size_t sender_party = 0);

private:
    PRNG prng_;
    std::size_t party_ = 0;
    block base_ot_choices_;
    std::shared_ptr<IOBase> net_ = nullptr;
    std::vector<std::size_t> now_idx_{};
    std::vector<block> send_msgs_0_{};
    std::vector<block> send_msgs_1_{};
    std::vector<block> recv_msgs_{};
    std::vector<block> ex_choices_{};
    std::shared_ptr<NaorPinkasOtSender> np_ot_sender_ = nullptr;
    std::shared_ptr<NaorPinkasOtReceiver> np_ot_recver_ = nullptr;
    std::shared_ptr<IknpOtExtSender> ex_ot_sender_ = nullptr;
    std::shared_ptr<IknpOtExtReceiver> ex_ot_recver_ = nullptr;
};

}  // namespace ppam
}  // namespace privacy_go
