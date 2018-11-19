#include <memory>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/MemoryBuffer.h>

#include "firc/Lexer.h"
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
    case TOKEN_ERROR_UNEXPECTED_CHAR: result += "ERROR_UNEXPECTED_CHAR"; break;
    case TOKEN_ERROR_INDENT_MISMATCH: result += "ERROR_INDENT_MISMATCH"; break;
    case TOKEN_EOF: result += "EOF"; break;
    case TOKEN_NEWLINE: result += "NEWLINE"; break;
    case TOKEN_INDENT: result += "INDENT"; break;
    case TOKEN_UNINDENT: result += "UNINDENT"; break;
    case TOKEN_COMMENT: result += "COMMENT"; break;
    case TOKEN_IDENTIFIER: result += "ID"; break;
    case TOKEN_INTEGER: result += "INTEGER"; break;
    case TOKEN_LEFT_PARENTHESIS: result += "LEFT_PARENTHESIS"; break;
    case TOKEN_RIGHT_PARENTHESIS: result += "RIGHT_PARENTHESIS"; break;
    case TOKEN_LEFT_BRACKET: result += "LEFT_BRACKET"; break;
    case TOKEN_RIGHT_BRACKET: result += "RIGHT_BRACKET"; break;
    case TOKEN_COLON: result += "COLON"; break;
    case TOKEN_SEMICOLON: result += "SEMICOLON"; break;
    case TOKEN_COMMA: result += "COMMA"; break;
    case TOKEN_DOT: result += "DOT"; break;
    case TOKEN_EQUAL: result += "EQUAL"; break;
    case TOKEN_PLUS: result += "PLUS"; break;
    case TOKEN_MINUS: result += "MINUS"; break;
    case TOKEN_ASTERISK: result += "ASTERISK"; break;
    case TOKEN_SLASH: result += "SLASH"; break;
    case TOKEN_PERCENT: result += "PERCENT"; break;
    case TOKEN_AND: result += "AND"; break;
    case TOKEN_CLASS: result += "CLASS"; break;
    case TOKEN_CONST: result += "CONST"; break;
    case TOKEN_ELSE: result += "ELSE"; break;
    case TOKEN_FOR: result += "FOR"; break;
    case TOKEN_IF: result += "IF"; break;
    case TOKEN_IMPORT: result += "IMPORT"; break;
    case TOKEN_IN: result += "IN"; break;
    case TOKEN_IS: result += "IS"; break;
    case TOKEN_NIL: result += "NIL"; break;
    case TOKEN_NOT: result += "NOT"; break;
    case TOKEN_OPTIONAL: result += "OPTIONAL"; break;
    case TOKEN_OR: result += "OR"; break;
    case TOKEN_PROC: result += "PROC"; break;
    case TOKEN_RETURN: result += "RETURN"; break;
    case TOKEN_VAR: result += "VAR"; break;
    case TOKEN_WHILE: result += "WHILE"; break;
    case TOKEN_WITH: result += "WITH"; break;
    case TOKEN_YIELD: result += "YIELD"; break;
    default: result += "???"; break;
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
  EXPECT_EQ(RunLexer("\xEF\xBB\xBF" "Foo\n"), "ID[Foo]|NEWLINE");
}

TEST(LexerTest, Keywords) {
  EXPECT_EQ(RunLexer("and"), "AND[and]");
  EXPECT_EQ(RunLexer("class"), "CLASS[class]");
  EXPECT_EQ(RunLexer("const"), "CONST[const]");
  EXPECT_EQ(RunLexer("else"), "ELSE[else]");
  EXPECT_EQ(RunLexer("for"), "FOR[for]");
  EXPECT_EQ(RunLexer("if"), "IF[if]");
  EXPECT_EQ(RunLexer("import"), "IMPORT[import]");
  EXPECT_EQ(RunLexer("in"), "IN[in]");
  EXPECT_EQ(RunLexer("is"), "IS[is]");
  EXPECT_EQ(RunLexer("nil"), "NIL[nil]");
  EXPECT_EQ(RunLexer("not"), "NOT[not]");
  EXPECT_EQ(RunLexer("optional"), "OPTIONAL[optional]");
  EXPECT_EQ(RunLexer("or"), "OR[or]");
  EXPECT_EQ(RunLexer("proc"), "PROC[proc]");
  EXPECT_EQ(RunLexer("return"), "RETURN[return]");
  EXPECT_EQ(RunLexer("var"), "VAR[var]");
  EXPECT_EQ(RunLexer("while"), "WHILE[while]");
  EXPECT_EQ(RunLexer("with"), "WITH[with]");
  EXPECT_EQ(RunLexer("yield"), "YIELD[yield]");
}

