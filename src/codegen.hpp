#pragma once

#include "ast.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
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

class CodeGenContext {
private:
    std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> scopes;
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
    
    // Variable management
    llvm::AllocaInst* findVariable(const std::string& name);
    
    void registerVariable(const std::string& name, llvm::AllocaInst* alloca);
    
    // Scope management
    void pushScope() { scopes.emplace_back(); }
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