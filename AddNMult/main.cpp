#include <iostream>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "CodeGen.h"

using namespace std;
using namespace addNMult;

int main() {
  std::string input =
    "let x = 2 + 2\n"
    "if (x > 5) {"
    "  set x = 2\n"
    "  if (x == 2) {"
    "     set x = 1"
    "     return x"
    "  }"
    "}\n"
    "return x\n";
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
