#include "Lexer.h"

namespace addNMult {

    Lexer::Lexer(const std::string& source) : src(source), n(source.size()) {}

    bool Lexer::isLetter(char c) {
        unsigned char u = static_cast<unsigned char>(c);
        return (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z') || (u == '_');
    }

    bool Lexer::isDigit(char c) {
        unsigned char u = static_cast<unsigned char>(c);
        return (u >= '0' && u <= '9');
    }

    void Lexer::skipWhitespace() {
        while (i < n) {
            char c = src[i];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                i++;
            } else {
                break;
            }
        }
    }

    Token Lexer::tokenize(TokenKind k, std::size_t start, std::size_t len) const {
        Token t;
        t.kind = k;
        t.stringToken = src.substr(start, len);
        t.offset = start;
        if (k == TokenKind::Number) {
            std::uint64_t v = 0;
            for (char ch : t.stringToken) {
                v = v * 10 + (static_cast<unsigned>(ch) - static_cast<unsigned>('0'));
            }
            t.numberValue = v;
        }
        return t;
    }

    Token Lexer::tokenizeOperator(TokenKind k, std::size_t len) {
        std::size_t start = i;
        i += len;
        return tokenize(k, start, len);
    }

    Token Lexer::lexIdentifierOrKeyword() {
        std::size_t start = i++;
        while (i < n && (isLetter(src[i]) || isDigit(src[i]))) i++;
        std::string s = src.substr(start, i - start);
        if (s == "let") return tokenize(TokenKind::Let, start, s.size());
        if (s == "return") return tokenize(TokenKind::Return, start, s.size());
        if (s == "set")    return tokenize(TokenKind::Set, start, s.size());
        if (s == "if")     return tokenize(TokenKind::If, start, s.size());
        if (s == "else")   return tokenize(TokenKind::Else, start, s.size());
        if (s == "true")   return tokenize(TokenKind::True, start, s.size());
        if (s == "false")  return tokenize(TokenKind::False, start, s.size());
        Token t;
        t.kind = TokenKind::Varname;
        t.stringToken = s;
        t.offset = start;
        return t;
    }

    Token Lexer::lexNumber() {
        std::size_t start = i;
        while (i < n && isDigit(src[i])) i++;
        return tokenize(TokenKind::Number, start, i - start);
    }

    Token Lexer::next() {
        skipWhitespace();
        if (i >= n) return tokenize(TokenKind::Eof, i, 0);

        unsigned char c = static_cast<unsigned char>(src[i]);
        if (isLetter(c)) return lexIdentifierOrKeyword();
        if (isDigit(c))  return lexNumber();

        switch (c) {
            case '+': return tokenizeOperator(TokenKind::Plus, 1);
            case '*': return tokenizeOperator(TokenKind::Star, 1);
            case '(': return tokenizeOperator(TokenKind::OpenParen, 1);
            case ')': return tokenizeOperator(TokenKind::CloseParen, 1);
            case '{': return tokenizeOperator(TokenKind::OpenBrace, 1);
            case '}': return tokenizeOperator(TokenKind::CloseBrace, 1);
            case '=': {
                if (i + 1 < n && src[i+1] == '=') {
                    Token t = tokenize(TokenKind::IsEqual, i, 2);
                    i += 2;
                    return t;
                }
                return tokenizeOperator(TokenKind::Eq, 1);
            }
            case '!': {
                if (i + 1 < n && src[i+1] == '=') {
                    Token t = tokenize(TokenKind::IsNotEqual, i, 2);
                    i += 2;
                    return t;
                }
                return tokenizeOperator(TokenKind::Invalid, 1);
            }
            default:  return tokenizeOperator(TokenKind::Invalid, 1);
        }
    }
}
