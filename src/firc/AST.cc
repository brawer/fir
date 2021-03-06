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

int Expr::getPrecedence() const {
  return -1;
}

void BoolExpr::write(std::ostream* Out) const {
  *Out << (Value ? "true" : "false");
}

BinaryExpr::BinaryExpr(Expr* LHS, TokenType Operator, Expr* RHS)
  : LHS(LHS), RHS(RHS), Operator(Operator) {
}

void BinaryExpr::write(std::ostream* Out) const {
  const int MyPrec = getPrecedence();
  const int LeftPrec = LHS->getPrecedence();
  const int RightPrec = RHS->getPrecedence();
  const bool NeedLHSParen = MyPrec > LeftPrec && LeftPrec > 0;
  const bool NeedRHSParen = MyPrec > RightPrec && RightPrec > 0;
  if (NeedLHSParen) *Out << '(';
  LHS->write(Out);
  if (NeedLHSParen) *Out << ')';

  switch (Operator) {
  case TOKEN_PLUS: *Out << " + "; break;
  case TOKEN_MINUS: *Out << " + "; break;
  case TOKEN_ASTERISK: *Out << " * "; break;
  case TOKEN_SLASH: *Out << " / "; break;
  case TOKEN_PERCENT: *Out << " % "; break;
  default: *Out << " <ERROR> "; break;
  }

  if (NeedRHSParen) *Out << '(';
  RHS->write(Out);
  if (NeedRHSParen) *Out << ')';
}

int BinaryExpr::getPrecedence() const {
  return Lexer::getPrecedence(Operator);
}

void DotExpr::write(std::ostream* Out) const {
  const bool IsOperator = LHS->getPrecedence() >= 0;
  if (IsOperator) *Out << '(';
  LHS->write(Out);
  if (IsOperator) *Out << ')';
  if (LHS->needsSpaceBeforeDot()) {
    *Out << ' ';
  }
  *Out << '.' << Name.str();
}

IntExpr::IntExpr(llvm::APSInt Value) :
  Value(Value) {
}

IntExpr::~IntExpr() {
}

void IntExpr::write(std::ostream* Out) const {
  *Out << Value.toString(/*radix*/ 10);
}

void NameExpr::write(std::ostream* Out) const {
  *Out << Name.str();
}

void NilExpr::write(std::ostream* Out) const {
  *Out << "nil";
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

void ImportStatement::write(int Indent, std::ostream* Out) const {
  startLine(Indent, Out);
  *Out << "import ";
  bool First = true;
  for (ImportDecl* Decl : Decls) {
    if (First) First = false; else *Out << ", ";
    Decl->write(Out);
  }
  endLine(Out);
}

void ImportDecl::write(std::ostream* Out) const {
  bool First = true;
  for (const Name& N: ModuleRef) {
    if (First) First = false; else *Out << '.';
    *Out << N.Text.str();
  }
  if (!AsName.Text.empty()) {
    *Out << " as " << AsName.Text.str();
  }
}

void ModuleDecl::write(int Indent, std::ostream* Out) const {
  startLine(Indent, Out);
  *Out << "module";

  bool First = true;
  for (auto NamePart : ModuleName) {
    *Out << (First ? " " : ".") << NamePart.Text.str();
    First = false;
  }
  endLine(Out);
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

FileAST::FileAST(llvm::StringRef Filename, llvm::StringRef Directory)
  : Filename(Filename), Directory(Directory), ModuleDeclaration(nullptr) {
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
