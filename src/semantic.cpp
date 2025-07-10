#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include "ast.hpp"

enum class SymbolType { VARIABLE, FUNCTION };

struct Symbol {
    SymbolType type;
    // Could add more info like data type, scope level, etc.
};

class SymbolTable {
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    
public:
    SymbolTable() {
        enterScope(); // Global scope
    }

    void enterScope() {
        scopes.emplace_back();
    }

    void exitScope() {
        if (scopes.size() <= 1) {
            throw std::runtime_error("Cannot exit global scope");
        }
        scopes.pop_back();
    }

    bool declare(const std::string& name, SymbolType type) {
        if (scopes.back().count(name)) {
            return false; // Redeclaration in same scope
        }
        scopes.back()[name] = {type};
        return true;
    }

    bool isDeclared(const std::string& name) const {
        // Search from innermost to outermost scope
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(name)) return true;
        }
        return false;
    }

    const Symbol* lookup(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (auto found = it->find(name); found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }
};