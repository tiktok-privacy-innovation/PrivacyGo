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

#include "ppam/mpc/ot/oblivious_transfer.h"

namespace privacy_go {
namespace ppam {

class BoolTripletGenerator {
public:
    BoolTripletGenerator() = default;

    ~BoolTripletGenerator() = default;

    void initialize(std::size_t party, const std::shared_ptr<ObliviousTransfer>& ot) {
        party_id_ = party;
        ot_ = ot;
        rand_triplet_buff.resize(8192);
        refill_rand_triplet_buffer(party);
    }

    std::vector<std::int64_t> get_rand_triplet(std::size_t party) {
        if (rand_triplet_idx >= rand_triplet_buff.size()) {
            refill_rand_triplet_buffer(party);
        }
        std::vector<std::int64_t> ret(3);
        ret[0] = rand_triplet_buff[rand_triplet_idx][0];
        ret[1] = rand_triplet_buff[rand_triplet_idx][1];
        ret[2] = rand_triplet_buff[rand_triplet_idx][2];
        rand_triplet_idx++;
        return ret;
    }

private:
    void refill_rand_triplet_buffer(std::size_t party) {
        gen_rand_triplet(party);
        rand_triplet_idx = 0;
        return;
    }

    void gen_rand_triplet(std::size_t party) {
        std::int64_t len = rand_triplet_buff.size();
        std::vector<std::int64_t> msgs0(64 * len);
        std::vector<std::int64_t> msgs1(64 * len);
        std::vector<std::int64_t> x0(len);
        std::vector<std::int64_t> x1(len);
        std::vector<std::int64_t> xa(len);
        std::vector<std::int64_t> a0(len);
        std::vector<std::int64_t> b1(len);
        std::vector<std::int64_t> u0(len);
        std::vector<std::int64_t> v0(len);

        for (int i = 0; i < 64 * len; i++) {
            auto msg = ot_->get_ot_instance();
            msgs0[i] = msg[0];
            msgs1[i] = msg[1];
        }

        if (party == 0) {
            lsb_to_int64(msgs0, x0);
            lsb_to_int64(msgs1, x1);
        } else {
            lsb_to_int64(msgs0, xa);
            lsb_to_int64(msgs1, a0);
        }

        if (party == 0) {
            for (int i = 0; i < len; i++) {
                b1[i] = x0[i] ^ x1[i];
                v0[i] = x0[i];
            }
        } else {
            for (int i = 0; i < len; i++) {
                u0[i] = xa[i];
            }
        }

        for (int i = 0; i < 64 * len; i++) {
            auto msg = ot_->get_ot_instance();
            msgs0[i] = msg[0];
            msgs1[i] = msg[1];
        }

        std::vector<std::int64_t> a1(len), b0(len), u1(len), v1(len);

        if (party == 0) {
            lsb_to_int64(msgs0, x0);
            lsb_to_int64(msgs1, x1);
        } else {
            lsb_to_int64(msgs0, xa);
            lsb_to_int64(msgs1, b0);
        }

        if (party == 0) {
            for (int i = 0; i < len; i++) {
                a1[i] = x0[i] ^ x1[i];
                v1[i] = x0[i];
            }
        } else {
            for (int i = 0; i < len; i++) {
                u1[i] = xa[i];
            }
        }

        for (int i = 0; i < len; i++) {
            rand_triplet_buff[i].resize(3);
            if (party == 0) {
                rand_triplet_buff[i][0] = b1[i];
                rand_triplet_buff[i][1] = a1[i];
                rand_triplet_buff[i][2] = (a1[i] & b1[i]) ^ v0[i] ^ v1[i];

            } else {
                rand_triplet_buff[i][0] = b0[i];
                rand_triplet_buff[i][1] = a0[i];
                rand_triplet_buff[i][2] = (a0[i] & b0[i]) ^ u0[i] ^ u1[i];
            }
        }
    }

    inline int lsb_to_int64(const std::vector<std::int64_t>& in, std::vector<std::int64_t>& out) {
        if ((in.size() % 64) != 0) {
            return -1;
        }

        int k = 0;
        for (std::size_t i = 0; i < in.size(); i += 64, k++) {
            out[k] = 0;
            for (std::size_t j = 0; j < 64; j++) {
                out[k] = (out[k] << 1) | (in[i + j] & 0x1);
            }
        }
        return 0;
    }

    std::size_t rand_triplet_idx = 0;
    std::vector<std::vector<std::int64_t>> rand_triplet_buff{};
    std::shared_ptr<ObliviousTransfer> ot_ = nullptr;
    std::size_t party_id_ = 0;
};

}  // namespace ppam
}  // namespace privacy_go
