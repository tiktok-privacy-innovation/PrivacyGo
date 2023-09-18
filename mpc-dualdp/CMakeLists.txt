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

#########################################################
# Project MPC_DUALDP includes the following components: #
#   1. MPC_DUALDP C++ library                           #
#   2. MPC_DUALDP C++ examples                          #
#   3. MPC_DUALDP C++ tests                             #
#########################################################

# [OPTION] CMAKE_BUILD_TYPE (DEFAULT: "Release")
# Select from Release, Debug, MiniSizeRel, or RelWithDebInfo.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS "Release" "Debug" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type (CMAKE_BUILD_TYPE): ${CMAKE_BUILD_TYPE}")

project(MPC_DUALDP VERSION 0.1.0 LANGUAGES CXX)

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
include(MPC_DualDPCustomMacros)

# In Debug mode, define MPC_DUALDP_DEBUG.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MPC_DUALDP_DEBUG ON)
else()
    set(MPC_DUALDP_DEBUG OFF)
endif()
message(STATUS "MPC_DUALDP debug mode: ${MPC_DUALDP_DEBUG}")

# In Debug mode, enable extra compiler flags.
include(EnableDebugFlags)

# Build position-independent-code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Make the install target depend on the all target
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY OFF)

# [OPTION] MPC_DUALDP_USE_CXX17 (default: OFF)
# Use C++17, use C++14 otherwise.
set(MPC_DUALDP_USE_CXX17_OPTION_STR "Use C++17")
option(MPC_DUALDP_USE_CXX17 ${MPC_DUALDP_USE_CXX17_OPTION_STR} OFF)
message(STATUS "MPC_DUALDP_USE_CXX17: ${MPC_DUALDP_USE_CXX17}")
# Enable features from C++17 if available, disable features if set to OFF.
include(EnableCXX17)

# Required files and directories
include(GNUInstallDirs)

# Runtime path
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Source Tree
set(MPC_DUALDP_INCLUDES_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(MPC_DUALDP_CONFIG_IN_FILENAME ${CMAKE_CURRENT_LIST_DIR}/cmake/MPC_DualDPConfig.cmake.in)
set(MPC_DUALDP_CONFIG_H_IN_FILENAME ${MPC_DUALDP_INCLUDES_DIR}/mpc-dualdp/utils/config.h.in)

# Build tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(MPC_DUALDP_THIRDPARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
set(MPC_DUALDP_TARGETS_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/MPC_DualDPTargets.cmake)
set(MPC_DUALDP_CONFIG_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/MPC_DualDPConfig.cmake)
set(MPC_DUALDP_CONFIG_VERSION_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/MPC_DualDPConfigVersion.cmake)
set(MPC_DUALDP_CONFIG_H_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/src/mpc-dualdp/utils/config.h)

# Install
set(MPC_DUALDP_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/MPC_DualDP-${MPC_DUALDP_VERSION_MAJOR}.${MPC_DUALDP_VERSION_MINOR})
set(MPC_DUALDP_INCLUDES_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/MPC_DualDP-${MPC_DUALDP_VERSION_MAJOR}.${MPC_DUALDP_VERSION_MINOR})
set(MPC_DUALDP_THIRDPARTY_INCLUDES_INSTALL_DIR ${MPC_DUALDP_INCLUDES_INSTALL_DIR}/thirdparty)

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
    MPC_DUALDP_ARM64
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
    MPC_DUALDP_AMD64
)
set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_OLD})
if (NOT MPC_DUALDP_AMD64 AND NOT MPC_DUALDP_ARM64)
    message(FATAL_ERROR "Supported target architectures are x86_64 and arm64")
endif()

add_compile_options(-msse4.2 -maes -mavx -Wno-ignored-attributes)

# Enable test coverage
set(MPC_DUALDP_ENABLE_GCOV_STR "Enable gcov")
option(MPC_DUALDP_ENABLE_GCOV ${MPC_DUALDP_ENABLE_GCOV_STR} OFF)
message(STATUS "MPC_DUALDP_ENABLE_GCOV: ${MPC_DUALDP_ENABLE_GCOV}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND MPC_DUALDP_ENABLE_GCOV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -fprofile-arcs -ftest-coverage -lgcov")
endif()

set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")

#########################
# External dependencies #
#########################

# [OPTION] MPC_DUALDP_BUILD_DEPS (DEFAULT: ON)
# Download and build dependencies if set to ON.
# Look for dependencies using find_package, otherwise.
set(MPC_DUALDP_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
option(MPC_DUALDP_BUILD_DEPS ${MPC_DUALDP_BUILD_DEPS_OPTION_STR} ON)
message(STATUS "MPC_DUALDP_BUILD_DEPS: ${MPC_DUALDP_BUILD_DEPS}")

if(MPC_DUALDP_BUILD_DEPS)
    include(FetchContent)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_QUIET)
