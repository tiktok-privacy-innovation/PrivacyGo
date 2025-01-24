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
'''A demo illustration of Privacy-Preserving Data Collection with Unknown Domain.'''

from cryptography.hazmat.primitives.asymmetric import rsa

from typing import List
from gcms_shuffler import GCMSShuffler
from unknown_domain_client import UnknownDomainClient
from unknown_domain_aux_server import UnknownDomainAuxServer
from unknown_domain_server import UnknownDomainServer

import math


def unknown_domain_demo(data: List[str], bench_nums: int, delta: float, epsilon: float):
    '''
    A demo illustration of Privacy-Preserving Data Collection with Unknown Domain.'

    Args:
        data: The raw data to be privatized and encrypted.
        bench_nums: The number of bench times.
        estimate_message: The message to be estimated.
    '''
    #0. Prepare the parameters for the algorithm.
    T = 1 + 1 / epsilon * math.log(1 / (2 * delta))
    scale = 1 / epsilon

    server_private_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    server_public_key = server_private_key.public_key()

    aux_server_private_key = rsa.generate_private_key(public_exponent=65537, key_size=3072)
    aux_server_public_key = aux_server_private_key.public_key()

    for i in range(bench_nums):
        #1. clients perform on-device ldp operations on the raw data.
        encrypted_messages = UnknownDomainClient.on_device_algorithm(data, server_public_key, aux_server_public_key)

        #2. Shuffler shuffle the encrypted messages.
        shuffled_message = GCMSShuffler.shuffle(encrypted_messages)

        #3.0 Init Aux Server
        aux_server = UnknownDomainAuxServer(aux_server_private_key)

        #3.1 Aux Server perform DP protection.
        encrypted_messages_with_server_pk = aux_server.dp_protection(shuffled_message, T, scale)

        #4.0 Init Server
        server = UnknownDomainServer(server_private_key)

        #4.1 Server decrypts messages.
        result = server.decrypt_message(encrypted_messages_with_server_pk)
        print("result", result)


if __name__ == "__main__":
    data = ['123' for i in range(100)]
    data.extend(['456' for i in range(50)])
    data.extend(['789' for i in range(25)])
    deltas = [1 / (10 * len(data)), 1 / (100 * len(data))]
    epsilon_list = [x / 10 for x in range(1, 100)]
    unknown_domain_demo(data, bench_nums=1, delta=deltas[0], epsilon=epsilon_list[0])
