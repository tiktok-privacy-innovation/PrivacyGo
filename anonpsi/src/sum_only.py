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
""" This file includes MIA attack that based on SUM information only."""

import heapq

import utils


def improved_attack_with_sum_only(victim_x, target_y, tau):
    """Improved MIA attack for PSI-CA that considers sum information only.

    Args:
        victim_x: A set of victim elements X.
        target_y: A set of target elements Y.
        tau: A int indicates the number of protocol call limitation.

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
    _, current_sum = utils.psi_sum(victim_x, target_y)
    total_sum = sum(pair[1] for pair in list(victim_x))
    protocol_call_num = 1  # Tracking the protocol calling times
    node = utils.TreeNode()
    tree = node.form_tree(victim_x)  # Generate Tree based on X
    heapq.heappush(forest, (min(-1 * current_sum / total_sum, -1 + current_sum / total_sum), current_sum, idx, tree))
    idx += 1

    while forest and protocol_call_num <= tau:
        _, current_node_sum, _, node = heapq.heappop(forest)
        total_sum = sum(pair[1] for pair in list(node.val))
        while 0 < current_node_sum < total_sum and protocol_call_num <= tau:
            left_node = node.left
            if left_node:
                left_node_total = sum(pair[1] for pair in list(left_node.val))
            else:
                left_node_total = 0

            right_node = node.right
            if right_node:
                right_node_total = sum(pair[1] for pair in list(right_node.val))
            else:
                right_node_total = 0

            if not right_node or not left_node or left_node_total == 0 or right_node_total == 0:
                break

            # Call protocol once and get density for both children node
            if right_node_total > left_node_total:
                _, left_node_sum = utils.psi_sum(left_node.val, target_y)
                protocol_call_num += 1
                right_node_sum = current_node_sum - left_node_sum
            else:
                _, right_node_sum = utils.psi_sum(right_node.val, target_y)
                protocol_call_num += 1
                left_node_sum = current_node_sum - right_node_sum

            # Check the density as the priority for moving to a child node and
            # push the other child node into the forest
            right_priority = max(right_node_sum / right_node_total, 1 - right_node_sum / right_node_total)
            left_priority = max(left_node_sum / left_node_total, 1 - left_node_sum / left_node_total)
            if right_priority > left_priority:
                if left_node_sum != 0:
                    pushed_priority = min(-1 * left_node_sum / left_node_total, -1 + left_node_sum / left_node_total)
                    heapq.heappush(forest, (pushed_priority, left_node_sum, idx, left_node))
                    idx += 1
                else:
                    z_neg.update(left_node.val)

                node = right_node
                current_node_sum = right_node_sum
            else:
                if right_node_sum != 0:
                    pushed_priority = min(-1 * right_node_sum / right_node_total,
                                          -1 + right_node_sum / right_node_total)
                    heapq.heappush(forest, (pushed_priority, right_node_sum, idx, right_node))
                    idx += 1
                else:
                    z_neg.update(right_node.val)

                node = left_node
                current_node_sum = left_node_sum

        if current_node_sum > 0 and protocol_call_num <= tau:
            z_pos.update(node.val)
        elif current_node_sum == 0 and protocol_call_num <= tau:
            z_neg.update(node.val)
    return [z_pos, z_neg]
