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

add_executable(dpca_psi_bench dpca_psi_bench.cpp)
if(TARGET DPCA_PSI::dpca_psi)
    target_link_libraries(dpca_psi_bench PRIVATE DPCA_PSI::dpca_psi glog::glog gflags::gflags)
elseif(TARGET DPCA_PSI::dpca_psi_shared)
    target_link_libraries(dpca_psi_bench PRIVATE DPCA_PSI::dpca_psi_shared glog::glog gflags::gflags)
else()
    message(FATAL_ERROR "Cannot find target DPCA_PSI::dpca_psi or DPCA_PSI::dpca_psi_shared")
endif()

set(BIN_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(JSON_DIR ${CMAKE_CURRENT_LIST_DIR}/json)
set(LOG_DIR ${CMAKE_CURRENT_BINARY_DIR}/log)
configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/receiver_test.sh.in ${DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR}/receiver_test.sh @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/receiver_use_file_data.sh.in ${DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR}/receiver_use_file_data.sh @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/receiver_with_preprocessing.sh.in ${DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR}/receiver_with_preprocessing.sh @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/receiver_without_dp.sh.in ${DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR}/receiver_without_dp.sh @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/sender_test.sh.in ${DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR}/sender_test.sh @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/sender_use_file_data.sh.in ${DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR}/sender_use_file_data.sh @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/sender_with_preprocessing.sh.in ${DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR}/sender_with_preprocessing.sh @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/sender_without_dp.sh.in ${DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR}/sender_without_dp.sh @ONLY)
