
# Scripts Description
We take sender_with_preprocessing.sh as an example to illustrate the details.

## Script
```bash
BIN_DIR="@BIN_DIR@"
JSON_DIR="@JSON_DIR@"
LOG_DIR="@LOG_DIR@"

mkdir -p "${LOG_DIR}/privacy-go/bench/balanced"
mkdir -p "${LOG_DIR}/privacy-go/bench/unbalanced"

sender_feature_size=1
default_tau_array=(722 954 1194 1440 1690)
balanced_log_path_bandwith="${LOG_DIR}/privacy-go/bench/balanced"
echo "Sender balanced test"
balanced_intersection_size_array=(500 5000 50000 500000 5000000)
for(( i=0;i<${#balanced_intersection_size_array[@]};i++))
do
echo "Test intersection size ${balanced_intersection_size_array[i]}"
echo "with dp"
"${BIN_DIR}/ppam_example" --config_path="${JSON_DIR}/sender_with_precomputed_tau.json" --log_path=$balanced_log_path_bandwith --use_random_data=true --intersection_size=${balanced_intersection_size_array[i]} --intersection_ratio=2 --feature_size=$sender_feature_size --use_default_tau=true --default_tau=${default_tau_array[i]}
done

unbalanced_log_path_bandwith="${LOG_DIR}/privacy-go/bench/unbalanced"
echo "Sender unbalanced test"
unbalanced_intersection_size_array=(10 100 1000 10000 100000)
for(( i=0;i<${#unbalanced_intersection_size_array[@]};i++))
do
echo "Test intersection size ${unbalanced_intersection_size_array[i]}"
echo "with dp"
"${BIN_DIR}/ppam_example" --config_path="${JSON_DIR}/sender_with_precomputed_tau.json" --log_path=$unbalanced_log_path_bandwith --use_random_data=true --intersection_size=${unbalanced_intersection_size_array[i]} --intersection_ratio=100 --feature_size=$sender_feature_size --use_default_tau=true --default_tau=${default_tau_array[i]}
done
```

## Parameters

| Name  |  Property | Type | Description | Default Value|
|---|---|---|---|---|
|config_path  |  required |  string | The path where the sender's config file located. | "./json/sender_with_precomputed_tau.json" |
|use_random_data  |  required |  bool | Use randomly generated data or read data from files. | true |
|log_path  |  optimal | string | The directory where log file located. | "./logs/" |
|intersection_size  |  required if use_random_data = true |  uint64 | The intersection size of both party.| 10 |
|intersection_ratio  |  required if use_random_data = true |  uint64 | The ratio of sender/receiver data size to intersection size. | 100 |
|feature_size  |  required if use_random_data = true |  uint64 | The feature size of sender/receiver data. | 1 |
|use_default_tau  |  required if you want to specify tau via command line rather than json config  |  bool | Whether or not use default tau, just for testing. | false |
|default_tau  |  required if you want to specify tau via command line rather than json config |  uint64 | Default tau, just for testing. | 1440 |
