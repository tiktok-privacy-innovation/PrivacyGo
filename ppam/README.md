# PPAM
[![License](https://img.shields.io/badge/license-Apache%202-blue.svg)](LICENSE)

Privacy-Preserving Ads Measurement (PPAM) shows how our [DPCA-PSI](../dpca-psi/README.md) protocol finds practical application in the field of advertising, specifically in scenarios where ad publishers and advertisers need to share their privileged data privately for measuring the effectiveness of advertising campaigns while maintaining privacy.  A significant challenge in such scenarios is that the data required for computing attribution statistics is typically split between two parties who may not be willing or able to share the underlying data. One party holds identifiers corresponding to its users who have viewed the advertising campaign, while the other party holds its identifiers and numeric values related to their users who have made purchases and corresponding spending amounts. These parties aim to compute aggregate population-level measurements, such as the number of users who both viewed an ad and made a purchase, as well as the total amount spent by these users, all while ensuring the privacy of individual user data.

There are two parts of our protocols for private ads measurement.
1. **Intersect**: Matching the identities of the converters from advertisers' offsite outcomes data and ad impressed users in ad publishers' onsite impression data. This will hide the identities of users at the intersection and the non-intersections. DPCA-PSI serves as this first step.
2. **Compute**: Determining if conversions happen within a pre-determined window after impressions and aggregating the qualified conversions, while protecting the conversion values and learning output of the computation. Only predetermined computation will be learned from the compute step.

PPAM effectively prevents advertisers and publishers from tracking their users' activities on each other's platforms, ensuring user privacy and adhering to relevant privacy requirements.

## How to use PPAM

### Requirements

- Linux
- CMake (>=3.15)
- GNU G++ (>=5.5) or Clang++ (>= 5.0)
- Python 3

### Building PPAM

PPAM depends on [DPCA-PSI](../dpca-psi/README.md). You need to build the dependencies of DPCA-PSI first, and then build PPAM.

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

At last, build PPAM using the following scripts.
Assume that PPAM is cloned into the directory `${PPAM}`.
```shell
cmake -B ${PPAM}/build -S ${PPAM} -DCMAKE_BUILD_TYPE=Release -DIPCL_DIR=${IPCL_INSTALL_DIR}/lib/cmake/ipcl-2.0.0  -Dnlohmann_json_DIR=${JSON}/build
cmake --build ${PPAM}/build -j
```

Output binaries can be found in `${PPAM}/build/lib/` and `${PPAM}/build/bin/` directories.

| Compile Options          | Values       | Default | Description                         |
|--------------------------|--------------|---------|-------------------------------------|
| `CMAKE_BUILD_TYPE`        | Release/Debug| Release | The build type.                     |
| `PPAM_BUILD_SHARED_LIBS` | ON/OFF        | OFF     | Build a shared library if set to ON.          |
| `PPAM_BUILD_EXAMPLE`        | ON/OFF        | ON      | Build C++ example if set to ON.                 |
| `PPAM_BUILD_TEST`                | ON/OFF        | ON      | Build C++ test if set to ON.                        |
| `PPAM_BUILD_DEPS`               | ON/OFF        | ON      | Download and build unmet dependencies if set to ON. |

PPAM further depends on [OpenSSL](https://github.com/openssl/openssl), [Eigen](https://gitlab.com/libeigen/eigen), [gflags](https://github.com/gflags/gflags), [Google Logging](https://github.com/google/glog), and [Google Test](https://github.com/google/googletest).
The build system will try to find these dependencies if they exist or will otherwise automatically download and build them.

### Running the PPAM protocol

Here we give a simple example to run our protocol. Please refer to [Example running PPAM](example/README.md) for more details.

To run Party A
```shell
cd ${PPAM}/build/example/scripts
bash sender_test.sh
```

To run Party B
```shell
cd ${PPAM}/build/example/scripts
bash receiver_test.sh
```

## License

PPAM is Apache-2.0 License licensed, as found in the [LICENSE](../LICENSE) file.

## Disclaimers

This software is not an officially supported product of TikTok. It is provided as-is, without any guarantees or warranties, whether express or implied.
