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

#include "gtest/gtest.h"

namespace dpca_psi {

class CsvFileIOTest : public ::testing::Test {
public:
    std::string tmp_data_path = "/tmp/tmp_data.csv";
    static void SetUpTestCase() {
    }
    static void TearDownTestCase() {
        std::remove("/tmp/tmp_data.csv");
    }
};

TEST_F(CsvFileIOTest, read_write_data_with_header) {
    std::vector<std::vector<std::string>> keys = {{"id1", "id2", "id3"}, {"ip1", "ip2", "ip3"}};
    std::vector<std::vector<std::uint64_t>> features = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    std::vector<std::string> header = {"ID", "IP", "feature1", "feature2", "feature3"};
    CsvFileIO csv;
    csv.write_data_to_file(keys, features, tmp_data_path, true, header);
    auto data = csv.read_data_from_file(tmp_data_path, true, 2);
    ASSERT_EQ(keys, data.first);
    ASSERT_EQ(features, data.second);
}

TEST_F(CsvFileIOTest, read_write_data_without_header) {
    std::vector<std::vector<std::string>> keys = {{"id1", "id2", "id3"}, {"ip1", "ip2", "ip3"}};
    std::vector<std::vector<std::uint64_t>> features = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    std::vector<std::string> header = {"ID", "IP", "feature1", "feature2", "feature3"};
    CsvFileIO csv;
    csv.write_data_to_file(keys, features, tmp_data_path, false, header);
    auto data = csv.read_data_from_file(tmp_data_path, false, 2);
    ASSERT_EQ(keys, data.first);
    ASSERT_EQ(features, data.second);
}

TEST_F(CsvFileIOTest, read_write_shares) {
    std::vector<std::vector<std::uint64_t>> shares = {{1, 2, 3}, {1, 2, 3}, {1, 2, 3}};
    CsvFileIO csv;
    csv.write_shares_to_file(shares, tmp_data_path);
    auto data = csv.read_shares_from_file(tmp_data_path);
    ASSERT_EQ(shares, data);
}

}  // namespace dpca_psi
