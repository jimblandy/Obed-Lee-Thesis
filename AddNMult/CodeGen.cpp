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
    named.clear();
    auto* functionType = llvm::FunctionType::get(i64Ty(ctx), false);
    auto* function = llvm::Function::Create(
        functionType, llvm::Function::ExternalLinkage, "addNMult", mod.get()
    );

    auto* entryBlock = llvm::BasicBlock::Create(ctx, "entry", function);
    builder->SetInsertPoint(entryBlock);
    for (const auto& stmtPtr : program.statements) {
        if (!emitStatement(stmtPtr.get(), function)) {
            function->eraseFromParent();
            return nullptr;
        }
    }
    if (llvm::verifyFunction(*function, &llvm::errs())) {
        function->eraseFromParent();
        return nullptr;
    }

    return function;
}

bool CodeGen::emitStatement(const Statement* s, Function* function) {
    if (auto* vd = dynamic_cast<const VarDecl*>(s)) {
        auto* slot = builder->CreateAlloca(i64Ty(ctx), nullptr, vd->name);
        named[vd->name] = slot;
        Value* init = codegen(vd->value.get());
        if (!init) return false;
        builder->CreateStore(init, slot);
        return true;
    }

    if (auto* st = dynamic_cast<const SetStatement*>(s)) {
        auto it = named.find(st->name);
        if (it == named.end()) return false;
        Value* v = codegen(st->value.get());
        if (!v) return false;
        builder->CreateStore(v, it->second);
        return true;
    }

    if (auto* iff = dynamic_cast<const IfStatement*>(s)) {
        return emitIf(*iff, function);
    }

    if (auto* ret = dynamic_cast<const ReturnStatement*>(s)) {
        Value* v = codegen(ret->value.get());
        if (!v) return false;
        builder->CreateRet(v);
        return true;
    }

    return false;
}

bool CodeGen::emitIf(const IfStatement& s, Function* function) {
    Value* cond = codegen(s.cond.get());
    if (!cond) return false;

    cond = builder->CreateICmpNE(
        cond,
        ConstantInt::get(i64Ty(ctx), 0, false),
        "ifcond"
    );

    BasicBlock* thenBlock = BasicBlock::Create(ctx, "then", function);
    BasicBlock* elseBlock = nullptr;
    BasicBlock* contBlock = BasicBlock::Create(ctx, "ifcont", function);

    bool hasElse = !s.elseBody.empty();
    if (hasElse) {
        elseBlock = BasicBlock::Create(ctx, "else", function);
        builder->CreateCondBr(cond, thenBlock, elseBlock);
    } else {
        builder->CreateCondBr(cond, thenBlock, contBlock);
    }

    builder->SetInsertPoint(thenBlock);
    for (const auto& stmtPtr : s.thenBody) {
        if (!emitStatement(stmtPtr.get(), function)) return false;
    }
    
    BasicBlock* thenEnd = builder->GetInsertBlock();
    if (!thenEnd->getTerminator()) {
        builder->CreateBr(contBlock);
    }

    if (hasElse) {
        builder->SetInsertPoint(elseBlock);
        for (const auto& stmtPtr : s.elseBody) {
            if (!emitStatement(stmtPtr.get(), function)) return false;
        }
        BasicBlock* elseEnd = builder->GetInsertBlock();
        if (!elseEnd->getTerminator()) {
            builder->CreateBr(contBlock);
        }
    }

    builder->SetInsertPoint(contBlock);
    return true;
}