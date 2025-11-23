#include "translator.h"
#include "types.h"
#include <stdexcept>
#include <iostream>

using namespace std;

Translator::Translator(const SymbolTable& symbolTable) : m_symbol_table(symbolTable) {}

QString Translator::translate(const ProgramNode* program) {
    declaredVariables.clear(); // Reset declarations for a fresh run
    QString result;

    // 1. C++ Headers
    result += "#include <iostream>\n";
    result += "#include <string>\n";
    result += "#include <vector>\n";
    result += "#include <cmath>\n";
    result += "#include <stdexcept>\n"; // Required for runtime_error
    result += "using namespace std;\n\n";

    // 2. Helper Functions Injection
    // We inject 'safe_divide' so 10/0 throws an error instead of crashing the program.
    result += "// Helper: Safe Division to allow try-catch handling\n";
    result += "template <typename T, typename U>\n";
    result += "double safe_divide(T a, U b) {\n";
    result += "    if (b == 0) throw runtime_error(\"Division by zero error\");\n";
    result += "    return (double)a / (double)b;\n";
    result += "}\n\n";

    QString functionsCode;
    QString mainBodyCode;

    // 3. Separate Functions from Main Script
    for (const auto& stmt : program->statements) {
        if (dynamic_cast<const FunctionDefNode*>(stmt.get())) {
            functionsCode += translateNode(stmt.get()) + "\n";
        } else {
            QString translatedStmt = translateNode(stmt.get());

            // Logic to determine if we need a semicolon
            // Blocks (ending in '}') typically don't need one, expressions do.
            if (!translatedStmt.endsWith("}")) {
                translatedStmt += ";";
            }

            mainBodyCode += "    " + translatedStmt + "\n";
        }
    }

    // 4. Assemble Final Output
    result += functionsCode;
    result += "int main() {\n";
    result += mainBodyCode;
    result += "\n    return 0;\n";
    result += "}\n";

    return result;
}

