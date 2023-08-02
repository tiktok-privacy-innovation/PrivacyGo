# Json Configuration Description
Please modify the relevant parameters according to your settings.

## Json Structure
```json
{
    "common": {
        "address": "127.0.0.1",
        "remote_port": 30330,
        "local_port": 30331,
        "timeout": 90,
        "input_file": "../data/sender_input_file.csv",
        "has_header": false,
        "output_file": "../data/sender_output_file.csv",
        "ids_num": 3,
        "is_sender": true,
        "verbose": true
    },
    "paillier_params": {
        "paillier_n_len": 2048,
        "enable_djn": true,
        "apply_packing": true,
        "statistical_security_bits": 40
    },
    "ecc_params": {
        "curve_id": 415
    },
    "dp_params": {
        "epsilon": 2.0,
        "maximum_queries": 10,
        "use_precomputed_tau": true,
        "precomputed_tau": 1440,
        "input_dp": true,
        "has_zero_column": false,
        "zero_column_index": -1
    }
}
```

## Parameters

| Name  |  Property | Type | Description | Default Value|
|---|---|---|---|---|
| common  |   |   |  |  |
|&emsp; address  |  required | string | Couterparty's ip address | 127.0.0.1 |
|&emsp; remote_port  |  required |  uint16 | Couterparty's Ip port.  | 30330 |
|&emsp; local_port  |  required |  uint16 | Local ip port.  | 30331 |
|&emsp; timeout  |  required |  uint64 | Timeout for net io.  | 90 |
|&emsp;  input_file |  optimal |  string |  Sender or receiver's input file. | "../data/sender_input_file.csv" |
|&emsp; has_header  |  optimal |  bool | Whether the input file has header. | false |
|&emsp; output_file |  optimal | string | The path of output file to save the additive shares belongs to the intersection sets.  | "../data/receiver_output_file.csv" |
|&emsp; ids_num  |  required |  uint64 | The number of ids column's of the sender or receiver.  | 3 |
|&emsp; is_sender  |  required |  bool |  Whether sender or receiver. | true |
|&emsp; verbose  |  required |  bool | Print logs or not. | true |
| paillier_params  |   |   |  |  |
|&emsp; paillier_n_len  |  required |  uint64 | The bit length of module n in the Paillier encryption.  | 2048 |
|&emsp; enable_djn  |  required |  bool | Enable DJN optimization or not.  | true |
|&emsp; apply_packing  |  required |  bool | Apply ciphertext packing or not.  | true |
|&emsp; statistical_security_bits |  required |  uint64 | The statistical security bits for randomness blinding in cipher packing.  | 40 |
| ecc_params  |   |   |  |  |
|&emsp; curve_id  |  required |  uint64 | Ecc curve id in openssl. | NID_X9_62_prime256v1(415) |
| dp_params  |   |   |  |  |
|&emsp; epsilon |  required |  double | Sensitity of differential privacy.  | 2.0 |
|&emsp; maximum_queries  |  required |  uint64 | The number of maximum queries of DPCA-PSI for one particular task. | 10 |
|&emsp; use_precomputed_tau |  required |  bool | Whether to use precomputed tau to avoid online calculation. | true |
|&emsp; precomputed_tau |  required |  uint64 | The precomputed tau corresponding to specific (data_size, epsilon, maximum_queries, key_size). | 1440 |
|&emsp; input_dp  |  required |  bool | Apply differentially privacy sampling or not. | true |
|&emsp; has_zero_column  |  required |  bool | Whether to add dummy data with a value of zero. | false |
|&emsp; zero_column_index  |  required |  int | The index indicates which column's dummy should be set to zero. | -1|
