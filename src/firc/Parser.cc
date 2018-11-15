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

firc::FileAST* Parser::ParseFile(const llvm::MemoryBuffer* buf,
                                 llvm::BumpPtrAllocator* allocator) {
  firc::Lexer lexer(buf, allocator);
  firc::Parser parser(&lexer);
  parser.Parse();
  return parser.FileAST.release();
}

Parser::Parser(firc::Lexer* Lexer)
  : Lexer(Lexer), FileAST(new firc::FileAST()) {
}

Parser::~Parser() {
}

void Parser::Parse() {
  while (Lexer->Advance()) {
    if (Lexer->CurToken == TOKEN_PROC) {
      std::unique_ptr<ProcedureAST> p(ParseProcedure());
      if (p.get() != nullptr) {
        FileAST->Procedures.push_back(p.release());
      }
      continue;
    }
  }
}

bool Parser::ParseTypeRef(TypeRef* T) {
  if (!ExpectSymbol(TOKEN_IDENTIFIER)) {
    return false;
  }
  T->QualifiedName.push_back(Lexer->CurTokenText);
  Lexer->Advance();
  while (Lexer->CurToken == TOKEN_DOT) {
    Lexer->Advance();
    if (!ExpectSymbol(TOKEN_IDENTIFIER)) {
      return false;
    }
    T->QualifiedName.push_back(Lexer->CurTokenText);
    Lexer->Advance();
  }
  return true;
}

ProcedureAST* Parser::ParseProcedure() {
  assert(Lexer->CurToken == TOKEN_PROC);
  Lexer->Advance();
  if (!ExpectSymbol(TOKEN_IDENTIFIER)) {
    return nullptr;
  }

  std::unique_ptr<ProcedureAST> result(new ProcedureAST(Lexer->CurTokenText));

  Lexer->Advance();
  if (!ExpectSymbol(TOKEN_LEFT_PARENTHESIS)) {
    return nullptr;
  }

  Lexer->Advance();
  if (Lexer->CurToken != TOKEN_RIGHT_PARENTHESIS) {
    if (!ParseProcedureParams(result.get())) {
      return nullptr;
    }
    while (Lexer->CurToken == TOKEN_SEMICOLON) {
      Lexer->Advance();
      if (!ParseProcedureParams(result.get())) {
        return nullptr;
      }
    }
  }

  if (!ExpectSymbol(TOKEN_RIGHT_PARENTHESIS)) {
    return nullptr;
  }

  Lexer->Advance();
  if (!ExpectSymbol(TOKEN_COLON)) {
    return nullptr;
  }

  Lexer->Advance();
  if (Lexer->CurToken != TOKEN_NEWLINE) {  // TODO: TOKEN_COMMENT
    if (!ParseTypeRef(&result->ResultType)) {
      return nullptr;
    }
  }

  if (!ExpectSymbol(TOKEN_NEWLINE)) {
    return nullptr;
  }

  Lexer->Advance();
  if (!ExpectSymbol(TOKEN_INDENT)) {
    return nullptr;
  }

  Lexer->Advance();
  if (!ExpectSymbol(TOKEN_RETURN)) {
    return nullptr;
  }

  Lexer->Advance();
  if (!ExpectSymbol(TOKEN_NEWLINE)) {
    return nullptr;
  }

  Lexer->Advance();
  if (!ExpectSymbol(TOKEN_UNINDENT)) {
    return nullptr;
  }

  return result.release();
}

bool Parser::ParseProcedureParams(ProcedureAST* proc) {
  llvm::SmallVector<llvm::StringRef, 4> ParamNames;
  if (!ExpectSymbol(TOKEN_IDENTIFIER)) {
    return false;
  }
  ParamNames.push_back(Lexer->CurTokenText);

  Lexer->Advance();
  while (Lexer->CurToken == TOKEN_COMMA) {
    Lexer->Advance();
    if (!ExpectSymbol(TOKEN_IDENTIFIER)) {
      return false;
    }
    ParamNames.push_back(Lexer->CurTokenText);
    Lexer->Advance();
  }

  TypeRef ParamType;
  if (Lexer->CurToken == TOKEN_COLON) {
    Lexer->Advance();
    if (!ParseTypeRef(&ParamType)) {
      return false;
    }
  }

  for (auto Name : ParamNames) {
    proc->Params.push_back(new ProcedureParamAST(Name, ParamType));
  }

  return true;
}

bool Parser::ExpectSymbol(TokenType Token) {
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
    default: Found = u8"‘" + Lexer->CurTokenText.str() + u8"’";
    }
    ReportError(Err + u8", found " + Found);
    return false;
  }
  return true;
}

void Parser::ReportError(const std::string& Error) {
  std::cerr << Error << std::endl;
}

}  // namespace firc
