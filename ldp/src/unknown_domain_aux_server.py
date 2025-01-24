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
'''Auxiliary Server in Privacy-Preserving Data Collection with Unknown Domain.'''

from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import hashes

from gcms_utils import generate_laplace_noise

import secrets

from typing import List, Tuple


class UnknownDomainAuxServer:
    '''Server is responsible for providing DP protection for releasing the item names.

    Attributes:
        sk: the secret key of the auxiliary server.
        hash_len: the length of the hash value.
    '''

    def __init__(self, sk: rsa.RSAPrivateKey, hash_len: int = 32) -> None:
        self.sk = sk
        self.hash_len = hash_len

    def dp_protection(self, encrypted_messages: List[bytes], t: int, b: int) -> List[bytes]:
        '''DP protection for releasing the item names.

        Args:
            encrypted_messages: A list of shuffled encrypted messages encrypted by auxiliary server.

        Returns:
            A list of encrypted messages encrypted by server.
        '''
        encrypted_messages_with_server_pk = []
        key_value_dict = {}
        for message in encrypted_messages:
            decrypted_message = self.sk.decrypt(
                message, padding.OAEP(mgf=padding.MGF1(algorithm=hashes.SHA256()),
                                      algorithm=hashes.SHA256(),
                                      label=None))
            key = (decrypted_message[-self.hash_len:]).hex()
            if key in key_value_dict:
                key_value_dict[key].append(decrypted_message[:-self.hash_len])
            else:
                key_value_dict[key] = [decrypted_message[:-self.hash_len]]

        for key, value in key_value_dict.items():
            if len(value) + generate_laplace_noise(0, b) >= t:
                idx = secrets.randbelow(len(value))
                encrypted_messages_with_server_pk.append(value[idx])

        return encrypted_messages_with_server_pk
