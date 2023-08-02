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

set(DPCA_PSI_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(DPCA_PSI_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(DPCA_PSI_BUILD_SHARED_LIBS ${PPAM_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)

set(DPCA_PSI_BUILD_DIR ${PPAM_THIRDPARTY_DIR}/dpca-psi-build CACHE STRING "" FORCE)

add_subdirectory(
    ${CMAKE_SOURCE_DIR}/../dpca-psi
    ${DPCA_PSI_BUILD_DIR}
    EXCLUDE_FROM_ALL)
