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
"""Shuffler in General Count Mean Sketch (GCMS) Framework."""

from typing import Any, List
import secrets


class GCMSShuffler:
    """Shuffler is responsible for shuffling and anonymizing the input list from clients."""

    @staticmethod
    def shuffle(data: List[Any]) -> List[Any]:
        """Shuffle the input list.

        Args:
            input_list: The input list to be shuffled.

        Returns:
            The shuffled list.
        """
        data_copy = data[:]

        for i in range(len(data_copy) - 1, 0, -1):
            j = secrets.randbelow(i + 1)
            data_copy[i], data_copy[j] = data_copy[j], data_copy[i]

        return data_copy
