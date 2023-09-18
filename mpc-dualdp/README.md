# MPC-DualDP
[![License](https://img.shields.io/badge/license-Apache%202-blue.svg)](LICENSE)

Secure multiparty computation (MPC) is a desired tool to provide privacy to the input data and intermediate results during secure computation. However, MPC can not help if the computation results leak information about the input data. To bound information leakage in outputs of protocols, we can apply differential privacy such that the MPC outputs are perturbed by the addition of noise before the outputs are revealed. Therefore, we need a mechanism that allows MPC servers to collaboratively generate random noise in a secret fashion.

MPC-DualDP is a distributed protocol for generating shared differential privacy noise in a two-server setting. MPC-DualDP leverages MPC to sample random noise according to specific distributions, and outputs the noise in the form of secret sharing.

Currently, we support the generation of noise following binomial distribution, and more distributions might be supported in the future. As for the security model, our protocol and implementation provide security and privacy against two semi-honest servers. The round complexity of our protocol is 1, and the communication complexity is $O(n)$ if we consider a binomial distribution $Bin(n,p = 0.5)$.

## How to use MPC-DualDP

### Requirements

- Linux
- CMake (>=3.15)
- GNU G++ (>=5.5) or Clang++ (>= 5.0)
- Python 3

### Building MPC-DualDP

MPC-DualDP depends on [PPAM](../ppam/README.md). You need to build the dependencies of PPAM first, and then build MPC-DualDP.

First, install NASM with `apt install nasm`, or build and install NASM from source code downloaded from [NASM's official page](https://www.nasm.us/). Assume that you are working in the root directory of NASM source code.
```shell
./configure
make -j
make install
```

Second, build and install IPCL using the following scripts.
Assume that IPCL is cloned into the directory `${IPCL}` and will be installed to the directory `${IPCL_INSTALL_DIR}`.
```shell
cmake -B ${IPCL}/build -S ${IPCL} -DCMAKE_INSTALL_PREFIX=${IPCL_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release -DIPCL_TEST=OFF -DIPCL_BENCHMARK=OFF
cmake --build ${IPCL}/build -j
cmake --build ${IPCL}/build --target install
```

Third, build [JSON for Modern C++ (JSON)](https://github.com/nlohmann/json) using the following scripts.
Assume that JSON is cloned into the directory `${JSON}`.
```shell
cmake -B ${JSON}/build -S ${JSON}
cmake --build ${JSON}/build -j
```

At last, build MPC-DualDP using the following scripts.
Assume that MPC-DualDP is cloned into the directory `${MPC-DualDP}`.
```shell
cmake -B ${MPC-DualDP}/build -S ${MPC-DualDP} -DCMAKE_BUILD_TYPE=Release -DIPCL_DIR=${IPCL_INSTALL_DIR}/lib/cmake/ipcl-2.0.0  -Dnlohmann_json_DIR=${JSON}/build
cmake --build ${MPC-DualDP}/build -j
```

Output binaries can be found in `${MPC-DualDP}/build/lib/` and `${MPC-DualDP}/build/bin/` directories.

| Compile Options                | Values        | Default | Description                                         |
|--------------------------------|---------------|---------|-----------------------------------------------------|
| `CMAKE_BUILD_TYPE`             | Release/Debug | Release | The build type.                                     |
| `MPC-DUALDP_BUILD_SHARED_LIBS` | ON/OFF        | OFF     | Build a shared library if set to ON.                |
| `MPC-DUALDP_BUILD_EXAMPLE`     | ON/OFF        | ON      | Build C++ example if set to ON.                     |
| `MPC-DUALDP_BUILD_TEST`        | ON/OFF        | ON      | Build C++ test if set to ON.                        |
| `MPC-DUALDP_BUILD_DEPS`        | ON/OFF        | ON      | Download and build unmet dependencies if set to ON. |

MPC-DualDP further depends on [Google Test](https://github.com/google/googletest).
The build system will try to find these dependencies if they exist or will otherwise automatically download and build them.

### Running the MPC-DualDP protocol

Here we give a simple example to run our protocol.

To run Party A
```shell
cd ${MPC-DualDP}/build/bin
./mpc_dualdp_example  127.0.0.1 8899 127.0.0.1 8890 0
```

To run Party B
```shell
cd ${MPC-DualDP}/build/bin
./mpc_dualdp_example  127.0.0.1 8890 127.0.0.1 8899 1
```

## License

MPC-DualDP is Apache-2.0 License licensed, as found in the [LICENSE](../LICENSE) file.

## Disclaimers

This software is not an officially supported product of TikTok. It is provided as-is, without any guarantees or warranties, whether express or implied.
