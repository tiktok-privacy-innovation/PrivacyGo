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

###################################################
# Project PPAM includes the following components: #
#   1. PPAM C++ library                           #
#   2. PPAM C++ examples                          #
#   3. PPAM C++ tests                             #
###################################################

# [OPTION] CMAKE_BUILD_TYPE (DEFAULT: "Release")
# Select from Release, Debug, MiniSizeRel, or RelWithDebInfo.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS "Release" "Debug" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type (CMAKE_BUILD_TYPE): ${CMAKE_BUILD_TYPE}")

project(PPAM VERSION 0.1.0 LANGUAGES CXX)

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
include(PPAMCustomMacros)

# In Debug mode, define PPAM_DEBUG.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PPAM_DEBUG ON)
else()
    set(PPAM_DEBUG OFF)
endif()
message(STATUS "PPAM debug mode: ${PPAM_DEBUG}")

# In Debug mode, enable extra compiler flags.
include(EnableDebugFlags)

# Build position-independent-code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Make the install target depend on the all target
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY OFF)

# [OPTION] PPAM_USE_CXX17 (default: OFF)
# Use C++17, use C++14 otherwise.
set(PPAM_USE_CXX17_OPTION_STR "Use C++17")
option(PPAM_USE_CXX17 ${PPAM_USE_CXX17_OPTION_STR} OFF)
message(STATUS "PPAM_USE_CXX17: ${PPAM_USE_CXX17}")
# Enable features from C++17 if available, disable features if set to OFF.
include(EnableCXX17)

# Required files and directories
include(GNUInstallDirs)

# Runtime path
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Source Tree
set(PPAM_INCLUDES_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(PPAM_CONFIG_IN_FILENAME ${CMAKE_CURRENT_LIST_DIR}/cmake/PPAMConfig.cmake.in)
set(PPAM_CONFIG_H_IN_FILENAME ${PPAM_INCLUDES_DIR}/ppam/utils/config.h.in)

# Build tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(PPAM_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
set(PPAM_TARGETS_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/PPAMTargets.cmake)
set(PPAM_CONFIG_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/PPAMConfig.cmake)
set(PPAM_CONFIG_VERSION_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/PPAMConfigVersion.cmake)
set(PPAM_CONFIG_H_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/src/ppam/utils/config.h)

# Install
set(PPAM_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/PPAM-${PPAM_VERSION_MAJOR}.${PPAM_VERSION_MINOR})
set(PPAM_INCLUDES_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/PPAM-${PPAM_VERSION_MAJOR}.${PPAM_VERSION_MINOR})
set(PPAM_THIRDPARTY_INCLUDES_INSTALL_DIR ${PPAM_INCLUDES_INSTALL_DIR}/thirdparty)

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
    PPAM_ARM64
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
    PPAM_AMD64
)
set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_OLD})
if (NOT PPAM_AMD64 AND NOT PPAM_ARM64)
    message(FATAL_ERROR "Supported target architectures are x86_64 and arm64")
endif()

add_compile_options(-msse4.2 -maes -mavx -Wno-ignored-attributes)

# Enable test coverage
set(PPAM_ENABLE_GCOV_STR "Enable gcov")
option(PPAM_ENABLE_GCOV ${PPAM_ENABLE_GCOV_STR} OFF)
message(STATUS "PPAM_ENABLE_GCOV: ${PPAM_ENABLE_GCOV}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND PPAM_ENABLE_GCOV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -fprofile-arcs -ftest-coverage -lgcov")
endif()

set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")

#########################
# External dependencies #
#########################

