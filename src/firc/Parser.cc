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

firc::FileAST* Parser::parseFile(llvm::StringRef Filename,
                                 llvm::StringRef Directory,
                                 const llvm::MemoryBuffer* Buf,
                                 llvm::BumpPtrAllocator* Allocator,
                                 ErrorHandler ErrHandler) {
  firc::Lexer lexer(Filename, Directory, Buf, Allocator);
  firc::Parser parser(&lexer, ErrHandler);
  parser.parse();
  return parser.FileAST.release();
}

Parser::Parser(firc::Lexer* Lexer, ErrorHandler ErrHandler)
  : Lexer(Lexer), ErrHandler(ErrHandler),
    FileAST(new firc::FileAST(Lexer->Filename, Lexer->Directory)) {
}

Parser::~Parser() {
}

void Parser::parse() {
  while (Lexer->Advance()) {
    std::unique_ptr<Statement> TopLevelStatement;
    switch (Lexer->CurToken) {
    case TOKEN_NEWLINE:
    case TOKEN_COMMENT:
    case TOKEN_CONST:
    case TOKEN_PROC:
    case TOKEN_VAR:
      TopLevelStatement.reset(parseStatement());
      break;

    default:
      SourceLocation Loc;
      setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &Loc);
      reportError("Expected const, proc, var, or comment", Loc);
      break;
    }

    if (TopLevelStatement) {
      FileAST->Body.push_back(TopLevelStatement.release());
    } else {
      Lexer->Advance();
      Lexer->skipAnythingIndented();
    }
  }
}

bool Parser::parseTypeRef(TypeRef* T) {
  setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &T->Location);
  T->Optional = false;
  if (Lexer->CurToken == TOKEN_OPTIONAL) {
    T->Optional = true;
    Lexer->Advance();
  }

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
  return Lexer->CurToken == TOKEN_INTEGER || Lexer->CurToken == TOKEN_NIL;
}

Expr* Parser::parseExpr() {
  switch (Lexer->CurToken) {
  case TOKEN_INTEGER: {
    std::unique_ptr<IntExpr> IntEx(
        new IntExpr(llvm::APSInt(Lexer->CurTokenText)));
    setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &IntEx->Location);
    Lexer->Advance();
    return IntEx.release();
  }

  case TOKEN_NIL: {
    std::unique_ptr<NilExpr> NilEx(new NilExpr());
    setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &NilEx->Location);
    Lexer->Advance();
    return NilEx.release();
  }

  default: {
    SourceLocation Loc;
    setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &Loc);
    reportError("Expected expression", Loc);
    return nullptr;
  }
  }
}

ProcedureAST* Parser::parseProcedure() {
  assert(Lexer->CurToken == TOKEN_PROC);
  uint32_t Line = Lexer->CurTokenLine;
  uint32_t Column = Lexer->CurTokenColumn;
  Lexer->Advance();
  if (!expectSymbol(TOKEN_IDENTIFIER)) {
    return nullptr;
  }

  std::unique_ptr<ProcedureAST> Result(new ProcedureAST(Lexer->CurTokenText));
  setLocation(Line, Column, &Result->Location);
  Lexer->Advance();
  if (!expectSymbol(TOKEN_LEFT_PARENTHESIS)) {
    return nullptr;
  }

  Lexer->Advance();
  if (Lexer->CurToken != TOKEN_RIGHT_PARENTHESIS) {
    std::unique_ptr<VarDecl> Decl(parseVarDecl());
    if (!Decl) {
      return nullptr;
    }
    Result->Params.push_back(Decl.release());
    while (Lexer->CurToken == TOKEN_SEMICOLON) {
      Lexer->Advance();
      Decl.reset(parseVarDecl());
      if (!Decl) {
        return nullptr;
      }
      Result->Params.push_back(Decl.release());
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
  if (Lexer->CurToken != TOKEN_NEWLINE && Lexer->CurToken != TOKEN_COMMENT) {
    if (!parseTypeRef(&Result->ResultType)) {
      return nullptr;
    }
  }

  if (Lexer->CurToken == TOKEN_COMMENT) {
    Result->Comment = Lexer->CurTokenText;
    Lexer->Advance();
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
      Result->Body.push_back(S);
    }
  }

  if (!expectSymbol(TOKEN_UNINDENT)) {
    return nullptr;
  }

  return Result.release();
}

Statement* Parser::parseStatement() {
  std::unique_ptr<Statement> Result;
  bool SingleLine = true;
  switch (Lexer->CurToken) {
  case TOKEN_CONST:
    Result.reset(parseConstStatement());
    break;

  case TOKEN_PROC:
    SingleLine = false;
    Result.reset(parseProcedure());
    break;

  case TOKEN_RETURN:
    Result.reset(parseReturnStatement());
    break;

  case TOKEN_VAR:
    Result.reset(parseVarStatement());
    break;

  case TOKEN_COMMENT:
    Result.reset(new EmptyStatement());
    break;

  default: {
    SourceLocation Loc;
    setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &Loc);
    reportError("Expected a statement", Loc);
    while (Lexer->CurToken != TOKEN_NEWLINE && Lexer->CurToken != TOKEN_EOF) {
      Lexer->Advance();
    }
    Lexer->Advance();
    return nullptr;
  }
  }

  if (SingleLine && Lexer->CurToken == TOKEN_COMMENT) {
    Result->Comment = Lexer->CurTokenText;
    Lexer->Advance();
  }

  if (SingleLine && !expectSymbol(TOKEN_NEWLINE)) {
    Lexer->Advance();
    return nullptr;
  }

  Lexer->Advance();
  return Result.release();
}

