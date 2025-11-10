#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <utility>

using namespace std;

// AUTOMATON STATES
enum class ParserState {
    START,
    EXPECT_STATEMENT,
    IN_FUNCTION_DEF,
    IN_FUNCTION_PARAMS,
    IN_FUNCTION_BODY,
    IN_IF_CONDITION,
    IN_IF_BODY,
    IN_ASSIGNMENT,
    IN_EXPRESSION,
    IN_TERM,
    IN_FACTOR,
    IN_FUNCTION_CALL,
    IN_TRY_BLOCK,
    IN_EXCEPT_BLOCK,
    EXPECT_OPERATOR,
    EXPECT_OPERAND,
    END_STATEMENT
};

struct AutomatonTransition {
    ParserState fromState;
    ParserState toState;
    TokenType triggerToken;

    AutomatonTransition(ParserState from, ParserState to, TokenType token)
        : fromState(from), toState(to), triggerToken(token) {}
};

class Parser {
public:
    Parser(const vector<Token>& tokens);
    unique_ptr<ProgramNode> parse();
    vector<pair<ParserState, Token>> getStateHistory() const { return m_state_history; }
    vector<AutomatonTransition> getTransitions() const { return m_transitions; }

private:

    vector<Token> m_tokens;
    int m_pos = 0;
    ParserState m_current_state = ParserState::START;
    vector<pair<ParserState, Token>> m_state_history;
    vector<AutomatonTransition> m_transitions;

    void changeState(ParserState newState, Token triggerToken, const QString& description);

    Token currentToken();
    Token peekToken();
    void advance();
    void expect(TokenType type);

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
