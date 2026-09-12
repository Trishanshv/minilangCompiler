#pragma once

#include "ast.hpp"
#include "semantic.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>

// Forward declarations
class Program;
class Expression;
class Statement;
class IfStatement;
class WhileStatement;
class ForStatement;
class ReturnStatement;
class VarDeclaration;
class Assignment;
class Block;
class ExprStatement;
class BreakStatement;
class ContinueStatement;
class FunctionDef;
class FunctionCall;
class BinaryExpr;
class ComparisonExpr;
class LogicalExpr;
class UnaryExpr;
class IntegerLiteral;
class StringLiteral;
class VariableExpr;

struct LoopContext {
    llvm::BasicBlock* breakBB = nullptr;
    llvm::BasicBlock* continueBB = nullptr;
};

class CodeGenContext {
private:
    std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> scopes;
    SymbolTable symbolTable;
    std::vector<LoopContext> loopStack;
    llvm::Function* currentFunction = nullptr;
    llvm::Function* printfFunction = nullptr;

public:
    std::unique_ptr<llvm::LLVMContext> context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    CodeGenContext();

    // Context access
    llvm::LLVMContext& getLLVMContext() { return *context; }
    SymbolTable& getSymbolTable() { return symbolTable; }

    // Variable management
    llvm::AllocaInst* findVariable(const std::string& name);
    void registerVariable(const std::string& name, llvm::AllocaInst* alloca);

    // Scope management
    void pushScope();
    void popScope();

    // Loop management
    void pushLoop(llvm::BasicBlock* breakBB, llvm::BasicBlock* continueBB);
    void popLoop();
    llvm::BasicBlock* getCurrentLoopEnd() const;
    llvm::BasicBlock* getCurrentLoopContinue() const;

    // Runtime helpers
    llvm::Function* getPrintfFunction();

    // Code generation interfaces
    llvm::Value* codegen(Expression* expr);
    llvm::Value* codegen(Statement* stmt);
    llvm::Function* generateCode(Program* prog);

    // Specific expression codegen
    llvm::Value* codegen(IntegerLiteral* expr);
    llvm::Value* codegen(StringLiteral* expr);
    llvm::Value* codegen(VariableExpr* expr);
    llvm::Value* codegen(UnaryExpr* expr);
    llvm::Value* codegen(BinaryExpr* expr);
    llvm::Value* codegen(ComparisonExpr* expr);
    llvm::Value* codegen(LogicalExpr* expr);
    llvm::Value* codegen(FunctionCall* expr);

    // Specific statement codegen
    llvm::Value* codegen(ReturnStatement* stmt);
    llvm::Value* codegen(VarDeclaration* stmt);
    llvm::Value* codegen(Assignment* stmt);
    llvm::Value* codegen(IfStatement* stmt);
    llvm::Value* codegen(WhileStatement* stmt);
    llvm::Value* codegen(ForStatement* stmt);
    llvm::Value* codegen(Block* stmt);
    llvm::Value* codegen(ExprStatement* stmt);
    llvm::Value* codegen(BreakStatement* stmt);
    llvm::Value* codegen(ContinueStatement* stmt);
    llvm::Value* codegen(FunctionDef* stmt);

    // Control flow management
    void setCurrentFunction(llvm::Function* func) { currentFunction = func; }
    llvm::Function* getCurrentFunction() const { return currentFunction; }
};