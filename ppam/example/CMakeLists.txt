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

cmake_minimum_required(VERSION 3.15)

project(PPAMExamples VERSION 0.1.0 LANGUAGES CXX)

# If not called from root CMakeLists.txt
if(NOT DEFINED PPAM_BUILD_EXAMPLE)
    set(PPAM_BUILD_EXAMPLE ON)

    # Import PPAM
    find_package(PPAM  0.1.0 EXACT REQUIRED)

    add_compile_options(-msse4.2 -maes -mavx -Wno-ignored-attributes)

    # Must define these variables and include macros
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
    set(PPAM_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
    set(THIRDPARTY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
    include(FetchContent)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_QUIET)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)
    include(PPAMCustomMacros)
else()
    set(THIRDPARTY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../thirdparty)
endif()

if(NOT DEFINED PPAM_BUILD_DEPS)
    # [option] PPAM_BUILD_DEPS (default: ON)
    # Download and build missing dependencies, throw error if disabled.
    set(PPAM_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
    option(PPAM_BUILD_DEPS ${PPAM_BUILD_DEPS_OPTION_STR} ON)
endif()

if(PPAM_BUILD_EXAMPLE)
    # gflags::gflags
    set (GFLAGS_USE_TARGET_NAMESPACE TRUE)
    find_package(gflags 2.2.2 QUIET CONFIG)
    if(gflags_FOUND)
        message(STATUS "gflags: found")
    else()
        if(PPAM_BUILD_DEPS)
            message(STATUS "gflags: downloading ...")
            ppam_fetch_thirdparty_content(ExternalGflags)
        else()
            message(FATAL_ERROR "gflags: not found, please download and install manually")
        endif()
    endif()

    # glog::glog
    if(NOT TARGET glog::glog)
        find_package(glog 0.4.0 QUIET CONFIG)
        if(glog_FOUND)
            message(STATUS "glog: found")
        else()
            if(PPAM_BUILD_DEPS)
                message(STATUS "glog: download ...")
                ppam_fetch_thirdparty_content(ExternalGlog)
            else()
                message(FATAL_ERROR "glog: not found, please download and install manually")
            endif()
        endif()
    endif()

    # Add source files to example
    set(PPAM_EXAMPLE_FILES "")
    # Test files in this directory
    set(PPAM_EXAMPLE_FILES ${PPAM_EXAMPLE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/ppam_example.cpp
    )

    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")
    add_executable(ppam_example ${PPAM_EXAMPLE_FILES})

    if(TARGET PPAM::ppam)
        target_link_libraries(ppam_example PRIVATE PPAM::ppam glog::glog gflags::gflags)
    elseif(TARGET PPAM::ppam_shared)
        target_link_libraries(ppam_example PRIVATE PPAM::ppam_shared glog::glog gflags::gflags)
    else()
        message(FATAL_ERROR "Cannot find target PPAM::ppam or PPAM::ppam_shared")
    endif()

    set(PPAM_EXAMPLE_SCRIPTS_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/scripts)
    set(BIN_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(JSON_DIR ${CMAKE_CURRENT_LIST_DIR}/json)
    set(LOG_DIR ${CMAKE_CURRENT_BINARY_DIR}/log)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/receiver_test.sh.in ${PPAM_EXAMPLE_SCRIPTS_INSTALL_DIR}/receiver_test.sh @ONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/receiver_with_preprocessing.sh.in ${PPAM_EXAMPLE_SCRIPTS_INSTALL_DIR}/receiver_with_preprocessing.sh @ONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/receiver_without_dp.sh.in ${PPAM_EXAMPLE_SCRIPTS_INSTALL_DIR}/receiver_without_dp.sh @ONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/sender_test.sh.in ${PPAM_EXAMPLE_SCRIPTS_INSTALL_DIR}/sender_test.sh @ONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/sender_with_preprocessing.sh.in ${PPAM_EXAMPLE_SCRIPTS_INSTALL_DIR}/sender_with_preprocessing.sh @ONLY)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/scripts/sender_without_dp.sh.in ${PPAM_EXAMPLE_SCRIPTS_INSTALL_DIR}/sender_without_dp.sh @ONLY)
endif()
