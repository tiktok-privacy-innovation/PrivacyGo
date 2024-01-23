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
"""This file includes a testing example."""

import matplotlib.pyplot as plt
import numpy as np
from tqdm import tqdm

import actbayesian
import baseline
import dynpathblizer
import dynpathblizer_params
import improved_baseline
import treesumexplorer
import utils


def algorithms_compare(tau, victim_x, target_y, gamma, phi, step, iteration, lower_bound, upper_bound, tolerance,
                       noise_scale, sampling_rate):
    """Runs all algorithms in this repo to compare their performance under given parameters.

    Args:
        tau: A int indicates the PSI call budget.
        victim_x: A set of victim elements X.
        target_y: A set of target elements Y.
        gamma: A hash map represents the optimal partition K for current state(N, C_N, tau).
        phi: A hash map represents average call required for current state(N, C_N).
        step: A int indicates the step of tau in testing.
        iteration: A int indicates the number of tesing times.
        lower_bound: A float indicates the lower bound of the stopping criterion in actbayesian.
        upper_bound: A float indicates the upper bound of the stopping criterion in actbayesian.
        tolerance: A floar indicates the tolerance factor in actbayesian.
        noise_scale: A float indicates the noise factor of the Laplacian mechanism in actbayesian.
        sample_rate: A float indicates the sampling rate in actbayesian.

    Returns:
        tau_list: A list of PSI call budget.
        y_base: leakage of the baseline attack.
        y_impr_base: leakage of the improvded baseline attack.
        y_dypathblizer: leakage of the dypathblizer.
        y_treesumexplorer: leakage of the treesumexplorer.
        y_actbayesian: leakage of the actbayesian.
    """
    leakage_base = []
    leakage_impr_base = []
    leakage_treesumexplorer = []
    leakage_dynpathblizer = []
    leakage_actbayesian = []

    inpute_dataset_actbayesian = []
    for x in victim_x:
        if x in target_y:
            inpute_dataset_actbayesian.append(1)
        else:
            inpute_dataset_actbayesian.append(0)

    for k in tqdm(range(0, tau, step)):
        cur_z_base = 0
        cur_z_impr_base = 0
        cur_z_dynpathblizer = 0
        cur_z_treesumexplorer = 0
        cur_z_actbayesian = 0
        for _ in range(iteration):
            [z_pos_base, z_neg_base] = baseline.baseline_attack_with_limited_call(victim_x, target_y, k)
            [z_pos_impr_base,
             z_neg_impr_base] = improved_baseline.improved_baseline_attack_with_limited_call(victim_x, target_y, k)
            [z_pos_dynpathblizer, z_neg_dynpathblizer] = dynpathblizer.dynpathblizer(victim_x, target_y, k, gamma, phi)
            [z_pos_treesumexplorer, z_neg_treesumexplorer] = treesumexplorer.psi_sum_attack(victim_x, target_y, k, 1e10)
            [z_pos_actbayesian, z_neg_actbayesian, _,
             _] = actbayesian.actbayesian(k, lower_bound, upper_bound, inpute_dataset_actbayesian, tolerance,
                                          noise_scale, sampling_rate)

            if len(z_pos_base) + len(z_neg_base) == utils.psi_cardinality(victim_x, target_y):
                cur_z_base += len(victim_x)
            else:
                cur_z_base += len(z_pos_base) + len(z_neg_base)

            cur_z_impr_base += len(z_pos_impr_base) + len(z_neg_impr_base)
            cur_z_dynpathblizer += len(z_pos_dynpathblizer) + len(z_neg_dynpathblizer)
            cur_z_treesumexplorer += len(z_pos_treesumexplorer) + len(z_neg_treesumexplorer)
            cur_z_actbayesian += z_pos_actbayesian + z_neg_actbayesian

        if len(leakage_base) > 0 and cur_z_base / iteration < leakage_base[-1]:
            leakage_base[-1] = cur_z_base / iteration

        leakage_base.append(cur_z_base / iteration)
        leakage_impr_base.append(cur_z_impr_base / iteration)
        leakage_dynpathblizer.append(cur_z_dynpathblizer / iteration)
        leakage_treesumexplorer.append(cur_z_treesumexplorer / iteration)
        leakage_actbayesian.append(cur_z_actbayesian / iteration)

    tau_list = np.array(list(range(1, tau + 1, step)))
    y_base = np.array(leakage_base)
    y_impr_base = np.array(leakage_impr_base)
    y_dypathblizer = np.array(leakage_dynpathblizer)
    y_treesumexplorer = np.array(leakage_treesumexplorer)
    y_actbayesian = np.array(leakage_actbayesian)

    return [tau_list, y_base, y_impr_base, y_dypathblizer, y_treesumexplorer, y_actbayesian]


def test():
    """Plot utils with default parameters"""
    victim_set_size = 20
    target_set_size = 20
    interserction_cardinality = 10
    dense = 2
    dummy_set_size = 0
    lower_bound = 0.25
    upper_bound = 0.8
    tolerance = 0.1
    noise_scale = 0
    sampling_rate = 0.5
    [victim_x, target_y, _] = utils.gen_dummy_set(victim_set_size, target_set_size, dummy_set_size, dense,
                                                  interserction_cardinality)
    [gamma, phi] = dynpathblizer_params.gen_gamma_and_phi(victim_set_size)

    tau = 5
    step = 1
    iteration = 200
    [tau_list, y_base, y_impr_base, y_dypathblizer, y_treesumexplorer,
     y_actbayesian] = algorithms_compare(tau, victim_x, target_y, gamma, phi, step, iteration, lower_bound, upper_bound,
                                         tolerance, noise_scale, sampling_rate)

    # plot
    plt.xlabel('# of PSI Runs')
    plt.ylabel('# of User Leakage')
    plt.grid(True)

    plt.plot(tau_list, y_base, label="USENIX22 attacking", marker='s', linewidth=2.0)
    plt.plot(tau_list, y_impr_base, label="Improved USENIX22", marker='<', linewidth=2.0)
    plt.plot(tau_list, y_dypathblizer, label="dynapathblizer_DP", marker='o', linewidth=2.0)
    plt.plot(tau_list, y_treesumexplorer, label="treesumexplorer", marker='d', linewidth=2.0)
    plt.plot(tau_list, y_actbayesian, label="actbayesian", marker='>', linewidth=2.0)
    plt.rcParams.update({'font.size': 9})
    plt.rc('axes', titlesize=12)
    plt.legend()
    plt.savefig('test', dpi=600)
    plt.show()


if __name__ == "__main__":
    test()
