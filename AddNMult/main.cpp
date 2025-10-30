#include <iostream>
#include "Lexer.h"
#include "Parser.h"
#include "CodeGen.h"
#include <llvm/Support/raw_ostream.h>

using namespace addNMult;

int main() {
  std::string input =
      "let x = 1\n"
      "set x = x + 41\n"
      "return x == 42";
  Lexer lexer(input);
  Parser p(lexer);
  try {
    auto prog = p.parseProgram();
    CodeGen cg("addNMult.cpp");
    auto* mainFunction = cg.emit(*prog);
    if (!mainFunction) {
      std::cerr << "codegen failed\n";
      return 1;
    }
    cg.module()->print(llvm::outs(), nullptr);
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  }
}
