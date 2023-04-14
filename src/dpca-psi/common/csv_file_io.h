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
#include <string>
#include <utility>
#include <vector>

#include "dpca-psi/common/file_io.h"

namespace dpca_psi {

class CsvFileIO : public FileIO {
public:
    // Reads keys and features form csv file.
    // keys: std::vector<std::vector<std::string>>
    // features: std::vector<std::vector<std::uint64_t>>
    // has_header: if true, ignore the first raw.
    std::pair<std::vector<std::vector<std::string>>, std::vector<std::vector<std::uint64_t>>> read_data_from_file(
            const std::string& file_path, bool has_header, std::size_t key_size) override;

    // Writes keys and features to csv file.
    // keys: std::vector<std::vector<std::string>>
    // features: std::vector<std::vector<std::uint64_t>>
    // has_header: if true, place the header in the first raw of file.
    void write_data_to_file(const std::vector<std::vector<std::string>>& keys,
            const std::vector<std::vector<std::uint64_t>>& features, const std::string& file_path, bool has_header,
            const std::vector<std::string>& header) override;

    // Reads mpc shares from local csv file.
    std::vector<std::vector<std::uint64_t>> read_shares_from_file(const std::string& file_path) override;

    // Writes mpc shares to local csv file.
    void write_shares_to_file(
            const std::vector<std::vector<std::uint64_t>>& shares, const std::string& file_path) override;

private:
    std::pair<std::size_t, std::size_t> count_rows_and_coulmns(std::ifstream& in, bool has_header);
};
}  // namespace dpca_psi
