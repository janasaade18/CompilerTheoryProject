#include "semantic_analyzer.h"
#include "types.h"
#include <iostream>

using namespace std;

SemanticAnalyzer::SemanticAnalyzer() {
    // --- Define Built-in Functions ---
    m_symbol_table.define("print", DataType::FUNCTION);
    m_symbol_table.define("input", DataType::FUNCTION);

    // Built-in Casts / Helpers
    m_symbol_table.define("int", DataType::FUNCTION);
    m_symbol_table.define("float", DataType::FUNCTION);
    m_symbol_table.define("str", DataType::FUNCTION);
    m_symbol_table.define("range", DataType::FUNCTION);

    // Pre-set return types for built-ins
    if(auto sym = m_symbol_table.lookup("int")) sym->functionReturnType = DataType::INTEGER;
    if(auto sym = m_symbol_table.lookup("float")) sym->functionReturnType = DataType::FLOAT;
    if(auto sym = m_symbol_table.lookup("str")) sym->functionReturnType = DataType::STRING;
    if(auto sym = m_symbol_table.lookup("input")) sym->functionReturnType = DataType::STRING;
}

void SemanticAnalyzer::analyze(ProgramNode* program) {
    // Process all statements in the main body
    for (const auto& statement : program->statements) {
        visit(statement.get());
    }
}

void SemanticAnalyzer::visit(ASTNode* node) {
    if (!node) return;

    // --- 1. Assignment ---
    if (auto p = dynamic_cast<AssignmentNode*>(node)) {
        DataType exprType = getExpressionType(p->expression.get());

        // Check if variable exists
        Symbol* existing = m_symbol_table.lookup(p->identifier->token.value);

        if (existing) {
            if (existing->type != exprType) {
                if (existing->type == DataType::FLOAT && exprType == DataType::INTEGER) {
                    // Allow: x (float) = 5 (int)
                } else {
                    throw SemanticError("Type Mismatch: Variable '" + p->identifier->token.value.toStdString() +
                                        "' is type " + DataTypeToString(existing->type).toStdString() +
                                        " but assigned " + DataTypeToString(exprType).toStdString());
                }
            }
        } else {
            // New Variable Definition
            m_symbol_table.define(p->identifier->token.value, exprType);
        }

        // Annotate AST for Translator
        p->identifier->determined_type = exprType;
        p->determined_type = exprType;
    }

    // --- 2. Function Definition ---
    else if (auto p = dynamic_cast<FunctionDefNode*>(node)) {
        QString funcName = p->name->token.value;
        if (!m_symbol_table.define(funcName, DataType::FUNCTION)) {
            throw SemanticError("Function '" + funcName.toStdString() + "' already defined.");
        }

        m_current_function = p; // Track current function context
        m_symbol_table.enterScope(); // Scope for params and body

        // Define Parameters
        for(const auto& param : p->parameters) {
            param->determined_type = DataType::INTEGER;
            m_symbol_table.define(param->token.value, DataType::INTEGER);
        }

        visit(p->body.get());

        m_symbol_table.leaveScope();
        m_current_function = nullptr;
    }

    // --- 3. For Loop (Range vs Generic) ---
    else if (auto p = dynamic_cast<ForNode*>(node)) {
        m_symbol_table.enterScope();

        if (p->isRange) {
            if(getExpressionType(p->start.get()) != DataType::INTEGER)
                throw SemanticError("Loop range 'start' must be Integer.");
            if(getExpressionType(p->stop.get()) != DataType::INTEGER)
                throw SemanticError("Loop range 'stop' must be Integer.");

            p->iterator->determined_type = DataType::INTEGER;
            m_symbol_table.define(p->iterator->token.value, DataType::INTEGER);
        }
        else {
            DataType iterType = getExpressionType(p->iterable.get());
            if (iterType == DataType::STRING) {
                p->iterator->determined_type = DataType::STRING;
                m_symbol_table.define(p->iterator->token.value, DataType::STRING);
            } else {
                m_symbol_table.define(p->iterator->token.value, DataType::UNDEFINED);
            }
        }

        visit(p->body.get());
        m_symbol_table.leaveScope();
    }

    // --- 4. If Statement ---
    else if (auto p = dynamic_cast<IfNode*>(node)) {
        getExpressionType(p->condition.get());
        visit(p->body.get());
        if (p->else_branch) {
            visit(p->else_branch.get());
        }
    }

    // --- 5. While Loop ---
    else if (auto p = dynamic_cast<WhileNode*>(node)) {
        getExpressionType(p->condition.get());
        visit(p->body.get());
    }

    // --- 6. Try / Except ---
    else if (auto p = dynamic_cast<TryExceptNode*>(node)) {
        visit(p->try_body.get());
        if (p->except_body) {
            visit(p->except_body.get());
        }
    }

    // --- 7. Return Statement ---
    else if (auto p = dynamic_cast<ReturnNode*>(node)) {
        if (!m_current_function) {
            throw SemanticError("Return statement outside of function.");
        }

        DataType returnType = DataType::NONE;
        if (p->expression) {
            returnType = getExpressionType(p->expression.get());
        }

        Symbol* funcSym = m_symbol_table.lookup(m_current_function->name->token.value);
        if (funcSym) {
            if (funcSym->functionReturnType == DataType::UNDEFINED) {
                funcSym->functionReturnType = returnType;
            } else if (funcSym->functionReturnType != returnType) {
                if (funcSym->functionReturnType == DataType::FLOAT && returnType == DataType::INTEGER) {
                    // OK
                } else {
                    throw SemanticError("Inconsistent return types in function '" +
                                        m_current_function->name->token.value.toStdString() +
                                        "'. Expected " + DataTypeToString(funcSym->functionReturnType).toStdString() +
                                        ", got " + DataTypeToString(returnType).toStdString());
                }
            }
        }
    }

    // --- 8. Expression Statements ---
    else if (auto p = dynamic_cast<PrintNode*>(node)) {
        getExpressionType(p->expression.get());
    }
    else if (auto p = dynamic_cast<BlockNode*>(node)) {
        for(const auto& stmt : p->statements) visit(stmt.get());
    }
    else if (dynamic_cast<FunctionCallNode*>(node) || dynamic_cast<IdentifierNode*>(node)) {
        getExpressionType(node);
    }
}