TEST(LexerTest, Symbols) {
  EXPECT_EQ(RunLexer("("), "LEFT_PARENTHESIS[(]");
  EXPECT_EQ(RunLexer(")"), "RIGHT_PARENTHESIS[)]");
  EXPECT_EQ(RunLexer("["), "LEFT_BRACKET[[]");
  EXPECT_EQ(RunLexer("]"), "RIGHT_BRACKET[]]");
  EXPECT_EQ(RunLexer(":"), "COLON[:]");
  EXPECT_EQ(RunLexer(";"), "SEMICOLON[;]");
  EXPECT_EQ(RunLexer(","), "COMMA[,]");
  EXPECT_EQ(RunLexer("."), "DOT[.]");
  EXPECT_EQ(RunLexer("="), "EQUAL[=]");
  EXPECT_EQ(RunLexer("+"), "PLUS[+]");
  EXPECT_EQ(RunLexer("-"), "MINUS[-]");
  EXPECT_EQ(RunLexer("*"), "ASTERISK[*]");
  EXPECT_EQ(RunLexer("/"), "SLASH[/]");
  EXPECT_EQ(RunLexer("%"), "PERCENT[%]");
}

TEST(LexerTest, UnexpectedChar) {
  EXPECT_EQ(RunLexer("§"), "ERROR_UNEXPECTED_CHAR[§]");
  EXPECT_EQ(RunLexer("₩"), "ERROR_UNEXPECTED_CHAR[₩]");
}

TEST(LexerTest, Comment) {
  EXPECT_EQ(RunLexer("# Foo"), "COMMENT[Foo]");
  EXPECT_EQ(RunLexer("#   Foo Bar "), "COMMENT[Foo Bar]");
  EXPECT_EQ(RunLexer("#"), "COMMENT");
  EXPECT_EQ(RunLexer("#\n"), "COMMENT|NEWLINE");
  EXPECT_EQ(RunLexer("#\r"), "COMMENT|NEWLINE");
  EXPECT_EQ(RunLexer("#\r\n"), "COMMENT|NEWLINE");
  EXPECT_EQ(RunLexer("# "), "COMMENT");
  EXPECT_EQ(RunLexer("# \n"), "COMMENT|NEWLINE");
  EXPECT_EQ(RunLexer("# \r"), "COMMENT|NEWLINE");
  EXPECT_EQ(RunLexer("# \r\n"), "COMMENT|NEWLINE");
}

TEST(LexerTest, Identifier) {
  EXPECT_EQ(RunLexer("Foo"), "ID[Foo]");
  EXPECT_EQ(RunLexer("_Foo"), "ID[_Foo]");
  EXPECT_EQ(RunLexer("Foo_"), "ID[Foo_]");
  EXPECT_EQ(RunLexer("__Foo__"), "ID[__Foo__]");
  EXPECT_EQ(RunLexer("Foo123"), "ID[Foo123]");
  EXPECT_EQ(RunLexer("識別子"), "ID[識別子]");
  EXPECT_EQ(RunLexer("شناختساز"), "ID[شناختساز]");
}

TEST(LexerTest, Identifier_HangulSyllables) {
  // Decomposed hangul syllables should get composed.
  EXPECT_EQ(RunLexer("\u1111\u1171"), "ID[\uD4CC]");
  EXPECT_EQ(RunLexer("\u1111\u1171\u11B6"), "ID[\uD4DB]");

  // Already pre-composed hangul syllables should stay composed.
  EXPECT_EQ(RunLexer("\uD4CC"), "ID[\uD4CC]");
  EXPECT_EQ(RunLexer("\uD4DB"), "ID[\uD4DB]");
}

TEST(LexerTest, Identifier_ShouldConvertToNFKC) {
  EXPECT_EQ(RunLexer("ＦｕｌｌｗｉｄｔｈX１２３"), "ID[FullwidthX123]");
  EXPECT_EQ(RunLexer("\u217B"), "ID[xii]"); // SMALL ROMAN NUMERAL TWELVE
  EXPECT_EQ(RunLexer("ｼｷﾍﾞﾂｼ"), "ID[シキベツシ]");
  EXPECT_EQ(RunLexer("Äöü"), "ID[Äöü]");
  EXPECT_EQ(RunLexer("A\u0308"), "ID[Ä]");
  EXPECT_EQ(RunLexer("\u01B7\u030C"), "ID[\u01EE]");  // Ǯ
  EXPECT_EQ(RunLexer("\u1E69"), "ID[\u1E69]");
  EXPECT_EQ(RunLexer("s\u0323\u0307s\u0323\u0307"), "ID[\u1E69\u1E69]");
  EXPECT_EQ(RunLexer("s\u0307\u0323s\u0307\u0323"), "ID[\u1E69\u1E69]");
  EXPECT_EQ(RunLexer("q\u0307\u0323"), "ID[q\u0323\u0307]");
  EXPECT_EQ(RunLexer("q\u0323\u0307"), "ID[q\u0323\u0307]");
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

TEST(LexerTest, Integer) {
  EXPECT_EQ(RunLexer("123"), "INTEGER[123]");
  EXPECT_EQ(RunLexer("-123"), "INTEGER[-123]");
  EXPECT_EQ(RunLexer("+123"), "INTEGER[+123]");
}

TEST(LexerTest, Whitespace) {
  EXPECT_EQ(RunLexer("if  foo : \n"), "IF[if]|ID[foo]|COLON[:]|NEWLINE");
}

}  // namespace firc
