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

TypeRef::TypeRef()
  : Optional(false) {
}

bool TypeRef::equals(const TypeRef &Other) const {
  if (this == &Other) {
    return true;
  }
  if (this->Optional != Other.Optional) {
    return false;
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
  if (Optional) {
    *Out << "optional ";
  }
  bool First = true;
  for (const llvm::StringRef& Name : QualifiedName) {
    if (!First) *Out << '.';
    First = false;
    *Out << Name.str();
  }
}

Statement::~Statement() {
}

IntExpr::IntExpr(llvm::APSInt Value) :
  Value(Value) {
}

IntExpr::~IntExpr() {
}

void IntExpr::write(std::ostream* Out) const {
  *Out << Value.toString(/*radix*/ 10);
}

void Statement::startLine(int Indent, std::ostream* Out) const {
  for (int i = 0; i < Indent; ++i) {
    *Out << "    ";
  }
}

void Statement::endLine(std::ostream* Out) const {
  if (!Comment.empty()) {
    *Out << "  # " << Comment.str();
  }
  *Out << '\n';
}

void ConstStatement::write(int Indent, std::ostream* Out) const {
  startLine(Indent, Out);
  *Out << "const ";
  bool First = true;
  for (auto Const : Consts) {
    if (First) First = false; else *Out << "; ";
    Const->write(Out);
  }
  endLine(Out);
}

void EmptyStatement::write(int Indent, std::ostream* Out) const {
  if (!Comment.empty()) {
    startLine(Indent, Out);
    *Out << "# " << Comment.str();
  }
  *Out << '\n';
}

void ReturnStatement::write(int Indent, std::ostream* Out) const {
  startLine(Indent, Out);
  *Out << "return";
  if (Result) {
    *Out << ' ';
    Result->write(Out);
  }
  endLine(Out);
}

void VarStatement::write(int Indent, std::ostream* Out) const {
  startLine(Indent, Out);
  *Out << "var ";
  bool First = true;
  for (auto CurVar : Vars) {
    if (First) First = false; else *Out << "; ";
    CurVar->write(Out);
  }
  endLine(Out);
}

FileAST::FileAST() {
}

FileAST::~FileAST() {
  for (auto Statement : Body) delete Statement;
}

void FileAST::write(std::ostream* Out) const {
  bool First = true;
  for (auto Statement : Body) {
    if (First) First = false; else *Out << '\n';
    Statement->write(/* Indent */ 0, Out);
  }
}

ProcedureAST::ProcedureAST(llvm::StringRef name)
  : Name(name) {
}

ProcedureAST::~ProcedureAST() {
  for (auto Param : Params) delete Param;
  for (auto Statement : Body) delete Statement;
}

void ProcedureAST::write(int Indent, std::ostream* Out) const {
  startLine(Indent, Out);
  *Out << "proc " << Name.str() << "(";
  bool First = true;
  for (auto Param : Params) {
    if (First) First = false; else *Out << "; ";
    Param->write(Out);
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
  endLine(Out);
  for (auto Statement : Body) {
    Statement->write(Indent + 1, Out);
  }
}

VarDecl::VarDecl(const Names &VarNames, const TypeRef &Type, Expr* Value)
  : VarNames(VarNames), Type(Type), Value(Value) {
}

void VarDecl::write(std::ostream* Out) const {
  bool First = true;
  for (auto Name : VarNames) {
    if (First) First = false; else *Out << ", ";
    *Out << Name.str();
  }
  if (Type.isSpecified()) {
    *Out << ": ";
    Type.write(Out);
  }
  if (Value) {
    *Out << " = ";
    Value->write(Out);
  }
}

}  // namespace firc