QString Translator::translateNode(const ASTNode* node) {
    if (!node) return "";

    // --- ASSIGNMENT ---
    if (auto p = dynamic_cast<const AssignmentNode*>(node)) {
        QString varName = p->identifier->token.value;
        QString expressionStr = translateNode(p->expression.get());
        QString typeStr = DataTypeToString(p->expression->determined_type);

        // Check if variable is already declared in C++ scope
        if (!declaredVariables.contains(varName)) {
            declaredVariables.insert(varName);
            return QString("%1 %2 = %3").arg(typeStr, varName, expressionStr);
        } else {
            return QString("%1 = %2").arg(varName, expressionStr);
        }
    }

    // --- BINARY OPERATIONS ---
    if (auto p = dynamic_cast<const BinaryOpNode*>(node)) {
        QString left = translateNode(p->left.get());
        QString right = translateNode(p->right.get());
        QString op = p->op.value;

        // Python -> C++ Operator Mapping
        if (op == "or") op = "||";
        else if (op == "and") op = "&&";

        // Handle Division safely
        else if (op == "/") {
            return QString("safe_divide(%1, %2)").arg(left, right);
        }

        return QString("(%1 %2 %3)").arg(left, op, right);
    }

    // --- UNARY OPERATIONS ---
    if (auto p = dynamic_cast<const UnaryOpNode*>(node)) {
        QString right = translateNode(p->right.get());
        QString op = p->op.value;
        if (op == "not") op = "!";
        return QString("(%1%2)").arg(op, right);
    }

    // --- LITERALS ---
    if (auto p = dynamic_cast<const IdentifierNode*>(node)) return p->token.value;
    if (auto p = dynamic_cast<const NumberNode*>(node)) return p->token.value;
    if (auto p = dynamic_cast<const StringNode*>(node)) return QString("\"%1\"").arg(p->token.value);
    if (dynamic_cast<const NoneNode*>(node)) return "nullptr";

    // --- PRINT ---
    if (auto p = dynamic_cast<const PrintNode*>(node)) {
        return QString("cout << %1 << endl").arg(translateNode(p->expression.get()));
    }

    // --- RETURN ---
    if (auto p = dynamic_cast<const ReturnNode*>(node)) {
        if (p->expression) return "return " + translateNode(p->expression.get());
        return "return";
    }

    // --- FUNCTION CALLS ---
    if (auto p = dynamic_cast<const FunctionCallNode*>(node)) {
        QString funcName = p->name->token.value;

        // Built-in Casts
        if (funcName == "int") {
            if (p->arguments.empty()) return "0";
            return "(int)(" + translateNode(p->arguments[0].get()) + ")";
        }
        if (funcName == "float") {
            if (p->arguments.empty()) return "0.0";
            return "(double)(" + translateNode(p->arguments[0].get()) + ")";
        }
        if (funcName == "str") {
            if (p->arguments.empty()) return "\"\"";
            return "to_string(" + translateNode(p->arguments[0].get()) + ")";
        }
        // Standard Call
        QString args;
        for (size_t i = 0; i < p->arguments.size(); ++i) {
            if (i > 0) args += ", ";
            args += translateNode(p->arguments[i].get());
        }
        return QString("%1(%2)").arg(funcName, args);
    }

    // --- IF STATEMENT ---
    if (auto p = dynamic_cast<const IfNode*>(node)) {
        QString condition = translateNode(p->condition.get());
        QString body = translateBlock(p->body.get());
        QString result = QString("if (%1) {\n%2    }").arg(condition, body);

        if (p->else_branch) {
            if (auto elseIf = dynamic_cast<const IfNode*>(p->else_branch.get())) {
                result += " else " + translateNode(elseIf);
            } else if (auto elseBlock = dynamic_cast<const BlockNode*>(p->else_branch.get())) {
                result += " else {\n" + translateBlock(elseBlock) + "    }";
            }
        }
        return result;
    }

    // --- WHILE LOOP ---
    if (auto p = dynamic_cast<const WhileNode*>(node)) {
        return QString("while (%1) {\n%2    }")
        .arg(translateNode(p->condition.get()), translateBlock(p->body.get()));
    }

    // --- FOR LOOP ---
    if (auto p = dynamic_cast<const ForNode*>(node)) {
        QString iterName = p->iterator->token.value;
        QString bodyStr = translateBlock(p->body.get());

        if (p->isRange) {
            // RANGE MODE: for(int i=0; i<10; i++)
            QString startStr = translateNode(p->start.get());
            QString stopStr = translateNode(p->stop.get());
            QString stepStr = translateNode(p->step.get());

            // Logic to handle ++ vs +=
            QString stepCode;
            if (stepStr == "1") stepCode = iterName + "++";
            else stepCode = iterName + " += " + stepStr;

            // Declare iterator inside the loop scope (C++ standard)
            return QString("for (int %1 = %2; %1 < %3; %4) {\n%5    }")
                .arg(iterName, startStr, stopStr, stepCode, bodyStr);
        } else {
            // GENERIC MODE: for(auto c : "text")
            QString iterableStr = translateNode(p->iterable.get());

            // Safety wrapper for string literals to ensure iterators work
            if (iterableStr.startsWith("\"")) {
                iterableStr = "string(" + iterableStr + ")";
            }

            return QString("for (auto %1 : %2) {\n%3    }")
                .arg(iterName, iterableStr, bodyStr);
        }
    }

    // --- TRY / EXCEPT ---
    if (auto p = dynamic_cast<const TryExceptNode*>(node)) {
        QString tryBody = translateBlock(p->try_body.get());
        QString exceptBody;

        if (p->except_body) {
            exceptBody = translateBlock(p->except_body.get());
        } else {
            // Default error message if no except block body provided (though parser usually ensures it)
            exceptBody = "        cout << \"An error occurred.\" << endl;\n";
        }

        // Map 'except' to 'catch (...)' which catches all C++ exceptions
        return QString("try {\n%1    } catch (...) {\n%2    }")
            .arg(tryBody, exceptBody);
    }

    // --- FUNCTION DEFINITION ---
    if (auto p = dynamic_cast<const FunctionDefNode*>(node)) {
        QString funcName = p->name->token.value;

        // Scope Handling: Save global declarations, clear for function, restore after
        QSet<QString> oldDeclared = declaredVariables;
        declaredVariables.clear();

        // Get return type from Symbol Table
        Symbol* sym = const_cast<SymbolTable&>(m_symbol_table).lookup(funcName);
        QString returnType = (sym && sym->functionReturnType != DataType::UNDEFINED)
                                 ? DataTypeToString(sym->functionReturnType)
                                 : "void";

        // Process Parameters
        QString params;
        for (size_t i = 0; i < p->parameters.size(); ++i) {
            if (i > 0) params += ", ";
            // Assuming params are Int for simplicity, or could be auto if using C++20 templates
            // For this implementation, we rely on SemanticAnalyzer defaults (usually Int)
            params += "int " + p->parameters[i]->token.value;
            declaredVariables.insert(p->parameters[i]->token.value);
        }

        QString body = translateBlock(p->body.get());

        // Restore Scope
        declaredVariables = oldDeclared;

        return QString("%1 %2(%3) {\n%4}\n").arg(returnType, funcName, params, body);
    }

    return "";
}

QString Translator::translateBlock(const BlockNode* block) {
    QString result;
    QString indent = "        "; // 8 spaces (inside main/func)

    for (const auto& stmt : block->statements) {
        QString translated = translateNode(stmt.get());

        if (translated.endsWith("}")) {
            // If it's a block-ender (if/while/try), no semicolon needed
            result += indent + translated + "\n";
        } else {
            // Standard statement needs semicolon
            result += indent + translated + ";\n";
        }
    }
    return result;
}
