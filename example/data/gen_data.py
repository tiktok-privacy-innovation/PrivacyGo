# Copyright 2023 TikTok Pte. Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Utils for simple data generation."""

import csv
import math
import time

def gen_simple_data(intersection_size, sender_data_ratio, receiver_data_ratio, sender_feature_size,
                    receiver_feature_size, key_size, sender_output_path, receiver_output_path):
    """generate simple keys and features."""
    sender_data_size = sender_data_ratio * intersection_size
    receiver_data_size = receiver_data_ratio * intersection_size
    sender_data = []
    receiver_data = []

    column_intersection_size = int((intersection_size + key_size - 1) / key_size)
    for i in range(sender_data_size):
        sender_line = []
        if i < intersection_size:
            intersection_idx = int(math.floor(i / column_intersection_size))
            for j in range(0, intersection_idx):
                sender_line.append("sender_id_" + str(j) + "_" + str(i))
            sender_line.append("id_" + str(i))
            for j in range(intersection_idx + 1, key_size):
                sender_line.append("sender_id_" + str(j) + "_" + str(i))
        else:
            for j in range(key_size):
                sender_line.append("sender_id_" + str(j) + "_" + str(i))
        for j in range(sender_feature_size):
            sender_line.append(int(round(time.time() * 1000)))
        sender_data.append(sender_line)

    for i in range(receiver_data_size):
        receiver_line = []
        if i < intersection_size:
            intersection_idx = int(math.floor(i / column_intersection_size))
            for j in range(0, intersection_idx):
                receiver_line.append("receiver_id_" + str(j) + "_" + str(i))
            receiver_line.append("id_" + str(i))
            for j in range(intersection_idx + 1, key_size):
                receiver_line.append("receiver_id_" + str(j) + "_" + str(i))
        else:
            for j in range(key_size):
                receiver_line.append("receiver_id_" + str(j) + "_" + str(i))
        for j in range(receiver_feature_size):
                receiver_line.append(int(round(time.time() * 1000)))
        receiver_data.append(receiver_line)

        with open(sender_output_path, "w", encoding='utf-8', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerows(sender_data)
        with open(receiver_output_path, "w", encoding='utf-8', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerows(receiver_data)

if __name__ == '__main__':
    gen_simple_data(5, 100, 10, 1, 1, 3, "sender_input_file.csv", "receiver_input_file.csv")
