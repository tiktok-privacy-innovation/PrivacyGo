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

include(CheckCXXCompilerFlag)
function(ppam_add_compiler_flag flag)
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
if(NOT MSVC AND PPAM_DEBUG)
    ppam_add_compiler_flag("-Wall")
    ppam_add_compiler_flag("-Wextra")
    ppam_add_compiler_flag("-Wconversion")
    ppam_add_compiler_flag("-Wshadow")
    ppam_add_compiler_flag("-pedantic")
endif()
