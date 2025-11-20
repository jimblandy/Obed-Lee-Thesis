#include <iostream>
#include <string>
#include <llvm/Support/raw_ostream.h>
#include "Lexer.h"
#include "Parser.h"
#include "CodeGen.h"
#include "SemanticAnalyzer.h"

using namespace std;
using namespace addNMult;

int main() {
  std::string input =
    "let x = 2 + 2\n"
    "return x\n";
  Lexer lexer(input);
  Parser p(lexer);
  try {
    auto prog = p.parseProgram();
    CodeGen cg("addNMult.cpp");

    SemanticAnalyzer semanticAnalyzer;
    if (!semanticAnalyzer.analyze(*prog)) {
      std::cerr << "semantic analysis failed\n";
      return 1;
    }

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
