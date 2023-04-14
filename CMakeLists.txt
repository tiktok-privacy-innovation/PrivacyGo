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

#######################################################
# Project DPCA_PSI includes the following components: #
#   1. DPCA_PSI C++ library                           #
#   2. DPCA_PSI C++ examples                          #
#   3. DPCA_PSI C++ tests                             #
#######################################################

# [OPTION] CMAKE_BUILD_TYPE (DEFAULT: "Release")
# Select from Release, Debug, MiniSizeRel, or RelWithDebInfo.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS "Release" "Debug" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type (CMAKE_BUILD_TYPE): ${CMAKE_BUILD_TYPE}")

project(DPCA_PSI VERSION 0.1.0 LANGUAGES CXX)

########################
# Global configuration #
########################

# Extra modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Build position-independent-code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Make the install target depend on the all target
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY OFF)

# Required files and directories
include(GNUInstallDirs)

# Source Tree
set(DPCA_PSI_INCLUDES_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(DPCA_PSI_CONFIG_IN_FILENAME ${CMAKE_CURRENT_LIST_DIR}/cmake/DPCA_PSI_Config.cmake.in)

# Build tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(DPCA_PSI_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
set(DPCA_PSI_TARGETS_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/DPCA_PSITargets.cmake)
set(DPCA_PSI_CONFIG_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/DPCA_PSIConfig.cmake)
set(DPCA_PSI_CONFIG_VERSION_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/DPCA_PSIConfigVersion.cmake)

# Install
set(DPCA_PSI_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/DPCA_PSI-${DPCA_PSI_VERSION_MAJOR}.${DPCA_PSI_VERSION_MINOR})
set(DPCA_PSI_INCLUDES_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/DPCA_PSI-${DPCA_PSI_VERSION_MAJOR}.${DPCA_PSI_VERSION_MINOR})

add_compile_options(-msse4.2 -maes -O2  -Wno-ignored-attributes -mavx)

# For easier adding of CXX compiler flags
include(CheckCXXCompilerFlag)
function(enable_cxx_compiler_flag_if_supported flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
    if(flag_already_set EQUAL -1)
        message(STATUS "Adding CXX compiler flag: ${flag} ...")
        check_cxx_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    enable_cxx_compiler_flag_if_supported("-Wall")
    enable_cxx_compiler_flag_if_supported("-Wextra")
    enable_cxx_compiler_flag_if_supported("-Wconversion")
    enable_cxx_compiler_flag_if_supported("-Wshadow")
    enable_cxx_compiler_flag_if_supported("-pedantic")
endif()

# Add source files to library and header files to install
set(DPCA_PSI_SOURCE_FILES "")
set(DPCA_PSI_TEST_FILES "")
add_subdirectory(src)

set(DPCA_PSI_BUILD_TESTS ON)
set(DPCA_PSI_BUILD_EXAMPLES ON)

#########################
# External dependencies #
#########################

include(FetchContent)
mark_as_advanced(FETCHCONTENT_BASE_DIR)
mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
mark_as_advanced(FETCHCONTENT_QUIET)

# Include a file to fetch thirdparty content
macro(fetch_thirdparty_content content_file)
    set(FETCHCONTENT_BASE_DIR_OLD ${FETCHCONTENT_BASE_DIR})
    set(FETCHCONTENT_BASE_DIR ${DPCA_PSI_THIRDPARTY_DIR} CACHE STRING "" FORCE)
    include(${content_file})
    set(FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR_OLD} CACHE STRING "" FORCE)
    unset(FETCHCONTENT_BASE_DIR_OLD)
endmacro()

# Threads::Threads
if(NOT TARGET Threads::Threads)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads QUIET REQUIRED)
    if(NOT Threads_FOUND)
        message(FATAL_ERROR "Threads: not found")
    else()
        message(STATUS "Threads: found")
    endif()
endif()

