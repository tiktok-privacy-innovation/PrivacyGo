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
"""This file includes the memo generting functions in the dypathblizer algorithm."""

import math

from tqdm import tqdm


def gen_gamma_and_phi(size):
    """Generates gamma, phi, which are the memo space for the dynamic
    programming algorithm and the minimized protocol call.

    Args:
        size: A int indicates the number of victim set elements.

    Returns:
        Memorized space gamma and phi.
    """
    gamma = {}
    gamma["S(1,0,0)"] = [1, 0]
    gamma["S(1,0,1)"] = [1, 0]
    gamma["S(1,1,0)"] = [1, 0]
    gamma["S(1,1,1)"] = [1, 0]
    phi = {}
    phi["(1,0)"] = 0
    phi["(1,1)"] = 0
    for n in tqdm(range(2, size + 1)):
        for c_n in range((n + 1) // 2 + 1):
            for tau in range(n + 1):
                cur_state = "S(" + str(n) + "," + \
                    str(c_n) + "," + str(tau) + ")"
                max_call = "(" + str(n) + "," + str(c_n) + ")"
                cur_state_reverse = "S(" + str(n) + "," + \
                    str(n - c_n) + "," + str(tau) + ")"
                max_call_reverse = "(" + str(n) + "," + str(n - c_n) + ")"
                if c_n == 0 or c_n == n:
                    phi[max_call] = 0
                    phi[max_call_reverse] = 0
                    gamma[cur_state] = (n, 0)
                    gamma[cur_state_reverse] = (n, 0)
                    break
                elif tau >= n:
                    gamma[cur_state] = (n, n // 2)
                    gamma[cur_state_reverse] = (n, n // 2)
                elif tau == 0:
                    gamma[cur_state] = (0, 0)
                    gamma[cur_state_reverse] = (0, 0)
                else:
                    expected_leakage = 0
                    max_k = 0
                    for k in range(1, n // 2 + 1):
                        left_leakage = 0
                        right_leakage = 0
                        for c in range(max(0, c_n + k - n), min(k, c_n) + 1):
                            prob = math.comb(c_n, c) * math.comb(n - c_n, k - c) / math.comb(n, k)
                            left_call_need = phi["(" + str(k) + "," + str(c) + ")"]
                            right_call_need = phi["(" + str(n - k) + "," + str(c_n - c) + ")"]

                            if tau - 1 < left_call_need:
                                left_leakage += prob * \
                                    gamma["S(" + str(k) + "," + str(c) +
                                          "," + str(tau - 1) + ")"][0]
                                if c_n == c or c_n - c == n - k:
                                    left_leakage += prob * (n - k)
                            else:
                                left_leakage += prob * k
                                if tau - 1 - left_call_need < right_call_need:
                                    left_leakage += prob * \
                                        gamma["S(" + str(n - k) + "," + str(c_n - c) +
                                              "," + str(tau - 1 - left_call_need) + ")"][0]
                                else:
                                    left_leakage += prob * (n - k)

                            if tau - 1 < right_call_need:
                                right_leakage += prob * \
                                    gamma["S(" + str(n-k) + "," +
                                          str(c_n-c) + "," + str(tau-1) + ")"][0]
                                if c == 0 or c == k:
                                    right_leakage += prob * k
                            else:
                                right_leakage += prob * (n - k)
                                if tau - 1 - right_call_need < left_call_need:
                                    right_leakage += prob * \
                                        gamma["S(" + str(k) + "," + str(c) +
                                              "," + str(tau - 1 - right_call_need) + ")"][0]
                                else:
                                    right_leakage += prob * k

                        if left_leakage >= expected_leakage:
                            expected_leakage = left_leakage
                            max_k = max(max_k, k)
                        if right_leakage >= expected_leakage:
                            expected_leakage = right_leakage
                            max_k = max(max_k, k)

                    gamma[cur_state] = (expected_leakage, max_k)
                    gamma[cur_state_reverse] = (expected_leakage, max_k)

                    if expected_leakage >= n:
                        if "(" + str(n) + "," + str(c_n) + ")" in phi:
                            phi["(" + str(n) + "," + str(c_n) + ")"] = min(phi["(" + str(n) + "," + str(c_n) + ")"],
                                                                           tau)
                            phi["(" + str(n) + "," + str(n - c_n) + ")"] = min(
                                phi["(" + str(n) + "," + str(n - c_n) + ")"], tau)
                        else:
                            phi["(" + str(n) + "," + str(c_n) + ")"] = tau
                            phi["(" + str(n) + "," + str(n - c_n) + ")"] = tau
                        break
    return [gamma, phi]
