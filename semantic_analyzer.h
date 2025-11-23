#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast.h"
#include "symbol_table.h"
#include <stdexcept>
#include <string>

using namespace std;

class SemanticError : public runtime_error {
public:
    explicit SemanticError(const string& message) : runtime_error(message) {}
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    void analyze(ProgramNode* program);

    // Expose symbol table for the Translator to use later
    const SymbolTable& getSymbolTable() const { return m_symbol_table; }

private:
    SymbolTable m_symbol_table;
    FunctionDefNode* m_current_function = nullptr; // To track return types

    void visit(ASTNode* node);
    DataType getExpressionType(ASTNode* node);
};

#endif // SEMANTIC_ANALYZER_H
