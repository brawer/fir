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
  EXPECT_EQ(RunLexer("\xEF\xBB\xBF" "Foo\n"), u8"ID[Foo]|NEWLINE");
}

TEST(LexerTest, Keywords) {
  EXPECT_EQ(RunLexer(u8"and"), u8"AND[and]");
  EXPECT_EQ(RunLexer(u8"class"), u8"CLASS[class]");
  EXPECT_EQ(RunLexer(u8"const"), u8"CONST[const]");
  EXPECT_EQ(RunLexer(u8"else"), u8"ELSE[else]");
  EXPECT_EQ(RunLexer(u8"for"), u8"FOR[for]");
  EXPECT_EQ(RunLexer(u8"if"), u8"IF[if]");
  EXPECT_EQ(RunLexer(u8"import"), u8"IMPORT[import]");
  EXPECT_EQ(RunLexer(u8"in"), u8"IN[in]");
  EXPECT_EQ(RunLexer(u8"is"), u8"IS[is]");
  EXPECT_EQ(RunLexer(u8"nil"), u8"NIL[nil]");
  EXPECT_EQ(RunLexer(u8"not"), u8"NOT[not]");
  EXPECT_EQ(RunLexer(u8"or"), u8"OR[or]");
  EXPECT_EQ(RunLexer(u8"proc"), u8"PROC[proc]");
  EXPECT_EQ(RunLexer(u8"return"), u8"RETURN[return]");
  EXPECT_EQ(RunLexer(u8"var"), u8"VAR[var]");
  EXPECT_EQ(RunLexer(u8"while"), u8"WHILE[while]");
  EXPECT_EQ(RunLexer(u8"with"), u8"WITH[with]");
  EXPECT_EQ(RunLexer(u8"yield"), u8"YIELD[yield]");
}

TEST(LexerTest, Symbols) {
  EXPECT_EQ(RunLexer(u8"("), u8"LEFT_PARENTHESIS[(]");
  EXPECT_EQ(RunLexer(u8")"), u8"RIGHT_PARENTHESIS[)]");
  EXPECT_EQ(RunLexer(u8"["), u8"LEFT_BRACKET[[]");
  EXPECT_EQ(RunLexer(u8"]"), u8"RIGHT_BRACKET[]]");
  EXPECT_EQ(RunLexer(u8":"), u8"COLON[:]");
  EXPECT_EQ(RunLexer(u8";"), u8"SEMICOLON[;]");
  EXPECT_EQ(RunLexer(u8","), u8"COMMA[,]");
  EXPECT_EQ(RunLexer(u8"."), u8"DOT[.]");
  EXPECT_EQ(RunLexer(u8"="), u8"EQUAL[=]");
  EXPECT_EQ(RunLexer(u8"+"), u8"PLUS[+]");
  EXPECT_EQ(RunLexer(u8"-"), u8"MINUS[-]");
  EXPECT_EQ(RunLexer(u8"*"), u8"ASTERISK[*]");
  EXPECT_EQ(RunLexer(u8"/"), u8"SLASH[/]");
  EXPECT_EQ(RunLexer(u8"%"), u8"PERCENT[%]");
}

TEST(LexerTest, UnexpectedChar) {
  EXPECT_EQ(RunLexer(u8"§"), u8"ERROR_UNEXPECTED_CHAR[§]");
  EXPECT_EQ(RunLexer(u8"₩"), u8"ERROR_UNEXPECTED_CHAR[₩]");
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
  EXPECT_EQ(RunLexer(u8"ｼｷﾍﾞﾂｼ"), u8"ID[シキベツシ]");
  EXPECT_EQ(RunLexer(u8"Äöü"), u8"ID[Äöü]");
  EXPECT_EQ(RunLexer(u8"A\u0308"), u8"ID[Ä]");
  EXPECT_EQ(RunLexer(u8"\u01B7\u030C"), u8"ID[\u01EE]");  // Ǯ
  EXPECT_EQ(RunLexer(u8"\u1E69"), u8"ID[\u1E69]");
  EXPECT_EQ(RunLexer(u8"s\u0323\u0307s\u0323\u0307"), u8"ID[\u1E69\u1E69]");
  EXPECT_EQ(RunLexer(u8"s\u0307\u0323s\u0307\u0323"), u8"ID[\u1E69\u1E69]");
  EXPECT_EQ(RunLexer(u8"q\u0307\u0323"), u8"ID[q\u0323\u0307]");
  EXPECT_EQ(RunLexer(u8"q\u0323\u0307"), u8"ID[q\u0323\u0307]");
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
