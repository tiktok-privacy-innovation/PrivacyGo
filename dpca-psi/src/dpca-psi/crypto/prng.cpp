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

#include "dpca-psi/crypto/prng.h"

#include <iostream>
#include <utility>

namespace privacy_go {
namespace dpca_psi {

PRNG::PRNG() : bytes_idx_(0), block_idx_(0), buffer_byte_capacity_(0) {
}

PRNG::PRNG(const block& seed, std::uint64_t buffer_size) : bytes_idx_(0), block_idx_(0), buffer_byte_capacity_(0) {
    set_seed(seed, buffer_size);
}

PRNG::PRNG(PRNG&& s)
        : buffer_(std::move(s.buffer_)),
          aes_(std::move(s.aes_)),
          bytes_idx_(s.bytes_idx_),
          block_idx_(s.block_idx_),
          buffer_byte_capacity_(s.buffer_byte_capacity_) {
    s.buffer_.resize(0);
    s.bytes_idx_ = 0;
    s.block_idx_ = 0;
    s.buffer_byte_capacity_ = 0;
}

void PRNG::operator=(PRNG&& s) {
    buffer_ = (std::move(s.buffer_));
    aes_ = (std::move(s.aes_));
    bytes_idx_ = (s.bytes_idx_);
    block_idx_ = (s.block_idx_);
    buffer_byte_capacity_ = (s.buffer_byte_capacity_);

    s.buffer_.resize(0);
    s.bytes_idx_ = 0;
    s.block_idx_ = 0;
    s.buffer_byte_capacity_ = 0;
}

void PRNG::set_seed(const block& seed, std::uint64_t buffer_size) {
    aes_.set_key(seed);
    block_idx_ = 0;

    if (buffer_.size() == 0) {
        buffer_.resize(buffer_size);
        buffer_byte_capacity_ = (sizeof(block) * buffer_size);
    }
    refill_buffer();
}

void PRNG::impl_get(std::uint8_t* dest_u8, std::uint64_t length_u8) {
    while (length_u8) {
        std::uint64_t step = std::min(length_u8, buffer_byte_capacity_ - bytes_idx_);
        memcpy(dest_u8, (reinterpret_cast<std::uint8_t*>(buffer_.data())) + bytes_idx_, step);

        dest_u8 += step;
        length_u8 -= step;
        bytes_idx_ += step;

        if (bytes_idx_ == buffer_byte_capacity_) {
            if (length_u8 >= 8 * sizeof(block)) {
                std::vector<block> b(length_u8 / sizeof(block));
                aes_.ecb_encrypt_counter_mode(block_idx_, b.size(), b.data());
                memcpy(dest_u8, reinterpret_cast<std::uint8_t*>(b.data()), b.size() * sizeof(block));  // NOLINT
                block_idx_ += b.size();

                step = b.size() * sizeof(block);
                dest_u8 += step;
                length_u8 -= step;
            }
            refill_buffer();
        }
    }
}

std::uint8_t PRNG::get_bit() {
    return get<bool>();
}

const block PRNG::get_seed() const {
    if (buffer_.size()) {
        return aes_.round_key_[0];
    } else {
        std::cout << "PRNG has not been keyed" << std::endl;
        return _mm_set_epi64x(0, 0);
    }
}

void PRNG::refill_buffer() {
    if (buffer_.size() == 0) {
        std::cout << "PRNG has not been keyed" << std::endl;
    }

    aes_.ecb_encrypt_counter_mode(block_idx_, buffer_.size(), buffer_.data());
    block_idx_ += buffer_.size();
    bytes_idx_ = 0;
}

}  // namespace dpca_psi
}  // namespace privacy_go
