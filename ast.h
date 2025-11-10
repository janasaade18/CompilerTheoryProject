#ifndef AST_H
#define AST_H

#include "token.h"
#include <memory>
#include <vector>
#include <QString>
#include <utility>

using namespace std;

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual QString getNodeName() const = 0;
};

struct ProgramNode : ASTNode {
    vector<unique_ptr<ASTNode>> statements;
    QString getNodeName() const override { return "Program"; }
};

struct NumberNode : ASTNode {
    Token token;
    explicit NumberNode(Token t) : token(std::move(t)) {}
    QString getNodeName() const override { return "Num: " + token.value; }
};

struct StringNode : ASTNode {
    Token token;
    explicit StringNode(Token t) : token(std::move(t)) {}
    QString getNodeName() const override { return "Str: \"" + token.value + "\""; }
};

struct IdentifierNode : ASTNode {
    Token token;
    explicit IdentifierNode(Token t) : token(std::move(t)) {}
    QString getNodeName() const override { return "ID: " + token.value; }
};

struct NoneNode : ASTNode {
    QString getNodeName() const override { return "None"; }
};

struct UnaryOpNode : ASTNode {
    Token op;
    unique_ptr<ASTNode> right;
    UnaryOpNode(Token o, unique_ptr<ASTNode> r) : op(std::move(o)), right(std::move(r)) {}
    QString getNodeName() const override { return "Op: " + op.value; }
};

struct BinaryOpNode : ASTNode {
    unique_ptr<ASTNode> left;
    Token op;
    unique_ptr<ASTNode> right;
    BinaryOpNode(unique_ptr<ASTNode> l, Token o, unique_ptr<ASTNode> r) : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
    QString getNodeName() const override { return "Op: " + op.value; }
};

struct AssignmentNode : ASTNode {
    unique_ptr<IdentifierNode> identifier;
    unique_ptr<ASTNode> expression;
    AssignmentNode(unique_ptr<IdentifierNode> id, unique_ptr<ASTNode> expr) : identifier(std::move(id)), expression(std::move(expr)) {}
    QString getNodeName() const override { return "Assign (=)"; }
};

struct PrintNode : ASTNode {
    unique_ptr<ASTNode> expression;
    explicit PrintNode(unique_ptr<ASTNode> expr) : expression(std::move(expr)) {}
    QString getNodeName() const override { return "Print"; }
};

struct ReturnNode : ASTNode {
    unique_ptr<ASTNode> expression;
    explicit ReturnNode(unique_ptr<ASTNode> expr) : expression(std::move(expr)) {}
    QString getNodeName() const override { return "Return"; }
};

struct FunctionCallNode : ASTNode {
    unique_ptr<IdentifierNode> name;
    vector<unique_ptr<ASTNode>> arguments;
    FunctionCallNode(unique_ptr<IdentifierNode> n, vector<unique_ptr<ASTNode>> args) : name(std::move(n)), arguments(std::move(args)) {}
    QString getNodeName() const override { return "Call: " + name->token.value; }
};

struct BlockNode : ASTNode {
    vector<unique_ptr<ASTNode>> statements;
    QString getNodeName() const override { return "Block"; }
};

struct IfNode : ASTNode {
    unique_ptr<ASTNode> condition;
    unique_ptr<BlockNode> body;
    IfNode(unique_ptr<ASTNode> cond, unique_ptr<BlockNode> b) : condition(std::move(cond)), body(std::move(b)) {}
    QString getNodeName() const override { return "If"; }
};

struct FunctionDefNode : ASTNode {
    unique_ptr<IdentifierNode> name;
    vector<unique_ptr<IdentifierNode>> parameters;
    unique_ptr<BlockNode> body;
    FunctionDefNode(unique_ptr<IdentifierNode> n, vector<unique_ptr<IdentifierNode>> p, unique_ptr<BlockNode> b) : name(std::move(n)), parameters(std::move(p)), body(std::move(b)) {}
    QString getNodeName() const override { return "Def: " + name->token.value; }
};

struct TryExceptNode : ASTNode {
    unique_ptr<BlockNode> try_body;
    explicit TryExceptNode(unique_ptr<BlockNode> body) : try_body(std::move(body)) {}
    QString getNodeName() const override { return "Try Block"; }
};

#endif
