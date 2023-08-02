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
    gflags
    GIT_REPOSITORY https://github.com/gflags/gflags.git
    GIT_TAG        e171aa2d15ed9eb17054558e0b3a6a413bb01067 # 2.2.2
)
FetchContent_GetProperties(gflags)

if(NOT gflags_POPULATED)
    FetchContent_Populate(gflags)

    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_GFLAGS)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_GFLAGS)

    add_subdirectory(
        ${gflags_SOURCE_DIR}
        ${gflags_SOURCE_DIR}/../gflags-build
        EXCLUDE_FROM_ALL)
endif()
