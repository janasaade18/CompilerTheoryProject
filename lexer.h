#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <QString>
#include <vector>
#include <stack>

using namespace std;

class Lexer {
public:
    Lexer(const QString& source);
    vector<Token> tokenize();

private:
    QString m_source;
    int m_pos = 0;
    int m_line = 1;
    stack<int> m_indent_stack;

    Token getNextTokenFromSource();
    void advance();
    QChar currentChar();
    QChar peek();
    void skipWhitespace();
    void skipComment();
    Token number();
    Token string();
    Token identifier();
    int getCurrentIndent();
};

#endif // LEXER_H
