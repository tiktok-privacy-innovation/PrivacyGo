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

project(DPCA_PSI VERSION 0.2.0 LANGUAGES CXX)

########################
# Global configuration #
########################

# CMake modules
include(CMakeDependentOption)
include(CMakePushCheckState)
include(CheckIncludeFiles)
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)

# Extra modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(DPCA_PSICustomMacros)

# In Debug mode, define DPCA_PSI_DEBUG.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DPCA_PSI_DEBUG ON)
else()
    set(DPCA_PSI_DEBUG OFF)
endif()
message(STATUS "DPCA_PSI debug mode: ${DPCA_PSI_DEBUG}")

# In Debug mode, enable extra compiler flags.
include(EnableDebugFlags)

# Build position-independent-code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Make the install target depend on the all target
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY OFF)

# [OPTION] DPCA_PSI_USE_CXX17 (default: OFF)
# Use C++17, use C++14 otherwise.
set(DPCA_PSI_USE_CXX17_OPTION_STR "Use C++17")
option(DPCA_PSI_USE_CXX17 ${DPCA_PSI_USE_CXX17_OPTION_STR} OFF)
message(STATUS "DPCA_PSI_USE_CXX17: ${DPCA_PSI_USE_CXX17}")
# Enable features from C++17 if available, disable features if set to OFF.
include(EnableCXX17)

# Required files and directories
include(GNUInstallDirs)

# Runtime path
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Source Tree
set(DPCA_PSI_INCLUDES_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(DPCA_PSI_CONFIG_IN_FILENAME ${CMAKE_CURRENT_LIST_DIR}/cmake/DPCA_PSIConfig.cmake.in)
set(DPCA_PSI_CONFIG_H_IN_FILENAME ${DPCA_PSI_INCLUDES_DIR}/dpca-psi/common/config.h.in)

# Build tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(DPCA_PSI_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
set(DPCA_PSI_TARGETS_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/DPCA_PSITargets.cmake)
set(DPCA_PSI_CONFIG_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/DPCA_PSIConfig.cmake)
set(DPCA_PSI_CONFIG_VERSION_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/DPCA_PSIConfigVersion.cmake)
set(DPCA_PSI_CONFIG_H_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/src/dpca-psi/common/config.h)

# Install
set(DPCA_PSI_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/DPCA_PSI-${DPCA_PSI_VERSION_MAJOR}.${DPCA_PSI_VERSION_MINOR})
set(DPCA_PSI_INCLUDES_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/DPCA_PSI-${DPCA_PSI_VERSION_MAJOR}.${DPCA_PSI_VERSION_MINOR})
set(DPCA_PSI_THIRDPARTY_INCLUDES_INSTALL_DIR ${DPCA_PSI_INCLUDES_INSTALL_DIR}/thirdparty)

# Supported target operating systems are Linux and macOS.
if (NOT DEFINED LINUX)
    if (UNIX AND NOT APPLE AND NOT CYGWIN AND NOT MINGW)
        set(LINUX ON)
    endif()
endif()
if (UNIX AND APPLE)
    set(MACOS ON)
endif()
if (NOT LINUX AND NOT MACOS)
    message(FATAL_ERROR "Supported target operating systems are Linux and macOS")
endif()

# Only support x86_64 and arm64
set(CMAKE_REQUIRED_QUIET_OLD ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ON)
check_cxx_source_runs("
    #if defined(__aarch64__)
        int main() {
            return 0;
        }
    #else
        #error
    #endif
    "
    DPCA_PSI_ARM64
)
check_cxx_source_runs("
    #if defined(__amd64)
        int main() {
            return 0;
        }
    #else
        #error
    #endif
    "
    DPCA_PSI_AMD64
)
set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_OLD})
if (NOT DPCA_PSI_AMD64 AND NOT DPCA_PSI_ARM64)
    message(FATAL_ERROR "Supported target architectures are x86_64 and arm64")
endif()

add_compile_options(-msse4.2 -maes -mavx -Wno-ignored-attributes)

# Enable test coverage
set(DPCA_PSI_ENABLE_GCOV_STR "Enable gcov")
option(DPCA_PSI_ENABLE_GCOV ${DPCA_PSI_ENABLE_GCOV_STR} OFF)
message(STATUS "DPCA_PSI_ENABLE_GCOV: ${DPCA_PSI_ENABLE_GCOV}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND DPCA_PSI_ENABLE_GCOV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -fprofile-arcs -ftest-coverage -lgcov")
endif()

set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")

#########################
# External dependencies #
#########################

# [OPTION] DPCA_PSI_BUILD_DEPS (DEFAULT: ON)
# Download and build dependencies if set to ON.
# Look for dependencies using find_package, otherwise.
set(DPCA_PSI_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
option(DPCA_PSI_BUILD_DEPS ${DPCA_PSI_BUILD_DEPS_OPTION_STR} ON)
message(STATUS "DPCA_PSI_BUILD_DEPS: ${DPCA_PSI_BUILD_DEPS}")

if(DPCA_PSI_BUILD_DEPS)
    include(FetchContent)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_QUIET)
