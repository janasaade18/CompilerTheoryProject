#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "ast.h"
#include <QString>
#include <QSet>

class Translator {
public:
    QString translate(const ProgramNode* program);

private:
    // Symbol table to track declared variables
    QSet<QString> declaredVariables;

    QString translateNode(const ASTNode* node);
};

#endif // TRANSLATOR_H
