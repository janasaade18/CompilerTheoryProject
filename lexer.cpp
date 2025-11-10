#include "lexer.h"
#include <QChar>

Lexer::Lexer(const QString& source) : m_source(source) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token token;
    do {
        token = getNextToken();
        tokens.push_back(token);
    } while (token.type != TokenType::END_OF_FILE);
    return tokens;
}

QChar Lexer::currentChar() {
    if (m_pos >= m_source.length()) {
        return '\0';
    }
    return m_source[m_pos];
}

void Lexer::advance() {
    m_pos++;
}

void Lexer::skipWhitespace() {
    while (currentChar().isSpace()) {
        advance();
    }
}

Token Lexer::number() {
    QString result;
    while (currentChar().isDigit()) {
        result += currentChar();
        advance();
    }
    return {TokenType::NUMBER, result};
}

Token Lexer::identifier() {
    QString result;
    if (currentChar().isLetter()) {
        result += currentChar();
        advance();
        while (currentChar().isLetterOrNumber() || currentChar() == '_') {
            result += currentChar();
            advance();
        }
    }
    return {TokenType::IDENTIFIER, result};
}

Token Lexer::getNextToken() {
    skipWhitespace();

    if (m_pos >= m_source.length()) {
        return {TokenType::END_OF_FILE, ""};
    }

    if (currentChar().isDigit()) return number();
    if (currentChar().isLetter()) return identifier();

    switch (currentChar().toLatin1()) {
    case '=': advance(); return {TokenType::EQUAL, "="};
    case '+': advance(); return {TokenType::PLUS, "+"};
    case '-': advance(); return {TokenType::MINUS, "-"};
    case '*': advance(); return {TokenType::STAR, "*"};
    case '/': advance(); return {TokenType::SLASH, "/"};
    case ';': advance(); return {TokenType::SEMICOLON, ";"};
    case '(': advance(); return {TokenType::LPAREN, "("};
    case ')': advance(); return {TokenType::RPAREN, ")"};
    }

    advance();
    return {TokenType::ILLEGAL, QString(m_source[m_pos-1])};
}
