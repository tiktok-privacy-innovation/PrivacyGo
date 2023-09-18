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

#include <cmath>

namespace privacy_go {
namespace mpc_dualdp {

void MPCDualDP::initialize(const std::size_t party_id, const std::shared_ptr<IOBase>& net) {
    party_id_ = party_id;
    net_ = net;

    ppam::block data_from_recv;
    auto data_to_send = ppam::read_block_from_dev_urandom();

    if (party_id_ == 0) {
        net_->send_block(&data_to_send, 1);
        net_->recv_block(&data_from_recv, 1);
    } else {
        net_->recv_block(&data_from_recv, 1);
        net_->send_block(&data_to_send, 1);
    }

    rand_generator_ = std::make_shared<ppam::PseudoRandGenerator>(data_to_send ^ data_from_recv);

    auto common_seed = _mm_set_epi64x(rand_generator_->get_common_rand(), rand_generator_->get_common_rand());
    auto unique_seed = _mm_set_epi64x(rand_generator_->get_unique_rand(), rand_generator_->get_unique_rand());
    oblivious_transfer_ = std::make_shared<ppam::ObliviousTransfer>(party_id_, net_, common_seed, unique_seed);
    oblivious_transfer_->initialize();

    return;
}

void MPCDualDP::binomial_sampling(const std::size_t n, const double epsilon, const double delta,
        const double sensitivity, std::vector<int64_t>& noise) {
    std::size_t binomial_n = std::ceil(8 * sensitivity * sensitivity * std::log(2 / delta) / (epsilon * epsilon));
    std::size_t rows = n;
    std::size_t cols = binomial_n;

    ppam::CryptoMatrix x(rows, cols);
    ppam::CryptoMatrix r(rows, cols);
    ppam::CryptoMatrix s0(rows, cols);
    ppam::CryptoMatrix s1(rows, cols);

    for (std::size_t i = 0; i < x.size(); i++) {
        x.shares(i) = rand_generator_->get_unique_rand() & 0x1;
    }

    if (party_id_ == 0) {
        for (std::size_t i = 0; i < r.size(); i++) {
            r.shares(i) = rand_generator_->get_unique_rand();
        }
        for (int i = 0; i < x.size(); i++) {
            s0.shares(i) = x.shares(i) - r.shares(i);
            s1.shares(i) = 1 - x.shares(i) - r.shares(i);
        }
    }

    std::size_t size = rows * cols;
    bool* k = new bool[size];
    ppam::CryptoMatrix y0(rows, cols);
    ppam::CryptoMatrix y1(rows, cols);
    ppam::CryptoMatrix rb(rows, cols);
    if (party_id_ == 1) {
        for (std::size_t i = 0; i < size; i++) {
            auto msg = oblivious_transfer_->get_ot_instance(1 - party_id_);
            k[i] = msg[1] ^ x.shares(i);
            rb.shares(i) = msg[0];
        }
        net_->send_bool(k, size);
        ppam::recv_matrix(net_, &y0, 1);
        ppam::recv_matrix(net_, &y1, 1);
        for (std::size_t i = 0; i < size; i++) {
            if (x.shares(i) == 0) {
                r.shares(i) = y0.shares(i) ^ rb.shares(i);
            } else {
                r.shares(i) = y1.shares(i) ^ rb.shares(i);
            }
        }
    } else {
        for (std::size_t i = 0; i < size; i++) {
            auto msg = oblivious_transfer_->get_ot_instance(party_id_);
            y0.shares(i) = msg[0];
            y1.shares(i) = msg[1];
        }
        net_->recv_bool(k, size);
        for (std::size_t i = 0; i < size; i++) {
            if (k[i] == 0) {
                y0.shares(i) ^= s0.shares(i);
                y1.shares(i) ^= s1.shares(i);

            } else {
                auto t = s0.shares(i) ^ y1.shares(i);
                y1.shares(i) = s1.shares(i) ^ y0.shares(i);
                y0.shares(i) = t;
            }
        }
        ppam::send_matrix(net_, &y0, 1);
        ppam::send_matrix(net_, &y1, 1);
    }
    delete[] k;

    auto binomial_sum = r.shares.rowwise().sum();
    double mean = static_cast<double>(binomial_n) / 2.0;

    if (party_id_ == 0) {
        for (std::size_t i = 0; i < n; i++) {
            noise.push_back(binomial_sum(i) << 16);
        }
    } else {
        std::int64_t fix_mean = static_cast<int64_t>(mean * static_cast<double>(1UL << 16));
        for (std::size_t i = 0; i < n; i++) {
            noise.push_back((binomial_sum(i) << 16) - fix_mean);
        }
    }

    return;
}

}  // namespace mpc_dualdp
}  // namespace privacy_go
