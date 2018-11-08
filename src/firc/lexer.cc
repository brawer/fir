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

#include "firc/lexer.h"

#include <algorithm>
#include <cstdlib>
#include <stdio.h>

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/ConvertUTF.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/UnicodeCharRanges.h>

namespace firc {

Lexer::Lexer(const llvm::MemoryBuffer* buffer,
             llvm::BumpPtrAllocator* allocator)
  : BufferPos(
        reinterpret_cast<const unsigned char*>(buffer->getBufferStart())),
    BufferEnd(reinterpret_cast<const unsigned char*>(buffer->getBufferEnd())),
    CurCharPos(BufferPos), NextCharPos(BufferPos),
    CurChar(0), NextChar(0x000A), Line(0), Column(0),
    CurToken(TOKEN_EOF), NextToken(TOKEN_EOF), Allocator(allocator) {
  // Skip file-initial U+FEFF Byte Order Mark, which is used by
  // some Windows editors to indicate UTF-8 encoding.
  if (BufferPos + 3 <= BufferEnd &&
      BufferPos[0] == 0xEF && BufferPos[1] == 0xBB && BufferPos[2] == 0xBF) {
    BufferPos += 3;
    CurCharPos += 3;
    NextCharPos += 3;
  }
  AdvanceChar();
  AdvanceChar();
  Advance();
}

Lexer::~Lexer() {
}

void Lexer::AdvanceChar() {
  CurChar = NextChar;
  CurCharPos = NextCharPos;

  if (isLineSeparator(CurChar, NextChar)) {
    Line += 1;
    Column = 0;
  } else {
    Column += 1;
  }

  llvm::UTF32 c;
  NextCharPos = BufferPos;
  if (LLVM_LIKELY(BufferPos < BufferEnd)) {
    const llvm::ConversionResult result = llvm::convertUTF8Sequence(
        &BufferPos, BufferEnd, &c, llvm::strictConversion);
    if (LLVM_LIKELY(result == llvm::conversionOK)) {
      NextChar = static_cast<uint32_t>(c);
    } else {
      NextChar = 0xFFFD;
    }
  } else {
    NextChar = EndOfFile;
  }
}

bool Lexer::Advance() {
  CurToken = NextToken;
  CurTokenText = NextTokenText;

  if (CurToken < 0) {
    NextToken = TOKEN_EOF;
    NextTokenText = llvm::StringRef();
    return true;
  }

  if (LLVM_UNLIKELY(CurChar == Lexer::EndOfFile)) {
    if (Indents.empty()) {
      NextToken = TOKEN_EOF;
    } else {
      NextToken = TOKEN_UNINDENT;
      Indents.pop_back();
    }
    NextTokenText = llvm::StringRef();
    return CurToken > TOKEN_EOF;
  }

  if (Column == 1) {
    SkipWhitespace();
    const uint32_t NumSpaces = Column - 1;
    const uint32_t IndentPos = Indents.empty() ? 0 : Indents.back();
    if (NumSpaces > IndentPos) {
      Indents.push_back(NumSpaces);
      NextToken = TOKEN_INDENT;
      NextTokenText = llvm::StringRef();
      return CurToken > TOKEN_EOF;
    } else if (NumSpaces < IndentPos) {
      if (!Indents.empty()) {
        Indents.pop_back();
      }
      if (NumSpaces > 0 &&
          std::find(Indents.begin(), Indents.end(), NumSpaces) ==
              Indents.end()) {
        NextToken = TOKEN_ERROR_INDENT_MISMATCH;
	NextTokenText = llvm::StringRef();
	return CurToken > TOKEN_EOF;
      }
      NextToken = TOKEN_UNINDENT;
      NextTokenText = llvm::StringRef();
      return CurToken > TOKEN_EOF;
    }
  }

  if (isLineSeparator(CurChar, NextChar)) {
    NextToken = TOKEN_NEWLINE;
    NextTokenText = llvm::StringRef();
    AdvanceChar();
    return CurToken > TOKEN_EOF;
  }

  if (isIdentifierStart(CurChar)) {
    const char* IDStart = reinterpret_cast<const char*>(CurCharPos);
    bool Normalized = true;
    NextToken = TOKEN_IDENTIFIER;
    do {
      Normalized = Normalized && isCertainlyNFKC(CurChar);
      if (CurChar == 0x3300) {
        printf("********** Normalized: %d\n", Normalized);
      }
      AdvanceChar();
    } while (isIdentifierPart(CurChar));
    const char* IDEnd = reinterpret_cast<const char*>(CurCharPos);
    NextTokenText = llvm::StringRef(IDStart, IDEnd - IDStart);
    if (!Normalized) {
      NextTokenText = ConvertToNFKC(NextTokenText);
      if (LLVM_UNLIKELY(NextTokenText.empty())) {
        NextToken = TOKEN_ERROR_MALFORMED_UNICODE;
        return CurToken > TOKEN_EOF;
      }
    }
    return CurToken > TOKEN_EOF;
  }

  NextToken = TOKEN_EOF;
  NextTokenText = llvm::StringRef();
  return CurToken > TOKEN_EOF;
}

void Lexer::SkipWhitespace() {
  for (; CurChar != EndOfFile && isWhitespace(CurChar); AdvanceChar()) {
  }
}

llvm::StringRef Lexer::ConvertToNFKC(const llvm::StringRef UTF8) {
  // Decompose to NFKD.
  llvm::SmallVector<uint32_t, 16> Decomposed;
  if (LLVM_UNLIKELY(!Decompose(UTF8, &Decomposed))) {
    return llvm::StringRef();
  }

  // TODO: Re-compose.

  // Convert from UTF32 to UTF8 buffer.
  llvm::SmallVector<uint8_t, 64> Converted(
      Decomposed.size() * UNI_MAX_UTF8_BYTES_PER_CODE_POINT);
  const uint32_t *SourceStart = Decomposed.data();
  const uint32_t *SourceEnd = SourceStart + Decomposed.size();
  uint8_t *TargetStart = Converted.data();
  uint8_t *TargetEnd = TargetStart + Converted.size();
  const llvm::ConversionResult ConvResult = llvm::ConvertUTF32toUTF8(
      &SourceStart, SourceEnd, &TargetStart, TargetEnd,
       llvm::strictConversion);
  if (LLVM_UNLIKELY(ConvResult != llvm::conversionOK)) {
    return llvm::StringRef();
  }
  size_t ConvertedLength = TargetStart - Converted.data();

  // Copy UTF8 buffer into BumpPtrAllocator arena.
  char *Result = static_cast<char*>(
      Allocator->Allocate(Converted.size(), /* alignment */ 1));
  memcpy(Result, Converted.data(), ConvertedLength);

  return llvm::StringRef(Result, ConvertedLength);
}

bool Lexer::Decompose(const llvm::StringRef UTF8,
                      llvm::SmallVector<uint32_t, 16> *Decomposed) {
  const unsigned char *BufferPos = UTF8.bytes_begin();
  const unsigned char *BufferEnd = UTF8.bytes_end();
  llvm::UTF32 c;
  while (LLVM_LIKELY(BufferPos < BufferEnd)) {
    if (LLVM_LIKELY(*BufferPos <= 0x7F)) {  // fast path for ASCII
      Decomposed->push_back(*BufferPos);
      ++BufferPos;
      continue;
    }

    const llvm::ConversionResult ConvResult = llvm::convertUTF8Sequence(
        &BufferPos, BufferEnd, &c, llvm::strictConversion);
    if (LLVM_UNLIKELY(ConvResult != llvm::conversionOK)) {
      return false;
    }

    const CharDecomposition Key = {c, 0, 0};
    const CharDecomposition *Value =
        reinterpret_cast<const CharDecomposition*>(bsearch(
	    &Key, CharDecompositions, NumCharDecompositions,
            sizeof(CharDecomposition), Lexer::CompareCharDecompositions));
    if (Value != nullptr) {
      if (Value->DecompositionLength == 1) {
        Decomposed->push_back(Value->Decomposition);
      } else {
        for (int i = 0; i < Value->DecompositionLength; ++i) {
          uint32_t Decomp = CharDecompositionData[Value->Decomposition + i];
          Decomposed->push_back(Decomp);
        }
      }
    } else {
      Decomposed->push_back(c);
    }
  }

  // TODO: Sort combining characters by Combining Character Class.
  return true;
}

int Lexer::CompareCharDecompositions(const void *A, const void *B) {
  const uint32_t CA = reinterpret_cast<const CharDecomposition*>(A)->Codepoint;
  const uint32_t CB = reinterpret_cast<const CharDecomposition*>(B)->Codepoint;
  if (CA < CB) {
    return -1;
  } else if (CA > CB) {
    return 1;
  } else {
    return 0;
  }
}

}  // namespace firc
