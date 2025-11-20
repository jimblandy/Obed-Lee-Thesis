#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Parser.h"

namespace addNMult {

    enum class VarState {
        Declared,
        Initialized
    };

    class SemanticAnalyzer {
        public:
            bool analyze(const Program& program);
        
        private:
            std::vector<std::unordered_map<std::string, VarState>> scopes;
        
            void pushScope();
            void popScope();
        
            bool declare(const std::string& name);
            bool isDeclared(const std::string& name) const;
            bool setInitialized(const std::string& name);
            bool checkVarUse(const std::string& name);
        
            bool analyzeStatement(const Statement* statement);
            bool analyzeExpression(const Expression* expression);
    };
}