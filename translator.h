#ifndef TRANSLATOR_H
#define TRANSLATOR_H
#include "ast.h"
#include "symbol_table.h"
#include <QString>
#include <QSet>

class Translator {
public:
    Translator(const SymbolTable& symbolTable);
    QString translate(const ProgramNode* program);
private:
    const SymbolTable& m_symbol_table;
    QSet<QString> declaredVariables;

    QString translateNode(const ASTNode* node);
    QString translateBlock(const BlockNode* block);
};
#endif // TRANSLATOR_H
