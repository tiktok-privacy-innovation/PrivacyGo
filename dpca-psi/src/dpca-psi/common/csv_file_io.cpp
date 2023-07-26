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

#include "dpca-psi/common/csv_file_io.h"

#include <sstream>

namespace privacy_go {
namespace dpca_psi {

std::pair<std::vector<std::vector<std::string>>, std::vector<std::vector<std::uint64_t>>>
CsvFileIO::read_data_from_file(const std::string& file_path, bool has_header, std::size_t key_size) {
    std::ifstream in(file_path);
    auto count = count_rows_and_coulmns(in, has_header);
    std::string line;
    if (has_header) {
        std::getline(in, line);
    }

    std::vector<std::vector<std::string>> keys;
    std::vector<std::vector<std::uint64_t>> features(count.second - key_size);
    keys.resize(key_size);
    while (std::getline(in, line)) {
        if (!line.empty()) {
            std::string column_str;
            std::stringstream ss(line);
            for (std::size_t i = 0; i < key_size; ++i) {
                std::getline(ss, column_str, ',');
                keys[i].emplace_back(column_str);
            }
            for (std::size_t i = 0; i < count.second - key_size; ++i) {
                std::getline(ss, column_str, ',');
                features[i].emplace_back(std::stoull(column_str));
            }
        }
    }
    in.close();
    return std::make_pair(std::move(keys), std::move(features));
}

void CsvFileIO::write_data_to_file(const std::vector<std::vector<std::string>>& keys,
        const std::vector<std::vector<std::uint64_t>>& features, const std::string& file_path, bool has_header,
        const std::vector<std::string>& header) {
    std::ofstream out(file_path);
    if (has_header) {
        for (std::size_t i = 0; i < header.size(); ++i) {
            if (i != header.size() - 1) {
                out << header[i] << ',';
            } else {
                out << header[i] << "\n";
            }
        }
    }

    for (std::size_t j = 0; j < keys[0].size(); ++j) {
        for (std::size_t i = 0; i < keys.size(); ++i) {
            out << keys[i][j] << ',';
        }
        for (std::size_t i = 0; i < features.size(); ++i) {
            if (i != features.size() - 1) {
                out << features[i][j] << ',';
            } else {
                out << features[i][j] << "\n";
            }
        }
    }
    out.close();
}

std::vector<std::vector<std::uint64_t>> CsvFileIO::read_shares_from_file(const std::string& file_path) {
    std::ifstream in(file_path);
    auto count = count_rows_and_coulmns(in, false);
    std::string line;
    std::vector<std::vector<std::uint64_t>> shares(count.second);
    while (std::getline(in, line)) {
        if (!line.empty()) {
            std::string column_str;
            std::stringstream ss(line);
            for (std::size_t i = 0; i < count.second; ++i) {
                std::getline(ss, column_str, ',');
                shares[i].emplace_back(std::stoull(column_str));
            }
        }
    }
    in.close();
    return std::move(shares);
}

void CsvFileIO::write_shares_to_file(
        const std::vector<std::vector<std::uint64_t>>& shares, const std::string& file_path) {
    std::ofstream out(file_path);
    for (std::size_t j = 0; j < shares[0].size(); ++j) {
        for (std::size_t i = 0; i < shares.size(); ++i) {
            if (i != shares.size() - 1) {
                out << shares[i][j] << ',';
            } else {
                out << shares[i][j] << "\n";
            }
        }
    }
    out.close();
}

std::pair<std::size_t, std::size_t> CsvFileIO::count_rows_and_coulmns(std::ifstream& in, bool has_header) {
    in.clear();
    in.seekg(0, in.beg);

    std::size_t raws = 0;
    std::size_t cols = 0;

    std::string line;
    std::getline(in, line);
    std::string column_str;
    std::stringstream ss(line);
    while (std::getline(ss, column_str, ',')) {
        if (!column_str.empty())
            ++cols;
    }
    if (!has_header) {
        ++raws;
    }
    while (getline(in, line)) {
        if (!line.empty()) {
            ++raws;
        }
    }

    in.clear();
    in.seekg(0, in.beg);
    return std::make_pair(raws, cols);
}

}  // namespace dpca_psi
}  // namespace privacy_go
