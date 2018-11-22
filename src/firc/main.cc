// Copyright 2018 by Sascha Brawer <sascha@brawer.ch>
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an “AS IS” BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <memory>

#include <llvm/ADT/StringRef.h>
#include "llvm/Support/CommandLine.h"
#include <llvm/Support/MemoryBuffer.h>

#include "firc/Compiler.h"

llvm::cl::opt<std::string> Command(
    llvm::cl::Positional, llvm::cl::Required,
    llvm::cl::desc("<build|format|run>"), llvm::cl::init("build"));

llvm::cl::opt<std::string> Input(
    llvm::cl::Positional, llvm::cl::Required, llvm::cl::desc("<Input>"));

int main(int argc, char** argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);
  if (Command == "build") {
    firc::Compiler Compiler;
    return Compiler.compile(Input) ? 0 : 1;
  } else if (Command == "format" || Command == "run") {
    std::cerr << "command ‘" << Command << "’ not yet implemented"
              << std::endl;
    return 1;
  } else {
    std::cerr << "command must be ‘build’, ‘format’, or ‘run’" << std::endl;
    return 1;
  }
  return 0;
}
