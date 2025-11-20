#include "SemanticAnalyzer.h"
#include <iostream>

namespace addNMult {

    void SemanticAnalyzer::pushScope() {
        scopes.push_back(std::unordered_map<std::string, VarState>());
    }

    void SemanticAnalyzer::popScope() {
        if (!scopes.empty()) {
            scopes.pop_back();
        }
    }

    bool SemanticAnalyzer::declare(const std::string& name) {
        if (scopes.empty()) {
            pushScope();
        }

        for (const auto& scope : scopes) {
            if (scope.find(name) != scope.end()) {
                std::cerr << "redeclaration of '" << name << "'\n";
                return false;
            }
        }
        
        // In reality of things, currentScope is what's actually enabling the tracking
        // of what's declared or not.
        std::unordered_map<std::string, VarState>& currentScope = scopes.back();
        currentScope[name] = VarState::Declared;
        return true;
    }

    bool SemanticAnalyzer::isDeclared(const std::string& name) const {
        auto it = scopes.rbegin();
        while (it != scopes.rend()) {
            const auto& scope = *it;
            if (scope.find(name) != scope.end()) {
                return true;
            }
            it++;
        }
        std::cerr << "undeclared variable '" << name << "'\n";
        return false;
    }

    bool SemanticAnalyzer::setInitialized(const std::string& name) {
        auto it = scopes.rbegin();
        while (it != scopes.rend()) {
            auto& scope = *it;
            auto found = scope.find(name);
            if (found != scope.end()) {
                found->second = VarState::Initialized;
                return true;
            }
            it++;
        }
        return false;
    }

    bool SemanticAnalyzer::checkVarUse(const std::string& name) {
        auto it = scopes.rbegin();
        while (it != scopes.rend()) {
            const auto& scope = *it;
            auto found = scope.find(name);
            if (found != scope.end()) {
                if (found->second == VarState::Initialized) {
                    return true;
                }
                std::cerr << "use of variable '" << name
                          << "' before an assignment\n";
                return false;
            }
            it++;
        }
        std::cerr << "use of undeclared variable: '" << name << "'\n";
        return false;
    }

    bool SemanticAnalyzer::analyze(const Program& program) {
        scopes.clear();
        pushScope();

        for (const std::unique_ptr<Statement>& statementPtr : program.statements) 
        {
            if (!analyzeStatement(statementPtr.get())) {
                return false;
            }
        }

        popScope();
        return true;
    }

    bool SemanticAnalyzer::analyzeStatement(const Statement* statement) {
        if (statement == nullptr) {
            return true;
        }

        if (auto varDecl = dynamic_cast<const VarDecl*>(statement)) {
            if (!declare(varDecl->name)) {
                return false;
            }
            if (varDecl->value) {
                if (!analyzeExpression(varDecl->value.get())) {
                    return false;
                }
                if (!setInitialized(varDecl->name)) {
                    return false;
                }
            }
            return true;
        }

        if (auto setStatement = dynamic_cast<const SetStatement*>(statement)) {
            if (!isDeclared(setStatement->name)) {
                return false;
            }
            if (setStatement->value) {
                if (!analyzeExpression(setStatement->value.get())) {
                    return false;
                }
                if (!setInitialized(setStatement->name)) {
                    return false;
                }
            }
            return true;
        }

        if (auto returnStatement = dynamic_cast<const ReturnStatement*>(statement)) {
            if (returnStatement->value) {
                return analyzeExpression(returnStatement->value.get());
            }
            return true;
        }

        if (auto ifStatement = dynamic_cast<const IfStatement*>(statement)) {
            if (ifStatement->cond) {
                if (!analyzeExpression(ifStatement->cond.get())) {
                    return false;
                }
            }

            pushScope();
            for (const std::unique_ptr<Statement>& thenStatementPtr : ifStatement->thenBody) {
                if (!analyzeStatement(thenStatementPtr.get())) {
                    popScope();
                    return false;
                }
            }
            popScope();

            if (!ifStatement->elseBody.empty()) {
                pushScope();
                for (const std::unique_ptr<Statement>& elseStatementPtr : ifStatement->elseBody) {
                    if (!analyzeStatement(elseStatementPtr.get())) {
                        popScope();
                        return false;
                    }
                }
                popScope();
            }

            return true;
        }

        std::cerr << "unknown statement kind\n";
        return false;
    }

    bool SemanticAnalyzer::analyzeExpression(const Expression* expression) {
        if (expression == nullptr) {
            return true;
        }

        if (dynamic_cast<const NumberExpression*>(expression)) {
            return true;
        }

        if (dynamic_cast<const BoolExpression*>(expression)) {
            return true;
        }

        if (auto variableExpression = 
            dynamic_cast<const VarExpression*>(expression)) {
            return checkVarUse(variableExpression->name);
        }

        if (auto binaryExpression = 
            dynamic_cast<const BinaryExpression*>(expression)) {
            if (!analyzeExpression(binaryExpression->lhs.get())) {
                return false;
            }
            if (!analyzeExpression(binaryExpression->rhs.get())) {
                return false;
            }
            return true;
        }

        std::cerr << "unknown expression kind\n";
        return false;
    }
}