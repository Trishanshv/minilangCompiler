#pragma once
#include "ast.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

class CodeGenContext {
public:
    llvm::LLVMContext llvmContext;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    CodeGenContext() : builder(llvmContext) {
        module = std::make_unique<llvm::Module>("main", llvmContext);
    }

    llvm::Value* codegen(Expression* expr);
    llvm::Value* codegen(Statement* stmt);
    llvm::Function* codegen(Program* prog);
};