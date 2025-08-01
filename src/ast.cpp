#include "ast.hpp"
#include "codegen.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Instructions.h>
#include <iostream>

// ==================== Expression Nodes ====================

// IntegerLiteral
IntegerLiteral::IntegerLiteral(int v) : value(v) {}
void IntegerLiteral::print() const { std::cout << value; }
llvm::Value* IntegerLiteral::codegen(CodeGenContext& context) {
    return llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context.getLLVMContext()),
        value,
        true
    );
}

// VariableExpr
VariableExpr::VariableExpr(std::string n) : name(std::move(n)) {}
void VariableExpr::print() const { std::cout << name; }
llvm::Value* VariableExpr::codegen(CodeGenContext& context) {
    llvm::AllocaInst* alloca = context.findVariable(name);
    if (!alloca) {
        std::cerr << "Unknown variable: " << name << std::endl;
        return nullptr;
    }
    return context.builder.CreateLoad(
        alloca->getAllocatedType(),
        alloca,
        name.c_str()
    );
}

// BinaryExpr
BinaryExpr::BinaryExpr(char o, Expression* l, Expression* r) 
    : op(o), lhs(l), rhs(r) {}
void BinaryExpr::print() const {
    std::cout << "(";
    lhs->print();
    std::cout << " " << op << " ";
    rhs->print();
    std::cout << ")";
}
llvm::Value* BinaryExpr::codegen(CodeGenContext& context) {
    llvm::Value* L = lhs->codegen(context);
    llvm::Value* R = rhs->codegen(context);
    if (!L || !R) return nullptr;

    switch (op) {
        case '+': return context.builder.CreateAdd(L, R, "addtmp");
        case '-': return context.builder.CreateSub(L, R, "subtmp");
        case '*': return context.builder.CreateMul(L, R, "multmp");
        case '/': return context.builder.CreateSDiv(L, R, "divtmp");
        case '<': return context.builder.CreateICmpSLT(L, R, "cmptmp");
        case '>': return context.builder.CreateICmpSGT(L, R, "cmptmp");
        case '=': return context.builder.CreateICmpEQ(L, R, "eqtmp");
        default:
            std::cerr << "Invalid binary operator: " << op << std::endl;
            return nullptr;
    }
}

// ComparisonExpr
ComparisonExpr::ComparisonExpr(std::string o, Expression* l, Expression* r)
    : op(std::move(o)), lhs(l), rhs(r) {}
void ComparisonExpr::print() const {
    lhs->print();
    std::cout << " " << op << " ";
    rhs->print();
}
llvm::Value* ComparisonExpr::codegen(CodeGenContext& context) {
    llvm::Value* L = lhs->codegen(context);
    llvm::Value* R = rhs->codegen(context);
    if (!L || !R) return nullptr;

    if (op == "<") return context.builder.CreateICmpSLT(L, R, "cmptmp");
    if (op == "<=") return context.builder.CreateICmpSLE(L, R, "cmptmp");
    if (op == ">") return context.builder.CreateICmpSGT(L, R, "cmptmp");
    if (op == ">=") return context.builder.CreateICmpSGE(L, R, "cmptmp");
    if (op == "==") return context.builder.CreateICmpEQ(L, R, "cmptmp");
    if (op == "!=") return context.builder.CreateICmpNE(L, R, "cmptmp");

    std::cerr << "Invalid comparison operator: " << op << std::endl;
    return nullptr;
}

// FunctionCall
FunctionCall::FunctionCall(const std::string& n, std::vector<Expression*>* a)
    : name(n), args(a ? a : new std::vector<Expression*>()) {}
void FunctionCall::print() const {
    std::cout << name << "(";
    if (args) {
        for (size_t i = 0; i < args->size(); ++i) {
            (*args)[i]->print();
            if (i + 1 < args->size()) std::cout << ", ";
        }
    }
    std::cout << ")";
}
llvm::Value* FunctionCall::codegen(CodeGenContext& context) {
    llvm::Function* callee = context.module->getFunction(name);
    if (!callee) {
        std::cerr << "Unknown function: " << name << std::endl;
        return nullptr;
    }

    std::vector<llvm::Value*> argsV;
    if (args) {
        for (auto& arg : *args) {
            argsV.push_back(arg->codegen(context));
            if (!argsV.back()) return nullptr;
        }
    }

    return context.builder.CreateCall(callee, argsV, "calltmp");
}

// ==================== Statement Nodes ====================

