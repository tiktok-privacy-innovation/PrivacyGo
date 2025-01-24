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
'''Server in Privacy-Preserving Data Collection with Unknown Domain.'''

from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import hashes

from typing import List


class UnknownDomainServer:
    '''Server is responsible for decrypting messages received from the auxiliary server.

    Attributes:
        sk: the secret key of the server.
        hash_len: the length of the hash value.
    '''

    def __init__(self, sk: rsa.RSAPrivateKey) -> None:
        self.sk = sk

    def decrypt_message(self, encrypted_messages: List[bytes]) -> List[str]:
        '''Decrypt the encrypted messages received from the auxiliary server.

        Args:
            encrypted_messages: A list of encrypted messages encrypted by server.

        Returns:
            A list of plaintext messages.
        '''
        result = []
        for message in encrypted_messages:
            decrypted_message = self.sk.decrypt(
                message, padding.OAEP(mgf=padding.MGF1(algorithm=hashes.SHA256()),
                                      algorithm=hashes.SHA256(),
                                      label=None))
            result.append(str(decrypted_message, encoding='utf-8'))
        return result
