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

#include <memory>
#include <sstream>
#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include "firc/Lexer.h"

namespace firc {

class FileAST;
class ProcedureAST;
class ProcedureParamAST;

typedef llvm::SmallVector<llvm::StringRef, 4> Names;

class SourceLocation {
public:
  SourceLocation() : File(nullptr), Line(0), Column(0) {}
  FileAST *File;
  uint32_t Line, Column;
};

class Name {
public:
  Name() {}
  Name(llvm::StringRef Text, SourceLocation Loc) : Text(Text), Location(Loc) {}
  ~Name() {}
  llvm::StringRef Text;
  SourceLocation Location;
};

typedef llvm::SmallVector<Name, 4> DottedName;

class TypeRef {
public:
  TypeRef();
  void write(std::ostream* Out) const;
  bool isSpecified() const { return !QualifiedName.empty(); }
  SourceLocation Location;
  llvm::SmallVector<llvm::StringRef, 4> QualifiedName;
  bool Optional;
};

class Expr {
public:
  virtual ~Expr() {}
  virtual void write(std::ostream* Out) const = 0;
  virtual int getPrecedence() const;
  virtual bool needsSpaceBeforeDot() const { return false; }
  SourceLocation Location;
};

class BoolExpr : public Expr {
public:
  explicit BoolExpr(bool V) : Value(V) {}
  virtual ~BoolExpr() {}
  virtual void write(std::ostream* Out) const;
  bool Value;
};

class BinaryExpr : public Expr {
public:
  explicit BinaryExpr(Expr* LHS, TokenType Operator, Expr* RHS);
  virtual ~BinaryExpr() {}
  virtual void write(std::ostream* Out) const;
  virtual int getPrecedence() const;
  std::unique_ptr<Expr> LHS, RHS;
  TokenType Operator;
};

class DotExpr : public Expr {
public:
  explicit DotExpr(Expr* LHS, llvm::StringRef Name) : LHS(LHS), Name(Name) {}
  virtual ~DotExpr() {}
  virtual void write(std::ostream* Out) const;
  std::unique_ptr<Expr> LHS;
  llvm::StringRef Name;
  SourceLocation NameLocation;
};

class IntExpr : public Expr {
public:
  explicit IntExpr(llvm::APSInt Value);
  virtual ~IntExpr();
  virtual void write(std::ostream* Out) const;
  virtual bool needsSpaceBeforeDot() const { return true; }
  llvm::APSInt Value;
};

class NameExpr : public Expr {
public:
  explicit NameExpr(llvm::StringRef Name) : Name(Name) {}
  virtual ~NameExpr() {}
  virtual void write(std::ostream* Out) const;
  llvm::StringRef Name;
};

class NilExpr : public Expr {
public:
  explicit NilExpr() {}
  virtual ~NilExpr() {}
  virtual void write(std::ostream* Out) const;
};

class Statement {
public:
  virtual ~Statement();
  virtual void write(int Indent, std::ostream* Out) const = 0;
  llvm::StringRef Comment;
  SourceLocation Location;

protected:
  void startLine(int Indent, std::ostream* Out) const;
  void endLine(std::ostream* Out) const;
};

class EmptyStatement : public Statement {
public:
  virtual void write(int Indent, std::ostream* Out) const;
};

class ImportDecl {
public:
  DottedName ModuleRef;
  Name AsName;
  void write(std::ostream* Out) const;
};

class ImportStatement : public Statement {
public:
  virtual void write(int Indent, std::ostream* Out) const;
  llvm::SmallVector<ImportDecl*, 4> Decls;
};

class ModuleDecl : public Statement {
public:
  ModuleDecl() {}
  virtual ~ModuleDecl() {}
  virtual void write(int Indent, std::ostream* Out) const;
  DottedName ModuleName;
};

class ReturnStatement : public Statement {
public:
  virtual void write(int Indent, std::ostream* Out) const;
  std::unique_ptr<Expr> Result;
};

class VarDecl {
public:
  VarDecl(const Names &Names, const TypeRef &Type, Expr *Value);
  virtual ~VarDecl() {}
  virtual void write(std::ostream *Out) const;
  Names VarNames;
  TypeRef Type;
  std::unique_ptr<Expr> Value;
  SourceLocation Location;
};

typedef llvm::SmallVector<VarDecl*, 4> VarDecls;

class ConstStatement : public Statement {
public:
  virtual void write(int Indent, std::ostream* Out) const;
  VarDecls Consts;
};

class VarStatement : public Statement {
public:
  virtual void write(int Indent, std::ostream* Out) const;
  VarDecls Vars;
};

class FileAST {
public:
  FileAST(llvm::StringRef Filename, llvm::StringRef Directory);
  ~FileAST();
  void write(std::ostream* Out) const;

  llvm::BumpPtrAllocator Allocator;  // for temp objects, eg. converted tokens
  llvm::SmallVector<Statement*, 32> Body;
  llvm::StringRef Filename, Directory;
  ModuleDecl* ModuleDeclaration;
  llvm::SmallVector<ImportStatement*, 8> Imports;  // anywhere in parsed file
};

class ProcedureAST : public Statement {
public:
  ProcedureAST(llvm::StringRef name);
  virtual ~ProcedureAST();
  virtual void write(int Indent, std::ostream* Out) const;

  llvm::StringRef Name;
  VarDecls Params;
  llvm::SmallVector<Statement*, 8> Body;
  TypeRef ResultType;
};

};  // namespace firc

#endif // FIRC_AST_H_
