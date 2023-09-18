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

project(MPC_DUALDPExamples VERSION 0.1.0 LANGUAGES CXX)

# If not called from root CMakeLists.txt
if(NOT DEFINED MPC_DUALDP_BUILD_EXAMPLE)
    set(MPC_DUALDP_BUILD_EXAMPLE ON)

    # Import MPC_DUALDP
    find_package(MPC_DualDP  0.1.0 EXACT REQUIRED)

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

if(MPC_DUALDP_BUILD_EXAMPLE)
    # Add source files to example
    set(MPC_DUALDP_EXAMPLE_FILES "")
    # Test files in this directory
    set(MPC_DUALDP_EXAMPLE_FILES ${MPC_DUALDP_EXAMPLE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/mpc_dualdp_example.cpp
    )

    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")
    add_executable(mpc_dualdp_example ${MPC_DUALDP_EXAMPLE_FILES})

    if(TARGET MPC_DualDP::mpc_dualdp)
        target_link_libraries(mpc_dualdp_example PRIVATE MPC_DualDP::mpc_dualdp)
    elseif(TARGET MPC_DualDP::mpc_dualdp_shared)
        target_link_libraries(mpc_dualdp_example PRIVATE MPC_DualDP::mpc_dualdp_shared)
    else()
        message(FATAL_ERROR "Cannot find target MPC_DualDP::mpc_dualdp or MPC_DualDP::mpc_dualdp_shared")
    endif()
endif()
