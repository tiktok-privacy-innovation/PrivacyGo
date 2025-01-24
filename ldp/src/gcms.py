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
'''A demo illustration of General Count Mean Sketch (GCMS) Framework.'''

from cryptography.hazmat.primitives.asymmetric import rsa

from typing import List
from gcms_utils import Paremeters
from gcms_client import GCMSClient
from gcms_shuffler import GCMSShuffler
from gcms_server import GCMSServer

import math


def gcms_demo(data: List[str], bench_nums: int, estimate_message: str):
    '''
    A demo illustration of General Count Mean Sketch (GCMS) Framework.

    Args:
        data: The raw data to be privatized and encrypted.
        bench_nums: The number of bench times.
        estimate_message: The message to be estimated.
    '''
    #0. Prepare the parameters for the algorithm.
    ocms_parms = Paremeters(k=1000, m=1024, s=56, p=0.5)
    epsilon = math.log((ocms_parms.m - ocms_parms.s) * ocms_parms.p / ((1 - ocms_parms.p) * ocms_parms.s))
    print("epsilin is ", epsilon)

    server_private_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    server_public_key = server_private_key.public_key()

    estimate_frequencies = []
    estimate_frequencies_debug = []
    for i in range(bench_nums):
        #1. clients perform on-device ldp operations on the raw data.
        encrypted_messages, hash_indexs_debug, plaintext_messages_debug = GCMSClient().on_device_ldp_algorithm(
            ocms_parms.k, ocms_parms.m, data, ocms_parms.s, ocms_parms.p, server_public_key)

        #2. Shuffler shuffle the encrypted messages.
        shuffled_message = GCMSShuffler.shuffle(encrypted_messages)

        #3.0 Init Server
        server = GCMSServer(ocms_parms.k, ocms_parms.m, server_private_key)

        #3.1 Server decrypt the shuffled messages.
        plaintext_messages, hash_indexs = server.decrypt_message(shuffled_message)

        #3.2 Server construct the sketch matrix.
        server.construct_sketch_matrix(plaintext_messages, hash_indexs)
        # for debug
        # print(sorted(hash_indexs_debug) == sorted(hash_indexs))
        # print(sorted(plaintext_messages_debug) == sorted(plaintext_messages_debug))

        #3.3. Server estimate the frequency of the specific message.
        estimate_frequency_i = server.estimate_frequency(estimate_message, ocms_parms.p, ocms_parms.s)
        estimate_frequencies.append(estimate_frequency_i)

        server_debug = GCMSServer(ocms_parms.k, ocms_parms.m, server_private_key)
        server_debug.construct_sketch_matrix(plaintext_messages_debug, hash_indexs_debug)
        estimate_frequency_i_debug = server_debug.estimate_frequency(estimate_message, ocms_parms.p, ocms_parms.s)
        estimate_frequencies_debug.append(estimate_frequency_i_debug)

    print(f"average estimated frequency of {estimate_message}:", sum(estimate_frequencies) / bench_nums)
    print(f"average debug estimated frequency of {estimate_message}:", sum(estimate_frequencies_debug) / bench_nums)


if __name__ == "__main__":
    data = ['123' for i in range(100)]
    data.extend(['456' for i in range(50)])
    data.extend(['789' for i in range(25)])
    gcms_demo(data, 10, '123')
