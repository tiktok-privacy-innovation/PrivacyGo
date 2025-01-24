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
"""On-device LDP Algorithm in General Count Mean Sketch (GCMS) Framework."""

from gcms_utils import hash_encode
from gcms_utils import bernoulli_sample
from gcms_utils import serialize_integers_to_bytes

from typing import List, Tuple
import pandas as pd
import secrets

from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import hashes


class GCMSClient:
    '''Client is responsible for privatizing and encrypting the raw messages using the on-device LDP algorithm.
    '''

    @staticmethod
    def on_device_ldp_algorithm(k: int, m: int, d: List[str], s: int, p: float,
                                pk: rsa.RSAPublicKey) -> Tuple[List[bytes], List[int], List[List[int]]]:
        """On device LDP Algorithm in GCMS.
        The detailed process for data privatization and encryption.

        Args:
            k: the number of hash functions.
            m: the module of hash encode function.
            d: the raw messages.
            s: the size of the result messages.
            p: the inclusion probability.
            pk: the public key of the server.

        Returns:
            A vector of encrypted privatized messages.
        """

        hash_indexs = []  # for debug only
        plaintext_messages = []  # for debug only
        encrypted_messages = []

        for raw_message in d:
            if not (pd.isna(raw_message)):
                # Randomly select k.
                random_index = secrets.randbelow(k)
                hash_indexs.append(random_index)
                # Calculate the hashed value r.
                hash_result_r = hash_encode(raw_message, random_index, m)

                # Initiate output vector x as an empty set.
                message_x = []

                # Add r to x with probability of p.
                if bernoulli_sample(p):
                    # Randomly select s − 1 elements from [m]/r;
                    message_x.append(hash_result_r)
                    while len(message_x) < s:
                        random_element = secrets.randbelow(m)
                        if random_element != hash_result_r and random_element not in message_x:
                            message_x.append(random_element)
                else:
                    # Randomly select s elements from [m]/r;
                    while len(message_x) < s:
                        random_element = secrets.randbelow(m)
                        if random_element != hash_result_r and random_element not in message_x:
                            message_x.append(random_element)
                plaintext_messages.append(message_x)  # for debug only

                # Encrypt with Server’s public key
                m_bytes_length = (m.bit_length() + 7) // 8
                k_bytes_length = (k.bit_length() + 7) // 8

                plaintext_x = serialize_integers_to_bytes(message_x, m_bytes_length)
                plaintext_x.extend(random_index.to_bytes(k_bytes_length, byteorder='big'))

                ciphertext = pk.encrypt(
                    bytes(plaintext_x),
                    padding.OAEP(mgf=padding.MGF1(algorithm=hashes.SHA256()), algorithm=hashes.SHA256(), label=None))
                encrypted_messages.append(ciphertext)
        return encrypted_messages, hash_indexs, plaintext_messages
