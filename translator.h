#ifndef TRANSLATOR_H
#define TRANSLATOR_H
#include "ast.h"
#include <QString>
#include <QSet>

class Translator {
public:
    QString translate(const ProgramNode* program);
private:
    QSet<QString> declaredVariables;
    QString translateNode(const ASTNode* node);
    QString translateBlock(const BlockNode* block, int indentLevel);  // ADD THIS LINE
};
#endif // TRANSLATOR_H
