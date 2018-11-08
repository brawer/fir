# Copyright © 2018 by Sascha Brawer <sascha@brawer.ch>
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an “AS IS” BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.0)

project(firc)

if(POLICY CMP0074)
cmake_policy(SET CMP0074 OLD)
endif()

set(LLVM_ROOT "" CACHE PATH "Root of LLVM installation.")
if(NOT EXISTS "${LLVM_ROOT}/include/llvm" )
    message(FATAL_ERROR "LLVM_ROOT (${LLVM_ROOT}) does not exist")
endif()

list(APPEND CMAKE_PREFIX_PATH "${LLVM_ROOT}/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)


# ---------------------------------------------------------------------------
# firc_lib
# ---------------------------------------------------------------------------

add_library(firc_lib
    lexer.cc lexer.h
    generated_charsets.cc
)

set_target_properties(firc_lib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_compile_definitions(firc_lib
    PRIVATE ${LLVM_DEFINITIONS})

target_include_directories(firc_lib
    PRIVATE ..
    PRIVATE ${LLVM_INCLUDE_DIRS}
)

target_link_directories(firc_lib
    PRIVATE ${LLVM_LIBRARY_DIRS})

target_link_libraries(firc_lib LLVMSupport)


# ---------------------------------------------------------------------------
# firc
# ---------------------------------------------------------------------------

add_executable(firc
    main.cc
)

set_target_properties(firc PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(firc
    PRIVATE .. ${LLVM_INCLUDE_DIRS})

target_link_libraries(firc firc_lib)


# ---------------------------------------------------------------------------
# firc_test
# ---------------------------------------------------------------------------

add_executable(firc_test
    lexer_test.cc
)

set_target_properties(firc_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(firc_test
    PRIVATE .. ${LLVM_INCLUDE_DIRS})

target_link_libraries(firc_test firc_lib gtest gtest_main)

enable_testing()

add_test(NAME firc_test COMMAND firc_test)
