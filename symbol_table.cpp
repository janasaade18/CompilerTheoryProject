#include "symbol_table.h"

using namespace std;

SymbolTable::SymbolTable() {
    // Start with the global scope
    enterScope();
}

void SymbolTable::enterScope() {
    m_scopes.push_back({});
}

void SymbolTable::leaveScope() {
    if (!m_scopes.empty()) {
        m_scopes.pop_back();
    }
}

bool SymbolTable::define(const QString& name, DataType type) {
    if (m_scopes.empty()) {
        return false; // Should never happen
    }

    // Check if symbol already exists in the current scope
    if (m_scopes.back().count(name)) {
        return false; // Re-declaration error
    }

    m_scopes.back()[name] = {name, type};
    return true;
}

Symbol* SymbolTable::lookup(const QString& name) {
    if (m_scopes.empty()) {
        return nullptr;
    }

    // Search from the innermost scope to the outermost
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto& scope = *it;
        if (scope.count(name)) {
            return &scope.at(name);
        }
    }

    return nullptr; // Symbol not found
}
