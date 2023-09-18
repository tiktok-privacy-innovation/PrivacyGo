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

set(PPAM_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(PPAM_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(PPAM_BUILD_SHARED_LIBS ${MPC_DUALDP_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)

set(PPAM_BUILD_DIR ${MPC_DUALDP_THIRDPARTY_DIR}/ppam-build CACHE STRING "" FORCE)

add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../ppam
    ${PPAM_BUILD_DIR}
    EXCLUDE_FROM_ALL)
