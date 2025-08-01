#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Forward declarations
namespace llvm {
    class Value;
    class BasicBlock;
}

class CodeGenContext;  // Forward declaration
struct Statement {
    virtual ~Statement() = default;
    virtual void print() const = 0;
    virtual llvm::Value* codegen(CodeGenContext& context) = 0;
};

struct Expression {
    virtual ~Expression() = default;
    virtual void print() const = 0;
    virtual llvm::Value* codegen(CodeGenContext& context) = 0;
};

struct ExprStatement : Statement {
    std::unique_ptr<Expression> expr;
    explicit ExprStatement(Expression* e);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct IntegerLiteral : Expression {
    int value;
    explicit IntegerLiteral(int v);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct VariableExpr : Expression {
    std::string name;
    explicit VariableExpr(std::string n);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct BinaryExpr : Expression {
    char op;
    std::unique_ptr<Expression> lhs, rhs;
    BinaryExpr(char o, Expression* l, Expression* r);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct ComparisonExpr : Expression {
    std::string op;
    std::unique_ptr<Expression> lhs, rhs;
    ComparisonExpr(std::string o, Expression* l, Expression* r);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};


struct ReturnStatement : Statement {
    std::unique_ptr<Expression> expr;
    explicit ReturnStatement(Expression* e);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct VarDeclaration : Statement {
    std::string name;
    std::unique_ptr<Expression> init;
    VarDeclaration(std::string n, Expression* i);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct Assignment : Statement {
    std::string name;
    std::unique_ptr<Expression> expr;
    Assignment(std::string n, Expression* e);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct Block : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
    explicit Block(std::vector<Statement*>* stmts);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct IfStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBlock;
    std::unique_ptr<Statement> elseBlock;
    IfStatement(Expression* cond, Statement* thenBlk, Statement* elseBlk = nullptr);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct WhileStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    WhileStatement(Expression* cond, Statement* b);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct BreakStatement : Statement {
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct ContinueStatement : Statement {
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct FunctionCall : Expression {
    std::string name;
    std::vector<Expression*>* args;
    FunctionCall(const std::string& n, std::vector<Expression*>* a);
    void print() const override;
    llvm::Value* codegen(CodeGenContext& context) override;
};

struct Program {
    std::vector<std::unique_ptr<Statement>> statements;
    explicit Program(std::vector<Statement*>* stmts);
    const auto& getStatements() const { return statements; }
    void print() const;
};