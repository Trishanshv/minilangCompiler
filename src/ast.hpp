#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
struct Expression;
struct Statement;
// ======== Expression Base ========
struct Expression {
    virtual ~Expression() = default;
    virtual void print() const = 0;
};

// ======== Integer Literal ========
struct IntegerLiteral : Expression {
    int value;
    IntegerLiteral(int v) : value(v) {}

    void print() const override {
        std::cout << value;
    }
};

// ======== Binary Expression ========
struct BinaryExpr : Expression {
    char op;
    Expression* lhs;
    Expression* rhs;

    BinaryExpr(char o, Expression* l, Expression* r)
        : op(o), lhs(l), rhs(r) {}

    void print() const override {
        std::cout << "(";
        if (lhs) lhs->print();
        std::cout << " " << op << " ";
        if (rhs) rhs->print();
        std::cout << ")";
    }
};

// ======== Statement Base ========
struct Statement {
    virtual ~Statement() = default;
    virtual void print() const = 0;
};

// ======== Return Statement ========
struct ReturnStatement : Statement {
    Expression* expr;

    ReturnStatement(Expression* e) : expr(e) {}

    void print() const override {
        std::cout << "return ";
        if (expr) expr->print();
        std::cout << ";\n";
    }
};

// ======== Program ========
struct Program {
    std::vector<Statement*>* stmts;

    Program(std::vector<Statement*>* s) : stmts(s) {}

    void print() const {
        for (auto stmt : *stmts) {
            if (stmt) stmt->print();
        }
    }
};
