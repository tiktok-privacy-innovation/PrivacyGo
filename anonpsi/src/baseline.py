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
"""This file includes baseline algorithm from USENIX22."""

import heapq
import random

import utils


def baseline_attack_with_limited_call(victim_x, target_y, tau):
    """Baseline MIA(Membership inference attack) for MPC.

    Args:
        victim_x: A set of victim elements X.
        target_y: A set of target elements Y.
        tau: A int indicates the number of protocol call limitation.

    Returns:
        Predicted set Z_pos and Z_neg.
    """
    # Initialize prediction set.
    z_pos = set()
    z_neg = set()

    # Initialize the priority Queue
    forest = []
    idx = 0  # Indexing for debug.
    heapq.heapify(forest)
    initial_node_cardinality = utils.psi_cardinality(victim_x, target_y)
    protocol_call_num = 1  # Tracking the protocol calling times
    victim_x = list(victim_x)
    random.shuffle(victim_x)
    victim_x = set(victim_x)
    node = utils.TreeNode()
    tree = node.form_tree(victim_x)  # Generate tree based on X
    heapq.heappush(forest, (-1 * initial_node_cardinality / len(victim_x), initial_node_cardinality, idx, tree))
    idx += 1

    while forest and protocol_call_num <= tau:
        _, current_node_cardinality, _, node = heapq.heappop(forest)
        # Check the number of elements for calculating priority
        while 0 < current_node_cardinality < len(node.val) and protocol_call_num <= tau:
            left_node = node.left
            right_node = node.right

            # Call protocol once and get density for both children node
            if len(right_node.val) > len(left_node.val):
                left_cardinality = utils.psi_cardinality(left_node.val, target_y)
                protocol_call_num += 1
                right_cardinality = current_node_cardinality - left_cardinality
            else:
                right_cardinality = utils.psi_cardinality(right_node.val, target_y)
                protocol_call_num += 1
                left_cardinality = current_node_cardinality - right_cardinality

            # Check the density as the priority for moving to a child node and
            # push the other child node into the forest
            right_priority = right_cardinality / len(right_node.val)
            left_priority = left_cardinality / len(left_node.val)
            if right_priority > left_priority:
                if left_cardinality != 0:
                    pushed_priority = -1 * left_cardinality / len(left_node.val)
                    heapq.heappush(forest, (pushed_priority, left_cardinality, idx, left_node))
                    idx += 1
                else:
                    z_neg.update(left_node.val)

                node = right_node
                current_node_cardinality = right_cardinality
            else:
                if right_cardinality != 0:
                    pushed_priority = -1 * right_cardinality / len(right_node.val)
                    heapq.heappush(forest, (pushed_priority, right_cardinality, idx, right_node))
                    idx += 1
                else:
                    z_neg.update(right_node.val)

                node = left_node
                current_node_cardinality = left_cardinality

        if current_node_cardinality > 0 and protocol_call_num <= tau:
            z_pos.update(node.val)
    return [z_pos, z_neg]
