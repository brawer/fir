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

#include "firc/AST.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>

namespace firc {

FileAST::FileAST() {
}

FileAST::~FileAST() {
}

void FileAST::Write(std::ostream* Out) const {
  bool First = true;
  for (auto p : Procedures) {
    if (!First) *Out << '\n';
    First = false;
    p->Write(Out);
  }
}

ProcedureAST::ProcedureAST(llvm::StringRef name)
  : Name(name) {
}

ProcedureAST::~ProcedureAST() {
}

void ProcedureAST::Write(std::ostream* Out) const {
  *Out << "proc " << Name.str() << "():\n    return\n";
}

}  // namespace firc
