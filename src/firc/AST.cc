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
  for (auto p : Procedures) delete p;
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
  for (auto p : Params) delete p;
}

void ProcedureAST::Write(std::ostream* Out) const {
  *Out << "proc " << Name.str() << "(";
  bool First = true;
  for (auto param : Params) {
    if (!First) *Out << ", ";
    First = false;
    *Out << param->Name.str();
  }
  *Out << "):\n    return\n";
}

ProcedureParamAST::ProcedureParamAST(llvm::StringRef Name)
  : Name(Name) {
}

void ProcedureParamAST::Write(std::ostream* Out) const {
  *Out << Name.str();  // TODO: Add colon and Type if present
}

}  // namespace firc
