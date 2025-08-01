#include "codegen.hpp"
#include "ast.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <iostream>

using namespace llvm;

CodeGenContext::CodeGenContext()
    : context(std::make_unique<LLVMContext>()),
      builder(*context) {
    module = std::make_unique<Module>("main", *context);
    pushScope(); // Create global scope
}

Function* CodeGenContext::generateCode(Program* prog) {
    FunctionType* funcType = FunctionType::get(
        Type::getInt32Ty(*context), 
        false
    );
    
    Function* mainFunc = Function::Create(
        funcType, 
        Function::ExternalLinkage, 
        "main", 
        module.get()
    );

    BasicBlock* entry = BasicBlock::Create(*context, "entry", mainFunc);
    builder.SetInsertPoint(entry);
    setCurrentFunction(mainFunc);

    for (const auto& stmt : prog->getStatements()) {
        if (!codegen(stmt.get())) {
            std::cerr << "Error generating code for statement" << std::endl;
            return nullptr;
        }
    }

    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateRet(ConstantInt::get(Type::getInt32Ty(*context), 0));
    }

    if (verifyFunction(*mainFunc, &llvm::errs())) {
        std::cerr << "Error: Function verification failed!" << std::endl;
        return nullptr;
    }

    return mainFunc;
}

// Variable management
AllocaInst* CodeGenContext::findVariable(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) return it->at(name);
    }
    return nullptr;
}

void CodeGenContext::registerVariable(const std::string& name, AllocaInst* alloca) {
    if (!scopes.empty()) {
        scopes.back()[name] = alloca;
    }
}

// Expression codegen
Value* CodeGenContext::codegen(Expression* expr) {
    if (auto intLit = dynamic_cast<IntegerLiteral*>(expr)) {
        return codegen(intLit);
    }
    if (auto varExpr = dynamic_cast<VariableExpr*>(expr)) {
        return codegen(varExpr);
    }
    if (auto binExpr = dynamic_cast<BinaryExpr*>(expr)) {
        return codegen(binExpr);
    }
    if (auto callExpr = dynamic_cast<FunctionCall*>(expr)) {
        return codegen(callExpr);
    }
    return nullptr;
}

// Statement codegen
Value* CodeGenContext::codegen(Statement* stmt) {
    if (auto retStmt = dynamic_cast<ReturnStatement*>(stmt)) {
        return codegen(retStmt);
    }
    if (auto varDecl = dynamic_cast<VarDeclaration*>(stmt)) {
        return codegen(varDecl);
    }
    if (auto assign = dynamic_cast<Assignment*>(stmt)) {
        return codegen(assign);
    }
    if (auto ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        return codegen(ifStmt);
    }
    if (auto whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        return codegen(whileStmt);
    }
    return nullptr;
}

// Specific implementations
Value* CodeGenContext::codegen(IntegerLiteral* expr) {
    return ConstantInt::get(
        Type::getInt32Ty(*context),
        expr->value,
        true
    );
}

Value* CodeGenContext::codegen(VariableExpr* expr) {
    AllocaInst* alloca = findVariable(expr->name);
    if (!alloca) {
        std::cerr << "Unknown variable: " << expr->name << std::endl;
        return nullptr;
    }
    return builder.CreateLoad(
        alloca->getAllocatedType(),
        alloca,
        expr->name.c_str()
    );
}

Value* CodeGenContext::codegen(BinaryExpr* expr) {
    Value* L = expr->lhs->codegen(*this);
    Value* R = expr->rhs->codegen(*this);
    if (!L || !R) return nullptr;

    switch (expr->op) {
        case '+': return builder.CreateAdd(L, R, "addtmp");
        case '-': return builder.CreateSub(L, R, "subtmp");
        case '*': return builder.CreateMul(L, R, "multmp");
        case '/': return builder.CreateSDiv(L, R, "divtmp");
        case '<': return builder.CreateICmpSLT(L, R, "cmptmp");
        case '>': return builder.CreateICmpSGT(L, R, "cmptmp");
        case '=': return builder.CreateICmpEQ(L, R, "eqtmp");
        default:
            std::cerr << "Invalid binary operator: " << expr->op << std::endl;
            return nullptr;
    }
}

