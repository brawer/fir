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

#include <iostream>
#include <memory>
#include "firc/AST.h"
#include "firc/Lexer.h"
#include "firc/Parser.h"

namespace firc {

firc::FileAST* Parser::parseFile(const llvm::MemoryBuffer* buf,
                                 llvm::BumpPtrAllocator* allocator) {
  firc::Lexer lexer(buf, allocator);
  firc::Parser parser(&lexer);
  parser.parse();
  return parser.FileAST.release();
}

Parser::Parser(firc::Lexer* Lexer)
  : Lexer(Lexer), FileAST(new firc::FileAST()) {
}

Parser::~Parser() {
}

void Parser::parse() {
  while (Lexer->Advance()) {
    if (Lexer->CurToken == TOKEN_PROC) {
      std::unique_ptr<ProcedureAST> p(parseProcedure());
      if (p.get() != nullptr) {
        FileAST->Procedures.push_back(p.release());
      }
      continue;
    }
  }
}

bool Parser::parseTypeRef(TypeRef* T) {
  if (!expectSymbol(TOKEN_IDENTIFIER)) {
    return false;
  }
  T->QualifiedName.push_back(Lexer->CurTokenText);
  Lexer->Advance();
  while (Lexer->CurToken == TOKEN_DOT) {
    Lexer->Advance();
    if (!expectSymbol(TOKEN_IDENTIFIER)) {
      return false;
    }
    T->QualifiedName.push_back(Lexer->CurTokenText);
    Lexer->Advance();
  }
  return true;
}

bool Parser::isAtExprStart() const {
  return Lexer->CurToken == TOKEN_INTEGER;
}

Expr* Parser::parseExpr() {
  switch (Lexer->CurToken) {
  case TOKEN_INTEGER: {
    std::unique_ptr<IntExpr> IntEx(
        new IntExpr(llvm::APSInt(Lexer->CurTokenText)));
    Lexer->Advance();
    return IntEx.release();
  }

  default: {
    reportError("Expected expression");
    return nullptr;
  }
  }
}

ProcedureAST* Parser::parseProcedure() {
  assert(Lexer->CurToken == TOKEN_PROC);
  Lexer->Advance();
  if (!expectSymbol(TOKEN_IDENTIFIER)) {
    return nullptr;
  }

  std::unique_ptr<ProcedureAST> result(new ProcedureAST(Lexer->CurTokenText));

  Lexer->Advance();
  if (!expectSymbol(TOKEN_LEFT_PARENTHESIS)) {
    return nullptr;
  }

  Lexer->Advance();
  if (Lexer->CurToken != TOKEN_RIGHT_PARENTHESIS) {
    if (!parseProcedureParams(result.get())) {
      return nullptr;
    }
    while (Lexer->CurToken == TOKEN_SEMICOLON) {
      Lexer->Advance();
      if (!parseProcedureParams(result.get())) {
        return nullptr;
      }
    }
  }

  if (!expectSymbol(TOKEN_RIGHT_PARENTHESIS)) {
    return nullptr;
  }

  Lexer->Advance();
  if (!expectSymbol(TOKEN_COLON)) {
    return nullptr;
  }

  Lexer->Advance();
  if (Lexer->CurToken != TOKEN_NEWLINE) {  // TODO: TOKEN_COMMENT
    if (!parseTypeRef(&result->ResultType)) {
      return nullptr;
    }
  }

  if (!expectSymbol(TOKEN_NEWLINE)) {
    return nullptr;
  }

  Lexer->Advance();
  if (!expectSymbol(TOKEN_INDENT)) {
    return nullptr;
  }

  Lexer->Advance();
  while (Lexer->CurToken != TOKEN_UNINDENT) {
    Statement* S = parseStatement();
    if (S != nullptr) {
      result->Body.push_back(S);
    }
  }

  if (!expectSymbol(TOKEN_UNINDENT)) {
    return nullptr;
  }

  return result.release();
}

bool Parser::parseProcedureParams(ProcedureAST* proc) {
  llvm::SmallVector<llvm::StringRef, 4> ParamNames;
  if (!expectSymbol(TOKEN_IDENTIFIER)) {
    return false;
  }
  ParamNames.push_back(Lexer->CurTokenText);

  Lexer->Advance();
  while (Lexer->CurToken == TOKEN_COMMA) {
    Lexer->Advance();
    if (!expectSymbol(TOKEN_IDENTIFIER)) {
      return false;
    }
    ParamNames.push_back(Lexer->CurTokenText);
    Lexer->Advance();
  }

  TypeRef ParamType;
  if (Lexer->CurToken == TOKEN_COLON) {
    Lexer->Advance();
    if (!parseTypeRef(&ParamType)) {
      return false;
    }
  }

  for (auto Name : ParamNames) {
    proc->Params.push_back(new ProcedureParamAST(Name, ParamType));
  }

  return true;
}

Statement* Parser::parseStatement() {
  std::unique_ptr<Statement> Result;
  switch (Lexer->CurToken) {
  case TOKEN_RETURN:
    Result.reset(parseReturnStatement());
    break;

  case TOKEN_VAR:
    Result.reset(parseVarStatement());
    break;

  case TOKEN_COMMENT:
    Result.reset(new EmptyStatement());
    break;

  default:
    reportError("Expected a statement");
    while (Lexer->CurToken != TOKEN_NEWLINE && Lexer->CurToken != TOKEN_EOF) {
      Lexer->Advance();
    }
    Lexer->Advance();
    return nullptr;
  }

  if (Lexer->CurToken == TOKEN_COMMENT) {
    Result->Comment = Lexer->CurTokenText;
    Lexer->Advance();
  }

  if (!expectSymbol(TOKEN_NEWLINE)) {
    Lexer->Advance();
    return nullptr;
  }

  Lexer->Advance();
  return Result.release();
}

ReturnStatement* Parser::parseReturnStatement() {
  std::unique_ptr<ReturnStatement> Result(new ReturnStatement());
  if (!expectSymbol(TOKEN_RETURN)) {
    return nullptr;
  }

  Lexer->Advance();
  if (isAtExprStart()) {
    Result->Result.reset(parseExpr());
  }

  return Result.release();
}

VarStatement* Parser::parseVarStatement() {
  if (!expectSymbol(TOKEN_VAR)) {
    return nullptr;
  }

  Lexer->Advance();
  std::unique_ptr<VarStatement> result(new VarStatement());
  if (!parseVarDecls(&result->Vars)) {
    return nullptr;
  }

  while (Lexer->CurToken == TOKEN_SEMICOLON) {
    Lexer->Advance();
    if (!parseVarDecls(&result->Vars)) {
      return nullptr;
    }
  }

  return result.release();
}

bool Parser::parseVarDecls(VarDecls* Decls) {
  llvm::SmallVector<llvm::StringRef, 4> VarNames;
  if (!expectSymbol(TOKEN_IDENTIFIER)) {
    return false;
  }
  VarNames.push_back(Lexer->CurTokenText);
  Lexer->Advance();
  while (Lexer->CurToken == TOKEN_COMMA) {
    Lexer->Advance();
    if (!expectSymbol(TOKEN_IDENTIFIER)) {
      return false;
    }
    VarNames.push_back(Lexer->CurTokenText);
    Lexer->Advance();
  }

  TypeRef VarType;
  if (Lexer->CurToken == TOKEN_COLON) {
    Lexer->Advance();
    if (!parseTypeRef(&VarType)) {
      return false;
    }
  }

  for (auto Name : VarNames) {
    Decls->push_back(new VarDecl(Name, VarType));
  }
  return true;
}

bool Parser::expectSymbol(TokenType Token) {
  if (LLVM_UNLIKELY(Lexer->CurToken != Token)) {
    std::string Err, Found;
    switch (Token) {
    case TOKEN_NEWLINE: Err = u8"Expected end of line"; break;
    case TOKEN_INDENT: Err = u8"Expected indentation"; break;
    case TOKEN_UNINDENT: Err = u8"Expected un-indentation"; break;
    case TOKEN_IDENTIFIER: Err = u8"Expected an identifier"; break;
    case TOKEN_COLON: Err = u8"Expected ‘:’"; break;
    case TOKEN_LEFT_PARENTHESIS: Err = u8"Expected ‘(’"; break;
    case TOKEN_RIGHT_PARENTHESIS: Err = u8"Expected ‘)’"; break;
    default: Err = "Expected something different"; break;
    }
    switch (Lexer->CurToken) {
    case TOKEN_NEWLINE: Found = "end of line"; break;
    case TOKEN_INDENT: Found = "indentation"; break;
    case TOKEN_UNINDENT: Found = "un-indentation"; break;
    case TOKEN_COMMENT: Found = "comment"; break;
    default: Found = u8"‘" + Lexer->CurTokenText.str() + u8"’";
    }
    reportError(Err + u8", found " + Found);
    return false;
  }
  return true;
}

void Parser::reportError(const std::string& Error) {
  std::cerr << Error << std::endl;
}

}  // namespace firc