// ReturnStatement
ReturnStatement::ReturnStatement(Expression* e) : expr(e) {}
void ReturnStatement::print() const {
    std::cout << "return ";
    expr->print();
    std::cout << ";\n";
}
llvm::Value* ReturnStatement::codegen(CodeGenContext& context) {
    llvm::Value* retVal = expr->codegen(context);
    return context.builder.CreateRet(retVal);
}

// VarDeclaration
VarDeclaration::VarDeclaration(std::string n, Expression* i)
    : name(std::move(n)), init(i) {}
void VarDeclaration::print() const {
    std::cout << "int " << name;
    if (init) {
        std::cout << " = ";
        init->print();
    }
    std::cout << ";\n";
}
llvm::Value* VarDeclaration::codegen(CodeGenContext& context) {
    llvm::Type* type = llvm::Type::getInt32Ty(context.getLLVMContext());
    llvm::AllocaInst* alloca = context.builder.CreateAlloca(
        type, 
        nullptr, 
        name
    );
    context.registerVariable(name, alloca);
    
    if (init) {
        llvm::Value* initVal = init->codegen(context);
        if (!initVal) return nullptr;
        context.builder.CreateStore(initVal, alloca);
    }
    
    return alloca;
}

// Assignment
Assignment::Assignment(std::string n, Expression* e)
    : name(std::move(n)), expr(e) {}
void Assignment::print() const {
    std::cout << name << " = ";
    expr->print();
    std::cout << ";\n";
}
llvm::Value* Assignment::codegen(CodeGenContext& context) {
    llvm::Value* alloca = context.findVariable(name);
    if (!alloca) {
        std::cerr << "Unknown variable: " << name << std::endl;
        return nullptr;
    }

    llvm::Value* val = expr->codegen(context);
    if (!val) return nullptr;

    context.builder.CreateStore(val, alloca);
    return val;
}

// Block
Block::Block(std::vector<Statement*>* stmts) {
    for (auto* stmt : *stmts) {
        statements.emplace_back(stmt);
    }
    delete stmts;
}
void Block::print() const {
    std::cout << "{\n";
    for (const auto& stmt : statements) {
        if (stmt) stmt->print();
    }
    std::cout << "}\n";
}
llvm::Value* Block::codegen(CodeGenContext& context) {
    context.pushScope();
    llvm::Value* last = nullptr;
    for (const auto& stmt : statements) {
        last = stmt->codegen(context);
        if (!last) {
            context.popScope();
            return nullptr;
        }
    }
    context.popScope();
    return last;
}

// IfStatement
IfStatement::IfStatement(Expression* cond, Statement* thenBlk, Statement* elseBlk)
    : condition(cond), thenBlock(thenBlk), elseBlock(elseBlk) {}
void IfStatement::print() const {
    std::cout << "if (";
    condition->print();
    std::cout << ") ";
    thenBlock->print();
    if (elseBlock) {
        std::cout << "else ";
        elseBlock->print();
    }
}
llvm::Value* IfStatement::codegen(CodeGenContext& context) {
    return context.codegen(this); // Implementation in CodeGenContext
}

// WhileStatement
WhileStatement::WhileStatement(Expression* cond, Statement* b)
    : condition(cond), body(b) {}
void WhileStatement::print() const {
    std::cout << "while (";
    condition->print();
    std::cout << ") ";
    body->print();
}
llvm::Value* WhileStatement::codegen(CodeGenContext& context) {
    return context.codegen(this); // Implementation in CodeGenContext
}

// BreakStatement
llvm::Value* BreakStatement::codegen(CodeGenContext& context) {
    if (!context.getCurrentLoopEnd()) {
        std::cerr << "break statement not inside loop\n";
        return nullptr;
    }
    return context.builder.CreateBr(context.getCurrentLoopEnd());
}

// ContinueStatement
void ContinueStatement::print() const { std::cout << "continue;\n"; }
llvm::Value* ContinueStatement::codegen(CodeGenContext& context) {
    if (!context.getCurrentLoopContinue()) {
        std::cerr << "continue statement not inside loop\n";
        return nullptr;
    }
    return context.builder.CreateBr(context.getCurrentLoopContinue());
}

// ExprStatement
ExprStatement::ExprStatement(Expression* e) : expr(e) {}
void ExprStatement::print() const {
    if (expr) {
        expr->print();
    }
    std::cout << ";\n";
}
llvm::Value* ExprStatement::codegen(CodeGenContext& context) {
    return expr ? expr->codegen(context) : nullptr;
}

// ==================== Program ====================

Program::Program(std::vector<Statement*>* stmts) {
    for (auto* stmt : *stmts) {
        statements.emplace_back(stmt);
    }
    delete stmts;
}
void Program::print() const {
    for (const auto& stmt : statements) {
        if (stmt) stmt->print();
    }
}