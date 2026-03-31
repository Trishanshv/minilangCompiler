#pragma once

#include "ast.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

// Forward declarations
class Program;
class Expression;
class Statement;
class IfStatement;
class WhileStatement;
class ReturnStatement;
class VarDeclaration;
class Assignment;
class FunctionCall;
class BinaryExpr;
class IntegerLiteral;
class VariableExpr;

// Symbol table for semantic analysis
enum class SymbolType { VARIABLE, FUNCTION };

struct Symbol {
    SymbolType type;
};

class SymbolTable {
private:
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

class CodeGenContext {
private:
    std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> scopes;
    SymbolTable symbolTable; // Semantic analysis symbol table
    llvm::Function* currentFunction = nullptr;
    llvm::BasicBlock* currentLoopEnd = nullptr;
    llvm::BasicBlock* currentLoopContinue = nullptr;
    
public:
    std::unique_ptr<llvm::LLVMContext> context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    llvm::BasicBlock* getCurrentLoopEnd() const { return currentLoopEnd; }
    llvm::BasicBlock* getCurrentLoopContinue() const { return currentLoopContinue; }

    CodeGenContext();
    
    // Context access
    llvm::LLVMContext& getLLVMContext() { return *context; }
    
    // Semantic analysis - Symbol table access
    SymbolTable& getSymbolTable() { return symbolTable; }
    
    // Variable management
    llvm::AllocaInst* findVariable(const std::string& name);
    
    void registerVariab
        scopes.emplace_back();
        symbolTable.enterScope(); // Sync symbol table with scope
    }
    void popScope() { 
        if (!scopes.empty()) {
            scopes.pop_back();
            symbolTable.exitScope(); // Sync symbol table with scopemplace_back(); }
    void popScope() { 
        if (!scopes.empty()) {
            scopes.pop_back(); 
        }
    }
    
    // Code generation interfaces
    llvm::Value* codegen(Expression* expr);
    llvm::Value* codegen(Statement* stmt);
    llvm::Function* generateCode(Program* prog);
    
    // Specific implementations
    llvm::Value* codegen(IntegerLiteral* expr);
    llvm::Value* codegen(VariableExpr* expr);
    llvm::Value* codegen(BinaryExpr* expr);
    llvm::Value* codegen(IfStatement* stmt);
    llvm::Value* codegen(WhileStatement* stmt);
    llvm::Value* codegen(ReturnStatement* stmt);
    llvm::Value* codegen(VarDeclaration* stmt);
    llvm::Value* codegen(Assignment* stmt);
    llvm::Value* codegen(FunctionCall* expr);
    
    // Control flow management
    void setCurrentFunction(llvm::Function* func) { currentFunction = func; }
    void setCurrentLoopBlocks(llvm::BasicBlock* end, llvm::BasicBlock* cont) {
        currentLoopEnd = end;
        currentLoopContinue = cont;
    }
};