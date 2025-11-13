#include "parser.h"
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

Parser::Parser(const vector<Token>& tokens) : m_tokens(tokens) {
    m_state_history.push_back({m_current_state, Token{TokenType::END_OF_FILE, "START"}});
}

void Parser::changeState(ParserState newState, Token triggerToken, const QString& description) {
    m_transitions.push_back(AutomatonTransition(m_current_state, newState, triggerToken.type));
    m_current_state = newState;
    m_state_history.push_back({m_current_state, triggerToken});
}

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
        advance(); // Skip on error
    }
}

unique_ptr<ProgramNode> Parser::parse() {
    auto programNode = make_unique<ProgramNode>();

    while (currentToken().type != TokenType::END_OF_FILE) {
        if (currentToken().type == TokenType::DEDENT ||
            currentToken().type == TokenType::INDENT) {
            advance();
            continue;
        }

        changeState(ParserState::EXPECT_STATEMENT, currentToken(), "Start parsing statement");
        auto stmt = parseStatement();
        if (stmt) {
            programNode->statements.push_back(std::move(stmt));
        }
        changeState(ParserState::END_STATEMENT, currentToken(), "Finished statement");
    }

    return programNode;
}

unique_ptr<BlockNode> Parser::parseBlock() {
    auto block = make_unique<BlockNode>();

    if (currentToken().type == TokenType::INDENT) {
        advance();
    }

    while (currentToken().type != TokenType::DEDENT &&
           currentToken().type != TokenType::END_OF_FILE) {

        if (currentToken().type == TokenType::INDENT) {
            advance();
            continue;
        }

        changeState(ParserState::EXPECT_STATEMENT, currentToken(), "Start block statement");
        auto stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
    }

    if (currentToken().type == TokenType::DEDENT) {
        advance();
    }

    return block;
}

unique_ptr<ASTNode> Parser::parseStatement() {
    while (currentToken().type == TokenType::INDENT ||
           currentToken().type == TokenType::DEDENT) {
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
    case TokenType::WHILE:
        return parseWhileStatement();
    case TokenType::TRY:
        return parseTryExceptStatement();
    case TokenType::RETURN:
        changeState(ParserState::IN_EXPRESSION, currentToken(), "Return statement");
        advance();
        return make_unique<ReturnNode>(parseExpression());
    case TokenType::PRINT:
        changeState(ParserState::IN_EXPRESSION, currentToken(), "Print statement");
        advance();
        return make_unique<PrintNode>(parseExpression());
    case TokenType::IDENTIFIER:
        return parseAssignmentOrExpression();
    default:
        changeState(ParserState::IN_EXPRESSION, currentToken(), "Expression statement");
        return parseExpression();
    }
}

unique_ptr<ASTNode> Parser::parseAssignmentOrExpression() {
    auto identifier = make_unique<IdentifierNode>(currentToken());

    if (peekToken().type == TokenType::EQUAL) {
        changeState(ParserState::IN_ASSIGNMENT, currentToken(), "Assignment detected");
        advance();
        advance();
        auto expr = parseExpression();
        return make_unique<AssignmentNode>(std::move(identifier), std::move(expr));
    }

    changeState(ParserState::IN_EXPRESSION, currentToken(), "Identifier expression");
    advance();
    return identifier;
}

unique_ptr<ASTNode> Parser::parseTryExceptStatement() {
    changeState(ParserState::IN_TRY_BLOCK, currentToken(), "Start try block");
    expect(TokenType::TRY);
    expect(TokenType::COLON);
    auto try_body = parseBlock();

    if (currentToken().type == TokenType::EXCEPT) {
        changeState(ParserState::IN_EXCEPT_BLOCK, currentToken(), "Start except block");
        advance();
        expect(TokenType::COLON);
        parseBlock();
    }

    return make_unique<TryExceptNode>(std::move(try_body));
}

unique_ptr<ASTNode> Parser::parseFunctionDefinition() {
    changeState(ParserState::IN_FUNCTION_DEF, currentToken(), "Start function definition");
    expect(TokenType::DEF);
    auto name = make_unique<IdentifierNode>(currentToken());
    expect(TokenType::IDENTIFIER);

    changeState(ParserState::IN_FUNCTION_PARAMS, currentToken(), "Start function parameters");
    expect(TokenType::LPAREN);

    vector<unique_ptr<IdentifierNode>> params;
    if (currentToken().type != TokenType::RPAREN) {
        params.push_back(make_unique<IdentifierNode>(currentToken()));
        expect(TokenType::IDENTIFIER);
    }

    expect(TokenType::RPAREN);
    expect(TokenType::COLON);

    changeState(ParserState::IN_FUNCTION_BODY, currentToken(), "Start function body");
    auto body = parseBlock();
    return make_unique<FunctionDefNode>(std::move(name), std::move(params), std::move(body));
}