endif()

# Require Threads::Threads
if(NOT TARGET Threads::Threads)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    if(NOT Threads_FOUND)
        message(FATAL_ERROR "Threads: not found, please download and install manually")
    else()
        message(STATUS "Threads: found")
    endif()
endif()

# OpenMP::OpenMP_CXX
if(NOT TARGET OpenMP::OpenMP_CXX)
    find_package(OpenMP REQUIRED)
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
        message(FATAL_ERROR "IPCL: not found, please download and install manually")
    else()
        message(STATUS "IPCL: found")
        if (TARGET ipcl AND NOT TARGET IPCL::ipcl)
            add_library(IPCL::ipcl ALIAS ipcl)
        endif()
    endif()
endif()

# OpenSSL::Crypto
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL: found")
    set(openssl "OpenSSL::Crypto")
else()
    if(DPCA_PSI_BUILD_DEPS)
        message(STATUS "OpenSSL: download ...")
        dpca_psi_fetch_thirdparty_content(ExternalOpenSSL)
        set(openssl "Crypto")
        set(DPCA_PSI_BUILD_OPENSSL TRUE CACHE BOOL "" FORCE)
    else()
        message(FATAL_ERROR "OpenSSL: not found, please download and install manually")
    endif()
endif()

# nlohmann_json::nlohmann_json
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json 3 QUIET REQUIRED)
    if(nlohmann_json_FOUND)
        message(STATUS "nlohmann_json: found")
    else()
        message(STATUS "nlohmann_json: not found, please download and install manually")
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
            set(DPCA_PSI_BUILD_GLOG TRUE CACHE BOOL "" FORCE)
        else()
            message(FATAL_ERROR "glog: not found, please download and install manually")
        endif()
    endif()
endif()

########################
# DPCA_PSI C++ library #
########################

# [OPTION] DPCA_PSI_BUILD_SHARED_LIBS (DEFAULT: OFF)
# Build a shared library if set to ON.
set(DPCA_PSI_BUILD_SHARED_LIBS_STR "Build shared library")
option(DPCA_PSI_BUILD_SHARED_LIBS ${DPCA_PSI_BUILD_SHARED_LIBS_STR} OFF)
message(STATUS "DPCA_PSI_BUILD_SHARED_LIBS: ${DPCA_PSI_BUILD_SHARED_LIBS}")

# Add source files to library and header files to install
set(DPCA_PSI_SOURCE_FILES "")
add_subdirectory(src/dpca-psi)

# Create the config file
configure_file(${DPCA_PSI_CONFIG_H_IN_FILENAME} ${DPCA_PSI_CONFIG_H_FILENAME})
install(
    FILES ${DPCA_PSI_CONFIG_H_FILENAME}
    DESTINATION ${DPCA_PSI_INCLUDES_INSTALL_DIR}/dpca-psi/common)

