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
    glog
    GIT_REPOSITORY https://github.com/google/glog.git
    GIT_TAG        96a2f23dca4cc7180821ca5f32e526314395d26a # 0.4.0
)
FetchContent_GetProperties(glog)

if(NOT glog_POPULATED)
    FetchContent_Populate(glog)

    set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
    set(WITH_GTEST OFF CACHE BOOL "" FORCE)
    set(WITH_UNWIND OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_GLOG)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_GLOG)

    add_subdirectory(
        ${glog_SOURCE_DIR}
        ${glog_SOURCE_DIR}/../glog-build
        EXCLUDE_FROM_ALL)
endif()
