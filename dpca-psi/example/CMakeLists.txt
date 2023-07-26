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

project(DPCA_PSIExamples VERSION 0.2.0 LANGUAGES CXX)

# If not called from root CMakeLists.txt
if(NOT DEFINED DPCA_PSI_BUILD_EXAMPLE)
    set(DPCA_PSI_BUILD_EXAMPLE ON)

    # Import DPCA_PSI
    find_package(DPCA_PSI  0.2.0 EXACT REQUIRED)

    # Must define these variables and include macros
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
    set(DPCA_PSI_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
    set(THIRDPARTY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
    include(FetchContent)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_QUIET)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)
    include(DPCA_PSICustomMacros)
else()
    set(THIRDPARTY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../thirdparty)
endif()

if(NOT DEFINED DPCA_PSI_BUILD_DEPS)
    # [option] DPCA_PSI_BUILD_DEPS (default: ON)
    # Download and build missing dependencies, throw error if disabled.
    set(DPCA_PSI_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
    option(DPCA_PSI_BUILD_DEPS ${DPCA_PSI_BUILD_DEPS_OPTION_STR} ON)
endif()

if(DPCA_PSI_BUILD_EXAMPLE)
    # gflags::gflags
    set (GFLAGS_USE_TARGET_NAMESPACE TRUE)
    find_package(gflags 2.2.2 QUIET CONFIG)
    if(gflags_FOUND)
        message(STATUS "gflags: found")
    else()
        if(DPCA_PSI_BUILD_DEPS)
            message(STATUS "gflags: downloading ...")
            dpca_psi_fetch_thirdparty_content(ExternalGflags)
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
            if(DPCA_PSI_BUILD_DEPS)
                message(STATUS "glog: download ...")
                dpca_psi_fetch_thirdparty_content(ExternalGlog)
            else()
                message(FATAL_ERROR "glog: not found, please download and install manually")
            endif()
        endif()
    endif()

    # Add source files to example
    set(DPCA_PSI_EXAMPLE_FILES "")
    # Test files in this directory
    set(DPCA_PSI_EXAMPLE_FILES ${DPCA_PSI_EXAMPLE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/dpca_psi_example.cpp
    )

    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")
    add_executable(dpca_psi_example ${DPCA_PSI_EXAMPLE_FILES})

    if(TARGET DPCA_PSI::dpca_psi)
        target_link_libraries(dpca_psi_example PRIVATE DPCA_PSI::dpca_psi glog::glog gflags::gflags)
    elseif(TARGET DPCA_PSI::dpca_psi_shared)
        target_link_libraries(dpca_psi_example PRIVATE DPCA_PSI::dpca_psi_shared glog::glog gflags::gflags)
    else()
        message(FATAL_ERROR "Cannot find target DPCA_PSI::dpca_psi or DPCA_PSI::dpca_psi_shared")
    endif()

    set(DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/scripts)
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
endif()
