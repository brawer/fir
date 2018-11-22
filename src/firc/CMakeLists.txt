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
# FircLib
# ---------------------------------------------------------------------------

add_library(FircLib
    AST.cc AST.h
    Compiler.cc Compiler.h
    CompiledFile.cc CompiledFile.h
    Lexer.cc Lexer.h
    Parser.cc Parser.h
    GeneratedCharsets.cc
)

set_target_properties(FircLib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_compile_definitions(FircLib
    PRIVATE ${LLVM_DEFINITIONS})

target_compile_options(FircLib PRIVATE -fno-rtti -Wall)

target_include_directories(FircLib
    PRIVATE ..
    PRIVATE ${LLVM_INCLUDE_DIRS}
)

target_link_directories(FircLib
    PRIVATE ${LLVM_LIBRARY_DIRS})

llvm_map_components_to_libnames(firc_llvm_libs Core Support Option)
target_link_libraries(FircLib ${firc_llvm_libs})


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

target_compile_options(firc PRIVATE -fno-rtti -Wall)

target_include_directories(firc
    PRIVATE .. ${LLVM_INCLUDE_DIRS})

target_link_libraries(firc FircLib)


# ---------------------------------------------------------------------------
# FircTest
# ---------------------------------------------------------------------------

add_executable(FircTest
    LexerTest.cc ParserTest.cc
)

set_target_properties(FircTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(FircTest
    PRIVATE .. ${LLVM_INCLUDE_DIRS})

target_link_libraries(FircTest FircLib gtest gtest_main)

enable_testing()

add_test(NAME FircTest COMMAND FircTest)
