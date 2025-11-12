#pragma once
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include "Lexer.h"

namespace addNMult {

    struct Expression {
        virtual ~Expression() = default;
    };

    struct NumberExpression : Expression {
        std::uint64_t value;
        explicit NumberExpression(std::uint64_t v) : value(v) {}
    };

    struct VarExpression : Expression {
        std::string name;
        explicit VarExpression(const std::string& n) : name(n) {}
    };

    struct BoolExpression : Expression {
        bool value;
        explicit BoolExpression(bool v) : value(v) {}
    };

    enum class Op { 
        Add, Mul, Equal, NotEqual, 
        LessThan, LessThanOrEqual, GreaterThan, GreaterThanOrEqual };

    struct BinaryExpression : Expression {
        // if we have 2 + 3
        Op op; // this will hold +
        // and lhs will hold 2 and rhs will hold 3.
        std::unique_ptr<Expression> lhs;
        std::unique_ptr<Expression> rhs;
        BinaryExpression(Op o, Expression* a, Expression* b) : op(o), lhs(a), rhs(b) {}
    };

    struct Statement {
        virtual ~Statement() = default;
    };

    struct VarDecl : Statement {
        std::string name;
        std::unique_ptr<Expression> value;
    };

    struct SetStatement : Statement {
        std::string name;
        std::unique_ptr<Expression> value;
    };

    struct ReturnStatement : Statement {
        std::unique_ptr<Expression> value;
    };


    struct IfStatement : Statement {
        std::unique_ptr<Expression> cond;
        std::vector<std::unique_ptr<Statement>> thenBody;
        std::vector<std::unique_ptr<Statement>> elseBody;
    };

    struct Program {
        std::vector<std::unique_ptr<Statement>> statements;
    };

    class Parser {
    public:
        explicit Parser(Lexer& lx);
        std::unique_ptr<Program> parseProgram();
        std::unique_ptr<VarDecl> parseLet();

    private:
        Lexer& lex;
        Token token;

        void next();
        bool is(TokenKind k) const;
        void expect(TokenKind k, const char* what);

        std::unique_ptr<Expression> parseRHS();
        std::unique_ptr<Expression> parseSumNums();
        std::unique_ptr<Expression> parseProdNums();
        std::unique_ptr<Expression> parseEval();
        std::unique_ptr<Expression> parseCompare();

        std::unique_ptr<Statement> parseStatement();
        std::unique_ptr<ReturnStatement> parseReturn();
        std::unique_ptr<IfStatement> parseIf();

        SetStatement parseSet();
    };

}
