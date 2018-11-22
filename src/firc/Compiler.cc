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
#include <llvm/ADT/Twine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/ThreadPool.h>
#include <llvm/Support/thread.h>
#include "firc/AST.h"
#include "firc/Compiler.h"
#include "firc/Lexer.h"
#include "firc/Parser.h"

namespace firc {

Compiler::Compiler() {
}

Compiler::~Compiler() {
  if (Threads) {
    Threads->wait();
  }
}

bool Compiler::compile(llvm::StringRef Path) {
  llvm::SmallVector<std::string, 16> Files;
  if (llvm::sys::fs::is_regular_file(Path)) {
    Files.push_back(Path);
  } else if (llvm::sys::fs::is_directory(Path)) {
    std::error_code Error;
    llvm::sys::fs::recursive_directory_iterator DirIt(Path, Error), DirEnd;
    if (Error) {
    }
    while (DirIt != DirEnd) {
      if (Error) {
        reportError(DirIt->path(), Error);
        return false;
      }
      if (llvm::StringRef(DirIt->path()).endswith(".fir")) {
        Files.push_back(DirIt->path());
      }
      DirIt.increment(Error);
    }
  }

  auto compileAsync = [](const std::string& SourceFile) {
    std::cerr << "TODO: Compile " << SourceFile << std::endl;
  };
  if (Files.size() == 0) {
    return false;
  } else if (Files.size() == 1) {
    compileAsync(Files[0]);
  } else {
    if (!Threads) {
      Threads.reset(
          new llvm::ThreadPool(llvm::thread::hardware_concurrency()));
    }
    for (const std::string& SourceFile : Files) {
      Threads->async(compileAsync, SourceFile);
    }
    Threads->wait();
  }
  return true;
}

void Compiler::reportError(llvm::StringRef Path,
                           const std::error_code& Error) {
  std::cerr << Path.str() << ": Error reading" << std::endl;
}

}  // namespace firc
