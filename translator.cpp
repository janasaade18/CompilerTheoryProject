#include "translator.h"
#include <stdexcept>

QString Translator::translate(const ProgramNode* program) {
    QString result;
    // Add standard C++ headers and main function boilerplate
    result += "#include <iostream>\n\n";
    result += "int main() {\n";

    for (const auto& stmt : program->statements) {
        result += "    " + translateNode(stmt.get()) + "\n";
    }

    result += "\n    return 0;\n";
    result += "}\n";
    return result;
}

QString Translator::translateNode(const ASTNode* node) {
    if (auto p = dynamic_cast<const AssignmentNode*>(node)) {
        QString varName = p->identifier->token.value;
        QString expressionStr = translateNode(p->expression.get());

        // Semantic Check: Is this the first time we see this variable?
        if (!declaredVariables.contains(varName)) {
            declaredVariables.insert(varName);
            // If new, declare it with 'auto'
            return QString("auto %1 = %2;").arg(varName, expressionStr);
        } else {
            // If already declared, just assign to it
            return QString("%1 = %2;").arg(varName, expressionStr);
        }
    }
    if (auto p = dynamic_cast<const BinaryOpNode*>(node)) {
        QString left = translateNode(p->left.get());
        QString right = translateNode(p->right.get());
        return QString("(%1 %2 %3)").arg(left, p->op.value, right);
    }
    if (auto p = dynamic_cast<const IdentifierNode*>(node)) {
        // Semantic Check: Are we using a variable that hasn't been declared?
        if (!declaredVariables.contains(p->token.value)) {
            throw std::runtime_error("Semantic Error: Variable '" + p->token.value.toStdString() + "' used before declaration.");
        }
        return p->token.value;
    }
    if (auto p = dynamic_cast<const NumberNode*>(node)) {
        return p->token.value;
    }

    return ""; // Should not happen
}
