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

bool TypeRef::equals(const TypeRef& Other) const {
  if (this == &Other) {
    return true;
  }
  if (this->QualifiedName.size() != Other.QualifiedName.size()) {
    return false;
  }
  for (size_t i = 0; i < this->QualifiedName.size(); ++i) {
    if (!this->QualifiedName[i].equals(Other.QualifiedName[i])) {
      return false;
    }
  }
  return true;
}

void TypeRef::write(std::ostream* Out) const {
  bool First = true;
  for (const llvm::StringRef& Name : QualifiedName) {
    if (!First) *Out << '.';
    First = false;
    *Out << Name.str();
  }
}

FileAST::FileAST() {
}

FileAST::~FileAST() {
  for (auto p : Procedures) delete p;
}

void FileAST::write(std::ostream* Out) const {
  bool First = true;
  for (auto p : Procedures) {
    if (!First) *Out << '\n';
    First = false;
    p->write(Out);
  }
}

ProcedureAST::ProcedureAST(llvm::StringRef name)
  : Name(name) {
}

ProcedureAST::~ProcedureAST() {
  for (auto p : Params) delete p;
}

void ProcedureAST::write(std::ostream* Out) const {
  *Out << "proc " << Name.str() << "(";
  const size_t NumParams = Params.size();
  char Separator = ' ';
  for (int i = 0; i < NumParams; ++i) {
    if (Separator != ' ') *Out << Separator << ' ';
    const ProcedureParamAST* Param = Params[i];
    const ProcedureParamAST* NextParam =
        i + 1 < NumParams ? Params[i + 1] : nullptr;
    *Out << Param->Name.str();
    if (NextParam != nullptr && NextParam->Type.equals(Param->Type)) {
      Separator = ',';
    } else {
      if (Param->Type.isSpecified()) {
        *Out << ": ";
        Param->Type.write(Out);
      }
      Separator = ';';
    }
  }
  *Out << "):";
  if (ResultType.isSpecified()) {
    *Out << ' ';
    bool FirstPart = true;
    for (auto NamePart : ResultType.QualifiedName) {
      if (!FirstPart) *Out << '.';
      FirstPart = false;
      *Out << NamePart.str();
    }
  }
  *Out << "\n    return\n";
}

ProcedureParamAST::ProcedureParamAST(llvm::StringRef Name, TypeRef Type)
  : Name(Name), Type(Type) {
}

void ProcedureParamAST::write(std::ostream* Out) const {
  *Out << Name.str();  // TODO: Add colon and Type if present
}

}  // namespace firc
