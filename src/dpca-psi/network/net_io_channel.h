// Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// Enquiries about further applications and development opportunities are welcome.
//
// This file may have been modified by TikTok Pte. Ltd. All TikTok's Modifications
// are Copyright (2023) TikTok Pte. Ltd.

#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "dpca-psi/network/io_channel.h"

namespace dpca_psi {

class NetIO : public IOChannel {
public:
    NetIO(const char* address, const int port, bool quiet = false, bool ipv6 = false) {
        if (port < 0 || port > 65535) {
            throw std::runtime_error("Invalid port number!");
        }

        this->port_ = port;
        is_server_ = (address == nullptr);
        if (address == nullptr) {
            if (ipv6) {
                struct sockaddr_in6 dest;
                struct sockaddr_in6 serv;
                socklen_t socksize = sizeof(struct sockaddr_in6);
                memset(&serv, 0, sizeof(serv));
                serv.sin6_family = AF_INET6;
                serv.sin6_addr = in6addr_any;
                serv.sin6_port = htons(static_cast<std::uint16_t>(port));
                mysocket_ = socket(AF_INET6, SOCK_STREAM, 0);
                int reuse = 1;
                setsockopt(mysocket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
                if (bind(mysocket_, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
                    perror("error: bind");
                    exit(1);
                }
                if (listen(mysocket_, 1) < 0) {
                    perror("error: listen");
                    exit(1);
                }
                consocket_ = accept(mysocket_, (struct sockaddr*)&dest, &socksize);
                close(mysocket_);
            } else {
                struct sockaddr_in dest;
                struct sockaddr_in serv;
                socklen_t socksize = sizeof(struct sockaddr_in);
                memset(&serv, 0, sizeof(serv));
                serv.sin_family = AF_INET;
                serv.sin_addr.s_addr = htonl(INADDR_ANY);
                serv.sin_port = htons(static_cast<std::uint16_t>(port));
                mysocket_ = socket(AF_INET, SOCK_STREAM, 0);
                int reuse = 1;
                setsockopt(mysocket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
                if (bind(mysocket_, (struct sockaddr*)&serv, sizeof(struct sockaddr)) < 0) {
                    perror("error: bind");
                    exit(1);
                }
                if (listen(mysocket_, 1) < 0) {
                    perror("error: listen");
                    exit(1);
                }
                consocket_ = accept(mysocket_, (struct sockaddr*)&dest, &socksize);
                close(mysocket_);
            }
        } else {
            if (ipv6) {
                addr_ = std::string(address);

                struct sockaddr_in6 dest;
                memset(&dest, 0, sizeof(dest));
                dest.sin6_family = AF_INET6;
                inet_pton(AF_INET6, address, &dest.sin6_addr);
                dest.sin6_port = htons(static_cast<std::uint16_t>(port));

                while (1) {
                    consocket_ = socket(AF_INET6, SOCK_STREAM, 0);

                    if (connect(consocket_, (struct sockaddr*)&dest, sizeof(dest)) == 0) {
                        break;
                    }

                    close(consocket_);
                    usleep(1000);
                }
            } else {
                addr_ = std::string(address);

                struct sockaddr_in dest;
                memset(&dest, 0, sizeof(dest));
                dest.sin_family = AF_INET;
                dest.sin_addr.s_addr = inet_addr(address);
                dest.sin_port = htons(static_cast<std::uint16_t>(port));

                while (1) {
                    consocket_ = socket(AF_INET, SOCK_STREAM, 0);

                    if (connect(consocket_, (struct sockaddr*)&dest, sizeof(struct sockaddr)) == 0) {
                        break;
                    }

                    close(consocket_);
                    usleep(1000);
                }
            }
        }
        set_nodelay();
        stream_ = fdopen(consocket_, "wb+");
        buffer_ = new char[1024 * 1024];
        memset(buffer_, 0, 1024 * 1024);
        setvbuf(stream_, buffer_, _IOFBF, 1024 * 1024);
        if (!quiet) {
            std::cout << "connected\n";
        }
    }

    ~NetIO() {
        flush();
        fclose(stream_);
        delete[] buffer_;
    }

    void sync() {
        int tmp = 0;
        if (is_server_) {
            send_data_internal(&tmp, 1);
            recv_data_internal(&tmp, 1);
        } else {
            recv_data_internal(&tmp, 1);
            send_data_internal(&tmp, 1);
            flush();
        }
    }

    void set_nodelay() {
        const int one = 1;
        setsockopt(consocket_, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    }

    void set_delay() {
        const int zero = 0;
        setsockopt(consocket_, IPPROTO_TCP, TCP_NODELAY, &zero, sizeof(zero));
    }

    void flush() {
        fflush(stream_);
    }

    void send_data_internal(const void* data, std::size_t len) {
        std::size_t sent = 0;
        while (sent < len) {
            auto* ptr = const_cast<void*>(data);
            std::size_t res = fwrite(sent + reinterpret_cast<char*>(ptr), 1, len - sent, stream_);
            if (res > 0) {
                sent += res;
            } else {
                exit(EXIT_FAILURE);
            }
        }
        has_sent_ = true;
    }

    void recv_data_internal(void* data, std::size_t len) {
        if (has_sent_) {
            fflush(stream_);
        }
        has_sent_ = false;
        std::size_t sent = 0;
        while (sent < len) {
            std::size_t res = fread(sent + reinterpret_cast<char*>(data), 1, len - sent, stream_);
            if (res > 0) {
                sent += res;
            } else {
                exit(EXIT_FAILURE);
            }
        }
    }

    void send_data(const void* data, std::size_t nbyte) {
        counter_ += nbyte;
        derived().send_data_internal(data, nbyte);
    }

    void recv_data(void* data, std::size_t nbyte) {
        derived().recv_data_internal(data, nbyte);
    }

    void send_block(const block* data, std::size_t nblock) {
        send_data(data, nblock * sizeof(block));
    }

    void recv_block(block* data, std::size_t nblock) {
        recv_data(data, nblock * sizeof(block));
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

    void send_string(const std::string& data) {
        send_value(data.size());
        if (data.size() != 0) {
            send_data(data.data(), data.size());
        }
        return;
    }

    void recv_string(std::string& data) {
        std::size_t len = recv_value<std::size_t>();
        data.resize(len);
        if (len != 0) {
            recv_data(&data.at(0), len);
        }
        return;
    }

    std::uint64_t get_counter() override {
        return counter_;
    }

private:
    NetIO& derived() {
        return *this;
    }

    std::uint64_t counter_ = 0;
    bool is_server_;
    int mysocket_ = -1;
    int consocket_ = -1;
    FILE* stream_ = nullptr;
    char* buffer_ = nullptr;
    bool has_sent_ = false;
    std::string addr_;
    int port_;
};

}  // namespace dpca_psi
