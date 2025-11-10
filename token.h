#ifndef TOKEN_H
#define TOKEN_H

#include <QString>

// Defines all the possible types of tokens in our language
enum class TokenType {
    // Literals
    IDENTIFIER,     // variable names like 'x', 'my_var'
    NUMBER,         // 123, 45.6

    // Operators
    EQUAL,          // =
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /

    // Punctuation
    SEMICOLON,      // ;
    LPAREN,         // (
    RPAREN,         // )

    // Control
    ILLEGAL,        // An unknown character
    END_OF_FILE     // The end of the input
};

// A structure to hold a token's type and its actual string value
struct Token {
    TokenType type;
    QString value;
};

#endif // TOKEN_H
