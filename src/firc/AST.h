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

namespace firc {

class FileAST;
class ProcedureAST;
class ProcedureParamAST;

class SourceLocation {
public:
  SourceLocation() : File(nullptr), Line(0), Column(0) {}
  FileAST *File;
  uint32_t Line, Column;
};

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
  SourceLocation Location;
};

class BoolExpr : public Expr {
public:
  explicit BoolExpr(bool V) : Value(V) {}
  virtual ~BoolExpr() {}
  virtual void write(std::ostream* Out) const;
  bool Value;
};

class IntExpr : public Expr {
public:
  explicit IntExpr(llvm::APSInt Value);
  virtual ~IntExpr();
  virtual void write(std::ostream* Out) const;
  llvm::APSInt Value;
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

class ReturnStatement : public Statement {
public:
  virtual void write(int Indent, std::ostream* Out) const;
  std::unique_ptr<Expr> Result;
};

typedef llvm::SmallVector<llvm::StringRef, 4> Names;

class VarDecl {
public:
  VarDecl(const Names &Names, const TypeRef &Type, Expr *Value);
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

  llvm::SmallVector<Statement*, 32> Body;
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
