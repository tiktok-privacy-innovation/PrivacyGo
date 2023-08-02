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

#pragma once

#include <fstream>
#include <random>
#include <sstream>

#include "ppam/mpc/common/utils.h"

namespace privacy_go {
namespace ppam {

inline int matrix_transpose(const std::vector<block>& in, const int& rows, const int& cols, std::vector<block>& out) {
    if ((rows % 128 != 0) || (cols % 128 != 0)) {
        return -1;
    }

    char* ptr_in = reinterpret_cast<char*>(const_cast<block*>(in.data()));
    char* ptr_out = reinterpret_cast<char*>(const_cast<block*>(out.data()));

    auto f = [&](int x, int y) { return ptr_in[x * cols / 8 + y / 8]; };
    auto g = [&](int x, int y) { return &(ptr_out[y * rows / 8 + x / 8]); };

    for (int i = 0; i < rows; i += 16) {
        for (int j = 0; j < cols; j += 8) {
            auto data = _mm_set_epi8(f(i + 15, j), f(i + 14, j), f(i + 13, j), f(i + 12, j), f(i + 11, j), f(i + 10, j),
                    f(i + 9, j), f(i + 8, j), f(i + 7, j), f(i + 6, j), f(i + 5, j), f(i + 4, j), f(i + 3, j),
                    f(i + 2, j), f(i + 1, j), f(i, j));

            for (int k = 7; k >= 0; --k) {
                *reinterpret_cast<uint16_t*>(g(i, j + k)) = static_cast<uint16_t>(_mm_movemask_epi8(data));
                data = _mm_slli_epi64(data, 1);
            }
        }
    }

    return 0;
}

inline void send_matrix(const std::shared_ptr<IOBase>& net, const CryptoMatrix* data, std::size_t nmatrix) {
    net->send_data(data->shares.data(), nmatrix * data->shares.size() * sizeof(int64_t));
}

inline void recv_matrix(const std::shared_ptr<IOBase>& net, CryptoMatrix* data, std::size_t nmatrix) {
    std::vector<int64_t> buf(data->shares.size() * nmatrix);
    net->recv_data(&buf[0], nmatrix * data->shares.size() * sizeof(int64_t));
    data->shares = Eigen::Map<eMatrix<int64_t>>(buf.data(), data->shares.rows(), data->shares.cols());
}

inline block read_block_from_dev_urandom() {
    block ret;
    std::ifstream in("/dev/urandom");
    in.read(reinterpret_cast<char*>(&ret), sizeof(ret));
    in.close();
    return ret;
}

template <typename T>
inline T read_data_from_dev_urandom() {
    T ret;
    std::ifstream in("/dev/urandom");
    in.read(reinterpret_cast<char*>(&ret), sizeof(ret));
    in.close();
    return ret;
}

}  // namespace ppam
}  // namespace privacy_go
