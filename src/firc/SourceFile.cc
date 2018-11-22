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
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>

#include "firc/Parser.h"
#include "firc/SourceFile.h"

namespace firc {

SourceFile::SourceFile(llvm::StringRef Filepath, llvm::StringRef Directory)
  : Filepath(Filepath), Directory(Directory) {
}

SourceFile::~SourceFile() {
}

void SourceFile::parse(ErrorHandler ErrHandler) {
  llvm::SmallVector<char, 200> path;
  llvm::sys::path::append(path, Directory, Filepath);
  llvm::StringRef SrcPath(path.data(), path.size());
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> Buf =
      llvm::MemoryBuffer::getFile(SrcPath, /* FileSize */ -1,
                                  /* RequiresNullTerminator */ false);
  std::error_code ReadError = Buf.getError();
  if (ReadError) {
    ErrHandler(SrcPath, 0, 0, ReadError.message());
    return;
  }
  Buffer.reset(Buf->release());
  AST.reset(Parser::parseFile(Buffer.get(), Filepath, Directory, ErrHandler));
}

}  // namespace firc
