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
  std::string result;
  llvm::BumpPtrAllocator allocator;
  std::unique_ptr<llvm::MemoryBuffer> buf(llvm::MemoryBuffer::getMemBuffer(s));
  std::unique_ptr<FileAST> AST(Parser::parseFile(buf.get(), &allocator));
  std::ostringstream Out;
  AST->write(&Out);
  return Out.str();
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

TEST(ParserTest, Proc) {
  EXPECT_EQ(parse("proc Foo():\n return\n"), "proc Foo():\n    return\n");
  EXPECT_EQ(parse("proc F(x):#Comment\n return\n"),
            "proc F(x):  # Comment\n    return\n");
  EXPECT_EQ(parse("proc F(x):\n return\n"), "proc F(x):\n    return\n");
  EXPECT_EQ(parse("proc Foo():\n return\n return\n"),
            "proc Foo():\n    return\n    return\n");
  EXPECT_EQ(parse("proc F(x,y ,z):\n return\n"),
            "proc F(x, y, z):\n    return\n");
  EXPECT_EQ(parse("proc Foo(): bar . Baz . Qux\n return\n"),
            "proc Foo(): bar.Baz.Qux\n    return\n");
  EXPECT_EQ(parse("proc Foo(x, y: bär.Baz ): bär.Qux\n return\n"),
            "proc Foo(x, y: bär.Baz): bär.Qux\n    return\n");
  EXPECT_EQ(parse("proc Foo(a, b, c):\n return\n"),
            "proc Foo(a, b, c):\n    return\n");
  EXPECT_EQ(parse("proc Foo(a, b, c; d: bar.Baz):\n return\n"),
            "proc Foo(a, b, c; d: bar.Baz):\n    return\n");
  EXPECT_EQ(parse("proc Foo(a: bar.Baz; b, c; d: bar.Qux):\n return\n"),
            "proc Foo(a: bar.Baz; b, c; d: bar.Qux):\n    return\n");
  EXPECT_EQ(parse("proc Foo(a: A; aa1: A.A; aa2: A.A; b: B):\n return\n"),
            "proc Foo(a: A; aa1: A.A; aa2: A.A; b: B):\n    return\n");
}

TEST(ParserTest, EmptyStatement) {
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
}

TEST(ParserTest, VarStatement) {
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

TEST(ParserTest, IntExpr) {
  EXPECT_EQ(parseExpr("1"), "1");
  EXPECT_EQ(parseExpr("-2"), "-2");
  EXPECT_EQ(parseExpr("+3"), "3");
  EXPECT_EQ(parseExpr("12345678901234567890123456789012345678901234567890"),
            "12345678901234567890123456789012345678901234567890");
}

}  // namespace firc
