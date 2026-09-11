#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

enum class SymbolType {
    VARIABLE,
    FUNCTION
};

struct Symbol {
    SymbolType type;
};

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;

public:
    SymbolTable();

    void enterScope();
    void exitScope();

    bool declare(const std::string& name, SymbolType type);
    bool isDeclared(const std::string& name) const;
    const Symbol* lookup(const std::string& name) const;
    size_t getScopeDepth() const;
};
