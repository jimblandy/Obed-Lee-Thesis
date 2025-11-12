#include "Parser.h"

namespace addNMult {

    Parser::Parser(Lexer& lexer) : lex(lexer) { next(); }

    void Parser::next() { token = lex.next(); }

    bool Parser::is(TokenKind k) const { return token.kind == k; }

    void Parser::expect(TokenKind k, const char* what) {
        if (!is(k)) throw std::runtime_error(std::string("expected ") + what);
        next();
    }

    std::unique_ptr<Program> Parser::parseProgram() {
        auto program = std::make_unique<Program>();

        while (!is(TokenKind::Eof)) {
            program->statements.push_back(parseStatement());
        }

        return program;
    }

    std::unique_ptr<VarDecl> Parser::parseLet() {
        expect(TokenKind::Let, "'let'");
        if (!is(TokenKind::Varname)) throw std::runtime_error("expected identifier");
        std::string name = token.stringToken;
        next();
        expect(TokenKind::Eq, "'='");
        auto valueExpr = parseCompare();

        auto decl = std::unique_ptr<VarDecl>(new VarDecl);
        decl->name = std::move(name);
        decl->value = std::move(valueExpr);
        return decl;
    }

    SetStatement Parser::parseSet() {
        expect(TokenKind::Set, "'set'");
        if (!is(TokenKind::Varname)) throw std::runtime_error("expected identifier");
        std::string name = token.stringToken;
        next();
        expect(TokenKind::Eq, "'='");
        auto valueExpr = parseCompare();

        SetStatement s;
        s.name = std::move(name);
        s.value = std::move(valueExpr);
        return s;
    }

    std::unique_ptr<IfStatement> Parser::parseIf() {
        expect(TokenKind::If, "'if'");
        auto condExpr = parseCompare();
        expect(TokenKind::OpenBrace, "'{'");

        auto s = std::unique_ptr<IfStatement>(new IfStatement);
        s->cond = std::move(condExpr);

        while (is(TokenKind::Let) || is(TokenKind::Set) ||
               is(TokenKind::If) || is(TokenKind::Return)) {
            s->thenBody.push_back(parseStatement());
        }
        expect(TokenKind::CloseBrace, "'}'");

        if (is(TokenKind::Else)) {
            next();
            expect(TokenKind::OpenBrace, "'{'");
            while (is(TokenKind::Let) || is(TokenKind::Set) ||
                   is(TokenKind::If) || is(TokenKind::Return)) {
                s->elseBody.push_back(parseStatement());
            }
            expect(TokenKind::CloseBrace, "'}'");
        }

        return s;
    }

    std::unique_ptr<Expression> Parser::parseCompare() {
        auto left = parseSumNums();
        if (is(TokenKind::IsEqual) || is(TokenKind::IsNotEqual)) {
            TokenKind k = token.kind;
            next();
            auto right = parseSumNums();
            Op op = (k == TokenKind::IsEqual) ? Op::Equal : Op::NotEqual;
            return std::unique_ptr<Expression>(new BinaryExpression(op, left.release(), right.release()));
        }
        return left;
    }

    std::unique_ptr<Expression> Parser::parseRHS() { return parseSumNums(); }

    std::unique_ptr<Expression> Parser::parseSumNums() {
        auto e = parseProdNums();
        while (is(TokenKind::Plus)) {
            next();
            auto r = parseProdNums();
            e = std::unique_ptr<Expression>(new BinaryExpression(Op::Add, e.release(), r.release()));
        }
        return e;
    }

    std::unique_ptr<Expression> Parser::parseProdNums() {
        auto e = parseEval();
        while (is(TokenKind::Star)) {
            next();
            auto r = parseEval();
            e = std::unique_ptr<Expression>(new BinaryExpression(Op::Mul, e.release(), r.release()));
        }
        return e;
    }

    std::unique_ptr<ReturnStatement> Parser::parseReturn() {
        expect(TokenKind::Return, "'return'");
        auto valueExpr = parseCompare();
        auto result = std::unique_ptr<ReturnStatement>(new ReturnStatement);
        result->value = std::move(valueExpr);
        return result;
    }

    std::unique_ptr<Statement> Parser::parseStatement() {
        if (is(TokenKind::Let)) {
            auto decl = parseLet();
            return std::unique_ptr<Statement>(decl.release());
        }
        if (is(TokenKind::Set)) {
            SetStatement s = parseSet();
            return std::unique_ptr<Statement>(new SetStatement(std::move(s)));
        }
        if (is(TokenKind::If)) {
            auto ifStmt = parseIf();
            return std::unique_ptr<Statement>(ifStmt.release());
        }
        if (is(TokenKind::Return)) {
            auto retStmt = parseReturn();
            return std::unique_ptr<Statement>(retStmt.release());
        }

        throw std::runtime_error("expected statement");
    }


    std::unique_ptr<Expression> Parser::parseEval() {
        switch (token.kind) {
            case TokenKind::Number: {
                auto tokenVal = token.numberValue;
                next();
                return std::unique_ptr<Expression>(new NumberExpression(tokenVal));
            }
            case TokenKind::Varname: {
                std::string tokenVal = token.stringToken;
                next();
                return std::unique_ptr<Expression>(new VarExpression(tokenVal));
            }
            case TokenKind::True: {
                next();
                return std::unique_ptr<Expression>(new BoolExpression(true));
            }
            case TokenKind::False: {
                next();
                return std::unique_ptr<Expression>(new BoolExpression(false));
            }
            case TokenKind::OpenParen: {
                next();
                auto inner = parseCompare();
                expect(TokenKind::CloseParen, "')'");
                return inner;
            }
            default:
                throw std::runtime_error("expected a number, variable, or parenthensis.");
        }
    }
}
