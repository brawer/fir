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

typedef std::function<void(llvm::StringRef, const SourceLocation&)>
    ErrorHandler;

class Parser {
public:
  static firc::FileAST* parseFile(
      llvm::StringRef Filename,
      llvm::StringRef Directory,
      const llvm::MemoryBuffer* Buf,
      llvm::BumpPtrAllocator* Allocator,
      ErrorHandler ErrHandler);

private:
  Parser(firc::Lexer* lexer, ErrorHandler ErrHandler);
  ~Parser();

  void parse();
  bool parseTypeRef(TypeRef* T);

  bool isAtExprStart() const;
  Expr* parseExpr();
  Expr* parseParenthesisExpr();
  Expr* parsePrimaryExpr();
  Expr* parseBinOpRHS(int Precedence, Expr* LHS);

  ProcedureAST* parseProcedure();
  VarDecl* parseConstDecl();
  VarDecl* parseVarDecl();

  Statement* parseStatement();
  ConstStatement* parseConstStatement();
  ReturnStatement* parseReturnStatement();
  VarStatement* parseVarStatement();

  bool expectSymbol(TokenType Token);
  void reportError(const std::string& Error, const SourceLocation &Loc);
  void setLocation(uint32_t Line, uint32_t Column, SourceLocation *Loc);

  firc::Lexer* Lexer;
  ErrorHandler ErrHandler;
  std::unique_ptr<firc::FileAST> FileAST;
};

}  // namespace firc

#endif // FIRC_PARSER_H_
