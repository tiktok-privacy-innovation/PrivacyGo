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
"""This file includes actbayesian algorithm."""

import random

import numpy as np


def actbayesian(tau, lower_bound, upper_bound, dataset, tolerance, laplacian_scale, sample_rate):
    """ActBaysian algorithm.
    Args:
        tau: A int indicates the remaining protocol call times.
        lower_bound: A float indicates the lower bound of the stopping criterion.
        upper_bound: A float indicates the upper bound of the stopping criterion.
        dataset: input data set with binary label (0,1).
        laplacian_scale: A float indicates the noise factor of the Laplacian mechanism.
        sample_rate: A float indicates the sampling rate.

    Returns:
        true_pos_leak: predicted postitive member set.
        true_neg_leak: predicted negative member set.
        pos_err: error in predicting positive member set.
        neg_err: error in predicting negative member set.
    """
    laplacian_location = 0  # noise parameter
    laplacian_noise = np.random.laplace(laplacian_location, laplacian_scale)  # add laplacian noise if needed

    tau = tau - 1  # use one protocol call to find the inital prior vector

    # preprocess the data set to be an 0,1 vector so sum (n) = PSI (dataset)
    n = len(dataset)
    summation = np.sum(dataset)
    summation = summation + laplacian_noise * summation * 1
    summation = max(summation, 0)
    summation = min(summation, len(dataset))
    prior = summation / n

    posterior = [prior for _ in range(n)]  # inital prior vector.
    random.shuffle(posterior)
    tau -= 1  # protocol budgeted used
    while 0 <= tau:
        if max(posterior) < 1:
            # upper_bound threshold for selecting input
            positive_bar = max(posterior) - tolerance
        else:
            positive_bar = list(set(posterior[:]))
            if len(positive_bar) < 2:
                break
            else:
                positive_bar.sort(reverse=True)
                positive_bar = positive_bar[1] - tolerance

        positive_set = []
        negtive_set = []
        expected_psi_cardinality = 0
        for j in range(len(posterior)):
            laplacian_noise = np.random.laplace(laplacian_location, laplacian_scale)
            cur_posterior = posterior[j]
            if cur_posterior >= positive_bar:
                r = random.choices([0, 1], [1 - sample_rate, sample_rate])[0]
                if r:
                    positive_set.append(j)
                else:
                    negtive_set.append(j)
                expected_psi_cardinality += cur_posterior

        real_psi_cardinality_returned = 0

        if not positive_set:
            for j in negtive_set:
                if posterior[j] >= 0.5:
                    posterior[j] = 1
                else:
                    posterior[j] = 0
            continue

        if not negtive_set:
            for j in positive_set:
                if posterior[j] >= 0.5:
                    posterior[j] = 1
                else:
                    posterior[j] = 0
            continue

        for j in range(len(dataset)):
            if j in positive_set and dataset[j] == 1:
                real_psi_cardinality_returned += 1

        real_psi_cardinality_returned += laplacian_noise * 0.1 * real_psi_cardinality_returned
        real_psi_cardinality_returned = max(real_psi_cardinality_returned, 0)
        real_psi_cardinality_returned = min(real_psi_cardinality_returned, len(positive_set))
        in_setprior = real_psi_cardinality_returned / len(positive_set)
        out_setprior = (expected_psi_cardinality - real_psi_cardinality_returned) / len(negtive_set)

        for j in range(len(posterior)):
            if j in positive_set:
                posterior[j] = in_setprior
            if j in negtive_set:
                posterior[j] = out_setprior

        for j in range(len(posterior)):
            if posterior[j] >= upper_bound:
                posterior[j] = 1
            if posterior[j] <= lower_bound:
                posterior[j] = 0

        if list(set(posterior)) == [0, 1] or list(set(posterior)) == [1, 0]:
            break
        tau -= 1

    error_pos = 0
    error_neg = 0
    true_pos_leak = 0
    true_neg_leak = 0
    for j in range(len(posterior)):
        if posterior[j] == 1:
            true_pos_leak += 1
            if dataset[j] == 0:
                error_pos += 1
                true_pos_leak -= 1
        elif posterior[j] == 0:
            true_neg_leak += 1
            if dataset[j] == 1:
                error_neg += 1
                true_neg_leak -= 1

    if true_pos_leak == 0:
        pos_err = 0
    else:
        pos_err = error_pos

    if true_neg_leak == 0:
        neg_err = 0
    else:
        neg_err = error_neg

    return true_pos_leak, true_neg_leak, pos_err, neg_err
