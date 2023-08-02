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
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_GetProperties(eigen)

if(NOT eigen_POPULATED)
    FetchContent_Populate(eigen)

    set(EIGEN_TEST_CXX11 ON CACHE BOOL "" FORCE)
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_EIGEN)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_EIGEN)

    add_subdirectory(
        ${eigen_SOURCE_DIR}
        ${eigen_SOURCE_DIR}/../eigen-build
        EXCLUDE_FROM_ALL)
endif()
