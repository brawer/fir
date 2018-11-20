#include <memory>
#include <sstream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/MemoryBuffer.h>

#include "firc/AST.h"
#include "firc/Parser.h"
#include "gtest/gtest.h"

namespace firc {

std::string parse(llvm::StringRef s) {
  llvm::BumpPtrAllocator Allocator;
  std::unique_ptr<llvm::MemoryBuffer> Buf(llvm::MemoryBuffer::getMemBuffer(s));
  std::ostringstream Errors;
  ErrorHandler ErrHandler =
      [&Errors](llvm::StringRef Err, const SourceLocation& Loc) {
    Errors << "Error:" << Loc.Line << ':' << Loc.Column << ": " << Err.str()
           << '\n';
  };
  std::unique_ptr<FileAST> AST(
      Parser::parseFile("test.fir", "", Buf.get(), &Allocator, ErrHandler));
  std::ostringstream Out;
  AST->write(&Out);
  return Out.str() + Errors.str();
}

std::string parseExpr(llvm::StringRef s) {
  const std::string Prefix = "proc P():\n    return ";
  std::string Parsed = parse(Prefix + s.str() + "\n");
  if (Parsed.size() >= Prefix.size() + 1) {
    Parsed.erase(0, Prefix.size());
    Parsed.pop_back();
  }
  return Parsed;
}

static const char *ExpectedTopLevelStatement =
    "Error:1:1: Expected const, proc, var, or comment\n";

TEST(ParserTest, Procedure) {
  EXPECT_EQ(parse("proc Foo():\n return\n"), "proc Foo():\n    return\n");
  EXPECT_EQ(parse("proc F(x):#Comment\n return\n"),
            "proc F(x):  # Comment\n    return\n");
  EXPECT_EQ(parse("proc F(x):\n return\n"), "proc F(x):\n    return\n");
  EXPECT_EQ(parse("proc Foo():\n return\n return\n"),
            "proc Foo():\n    return\n    return\n");
  EXPECT_EQ(parse("proc F(x,y ,z):\n return\n"),
            "proc F(x, y, z):\n    return\n");
  EXPECT_EQ(parse("proc F(x:Int=-1; y:Int=7):\n return\n"),
            "proc F(x: Int = -1; y: Int = 7):\n    return\n");
  EXPECT_EQ(parse("proc Foo(): bar . Baz . Qux\n return\n"),
            "proc Foo(): bar.Baz.Qux\n    return\n");
  EXPECT_EQ(parse("proc Foo(x, y: bär.Baz ): bär.Qux\n return\n"),
            "proc Foo(x, y: bär.Baz): bär.Qux\n    return\n");
  EXPECT_EQ(parse("proc Foo(a, b, c):\n return\n"),
            "proc Foo(a, b, c):\n    return\n");
  EXPECT_EQ(parse("proc Foo(a, b, c; d: bar.Baz):\n return\n"),
            "proc Foo(a, b, c; d: bar.Baz):\n    return\n");
  EXPECT_EQ(parse("proc Foo(a, b, c; d: optional bar.Baz):\n return\n"),
            "proc Foo(a, b, c; d: optional bar.Baz):\n    return\n");
  EXPECT_EQ(parse("proc Foo(a: bar.Baz; b, c; d: bar.Qux):\n return\n"),
            "proc Foo(a: bar.Baz; b, c; d: bar.Qux):\n    return\n");
  EXPECT_EQ(parse("proc Foo(a: A; aa1: A.A; aa2: A.A; b: B):\n return\n"),
            "proc Foo(a: A; aa1: A.A; aa2: A.A; b: B):\n    return\n");
  EXPECT_EQ(parse("proc Bad():\n    12\n"),
            "proc Bad():\nError:2:5: Expected a statement\n");
}

TEST(ParserTest, Procedure_Nested) {
  const char *Nested = 
    "proc Foo():\n"
    "    var k\n"
    "    proc Bar():\n"
    "        var i, j\n"
    "        return 1\n"
    "    return 2\n";
  EXPECT_EQ(parse(Nested), Nested);
}

TEST(ParserTest, ConstStatement) {
  EXPECT_EQ(parse("const i = 6 #Comment\n"), "const i = 6  # Comment\n");
  EXPECT_EQ(parse("const i = 6; j = 7\n"), "const i = 6; j = 7\n");
  EXPECT_EQ(parse("const i: Int = 6\n"), "const i: Int = 6\n");
  EXPECT_EQ(parse("const i: optional Int = 6\n"),
            "const i: optional Int = 6\n");
  EXPECT_EQ(parse("const i: Int = 6; j: Int = 7\n"),
            "const i: Int = 6; j: Int = 7\n");
  EXPECT_EQ(parse("const i\n"),
            "const i\n"
            "Error:1:7: Constant “i” must have a value\n");
  EXPECT_EQ(parse("const i, j = 1\n"),
	    "const i, j = 1\n"
	    "Error:1:7: Constants must be separated by ‘;’, not ‘,’\n");
}

TEST(ParserTest, EmptyStatement) {
  EXPECT_EQ(parse("#\n"), "\n");
  EXPECT_EQ(parse("# Comment\n"), "# Comment\n");
  EXPECT_EQ(parse("proc P():\n \n return\n"), "proc P():\n    return\n");
  EXPECT_EQ(parse("proc P():\n #\n return\n"), "proc P():\n\n    return\n");
}

TEST(ParserTest, ReturnStatement) {
  EXPECT_EQ(parse("proc P():\n return\n"), "proc P():\n    return\n");
  EXPECT_EQ(parse("proc P():\n return 2\n"), "proc P():\n    return 2\n");
  EXPECT_EQ(parse("proc P():\n return #Blah blah.\n"),
            "proc P():\n    return  # Blah blah.\n");
  EXPECT_EQ(parse("proc P():\n return -123 # Blah blah.\n"),
            "proc P():\n    return -123  # Blah blah.\n");
  EXPECT_EQ(parse("return\n"), ExpectedTopLevelStatement);
}

TEST(ParserTest, VarStatement) {
  EXPECT_EQ(parse("var i\n"), "var i\n");
  EXPECT_EQ(parse("var i = 6\n"), "var i = 6\n");
  EXPECT_EQ(parse("var i: Int = 6\n"), "var i: Int = 6\n");
  EXPECT_EQ(parse("var i, j: Int = 6\n"), "var i, j: Int = 6\n");
  EXPECT_EQ(parse("var i = 7; j = 6\n"), "var i = 7; j = 6\n");
  EXPECT_EQ(parse("var i: Int = 7; j: Int = 6\n"),
            "var i: Int = 7; j: Int = 6\n");
  EXPECT_EQ(parse("var i#Comment\n"), "var i  # Comment\n");
  EXPECT_EQ(parse("var i: optional Int\n"), "var i: optional Int\n");
  EXPECT_EQ(parse("proc P():\n var i\n"), "proc P():\n    var i\n");
  EXPECT_EQ(parse("proc P():\n var i,j\n"), "proc P():\n    var i, j\n");
  EXPECT_EQ(parse("proc P():\n var i,j:bar . Baz\n"),
            "proc P():\n    var i, j: bar.Baz\n");
  EXPECT_EQ(parse("proc P():\n var i,j:bar . Baz; cond: fir.Bool\n"),
            "proc P():\n    var i, j: bar.Baz; cond: fir.Bool\n");
  EXPECT_EQ(parse("proc P():\n var i,j:Int; k: Int; cond: Bool\n"),
            "proc P():\n    var i, j: Int; k: Int; cond: Bool\n");
  EXPECT_EQ(parse("proc P():\n var i#C\n"), "proc P():\n    var i  # C\n");
}

TEST(ParserTest, BoolExpr) {
  EXPECT_EQ(parseExpr("false"), "false");
  EXPECT_EQ(parseExpr("true"), "true");
  EXPECT_EQ(parse("false\n"), ExpectedTopLevelStatement);
  EXPECT_EQ(parse("true\n"), ExpectedTopLevelStatement);
}

TEST(ParserTest, DotExpr) {
  EXPECT_EQ(parseExpr("1 .add"), "1 .add");
  EXPECT_EQ(parseExpr("foo.bar"), "foo.bar");
  // TODO: EXPECT_EQ(parseExpr("foo.bar"), "foo.bar");
  EXPECT_EQ(parseExpr("nil.toString"), "nil.toString");
  EXPECT_EQ(parseExpr("true.toString"), "true.toString");
}

TEST(ParserTest, IntExpr) {
  EXPECT_EQ(parseExpr("1"), "1");
  EXPECT_EQ(parseExpr("-2"), "-2");
  EXPECT_EQ(parseExpr("+3"), "3");
  EXPECT_EQ(parseExpr("12345678901234567890123456789012345678901234567890"),
            "12345678901234567890123456789012345678901234567890");
  EXPECT_EQ(parse("123\n"), ExpectedTopLevelStatement);
}

TEST(ParserTest, NameExpr) {
  EXPECT_EQ(parseExpr("foo"), "foo");
}

TEST(ParserTest, NilExpr) {
  EXPECT_EQ(parseExpr("nil"), "nil");
  EXPECT_EQ(parse("nil\n"), ExpectedTopLevelStatement);
}

TEST(ParserTest, ParenthesisExpr) {
  EXPECT_EQ(parseExpr("(0)"), "0");
  EXPECT_EQ(parseExpr("((-23))"), "-23");
  EXPECT_EQ(parseExpr("(false)"), "false");
  EXPECT_EQ(parseExpr("(true)"), "true");
  EXPECT_EQ(parseExpr("(nil)"), "nil");
  EXPECT_EQ(parseExpr("(nil.hash)"), "nil.hash");
}

}  // namespace firc
