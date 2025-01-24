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
"""Utils in General Count Mean Sketch (GCMS) Framework."""

from typing import List
from hashlib import sha256
import secrets
import numpy as np


def hash_encode(messgae: str, index: int, module: int) -> int:
    """Hash the message with a hash function index, and reduce it with a module.

    Args:
        messgae: A message to be encoded.
        index: A index of the hash function.
        module: A module of the encoded result.

    Returns:
        A hash encoded message.
    """
    sha_input = messgae + "$$$" + str(index)
    return int(sha256(sha_input.encode('utf-8')).hexdigest(), 16) % module


def bernoulli_sample(p: float) -> int:
    """Generate a sample value of 0 or 1 with probability p.

    Args:
        p: The probability of generating 1.

    Returns:
        0 or 1
    """
    return 1 if secrets.randbelow(1000000) < p * 1000000 else 0


def serialize_integers_to_bytes(integers: List[int], fixed_length: int) -> bytearray:
    '''Serialize a list of integers to a bytearray in big endian.

    Args:
        integers: A list of integers.
        fixed_length: The fixed length of each integer.

    Returns:
        A bytearray of the serialized integers.
    '''
    serialized_data = bytearray()
    for num in integers:
        padded_bytes = num.to_bytes(fixed_length, byteorder='big')
        serialized_data.extend(padded_bytes)
    return serialized_data


def deserialize_integers_from_bytes(serialized_data: bytearray, fixed_length: int) -> List[int]:
    '''Deserialize a list of integers from a bytearray in big endian.

    Args:
        serialized_data: A bytearray of the serialized integers.
        fixed_length: The fixed length of each integer.

    Returns:
        A list of integers.
    '''
    integers = []
    num_integers = len(serialized_data) // fixed_length
    for i in range(num_integers):
        start_idx = i * fixed_length
        end_idx = start_idx + fixed_length
        number = int.from_bytes(serialized_data[start_idx:end_idx], byteorder='big')
        integers.append(number)
    return integers


class Paremeters:
    '''
    Attributes:
        k: the number of hash functions.
        m: the module of hash encode function.
        s: the size of the result messages.
        p: the inclusion probability.
    '''

    def __init__(self, k: int, m: int, s: int, p: float) -> None:
        self.k = k
        self.m = m
        self.s = s
        self.p = p


def generate_laplace_noise(loc: float, scale: float) -> float:
    '''Generate a laplace noise with the given location and scale.
    This is only for experimental use, please do not use in production environment.

    Args:
        loc: The location of the laplace noise.
        scale: The scale of the laplace noise.

    Returns:
        A laplace noise.
    '''
    return np.random.laplace(loc=loc, scale=scale)
