#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <QString>
#include <vector>

class Lexer {
public:
    Lexer(const QString& source);
    std::vector<Token> tokenize(); // A simple method to get all tokens at once

private:
    QString m_source;
    int m_pos = 0;

    Token getNextToken();
    QChar currentChar();
    void advance();
    void skipWhitespace();
    Token number();
    Token identifier();
};

#endif // LEXER_H

