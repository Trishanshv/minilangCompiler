%code requires {
    #include "ast.hpp"
    #include <memory>
}

%{
#define YYDEBUG 1
#include <vector>
#include <memory>
#include "ast.hpp"

extern int yylex();
extern char* yytext;
void yyerror(const char *s) {
    fprintf(stderr, "Error: %s at token '%s'\n", s, yytext);
}

std::unique_ptr<Program> root;

template<typename T>
std::vector<T*>* make_vector(T* item) {
    auto vec = new std::vector<T*>();
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
%token INT RETURN 
%token TOK_IF TOK_ELSE TOK_WHILE TOK_FOR TOK_BREAK TOK_CONTINUE
%token EQ NE LE GE

%nonassoc LOWER_THAN_ELSE
%nonassoc TOK_ELSE
%left '+' '-'
%left '*' '/'
%left '<' '>' LE GE EQ NE

%type <expr> expression
%type <stmt> statement
%type <stmt_list> statement_list
%type <expr_list> expression_list

%%

program:
    statement_list { root = std::make_unique<Program>($1); }
;

statement_list:
    statement { $$ = make_vector($1); }
  | statement_list statement { $1->push_back($2); $$ = $1; }
;

statement:
    RETURN expression ';' {
        $$ = new ReturnStatement($2);   
    }
    | INT IDENTIFIER '=' expression ';' {
        $$ = new VarDeclaration($2, $4);   
        free($2);  
    }
    | IDENTIFIER '=' expression ';' {
        $$ = new Assignment($1, $3);   
        free($1); 
    }
    | '{' statement_list '}' {
        $$ = new Block($2);   
    }
    | expression ';' {
        $$ = new ExprStatement($1);   
        printf("Parsed expression statement\n");
    }
    | TOK_IF '(' expression ')' statement %prec LOWER_THAN_ELSE {
        $$ = new IfStatement($3, $5);
    }
    | TOK_IF '(' expression ')' statement TOK_ELSE statement {
        $$ = new IfStatement($3, $5, $7);
    }
    | TOK_WHILE '(' expression ')' statement {
        $$ = new WhileStatement($3, $5);
    }
    | TOK_BREAK ';' {
        $$ = new BreakStatement();
    }
    | TOK_CONTINUE ';' {
        $$ = new ContinueStatement();
    }
;

expression:
    NUMBER {
        $$ = new IntegerLiteral($1);  // Raw pointer creation
    }
    | IDENTIFIER {
        $$ = new VariableExpr($1);   
        free($1);  // Free the lexer-allocated string
    }
    | '(' expression ')' {
        $$ = $2;  // Direct pass-through
    }
    | expression '+' expression {
        $$ = new BinaryExpr('+', $1, $3);   
    }
    | expression '-' expression {
        $$ = new BinaryExpr('-', $1, $3);   
    }
    | expression '*' expression {
        $$ = new BinaryExpr('*', $1, $3);   
    }
    | expression '/' expression {
        $$ = new BinaryExpr('/', $1, $3);   
    }
    | expression '<' expression {
        $$ = new ComparisonExpr("<", $1, $3);   
    }
    | expression '>' expression {
        $$ = new ComparisonExpr(">", $1, $3);   
    }
    | expression LE expression {
        $$ = new ComparisonExpr("<=", $1, $3);   
    }
    | expression GE expression {
        $$ = new ComparisonExpr(">=", $1, $3);   
    }
    | expression EQ expression {
        $$ = new ComparisonExpr("==", $1, $3);   
    }
    | expression NE expression {
        $$ = new ComparisonExpr("!=", $1, $3);   
    }
    | IDENTIFIER '(' ')' {
        $$ = new FunctionCall(
            std::string($1), 
            new std::vector<Expression*>()   
        );
        free($1); 
    }
    | IDENTIFIER '(' expression_list ')' {
        $$ = new FunctionCall(std::string($1), $3);   
        free($1);
    }
;

expression_list:
    expression { $$ = make_vector($1); }
  | expression_list ',' expression { $1->push_back($3); $$ = $1; }
;

%%