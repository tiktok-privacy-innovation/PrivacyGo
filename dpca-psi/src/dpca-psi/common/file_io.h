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

#include <string>
#include <utility>
#include <vector>

namespace privacy_go {
namespace dpca_psi {

class FileIO {
public:
    // Reads keys and features form file.
    // keys: std::vector<std::vector<std::string>>
    // features: std::vector<std::vector<std::uint64_t>>
    // has_header: if true, ignore the first raw.
    virtual std::pair<std::vector<std::vector<std::string>>, std::vector<std::vector<std::uint64_t>>>
    read_data_from_file(const std::string& file_path, bool has_header, std::size_t key_size) = 0;

    // Writes keys and features to file.
    // keys: std::vector<std::vector<std::string>>
    // features: std::vector<std::vector<std::uint64_t>>
    // has_header: if true, place the header in the first raw of file.
    virtual void write_data_to_file(const std::vector<std::vector<std::string>>& keys,
            const std::vector<std::vector<std::uint64_t>>& features, const std::string& file_path, bool hash_header,
            const std::vector<std::string>& header) = 0;

    // Reads mpc shares from local file.
    virtual std::vector<std::vector<std::uint64_t>> read_shares_from_file(const std::string& file_path) = 0;

    // Writes mpc shares to local file.
    virtual void write_shares_to_file(
            const std::vector<std::vector<std::uint64_t>>& shares, const std::string& file_path) = 0;
};

}  // namespace dpca_psi
}  // namespace privacy_go
