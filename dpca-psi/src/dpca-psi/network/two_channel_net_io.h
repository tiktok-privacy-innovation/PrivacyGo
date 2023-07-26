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

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "dpca-psi/network/io_base.h"

namespace privacy_go {
namespace dpca_psi {

class TwoChannelNetIO : public IOBase {
public:
    TwoChannelNetIO() = delete;

    TwoChannelNetIO(const TwoChannelNetIO& other) = delete;

    TwoChannelNetIO& operator=(const TwoChannelNetIO& other) = delete;

    // The remote_port and local_port must be swapped and not the same if two instance running on same IP.
    // Example:
    // Machine A: TwoChannelNetIO("127.0.0.1", 1234, 4321);
    // Machine B: TwoChannelNetIO("127.0.0.1", 4321, 1234);
    TwoChannelNetIO(const std::string& remote_ip_address, std::uint16_t remote_port, std::uint16_t local_port);

    ~TwoChannelNetIO() override;

private:
    static void set_nodelay(int socket);

    static void set_delay(int socket);

    void send_data_impl(const void* data, std::size_t nbyte) override;

    void recv_data_impl(void* data, std::size_t nbyte) override;

    static int get_address_family(const std::string& ip_address);

    static int init_server(int domain, std::uint16_t port);

    static int init_client(int domain, const std::string& ip_address, std::uint16_t port);

    int send_socket_ = -1;
    int recv_socket_ = -1;
};

}  // namespace dpca_psi
}  // namespace privacy_go