DataType SemanticAnalyzer::getExpressionType(ASTNode* node) {
    if (!node) return DataType::UNDEFINED;

    if (auto p = dynamic_cast<NumberNode*>(node)) {
        if (p->token.value.contains('.')) {
            p->determined_type = DataType::FLOAT;
            return DataType::FLOAT;
        }
        p->determined_type = DataType::INTEGER;
        return DataType::INTEGER;
    }
    if (auto p = dynamic_cast<StringNode*>(node)) {
        p->determined_type = DataType::STRING;
        return DataType::STRING;
    }
    if (auto p = dynamic_cast<NoneNode*>(node)) {
        p->determined_type = DataType::NONE;
        return DataType::NONE;
    }

    if (auto p = dynamic_cast<IdentifierNode*>(node)) {
        Symbol* sym = m_symbol_table.lookup(p->token.value);
        if (!sym) {
            throw SemanticError("Variable '" + p->token.value.toStdString() + "' is not defined.");
        }
        p->determined_type = sym->type;
        return sym->type;
    }

    if (auto p = dynamic_cast<BinaryOpNode*>(node)) {
        DataType left = getExpressionType(p->left.get());
        DataType right = getExpressionType(p->right.get());

        if (p->op.type == TokenType::PLUS || p->op.type == TokenType::MINUS ||
            p->op.type == TokenType::STAR || p->op.type == TokenType::SLASH) {

            if (left == DataType::STRING || right == DataType::STRING) {
                if (p->op.type == TokenType::PLUS) {
                    p->determined_type = DataType::STRING;
                    return DataType::STRING;
                }
                throw SemanticError("Cannot perform arithmetic on Strings (except +).");
            }

            if (left == DataType::FLOAT || right == DataType::FLOAT) {
                p->determined_type = DataType::FLOAT;
                return DataType::FLOAT;
            }

            p->determined_type = DataType::INTEGER;
            return DataType::INTEGER;
        }

        if (p->op.type == TokenType::GREATER || p->op.type == TokenType::LESS_EQUAL ||
            p->op.type == TokenType::DOUBLE_EQUAL) {
            p->determined_type = DataType::BOOLEAN;
            return DataType::BOOLEAN;
        }

        if (p->op.type == TokenType::OR || p->op.type == TokenType::NOT) {
            p->determined_type = DataType::BOOLEAN;
            return DataType::BOOLEAN;
        }
    }

    if (auto p = dynamic_cast<UnaryOpNode*>(node)) {
        DataType t = getExpressionType(p->right.get());
        if (p->op.type == TokenType::NOT) {
            p->determined_type = DataType::BOOLEAN;
            return DataType::BOOLEAN;
        }
        p->determined_type = t;
        return t;
    }

    if (auto p = dynamic_cast<FunctionCallNode*>(node)) {
        Symbol* sym = m_symbol_table.lookup(p->name->token.value);
        if (!sym) throw SemanticError("Function '" + p->name->token.value.toStdString() + "' not defined.");

        for(auto& arg : p->arguments) {
            getExpressionType(arg.get());
        }

        if (sym->functionReturnType != DataType::UNDEFINED) {
            p->determined_type = sym->functionReturnType;
            return sym->functionReturnType;
        }
        return DataType::NONE;
    }

    return DataType::UNDEFINED;
}
