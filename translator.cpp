#include "translator.h"
#include <stdexcept>

using namespace std;

QString Translator::translate(const ProgramNode* program) {
    QString result;

    result += "#include <iostream>\n";
    result += "#include <string>\n";
    result += "#include <map>\n";
    result += "#include <any>\n";
    result += "#include <stdexcept>\n\n";
    result += "using namespace std;\n\n";

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

    if (auto p = dynamic_cast<const BinaryOpNode*>(node)) {
        QString left = translateNode(p->left.get());
        QString right = translateNode(p->right.get());
        QString op = p->op.value;

        if (op == "or") op = "||";
        if (op == "==") op = "==";

        return QString("(%1 %2 %3)").arg(left, op, right);
    }

    if (auto p = dynamic_cast<const UnaryOpNode*>(node)) {
        QString right = translateNode(p->right.get());
        QString op = p->op.value;

        if (op == "not") op = "!";

        return QString("(%1%2)").arg(op, right);
    }

    if (auto p = dynamic_cast<const IdentifierNode*>(node)) return p->token.value;
    if (auto p = dynamic_cast<const NumberNode*>(node)) return p->token.value;
    if (auto p = dynamic_cast<const StringNode*>(node)) return QString("\"%1\"").arg(p->token.value);
    if (dynamic_cast<const NoneNode*>(node)) return "nullptr";

    if (auto p = dynamic_cast<const PrintNode*>(node)) {
        QString expr = translateNode(p->expression.get());
        return QString("cout << %1 << endl;").arg(expr);
    }

    if (auto p = dynamic_cast<const ReturnNode*>(node)) {
        if (p->expression) {
            QString expr = translateNode(p->expression.get());
            return QString("return %1;").arg(expr);
        }
        return "return;";
    }

    if (auto p = dynamic_cast<const FunctionCallNode*>(node)) {
        QString funcName = p->name->token.value;

        if (funcName == "isinstance") return "true";
        if (funcName == "int") return p->arguments.empty() ? "0" : QString("stoi(%1)").arg(translateNode(p->arguments[0].get()));
        if (funcName == "float") return p->arguments.empty() ? "0.0" : QString("stof(%1)").arg(translateNode(p->arguments[0].get()));
        if (funcName == "input") return "\"user_input\"";

        QString args;
        for (size_t i = 0; i < p->arguments.size(); ++i) {
            if (i > 0) args += ", ";
            args += translateNode(p->arguments[i].get());
        }
        return QString("%1(%2)").arg(funcName, args);
    }

    if (auto p = dynamic_cast<const IfNode*>(node)) {
        QString condition = translateNode(p->condition.get());
        QString body = translateBlock(p->body.get(), 1);
        QString result = QString("if %1 {\n%2    }").arg(condition, body);

        if (p->else_branch) {
            QString else_code;
            if (auto else_if_node = dynamic_cast<const IfNode*>(p->else_branch.get())) {
                else_code = translateNode(else_if_node);
                result += " else " + else_code;
            } else if (auto else_block_node = dynamic_cast<const BlockNode*>(p->else_branch.get())) {
                else_code = translateBlock(else_block_node, 1);
                result += QString(" else {\n%1    }").arg(else_code);
            }
        }
        return result;
    }

    if (auto p = dynamic_cast<const WhileNode*>(node)) {
        QString condition = translateNode(p->condition.get());
        QString body = translateBlock(p->body.get(), 1);
        return QString("while %1 {\n%2    }").arg(condition, body);
    }

    if (auto p = dynamic_cast<const FunctionDefNode*>(node)) {
        QString funcName = p->name->token.value;

        QString params;
        for (size_t i = 0; i < p->parameters.size(); ++i) {
            if (i > 0) params += ", ";
            QString paramName = p->parameters[i]->token.value;
            params += QString("auto %1").arg(paramName);
            declaredVariables.insert(paramName);
        }

        QString body = translateBlock(p->body.get(), 1);
        return QString("auto %1(%2) {\n%3}\n").arg(funcName, params, body);
    }

    if (auto p = dynamic_cast<const TryExceptNode*>(node)) {
        QString tryBody = translateBlock(p->try_body.get(), 1);
        return QString("try {\n%1    } catch (...) {\n        // Exception handling\n    }").arg(tryBody);
    }

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
