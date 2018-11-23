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

#ifndef FIRC_PARSER_H_
#define FIRC_PARSER_H_

#include <functional>
#include <memory>
#include <string>

#include <llvm/Support/Allocator.h>
#include "firc/AST.h"
#include "firc/Lexer.h"

namespace llvm {
class MemoryBuffer;
}  // namespace llvm

namespace firc {

typedef std::function<void(llvm::StringRef, uint32_t, uint32_t,
                           llvm::StringRef)> ErrorHandler;

class Parser {
public:
  static firc::FileAST* parseFile(
      const llvm::MemoryBuffer* Buffer,
      llvm::StringRef Filename,
      llvm::StringRef Directory,
      ErrorHandler ErrHandler);

private:
  Parser(const llvm::MemoryBuffer* Buffer,
         llvm::StringRef Filename, llvm::StringRef Directory,
         ErrorHandler ErrHandler);
  ~Parser();

  void parse();
  bool parseTypeRef(TypeRef* T);
  bool parseName(Name* N);
  bool parseDottedName(DottedName* D);

  bool isAtExprStart() const;
  Expr* parseExpr();
  Expr* parseParenthesisExpr();
  Expr* parsePrimaryExpr();
  bool isBinaryOperator(TokenType Token) const;
  Expr* parseBinOpRHS(int Precedence, Expr* LHS);

  ProcedureAST* parseProcedure();
  VarDecl* parseConstDecl();
  ImportStatement* parseImportStatement();
  ImportDecl* parseImportDecl();
  ModuleDecl* parseModuleDecl();
  VarDecl* parseVarDecl();

  Statement* parseStatement();
  ConstStatement* parseConstStatement();
  ReturnStatement* parseReturnStatement();
  VarStatement* parseVarStatement();

  bool expectSymbol(TokenType Token);
  void reportError(const std::string& Error, const SourceLocation &Loc);
  void setLocation(uint32_t Line, uint32_t Column, SourceLocation *Loc);

  std::unique_ptr<firc::FileAST> FileAST;
  std::unique_ptr<firc::Lexer> Lexer;
  ErrorHandler ErrHandler;
};

}  // namespace firc

#endif // FIRC_PARSER_H_
