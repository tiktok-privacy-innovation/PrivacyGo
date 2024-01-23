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
"""This file includes the dypathblizer algorithm based on dynamic programming."""

import heapq
import random

import utils


def dynpathblizer(victim_x, target_y, tau, gamma, phi):
    """Dynamic programming for MIA.

    Args:
        victim_x: A set of victim elements X.
        target_y: A set of target elements Y.
        tau: A int indicates the number of protocol call limitation.
        gamma: A hash map represents the optimal partition K for current state(N, C_N, tau).
        phi: A hash map represents average call required for current state(N, C_N).

    Returns:
        Predicted set Z_pos and Z_neg.
    """
    # Initialize prediction set
    z_pos = set()
    z_neg = set()

    # Initialize the priority Queue
    forest = []
    idx = 0
    heapq.heapify(forest)
    initial_node_cardinality = utils.psi_cardinality(victim_x, target_y)
    protocol_call_num = 1  # Tracking the protocol calling times
    victim_x = list(victim_x)
    random.shuffle(victim_x)
    victim_x = set(victim_x)
    heapq.heappush(forest,
                   (min(-1 * initial_node_cardinality / len(victim_x),
                        -1 + initial_node_cardinality / len(victim_x)), initial_node_cardinality, idx, victim_x))
    idx += 1

    while forest and protocol_call_num <= tau:
        _, current_node_cardinality, _, node = heapq.heappop(forest)
        while 0 < current_node_cardinality < len(node) and protocol_call_num < tau:
            max_call_num = phi["(" + str(len(node)) + "," + str(current_node_cardinality) + ")"]
            if max_call_num < tau - protocol_call_num:
                partition_factor_k = len(node) // 2
            else:
                partition_factor_k = gamma["S(" + str(len(node)) + "," + str(current_node_cardinality) + "," +
                                           str(tau - protocol_call_num) + ")"][1]

            cur_node = list(node)
            if partition_factor_k == 0 or partition_factor_k == len(node):
                break
            random.shuffle(cur_node)
            left_node = set(cur_node[:partition_factor_k])
            right_node = set(cur_node[partition_factor_k:])

            # Call protocol once and get density for both children node
            if len(right_node) > len(left_node):
                left_cardinality = utils.psi_cardinality(left_node, target_y)
                protocol_call_num += 1
                right_cardinality = current_node_cardinality - left_cardinality
            else:
                right_cardinality = utils.psi_cardinality(right_node, target_y)
                protocol_call_num += 1
                left_cardinality = current_node_cardinality - right_cardinality

            # Check the density as the priority for moving to a child node and
            # push the other child node into the forest
            right_priority = max(right_cardinality / len(right_node), 1 - right_cardinality / len(right_node))
            left_priority = max(left_cardinality / len(left_node), 1 - left_cardinality / len(left_node))
            if right_priority > left_priority:
                if left_cardinality != 0:
                    pushed_priority = min(-1 * left_cardinality / len(left_node),
                                          -1 + left_cardinality / len(left_node))
                    heapq.heappush(forest, (pushed_priority, left_cardinality, idx, left_node))
                    idx += 1
                else:
                    z_neg.update(left_node)

                node = right_node
                current_node_cardinality = right_cardinality
            else:
                if right_cardinality != 0:
                    pushed_priority = min(-1 * right_cardinality / len(right_node),
                                          -1 + right_cardinality / len(right_node))
                    heapq.heappush(forest, (pushed_priority, right_cardinality, idx, right_node))
                    idx += 1
                else:
                    z_neg.update(right_node)

                node = left_node
                current_node_cardinality = left_cardinality

        if current_node_cardinality == len(node) and protocol_call_num <= tau:
            z_pos.update(node)
        elif current_node_cardinality == 0 and protocol_call_num <= tau:
            z_neg.update(node)
    return [z_pos, z_neg]
