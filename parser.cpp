#include "parser.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : m_tokens(tokens) {}

Token Parser::currentToken() {
    if (m_pos >= m_tokens.size()) {
        return {TokenType::END_OF_FILE, ""};
    }
    return m_tokens[m_pos];
}

void Parser::advance() {
    if (m_pos < m_tokens.size()) {
        m_pos++;
    }
}

std::unique_ptr<ASTNode> Parser::parseFactor() {
    Token token = currentToken();
    if (token.type == TokenType::NUMBER) {
        advance();
        return std::make_unique<NumberNode>(token);
    } else if (token.type == TokenType::IDENTIFIER) {
        advance();
        return std::make_unique<IdentifierNode>(token);
    } else if (token.type == TokenType::LPAREN) {
        advance();
        auto expr = parseExpression();
        if (currentToken().type != TokenType::RPAREN) {
            throw std::runtime_error("Syntax Error: Expected ')'");
        }
        advance(); // Consume ')'
        return expr;
    }
    throw std::runtime_error("Syntax Error: Unexpected token in expression");
}

std::unique_ptr<ASTNode> Parser::parseTerm() {
    auto node = parseFactor();
    while (currentToken().type == TokenType::STAR || currentToken().type == TokenType::SLASH) {
        Token op = currentToken();
        advance();
        auto right = parseFactor();
        node = std::make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parseExpression() {
    auto node = parseTerm();
    while (currentToken().type == TokenType::PLUS || currentToken().type == TokenType::MINUS) {
        Token op = currentToken();
        advance();
        auto right = parseTerm();
        node = std::make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    if (currentToken().type == TokenType::IDENTIFIER && m_tokens[m_pos + 1].type == TokenType::EQUAL) {
        auto identifier = std::make_unique<IdentifierNode>(currentToken());
        advance(); // consume identifier
        if (currentToken().type != TokenType::EQUAL) {
            throw std::runtime_error("Syntax Error: Expected '=' for assignment");
        }
        advance(); // consume '='
        auto expr = parseExpression();
        if (currentToken().type != TokenType::SEMICOLON) {
            throw std::runtime_error("Syntax Error: Expected ';' after statement");
        }
        advance(); // consume ';'
        return std::make_unique<AssignmentNode>(std::move(identifier), std::move(expr));
    }
    throw std::runtime_error("Syntax Error: Invalid statement");
}

std::unique_ptr<ProgramNode> Parser::parse() {
    auto programNode = std::make_unique<ProgramNode>();
    while (currentToken().type != TokenType::END_OF_FILE) {
        programNode->statements.push_back(parseStatement());
    }
    return programNode;
}