endif()

# [OPTION] MPC_DUALDP_BUILD_SHARED_LIBS (DEFAULT: OFF)
# Build a shared library if set to ON.
set(MPC_DUALDP_BUILD_SHARED_LIBS_STR "Build shared library")
option(MPC_DUALDP_BUILD_SHARED_LIBS ${MPC_DUALDP_BUILD_SHARED_LIBS_STR} OFF)
message(STATUS "MPC_DUALDP_BUILD_SHARED_LIBS: ${MPC_DUALDP_BUILD_SHARED_LIBS}")

# PPAM:ppam
if(NOT TARGET PPAM::ppam AND NOT TARGET PPAM::ppam_shared)
    find_package(PPAM 0.1 QUIET CONFIG)
    if(PPAM_FOUND)
        message(STATUS "PPAM found")
        if(PPAM_STATIC_FOUND)
            set(ppam "PPAM::ppam")
        else()
            set(ppam "PPAM::ppam_shared")
        endif()
    else()
        if(MPC_DUALDP_BUILD_DEPS)
            message(STATUS "PPAM fetching internal...")
            include(PPAM)
            if(TARGET ppam)
                set(ppam "PPAM::ppam")
            else()
                set(ppam "PPAM::ppam_shared")
            endif()
            set(MPC_DUALDP_BUILD_PPAM TRUE CACHE BOOL "" FORCE)
        else()
            message(FATAL_ERROR "PPAM: not found, please download and install manually")
        endif()
    endif()
endif()

##########################
# MPC_DUALDP C++ library #
##########################

# Add source files to library and header files to install
set(MPC_DUALDP_SOURCE_FILES "")
add_subdirectory(src/mpc-dualdp)

# Create the config file
configure_file(${MPC_DUALDP_CONFIG_H_IN_FILENAME} ${MPC_DUALDP_CONFIG_H_FILENAME})
install(
    FILES ${MPC_DUALDP_CONFIG_H_FILENAME}
    DESTINATION ${MPC_DUALDP_INCLUDES_INSTALL_DIR}/mpc-dualdp/utils)

