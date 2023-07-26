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

#include <cstddef>
#include <memory>
#include <string>

#include "dpca-psi/common/defines.h"

namespace privacy_go {
namespace dpca_psi {

class IOBase {
public:
    IOBase() = default;

    virtual ~IOBase() = default;

    // The send and receive data function handling some basic operation such as
    // count the data flow size and header metadata.
    // Leave the detailed data transfer implementation to child class.
    void send_data(const void* data, std::size_t nbyte) {
        send_data_impl(data, nbyte);
        bytes_sent_ += nbyte;
    }

    void recv_data(void* data, std::size_t nbyte) {
        recv_data_impl(data, nbyte);
        bytes_received_ += nbyte;
    }

    void send_block(const block* data, std::size_t nblock) {
        send_data(data, nblock * sizeof(block));
    }

    void recv_block(block* data, std::size_t nblock) {
        recv_data(data, nblock * sizeof(block));
    }

    // Can only be used for primitive type. Because the actual data size will be incorrect by using sizeof() for
    // std::string or std::vector etc.
    template <typename T>
    void send_value(T val) {
        send_data(&val, sizeof(val));
    }

    template <typename T>
    T recv_value() {
        T val;
        recv_data(&val, sizeof(val));
        return val;
    }

    void send_string(const std::string& msg) {
        std::size_t len = msg.size();
        send_data(&len, sizeof(std::size_t));
        send_data(msg.data(), msg.size());
    }

    std::string recv_string() {
        std::size_t len;
        recv_data(&len, sizeof(std::size_t));
        std::string msg(len, '\0');
        recv_data(&msg[0], len);
        return msg;
    }

    void send_bytes(const ByteVector& data) {
        send_value(data.size());
        if (data.size() != 0) {
            send_data(reinterpret_cast<const unsigned char*>(data.data()), data.size());
        }
        return;
    }

    void recv_bytes(ByteVector& data) {
        std::size_t len = recv_value<std::size_t>();
        data.resize(len);
        if (len != 0) {
            recv_data(reinterpret_cast<unsigned char*>(data.data()), len);
        }
        return;
    }

    void send_bool(bool* data, std::size_t length) {
        void* ptr = reinterpret_cast<void*>(data);
        std::size_t space = length;
        const void* aligned = std::align(alignof(std::uint64_t), sizeof(std::uint64_t), ptr, space);
        if (aligned == nullptr) {
            send_data(data, length);
        } else {
            std::size_t diff = length - space;
            send_data(data, diff);
            send_bool_aligned((const bool*)aligned, length - diff);
        }
    }

    void recv_bool(bool* data, std::size_t length) {
        void* ptr = reinterpret_cast<void*>(data);
        std::size_t space = length;
        void* aligned = std::align(alignof(std::uint64_t), sizeof(std::uint64_t), ptr, space);
        if (aligned == nullptr) {
            recv_data(data, length);
        } else {
            std::size_t diff = length - space;
            recv_data(data, diff);
            recv_bool_aligned(reinterpret_cast<bool*>(aligned), length - diff);
        }
    }

    std::uint64_t get_bytes_sent() {
        return bytes_sent_;
    }

    std::uint64_t get_bytes_received() {
        return bytes_received_;
    }

private:
    // Implementation details for send and receiving data.
    virtual void send_data_impl(const void* data, std::size_t nbyte) = 0;

    virtual void recv_data_impl(void* data, std::size_t nbyte) = 0;

    void send_bool_aligned(const bool* data, std::size_t length) {
        const std::uint64_t* data64 = reinterpret_cast<const std::uint64_t*>(data);
        std::size_t i = 0;
        for (; i < length / 8; ++i) {
            std::uint64_t mask = 0x0101010101010101ULL;
            std::uint64_t tmp = 0;
#if defined(__BMI2__)
            tmp = _pext_u64(data64[i], mask);
#else
            // https://github.com/Forceflow/libmorton/issues/6
            for (std::uint64_t bb = 1; mask != 0; bb += bb) {
                if (data64[i] & mask & -mask) {
                    tmp |= bb;
                }
                mask &= (mask - 1);
            }
#endif
            send_data(&tmp, 1);
        }
        if (8 * i != length)
            send_data(data + 8 * i, length - 8 * i);
    }

    void recv_bool_aligned(bool* data, std::size_t length) {
        std::uint64_t* data64 = reinterpret_cast<std::uint64_t*>(data);
        std::size_t i = 0;
        for (; i < length / 8; ++i) {
            std::uint64_t mask = 0x0101010101010101ULL;
            std::uint64_t tmp = 0;
            recv_data(&tmp, 1);
#if defined(__BMI2__)
            data64[i] = _pdep_u64(tmp, mask);
#else
            data64[i] = 0;
            for (std::uint64_t bb = 1; mask != 0; bb += bb) {
                if (tmp & bb) {
                    data64[i] |= mask & (-mask);
                }
                mask &= (mask - 1);
            }
#endif
        }
        if (8 * i != length)
            recv_data(data + 8 * i, length - 8 * i);
    }

    std::uint64_t bytes_sent_ = 0;
    std::uint64_t bytes_received_ = 0;
};

}  // namespace dpca_psi
}  // namespace privacy_go
