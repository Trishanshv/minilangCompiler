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
    return context.codegen(this);
}

// StringLiteral
StringLiteral::StringLiteral(std::string v) : value(std::move(v)) {}
void StringLiteral::print() const { std::cout << "\"" << value << "\""; }
llvm::Value* StringLiteral::codegen(CodeGenContext& context) {
    return context.codegen(this);
}

// VariableExpr
VariableExpr::VariableExpr(std::string n) : name(std::move(n)) {}
void VariableExpr::print() const { std::cout << name; }
llvm::Value* VariableExpr::codegen(CodeGenContext& context) {
    return context.codegen(this);
}

// UnaryExpr
UnaryExpr::UnaryExpr(char o, Expression* e) : op(o), operand(e) {}
void UnaryExpr::print() const {
    std::cout << "(" << op;
    if (operand) operand->print();
    std::cout << ")";
}
llvm::Value* UnaryExpr::codegen(CodeGenContext& context) {
    return context.codegen(this);
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
    return context.codegen(this);
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
    return context.codegen(this);
}

// LogicalExpr
LogicalExpr::LogicalExpr(std::string o, Expression* l, Expression* r)
    : op(std::move(o)), lhs(l), rhs(r) {}
void LogicalExpr::print() const {
    lhs->print();
    std::cout << " " << op << " ";
    rhs->print();
}
llvm::Value* LogicalExpr::codegen(CodeGenContext& context) {
    return context.codegen(this);
}

// FunctionCall
FunctionCall::FunctionCall(const std::string& n, std::vector<Expression*>* a)
    : name(n), args(a ? a : new std::vector<Expression*>()) {}
FunctionCall::~FunctionCall() {
    if (args) {
        for (auto* a : *args) {
            delete a;
        }
        delete args;
    }
}
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
    return context.codegen(this);
}

// ==================== Statement Nodes ====================

// ReturnStatement
ReturnStatement::ReturnStatement(Expression* e) : expr(e) {}
void ReturnStatement::print() const {
    std::cout << "return";
    if (expr) {
        std::cout << " ";
        expr->print();
    }
    std::cout << ";\n";
}
llvm::Value* ReturnStatement::codegen(CodeGenContext& context) {
    return context.codegen(this);
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
    return context.codegen(this);
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
    return context.codegen(this);
}

// Block
Block::Block(std::vector<Statement*>* stmts) {
    if (stmts) {
        for (auto* stmt : *stmts) {
            statements.emplace_back(stmt);
        }
        delete stmts;
    }
}
void Block::print() const {
    std::cout << "{\n";
    for (const auto& stmt : statements) {
        if (stmt) stmt->print();
    }
    std::cout << "}\n";
}
llvm::Value* Block::codegen(CodeGenContext& context) {
    return context.codegen(this);
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
    return context.codegen(this);
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
    return context.codegen(this);
}

// ForStatement
ForStatement::ForStatement(Statement* i, Expression* c, Statement* inc, Statement* b)
    : init(i), condition(c), increment(inc), body(b) {}

void ForStatement::print() const {
    std::cout << "for (";
    if (init) init->print();
    else std::cout << "; ";
    if (condition) condition->print();
    std::cout << "; ";
    if (increment) increment->print();
    std::cout << ") ";
    if (body) body->print();
}

llvm::Value* ForStatement::codegen(CodeGenContext& context) {
    return context.codegen(this);
}

// BreakStatement
void BreakStatement::print() const { std::cout << "break;\n"; }
llvm::Value* BreakStatement::codegen(CodeGenContext& context) {
    return context.codegen(this);
}

// ContinueStatement
void ContinueStatement::print() const { std::cout << "continue;\n"; }
llvm::Value* ContinueStatement::codegen(CodeGenContext& context) {
    return context.codegen(this);
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
    return context.codegen(this);
}

// Parameter
Parameter::Parameter(std::string t, std::string n)
    : type(std::move(t)), name(std::move(n)) {}

// FunctionDef
FunctionDef::FunctionDef(std::string retType, std::string n, std::vector<Parameter>* p, Block* b)
    : returnType(std::move(retType)), name(std::move(n)), body(b) {
    if (p) {
        params = std::move(*p);
        delete p;
    }
}
void FunctionDef::print() const {
    std::cout << returnType << " " << name << "(";
    for (size_t i = 0; i < params.size(); ++i) {
        std::cout << params[i].type << " " << params[i].name;
        if (i + 1 < params.size()) std::cout << ", ";
    }
    std::cout << ") ";
    if (body) body->print();
}
llvm::Value* FunctionDef::codegen(CodeGenContext& context) {
    return context.codegen(this);
}

// ==================== Program ====================

Program::Program(std::vector<Statement*>* stmts) {
    if (stmts) {
        for (auto* stmt : *stmts) {
            statements.emplace_back(stmt);
        }
        delete stmts;
    }
}
void Program::print() const {
    for (const auto& stmt : statements) {
        if (stmt) stmt->print();
    }
}