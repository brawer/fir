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

#include <memory>
#include <llvm/Support/Allocator.h>
#include "firc/AST.h"
#include "firc/Lexer.h"

namespace llvm {
class MemoryBuffer;
}  // namespace llvm

namespace firc {

class Parser {
public:
  static firc::FileAST* ParseFile(
      const llvm::MemoryBuffer* buf,
      llvm::BumpPtrAllocator* allocator);

private:
  Parser(firc::Lexer* lexer);
  ~Parser();

  void Parse();
  bool ParseTypeRef(TypeRef* T);
  ProcedureAST* ParseProcedure();
  bool ParseProcedureParams(ProcedureAST* P);

  Statement* ParseStatement();
  ReturnStatement* ParseReturnStatement();

  bool ExpectSymbol(TokenType Token);
  void ReportError(const std::string& Error);

  firc::Lexer* Lexer;
  std::unique_ptr<firc::FileAST> FileAST;
};

}  // namespace firc

#endif // FIRC_PARSER_H_
