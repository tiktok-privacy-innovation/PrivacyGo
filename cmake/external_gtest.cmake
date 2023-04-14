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

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        58d77fa8070e8cec2dc1ed015d66b454c8d78850 # 1.12.1
)
FetchContent_GetProperties(googletest)

if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)

    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    mark_as_advanced(BUILD_GMOCK)
    mark_as_advanced(INSTALL_GTEST)
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_GOOGLETEST)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_GOOGLETEST)

    add_subdirectory(
        ${googletest_SOURCE_DIR}
        ${googletest_SOURCE_DIR}/../googletest-build
        EXCLUDE_FROM_ALL)
endif()
