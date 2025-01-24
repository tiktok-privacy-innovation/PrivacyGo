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
'''Server in General Count Mean Sketch (GCMS) Framework.'''

from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import hashes

from gcms_utils import deserialize_integers_from_bytes
from gcms_utils import hash_encode

import numpy as np

from typing import List, Tuple


class GCMSServer:
    '''Server is responsible for aggregation and frequency estimation.

    Attributes:
        k: the number of hash functions.
        m: the module of hash encode function.
        sk: the private key of the server.
        matrix: the sketch matrix.
        m_bytes_length: the length of the m in bytes.
        k_bytes_length: the length of the k in bytes.
        n: the number of messages.
    '''

    def __init__(self, k: int, m: int, sk: rsa.RSAPrivateKey) -> None:
        self.k = k
        self.m = m
        self.sk = sk
        self.matrix = np.zeros((k, m))
        self.m_bytes_length = (m.bit_length() + 7) // 8
        self.k_bytes_length = (k.bit_length() + 7) // 8
        self.n = 0

    def decrypt_message(self, encrypted_messages: List[bytes]) -> Tuple[List[int], int]:
        '''Decrypt encrypted messages with the server's secret key.

        Args:
            encrypted_messages: A list of encrypted messages.

        Returns:
            A tuple of the plaintext messages and the hash indexs.
        '''
        hash_indexs = []
        plaintext_messages = []
        for message in encrypted_messages:
            decrypted_message = self.sk.decrypt(
                message, padding.OAEP(mgf=padding.MGF1(algorithm=hashes.SHA256()),
                                      algorithm=hashes.SHA256(),
                                      label=None))
            hash_index = int.from_bytes(decrypted_message[-self.k_bytes_length:], byteorder='big')
            plaintext_message = deserialize_integers_from_bytes(decrypted_message[:-self.k_bytes_length],
                                                                self.m_bytes_length)
            hash_indexs.append(hash_index)
            plaintext_messages.append(plaintext_message)

        return plaintext_messages, hash_indexs

    def construct_sketch_matrix(self, plaintext_messages: List[List[int]], hash_indexs: List[int]) -> None:
        '''Construct the sketch matrix.

        Args:
            plaintext_messages: A list of plaintext messages.
            hash_indexs: A list of hash indexs.
        '''
        self.n += len(plaintext_messages)
        for i in range(len(plaintext_messages)):
            for j in plaintext_messages[i]:
                self.matrix[hash_indexs[i]][j] += 1

    def estimate_frequency(self, message: str, p: float, s: int) -> float:
        '''Estimate the frequency of the given message.

        Args:
            message: The message to be estimated.
            p: The inclusion probability.
            s: The size of the result messages.

        Returns:
            The estimated frequency of the given message.
        '''
        hash_k = []
        total_count = 0
        for i in range(self.k):
            hash_result_i = hash_encode(message, i, self.m)
            total_count += self.matrix[i][hash_result_i]
            hash_k.append(hash_result_i)
        q = (p * (s - 1) + (1 - p) * s) / (self.m - 1)
        estimated_frequency = (total_count - (p * self.n / self.m) - (q * self.n *
                                                                      (1 - 1 / self.m))) / ((p - q) * (1 - 1 / self.m))
        return estimated_frequency
