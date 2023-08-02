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

#include <stdlib.h>
#include <time.h>

#include <algorithm>
#include <memory>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "ppam/mpc/common/defines.h"

namespace privacy_go {
namespace ppam {

class AbyTest : public ::testing::Test {
public:
    void SetUp() {
        mpc_party0_ = 0;
        mpc_party1_ = 1;
    }

    bool is_equal_plain_matrix(const eMatrix<double>& m1, const eMatrix<double>& m2, double e) {
        if ((m1.rows() != m2.rows()) || (m1.cols() != m2.cols())) {
            return false;
        }

        for (int i = 0; i < m1.size(); i++) {
            double d = m1(i) - m2(i);
            if ((d < -e) || (d > e)) {
                return false;
            }
        }

        return true;
    }

    double get_rand_double() {
        double res;
        double threshold = 1.0 * (1 << 16);
        unsigned int seed = time(NULL);
        do {
            res = 1.0 * rand_r(&seed) / (rand_r(&seed) + 1);
            if (rand_r(&seed) & 1) {
                res = -res;
            }
        } while ((res > threshold) || (res < -threshold));

        return res;
    }

    void get_rand_plain_matrix(eMatrix<double>& plain) {
        for (int i = 0; i < plain.size(); i++) {
            plain(i) = get_rand_double();
        }
    }

    void add_test(std::size_t& party) {
        auto aby_test = AbyProtocol::Instance();
        std::uint16_t remote_port = (party == mpc_party0_) ? 9989 : 9990;
        std::uint16_t local_port = (party == mpc_party0_) ? 9990 : 9989;
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", remote_port, local_port);
        aby_test->initialize(party, net);

        eMatrix<double> plain_a(4, 3);
        eMatrix<double> plain_b(4, 3);
        eMatrix<double> plain_c(4, 3);
        CryptoMatrix cipher_a(4, 3);
        CryptoMatrix cipher_b(4, 3);
        CryptoMatrix cipher_c(4, 3);

        get_rand_plain_matrix(plain_a);
        get_rand_plain_matrix(plain_b);

        aby_test->share(0, plain_a, cipher_a);
        aby_test->share(0, plain_b, cipher_b);

        aby_test->add(cipher_a, cipher_b, cipher_c);

        aby_test->reveal(0, cipher_c, reveal_plain_);
        plain_ = plain_a + plain_b;

        return;
    }

    void sub_test(std::size_t& party) {
        auto aby_test = AbyProtocol::Instance();

        std::uint16_t remote_port = (party == mpc_party0_) ? 9989 : 9990;
        std::uint16_t local_port = (party == mpc_party0_) ? 9990 : 9989;
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", remote_port, local_port);
        aby_test->initialize(party, net);

        eMatrix<double> plain_a(4, 3);
        eMatrix<double> plain_b(4, 3);
        eMatrix<double> plain_c(4, 3);
        CryptoMatrix cipher_a(4, 3);
        CryptoMatrix cipher_b(4, 3);
        CryptoMatrix cipher_c(4, 3);
        get_rand_plain_matrix(plain_a);
        get_rand_plain_matrix(plain_b);

        aby_test->share(0, plain_a, cipher_a);
        aby_test->share(0, plain_b, cipher_b);

        aby_test->sub(cipher_a, cipher_b, cipher_c);

        aby_test->reveal(0, cipher_c, reveal_plain_);
        plain_ = plain_a - plain_b;

        return;
    }

    void greater_test(std::size_t& party) {
        auto aby_test = AbyProtocol::Instance();

        std::uint16_t remote_port = (party == mpc_party0_) ? 9989 : 9990;
        std::uint16_t local_port = (party == mpc_party0_) ? 9990 : 9989;
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", remote_port, local_port);
        aby_test->initialize(party, net);

        eMatrix<double> plain_a(4, 3);
        eMatrix<double> plain_b(4, 3);
        eMatrix<double> plain_c(4, 3);
        eMatrix<double> plain_d(4, 3);
        CryptoMatrix cipher_a(4, 3);
        CryptoMatrix cipher_b(4, 3);
        CryptoMatrix cipher_c(4, 3);
        CryptoMatrix cipher_d(4, 3);

        get_rand_plain_matrix(plain_a);
        get_rand_plain_matrix(plain_b);
        get_rand_plain_matrix(plain_d);

        aby_test->share(0, plain_a, cipher_a);
        aby_test->share(0, plain_b, cipher_b);
        aby_test->share(0, plain_d, cipher_d);

        aby_test->greater(cipher_a, cipher_b, cipher_c);
        aby_test->multiplexer(cipher_c, cipher_d, cipher_b);

        reveal_plain_.resize(4, 3);
        aby_test->reveal(0, cipher_b, reveal_plain_);

        plain_.resize(4, 3);
        for (int i = 0; i < plain_d.size(); i++) {
            plain_(i) = (plain_a(i) > plain_b(i)) ? plain_d(i) : 0;
        }

        return;
    }

