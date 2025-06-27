%code requires {
    #include "ast.hpp"
}

%{
#define YYDEBUG 1
#include <vector>
#include <memory>
#include "ast.hpp"

extern int yylex();
void yyerror(const char *s) {
    fprintf(stderr, "Error: %s at token '%s'\n", s, yylex);
}

std::unique_ptr<Program> root;

template<typename T>
std::vector<T>* make_vector(T item) {
    auto vec = new std::vector<T>();
    vec->push_back(item);
    return vec;
}
%}

%union {
    int ival;
    char* id;
    Expression* expr;
    Statement* stmt;
    std::vector<Statement*>* stmt_list;
    std::vector<Expression*>* expr_list;
}

%start program

%token <id> IDENTIFIER
%token <ival> NUMBER
%token INT RETURN IF ELSE WHILE
%token EQ NE LE GE

%left '+' '-'
%left '*' '/'
%left '<' '>' LE GE EQ NE

%type <expr> expression
%type <stmt> statement
%type <stmt_list> statement_list

%%

program:
    statement_list { root = std::make_unique<Program>($1); }
;

statement_list:
    statement { $$ = make_vector($1); }
  | statement_list statement { $1->push_back($2); $$ = $1; }
;

statement:
    RETURN expression ';' { $$ = new ReturnStatement($2); }
  | INT IDENTIFIER '=' expression ';' { 
        $$ = new VarDeclaration($2, $4); 
        free($2);
    }
  | IDENTIFIER '=' expression ';' { 
        $$ = new Assignment($1, $3); 
        free($1);
    }
  | '{' statement_list '}' { $$ = new Block($2); }
  | IF '(' expression ')' statement { $$ = new IFStatement($3, $5); }
  | IF '(' expression ')' statement ELSE statement { $$ = new IFStatement($3, $5, $7); }
  | WHILE '(' expression ')' statement { $$ = new WhileStatement($3, $5); }
  | expression ';' { 
        $$ = new ExprStatement($1); 
        printf("Parsed expression statement\n");  // Debug output
    }
;

expression:
    NUMBER { $$ = new IntegerLiteral($1); }
  | IDENTIFIER { $$ = new VariableExpr($1); free($1); }
  | '(' expression ')' { $$ = $2; }
  | expression '+' expression { $$ = new BinaryExpr('+', $1, $3); }
  | expression '-' expression { $$ = new BinaryExpr('-', $1, $3); }
  | expression '*' expression { $$ = new BinaryExpr('*', $1, $3); }
  | expression '/' expression { $$ = new BinaryExpr('/', $1, $3); }
  | expression '<' expression { $$ = new ComparisonExpr("<", $1, $3); }
  | expression '>' expression { $$ = new ComparisonExpr(">", $1, $3); }
  | expression LE expression { $$ = new ComparisonExpr("<=", $1, $3); }
  | expression GE expression { $$ = new ComparisonExpr(">=", $1, $3); }
  | expression EQ expression { $$ = new ComparisonExpr("==", $1, $3); }
  | expression NE expression { $$ = new ComparisonExpr("!=", $1, $3); }
;

%%