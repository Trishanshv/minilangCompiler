#pragma once

#include "ast.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <map>

class CodeGenContext {
public:
    llvm::LLVMContext llvmContext;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    std::map<std::string, llvm::Value*> namedValues;

    CodeGenContext();

    llvm::Function* generateCode(Program* prog);
    llvm::Value* codegen(Expression* expr);
    llvm::Value* codegen(Statement* stmt);
};
