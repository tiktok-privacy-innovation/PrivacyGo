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

# Source files in this directory
set(DPCA_PSI_SOURCE_FILES ${DPCA_PSI_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/aes.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dp_sampling.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ecc_cipher.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ipcl_paillier.cpp
    ${CMAKE_CURRENT_LIST_DIR}/prng.cpp
)

# Test files in this directory
set(DPCA_PSI_TEST_FILES ${DPCA_PSI_TEST_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/aes_test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dp_sampling_test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ecc_cipher_test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ipcl_paillier_test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/prng_test.cpp
)

# Add header files for installation
install(
    FILES
        ${CMAKE_CURRENT_LIST_DIR}/aes.h
        ${CMAKE_CURRENT_LIST_DIR}/dp_sampling.h
        ${CMAKE_CURRENT_LIST_DIR}/ecc_cipher.h
        ${CMAKE_CURRENT_LIST_DIR}/ipcl_paillier.h
        ${CMAKE_CURRENT_LIST_DIR}/ipcl_utils.h
        ${CMAKE_CURRENT_LIST_DIR}/prng.h
        ${CMAKE_CURRENT_LIST_DIR}/smart_pointer.h
    DESTINATION
        ${DPCA_PSI_INCLUDES_INSTALL_DIR}/dpca-psi/crypto
)

set(DPCA_PSI_SOURCE_FILES ${DPCA_PSI_SOURCE_FILES} PARENT_SCOPE)
set(DPCA_PSI_TEST_FILES ${DPCA_PSI_TEST_FILES} PARENT_SCOPE)
