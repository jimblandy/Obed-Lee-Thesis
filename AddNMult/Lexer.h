#pragma once
#include <cstdint>
#include <string>

namespace addNMult {

    enum class TokenKind {
        Eof, Let, Return, Varname, Number, Plus, Set, Star, Eq, Invalid, If, Else,
        OpenParen, CloseParen, OpenBrace, CloseBrace,
        True, False, IsEqual, IsNotEqual,
        Less, LessEqual, Greater, GreaterEqual
    };

    struct Token {
        TokenKind kind = TokenKind::Invalid;
        std::string stringToken;
        std::uint64_t numberValue = 0;
        std::size_t offset = 0;
    };

    class Lexer {
        public:
            Lexer(const std::string& source);
            Token next();
        private:
            const std::string& src;
            std::size_t n = 0;
            std::size_t i = 0;

            static bool isLetter(char c);
            static bool isDigit(char c);
            void skipWhitespace();
            Token tokenize(TokenKind k, std::size_t start, std::size_t len) const;
            Token tokenizeOperator(TokenKind k, std::size_t len);
            Token lexIdentifierOrKeyword();
            Token lexNumber();
    };
}