#ifndef AST_H
#define AST_H

#include "token.h"
#include "types.h"
#include <memory>
#include <vector>
#include <QString>
#include <utility>

using namespace std;

struct ASTNode {
    DataType determined_type = DataType::UNDEFINED;
    virtual ~ASTNode() = default;
    virtual QString getNodeName() const = 0;
    // New: Helper to get line number for error reporting
    virtual int getLine() const { return 0; }
};

struct ProgramNode : ASTNode {
    vector<unique_ptr<ASTNode>> statements;
    QString getNodeName() const override { return "Program"; }
};

struct NumberNode : ASTNode {
    Token token;
    explicit NumberNode(Token t) : token(std::move(t)) {}
    QString getNodeName() const override { return "Num: " + token.value; }
    int getLine() const override { return token.line; }
};

struct StringNode : ASTNode {
    Token token;
    explicit StringNode(Token t) : token(std::move(t)) {}
    QString getNodeName() const override { return "Str: \"" + token.value + "\""; }
    int getLine() const override { return token.line; }
};

struct IdentifierNode : ASTNode {
    Token token;
    explicit IdentifierNode(Token t) : token(std::move(t)) {}
    QString getNodeName() const override { return "ID: " + token.value; }
    int getLine() const override { return token.line; }
};

struct NoneNode : ASTNode {
    QString getNodeName() const override { return "None"; }
};

struct UnaryOpNode : ASTNode {
    Token op;
    unique_ptr<ASTNode> right;
    UnaryOpNode(Token o, unique_ptr<ASTNode> r) : op(std::move(o)), right(std::move(r)) {}
    QString getNodeName() const override { return "Unary Op: " + op.value; }
    int getLine() const override { return op.line; }
};

struct BinaryOpNode : ASTNode {
    unique_ptr<ASTNode> left;
    Token op;
    unique_ptr<ASTNode> right;
    BinaryOpNode(unique_ptr<ASTNode> l, Token o, unique_ptr<ASTNode> r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
    QString getNodeName() const override { return "Bin Op: " + op.value; }
    int getLine() const override { return op.line; }
};

struct AssignmentNode : ASTNode {
    unique_ptr<IdentifierNode> identifier;
    unique_ptr<ASTNode> expression;
    AssignmentNode(unique_ptr<IdentifierNode> id, unique_ptr<ASTNode> expr)
        : identifier(std::move(id)), expression(std::move(expr)) {}
    QString getNodeName() const override { return "Assign (=)"; }
    int getLine() const override { return identifier->getLine(); }
};

struct PrintNode : ASTNode {
    unique_ptr<ASTNode> expression;
    explicit PrintNode(unique_ptr<ASTNode> expr) : expression(std::move(expr)) {}
    QString getNodeName() const override { return "Print"; }
    int getLine() const override { return expression ? expression->getLine() : 0; }
};

struct ReturnNode : ASTNode {
    unique_ptr<ASTNode> expression;
    explicit ReturnNode(unique_ptr<ASTNode> expr) : expression(std::move(expr)) {}
    QString getNodeName() const override { return "Return"; }
    int getLine() const override { return expression ? expression->getLine() : 0; }
};

struct FunctionCallNode : ASTNode {
    unique_ptr<IdentifierNode> name;
    vector<unique_ptr<ASTNode>> arguments;
    FunctionCallNode(unique_ptr<IdentifierNode> n, vector<unique_ptr<ASTNode>> args)
        : name(std::move(n)), arguments(std::move(args)) {}
    QString getNodeName() const override { return "Call: " + name->token.value; }
    int getLine() const override { return name->getLine(); }
};

struct BlockNode : ASTNode {
    vector<unique_ptr<ASTNode>> statements;
    QString getNodeName() const override { return "Block"; }
};

struct IfNode : ASTNode {
    unique_ptr<ASTNode> condition;
    unique_ptr<BlockNode> body;
    unique_ptr<ASTNode> else_branch; // Can be BlockNode (else) or IfNode (elif)
    IfNode(unique_ptr<ASTNode> cond, unique_ptr<BlockNode> b)
        : condition(std::move(cond)), body(std::move(b)), else_branch(nullptr) {}
    QString getNodeName() const override { return "If"; }
    int getLine() const override { return condition ? condition->getLine() : 0; }
};

struct WhileNode : ASTNode {
    unique_ptr<ASTNode> condition;
    unique_ptr<BlockNode> body;
    WhileNode(unique_ptr<ASTNode> cond, unique_ptr<BlockNode> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    QString getNodeName() const override { return "While"; }
    int getLine() const override { return condition ? condition->getLine() : 0; }
};

struct FunctionDefNode : ASTNode {
    unique_ptr<IdentifierNode> name;
    vector<unique_ptr<IdentifierNode>> parameters;
    unique_ptr<BlockNode> body;
    FunctionDefNode(unique_ptr<IdentifierNode> n, vector<unique_ptr<IdentifierNode>> p, unique_ptr<BlockNode> b)
        : name(std::move(n)), parameters(std::move(p)), body(std::move(b)) {}
    QString getNodeName() const override { return "Def: " + name->token.value; }
    int getLine() const override { return name->getLine(); }
};

struct TryExceptNode : ASTNode {
    unique_ptr<BlockNode> try_body;
    unique_ptr<BlockNode> except_body;

    TryExceptNode(unique_ptr<BlockNode> tb, unique_ptr<BlockNode> eb)
        : try_body(std::move(tb)), except_body(std::move(eb)) {}
    QString getNodeName() const override { return "Try/Except"; }
};

struct ForNode : ASTNode {
    unique_ptr<IdentifierNode> iterator;

    // Range Loop Data
    unique_ptr<ASTNode> start;
    unique_ptr<ASTNode> stop;
    unique_ptr<ASTNode> step;

    // Generic Loop Data
    unique_ptr<ASTNode> iterable;

    unique_ptr<BlockNode> body;
    bool isRange;

    // Constructor for Range Loop
    ForNode(unique_ptr<IdentifierNode> iter, unique_ptr<ASTNode> s, unique_ptr<ASTNode> e, unique_ptr<ASTNode> st, unique_ptr<BlockNode> b)
        : iterator(std::move(iter)), start(std::move(s)), stop(std::move(e)), step(std::move(st)), iterable(nullptr), body(std::move(b)), isRange(true) {}

    // Constructor for Generic Loop
    ForNode(unique_ptr<IdentifierNode> iter, unique_ptr<ASTNode> src, unique_ptr<BlockNode> b)
        : iterator(std::move(iter)), start(nullptr), stop(nullptr), step(nullptr), iterable(std::move(src)), body(std::move(b)), isRange(false) {}

    QString getNodeName() const override { return isRange ? "For (Range)" : "For (Generic)"; }
    int getLine() const override { return iterator ? iterator->getLine() : 0; }
};

#endif // AST_H
