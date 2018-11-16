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

#ifndef FIRC_LEXER_H_
#define FIRC_LEXER_H_

#include <memory>
#include <string>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/UnicodeCharRanges.h>

namespace llvm {
class MemoryBuffer;
}  // namespace llvm

namespace firc {

enum TokenType {
  TOKEN_ERROR_UNEXPECTED_CHAR = -3,
  TOKEN_ERROR_MALFORMED_UNICODE = -2,
  TOKEN_ERROR_INDENT_MISMATCH = -1,
  TOKEN_EOF = 0, TOKEN_NEWLINE = 1,

  TOKEN_INDENT, TOKEN_UNINDENT, TOKEN_COMMENT, TOKEN_IDENTIFIER, TOKEN_INTEGER,
  TOKEN_LEFT_PARENTHESIS, TOKEN_RIGHT_PARENTHESIS,
  TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
  TOKEN_COLON, TOKEN_SEMICOLON, TOKEN_COMMA, TOKEN_DOT, TOKEN_EQUAL,
  TOKEN_PLUS, TOKEN_MINUS, TOKEN_ASTERISK, TOKEN_SLASH, TOKEN_PERCENT,

  TOKEN_AND = 100, TOKEN_CLASS, TOKEN_CONST, TOKEN_ELSE, TOKEN_FOR,
  TOKEN_IF, TOKEN_IMPORT, TOKEN_IN, TOKEN_IS, TOKEN_NIL, TOKEN_NOT,
  TOKEN_OR, TOKEN_PROC, TOKEN_RETURN, TOKEN_VAR, TOKEN_WHILE,
  TOKEN_WITH, TOKEN_YIELD,
};

class Lexer {
public:
  Lexer(const llvm::MemoryBuffer* buf,
        llvm::BumpPtrAllocator* allocator);
  ~Lexer();
  bool Advance();
  void skipAnythingIndented();
  TokenType CurToken, NextToken;
  llvm::StringRef CurTokenText, NextTokenText;

private:
  const uint32_t EndOfFile = 0xFFFFFFFF;

  const unsigned char *BufferPos, *BufferEnd;
  const unsigned char *CurCharPos, *NextCharPos;
  uint32_t CurChar, NextChar;
  uint32_t Line, Column;

  llvm::SmallVector<uint32_t, 16> Indents;
  llvm::BumpPtrAllocator* Allocator;

  static const llvm::sys::UnicodeCharSet
      IDStartChars, IDPartChars, WhitespaceChars, PossiblyNotNFKCChars;

  bool isDigit(uint32_t c) const {
    return c >= 0x30 && c <= 0x39;
  }

  bool isLineSeparator(uint32_t CurChar, uint32_t NextChar) const {
    return (CurChar == 0x000A || CurChar == 0x000B || CurChar == 0x000C ||
            (CurChar == 0x000D && NextChar != 0x000A) ||
            CurChar == 0x0085 || CurChar == 0x2028 || CurChar == 0x2029);
  }

  bool isWhitespace(uint32_t c) const {
    if (LLVM_LIKELY(c <= 0x7f)) {
      return (c >= 9 && c <= 13) || (c == 0x20);
    } else {
      return WhitespaceChars.contains(c);
    }
  }

  bool isIdentifierStart(uint32_t c) const {
    if (LLVM_LIKELY(c <= 0x7f)) {
      return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_');
    } else {
      return IDStartChars.contains(c);
    }
  }

  bool isIdentifierPart(uint32_t c) const {
    if (LLVM_LIKELY(c <= 0x7f)) {
      return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
             (c >= '0' && c <= '9') || (c == '_');
    } else {
      return IDPartChars.contains(c);
    }
  }

  // Returns whether character c is certainly in NFKC form.
  // If the result is false, the character might or might not be part of
  // a non-NFKC sequence.
  inline bool isCertainlyNFKC(uint32_t c) const {
    if (LLVM_LIKELY(c <= 0x7f)) {
      return true;
    } else {
      return !PossiblyNotNFKCChars.contains(c);
    }
  }

  // The Unicode Standard 11.0, chapter 3.12 “Conjoining Jamo Behavior”,
  // page 144, section “Common Constants”.
  // https://www.unicode.org/versions/Unicode11.0.0/UnicodeStandard-11.0.pdf
  static const uint32_t HangulSBase = 0xAC00;
  static const uint32_t HangulLBase = 0x1100;
  static const uint32_t HangulVBase = 0x1161;
  static const uint32_t HangulTBase = 0x11A7;
  static const uint32_t HangulLCount = 19;
  static const uint32_t HangulVCount = 21;
  static const uint32_t HangulTCount = 28;
  static const uint32_t HangulNCount = HangulVCount * HangulTCount;
  static const uint32_t HangulSCount = HangulLCount * HangulNCount;
  static_assert(HangulNCount == 588, "NCount should match Unicode standard");
  static_assert(HangulSCount == 11172, "SCount should match Unicode");

  inline bool isHangulSyllable(uint32_t c) const {
    return c >= HangulSBase && c < HangulSBase + HangulSCount;
  }

  void AdvanceChar();
  void SkipWhitespace(bool AlsoSkipLineSeparators);

  llvm::StringRef ConvertToNFKC(const llvm::StringRef UTF8);
  bool Decompose(const llvm::StringRef UTF8,
                 llvm::SmallVector<uint32_t, 16> *Decomposed);
  static uint8_t getCombiningCharClass(uint32_t c);
  static bool isCCCSmaller(uint32_t a, uint32_t b);
  void Compose(llvm::SmallVector<uint32_t, 16> *Text);
  int32_t ComposeChars(uint32_t a, uint32_t b) const;

  struct CharDecomposition {
    uint32_t Codepoint;
    unsigned int DecompositionLength : 8;
    unsigned int Decomposition : 24;
  };

  static const CharDecomposition CharDecompositions[];
  static const uint32_t NumCharDecompositions;
  static const uint32_t CharDecompositionData[];

  struct CCCEntry {  // Unicode Combining Character Class
    unsigned int Codepoint : 24;
    unsigned int CombiningCharClass : 8;
  };

  static const uint16_t NumCCCEntries;
  static const CCCEntry CCCEntries[];

  struct CharComposition {
    uint32_t First;
    uint32_t Second;
    uint32_t Composed;
  };

  static const CharComposition CharCompositions[];
  static const uint32_t NumCharCompositions;

  static int CompareCharDecompositions(const void *A, const void *B);
  static int CompareCharCompositions(const void *A, const void *B);
  static int CompareCCCEntries(const void *A, const void *B);
};

}  // namespace firc

#endif // FIRC_LEXER_H_
