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

project(DPCA_PSITest VERSION 0.2.0 LANGUAGES CXX C)

# If not called from root CMakeLists.txt
if(NOT DEFINED DPCA_PSI_BUILD_TEST)
    set(DPCA_PSI_BUILD_TEST ON)

    find_package(DPCA_PSI 0.2.0 EXACT REQUIRED)

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

# if DPCA_PSI_BUILD_TEST is ON, use GoogleTest
if(DPCA_PSI_BUILD_TEST)
    find_package(GTest 1 CONFIG)
    if(GTest_FOUND)
        message(STATUS "GoogleTest: found")
    else()
        if(DPCA_PSI_BUILD_DEPS)
            message(STATUS "GoogleTest: downloading ...")
            dpca_psi_fetch_thirdparty_content(ExternalGTest)
            add_library(GTest::gtest ALIAS gtest)
        else()
            message(FATAL_ERROR "GoogleTest: not found, please download and install manually")
        endif()
    endif()

    # Add source files to test
    set(DPCA_PSI_TEST_FILES
        ${CMAKE_CURRENT_LIST_DIR}/common/csv_file_io_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crypto/aes_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crypto/ecc_cipher_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crypto/prng_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crypto/dp_sampling_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crypto/ipcl_paillier_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/network/two_channel_net_io_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dp_cardinality_psi_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_runner.cpp
    )

    # CMAKE_CXX_LINK_EXECUTABLE
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND DPCA_PSI_ENABLE_GCOV)
        set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -fprofile-arcs -ftest-coverage -lgcov")
    endif()

    add_executable(dpca_psi_test ${DPCA_PSI_TEST_FILES})

    if(TARGET DPCA_PSI::dpca_psi)
        target_link_libraries(dpca_psi_test PRIVATE DPCA_PSI::dpca_psi GTest::gtest)
    elseif(TARGET DPCA_PSI::dpca_psi_shared)
        target_link_libraries(dpca_psi_test PRIVATE DPCA_PSI::dpca_psi_shared GTest::gtest)
    else()
        message(FATAL_ERROR "Cannot find target DPCA_PSI::dpca_psi or DPCA_PSI::dpca_psi_shared")
    endif()

    # In Debug mode, enable AddressSanitizer (and LeakSanitizer) on Unix-like platforms.
    if(DPCA_PSI_DEBUG AND UNIX)
        # On macOS, only AddressSanitizer is enabled.
        # On Linux, LeakSanitizer is enabled by default.
        target_compile_options(dpca_psi_test PUBLIC -fsanitize=address)
        target_link_options(dpca_psi_test PUBLIC -fsanitize=address)
        if(NOT APPLE)
            message(STATUS "Sanitizers enabled: address, leak")
        else()
            message(STATUS "Sanitizers enabled: address")
        endif()
    endif()

    add_custom_target(test_report
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dpca_psi_test --gtest_output="xml:${CMAKE_CURRENT_BINARY_DIR}/../report/dpca_psi_test.xml"
        DEPENDS dpca_psi_test)
endif()
