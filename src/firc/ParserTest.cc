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
  AST->Write(&Out);
  return Out.str();
}

TEST(ParserTest, Proc) {
  EXPECT_EQ(Parse("proc Foo():\n return\n"), "proc Foo():\n    return\n");
  EXPECT_EQ(Parse("proc F(x):\n return\n"), "proc F(x):\n    return\n");
  EXPECT_EQ(Parse("proc F(x,y ,z):\n return\n"),
            "proc F(x, y, z):\n    return\n");
}

}  // namespace firc
