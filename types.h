#ifndef TYPES_H
#define TYPES_H

#include <QString>

enum class DataType {
    UNDEFINED,
    INTEGER,
    STRING,
    BOOLEAN,
    NONE,
    FUNCTION,
    FLOAT
};

inline QString DataTypeToString(DataType type) {
    switch (type) {
    case DataType::INTEGER: return "int";
    case DataType::STRING:  return "string";
    case DataType::FLOAT:   return "double";
    case DataType::BOOLEAN: return "bool";
    case DataType::NONE:    return "nullptr_t";
    default:                return "auto";
    }
}

#endif // TYPES_H
