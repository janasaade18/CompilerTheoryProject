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

Token Parser::peekToken(int offset) {
    if (m_pos + offset >= m_tokens.size()) return {TokenType::END_OF_FILE, ""};
    return m_tokens[m_pos + offset];
}

void Parser::advance() {
    if (m_pos < m_tokens.size()) m_pos++;
}

void Parser::expect(TokenType type) {
    if (currentToken().type == type) {
        advance();
    } else {
        // Simple error recovery: Skip to next line or just advance
        advance();
    }
}

unique_ptr<ProgramNode> Parser::parse() {
    auto programNode = make_unique<ProgramNode>();

    while (currentToken().type != TokenType::END_OF_FILE) {
        if (currentToken().type == TokenType::DEDENT || currentToken().type == TokenType::INDENT) {
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

    while (currentToken().type != TokenType::DEDENT && currentToken().type != TokenType::END_OF_FILE) {
        if (currentToken().type == TokenType::INDENT) {
            advance(); // skip extra indent
            continue;
        }

        changeState(ParserState::EXPECT_STATEMENT, currentToken(), "Block statement");
        auto stmt = parseStatement();
        if (stmt) block->statements.push_back(std::move(stmt));
    }

    if (currentToken().type == TokenType::DEDENT) {
        advance();
    }

    return block;
}

unique_ptr<ASTNode> Parser::parseStatement() {
    // Clean up any leading indentation tokens
    while (currentToken().type == TokenType::INDENT || currentToken().type == TokenType::DEDENT) {
        advance();
    }
    if (currentToken().type == TokenType::END_OF_FILE) return nullptr;

    switch(currentToken().type) {
    case TokenType::DEF:    return parseFunctionDefinition();
    case TokenType::IF:     return parseIfStatement();
    case TokenType::WHILE:  return parseWhileStatement();
    case TokenType::FOR:    return parseForStatement();
    case TokenType::TRY:    return parseTryExceptStatement();
    case TokenType::RETURN:
        changeState(ParserState::IN_EXPRESSION, currentToken(), "Return");
        advance();
        return make_unique<ReturnNode>(parseExpression());
    case TokenType::PRINT:
        changeState(ParserState::IN_EXPRESSION, currentToken(), "Print");
        advance();
        return make_unique<PrintNode>(parseExpression());
    case TokenType::IDENTIFIER:
        return parseAssignmentOrExpression();
    default:
        return parseExpression();
    }
}

unique_ptr<ASTNode> Parser::parseAssignmentOrExpression() {
    Token idToken = currentToken();
    Token next1 = peekToken(1);
    Token next2 = peekToken(2);

    // Case 1: Standard Assignment (x = 5)
    if (next1.type == TokenType::EQUAL) {
        changeState(ParserState::IN_ASSIGNMENT, currentToken(), "Standard Assignment");
        auto idNode = make_unique<IdentifierNode>(idToken);
        advance(); // consume ID
        advance(); // consume =
        auto expr = parseExpression();
        return make_unique<AssignmentNode>(std::move(idNode), std::move(expr));
    }

    // Case 2: Complex Assignment via Desugaring (x += 5 -> x = x + 5)
    // Lexer gives tokens: ID, PLUS, EQUAL, NUM
    bool isComplex = false;
    TokenType binOp;

    if (next2.type == TokenType::EQUAL) {
        if (next1.type == TokenType::PLUS) { isComplex = true; binOp = TokenType::PLUS; }
        else if (next1.type == TokenType::MINUS) { isComplex = true; binOp = TokenType::MINUS; }
        else if (next1.type == TokenType::STAR) { isComplex = true; binOp = TokenType::STAR; }
        else if (next1.type == TokenType::SLASH) { isComplex = true; binOp = TokenType::SLASH; }
    }

    if (isComplex) {
        changeState(ParserState::IN_ASSIGNMENT, currentToken(), "Complex Assignment");
        // Construct: ID = ID op Expr
        auto leftId = make_unique<IdentifierNode>(idToken); // For LHS
        auto rightId = make_unique<IdentifierNode>(idToken); // For RHS inside binary op

        advance(); // consume ID
        Token opToken = currentToken(); // The +, -, *, /
        advance(); // consume Op
        advance(); // consume =

        auto rightExpr = parseExpression();

        auto binaryOpNode = make_unique<BinaryOpNode>(std::move(rightId), opToken, std::move(rightExpr));
        return make_unique<AssignmentNode>(std::move(leftId), std::move(binaryOpNode));
    }

    // Case 3: Expression / Function Call
    auto idNode = make_unique<IdentifierNode>(idToken);
    advance(); // consume ID

    if (currentToken().type == TokenType::LPAREN) {
        changeState(ParserState::IN_FUNCTION_CALL, currentToken(), "Function Call");
        advance(); // (
        vector<unique_ptr<ASTNode>> args;
        if (currentToken().type != TokenType::RPAREN) {
            args.push_back(parseExpression());
            while (currentToken().type == TokenType::COMMA) {
                advance();
                args.push_back(parseExpression());
            }
        }
        expect(TokenType::RPAREN);
        return make_unique<FunctionCallNode>(std::move(idNode), std::move(args));
    }

    return idNode;
}

unique_ptr<ASTNode> Parser::parseFunctionDefinition() {
    changeState(ParserState::IN_FUNCTION_DEF, currentToken(), "Func Def");
    expect(TokenType::DEF);
    auto name = make_unique<IdentifierNode>(currentToken());
    expect(TokenType::IDENTIFIER);

    changeState(ParserState::IN_FUNCTION_PARAMS, currentToken(), "Func Params");
    expect(TokenType::LPAREN);
    vector<unique_ptr<IdentifierNode>> params;

    if (currentToken().type != TokenType::RPAREN) {
        params.push_back(make_unique<IdentifierNode>(currentToken()));
        expect(TokenType::IDENTIFIER);
        while (currentToken().type == TokenType::COMMA) {
            advance();
            params.push_back(make_unique<IdentifierNode>(currentToken()));
            expect(TokenType::IDENTIFIER);
        }
    }
    expect(TokenType::RPAREN);
    expect(TokenType::COLON);

    changeState(ParserState::IN_FUNCTION_BODY, currentToken(), "Func Body");
    auto body = parseBlock();
    return make_unique<FunctionDefNode>(std::move(name), std::move(params), std::move(body));
}

unique_ptr<ASTNode> Parser::parseForStatement() {
    changeState(ParserState::IN_IF_CONDITION, currentToken(), "For Loop");
    expect(TokenType::FOR);
    auto iterator = make_unique<IdentifierNode>(currentToken());
    expect(TokenType::IDENTIFIER);
    expect(TokenType::IN);

    // Check if generic or range
    if (currentToken().type == TokenType::IDENTIFIER && currentToken().value == "range") {
        // --- RANGE LOOP ---
        advance(); // consume 'range'
        expect(TokenType::LPAREN);
        vector<unique_ptr<ASTNode>> args;
        args.push_back(parseExpression());
        while (currentToken().type == TokenType::COMMA) {
            advance();
            args.push_back(parseExpression());
        }
        expect(TokenType::RPAREN);
        expect(TokenType::COLON);

        unique_ptr<ASTNode> start, stop, step;
        if (args.size() == 1) {
            start = make_unique<NumberNode>(Token{TokenType::NUMBER, "0", 0});
            stop = std::move(args[0]);
            step = make_unique<NumberNode>(Token{TokenType::NUMBER, "1", 0});
        } else if (args.size() == 2) {
            start = std::move(args[0]);
            stop = std::move(args[1]);
            step = make_unique<NumberNode>(Token{TokenType::NUMBER, "1", 0});
        } else if (args.size() >= 3) {
            start = std::move(args[0]);
            stop = std::move(args[1]);
            step = std::move(args[2]);
        }

        changeState(ParserState::IN_IF_BODY, currentToken(), "For Body");
        auto body = parseBlock();
        return make_unique<ForNode>(std::move(iterator), std::move(start), std::move(stop), std::move(step), std::move(body));
    }
    else {
        // --- GENERIC LOOP ---
        auto iterable = parseExpression();
        expect(TokenType::COLON);
        changeState(ParserState::IN_IF_BODY, currentToken(), "For Body");
        auto body = parseBlock();
        return make_unique<ForNode>(std::move(iterator), std::move(iterable), std::move(body));
    }
}

unique_ptr<ASTNode> Parser::parseIfStatement() {
    changeState(ParserState::IN_IF_CONDITION, currentToken(), "If Condition");
    if (currentToken().type == TokenType::ELIF) expect(TokenType::ELIF);
    else expect(TokenType::IF);

    auto condition = parseExpression();
    expect(TokenType::COLON);
    changeState(ParserState::IN_IF_BODY, currentToken(), "If Body");
    auto body = parseBlock();

    auto ifNode = make_unique<IfNode>(std::move(condition), std::move(body));

    while (currentToken().type == TokenType::INDENT || currentToken().type == TokenType::DEDENT) advance();

    if (currentToken().type == TokenType::ELIF) {
        ifNode->else_branch = parseIfStatement();
    } else if (currentToken().type == TokenType::ELSE) {
        advance();
        expect(TokenType::COLON);
        ifNode->else_branch = parseBlock();
    }
    return ifNode;
}

unique_ptr<ASTNode> Parser::parseWhileStatement() {
    changeState(ParserState::IN_IF_CONDITION, currentToken(), "While Condition");
    expect(TokenType::WHILE);
    auto condition = parseExpression();
    expect(TokenType::COLON);
    changeState(ParserState::IN_IF_BODY, currentToken(), "While Body");
    auto body = parseBlock();
    return make_unique<WhileNode>(std::move(condition), std::move(body));
}

unique_ptr<ASTNode> Parser::parseTryExceptStatement() {
    changeState(ParserState::IN_TRY_BLOCK, currentToken(), "Try Block");
    expect(TokenType::TRY);
    expect(TokenType::COLON);
    auto tryBody = parseBlock();

    while (currentToken().type == TokenType::INDENT || currentToken().type == TokenType::DEDENT) advance();

    unique_ptr<BlockNode> exceptBody = nullptr;
    if (currentToken().type == TokenType::EXCEPT) {
        changeState(ParserState::IN_EXCEPT_BLOCK, currentToken(), "Except Block");
        advance();
        expect(TokenType::COLON);
        exceptBody = parseBlock();
    }
    return make_unique<TryExceptNode>(std::move(tryBody), std::move(exceptBody));
}

// --- Expression Parsing ---

unique_ptr<ASTNode> Parser::parseExpression() {
    changeState(ParserState::IN_EXPRESSION, currentToken(), "Expression");
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
    if (currentToken().type == TokenType::NOT || currentToken().type == TokenType::MINUS) {
        Token op = currentToken();
        advance();
        auto right = parseUnary();
        return make_unique<UnaryOpNode>(op, std::move(right));
    }
    return parsePrimary();
}

unique_ptr<ASTNode> Parser::parsePrimary() {
    Token t = currentToken();
    switch(t.type) {
    case TokenType::NONE:   advance(); return make_unique<NoneNode>();
    case TokenType::TRUE:   advance(); return make_unique<NumberNode>(Token{TokenType::NUMBER, "1"});
    case TokenType::FALSE:  advance(); return make_unique<NumberNode>(Token{TokenType::NUMBER, "0"});
    case TokenType::NUMBER: advance(); return make_unique<NumberNode>(t);
    case TokenType::STRING: advance(); return make_unique<StringNode>(t);
    case TokenType::LPAREN: {
        advance();
        auto expr = parseExpression();
        expect(TokenType::RPAREN);
        return expr;
    }
    case TokenType::IDENTIFIER: {
        auto name = make_unique<IdentifierNode>(t);
        advance();
        // Function Call Check
        if (currentToken().type == TokenType::LPAREN) {
            advance();
            vector<unique_ptr<ASTNode>> args;
            if (currentToken().type != TokenType::RPAREN) {
                args.push_back(parseExpression());
                while (currentToken().type == TokenType::COMMA) {
                    advance();
                    args.push_back(parseExpression());
                }
            }
            expect(TokenType::RPAREN);
            return make_unique<FunctionCallNode>(std::move(name), std::move(args));
        }
        return name;
    }
    default:
        advance();
        return nullptr;
    }
}
