#ifndef TOKEN_H
#define TOKEN_H

#include <QString>

enum class TokenType {
    ILLEGAL, END_OF_FILE, IDENTIFIER, NUMBER, STRING,
    DEF, IF, RETURN, PRINT,
    WHILE, ELSE, ELIF,
    NOT, OR, NONE, TRUE, FALSE,
    EQUAL, DOUBLE_EQUAL, PLUS, MINUS, STAR, SLASH, LESS_EQUAL, GREATER,
    LPAREN, RPAREN, LBRACE, RBRACE, COLON, COMMA, SEMICOLON, DOT,
    INDENT, DEDENT,
    TRY, EXCEPT
};

struct Token {
    TokenType type;
    QString value;
    int line = 0;
};

#endif // TOKEN_H
