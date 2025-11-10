#ifndef AST_H
#define AST_H

#include "token.h"
#include <memory>
#include <vector>

// Base class for all expression nodes
struct ASTNode {
    virtual ~ASTNode() = default;
};

// Node for numeric literals (e.g., 5)
struct NumberNode : ASTNode {
    Token token;
    explicit NumberNode(Token token) : token(std::move(token)) {}
};

// Node for identifiers (e.g., variables like 'x')
struct IdentifierNode : ASTNode {
    Token token;
    explicit IdentifierNode(Token token) : token(std::move(token)) {}
};

// Node for binary operations (e.g., left + right)
struct BinaryOpNode : ASTNode {
    std::unique_ptr<ASTNode> left;
    Token op;
    std::unique_ptr<ASTNode> right;

    BinaryOpNode(std::unique_ptr<ASTNode> left, Token op, std::unique_ptr<ASTNode> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
};

// Node for assignment (e.g., x = 5;)
struct AssignmentNode : ASTNode {
    std::unique_ptr<IdentifierNode> identifier;
    std::unique_ptr<ASTNode> expression;

    AssignmentNode(std::unique_ptr<IdentifierNode> id, std::unique_ptr<ASTNode> expr)
        : identifier(std::move(id)), expression(std::move(expr)) {}
};

// The root of the tree, which contains a list of statements
struct ProgramNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
};

#endif // AST_H
