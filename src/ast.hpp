#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Forward declarations
struct Expression;
struct Statement;

// Base Expression
struct Expression {
    virtual ~Expression() = default;
    virtual void print() const = 0;
};

// Literals and Variables
struct IntegerLiteral : Expression {
    int value;
    explicit IntegerLiteral(int v) : value(v) {}
    void print() const override { std::cout << value; }
};

struct VariableExpr : Expression {
    std::string name;
    explicit VariableExpr(std::string n) : name(std::move(n)) {}
    void print() const override { std::cout << name; }
};

// Expressions
struct BinaryExpr : Expression {
    char op;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;

    BinaryExpr(char o, Expression* l, Expression* r)
        : op(o), lhs(l), rhs(r) {}

    void print() const override {
        std::cout << "(";
        lhs->print();
        std::cout << " " << op << " ";
        rhs->print();
        std::cout << ")";
    }
};

struct ComparisonExpr : Expression {
    std::string op;
    std::unique_ptr<Expression> lhs, rhs;
    
    ComparisonExpr(std::string o, Expression* l, Expression* r)
        : op(std::move(o)), lhs(l), rhs(r) {}
    
    void print() const override {
        std::cout << "(";
        lhs->print();
        std::cout << " " << op << " ";
        rhs->print();
        std::cout << ")";
    }
};

// Base Statement
struct Statement {
    virtual ~Statement() = default;
    virtual void print() const = 0;
};

// Statements
struct ReturnStatement : Statement {
    std::unique_ptr<Expression> expr;

    explicit ReturnStatement(Expression* e) : expr(e) {}

    void print() const override {
        std::cout << "return ";
        expr->print();
        std::cout << ";\n";
    }
};

struct VarDeclaration : Statement {
    std::string name;
    std::unique_ptr<Expression> init;
    
    VarDeclaration(std::string n, Expression* i) : name(std::move(n)), init(i) {}
    
    void print() const override {
        std::cout << "int " << name;
        if (init) {
            std::cout << " = ";
            init->print();
        }
        std::cout << ";\n";
    }
};

struct Assignment : Statement {
    std::string name;
    std::unique_ptr<Expression> expr;
    
    Assignment(std::string n, Expression* e) : name(std::move(n)), expr(e) {}
    
    void print() const override {
        std::cout << name << " = ";
        expr->print();
        std::cout << ";\n";
    }
};

struct Block : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
    
    explicit Block(std::vector<Statement*>* stmts) {
        for (auto* stmt : *stmts) {
            statements.emplace_back(stmt);
        }
        delete stmts;
    }
    
    void print() const override {
        std::cout << "{\n";
        for (const auto& stmt : statements) {
            if (stmt) stmt->print();
        }
        std::cout << "}\n";
    }
};

struct IFStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> ifBranch, elseBranch;
    
    IFStatement(Expression* cond, Statement* ifb, Statement* elseb = nullptr)
        : condition(cond), ifBranch(ifb), elseBranch(elseb) {}
    
    void print() const override {
        std::cout << "if (";
        condition->print();
        std::cout << ")\n";
        ifBranch->print();
        if (elseBranch) {
            std::cout << "else\n";
            elseBranch->print();
        }
    }
};

struct WhileStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    
    WhileStatement(Expression* cond, Statement* b) : condition(cond), body(b) {}
    
    void print() const override {
        std::cout << "while(";
        condition->print();
        std::cout << ") ";
        body->print();
    }
};

struct FunctionCall : public Expression {
    std::string name;
    std::vector<Expression*>* args;
public:
    FunctionCall(const std::string& n, std::vector<Expression*>* a)
    : name(n), args(a ? a : new std::vector<Expression*>()) {}
    
   void print() const override {
        std::cout << name << "(";
        if (args) { 
            for (size_t i = 0; i < args->size(); ++i) {  
                (*args)[i]->print();
                if (i + 1 < args->size()) std::cout << ", ";
            }
        }
        std::cout << ")";
    }
};

struct ExprStatement : Statement {
    std::unique_ptr<Expression> expr; 
    explicit ExprStatement(Expression* e) : expr(e) {}
    void print() const override {
        std::cout << "[DEBUG] Expression statement: ";
        expr->print();
        std::cout << ";\n";
    }
};

// Program
struct Program {
    std::vector<std::unique_ptr<Statement>> statements;
    
    explicit Program(std::vector<Statement*>* stmts) {
        for (auto* stmt : *stmts) {
            statements.emplace_back(stmt);
        }
        delete stmts;
    }
    
    const auto& getStatements() const { return statements; }
    
    void print() const {
        for (const auto& stmt : statements) {
            if (stmt) stmt->print();
        }
    }
};