# [OPTION] PPAM_BUILD_DEPS (DEFAULT: ON)
# Download and build dependencies if set to ON.
# Look for dependencies using find_package, otherwise.
set(PPAM_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
option(PPAM_BUILD_DEPS ${PPAM_BUILD_DEPS_OPTION_STR} ON)
message(STATUS "PPAM_BUILD_DEPS: ${PPAM_BUILD_DEPS}")

if(PPAM_BUILD_DEPS)
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

# OpenSSL::Crypto
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL: found")
    set(openssl "OpenSSL::Crypto")
else()
    if(PPAM_BUILD_DEPS)
        message(STATUS "OpenSSL: download ...")
        ppam_fetch_thirdparty_content(ExternalOpenSSL)
        set(openssl "Crypto")
        set(PPAM_BUILD_OPENSSL TRUE CACHE BOOL "" FORCE)
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

# eigen
if(NOT TARGET Eigen3::Eigen)
    find_package(Eigen3 3.4.0 QUIET CONFIG)
    if(Eigen3_FOUND)
        message(STATUS "Eigen: found")
    else()
        if(PPAM_BUILD_DEPS)
            message(STATUS "Eigen: download ...")
            ppam_fetch_thirdparty_content(ExternalEigen)
            set(PPAM_BUILD_EIGEN TRUE CACHE BOOL "" FORCE)
        else()
            message(FATAL_ERROR "Eigen: not found, please download and install manually")
        endif()
    endif()
endif()

# [OPTION] PPAM_BUILD_SHARED_LIBS (DEFAULT: OFF)
# Build a shared library if set to ON.
set(PPAM_BUILD_SHARED_LIBS_STR "Build shared library")
option(PPAM_BUILD_SHARED_LIBS ${PPAM_BUILD_SHARED_LIBS_STR} OFF)
message(STATUS "PPAM_BUILD_SHARED_LIBS: ${PPAM_BUILD_SHARED_LIBS}")

# DPCA_PSI:dpca_psi
if(NOT TARGET DPCA_PSI::dpca_psi AND NOT TARGET DPCA_PSI::dpca_psi_shared)
    find_package(DPCA_PSI 0.1 QUIET CONFIG)
    if(DPCA_PSI_FOUND)
        message(STATUS "DPCA_PSI found")
        if(DPCA_PSI_STATIC_FOUND)
            set(dpca_psi "DPCA_PSI::dpca_psi")
        else()
            set(dpca_psi "DPCA_PSI::dpca_psi_shared")
        endif()
    else()
        if(PPAM_BUILD_DEPS)
            message(STATUS "DPCA_PSI fetching internal...")
            include(DPCA_PSI)
            if(TARGET dpca_psi)
                set(dpca_psi "DPCA_PSI::dpca_psi")
            else()
                set(dpca_psi "DPCA_PSI::dpca_psi_shared")
            endif()
            set(PPAM_BUILD_DPCA_PSI TRUE CACHE BOOL "" FORCE)
        else()
            message(FATAL_ERROR "DPCA_PSI: not found, please download and install manually")
        endif()
    endif()
endif()

####################
# PPAM C++ library #
####################

# Add source files to library and header files to install
set(PPAM_SOURCE_FILES "")
add_subdirectory(src/ppam)

# Create the config file
configure_file(${PPAM_CONFIG_H_IN_FILENAME} ${PPAM_CONFIG_H_FILENAME})
install(
    FILES ${PPAM_CONFIG_H_FILENAME}
    DESTINATION ${PPAM_INCLUDES_INSTALL_DIR}/ppam/utils)

# Build only a static library
if(NOT PPAM_BUILD_SHARED_LIBS)
    add_library(ppam STATIC ${PPAM_SOURCE_FILES})
    if(PPAM_USE_CXX17)
        target_compile_features(ppam PUBLIC cxx_std_17)
    else()
        target_compile_features(ppam PUBLIC cxx_std_14)
    endif()
    target_include_directories(ppam PUBLIC
        $<BUILD_INTERFACE:${PPAM_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${PPAM_INCLUDES_INSTALL_DIR}>)
    target_include_directories(ppam PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>)
    set_target_properties(ppam PROPERTIES OUTPUT_NAME ppam-${PPAM_VERSION_MAJOR}.${PPAM_VERSION_MINOR})
    set_target_properties(ppam PROPERTIES VERSION ${PPAM_VERSION})

    target_link_libraries(ppam PUBLIC nlohmann_json::nlohmann_json m)
    target_link_libraries(ppam PRIVATE Threads::Threads)

    if(PPAM_BUILD_DPCA_PSI)
        add_dependencies(ppam ${dpca_psi})
        target_include_directories(ppam PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${dpca_psi},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${DPCA_PSI_INCLUDES_INSTALL_DIR}>)
    endif()
    set(PPAM_CARRY_DPCA_PSI FALSE)
    target_link_libraries(ppam PUBLIC ${dpca_psi})

    if(PPAM_BUILD_EIGEN)
        add_dependencies(ppam Eigen3::Eigen)
        target_include_directories(ppam PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${PPAM_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
        set(PPAM_CARRY_EIGEN TRUE)
    else()
        target_link_libraries(ppam PUBLIC Eigen3::Eigen)
        set(PPAM_CARRY_EIGEN FALSE)
    endif()

    if(PPAM_BUILD_OPENSSL)
        add_dependencies(ppam ${openssl})
        target_include_directories(ppam PUBLIC
            $<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${PPAM_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
        ppam_combine_archives(ppam ${openssl})
        set(PPAM_CARRY_OPENSSL TRUE)
    else()
        target_link_libraries(ppam PUBLIC ${openssl})
        set(PPAM_CARRY_OPENSSL FALSE)
    endif()

    install(TARGETS ppam
        EXPORT PPAMTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Build only a shared library
else()
    add_library(ppam_shared SHARED ${PPAM_SOURCE_FILES})
    if(PPAM_USE_CXX17)
        target_compile_features(ppam_shared PUBLIC cxx_std_17)
    else()
        target_compile_features(ppam_shared PUBLIC cxx_std_14)
    endif()
    target_include_directories(ppam_shared PUBLIC
        $<BUILD_INTERFACE:${PPAM_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${PPAM_INCLUDES_INSTALL_DIR}>)
    target_include_directories(ppam_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>)
    set_target_properties(ppam_shared PROPERTIES OUTPUT_NAME ppam)
    set_target_properties(ppam_shared PROPERTIES VERSION ${PPAM_VERSION})
    set_target_properties(ppam_shared PROPERTIES SOVERSION ${PPAM_VERSION_MAJOR}.${PPAM_VERSION_MINOR})

    target_link_libraries(ppam_shared PUBLIC nlohmann_json::nlohmann_json m)
    target_link_libraries(ppam_shared PRIVATE Threads::Threads)

    if(PPAM_BUILD_DPCA_PSI)
        add_dependencies(ppam_shared ${dpca_psi})
        target_include_directories(ppam_shared PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${dpca_psi},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${DPCA_PSI_INCLUDES_INSTALL_DIR}>)
    endif()
    set(PPAM_CARRY_DPCA_PSI FALSE)
    target_link_libraries(ppam_shared PUBLIC ${dpca_psi})

    if(PPAM_BUILD_EIGEN)
        add_dependencies(ppam_shared Eigen3::Eigen)
        target_include_directories(ppam_shared PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${PPAM_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
        set(PPAM_CARRY_EIGEN TRUE)
    else()
        target_link_libraries(ppam_shared PUBLIC Eigen3::Eigen)
        set(PPAM_CARRY_EIGEN FALSE)
    endif()

    if(PPAM_BUILD_OPENSSL)
        target_include_directories(ppam_shared PUBLIC $<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>)
        target_include_directories(ppam_shared PUBLIC $<INSTALL_INTERFACE:${PPAM_THIRDPARTY_INCLUDES_INSTALL_DIR}>)
    endif()
    target_link_libraries(ppam_shared PUBLIC ${openssl})
    set(PPAM_CARRY_OPENSSL FALSE)

    install(TARGETS ppam_shared
        EXPORT PPAMTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Add standard alias targets for PPAM::ppam and PPAM::ppam_shared
if(TARGET ppam)
    add_library(PPAM::ppam ALIAS ppam)
endif()
if(TARGET ppam_shared)
    add_library(PPAM::ppam_shared ALIAS ppam_shared)
endif()

#################################
# Installation and CMake config #
#################################

# Create the CMake config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${PPAM_CONFIG_IN_FILENAME} ${PPAM_CONFIG_FILENAME}
    INSTALL_DESTINATION ${PPAM_CONFIG_INSTALL_DIR}
)

# Install the export
install(
    EXPORT PPAMTargets
    NAMESPACE PPAM::
    DESTINATION ${PPAM_CONFIG_INSTALL_DIR})

# Version file; we require exact version match for downstream
write_basic_package_version_file(
    ${PPAM_CONFIG_VERSION_FILENAME}
    VERSION ${PPAM_VERSION}
    COMPATIBILITY SameMinorVersion)

# Install config and module files
install(
    FILES
        ${PPAM_CONFIG_FILENAME}
        ${PPAM_CONFIG_VERSION_FILENAME}
    DESTINATION ${PPAM_CONFIG_INSTALL_DIR})

# We export PPAMTargets from the build tree so it can be used by other projects
# without requiring an install.
export(
    EXPORT PPAMTargets
    NAMESPACE PPAM::
    FILE ${PPAM_TARGETS_FILENAME})

# Install header files of dependencies if PPAM_BUILD_DEPS is ON
if(PPAM_BUILD_DEPS)
    # Insert dependencies here
    if(PPAM_BUILD_DPCA_PSI)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${DPCA_PSI_BUILD_DIR} -t install)")
    endif()
    if(PPAM_BUILD_EIGEN)
        install(
            DIRECTORY ${eigen_SOURCE_DIR}/Eigen
            DESTINATION ${PPAM_THIRDPARTY_INCLUDES_INSTALL_DIR}
            COMPONENT Devel)
    endif()
    if(PPAM_BUILD_OPENSSL)
        install(
            FILES ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY}
            DESTINATION ${CMAKE_INSTALL_LIBDIR})
        install(
            DIRECTORY ${OPENSSL_INCLUDE_DIR}
            DESTINATION ${PPAM_THIRDPARTY_INCLUDES_INSTALL_DIR}/openssl)
    endif()
endif()

#####################
# PPAM C++ examples #
#####################

# [option] PPAM_BUILD_EXAMPLE
set(PPAM_BUILD_EXAMPLE_OPTION_STR "Build C++ example for PPAM")
option(PPAM_BUILD_EXAMPLE ${PPAM_BUILD_EXAMPLE_OPTION_STR} ON)
message(STATUS "PPAM_BUILD_EXAMPLE: ${PPAM_BUILD_EXAMPLE}")

if(PPAM_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()

##################
# PPAM C++ tests #
##################

# [option] PPAM_BUILD_TEST
set(PPAM_BUILD_TEST_OPTION_STR "Build C++ test for PPAM")
option(PPAM_BUILD_TEST ${PPAM_BUILD_TEST_OPTION_STR} TRUE)
message(STATUS "PPAM_BUILD_TEST: ${PPAM_BUILD_TEST}")

if(PPAM_BUILD_TEST)
    add_subdirectory(test)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND PPAM_ENABLE_GCOV)
        add_custom_target(test_coverage
            COMMAND gcovr -r ${CMAKE_CURRENT_LIST_DIR} -f \"src\" -e \".+\(test\\.cpp\)\" --xml-pretty -o "${CMAKE_CURRENT_BINARY_DIR}/report/coverage.xml"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    endif()
endif()
