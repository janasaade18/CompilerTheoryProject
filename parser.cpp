#include "parser.h"
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

Parser::Parser(const vector<Token>& tokens) : m_tokens(tokens) {}

Token Parser::currentToken() {
    if (m_pos >= m_tokens.size()) return {TokenType::END_OF_FILE, ""};
    return m_tokens[m_pos];
}

Token Parser::peekToken() {
    if (m_pos + 1 >= m_tokens.size()) return {TokenType::END_OF_FILE, ""};
    return m_tokens[m_pos + 1];
}

void Parser::advance() {
    if (m_pos < m_tokens.size()) m_pos++;
}

void Parser::expect(TokenType type) {
    if (currentToken().type == type) {
        advance();
    } else {
        advance(); // Skip unexpected tokens instead of throwing
    }
}

unique_ptr<ProgramNode> Parser::parse() {
    auto programNode = make_unique<ProgramNode>();
    while (currentToken().type != TokenType::END_OF_FILE) {
        // Skip INDENT/DEDENT at program level
        if (currentToken().type == TokenType::INDENT || currentToken().type == TokenType::DEDENT) {
            advance();
            continue;
        }

        auto stmt = parseStatement();
        if (stmt) {
            programNode->statements.push_back(std::move(stmt));
        }
    }
    return programNode;
}

unique_ptr<BlockNode> Parser::parseBlock() {
    auto block = make_unique<BlockNode>();

    // Skip the INDENT token that starts the block
    if (currentToken().type == TokenType::INDENT) {
        advance();
    }

    // Parse statements until we hit DEDENT or EOF
    while (currentToken().type != TokenType::DEDENT &&
           currentToken().type != TokenType::END_OF_FILE) {

        // Skip any extra INDENT tokens
        if (currentToken().type == TokenType::INDENT) {
            advance();
            continue;
        }

        auto stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
    }

    // Skip the DEDENT token that ends the block
    if (currentToken().type == TokenType::DEDENT) {
        advance();
    }

    return block;
}

unique_ptr<ASTNode> Parser::parseStatement() {
    // Skip INDENT/DEDENT at statement start
    while (currentToken().type == TokenType::INDENT || currentToken().type == TokenType::DEDENT) {
        advance();
    }

    if (currentToken().type == TokenType::END_OF_FILE) {
        return nullptr;
    }

    switch(currentToken().type) {
    case TokenType::DEF:
        return parseFunctionDefinition();
    case TokenType::IF:
        return parseIfStatement();
    case TokenType::TRY:
        return parseTryExceptStatement();
    case TokenType::RETURN:
        advance();
        return make_unique<ReturnNode>(parseExpression());
    case TokenType::PRINT:
        advance();
        return make_unique<PrintNode>(parseExpression());
    case TokenType::IDENTIFIER:
        return parseAssignmentOrExpression();
    default:
        return parseExpression();
    }
}

unique_ptr<ASTNode> Parser::parseAssignmentOrExpression() {
    auto identifier = make_unique<IdentifierNode>(currentToken());
    advance();

    if (currentToken().type == TokenType::EQUAL) {
        advance();
        auto expr = parseExpression();
        return make_unique<AssignmentNode>(std::move(identifier), std::move(expr));
    }

    return identifier;
}

unique_ptr<ASTNode> Parser::parseTryExceptStatement() {
    expect(TokenType::TRY);
    expect(TokenType::COLON);
    auto try_body = parseBlock();

    if (currentToken().type == TokenType::EXCEPT) {
        advance();
        expect(TokenType::COLON);
        parseBlock();
    }

    return make_unique<TryExceptNode>(std::move(try_body));
}

unique_ptr<ASTNode> Parser::parseFunctionDefinition() {
    expect(TokenType::DEF);
    auto name = make_unique<IdentifierNode>(currentToken());
    expect(TokenType::IDENTIFIER);
    expect(TokenType::LPAREN);

    vector<unique_ptr<IdentifierNode>> params;
    if (currentToken().type != TokenType::RPAREN) {
        params.push_back(make_unique<IdentifierNode>(currentToken()));
        expect(TokenType::IDENTIFIER);
    }

    expect(TokenType::RPAREN);
    expect(TokenType::COLON);
    auto body = parseBlock();
    return make_unique<FunctionDefNode>(std::move(name), std::move(params), std::move(body));
}

