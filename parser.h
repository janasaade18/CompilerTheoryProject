#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"
#include <vector>
#include <memory>

using namespace std;

class Parser {
public:
    Parser(const vector<Token>& tokens);
    unique_ptr<ProgramNode> parse();

private:
    vector<Token> m_tokens;
    int m_pos = 0;

    Token currentToken();
    Token peekToken();
    void advance();
    void expect(TokenType type);
    void skipUntil(TokenType type);
    void skipUntilAny(const vector<TokenType>& types);

    unique_ptr<ASTNode> parseStatement();
    unique_ptr<BlockNode> parseBlock();
    unique_ptr<ASTNode> parseExpression();
    unique_ptr<ASTNode> parseLogicalOr();
    unique_ptr<ASTNode> parseComparison();
    unique_ptr<ASTNode> parseTerm();
    unique_ptr<ASTNode> parseFactor();
    unique_ptr<ASTNode> parseUnary();
    unique_ptr<ASTNode> parsePrimary();
    unique_ptr<ASTNode> parseFunctionDefinition();
    unique_ptr<ASTNode> parseIfStatement();
    unique_ptr<ASTNode> parseTryExceptStatement();
    unique_ptr<ASTNode> parseAssignmentOrExpression();
};

#endif // PARSER_H
