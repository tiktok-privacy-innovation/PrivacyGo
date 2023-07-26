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

#include "dpca-psi/network/two_channel_net_io.h"

#include <iostream>
#include <thread>

namespace privacy_go {
namespace dpca_psi {

TwoChannelNetIO::TwoChannelNetIO(
        const std::string& remote_ip_address, std::uint16_t remote_port, std::uint16_t local_port) {
    int domain = get_address_family(remote_ip_address);

    // Using two threads running initServer and initClient in parallel to resolve  two instances dead lock.
    std::thread server_thread([this, domain, local_port]() {
        send_socket_ = init_server(domain, local_port);
        set_nodelay(send_socket_);
    });

    std::thread client_thread([this, domain, &remote_ip_address, remote_port]() {
        recv_socket_ = init_client(domain, remote_ip_address, remote_port);
        set_nodelay(recv_socket_);
    });

    server_thread.join();
    client_thread.join();
}

TwoChannelNetIO::~TwoChannelNetIO() {
    if (send_socket_ >= 0) {
        close(send_socket_);
    }
    if (recv_socket_ >= 0) {
        close(recv_socket_);
    }
}

void TwoChannelNetIO::set_nodelay(int socket) {
    const int one = 1;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

void TwoChannelNetIO::set_delay(int socket) {
    const int zero = 0;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &zero, sizeof(zero));
}

void TwoChannelNetIO::send_data_impl(const void* data, std::size_t nbyte) {
    std::size_t sent = 0;
    while (sent < nbyte) {
        ssize_t res = send(send_socket_, reinterpret_cast<const char*>(data) + sent, nbyte - sent, 0);
        if (res > 0) {
            sent += res;
        } else {
            perror("send");
            exit(EXIT_FAILURE);
        }
    }
}

void TwoChannelNetIO::recv_data_impl(void* data, std::size_t nbyte) {
    std::size_t received = 0;
    while (received < nbyte) {
        ssize_t res = recv(recv_socket_, reinterpret_cast<char*>(data) + received, nbyte - received, 0);
        if (res > 0) {
            received += res;
        } else {
            perror("recv");
            exit(EXIT_FAILURE);
        }
    }
}

// static
int TwoChannelNetIO::get_address_family(const std::string& ip_address) {
    addrinfo hints;
    addrinfo* res;
    memset(&hints, 0, sizeof(hints));
    // Allow both IPv4 and IPv6
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Get address family
    int ret = getaddrinfo(ip_address.c_str(), nullptr, &hints, &res);
    if (ret != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }
    int domain = res->ai_family;
    freeaddrinfo(res);
    return domain;
}

// static
int TwoChannelNetIO::init_server(int domain, std::uint16_t port) {
    sockaddr_storage serv;
    memset(&serv, 0, sizeof(serv));
    socklen_t socksize;
    if (domain == AF_INET) {
        auto* serv_in = reinterpret_cast<sockaddr_in*>(&serv);
        serv_in->sin_family = AF_INET;
        serv_in->sin_addr.s_addr = htonl(INADDR_ANY);
        serv_in->sin_port = htons(port);
        socksize = sizeof(sockaddr_in);
    } else if (domain == AF_INET6) {
        auto* serv_in6 = reinterpret_cast<sockaddr_in6*>(&serv);
        serv_in6->sin6_family = AF_INET6;
        serv_in6->sin6_addr = in6addr_any;
        serv_in6->sin6_port = htons(port);
        socksize = sizeof(sockaddr_in6);
    } else {
        std::cerr << "Invalid domain. Use AF_INET or AF_INET6." << std::endl;
        exit(EXIT_FAILURE);
    }
    int server_socket = socket(domain, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    if (bind(server_socket, (struct sockaddr*)&serv, socksize) < 0) {
        perror("error: bind");
        exit(1);
    }
    if (listen(server_socket, 1) < 0) {
        perror("error: listen");
        exit(1);
    }
    sockaddr_storage dest;
    int send_socket = accept(server_socket, (struct sockaddr*)&dest, &socksize);
    close(server_socket);
    return send_socket;
}

// static
int TwoChannelNetIO::init_client(int domain, const std::string& ip_address, std::uint16_t port) {
    std::cout << "Connecting to: " << ip_address << std::endl;
    sockaddr_storage dest;
    memset(&dest, 0, sizeof(dest));
    if (domain == AF_INET) {
        auto* dest_in = reinterpret_cast<sockaddr_in*>(&dest);
        dest_in->sin_family = AF_INET;
        dest_in->sin_addr.s_addr = inet_addr(ip_address.c_str());
        dest_in->sin_port = htons(port);
    } else if (domain == AF_INET6) {
        auto* dest_in6 = reinterpret_cast<sockaddr_in6*>(&dest);
        dest_in6->sin6_family = AF_INET6;
        inet_pton(AF_INET6, ip_address.c_str(), &dest_in6->sin6_addr);
        dest_in6->sin6_port = htons(port);
    } else {
        std::cerr << "Invalid domain. Use AF_INET or AF_INET6." << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        int client_socket = socket(domain, SOCK_STREAM, 0);
        if (connect(client_socket, (struct sockaddr*)&dest, sizeof(dest)) == 0) {
            std::cout << "Connected to IP: " << ip_address << ":" << port << std::endl;
            return client_socket;
        }
        close(client_socket);
        usleep(1000);
    }
    return -1;
}

}  // namespace dpca_psi
}  // namespace privacy_go
