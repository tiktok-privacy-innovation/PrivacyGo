# Example running DPCA-PSI

## Quick Start

We provide four pairs of scripts to help users run our protocol within different settings, as you can see under [scripts](./scripts/) directory.

Party A
```shell
cd ${DPCA-PSI}/build/example/scripts
bash sender_test.sh/sender_with_preprocessing.sh/sender_without_dp.sh/sender_use_file_data.sh
```

Party B
```shell
cd ${DPCA-PSI}/build/example/scripts
bash receiver_test.sh/receiver_with_preprocessing.sh/receiver_without_dp.sh/receiver_use_file_data.sh
```

| Party A  |  Party B | Description |
|---|---|---|
| sender_test.sh  |  receiver_test.sh | Simple test of DPCA-PSI.|
| sender_with_preprocessing.sh  |  receiver_with_preprocessing.sh | Benchmark of DPCA-PSI with input set ranging from 1 thousand to 10 million.|
| sender_without_dp.sh  |  receiver_without_dp.sh | Benchmark of DPCA-PSI without applying differentially privacy sampling. |
| sender_use_file_data.sh |  receiver_use_file_data.sh | Simple test of DPCA-PSI using file data. Before running this script, please change the "input_file" and "output_file" of the json configuration of [Party A](./json/sender_with_precomputed_tau.json) and [Party B](./json/receiver_with_precomputed_tau.json) to the correct path of the files.|

Please refer to [Scripts Description](./scripts/README.md) for more details about parameters description.

## How to Choose the Number of Dummy Sets

The number of dummy sets $`{\tau}`$  can be set either in json config or a specific value default_tau when executing dpca_psi_example.

Users can select appropriate parameters according to our table, and we will give the specific calculation formulas in the future.

### Multi-ID(3)

$`T`$: the number of maximum queries.

$`(\epsilon, \delta)`$: DP budget.

$`N`$: maximum data size between sender and receiver.

|  |  | |  | $`T=6`$ | | |  |  | $`T=10`$ |  |   |   |   |  $`T=20`$ |  |  |
|:---------------------:|:---------------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|
| Input size $`N`$| $`\delta = 1 / (10N)`$ | $`\epsilon=1`$ | $`\epsilon=2`$ | $`\epsilon=4`$ | $`\epsilon=6`$ | $`\epsilon=8`$ | $`\epsilon=1`$ | $`\epsilon=2`$ | $`\epsilon=4`$ | $`\epsilon=6`$ | $`\epsilon=8`$ | $`\epsilon=1`$ | $`\epsilon=2`$ | $`\epsilon=4`$ | $`\epsilon=6`$ | $`\epsilon=8`$ |
| $`10^3`$ | $`10^{-4}`$ | 1462 |  433 | 133 |  68 | 43 | 2437 |  **722** | 221 | 113 |  71 |  4882 | 1445 | 441 | 225 | 142 |
| $`10^4`$ | $`10^{-5}`$ | 2005 |  573 | 169 |  84 | 52 | 3342 |  **954** | 281 | 140 |  87 |  6697 | 1910 | 561 | 280 | 173 |
| $`10^5`$ | $`10^{-6}`$ | 2571 |  717 | 205 | 101 | 62 | 4286 | **1194** | 342 | 168 | 103 |  8588 | 2390 | 684 | 336 | 205 |
| $`10^6`$ | $`10^{-7}`$ | 3153 |  864 | 243 | 118 | 71 | 5257 | **1440** | 405 | 196 | 119 | 10534 | 2882 | 809 | 392 | 237 |
| $`10^7`$ | $`10^{-8}`$ | 3747 | 1014 | 281 | 135 | 81 | 6247 | **1690** | 468 | 224 | 135 | 12518 | 3382 | 935 | 449 | 269 |

### Single-ID

|  |  | |  | $`T=6`$ | | |  |  | $`T=10`$ |  |   |   |   |  $`T=20`$ |  |  |
|:---------------------:|:---------------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|:-------------------:|
| Input size $`N`$| $`\delta = 1 / (10N)`$ | $`\epsilon=1`$ | $`\epsilon=2`$ | $`\epsilon=4`$ | $`\epsilon=6`$ | $`\epsilon=8`$ | $`\epsilon=1`$ | $`\epsilon=2`$ | $`\epsilon=4`$ | $`\epsilon=6`$ | $`\epsilon=8`$ | $`\epsilon=1`$ | $`\epsilon=2`$ | $`\epsilon=4`$ | $`\epsilon=6`$ | $`\epsilon=8`$ |
| $`10^3`$ | $`10^{-4}`$ |  487 | 145 | 45 | 23 | 15 |  812 | 241 |  74 | 38 | 24 | 1624 |  481 | 147 |  75 | 47 |
| $`10^4`$ | $`10^{-5}`$ |  669 | 191 | 57 | 29 | 18 | 1114 | 318 |  94 | 47 | 29 | 2227 |  636 | 187 |  94 | 58 |
| $`10^5`$ | $`10^{-6}`$ |  857 | 240 | 69 | 35 | 22 | 1428 | 399 | 114 | 56 | 35 | 2857 |  796 | 228 | 112 | 69 |
| $`10^6`$ | $`10^{-7}`$ | 1052 | 289 | 82 | 40 | 25 | 1752 | 480 | 135 | 66 | 40 | 3503 |  960 | 270 | 131 | 79 |
| $`10^7`$ | $`10^{-8}`$ | 1250 | 339 | 95 | 46 | 29 | 2082 | 564 | 157 | 76 | 46 | 4163 | 1127 | 312 | 150 | 90 |

## Sender/Receiver Configuration

Please refer to [Json Configuration Description](./json/README.md) for more details about the json configuration for sender and receiver.
