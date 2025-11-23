#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <QString>
#include <map>
#include <vector>
#include "types.h"

using namespace std;

struct Symbol {
    QString name;
    DataType type;
    // Store function return type separately
    DataType functionReturnType = DataType::UNDEFINED;
};

class SymbolTable {
public:
    SymbolTable();

    void enterScope();
    void leaveScope();

    bool define(const QString& name, DataType type);
    Symbol* lookup(const QString& name);

private:
    vector<map<QString, Symbol>> m_scopes;
};

#endif // SYMBOL_TABLE_H
