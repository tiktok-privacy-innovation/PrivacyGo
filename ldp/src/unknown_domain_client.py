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
'''On-device algorithm in Privacy-Preserving Data Collection with Unknown Domain.'''

from typing import List
from hashlib import sha256

from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import hashes


class UnknownDomainClient:
    '''Client is responsible for collecting and encrypting the raw messages using the on-device LDP algorithm.
    '''

    @staticmethod
    def on_device_algorithm(d: List[str], pk1: rsa.RSAPublicKey, pk2: rsa.RSAPublicKey) -> List[bytes]:
        """On device LDP Algorithm for unknown data string collection.

        Args:
            d: the raw messages.
            pk1: the public key of the server.
            pk2: the public key of the auxiliary server.

        Returns:
            A vector of encrypted messages.
        """
        encrypted_messages = []
        for message in d:
            hash_message = sha256(message.encode('utf-8')).digest()

            ciphertext_with_pk1 = pk1.encrypt(
                bytes(message, encoding='utf-8'),
                padding.OAEP(mgf=padding.MGF1(algorithm=hashes.SHA256()), algorithm=hashes.SHA256(), label=None))
            ciphertext_with_pk2 = pk2.encrypt(
                ciphertext_with_pk1 + hash_message,
                padding.OAEP(mgf=padding.MGF1(algorithm=hashes.SHA256()), algorithm=hashes.SHA256(), label=None))
            encrypted_messages.append(ciphertext_with_pk2)
        return encrypted_messages
