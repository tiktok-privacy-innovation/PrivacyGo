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

#include <cassert>
#include <memory>

#include "dpca-psi/common/utils.h"

namespace dpca_psi {

class IOChannel {
public:
    IOChannel() {
    }

    ~IOChannel() {
    }

    virtual void send_data(const void* data, std::size_t nbyte) = 0;

    virtual void recv_data(void* data, std::size_t nbyte) = 0;

    virtual void send_block(const block* data, std::size_t nblock) = 0;

    virtual void recv_block(block* data, std::size_t nblock) = 0;

    virtual void send_bool(bool* data, std::size_t length) = 0;

    virtual void recv_bool(bool* data, std::size_t length) = 0;

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

    virtual void send_string(const std::string& data) = 0;

    virtual void recv_string(std::string& data) = 0;

    virtual std::uint64_t get_counter() = 0;

    virtual void sync() = 0;
};
}  // namespace dpca_psi