# OpenSSL::Crypto
if(NOT TARGET OpenSSL::Crypto)
    find_package(OpenSSL QUIET REQUIRED)
    if(NOT OpenSSL_FOUND)
        message(FATAL_ERROR "OpenSSL: not found")
    else()
        message(STATUS "OpenSSL: found")
    endif()
endif()
include(external_openssl)

# nlohmann_json::nlohmann_json
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json 3 QUIET REQUIRED)
    if(nlohmann_json_FOUND)
        message(STATUS "nlohmann_json: found")
    else()
        message(STATUS "nlohmann_json: not found")
    endif()
endif()

# gflags::gflags
if(NOT TARGET gflags::gflags)
    find_package(gflags 2.2.2 QUIET CONFIG)
    if(gflags_FOUND)
        message(STATUS "gflags: found")
    else()
        message(STATUS "gflags: not found, auto fetching...")
        fetch_thirdparty_content(external_gflags)
    endif()
endif()

# glog::glog
if(NOT TARGET glog::glog)
    find_package(glog 0.4.0 QUIET CONFIG)
    if(glog_FOUND)
        message(STATUS "glog: found")
    else()
        message(STATUS "glog: not found, auto fetching...")
        fetch_thirdparty_content(external_glog)
    endif()
endif()

# OpenMP::OpenMP_CXX
if(NOT TARGET OpenMP::OpenMP_CXX)
    find_package(OpenMP)
    if(NOT OpenMP_FOUND)
        message(FATAL_ERROR "OpenMP: not found")
    else()
        message(STATUS "OpenMP: found")
    endif()
endif()

# IPCL::ipcl
if(NOT TARGET IPCL::ipcl OR NOT TARGET ipcl)
    find_package(IPCL QUIET REQUIRED)
    if(NOT IPCL_FOUND)
        message(FATAL_ERROR "IPCL: not found")
    else()
        message(STATUS "IPCL: found")
        if (TARGET ipcl AND NOT TARGET IPCL::ipcl)
            add_library(IPCL::ipcl ALIAS ipcl)
        endif()
    endif()
endif()

# GTest::gtest
if(DPCA_PSI_BUILD_TESTS)
    if(NOT TARGET GTest::gtest)
        find_package(GTest 1 CONFIG)
        if(GTest_FOUND)
            message(STATUS "GoogleTest: found")
        else()
            message(STATUS "GoogleTest: not found, auto fetching...")
            fetch_thirdparty_content(external_gtest)
        endif()
    endif()
endif()

# CMAKE_CXX_LINK_EXECUTABLE
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")

########################
# DPCA_PSI C++ library #
########################

# [OPTION] BUILD_SHARED_LIBS (DEFAULT: OFF)
# Build a shared library if set to ON.
set(BUILD_SHARED_LIBS_STR "Build shared library")
option(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_STR} OFF)
message(STATUS "BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")