Value* CodeGenContext::codegen(FunctionCall* expr) {
    Function* callee = module->getFunction(expr->name);
    if (!callee) {
        std::cerr << "Unknown function: " << expr->name << std::endl;
        return nullptr;
    }

    std::vector<Value*> args;
    for (auto& arg : *expr->args) {
        args.push_back(arg->codegen(*this));
        if (!args.back()) return nullptr;
    }

    return builder.CreateCall(callee, args, "calltmp");
}

Value* CodeGenContext::codegen(VarDeclaration* stmt) {
    Type* type = Type::getInt32Ty(*context);
    AllocaInst* alloca = builder.CreateAlloca(
        type, 
        nullptr, 
        stmt->name
    );
    
    registerVariable(stmt->name, alloca);
    
    if (stmt->init) {
        Value* initVal = stmt->init->codegen(*this);
        if (!initVal) return nullptr;
        builder.CreateStore(initVal, alloca);
    }
    
    return alloca;
}

Value* CodeGenContext::codegen(Assignment* stmt) {
    Value* alloca = findVariable(stmt->name);
    if (!alloca) {
        std::cerr << "Unknown variable: " << stmt->name << std::endl;
        return nullptr;
    }

    Value* val = stmt->expr->codegen(*this);
    if (!val) return nullptr;

    builder.CreateStore(val, alloca);
    return val;
}

Value* CodeGenContext::codegen(ReturnStatement* stmt) {
    if (stmt->expr) {
        Value* retVal = stmt->expr->codegen(*this);
        return builder.CreateRet(retVal);
    }
    return builder.CreateRetVoid();
}

Value* CodeGenContext::codegen(IfStatement* stmt) {
    Value* condVal = stmt->condition->codegen(*this);
    if (!condVal) return nullptr;
    
    condVal = builder.CreateICmpNE(
        condVal, 
        ConstantInt::get(*context, APInt(1, 0)), 
        "ifcond");

    Function* func = builder.GetInsertBlock()->getParent();

    BasicBlock* thenBB = BasicBlock::Create(*context, "then", func);
    BasicBlock* elseBB = BasicBlock::Create(*context, "else");
    BasicBlock* mergeBB = BasicBlock::Create(*context, "ifcont");

    builder.CreateCondBr(condVal, thenBB, elseBB);

    builder.SetInsertPoint(thenBB);
    codegen(stmt->thenBlock.get());
    builder.CreateBr(mergeBB);

    func->insert(func->end(), elseBB);
    builder.SetInsertPoint(elseBB);
    if (stmt->elseBlock) {
        codegen(stmt->elseBlock.get());
    }
    builder.CreateBr(mergeBB);

    func->insert(func->end(), mergeBB);
    builder.SetInsertPoint(mergeBB);

    return nullptr;
}

Value* CodeGenContext::codegen(WhileStatement* stmt) {
    Function* func = builder.GetInsertBlock()->getParent();

    BasicBlock* loopCondBB = BasicBlock::Create(*context, "loopcond", func);
    BasicBlock* loopBodyBB = BasicBlock::Create(*context, "loopbody");
    BasicBlock* afterLoopBB = BasicBlock::Create(*context, "afterloop");

    builder.CreateBr(loopCondBB);

    builder.SetInsertPoint(loopCondBB);
    Value* condVal = stmt->condition->codegen(*this);
    if (!condVal) return nullptr;

    condVal = builder.CreateICmpNE(
        condVal,
        ConstantInt::get(*context, APInt(1, 0)),
        "loopcond");

    builder.CreateCondBr(condVal, loopBodyBB, afterLoopBB);

    func->insert(func->end(), loopBodyBB);
    builder.SetInsertPoint(loopBodyBB);
    codegen(stmt->body.get());
    builder.CreateBr(loopCondBB);

    func->insert(func->end(), afterLoopBB);
    builder.SetInsertPoint(afterLoopBB);

    return nullptr;
}