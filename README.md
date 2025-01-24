# PrivacyGo
[![License](https://img.shields.io/badge/license-Apache%202-blue.svg)](LICENSE)

Welcome to PrivacyGo, an open-source project dedicated to exploring the fusion of synergistic privacy-enhancing technologies (PETs). Our mission is to enable secure and privacy-preserving computations by leveraging innovative technological advancements.

We meticulously design strategies that harness the unique strengths of each PET, effectively mitigating their individual limitations, especially in privacy-sensitive scenarios. Through collaboration and a shared vision, we aim to shape a safer, more privacy-centric future.

Whether you're a developer, a privacy advocate, or simply enthusiastic about technological advancements, we welcome your contributions and ideas, which will be the driving force behind this transformative endeavor.

## DPCA-PSI

Among various PSI protocols that have been developed in academia and industry, ECDH-style PSI are widely used due to their low communication complexity, especially for large-scale data handling and multi-ID matching. Recent studies in [USENIX22'](https://www.usenix.org/system/files/sec22-guo.pdf) and [PoPETs23'](https://petsymposium.org/popets/2023/popets-2023-0043.pdf) revealed that ECDH-style PSI protocols disclosing intersection size might unintentionally leak membership information about the parties' sets. Particularly, in cases involving multi-ID records, the leakage is more significant compared to single-ID record cases. To address the de-anonymization concerns in PSI, we developed the DPCA-PSI protocol for computing intersection-related statistics from private datasets while reducing privacy leakage. For more detailed information, please refer to [this folder](./dpca-psi).

## PPAM

Privacy-Preserving Ads Measurement (PPAM) leveraging DPCA-PSI enables private ad measurement, offering advertisers the ability to measure the effectiveness of their ads while ensuring user privacy protection. This is achieved through the implementation of key privacy-preserving features, such as encrypted match keys and differential privacy (DP) guaranteed matched group size. These features ensure that user interaction between ad provider and advertiser for ad measurement computation cannot be traced back to individual users. For more detailed information, please refer to [this folder](./ppam).

## MPC-DualDP
Secure multiparty computation (MPC) is a desired tool to provide privacy to the input data and intermediate results during secure computation. However, MPC can not help if the computation results leak information about the input data. To bound information leakage in outputs of protocols, we can apply differential privacy such that the MPC outputs are perturbed by the addition of noise before the outputs are revealed. Therefore, we need a mechanism that allows MPC servers to collaboratively generate random noise in a secret fashion.

MPC-DualDP is a distributed protocol for generating shared differential privacy noise in a two-server setting. MPC-DualDP leverages MPC to sample random noise according to specific distributions, and outputs the noise in the form of secret sharing.  For more detailed information, please refer to [this folder](./mpc-dualdp).

## AnonPSI
The widely used ECDH-PSI, while keeping all data encrypted, discloses the size of the intersection set during protocol execution. We refer to such protocols as size-revealing PSI. AnonPSI offers a framework for systematically assessing the privacy of intersection-size-revealing PSI protocols by employing carefully designed set membership inference attacks. It enables an adversary to infer whether a targeted individual is in the intersection, which is also known as membership information. For more detailed information, please refer to [this folder](./anonpsi). AnonPSI was recently accepted for NDSS24, and we look forward to engaging in discussions during the offline sessions at NDSS.

## GCMS/OCMS
Local Differential Privacy (LDP) protocols enable the collection of randomized client messages for data analysis, without the necessity of a trusted data curator. Such protocols have been successfully deployed in real-world scenarios by major tech companies like Google, Apple, and Microsoft.

We propose a Generalized Count Mean Sketch (GCMS) protocol that captures many existing frequency estimation protocols. Our method significantly improves the three-way trade-offs between communication, privacy, and accuracy. We also introduce a general utility analysis framework that enables optimizing parameter designs. Based on that, we propose an
Optimal Count Mean Sketch (OCMS) framework that minimizes the variance for collecting items with targeted frequencies. Moreover, we present a novel protocol for collecting data within unknown domain, as our frequency estimation protocols only work effectively with known data domain. For more detailed information, please refer to [this folder](./ldp).

## Contribution

Please check [Contributing](CONTRIBUTING.md) for more details.

## Code of Conduct

Please check [Code of Conduct](CODE_OF_CONDUCT.md) for more details.

## License

PrivacyGo is Apache-2.0 License licensed, as found in the [LICENSE](LICENSE) file.

## Disclaimers

This software is not an officially supported product of TikTok. It is provided as-is, without any guarantees or warranties, whether express or implied.