ReturnStatement* Parser::parseReturnStatement() {
  std::unique_ptr<ReturnStatement> Result(new ReturnStatement());
  setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &Result->Location);
  if (!expectSymbol(TOKEN_RETURN)) {
    return nullptr;
  }

  Lexer->Advance();
  if (isAtExprStart()) {
    Result->Result.reset(parseExpr());
  }

  return Result.release();
}

ConstStatement* Parser::parseConstStatement() {
  std::unique_ptr<ConstStatement> Result(new ConstStatement());
  setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &Result->Location);
  if (!expectSymbol(TOKEN_CONST)) {
    return nullptr;
  }

  Lexer->Advance();
  std::unique_ptr<VarDecl> Decl(parseConstDecl());
  if (!Decl) {
    return nullptr;
  }
  Result->Consts.push_back(Decl.release());

  while (Lexer->CurToken == TOKEN_SEMICOLON) {
    Lexer->Advance();
    Decl.reset(parseConstDecl());
    if (!Decl) {
      return nullptr;
    }
    Result->Consts.push_back(Decl.release());
  }

  return Result.release();
}

VarStatement* Parser::parseVarStatement() {
  std::unique_ptr<VarStatement> Result(new VarStatement());
  setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &Result->Location);
  if (!expectSymbol(TOKEN_VAR)) {
    return nullptr;
  }

  Lexer->Advance();
  std::unique_ptr<VarDecl> Decl(parseVarDecl());
  if (!Decl) {
    return nullptr;
  }
  Result->Vars.push_back(Decl.release());

  while (Lexer->CurToken == TOKEN_SEMICOLON) {
    Lexer->Advance();
    Decl.reset(parseVarDecl());
    if (!Decl) {
      return nullptr;
    }
    Result->Vars.push_back(Decl.release());
  }

  return Result.release();
}

VarDecl* Parser::parseConstDecl() {
  VarDecl* Decl = parseVarDecl();
  if (Decl->VarNames.size() > 1) {
    reportError("Constants must be separated by ‘;’, not ‘,’", Decl->Location);
  }
  if (Decl && !Decl->Value) {
    reportError(std::string("Constant “") + Decl->VarNames[0].str() +
                "” must have a value", Decl->Location);
  }
  return Decl;
}

VarDecl* Parser::parseVarDecl() {
  Names VarNames;
  uint32_t Line = Lexer->CurTokenLine;
  uint32_t Column = Lexer->CurTokenColumn;
  if (!expectSymbol(TOKEN_IDENTIFIER)) {
    return nullptr;
  }
  VarNames.push_back(Lexer->CurTokenText);
  Lexer->Advance();
  while (Lexer->CurToken == TOKEN_COMMA) {
    Lexer->Advance();
    if (!expectSymbol(TOKEN_IDENTIFIER)) {
      return nullptr;
    }
    VarNames.push_back(Lexer->CurTokenText);
    Lexer->Advance();
  }

  TypeRef VarType;
  if (Lexer->CurToken == TOKEN_COLON) {
    Lexer->Advance();
    if (!parseTypeRef(&VarType)) {
      return nullptr;
    }
  }

  std::unique_ptr<Expr> Value;
  if (Lexer->CurToken == TOKEN_EQUAL) {
    Lexer->Advance();
    Value.reset(parseExpr());
  }

  std::unique_ptr<VarDecl> Result(
      new VarDecl(VarNames, VarType, Value.release()));
  setLocation(Line, Column, &Result->Location);
  return Result.release();
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

    SourceLocation Loc;
    setLocation(Lexer->CurTokenLine, Lexer->CurTokenColumn, &Loc);
    reportError(Err + u8", found " + Found, Loc);

    return false;
  }
  return true;
}

void Parser::reportError(const std::string& Error, const SourceLocation &Loc) {
  ErrHandler(Error, Loc);
}

void Parser::setLocation(uint32_t Line, uint32_t Column, SourceLocation *Loc) {
  Loc->File = FileAST.get();
  Loc->Line = Line;
  Loc->Column = Column;
}

}  // namespace firc
