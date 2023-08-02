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

#include "ppam/mpc/aby/aby_protocol.h"

#include "ppam/mpc/common/utils.h"

namespace privacy_go {
namespace ppam {

int AbyProtocol::set_seed() {
    block data_from_recv;
    auto data_to_send = read_block_from_dev_urandom();

    if (party_id_ == 0) {
        net_io_->send_block(&data_to_send, 1);
        net_io_->recv_block(&data_from_recv, 1);
    } else {
        net_io_->recv_block(&data_from_recv, 1);
        net_io_->send_block(&data_to_send, 1);
    }

    rand_generator_ = std::make_shared<PseudoRandGenerator>(data_to_send ^ data_from_recv);
    return 0;
}

int AbyProtocol::initialize(const std::size_t party_id, const std::shared_ptr<IOBase>& net_io) {
    // set party id
    party_id_ = party_id;

    // set network
    net_io_ = net_io;

    // set random seed
    set_seed();

    // set OT
    auto common_seed = _mm_set_epi64x(rand_generator_->get_common_rand(), rand_generator_->get_common_rand());
    auto unique_seed = _mm_set_epi64x(rand_generator_->get_unique_rand(), rand_generator_->get_unique_rand());
    oblivious_transfer_ = std::make_shared<ObliviousTransfer>(party_id_, net_io_, common_seed, unique_seed);
    oblivious_transfer_->initialize();

    // set triplet
    rand_bool_triplet_generator_ = std::make_shared<BoolTripletGenerator>();
    rand_bool_triplet_generator_->initialize(party_id_, oblivious_transfer_);

    return 0;
}

int AbyProtocol::release() {
    return 0;
}

int AbyProtocol::share(const std::size_t party, const eMatrix<double>& in, CryptoMatrix& out) {
    std::size_t row = in.rows();
    std::size_t col = in.cols();
    out.resize(row, col);

    if (party == party_id_) {
        for (std::size_t i = 0; i < out.size(); i++) {
            out.shares(i) = float_to_fixed(in(i)) - rand_generator_->get_common_rand();
        }
    } else {
        for (std::size_t i = 0; i < out.size(); i++) {
            out.shares(i) = rand_generator_->get_common_rand();
        }
    }
    return 0;
}

int AbyProtocol::reveal(const std::size_t party, const CryptoMatrix& in, eMatrix<double>& out) {
    CryptoMatrix fixed_matrix(in.rows(), in.cols());
    if (party_id_ != party) {
        send_matrix(net_io_, &in, 1);
    } else {
        recv_matrix(net_io_, &fixed_matrix, 1);
    }

    out.resize(in.rows(), in.cols());

    if (party_id_ == party) {
        fixed_matrix = fixed_matrix + in;

        for (std::size_t i = 0; i < in.size(); i++) {
            out(i) = fixed_to_float(fixed_matrix.shares(i));
        }
    }
    return 0;
}

int AbyProtocol::add(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z) {
    z = x + y;
    return 0;
}

int AbyProtocol::sub(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z) {
    z = x - y;
    return 0;
}

int AbyProtocol::elementwise_bool_mul(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z) {
    if (x.size() != y.size()) {
        return -1;
    }
    std::size_t row = x.rows();
    std::size_t col = x.cols();
    z.resize(row, col);

    CryptoMatrix triplet_a(row, col);
    CryptoMatrix triplet_b(row, col);
    CryptoMatrix triplet_c(row, col);
    for (std::size_t i = 0; i < x.size(); ++i) {
        auto triplet = rand_bool_triplet_generator_->get_rand_triplet(party_id_);
        triplet_a.shares(i) = triplet[0];
        triplet_b.shares(i) = triplet[1];
        triplet_c.shares(i) = triplet[2];
    }

    CryptoMatrix e(row, col);
    CryptoMatrix f(row, col);
    for (std::size_t i = 0; i < e.size(); ++i) {
        e.shares(i) = x.shares(i) ^ triplet_a.shares(i);
        f.shares(i) = y.shares(i) ^ triplet_b.shares(i);
    }

    CryptoMatrix reveal_e(row, col);
    CryptoMatrix reveal_f(row, col);
    if (party_id_ == 0) {
        send_matrix(net_io_, &e, 1);
        send_matrix(net_io_, &f, 1);
        recv_matrix(net_io_, &reveal_e, 1);
        recv_matrix(net_io_, &reveal_f, 1);
    } else {
        recv_matrix(net_io_, &reveal_e, 1);
        recv_matrix(net_io_, &reveal_f, 1);
        send_matrix(net_io_, &e, 1);
        send_matrix(net_io_, &f, 1);
    }

    for (std::size_t i = 0; i < reveal_e.size(); ++i) {
        reveal_e.shares(i) = reveal_e.shares(i) ^ e.shares(i);
        reveal_f.shares(i) = reveal_f.shares(i) ^ f.shares(i);
    }

    if (party_id_ == 0) {
        for (std::size_t i = 0; i < z.size(); ++i) {
            z.shares(i) = (reveal_f.shares(i) & triplet_a.shares(i)) ^ (reveal_e.shares(i) & triplet_b.shares(i)) ^
                          triplet_c.shares(i);
        }
    } else {
        for (std::size_t i = 0; i < z.size(); ++i) {
            z.shares(i) = (reveal_e.shares(i) & reveal_f.shares(i)) ^ (reveal_f.shares(i) & triplet_a.shares(i)) ^
                          (reveal_e.shares(i) & triplet_b.shares(i)) ^ triplet_c.shares(i);
        }
    }
    return 0;
}

int AbyProtocol::kogge_stone_ppa(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z) {
    if (x.size() != y.size()) {
        return -1;
    }
    std::size_t row = x.rows();
    std::size_t col = x.cols();
    std::size_t depth = 6;
    CryptoMatrix g1(row, col);
    CryptoMatrix p1(row, col);
    CryptoMatrix g(row, col);
    CryptoMatrix p(row, col);
    std::int64_t keep_masks[6] = {0x0000000000000001, 0x0000000000000003, 0x000000000000000f, 0x00000000000000ff,
            0x000000000000ffff, 0x00000000ffffffff};

    elementwise_bool_mul(x, y, g);
    for (std::size_t i = 0; i < x.size(); i++) {
        p.shares(i) = x.shares(i) ^ y.shares(i);
    }

    for (std::size_t i = 0; i < depth; i++) {
        std::size_t shift = 1L << i;
        for (std::size_t k = 0; k < p.size(); k++) {
            p1.shares(k) = p.shares(k) << shift;
        }
        for (std::size_t k = 0; k < g.size(); k++) {
            g1.shares(k) = g.shares(k) << shift;
        }

        if (party_id_ == 0) {
            for (std::size_t k = 0; k < p.size(); k++) {
                p1.shares(k) ^= keep_masks[i];
            }
        }
        elementwise_bool_mul(p, g1, g1);
        for (std::size_t k = 0; k < g.size(); k++) {
            g.shares(k) ^= g1.shares(k);
        }
        elementwise_bool_mul(p, p1, p);
    }

    for (std::size_t k = 0; k < g.size(); k++) {
        g1.shares(k) = g.shares(k) << 1;
    }

    for (std::size_t k = 0; k < g.size(); k++) {
        z.shares(k) = g1.shares(k) ^ x.shares(k) ^ y.shares(k);
    }

    return 0;
}

int AbyProtocol::a2b(const CryptoMatrix& x, CryptoMatrix& z) {
    std::size_t size = x.size();
    std::size_t row = x.rows();
    std::size_t col = x.cols();

    CryptoMatrix input_0(row, col);
    CryptoMatrix input_1(row, col);
    if (party_id_ == 0) {
        for (std::size_t j = 0; j < size; j++) {
            input_0.shares(j) = x.shares(j) ^ rand_generator_->get_common_rand();
            input_1.shares(j) = rand_generator_->get_common_rand();
        }
    } else {
        for (std::size_t j = 0; j < size; j++) {
            input_0.shares(j) = rand_generator_->get_common_rand();
            input_1.shares(j) = x.shares(j) ^ rand_generator_->get_common_rand();
        }
    }
    kogge_stone_ppa(input_0, input_1, z);
    for (std::size_t j = 0; j < size; j++) {
        z.shares(j) = (z.shares(j) >> 63) & 0x1;
    }

    return 0;
}

int AbyProtocol::greater(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z) {
    a2b(y - x, z);
    return 0;
}

int AbyProtocol::greater(const CryptoMatrix& x, const eMatrix<double>& y, CryptoMatrix& z) {
    std::size_t size = x.size();
    CryptoMatrix c(x.rows(), x.cols());
    if (party_id_ == 0) {
        for (std::size_t j = 0; j < size; j++) {
            c.shares(j) = float_to_fixed(y(j)) - x.shares(j);
        }
    } else {
        c.shares = -x.shares;
    }
    a2b(c, z);
    return 0;
}

int AbyProtocol::less(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z) {
    a2b(x - y, z);
    return 0;
}

int AbyProtocol::less(const CryptoMatrix& x, const eMatrix<double>& y, CryptoMatrix& z) {
    std::size_t size = x.size();
    CryptoMatrix c(x.rows(), x.cols());
    if (party_id_ == 0) {
        for (std::size_t j = 0; j < size; j++) {
            c.shares(j) = x.shares(j) - float_to_fixed(y(j));
        }
    } else {
        c.shares = x.shares;
    }
    a2b(c, z);
    return 0;
}

int AbyProtocol::sum(const CryptoMatrix& in, CryptoMatrix& out) {
    out.resize(1, in.cols());
    out.shares.row(0) = in.shares.colwise().sum();
    return 0;
}

int AbyProtocol::multiplexer(const CryptoMatrix& x, const CryptoMatrix& y, CryptoMatrix& z) {
    if (x.size() != y.size()) {
        return -1;
    }
    std::size_t size = x.size();
    std::size_t row = x.rows();
    std::size_t col = x.cols();

    CryptoMatrix r(row, col);
    CryptoMatrix s0(row, col);
    CryptoMatrix s1(row, col);
    for (std::size_t i = 0; i < size; i++) {
        r.shares(i) = rand_generator_->get_unique_rand();
    }

    for (std::size_t i = 0; i < size; i++) {
        if (x.shares(i) == 0) {
            s0.shares(i) = -r.shares(i);
            s1.shares(i) = y.shares(i) - r.shares(i);
        } else {
            s0.shares(i) = y.shares(i) - r.shares(i);
            s1.shares(i) = -r.shares(i);
        }
    }

    CryptoMatrix y0(row, col);
    CryptoMatrix y1(row, col);
    CryptoMatrix rb(row, col);
    bool* k = new bool[size];
    if (party_id_ == 0) {
        for (std::size_t i = 0; i < size; i++) {
            auto msg = oblivious_transfer_->get_ot_instance(1);
            k[i] = msg[1] ^ x.shares(i);
            rb.shares(i) = msg[0];
        }

        net_io_->send_bool(k, size);
        recv_matrix(net_io_, &y0, 1);
        recv_matrix(net_io_, &y1, 1);
        for (std::size_t i = 0; i < size; i++) {
            if (x.shares(i) == 0) {
                z.shares(i) = y0.shares(i) ^ rb.shares(i);
            } else {
                z.shares(i) = y1.shares(i) ^ rb.shares(i);
            }
        }
    } else {
        for (std::size_t i = 0; i < size; i++) {
            auto msg = oblivious_transfer_->get_ot_instance(1);
            y0.shares(i) = msg[0];
            y1.shares(i) = msg[1];
        }
        net_io_->recv_bool(k, size);

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
        send_matrix(net_io_, &y0, 1);
        send_matrix(net_io_, &y1, 1);
    }

    if (party_id_ == 1) {
        for (std::size_t i = 0; i < size; i++) {
            auto msg = oblivious_transfer_->get_ot_instance(0);
            k[i] = msg[1] ^ x.shares(i);
            rb.shares(i) = msg[0];
        }
        net_io_->send_bool(k, size);
        recv_matrix(net_io_, &y0, 1);
        recv_matrix(net_io_, &y1, 1);
        for (std::size_t i = 0; i < size; i++) {
            if (x.shares(i) == 0) {
                z.shares(i) = y0.shares(i) ^ rb.shares(i);
            } else {
                z.shares(i) = y1.shares(i) ^ rb.shares(i);
            }
        }
    } else {
        for (std::size_t i = 0; i < size; i++) {
            auto msg = oblivious_transfer_->get_ot_instance(0);
            y0.shares(i) = msg[0];
            y1.shares(i) = msg[1];
        }
        net_io_->recv_bool(k, size);
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
        send_matrix(net_io_, &y0, 1);
        send_matrix(net_io_, &y1, 1);
    }

    for (std::size_t i = 0; i < size; i++) {
        z.shares(i) += r.shares(i);
    }
    delete[] k;

    return 0;
}

int AbyProtocol::attribution(const double& tf, const CryptoMatrix& in, CryptoMatrix& out) {
    auto t0 = in.shares.col(0);
    auto t1 = in.shares.col(1);
    auto value = in.shares.col(2);

    auto t = t1 - t0;

    CryptoMatrix ct(in.rows(), 1);
    ct.shares = t;

    CryptoMatrix gt_zero(in.rows(), 1);
    eMatrix<double> zero_matrix(in.rows(), 1);
    zero_matrix.setZero();
    greater(ct, zero_matrix, gt_zero);

    CryptoMatrix ls_tf(in.rows(), 1);
    eMatrix<double> tf_matrix(in.rows(), 1);
    tf_matrix.setConstant(tf);
    less(ct, tf_matrix, ls_tf);

    CryptoMatrix select_bits(in.rows(), 1);
    elementwise_bool_mul(gt_zero, ls_tf, select_bits);
    for (std::size_t i = 0; i < in.rows(); i++) {
        select_bits.shares(i) &= 0x1;
    }

    CryptoMatrix c_value(in.rows(), 1), select_result(in.rows(), 1);
    c_value.shares = value;
    multiplexer(select_bits, c_value, select_result);

    sum(select_result, out);
    return 0;
}

}  // namespace ppam
}  // namespace privacy_go
