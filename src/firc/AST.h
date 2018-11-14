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

#ifndef FIRC_AST_H_
#define FIRC_AST_H_

#include <sstream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>

namespace firc {

class ProcedureAST;

class FileAST {
public:
  FileAST();
  ~FileAST();
  void Write(std::ostream* Out) const;

  llvm::SmallVector<ProcedureAST*, 32> Procedures;
};

class ProcedureAST {
public:
  ProcedureAST(llvm::StringRef name);
  ~ProcedureAST();
  void Write(std::ostream* Out) const;

  llvm::StringRef Name;
};

};  // namespace firc

#endif // FIRC_AST_H_
