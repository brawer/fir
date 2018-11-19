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
#include <llvm/Support/MemoryBuffer.h>

#include "firc/Lexer.h"

int main(int argc, char** argv) {
  llvm::StringRef s("Foo Bar Baz");
  llvm::BumpPtrAllocator allocator;
  std::unique_ptr<llvm::MemoryBuffer> buf(llvm::MemoryBuffer::getMemBuffer(s));
  firc::Lexer lexer("hello.fir", "path/to/module", buf.get(), &allocator);
  std::cout << "Hello world" << std::endl;
  while (lexer.Advance()) {
  }
  return 0;
}
