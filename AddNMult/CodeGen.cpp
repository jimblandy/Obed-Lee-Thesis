#include "CodeGen.h"
#include <llvm/IR/Verifier.h>

using namespace addNMult;
using llvm::BasicBlock;
using llvm::ConstantInt;
using llvm::Function;
using llvm::Module;
using llvm::Value;

static llvm::Type* i64Ty(llvm::LLVMContext& ctx) {
    return llvm::Type::getInt64Ty(ctx);
}

CodeGen::CodeGen(const std::string& moduleName) {
    mod = std::make_unique<Module>(moduleName, ctx);
    mod->setSourceFileName("addNMult.cpp");
    builder = std::make_unique<llvm::IRBuilder<>>(ctx);
}

Value* CodeGen::codegen(const Expression* e) {
    if (!e) return nullptr;
    if (auto n = dynamic_cast<const NumberExpression*>(e)) return codegenNumber(n);
    if (auto v = dynamic_cast<const VarExpression*>(e))    return codegenVar(v);
    if (auto b = dynamic_cast<const BoolExpression*>(e))   return codegenBool(b);
    if (auto bin = dynamic_cast<const BinaryExpression*>(e)) return codegenBinary(bin);
    return nullptr;
}

Value* CodeGen::codegenNumber(const NumberExpression* e) {
    return ConstantInt::get(i64Ty(ctx), e->value, false);
}

Value* CodeGen::codegenVar(const VarExpression* e) {
    auto it = named.find(e->name);
    if (it == named.end()) return nullptr;
    return builder->CreateLoad(i64Ty(ctx), it->second, e->name.c_str());
}

Value* CodeGen::codegenBool(const BoolExpression* e) {
    return ConstantInt::get(i64Ty(ctx), e->value ? 1 : 0, false);
}

Value* CodeGen::codegenBinary(const BinaryExpression* e) {
    Value* L = codegen(e->lhs.get());
    if (!L) return nullptr;
    Value* R = codegen(e->rhs.get());
    if (!R) return nullptr;

    switch (e->op) {
        case Op::Add:
            return builder->CreateAdd(L, R, "addval");
        case Op::Mul:
            return builder->CreateMul(L, R, "mulval");
        case Op::Equal: {
            Value* cmp = builder->CreateICmpEQ(L, R, "eq");
            return builder->CreateZExt(cmp, i64Ty(ctx), "cmpeq");
        }
        case Op::NotEqual: {
            Value* cmp = builder->CreateICmpNE(L, R, "ne");
            return builder->CreateZExt(cmp, i64Ty(ctx), "cmpne");
        }
    }
    return nullptr;
}

llvm::Function* CodeGen::emit(const Program& program) {
    named.clear(); // To prevent the reuse of objects
    auto* functionType = llvm::FunctionType::get(i64Ty(ctx), false);
    auto* function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, "addNMult", mod.get());
    auto* entryBlock = llvm::BasicBlock::Create(ctx, "entry", function);
    builder->SetInsertPoint(entryBlock);

    for (const auto& declaration : program.decls) {
        auto* variableSlot = builder->CreateAlloca(i64Ty(ctx), nullptr, declaration.name);
        named[declaration.name] = variableSlot;
        Value* initializerValue = codegen(declaration.value.get());
        if (!initializerValue) { function->eraseFromParent(); return nullptr; }
        builder->CreateStore(initializerValue, variableSlot);
    }

    for (const auto& s : program.sets) {
        auto it = named.find(s.name);
        if (it == named.end()) { function->eraseFromParent(); return nullptr; }
        Value* v = codegen(s.value.get());
        if (!v) { function->eraseFromParent(); return nullptr; }
        builder->CreateStore(v, it->second);
    }

    for (const auto& s : program.ifs) {
        if (!emitIf(s, function)) { function->eraseFromParent(); return nullptr; }
    }

    Value* returnValue = codegen(program.ret.get());
    if (!returnValue) { function->eraseFromParent(); return nullptr; }
    builder->CreateRet(returnValue);

    if (llvm::verifyFunction(*function, &llvm::errs())) {
        function->eraseFromParent();
        return nullptr;
    }
    return function;
}

bool CodeGen::emitIf(const IfStatement& s, Function* function) {
    Value* cond64 = codegen(s.cond.get());
    if (!cond64) { 
        return false; 
    }

    Value* cond = builder->CreateICmpNE(cond64, builder->getInt64(0), "ifcond");

    bool hasElse =
        !s.elseDecls.empty() ||
        !s.elseSets.empty()  ||
        !s.elseIfs.empty();
    
    BasicBlock* thenBlock = BasicBlock::Create(ctx, "then", function);
    BasicBlock* elseBlock = hasElse ? BasicBlock::Create(ctx, "else", function) : nullptr;
    BasicBlock* contBlock = BasicBlock::Create(ctx, "ifcont", function);
    
    if (hasElse) {
        builder->CreateCondBr(cond, thenBlock, elseBlock);
    } else {
        builder->CreateCondBr(cond, thenBlock, contBlock);
    }

    builder->SetInsertPoint(thenBlock);
    for (const auto& d : s.thenDecls) {
        auto* slot = builder->CreateAlloca(i64Ty(ctx), nullptr, d.name);
        named[d.name] = slot;
        Value* init = codegen(d.value.get());
        if (!init) { return false; }
        builder->CreateStore(init, slot);
    }

    for (const auto& st : s.thenSets) {
        auto it = named.find(st.name);
        if (it == named.end()) return false;
        Value* v = codegen(st.value.get());
        if (!v) return false;
        builder->CreateStore(v, it->second);
    }

    for (const auto& nested : s.thenIfs) {
        if (!emitIf(nested, function)) return false;
    }
    builder->CreateBr(contBlock);
    
    if (hasElse) {
        builder->SetInsertPoint(elseBlock);
        for (const auto& d : s.elseDecls) {
            auto* slot = builder->CreateAlloca(i64Ty(ctx), nullptr, d.name);
            named[d.name] = slot;
            Value* init = codegen(d.value.get());
            if (!init) return false;
            builder->CreateStore(init, slot);
        }
        for (const auto& st : s.elseSets) {
            auto it = named.find(st.name);
            if (it == named.end()) return false;
            Value* v = codegen(st.value.get());
            if (!v) return false;
            builder->CreateStore(v, it->second);
        }
        for (const auto& nested : s.elseIfs) {
            if (!emitIf(nested, function)) return false;
        }
        builder->CreateBr(contBlock);
    }
    
    builder->SetInsertPoint(contBlock);
    return true;
}