unique_ptr<ASTNode> Parser::parseIfStatement() {
    expect(TokenType::IF);
    auto condition = parseExpression();
    expect(TokenType::COLON);
    auto body = parseBlock();
    return make_unique<IfNode>(std::move(condition), std::move(body));
}

unique_ptr<ASTNode> Parser::parseExpression() {
    // Skip INDENT/DEDENT in expressions
    while (currentToken().type == TokenType::INDENT || currentToken().type == TokenType::DEDENT) {
        advance();
    }

    if (currentToken().type == TokenType::END_OF_FILE) {
        return make_unique<NoneNode>();
    }

    return parseLogicalOr();
}

unique_ptr<ASTNode> Parser::parseLogicalOr() {
    auto node = parseComparison();
    while (currentToken().type == TokenType::OR) {
        Token op = currentToken();
        advance();
        auto right = parseComparison();
        node = make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ASTNode> Parser::parseComparison() {
    auto node = parseTerm();
    while (currentToken().type == TokenType::GREATER ||
           currentToken().type == TokenType::LESS_EQUAL ||
           currentToken().type == TokenType::DOUBLE_EQUAL) {
        Token op = currentToken();
        advance();
        auto right = parseTerm();
        node = make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ASTNode> Parser::parseTerm() {
    auto node = parseFactor();
    while (currentToken().type == TokenType::PLUS || currentToken().type == TokenType::MINUS) {
        Token op = currentToken();
        advance();
        auto right = parseFactor();
        node = make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ASTNode> Parser::parseFactor() {
    auto node = parseUnary();
    while (currentToken().type == TokenType::STAR || currentToken().type == TokenType::SLASH) {
        Token op = currentToken();
        advance();
        auto right = parseUnary();
        node = make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ASTNode> Parser::parseUnary() {
    if (currentToken().type == TokenType::NOT) {
        Token op = currentToken();
        advance();
        auto right = parseUnary();
        return make_unique<UnaryOpNode>(op, std::move(right));
    }
    return parsePrimary();
}

unique_ptr<ASTNode> Parser::parsePrimary() {
    // Skip INDENT/DEDENT in primary expressions
    while (currentToken().type == TokenType::INDENT || currentToken().type == TokenType::DEDENT) {
        advance();
    }

    if (currentToken().type == TokenType::END_OF_FILE) {
        return make_unique<NoneNode>();
    }

    Token token = currentToken();

    switch(token.type) {
    case TokenType::NONE:
        advance();
        return make_unique<NoneNode>();
    case TokenType::TRUE:
        advance();
        return make_unique<NumberNode>(Token{TokenType::NUMBER, "1"});
    case TokenType::FALSE:
        advance();
        return make_unique<NumberNode>(Token{TokenType::NUMBER, "0"});
    case TokenType::NUMBER:
        advance();
        return make_unique<NumberNode>(token);
    case TokenType::STRING:
        advance();
        return make_unique<StringNode>(token);
    case TokenType::IDENTIFIER: {
        auto name = make_unique<IdentifierNode>(token);
        advance();

        if (currentToken().type == TokenType::LPAREN) {
            advance();
            vector<unique_ptr<ASTNode>> args;
            if (currentToken().type != TokenType::RPAREN) {
                args.push_back(parseExpression());
            }
            expect(TokenType::RPAREN);
            return make_unique<FunctionCallNode>(std::move(name), std::move(args));
        }

        return name;
    }
    case TokenType::LPAREN: {
        advance();
        auto expr = parseExpression();
        expect(TokenType::RPAREN);
        return expr;
    }
    case TokenType::LBRACE: {
        advance();
        while (currentToken().type != TokenType::RBRACE && currentToken().type != TokenType::END_OF_FILE) {
            advance();
        }
        if (currentToken().type == TokenType::RBRACE) {
            advance();
        }
        return make_unique<StringNode>(Token{TokenType::STRING, "{...}"});
    }
    default:
        advance();
        return parsePrimary();
    }
}
