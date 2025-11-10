#include "translator.h"
#include <stdexcept>

QString Translator::translate(const ProgramNode* program) {
    QString result;

    // Add necessary includes
    result += "#include <iostream>\n";
    result += "#include <string>\n";
    result += "#include <map>\n";
    result += "#include <any>\n";
    result += "#include <stdexcept>\n\n";
    result += "using namespace std;\n\n";

    // Translate all statements (functions first, then main logic)
    QString functions;
    QString mainBody;

    for (const auto& stmt : program->statements) {
        if (dynamic_cast<const FunctionDefNode*>(stmt.get())) {
            functions += translateNode(stmt.get()) + "\n";
        } else {
            mainBody += "    " + translateNode(stmt.get()) + "\n";
        }
    }

    result += functions;

    // Add main function
    result += "int main() {\n";

    if (!mainBody.isEmpty()) {
        result += mainBody;
    }

    result += "\n    return 0;\n";
    result += "}\n";

    return result;
}

QString Translator::translateNode(const ASTNode* node) {
    if (!node) return "";

    // Assignment
    if (auto p = dynamic_cast<const AssignmentNode*>(node)) {
        QString varName = p->identifier->token.value;
        QString expressionStr = translateNode(p->expression.get());

        if (!declaredVariables.contains(varName)) {
            declaredVariables.insert(varName);
            return QString("auto %1 = %2;").arg(varName, expressionStr);
        } else {
            return QString("%1 = %2;").arg(varName, expressionStr);
        }
    }

    // Binary Operation
    if (auto p = dynamic_cast<const BinaryOpNode*>(node)) {
        QString left = translateNode(p->left.get());
        QString right = translateNode(p->right.get());
        QString op = p->op.value;

        // Handle Python's "or" operator
        if (op == "or") {
            op = "||";
        }
        // Handle Python's "==" operator
        if (op == "==") {
            op = "==";
        }

        return QString("(%1 %2 %3)").arg(left, op, right);
    }

    // Unary Operation
    if (auto p = dynamic_cast<const UnaryOpNode*>(node)) {
        QString right = translateNode(p->right.get());
        QString op = p->op.value;

        if (op == "not") {
            op = "!";
        }

        return QString("(%1%2)").arg(op, right);
    }

    // Identifier
    if (auto p = dynamic_cast<const IdentifierNode*>(node)) {
        return p->token.value;
    }

    // Number
    if (auto p = dynamic_cast<const NumberNode*>(node)) {
        return p->token.value;
    }

    // String
    if (auto p = dynamic_cast<const StringNode*>(node)) {
        return QString("\"%1\"").arg(p->token.value);
    }

    // None
    if (dynamic_cast<const NoneNode*>(node)) {
        return "nullptr";
    }

    // Print Statement
    if (auto p = dynamic_cast<const PrintNode*>(node)) {
        QString expr = translateNode(p->expression.get());
        return QString("cout << %1 << endl;").arg(expr);
    }

    // Return Statement
    if (auto p = dynamic_cast<const ReturnNode*>(node)) {
        if (p->expression) {
            QString expr = translateNode(p->expression.get());
            return QString("return %1;").arg(expr);
        }
        return "return;";
    }

    // Function Call
    if (auto p = dynamic_cast<const FunctionCallNode*>(node)) {
        QString funcName = p->name->token.value;

        // Handle built-in Python functions
        if (funcName == "isinstance") {
            return "true"; // Simplified - type checking
        }
        if (funcName == "int") {
            if (!p->arguments.empty()) {
                return QString("stoi(%1)").arg(translateNode(p->arguments[0].get()));
            }
            return "0";
        }
        if (funcName == "float") {
            if (!p->arguments.empty()) {
                return QString("stof(%1)").arg(translateNode(p->arguments[0].get()));
            }
            return "0.0";
        }
        if (funcName == "input") {
            return "\"user_input\""; // Simplified
        }

        // Regular function call
        QString args;
        for (size_t i = 0; i < p->arguments.size(); ++i) {
            if (i > 0) args += ", ";
            args += translateNode(p->arguments[i].get());
        }
        return QString("%1(%2)").arg(funcName, args);
    }

    // If Statement
    if (auto p = dynamic_cast<const IfNode*>(node)) {
        QString condition = translateNode(p->condition.get());
        QString body = translateBlock(p->body.get(), 1);

        return QString("if %1 {\n%2    }").arg(condition, body);
    }

    // Function Definition
    if (auto p = dynamic_cast<const FunctionDefNode*>(node)) {
        QString funcName = p->name->token.value;

        // Parameters
        QString params;
        for (size_t i = 0; i < p->parameters.size(); ++i) {
            if (i > 0) params += ", ";
            QString paramName = p->parameters[i]->token.value;
            params += QString("auto %1").arg(paramName);
            declaredVariables.insert(paramName);
        }

        // Body
        QString body = translateBlock(p->body.get(), 1);

        return QString("auto %1(%2) {\n%3}\n").arg(funcName, params, body);
    }

    // Try-Except Block
    if (auto p = dynamic_cast<const TryExceptNode*>(node)) {
        QString tryBody = translateBlock(p->try_body.get(), 1);

        return QString("try {\n%1    } catch (...) {\n        // Exception handling\n    }").arg(tryBody);
    }

    // Block
    if (auto p = dynamic_cast<const BlockNode*>(node)) {
        return translateBlock(p, 0);
    }

    return "// Unsupported node type";
}

QString Translator::translateBlock(const BlockNode* block, int indentLevel) {
    QString result;
    QString indent = QString("    ").repeated(indentLevel);

    for (const auto& stmt : block->statements) {
        QString translated = translateNode(stmt.get());

        // Handle multi-line statements (like if, function def)
        if (translated.contains('\n')) {
            QStringList lines = translated.split('\n');
            for (const QString& line : lines) {
                if (!line.isEmpty()) {
                    result += indent + line + "\n";
                }
            }
        } else {
            result += indent + translated + "\n";
        }
    }

    return result;
}
