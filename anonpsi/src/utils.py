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
"""This file defines basic functions used throughout the projects."""

import random


def psi_cardinality(victim, target):
    """Performs private set intersection cardinality (PSI-CA) protocol.

    Args:
        victim: A set of victim elements.
        target: A set of target elements.

    Returns:
        Cardinality of the intersection.
    """
    intersected_set = victim.intersection(target)
    return len(intersected_set)


def psi(victim, target):
    """Performs private set intersection (PSI) protocol.

    Args:
        victim: A set of victim elements.
        target: A set of target elements.

    Returns:
        Full intersection set.
    """
    intersected_set = victim.intersection(target)
    return intersected_set


def psi_sum(victim, target):
    """Performs private set intersection - summation (PSI-SUM) protocol.

    Args:
        victim: A set of victim elements with its value.
        target: A set of target elements with its value.

    Returns:
        Cardinality and summation of the intersection.
    """
    intersected_set = victim.intersection(target)
    intersected_list = list(intersected_set)
    summation = sum(pair[1] for pair in intersected_list)
    return [len(intersected_set), summation]


def psi_sum_int(victim, target):
    """Performs private set intersection - summation protocol (PSI-SUM) for int vector input.

    Args:
        victim: A set of victim elements.
        target: A set of target elements.

    Returns:
        Cardinality and summation of the intersection.
    """
    intersected_set = victim.intersection(target)
    intersected_list = list(intersected_set)
    summation = sum(intersected_list)
    return [len(intersected_set), summation]


class TreeNode:
    """Tree node which is used to structure the target set"""

    def __init__(self, val=0, left=None, right=None):
        self.val = val
        self.left = left
        self.right = right

    def form_tree(self, input_set):
        """Forms the tree given the input set.

        Args:
            input_set: A set of target elements.

        Returns:
            Root of the target set tree.
        """
        cur_node = TreeNode(input_set)
        if len(input_set) == 1:
            return cur_node
        cur_node_list = list(cur_node.val)
        random.shuffle(cur_node_list)
        left_node_list = cur_node_list[:len(cur_node_list) // 2]
        right_node_list = cur_node_list[len(cur_node_list) // 2:]
        cur_node.left = self.form_tree(set(left_node_list))
        cur_node.right = self.form_tree(set(right_node_list))
        return cur_node

    def print_tree(self, root):
        """prints the formed tree."""
        if not root:
            return
        print(root.val)
        self.print_tree(root.left)
        self.print_tree(root.right)


def gen_simple_set(victim_set_num, target_set_num, upper_bound):
    """Generates testing set.
    Args:
        victim_set_num: A int indicates the number of victim set elements.
        target_set_num: A int indicates the number of target set elements.
        upper_bound: sample upper bound.

    Returns:
        Testing set of victim X and target Y.
    """
    sampling_range = range(0, upper_bound)
    victim_x = set(random.sample(sampling_range, victim_set_num))
    target_y = set(random.sample(sampling_range, target_set_num))
    return [victim_x, target_y]


def gen_set_with_intersection(victim_set_num, target_set_num, dense, intersection_num):
    """Generates set with given intersection.

    Args:
        victim_set_num: A int indicates the number of victim set elements.
        target_set_num: A int indicates the number of target set elements.
        dense: sample density (>1).
        intersection_num: A int indicates the number of intersection.

    Returns:
        Testing set of victim X and target Y.
    """
    sampling_range = range(0, max(victim_set_num, target_set_num) * dense)
    sampled_data = random.sample(sampling_range, victim_set_num + target_set_num - intersection_num)
    intersection = sampled_data[:intersection_num]
    victim_x = sampled_data[intersection_num:victim_set_num]
    target_y = sampled_data[victim_set_num:]
    victim_x += intersection
    target_y += intersection
    random.shuffle(victim_x)
    random.shuffle(target_y)
    return [set(victim_x), set(target_y)]


def gen_dummy_set(victim_set_num, target_set_num, dummy_set_num, dense, intersection_num):
    """Generates testing set with given dummy set.

    Args:
        victim_set_num: A int indicates the number of victim set elements.
        target_set_num: A int indicates the number of target set elements.
        dummy_set_num: A int indicates the number of dummy set elements.
        intersection_num: A int indicates the number of intersection.
        dense: sample density (>1).

    Returns:
        Testing set of target X , victim Y, and dummy set.
    """
    sampling_range = range(0, (victim_set_num + target_set_num + dummy_set_num) * dense)
    sampled_data = random.sample(sampling_range, victim_set_num + target_set_num + dummy_set_num - intersection_num)
    intersection = sampled_data[:intersection_num]
    victim_x = sampled_data[intersection_num:victim_set_num]
    target_y = sampled_data[victim_set_num:victim_set_num + target_set_num - intersection_num]
    dummy_set = sampled_data[victim_set_num + target_set_num - intersection_num:]
    victim_x += intersection
    target_y += intersection
    random.shuffle(victim_x)
    random.shuffle(target_y)
    random.shuffle(dummy_set)
    return [set(victim_x), set(target_y), set(dummy_set)]