    void less_test(std::size_t& party) {
        auto aby_test = AbyProtocol::Instance();

        std::uint16_t remote_port = (party == mpc_party0_) ? 9989 : 9990;
        std::uint16_t local_port = (party == mpc_party0_) ? 9990 : 9989;
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", remote_port, local_port);
        aby_test->initialize(party, net);

        eMatrix<double> plain_a(4, 3);
        eMatrix<double> plain_b(4, 3);
        eMatrix<double> plain_c(4, 3);
        eMatrix<double> plain_d(4, 3);
        CryptoMatrix cipher_a(4, 3);
        CryptoMatrix cipher_b(4, 3);
        CryptoMatrix cipher_c(4, 3);
        CryptoMatrix cipher_d(4, 3);

        get_rand_plain_matrix(plain_a);
        get_rand_plain_matrix(plain_b);
        get_rand_plain_matrix(plain_d);

        aby_test->share(0, plain_a, cipher_a);
        aby_test->share(0, plain_b, cipher_b);
        aby_test->share(0, plain_d, cipher_d);

        aby_test->less(cipher_a, cipher_b, cipher_c);
        aby_test->multiplexer(cipher_c, cipher_d, cipher_b);

        reveal_plain_.resize(4, 3);
        aby_test->reveal(0, cipher_b, reveal_plain_);

        plain_.resize(4, 3);
        for (int i = 0; i < plain_d.size(); i++) {
            plain_(i) = (plain_a(i) < plain_b(i)) ? plain_d(i) : 0;
        }

        return;
    }

    void sum_test(std::size_t& party) {
        auto aby_test = AbyProtocol::Instance();

        std::uint16_t remote_port = (party == mpc_party0_) ? 9989 : 9990;
        std::uint16_t local_port = (party == mpc_party0_) ? 9990 : 9989;
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", remote_port, local_port);
        aby_test->initialize(party, net);

        eMatrix<double> plain_a(4, 3);
        eMatrix<double> plain_b(4, 3);
        CryptoMatrix cipher_a(4, 3);
        CryptoMatrix cipher_b(4, 3);

        get_rand_plain_matrix(plain_a);
        get_rand_plain_matrix(plain_b);

        aby_test->share(0, plain_a, cipher_a);
        aby_test->share(0, plain_b, cipher_b);

        aby_test->sum(cipher_a, cipher_b);

        reveal_plain_.resize(1, 3);
        aby_test->reveal(0, cipher_b, reveal_plain_);

        plain_.resize(1, 3);
        plain_ = plain_a.colwise().sum();

        return;
    }

    void attribution_test(std::size_t& party) {
        auto aby_test = AbyProtocol::Instance();

        std::uint16_t remote_port = (party == mpc_party0_) ? 9989 : 9990;
        std::uint16_t local_port = (party == mpc_party0_) ? 9990 : 9989;
        auto net = std::make_shared<TwoChannelNetIO>("127.0.0.1", remote_port, local_port);
        aby_test->initialize(party, net);

        eMatrix<double> plain_a(4, 3);
        eMatrix<double> plain_b(4, 3);
        eMatrix<double> plain_col(4, 1);
        CryptoMatrix cipher_a(4, 3);
        CryptoMatrix cipher_b(4, 3);

        get_rand_plain_matrix(plain_a);
        get_rand_plain_matrix(plain_b);

        aby_test->share(0, plain_a, cipher_a);
        aby_test->share(0, plain_b, cipher_b);

        aby_test->attribution(0.5, cipher_a, cipher_b);

        reveal_plain_.resize(1, 1);
        aby_test->reveal(0, cipher_b, reveal_plain_);

        plain_.resize(1, 1);
        auto t = plain_a.col(1) - plain_a.col(0);
        for (int i = 0; i < t.size(); i++) {
            plain_col(i) = (t(i) > 0) & (t(i) < 0.5);
        }
        plain_ = plain_col.cwiseProduct(plain_a.col(2)).colwise().sum();

        return;
    }

public:
    std::size_t mpc_party0_;
    std::size_t mpc_party1_;
    eMatrix<double> reveal_plain_;
    eMatrix<double> plain_;
};

TEST_F(AbyTest, add_test) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        status = -1;
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        add_test(mpc_party1_);
        exit(EXIT_SUCCESS);
    } else {
        add_test(mpc_party0_);
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
        ASSERT_TRUE(is_equal_plain_matrix(reveal_plain_, plain_, 0.001));
        return;
    }
}

TEST_F(AbyTest, sub_test) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        status = -1;
    } else if (pid == 0) {
        sub_test(mpc_party1_);
        exit(EXIT_SUCCESS);
    } else {
        sub_test(mpc_party0_);
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
        ASSERT_TRUE(is_equal_plain_matrix(reveal_plain_, plain_, 0.001));
    }
}

TEST_F(AbyTest, greater_test) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        status = -1;
    } else if (pid == 0) {
        greater_test(mpc_party1_);
        exit(EXIT_SUCCESS);
    } else {
        greater_test(mpc_party0_);
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
        ASSERT_TRUE(is_equal_plain_matrix(reveal_plain_, plain_, 0.001));
    }
}

TEST_F(AbyTest, less_test) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        status = -1;
    } else if (pid == 0) {
        greater_test(mpc_party1_);
        exit(EXIT_SUCCESS);
    } else {
        greater_test(mpc_party0_);
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
        ASSERT_TRUE(is_equal_plain_matrix(reveal_plain_, plain_, 0.001));
    }
}

TEST_F(AbyTest, sum_test) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        status = -1;
    } else if (pid == 0) {
        sum_test(mpc_party1_);
        exit(EXIT_SUCCESS);
    } else {
        sum_test(mpc_party0_);
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
        ASSERT_TRUE(is_equal_plain_matrix(reveal_plain_, plain_, 0.001));
    }
}

TEST_F(AbyTest, attribution_test) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        status = -1;
    } else if (pid == 0) {
        attribution_test(mpc_party1_);
        exit(EXIT_SUCCESS);
    } else {
        attribution_test(mpc_party0_);
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
        ASSERT_TRUE(is_equal_plain_matrix(reveal_plain_, plain_, 0.001));
    }
}

}  // namespace ppam
}  // namespace privacy_go
