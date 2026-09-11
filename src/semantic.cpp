#include "semantic.hpp"

SymbolTable::SymbolTable() {
    enterScope(); // Global scope
}

void SymbolTable::enterScope() {
    scopes.emplace_back();
}

void SymbolTable::exitScope() {
    if (scopes.size() <= 1) {
        throw std::runtime_error("Cannot exit global scope");
    }
    scopes.pop_back();
}

bool SymbolTable::declare(const std::string& name, SymbolType type) {
    if (scopes.back().count(name)) {
        return false; // Redeclaration in same scope
    }
    scopes.back()[name] = {type};
    return true;
}

bool SymbolTable::isDeclared(const std::string& name) const {
    // Search from innermost to outermost scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) {
            return true;
        }
    }
    return false;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (auto found = it->find(name); found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

size_t SymbolTable::getScopeDepth() const {
    return scopes.size();
}