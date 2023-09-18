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

project(MPC_DUALDPTest VERSION 0.1.0 LANGUAGES CXX C)

# If not called from root CMakeLists.txt
if(NOT DEFINED MPC_DUALDP_BUILD_TEST)
    set(MPC_DUALDP_BUILD_TEST ON)

    find_package(MPC_DUALDP 0.1.0 EXACT REQUIRED)

    add_compile_options(-msse4.2 -maes -mavx -Wno-ignored-attributes)

    # Must define these variables and include macros
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
    set(MPC_DUALDP_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
    set(THIRDPARTY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
    include(FetchContent)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_QUIET)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)
    include(MPC_DualDPCustomMacros)
else()
    set(THIRDPARTY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../thirdparty)
endif()

if(NOT DEFINED MPC_DUALDP_BUILD_DEPS)
    # [option] MPC_DUALDP_BUILD_DEPS (default: ON)
    # Download and build missing dependencies, throw error if disabled.
    set(MPC_DUALDP_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
    option(MPC_DUALDP_BUILD_DEPS ${MPC_DUALDP_BUILD_DEPS_OPTION_STR} ON)
endif()

# if MPC_DUALDP_BUILD_TEST is ON, use GoogleTest
if(MPC_DUALDP_BUILD_TEST)
    find_package(GTest 1 CONFIG)
    if(GTest_FOUND)
        message(STATUS "GoogleTest: found")
    else()
        if(MPC_DUALDP_BUILD_DEPS)
            message(STATUS "GoogleTest: downloading ...")
            mpc_dualdp_fetch_thirdparty_content(ExternalGTest)
            add_library(GTest::gtest ALIAS gtest)
        else()
            message(FATAL_ERROR "GoogleTest: not found, please download and install manually")
        endif()
    endif()

    # Add source files to test
    set(MPC_DUALDP_TEST_FILES
        ${CMAKE_CURRENT_LIST_DIR}/mpc_dualdp_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_runner.cpp
    )

    # CMAKE_CXX_LINK_EXECUTABLE
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND MPC_DUALDP_ENABLE_GCOV)
        set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -fprofile-arcs -ftest-coverage -lgcov")
    endif()

    add_executable(mpc_dualdp_test ${MPC_DUALDP_TEST_FILES})

    if(TARGET MPC_DualDP::mpc_dualdp)
        target_link_libraries(mpc_dualdp_test PRIVATE MPC_DualDP::mpc_dualdp GTest::gtest)
    elseif(TARGET MPC_DualDP::mpc_dualdp_shared)
        target_link_libraries(mpc_dualdp_test PRIVATE MPC_DualDP::mpc_dualdp_shared GTest::gtest)
    else()
        message(FATAL_ERROR "Cannot find target MPC_DualDP::mpc_dualdp or MPC_DualDP::mpc_dualdp_shared")
    endif()

    # In Debug mode, enable AddressSanitizer (and LeakSanitizer) on Unix-like platforms.
    if(MPC_DUALDP_DEBUG AND UNIX)
        # On macOS, only AddressSanitizer is enabled.
        # On Linux, LeakSanitizer is enabled by default.
        target_compile_options(mpc_dualdp_test PUBLIC -fsanitize=address)
        target_link_options(mpc_dualdp_test PUBLIC -fsanitize=address)
        if(NOT APPLE)
            message(STATUS "Sanitizers enabled: address, leak")
        else()
            message(STATUS "Sanitizers enabled: address")
        endif()
    endif()

    add_custom_target(test_report
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mpc_dualdp_test --gtest_output="xml:${CMAKE_CURRENT_BINARY_DIR}/../report/mpc_dualdp_test.xml"
        DEPENDS mpc_dualdp_test)
endif()
