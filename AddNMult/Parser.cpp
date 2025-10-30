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

        while (is(TokenKind::Let) || is(TokenKind::Set) || is(TokenKind::If)) {
            if (is(TokenKind::Let)) {
                auto decl = parseLet();
                program->decls.resize(program->decls.size() + 1);
                VarDecl& slot = program->decls.back();
                slot.name = decl->name;
                slot.value.reset(decl->value.release());
            } else if (is(TokenKind::Set)) {
                program->sets.push_back(parseSet());
            } else {
                program->ifs.push_back(parseIf());
            }
        }

        expect(TokenKind::Return, "'return'");
        program->ret = parseCompare();

        if (!is(TokenKind::Eof)) {
            throw std::runtime_error("unintended token");
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
        return std::unique_ptr<VarDecl>(new VarDecl{name, std::unique_ptr<Expression>(valueExpr.release())});
    }

    SetStatement Parser::parseSet() {
        expect(TokenKind::Set, "'set'");
        if (!is(TokenKind::Varname)) throw std::runtime_error("expected identifier");
        std::string name = token.stringToken;
        next();
        expect(TokenKind::Eq, "'='");
        auto valueExpr = parseCompare();
        return SetStatement{std::move(name), std::move(valueExpr)};
    }

    IfStatement Parser::parseIf() {
        expect(TokenKind::If, "'if'");
        auto condExpr = parseCompare();
        expect(TokenKind::OpenBrace, "'{'");
        IfStatement s;
        s.cond = std::move(condExpr);
        while (is(TokenKind::Let) || is(TokenKind::Set) || is(TokenKind::If)) {
            if (is(TokenKind::Let)) {
                auto decl = parseLet();
                s.decls.push_back(VarDecl{decl->name, std::unique_ptr<Expression>(decl->value.release())});
            } else if (is(TokenKind::Set)) {
                s.sets.push_back(parseSet());
            } else {
                s.ifs.push_back(parseIf());
            }
        }
        expect(TokenKind::CloseBrace, "'}'");
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