# Build only a static library
if(NOT DPCA_PSI_BUILD_SHARED_LIBS)
    add_library(dpca_psi STATIC ${DPCA_PSI_SOURCE_FILES})
    if(DPCA_PSI_USE_CXX17)
        target_compile_features(dpca_psi PUBLIC cxx_std_17)
    else()
        target_compile_features(dpca_psi PUBLIC cxx_std_14)
    endif()
    target_include_directories(dpca_psi PUBLIC
        $<BUILD_INTERFACE:${DPCA_PSI_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${DPCA_PSI_INCLUDES_INSTALL_DIR}>)
    target_include_directories(dpca_psi PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>)
    set_target_properties(dpca_psi PROPERTIES OUTPUT_NAME dpca_psi-${DPCA_PSI_VERSION_MAJOR}.${DPCA_PSI_VERSION_MINOR})
    set_target_properties(dpca_psi PROPERTIES VERSION ${DPCA_PSI_VERSION})

    target_link_libraries(dpca_psi PUBLIC nlohmann_json::nlohmann_json IPCL::ipcl)
    target_link_libraries(dpca_psi PRIVATE OpenMP::OpenMP_CXX Threads::Threads)

    if(DPCA_PSI_BUILD_GLOG)
        add_dependencies(dpca_psi glog::glog)
        target_include_directories(dpca_psi PRIVATE
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:glog::glog,INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${DPCA_PSI_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
        dpca_psi_combine_archives(dpca_psi glog::glog)
        set(DPCA_PSI_CARRY_GLOG TRUE)
    else()
        target_link_libraries(dpca_psi PRIVATE glog::glog)
        set(DPCA_PSI_CARRY_GLOG FALSE)
    endif()

    if(DPCA_PSI_BUILD_OPENSSL)
        add_dependencies(dpca_psi ${openssl})
        target_include_directories(dpca_psi PUBLIC
            $<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${DPCA_PSI_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
        dpca_psi_combine_archives(dpca_psi ${openssl})
        set(DPCA_PSI_CARRY_OPENSSL TRUE)
    else()
        target_link_libraries(dpca_psi PUBLIC ${openssl})
        set(DPCA_PSI_CARRY_OPENSSL FALSE)
    endif()

    install(TARGETS dpca_psi
        EXPORT DPCA_PSITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Build only a shared library
else()
    add_library(dpca_psi_shared SHARED ${DPCA_PSI_SOURCE_FILES})
    if(DPCA_PSI_USE_CXX17)
        target_compile_features(dpca_psi_shared PUBLIC cxx_std_17)
    else()
        target_compile_features(dpca_psi_shared PUBLIC cxx_std_14)
    endif()
    target_include_directories(dpca_psi_shared PUBLIC
        $<BUILD_INTERFACE:${DPCA_PSI_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${DPCA_PSI_INCLUDES_INSTALL_DIR}>)
    target_include_directories(dpca_psi_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>)
    set_target_properties(dpca_psi_shared PROPERTIES OUTPUT_NAME dpca_psi)
    set_target_properties(dpca_psi_shared PROPERTIES VERSION ${DPCA_PSI_VERSION})
    set_target_properties(dpca_psi_shared PROPERTIES SOVERSION ${DPCA_PSI_VERSION_MAJOR}.${DPCA_PSI_VERSION_MINOR})

    target_link_libraries(dpca_psi_shared PUBLIC nlohmann_json::nlohmann_json IPCL::ipcl)
    target_link_libraries(dpca_psi_shared PRIVATE OpenMP::OpenMP_CXX Threads::Threads)

    if(DPCA_PSI_BUILD_GLOG)
        target_include_directories(dpca_psi_shared PRIVATE
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:glog::glog,INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${DPCA_PSI_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
        set(DPCA_PSI_CARRY_GLOG TRUE)
    else()
        set(DPCA_PSI_CARRY_GLOG FALSE)
    endif()
    target_link_libraries(dpca_psi_shared PRIVATE glog::glog)

    if(DPCA_PSI_BUILD_OPENSSL)
        target_include_directories(dpca_psi_shared PUBLIC $<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>)
        target_include_directories(dpca_psi_shared PUBLIC $<INSTALL_INTERFACE:${DPCA_PSI_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(dpca_psi_shared PUBLIC ${openssl})
    set(DPCA_PSI_CARRY_OPENSSL FALSE)

    install(TARGETS dpca_psi_shared
        EXPORT DPCA_PSITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Add standard alias targets for DPCA_PSI::dpca_psi and DPCA_PSI::dpca_psi_shared
if(TARGET dpca_psi)
    add_library(DPCA_PSI::dpca_psi ALIAS dpca_psi)
endif()
if(TARGET dpca_psi_shared)
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

# Install header files of dependencies if DPCA_PSI_BUILD_DEPS is ON
if(DPCA_PSI_BUILD_DEPS)
    # Insert dependencies here
    if(DPCA_PSI_BUILD_GLOG)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${glog_BINARY_DIR} -t install)")
    endif()
    if(DPCA_PSI_BUILD_OPENSSL)
        install(
            FILES ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY}
            DESTINATION ${CMAKE_INSTALL_LIBDIR})
        install(
            DIRECTORY ${OPENSSL_INCLUDE_DIR}
            DESTINATION ${DPCA_PSI_THIRDPARTY_INCLUDES_INSTALL_DIR}/openssl)
    endif()
endif()

#########################
# DPCA_PSI C++ examples #
#########################

# [option] DPCA_PSI_BUILD_EXAMPLE
set(DPCA_PSI_BUILD_EXAMPLE_OPTION_STR "Build C++ example for DPCA_PSI")
option(DPCA_PSI_BUILD_EXAMPLE ${DPCA_PSI_BUILD_EXAMPLE_OPTION_STR} ON)
message(STATUS "DPCA_PSI_BUILD_EXAMPLE: ${DPCA_PSI_BUILD_EXAMPLE}")

if(DPCA_PSI_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()

######################
# DPCA_PSI C++ tests #
######################

# [option] DPCA_PSI_BUILD_TEST
set(DPCA_PSI_BUILD_TEST_OPTION_STR "Build C++ test for DPCA_PSI")
option(DPCA_PSI_BUILD_TEST ${DPCA_PSI_BUILD_TEST_OPTION_STR} ON)
message(STATUS "DPCA_PSI_BUILD_TEST: ${DPCA_PSI_BUILD_TEST}")

if(DPCA_PSI_BUILD_TEST)
    add_subdirectory(test)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND DPCA_PSI_ENABLE_GCOV)
        add_custom_target(test_coverage
            COMMAND gcovr -r ${CMAKE_CURRENT_LIST_DIR} -f \"src\" -e \".+\(test\\.cpp\)\" --xml-pretty -o "${CMAKE_CURRENT_BINARY_DIR}/report/coverage.xml"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    endif()
endif()
