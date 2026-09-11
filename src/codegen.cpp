#include "codegen.hpp"
#include "ast.hpp"
#include "semantic.hpp"
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
        codegen(stmt.get());
        if (builder.GetInsertBlock()->getTerminator()) {
            break;
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

// Scope management
void CodeGenContext::pushScope() {
    scopes.emplace_back();
    symbolTable.enterScope();
}

void CodeGenContext::popScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
    if (symbolTable.getScopeDepth() > 1) {
        symbolTable.exitScope();
    }
}

// Loop management
void CodeGenContext::pushLoop(BasicBlock* breakBB, BasicBlock* continueBB) {
    loopStack.push_back({breakBB, continueBB});
}

void CodeGenContext::popLoop() {
    if (!loopStack.empty()) {
        loopStack.pop_back();
    }
}

BasicBlock* CodeGenContext::getCurrentLoopEnd() const {
    return loopStack.empty() ? nullptr : loopStack.back().breakBB;
}

BasicBlock* CodeGenContext::getCurrentLoopContinue() const {
    return loopStack.empty() ? nullptr : loopStack.back().continueBB;
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

// Expression codegen dispatch
Value* CodeGenContext::codegen(Expression* expr) {
    if (!expr) return nullptr;
    if (auto intLit = dynamic_cast<IntegerLiteral*>(expr)) {
        return codegen(intLit);
    }
    if (auto varExpr = dynamic_cast<VariableExpr*>(expr)) {
        return codegen(varExpr);
    }
    if (auto binExpr = dynamic_cast<BinaryExpr*>(expr)) {
        return codegen(binExpr);
    }
    if (auto cmpExpr = dynamic_cast<ComparisonExpr*>(expr)) {
        return codegen(cmpExpr);
    }
    if (auto callExpr = dynamic_cast<FunctionCall*>(expr)) {
        return codegen(callExpr);
    }
    return nullptr;
}

// Statement codegen dispatch
Value* CodeGenContext::codegen(Statement* stmt) {
    if (!stmt) return nullptr;
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
    if (auto forStmt = dynamic_cast<ForStatement*>(stmt)) {
        return codegen(forStmt);
    }
    if (auto block = dynamic_cast<Block*>(stmt)) {
        return codegen(block);
    }
    if (auto exprStmt = dynamic_cast<ExprStatement*>(stmt)) {
        return codegen(exprStmt);
    }
    if (auto breakStmt = dynamic_cast<BreakStatement*>(stmt)) {
        return codegen(breakStmt);
    }
    if (auto contStmt = dynamic_cast<ContinueStatement*>(stmt)) {
        return codegen(contStmt);
    }
    return nullptr;
}

// Specific expression implementations
Value* CodeGenContext::codegen(IntegerLiteral* expr) {
    return ConstantInt::get(
        Type::getInt32Ty(*context),
        expr->value,
        true
    );
}

Value* CodeGenContext::codegen(VariableExpr* expr) {
    if (!symbolTable.isDeclared(expr->name)) {
        std::cerr << "Error: Undeclared variable '" << expr->name << "'" << std::endl;
        return nullptr;
    }
    
    AllocaInst* alloca = findVariable(expr->name);
    if (!alloca) {
        std::cerr << "Internal error: Variable '" << expr->name << "' declared but not allocated" << std::endl;
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
        default:
            std::cerr << "Invalid binary operator: " << expr->op << std::endl;
            return nullptr;
    }
}

Value* CodeGenContext::codegen(ComparisonExpr* expr) {
    Value* L = expr->lhs->codegen(*this);
    Value* R = expr->rhs->codegen(*this);
    if (!L || !R) return nullptr;

    if (expr->op == "<") return builder.CreateICmpSLT(L, R, "cmptmp");
    if (expr->op == "<=") return builder.CreateICmpSLE(L, R, "cmptmp");
    if (expr->op == ">") return builder.CreateICmpSGT(L, R, "cmptmp");
    if (expr->op == ">=") return builder.CreateICmpSGE(L, R, "cmptmp");
    if (expr->op == "==") return builder.CreateICmpEQ(L, R, "cmptmp");
    if (expr->op == "!=") return builder.CreateICmpNE(L, R, "cmptmp");

    std::cerr << "Invalid comparison operator: " << expr->op << std::endl;
    return nullptr;
}

Value* CodeGenContext::codegen(FunctionCall* expr) {
    Function* callee = module->getFunction(expr->name);
    if (!callee) {
        std::cerr << "Unknown function: " << expr->name << std::endl;
        return nullptr;
    }

    std::vector<Value*> args;
    if (expr->args) {
        for (auto& arg : *expr->args) {
            args.push_back(arg->codegen(*this));
            if (!args.back()) return nullptr;
        }
    }

    return builder.CreateCall(callee, args, "calltmp");
}

// Specific statement implementations
Value* CodeGenContext::codegen(VarDeclaration* stmt) {
    if (!symbolTable.declare(stmt->name, SymbolType::VARIABLE)) {
        std::cerr << "Error: Variable '" << stmt->name << "' already declared in this scope" << std::endl;
        return nullptr;
    }
    
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
    AllocaInst* alloca = findVariable(stmt->name);
    if (!alloca) {
        std::cerr << "Error: Undeclared variable '" << stmt->name << "'" << std::endl;
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
        if (!retVal) return nullptr;
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

    builder.CreateCondBr(condVal, thenBB, stmt->elseBlock ? elseBB : mergeBB);

    // Then branch
    builder.SetInsertPoint(thenBB);
    codegen(stmt->thenBlock.get());
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(mergeBB);
    }

    // Else branch
    if (stmt->elseBlock) {
        func->insert(func->end(), elseBB);
        builder.SetInsertPoint(elseBB);
        codegen(stmt->elseBlock.get());
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(mergeBB);
        }
    }

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

    pushLoop(afterLoopBB, loopCondBB);
    codegen(stmt->body.get());
    popLoop();

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(loopCondBB);
    }

    func->insert(func->end(), afterLoopBB);
    builder.SetInsertPoint(afterLoopBB);

    return nullptr;
}