# Build only a static library
if(NOT BUILD_SHARED_LIBS)
    add_library(dpca_psi STATIC ${DPCA_PSI_SOURCE_FILES})
    set_target_properties(dpca_psi PROPERTIES OUTPUT_NAME dpca_psi)
    set_target_properties(dpca_psi PROPERTIES VERSION ${DPCA_PSI_VERSION})
    target_compile_features(dpca_psi PUBLIC cxx_std_14)
    target_include_directories(dpca_psi PUBLIC
        $<BUILD_INTERFACE:${DPCA_PSI_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${DPCA_PSI_INCLUDES_INSTALL_DIR}>)
    target_link_libraries(dpca_psi PUBLIC nlohmann_json::nlohmann_json IPCL::ipcl)
    target_link_libraries(dpca_psi PRIVATE crypto glog::glog)
    install(TARGETS dpca_psi
        EXPORT DPCA_PSITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    add_library(DPCA_PSI::dpca_psi ALIAS dpca_psi)
# Build only a shared library
else()
    add_library(dpca_psi_shared SHARED ${DPCA_PSI_SOURCE_FILES})
    set_target_properties(dpca_psi_shared PROPERTIES OUTPUT_NAME dpca_psi)
    set_target_properties(dpca_psi_shared PROPERTIES VERSION ${DPCA_PSI_VERSION})
    set_target_properties(dpca_psi_shared PROPERTIES SOVERSION ${DPCA_PSI_VERSION_MAJOR}.${DPCA_PSI_VERSION_MINOR})
    target_compile_features(dpca_psi_shared PUBLIC cxx_std_14)
    target_include_directories(dpca_psi_shared PUBLIC
        $<BUILD_INTERFACE:${DPCA_PSI_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${DPCA_PSI_INCLUDES_INSTALL_DIR}>)
    target_link_libraries(dpca_psi_shared PUBLIC nlohmann_json::nlohmann_json IPCL::ipcl)
    target_link_libraries(dpca_psi_shared PRIVATE crypto glog::glog)
    install(TARGETS dpca_psi_shared
        EXPORT DPCA_PSITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    add_library(DPCA_PSI::dpca_psi_shared ALIAS dpca_psi_shared)
endif()

#################################
# Installation and CMake config #
#################################

# Create the CMake config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${DPCA_PSI_CONFIG_IN_FILENAME} ${DPCA_PSI_CONFIG_FILENAME}
    INSTALL_DESTINATION ${DPCA_PSI_CONFIG_INSTALL_DIR}
)

# Install the export
install(
    EXPORT DPCA_PSITargets
    NAMESPACE DPCA_PSI::
    DESTINATION ${DPCA_PSI_CONFIG_INSTALL_DIR})

# Version file; we require exact version match for downstream
write_basic_package_version_file(
    ${DPCA_PSI_CONFIG_VERSION_FILENAME}
    VERSION ${DPCA_PSI_VERSION}
    COMPATIBILITY SameMinorVersion)

# Install config and module files
install(
    FILES
        ${DPCA_PSI_CONFIG_FILENAME}
        ${DPCA_PSI_CONFIG_VERSION_FILENAME}
    DESTINATION ${DPCA_PSI_CONFIG_INSTALL_DIR})

# We export DPCA_PSITargets from the build tree so it can be used by other projects
# without requiring an install.
export(
    EXPORT DPCA_PSITargets
    NAMESPACE DPCA_PSI::
    FILE ${DPCA_PSI_TARGETS_FILENAME})

######################
# DPCA_PSI C++ tests #
######################

if(DPCA_PSI_BUILD_TESTS)
    add_library(GTest::gtest ALIAS gtest)
    add_executable(dpca_psi_tests ${DPCA_PSI_TEST_FILES})
    if(TARGET DPCA_PSI::dpca_psi)
        target_link_libraries(dpca_psi_tests PRIVATE DPCA_PSI::dpca_psi GTest::gtest)
    elseif(TARGET DPCA_PSI::dpca_psi_shared)
        target_link_libraries(dpca_psi_tests PRIVATE DPCA_PSI::dpca_psi_shared GTest::gtest)
    else()
        message(FATAL_ERROR "Cannot find target DPCA_PSI::dpca_psi or DPCA_PSI::dpca_psi_shared")
    endif()

    # In Debug mode, enable AddressSanitizer (and LeakSanitizer) on Unix-like platforms.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND UNIX)
        # On macOS, only AddressSanitizer is enabled.
        # On Linux, LeakSanitizer is enabled by default.
        target_compile_options(dpca_psi_tests PUBLIC -fsanitize=address)
        target_link_options(dpca_psi_tests PUBLIC -fsanitize=address)
        if(NOT APPLE)
            message(STATUS "Sanitizers enabled: address, leak")
        else()
            message(STATUS "Sanitizers enabled: address")
        endif()
    endif()
endif()

#########################
# DPCA_PSI C++ examples #
#########################

if(DPCA_PSI_BUILD_EXAMPLES)
    set(DPCA_PSI_EXAMPLE_SCRIPTS_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/example/scripts)
    add_subdirectory(example)
endif()
