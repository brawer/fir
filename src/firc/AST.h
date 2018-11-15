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
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>

namespace firc {

class ProcedureAST;
class ProcedureParamAST;

class TypeRef {
public:
  void write(std::ostream* Out) const;
  bool isSpecified() const { return !QualifiedName.empty(); }
  bool equals(const TypeRef& Other) const;
  llvm::SmallVector<llvm::StringRef, 4> QualifiedName;
};

class Statement {
public:
  virtual ~Statement();
  virtual void write(int Indent, std::ostream* Out) const = 0;
  llvm::StringRef Comment;

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
};

class VarDecl {
public:
  VarDecl(const llvm::StringRef &Name, const TypeRef &Type);
  llvm::StringRef Name;
  TypeRef Type;
};

typedef llvm::SmallVector<VarDecl*, 4> VarDecls;

class VarStatement : public Statement {
public:
  virtual void write(int Indent, std::ostream* Out) const;
  VarDecls Vars;
};

class FileAST {
public:
  FileAST();
  ~FileAST();
  void write(std::ostream* Out) const;

  llvm::SmallVector<ProcedureAST*, 32> Procedures;
};

class ProcedureAST {
public:
  ProcedureAST(llvm::StringRef name);
  ~ProcedureAST();
  void write(std::ostream* Out) const;

  llvm::StringRef Name;
  llvm::SmallVector<ProcedureParamAST*, 8> Params;
  llvm::SmallVector<Statement*, 8> Body;
  TypeRef ResultType;
};

class ProcedureParamAST {
public:
  ProcedureParamAST(const llvm::StringRef& Name, const TypeRef& Type);
  void write(std::ostream* Out) const;

  llvm::StringRef Name;
  firc::TypeRef Type;
};

};  // namespace firc

#endif // FIRC_AST_H_
