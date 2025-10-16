#pragma once
#include <string>
#include <unordered_map>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "Parser.h"

namespace addNMult {

class CodeGen {
    public:
        explicit CodeGen(std::string moduleName = "addNMult");
        llvm::Module* module() const { return mod.get(); }
        llvm::Function* emit(const Program& program);

    private:
        llvm::LLVMContext ctx;
        std::unique_ptr<llvm::Module> mod;
        std::unique_ptr<llvm::IRBuilder<>> builder;
        std::unordered_map<std::string, llvm::Value*> named;

        llvm::Value* codegen(const Expression* e);
        llvm::Value* codegenNumber(const NumberExpression* e);
        llvm::Value* codegenVar(const VarExpression* e);
        llvm::Value* codegenBinary(const BinaryExpression* e);
    };
}