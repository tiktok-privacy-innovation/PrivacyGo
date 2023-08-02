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

include(ExternalProject)

set(OPENSSL_SOURCES_DIR ${PPAM_THIRDPARTY_DIR}/openssl-src)
set(OPENSSL_INSTALL_DIR "${OPENSSL_SOURCES_DIR}/install")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_INSTALL_DIR}/include")
set(OPENSSL_NAME "openssl")

include(ProcessorCount)
ProcessorCount(NUM_OF_PROCESSOR)

if((NOT DEFINED OPENSSL_URL) OR (NOT DEFINED OPENSSL_VER))
  set(OPENSSL_URL "https://www.openssl.org/source/old/1.1.1/openssl-1.1.1d.tar.gz" CACHE STRING "" FORCE)
  set(OPENSSL_VER "openssl-1.1.1d" CACHE STRING "" FORCE)
endif()

ExternalProject_Add(
  extern_openssl
  PREFIX            ${OPENSSL_SOURCES_DIR}
  DOWNLOAD_COMMAND  wget --no-check-certificate ${OPENSSL_URL} -c -q -O ${OPENSSL_NAME}.tar.gz
                    && tar -xvf ${OPENSSL_NAME}.tar.gz
  SOURCE_DIR        ${OPENSSL_SOURCES_DIR}/src/${OPENSSL_VER}
  CONFIGURE_COMMAND ./config no-shared -fPIC --prefix=${OPENSSL_INSTALL_DIR} ${OPENSSL_ASM_OPTION}
  BUILD_COMMAND     make depend -j ${NUM_OF_PROCESSOR} && make -j ${NUM_OF_PROCESSOR}
  INSTALL_COMMAND   make install_sw
  BUILD_IN_SOURCE   1
)

set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_INSTALL_DIR}/lib/libcrypto.a")
set(OPENSSL_SSL_LIBRARY "${OPENSSL_INSTALL_DIR}/lib/libssl.a")

add_library(Crypto STATIC IMPORTED GLOBAL)
add_library(ssl STATIC IMPORTED GLOBAL)
set_property(TARGET Crypto PROPERTY IMPORTED_LOCATION ${OPENSSL_CRYPTO_LIBRARY})
set_property(TARGET ssl PROPERTY IMPORTED_LOCATION ${OPENSSL_SSL_LIBRARY})

add_dependencies(Crypto extern_openssl)
add_dependencies(ssl extern_openssl)

include_directories(${OPENSSL_INCLUDE_DIR})