# Build only a static library
if(NOT MPC_DUALDP_BUILD_SHARED_LIBS)
    add_library(mpc_dualdp STATIC ${MPC_DUALDP_SOURCE_FILES})
    if(MPC_DUALDP_USE_CXX17)
        target_compile_features(mpc_dualdp PUBLIC cxx_std_17)
    else()
        target_compile_features(mpc_dualdp PUBLIC cxx_std_14)
    endif()
    target_include_directories(mpc_dualdp PUBLIC
        $<BUILD_INTERFACE:${MPC_DUALDP_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${MPC_DUALDP_INCLUDES_INSTALL_DIR}>)
    target_include_directories(mpc_dualdp PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>)
    set_target_properties(mpc_dualdp PROPERTIES OUTPUT_NAME mpc_dualdp-${MPC_DUALDP_VERSION_MAJOR}.${MPC_DUALDP_VERSION_MINOR})
    set_target_properties(mpc_dualdp PROPERTIES VERSION ${MPC_DUALDP_VERSION})

    if(MPC_DUALDP_BUILD_PPAM)
        add_dependencies(mpc_dualdp ${ppam})
        target_include_directories(mpc_dualdp PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${ppam},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${PPAM_INCLUDES_INSTALL_DIR}>)
    endif()
    set(MPC_DUALDP_CARRY_PPAM FALSE)
    target_link_libraries(mpc_dualdp PUBLIC ${ppam})

    install(TARGETS mpc_dualdp
        EXPORT MPC_DualDPTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Build only a shared library
else()
    add_library(mpc_dualdp_shared SHARED ${MPC_DUALDP_SOURCE_FILES})
    if(MPC_DUALDP_USE_CXX17)
        target_compile_features(mpc_dualdp_shared PUBLIC cxx_std_17)
    else()
        target_compile_features(mpc_dualdp_shared PUBLIC cxx_std_14)
    endif()
    target_include_directories(mpc_dualdp_shared PUBLIC
        $<BUILD_INTERFACE:${MPC_DUALDP_INCLUDES_DIR}>
        $<INSTALL_INTERFACE:${MPC_DUALDP_INCLUDES_INSTALL_DIR}>)
    target_include_directories(mpc_dualdp_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>)
    set_target_properties(mpc_dualdp_shared PROPERTIES OUTPUT_NAME mpc_dualdp)
    set_target_properties(mpc_dualdp_shared PROPERTIES VERSION ${MPC_DUALDP_VERSION})
    set_target_properties(mpc_dualdp_shared PROPERTIES SOVERSION ${MPC_DUALDP_VERSION_MAJOR}.${MPC_DUALDP_VERSION_MINOR})

    if(MPC_DUALDP_BUILD_PPAM)
        add_dependencies(mpc_dualdp_shared ${ppam})
        target_include_directories(mpc_dualdp_shared PUBLIC
            $<BUILD_INTERFACE:$<TARGET_PROPERTY:${ppam},INTERFACE_INCLUDE_DIRECTORIES>>
            $<INSTALL_INTERFACE:${PPAM_INCLUDES_INSTALL_DIR}>)
    endif()
    set(MPC_DUALDP_CARRY_PPAM FALSE)
    target_link_libraries(mpc_dualdp_shared PUBLIC ${ppam})

    install(TARGETS mpc_dualdp_shared
        EXPORT MPC_DualDPTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Add standard alias targets for MPC_DualDP::mpc_dualdp and MPC_DualDP::mpc_dualdp_shared
if(TARGET mpc_dualdp)
    add_library(MPC_DualDP::mpc_dualdp ALIAS mpc_dualdp)
endif()
if(TARGET mpc_dualdp_shared)
    add_library(MPC_DualDP::mpc_dualdp_shared ALIAS mpc_dualdp_shared)
endif()

#################################
# Installation and CMake config #
#################################

# Create the CMake config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${MPC_DUALDP_CONFIG_IN_FILENAME} ${MPC_DUALDP_CONFIG_FILENAME}
    INSTALL_DESTINATION ${MPC_DUALDP_CONFIG_INSTALL_DIR}
)

# Install the export
install(
    EXPORT MPC_DualDPTargets
    NAMESPACE MPC_DualDP::
    DESTINATION ${MPC_DUALDP_CONFIG_INSTALL_DIR})

# Version file; we require exact version match for downstream
write_basic_package_version_file(
    ${MPC_DUALDP_CONFIG_VERSION_FILENAME}
    VERSION ${MPC_DUALDP_VERSION}
    COMPATIBILITY SameMinorVersion)

# Install config and module files
install(
    FILES
        ${MPC_DUALDP_CONFIG_FILENAME}
        ${MPC_DUALDP_CONFIG_VERSION_FILENAME}
    DESTINATION ${MPC_DUALDP_CONFIG_INSTALL_DIR})

# We export MPC_DualDPTargets from the build tree so it can be used by other projects
# without requiring an install.
export(
    EXPORT MPC_DualDPTargets
    NAMESPACE MPC_DualDP::
    FILE ${MPC_DUALDP_TARGETS_FILENAME})

# Install header files of dependencies if MPC_DUALDP_BUILD_DEPS is ON
if(MPC_DUALDP_BUILD_DEPS)
    # Insert dependencies here
    if(MPC_DUALDP_BUILD_PPAM)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${PPAM_BUILD_DIR} -t install)")
    endif()
endif()

###########################
# MPC_DUALDP C++ examples #
###########################

# [option] MPC_DUALDP_BUILD_EXAMPLE
set(MPC_DUALDP_BUILD_EXAMPLE_OPTION_STR "Build C++ example for MPC_DUALDP")
option(MPC_DUALDP_BUILD_EXAMPLE ${MPC_DUALDP_BUILD_EXAMPLE_OPTION_STR} ON)
message(STATUS "MPC_DUALDP_BUILD_EXAMPLE: ${MPC_DUALDP_BUILD_EXAMPLE}")

if(MPC_DUALDP_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()

########################
# MPC_DUALDP C++ tests #
########################

# [option] MPC_DUALDP_BUILD_TEST
set(MPC_DUALDP_BUILD_TEST_OPTION_STR "Build C++ test for MPC_DUALDP")
option(MPC_DUALDP_BUILD_TEST ${MPC_DUALDP_BUILD_TEST_OPTION_STR} TRUE)
message(STATUS "MPC_DUALDP_BUILD_TEST: ${MPC_DUALDP_BUILD_TEST}")

if(MPC_DUALDP_BUILD_TEST)
    add_subdirectory(test)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND MPC_DUALDP_ENABLE_GCOV)
        add_custom_target(test_coverage
            COMMAND gcovr -r ${CMAKE_CURRENT_LIST_DIR} -f \"src\" -e \".+\(test\\.cpp\)\" --xml-pretty -o "${CMAKE_CURRENT_BINARY_DIR}/report/coverage.xml"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    endif()
endif()