Value* CodeGenContext::codegen(ForStatement* stmt) {
    pushScope(); // Loop variable scope

    if (stmt->init) {
        codegen(stmt->init.get());
    }

    Function* func = builder.GetInsertBlock()->getParent();

    BasicBlock* condBB = BasicBlock::Create(*context, "for.cond", func);
    BasicBlock* bodyBB = BasicBlock::Create(*context, "for.body");
    BasicBlock* incBB = BasicBlock::Create(*context, "for.inc");
    BasicBlock* afterBB = BasicBlock::Create(*context, "for.after");

    builder.CreateBr(condBB);

    // Condition
    builder.SetInsertPoint(condBB);
    Value* condVal = nullptr;
    if (stmt->condition) {
        condVal = stmt->condition->codegen(*this);
        if (!condVal) {
            popScope();
            return nullptr;
        }
        condVal = builder.CreateICmpNE(
            condVal,
            ConstantInt::get(*context, APInt(1, 0)),
            "for.condval");
    } else {
        condVal = ConstantInt::get(Type::getInt1Ty(*context), 1);
    }
    builder.CreateCondBr(condVal, bodyBB, afterBB);

    // Body
    func->insert(func->end(), bodyBB);
    builder.SetInsertPoint(bodyBB);

    pushLoop(afterBB, incBB);
    if (stmt->body) {
        codegen(stmt->body.get());
    }
    popLoop();

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(incBB);
    }

    // Increment (continue target)
    func->insert(func->end(), incBB);
    builder.SetInsertPoint(incBB);
    if (stmt->increment) {
        codegen(stmt->increment.get());
    }
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(condBB);
    }

    // After loop (break target)
    func->insert(func->end(), afterBB);
    builder.SetInsertPoint(afterBB);

    popScope();
    return nullptr;
}

Value* CodeGenContext::codegen(Block* stmt) {
    pushScope();
    Value* last = nullptr;
    for (const auto& s : stmt->statements) {
        last = codegen(s.get());
        if (builder.GetInsertBlock()->getTerminator()) {
            break;
        }
    }
    popScope();
    return last ? last : ConstantInt::get(Type::getInt32Ty(*context), 0);
}

Value* CodeGenContext::codegen(ExprStatement* stmt) {
    return stmt->expr ? stmt->expr->codegen(*this) : nullptr;
}

Value* CodeGenContext::codegen(BreakStatement* stmt) {
    BasicBlock* breakBB = getCurrentLoopEnd();
    if (!breakBB) {
        std::cerr << "Error: 'break' statement not inside loop\n";
        return nullptr;
    }
    return builder.CreateBr(breakBB);
}

Value* CodeGenContext::codegen(ContinueStatement* stmt) {
    BasicBlock* contBB = getCurrentLoopContinue();
    if (!contBB) {
        std::cerr << "Error: 'continue' statement not inside loop\n";
        return nullptr;
    }
    return builder.CreateBr(contBB);
}