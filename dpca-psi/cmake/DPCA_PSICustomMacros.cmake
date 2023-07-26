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

# Manually combine archives, using ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} to keep temporary files.
macro(dpca_psi_combine_archives target dependency)
    if(MSVC)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND lib.exe /OUT:$<TARGET_FILE:${target}> $<TARGET_FILE:${target}> $<TARGET_FILE:${dependency}>
            DEPENDS $<TARGET_FILE:${target}> $<TARGET_FILE:${dependency}>
            WORKING_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    else()
        if(CMAKE_HOST_WIN32)
            get_filename_component(CXX_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
            set(AR_CMD_PATH "${CXX_DIR}/llvm-ar.exe")
            file(TO_NATIVE_PATH "${AR_CMD_PATH}" AR_CMD_PATH)
            set(DEL_CMD "del")
            set(DEL_CMD_OPTS "")
        else()
            set(AR_CMD_PATH "ar")
            set(DEL_CMD "rm")
            set(DEL_CMD_OPTS "-rf")
        endif()
        if(EMSCRIPTEN)
            set(AR_CMD_PATH "emar")
        endif()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${AR_CMD_PATH}" x $<TARGET_FILE:${target}>
            COMMAND "${AR_CMD_PATH}" x $<TARGET_FILE:${dependency}>
            COMMAND "${AR_CMD_PATH}" rcs $<TARGET_FILE:${target}> *.o
            COMMAND ${DEL_CMD} ${DEL_CMD_OPTS} *.o
            WORKING_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    endif()
endmacro()

# Fetch thirdparty content specified by content_file
macro(dpca_psi_fetch_thirdparty_content content_file)
    set(DPCA_PSI_FETCHCONTENT_BASE_DIR_OLD ${FETCHCONTENT_BASE_DIR})
    set(FETCHCONTENT_BASE_DIR ${DPCA_PSI_THIRDPARTY_DIR} CACHE STRING "" FORCE)
    include(${content_file})
    set(FETCHCONTENT_BASE_DIR ${DPCA_PSI_FETCHCONTENT_BASE_DIR_OLD} CACHE STRING "" FORCE)
    unset(DPCA_PSI_FETCHCONTENT_BASE_DIR_OLD)
endmacro()
