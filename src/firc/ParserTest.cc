#include <memory>
#include <sstream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/MemoryBuffer.h>

#include "firc/AST.h"
#include "firc/Parser.h"
#include "gtest/gtest.h"

namespace firc {

std::string Parse(llvm::StringRef s) {
  std::string result;
  llvm::BumpPtrAllocator allocator;
  std::unique_ptr<llvm::MemoryBuffer> buf(llvm::MemoryBuffer::getMemBuffer(s));
  std::unique_ptr<FileAST> AST(Parser::ParseFile(buf.get(), &allocator));
  std::ostringstream Out;
  AST->write(&Out);
  return Out.str();
}

TEST(ParserTest, Proc) {
  EXPECT_EQ(Parse(u8"proc Foo():\n return\n"), u8"proc Foo():\n    return\n");
  EXPECT_EQ(Parse(u8"proc F(x):\n return\n"), u8"proc F(x):\n    return\n");
  EXPECT_EQ(Parse(u8"proc Foo():\n return\n return\n"),
            u8"proc Foo():\n    return\n    return\n");
  EXPECT_EQ(Parse(u8"proc F(x,y ,z):\n return\n"),
            u8"proc F(x, y, z):\n    return\n");
  EXPECT_EQ(Parse(u8"proc Foo(): bar . Baz . Qux\n return\n"),
            u8"proc Foo(): bar.Baz.Qux\n    return\n");
  EXPECT_EQ(Parse(u8"proc Foo(x, y: bär.Baz ): bär.Qux\n return\n"),
            u8"proc Foo(x, y: bär.Baz): bär.Qux\n    return\n");
  EXPECT_EQ(Parse(u8"proc Foo(a, b, c):\n return\n"),
            u8"proc Foo(a, b, c):\n    return\n");
  EXPECT_EQ(Parse(u8"proc Foo(a, b, c; d: bar.Baz):\n return\n"),
            u8"proc Foo(a, b, c; d: bar.Baz):\n    return\n");
  EXPECT_EQ(Parse(u8"proc Foo(a: bar.Baz; b, c; d: bar.Qux):\n return\n"),
            u8"proc Foo(a: bar.Baz; b, c; d: bar.Qux):\n    return\n");
  EXPECT_EQ(Parse(u8"proc Foo(a: A; aa1: A.A; aa2: A.A; b: B):\n return\n"),
            u8"proc Foo(a: A; aa1, aa2: A.A; b: B):\n    return\n");
}

TEST(ParserTest, EmptyStatement) {
  EXPECT_EQ(Parse(u8"proc P():\n \n return\n"), u8"proc P():\n    return\n");
}

TEST(ParserTest, ReturnStatement) {
  EXPECT_EQ(Parse(u8"proc P():\n return\n"), u8"proc P():\n    return\n");
}

TEST(ParserTest, VarStatement) {
  EXPECT_EQ(Parse(u8"proc P():\n var i\n"), u8"proc P():\n    var i\n");
  EXPECT_EQ(Parse(u8"proc P():\n var i,j\n"), u8"proc P():\n    var i, j\n");
  EXPECT_EQ(Parse(u8"proc P():\n var i,j:bar . Baz\n"),
            u8"proc P():\n    var i, j: bar.Baz\n");
  EXPECT_EQ(Parse(u8"proc P():\n var i,j:bar . Baz; cond: fir.Bool\n"),
            u8"proc P():\n    var i, j: bar.Baz; cond: fir.Bool\n");
  EXPECT_EQ(Parse(u8"proc P():\n var i,j:Int; k: Int; cond: Bool\n"),
            u8"proc P():\n    var i, j, k: Int; cond: Bool\n");
}

}  // namespace firc
