%code requires {
    #include "ast.hpp"  // Ensures Bison includes this in generated parser.hpp
}
%{
#include <vector>
#include <memory>
#include "ast.hpp"
#include "parser.hpp"

extern int yylex();
void yyerror(const char *s) {
    fprintf(stderr, "Parser error: %s\n", s);
}

std::unique_ptr<Program> root;
%}

%union {
    int ival;
    char* id;
    Expression* expr;
    Statement* stmt;
    std::vector<Statement*>* stmt_list;
}

%start program

%token <id> IDENTIFIER
%token <ival> NUMBER
%token INT RETURN IF ELSE WHILE

%left '+' '-'
%left '*' '/'

%type <expr> expression
%type <stmt> statement
%type <stmt_list> statement_list

%%

program:
    statement_list           { root = std::make_unique<Program>($1); }
;

statement_list:
    statement                { $$ = new std::vector<Statement*>(); $$->push_back($1); }
  | statement_list statement { $$ = $1; $$->push_back($2); }
;

statement:
    RETURN expression ';'    { $$ = new ReturnStatement($2); }
;

expression:
    NUMBER                   { $$ = new IntegerLiteral($1); }
  | expression '+' expression { $$ = new BinaryExpr('+', $1, $3); }
  | expression '-' expression { $$ = new BinaryExpr('-', $1, $3); }
  | expression '*' expression { $$ = new BinaryExpr('*', $1, $3); }
  | expression '/' expression { $$ = new BinaryExpr('/', $1, $3); }
;

%%
