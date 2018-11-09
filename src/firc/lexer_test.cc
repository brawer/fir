#include <memory>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/MemoryBuffer.h>

#include "firc/lexer.h"
#include "gtest/gtest.h"

namespace firc {

std::string RunLexer(llvm::StringRef s) {
  std::string result;
  llvm::BumpPtrAllocator allocator;
  std::unique_ptr<llvm::MemoryBuffer> buf(llvm::MemoryBuffer::getMemBuffer(s));
  firc::Lexer lexer(buf.get(), &allocator);
  while (lexer.Advance()) {
    if (!result.empty()) {
      result += '|';
    }
    switch (lexer.CurToken) {
    case TOKEN_ERROR_INDENT_MISMATCH:
      result += "ERROR_INDENT_MISMATCH";
      break;

    case TOKEN_EOF:
      result += "EOF";
      break;

    case TOKEN_NEWLINE:
      result += "NEWLINE";
      break;

    case TOKEN_INDENT:
      result += "INDENT";
      break;

    case TOKEN_UNINDENT:
      result += "UNINDENT";
      break;

    case TOKEN_IDENTIFIER:
      result += "ID";
      break;

    default:
      result += "???";
      break;
    }

    if (!lexer.CurTokenText.empty()) {
      result += '[';
      result += lexer.CurTokenText.str();
      result += ']';
    }
  }
  return result;
}

TEST(LexerTest, ShouldIgnoreByteOrderMark) {
  EXPECT_EQ(RunLexer("\xEF\xBB\xBF"), "");
  EXPECT_EQ(RunLexer("\xEF\xBB\xBF" "Foo\n"), u8"ID[Foo]|NEWLINE");
}

TEST(LexerTest, Identifier) {
  EXPECT_EQ(RunLexer(u8"Foo"), u8"ID[Foo]");
  EXPECT_EQ(RunLexer(u8"Foo123"), u8"ID[Foo123]");
  EXPECT_EQ(RunLexer(u8"識別子"), u8"ID[識別子]");
  EXPECT_EQ(RunLexer(u8"شناختساز"), u8"ID[شناختساز]");
}

TEST(LexerTest, Identifier_HangulSyllables) {
  // Decomposed hangul syllables should get composed.
  EXPECT_EQ(RunLexer(u8"\u1111\u1171"), u8"ID[\uD4CC]");
  EXPECT_EQ(RunLexer(u8"\u1111\u1171\u11B6"), u8"ID[\uD4DB]");

  // Already pre-composed hangul syllables should stay composed.
  EXPECT_EQ(RunLexer(u8"\uD4CC"), u8"ID[\uD4CC]");
  EXPECT_EQ(RunLexer(u8"\uD4DB"), u8"ID[\uD4DB]");
}

TEST(LexerTest, Identifier_ShouldConvertToNFKC) {
  EXPECT_EQ(RunLexer(u8"ＦｕｌｌｗｉｄｔｈX１２３"), u8"ID[FullwidthX123]");
  EXPECT_EQ(RunLexer(u8"\u217B"), u8"ID[xii]"); // SMALL ROMAN NUMERAL TWELVE
}

TEST(LexerTest, DISABLED_Identifier_ShouldConvertToNFKC_NotWorkingYet) {
  EXPECT_EQ(RunLexer(u8"ｼｷﾍﾞﾂｼ"), u8"ID[シキベツシ]");  // needs compose

  // U+01EE LATIN CAPITAL LETTER EZH WITH CARON [Ǯ]
  EXPECT_EQ(RunLexer(u8"\u01B7\u030C"), u8"ID[Ǯ]");
}

TEST(LexerTest, Indent) {
  EXPECT_EQ(RunLexer("A\n  B\n    C\n    C\n  B\nA\n"),
            "ID[A]|NEWLINE|"            // A
            "INDENT|ID[B]|NEWLINE|"     //   B
            "INDENT|ID[C]|NEWLINE|"     //     C
            "ID[C]|NEWLINE|"            //     C
            "UNINDENT|ID[B]|NEWLINE|"   //   B
            "UNINDENT|ID[A]|NEWLINE");  // A
  EXPECT_EQ(RunLexer("A\n  B\n    C\n    C\nA\n"),
            "ID[A]|NEWLINE|"            // A
            "INDENT|ID[B]|NEWLINE|"     //   B
            "INDENT|ID[C]|NEWLINE|"     //     C
            "ID[C]|NEWLINE|"            //     C
            "UNINDENT|"                 //   <implicit unindent>
            "UNINDENT|ID[A]|NEWLINE");  // A
  EXPECT_EQ(RunLexer("A\n  B\n    C\n    C\n      D\nA\n"),
            "ID[A]|NEWLINE|"            // A
            "INDENT|ID[B]|NEWLINE|"     //   B
            "INDENT|ID[C]|NEWLINE|"     //     C
            "ID[C]|NEWLINE|"            //     C
            "INDENT|ID[D]|NEWLINE|"     //       D
            "UNINDENT|"                 //   <implicit unindent>
            "UNINDENT|"                 //   <implicit unindent>
            "UNINDENT|ID[A]|NEWLINE");  // A
}

TEST(LexerTest, Indent_ShouldDetectMismatch) {
  EXPECT_EQ(RunLexer("A\n    B\n        C\n  X"),
            "ID[A]|NEWLINE|"              // A
	    "INDENT|ID[B]|NEWLINE|"       //     B
	    "INDENT|ID[C]|NEWLINE|"       //         C
            "ERROR_INDENT_MISMATCH");     //   X
}

TEST(LexerTest, Indent_ShouldUnindentAtEndOfFile) {
  EXPECT_EQ(RunLexer("A\n  B\n    C\n"),
            "ID[A]|NEWLINE|"            // A
            "INDENT|ID[B]|NEWLINE|"     //   B
            "INDENT|ID[C]|NEWLINE|"     //     C
            "UNINDENT|UNINDENT");       // <2 implicit unindents>
}

}  // namespace firc
