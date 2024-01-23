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
"""This file includes treesumexplorer algorithm for SMIA.

Treesumexplorer combines an offline N sum problem and the baseline attack.
"""

import heapq
import math
import random

import utils


def n_sum(nums, target, n):
    """Calculates all unique combinations of n pairs of values in nums
    that the second values add up to the target value.

    Args:
        nums: A list of input sequence.
        target: A int indicates the target value.
        n: A int indicates the length of the subsequence that add up to target.

    Returns:
        A list of result, each mathes the condition.
    """
    res = []
    nums = sorted(nums)  # sort the input list for int vector
    # backtrack(nums, target, n, [], res)
    backtrack_int(nums, target, n, [], res)
    return res


def backtrack(nums, target, n, paths, res):
    """A backtracking solution to traversal all possible combinations.

    Args:
        nums: A list of input sequence.
        target: A int indicates the target value.
        n: A int indicates the length of the subsequence that add up to target.
        paths: A list saves the current paths of traversal.
        res: A list saves the previous result.
    """
    if target < 0 or len(paths) > n:
        return
    if target == 0 and len(paths) == n:
        res.append(paths)
        return
    for i in range(len(nums)):
        if i > 0 and nums[i][1] == nums[i - 1][1]:
            continue
        backtrack(nums[i + 1:], target - nums[i][1], n, paths + [nums[i]], res)


def backtrack_int(nums, target, n, paths, res):
    """A backtracking solution to traversal all possible combinations for int vector input.

    Args:
        nums: A list of input sequence.
        target: A int indicates the target value.
        n: A int indicates the length of the subsequence that add up to target.
        paths: A list saves the current paths of traversal.
        res: A list saves the previous result.
    """
    if target < 0 or len(paths) > n:
        return
    if target == 0 and len(paths) == n:
        res.append(paths)
        return
    for i in range(len(nums)):
        if i > 0 and nums[i] == nums[i - 1]:
            continue
        backtrack_int(nums[i + 1:], target - nums[i], n, paths + [nums[i]], res)


def computation_complexity(length, dense):
    """Calculates the computation complexity given the current membership density and length of a seqeunce

    Args:
        length: A int indicates the input length.
        dense: A int indicates the membership density.

    Returns:
        A float value indicates the computation complexity.
    """
    return math.log10(length**(length // dense - 1))


def solve_equation(lower_bound, upper_bound, tau, computation_budget, dense, tol=1e-5):
    """Calculates the length of input given current computation cost.

    Args:
        lower_bound: A int indicates the lower bound of the output range.
        upper_bound: A int indicates the upper bound of the output range.
        tau: A int indicates the protocol invocation budget.
        computation_budget: A int indicates the computation budget.
        dense: A int indicates the membership density.
        tol: A double indicates the error tolerance.

    Returns:
        A int indicates the Length of the input.
    """
    val = math.log10(computation_budget / tau)
    out = 0
    while lower_bound < upper_bound:
        middle = (upper_bound + lower_bound) // 2
        cur = computation_complexity(middle, dense)
        if abs(cur - val) < tol:
            return middle
        elif cur > val:
            upper_bound = middle - 1
        else:
            out = middle
            lower_bound = middle + 1
    return out


def psi_sum_attack(victim_x, target_y, tau, computation_budget):
    """TreeSumExporer Attack algorithm.
    Args:
        victim_x: A set of victim elements X.
        target_y: A set of target elements Y.
        tau: A int indicates the protocol invocation budget.
        computation_budget: A int indicates the computation budget.

    Returns:
        Predicted postive(z_pos) and negative(z_neg) membership sets.
    """
    z_pos = set()
    z_neg = set()
    while tau > 0:
        victim_x = victim_x.difference(z_pos)
        victim_x = victim_x.difference(z_neg)
        if len(victim_x) > 0:
            z_pos, z_neg, tau = psi_sum_helper(victim_x, target_y, tau, computation_budget, z_pos, z_neg)
        else:
            break
    return [z_pos, z_neg]


def psi_sum_helper(victim_x, target_y, tau, computation_budget, z_pos, z_neg):
    """ TreeSumExporer Attack algorithm helper function.

    Args:
        victim_x: A set of victim elements X.
        target_y: A set of target elements Y.
        tau: A int indicates the protocol invocation budget.
        computation_budget: A int indicates the computation budget.
        z_pos: previous predicted postive membership sets.
        z_neg: previous predicted negative membership sets.

    Returns:
        Predicted postive(z_pos) and negative(z_neg) membership sets and updated tau.
    """
    computation_budget = int(computation_budget)
    forest = []
    idx = 0
    heapq.heapify(forest)
    [current_cardinality, current_sum] = utils.psi_sum_int(victim_x, target_y)
    tau -= 1

    if tau == 0:
        return (z_pos, z_neg, tau)

    candidate_size = len(victim_x)
    if len(victim_x)**(current_cardinality - 1) >= computation_budget / (tau):
        candidate_size = solve_equation(0, 500, tau, computation_budget, len(victim_x) // current_cardinality)
        victim_x = list(victim_x)
        random.shuffle(victim_x)
        victim_x = set(victim_x[:candidate_size])
        [current_cardinality, current_sum] = utils.psi_sum_int(victim_x, target_y)
        tau -= 1

    candidate_list = n_sum(victim_x, current_sum, current_cardinality)
    priority = len(victim_x) if len(candidate_list) <= tau else len(victim_x) * (1 -
                                                                                 (1 - 1 / len(candidate_list))**(tau))
    heapq.heappush(forest, (-1 * priority, current_cardinality, current_sum, victim_x, candidate_list, idx))
    idx += 1
    while forest:
        priority, current_cardinality, _, victim_x, candidate_list, idx = heapq.heappop(forest)
        while 1 < len(candidate_list) and 0 < tau:
            left_node = candidate_list[0]
            right_node = victim_x.difference(set(left_node))
            [left_cardinality, left_sum] = utils.psi_sum_int(set(left_node), target_y)
            tau -= 1
            right_cardinality = current_cardinality - left_cardinality
            right_sum = current_sum - left_sum

            left_candidate_list = n_sum(left_node, left_sum, left_cardinality)
            right_candidate_list = n_sum(right_node, right_sum, right_cardinality)
            left_priority = len(left_node) if len(
                left_candidate_list) <= tau else len(left_node) * (1 - (1 - 1 / len(left_candidate_list))**(tau))
            right_priority = len(right_node) if len(
                right_candidate_list) <= tau else len(right_node) * (1 - (1 - 1 / len(right_candidate_list))**(tau))

            if left_priority > right_priority:
                heapq.heappush(
                    forest,
                    (-1 * right_priority, right_cardinality, right_sum, set(right_node), right_candidate_list, idx))
                idx += 1
                victim_x = set(left_node)
                current_cardinality = left_cardinality
                candidate_list = left_candidate_list
                current_sum = left_sum
            else:
                heapq.heappush(
                    forest, (-1 * left_priority, left_cardinality, left_sum, set(left_node), left_candidate_list, idx))
                idx += 1
                victim_x = set(right_node)
                current_cardinality = right_cardinality
                candidate_list = right_candidate_list
                current_sum = right_sum

        if len(candidate_list) == 1 and 0 <= tau:
            z_pos.update(set(candidate_list[0]))
            z_neg.update(victim_x.difference(set(candidate_list[0])))
    return (z_pos, z_neg, tau)