unique_ptr<ASTNode> Parser::parseIfStatement() {
    changeState(ParserState::IN_IF_CONDITION, currentToken(), "Start if condition");
    // 'elif' is just a keyword variation of 'if', so we consume either.
    if (currentToken().type == TokenType::ELIF) {
        expect(TokenType::ELIF);
    } else {
        expect(TokenType::IF);
    }
    auto condition = parseExpression();
    expect(TokenType::COLON);

    changeState(ParserState::IN_IF_BODY, currentToken(), "Start if body");
    auto body = parseBlock();

    auto ifNode = make_unique<IfNode>(std::move(condition), std::move(body));

    if (currentToken().type == TokenType::ELIF) {
        ifNode->else_branch = parseIfStatement();
    } else if (currentToken().type == TokenType::ELSE) {
        advance(); // consume 'else'
        expect(TokenType::COLON);
        changeState(ParserState::IN_IF_BODY, currentToken(), "Start else body");
        ifNode->else_branch = parseBlock();
    }

    return ifNode;
}

unique_ptr<ASTNode> Parser::parseWhileStatement() {
    changeState(ParserState::IN_IF_CONDITION, currentToken(), "Start while loop");
    expect(TokenType::WHILE);
    auto condition = parseExpression();
    expect(TokenType::COLON);

    changeState(ParserState::IN_IF_BODY, currentToken(), "Start while body");
    auto body = parseBlock();
    return make_unique<WhileNode>(std::move(condition), std::move(body));
}

unique_ptr<ASTNode> Parser::parseExpression() {
    changeState(ParserState::IN_EXPRESSION, currentToken(), "Start expression");
    return parseLogicalOr();
}

unique_ptr<ASTNode> Parser::parseLogicalOr() {
    auto node = parseComparison();
    while (currentToken().type == TokenType::OR) {
        changeState(ParserState::EXPECT_OPERAND, currentToken(), "OR operator");
        Token op = currentToken();
        advance();
        auto right = parseComparison();
        node = make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
        changeState(ParserState::IN_EXPRESSION, currentToken(), "Continue expression");
    }
    return node;
}

unique_ptr<ASTNode> Parser::parseComparison() {
    auto node = parseTerm();
    while (currentToken().type == TokenType::GREATER ||
           currentToken().type == TokenType::LESS_EQUAL ||
           currentToken().type == TokenType::DOUBLE_EQUAL) {
        changeState(ParserState::EXPECT_OPERAND, currentToken(), "Comparison operator");
        Token op = currentToken();
        advance();
        auto right = parseTerm();
        node = make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
        changeState(ParserState::IN_EXPRESSION, currentToken(), "Continue expression");
    }
    return node;
}

unique_ptr<ASTNode> Parser::parseTerm() {
    changeState(ParserState::IN_TERM, currentToken(), "Start term");
    auto node = parseFactor();
    while (currentToken().type == TokenType::PLUS || currentToken().type == TokenType::MINUS) {
        changeState(ParserState::EXPECT_OPERAND, currentToken(), "Add/sub operator");
        Token op = currentToken();
        advance();
        auto right = parseFactor();
        node = make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
        changeState(ParserState::IN_TERM, currentToken(), "Continue term");
    }
    return node;
}

unique_ptr<ASTNode> Parser::parseFactor() {
    changeState(ParserState::IN_FACTOR, currentToken(), "Start factor");
    auto node = parseUnary();
    while (currentToken().type == TokenType::STAR || currentToken().type == TokenType::SLASH) {
        changeState(ParserState::EXPECT_OPERAND, currentToken(), "Mul/div operator");
        Token op = currentToken();
        advance();
        auto right = parseUnary();
        node = make_unique<BinaryOpNode>(std::move(node), op, std::move(right));
        changeState(ParserState::IN_FACTOR, currentToken(), "Continue factor");
    }
    return node;
}

unique_ptr<ASTNode> Parser::parseUnary() {
    if (currentToken().type == TokenType::NOT) {
        changeState(ParserState::EXPECT_OPERAND, currentToken(), "NOT operator");
        Token op = currentToken();
        advance();
        auto right = parseUnary();
        return make_unique<UnaryOpNode>(op, std::move(right));
    }
    return parsePrimary();
}

unique_ptr<ASTNode> Parser::parsePrimary() {
    changeState(ParserState::EXPECT_OPERAND, currentToken(), "Expect primary operand");

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
            changeState(ParserState::IN_FUNCTION_CALL, currentToken(), "Function call");
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
