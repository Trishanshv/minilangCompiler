%code requires {
    #include "ast.hpp"
    #include <memory>
    #include <vector>
    #include <string>
}

%{
#define YYDEBUG 1
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>
#include "ast.hpp"

extern int yylex();
extern char* yytext;
extern int yylineno;

void yyerror(const char *s) {
    fprintf(stderr, "Error at line %d: %s near '%s'\n", yylineno, s, yytext);
}

std::unique_ptr<Program> root;

template<typename T>
std::vector<T*>* make_vector(T* item) {
    auto vec = new std::vector<T*>();
    if (item) {
        vec->push_back(item);
    }
    return vec;
}
%}

%locations

%union {
    int ival;
    char* id;
    char* type_name;
    Expression* expr;
    Statement* stmt;
    std::vector<Statement*>* stmt_list;
    std::vector<Expression*>* expr_list;
    std::vector<Parameter>* param_list;
}

%start program

%token <id> IDENTIFIER STRING_LITERAL
%token <ival> NUMBER
%token INT TOK_VOID RETURN
%token TOK_IF TOK_ELSE TOK_WHILE TOK_FOR TOK_BREAK TOK_CONTINUE
%token EQ NE LE GE AND OR
%token TOK_INC TOK_DEC ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN

%nonassoc LOWER_THAN_ELSE
%nonassoc TOK_ELSE
%right '=' ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN
%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right '!' UMINUS

%type <expr> expression for_cond
%type <stmt> statement for_init for_inc function_def top_level_item
%type <stmt_list> statement_list top_level_list
%type <expr_list> expression_list
%type <param_list> param_list param_nonempty_list
%type <type_name> type

%%

program:
    top_level_list { root = std::make_unique<Program>($1); }
  | /* empty */    { root = std::make_unique<Program>(new std::vector<Statement*>()); }
;

top_level_list:
    top_level_item { $$ = make_vector($1); }
  | top_level_list top_level_item {
        if ($2) {
            $1->push_back($2);
        }
        $$ = $1;
    }
;

top_level_item:
    function_def { $$ = $1; }
  | statement    { $$ = $1; }
;

statement_list:
    statement { $$ = make_vector($1); }
  | statement_list statement {
        if ($2) {
            $1->push_back($2);
        }
        $$ = $1;
    }
;

function_def:
    type IDENTIFIER '(' param_list ')' '{' statement_list '}' {
        $$ = new FunctionDef($1, $2, $4, new Block($7));
        free($1);
        free($2);
    }
  | type IDENTIFIER '(' param_list ')' '{' '}' {
        $$ = new FunctionDef($1, $2, $4, new Block(new std::vector<Statement*>()));
        free($1);
        free($2);
    }
;

type:
    INT      { $$ = strdup("int"); }
  | TOK_VOID { $$ = strdup("void"); }
;

param_list:
    /* empty */         { $$ = new std::vector<Parameter>(); }
  | param_nonempty_list { $$ = $1; }
;

param_nonempty_list:
    type IDENTIFIER {
        $$ = new std::vector<Parameter>();
        $$->emplace_back($1, $2);
        free($1);
        free($2);
    }
  | param_nonempty_list ',' type IDENTIFIER {
        $1->emplace_back($3, $4);
        free($3);
        free($4);
        $$ = $1;
    }
;

statement:
    RETURN expression ';' {
        $$ = new ReturnStatement($2);   
    }
    | RETURN ';' {
        $$ = new ReturnStatement(nullptr);
    }
    | INT IDENTIFIER '=' expression ';' {
        $$ = new VarDeclaration($2, $4);   
        free($2);  
    }
    | INT IDENTIFIER ';' {
        $$ = new VarDeclaration($2, nullptr);
        free($2);
    }
    | IDENTIFIER '=' expression ';' {
        $$ = new Assignment($1, $3);   
        free($1); 
    }
    | IDENTIFIER ADD_ASSIGN expression ';' {
        $$ = new Assignment($1, new BinaryExpr('+', new VariableExpr($1), $3));
        free($1);
    }
    | IDENTIFIER SUB_ASSIGN expression ';' {
        $$ = new Assignment($1, new BinaryExpr('-', new VariableExpr($1), $3));
        free($1);
    }
    | IDENTIFIER MUL_ASSIGN expression ';' {
        $$ = new Assignment($1, new BinaryExpr('*', new VariableExpr($1), $3));
        free($1);
    }
    | IDENTIFIER DIV_ASSIGN expression ';' {
        $$ = new Assignment($1, new BinaryExpr('/', new VariableExpr($1), $3));
        free($1);
    }
    | IDENTIFIER TOK_INC ';' {
        $$ = new Assignment($1, new BinaryExpr('+', new VariableExpr($1), new IntegerLiteral(1)));
        free($1);
    }
    | IDENTIFIER TOK_DEC ';' {
        $$ = new Assignment($1, new BinaryExpr('-', new VariableExpr($1), new IntegerLiteral(1)));
        free($1);
    }
    | '{' statement_list '}' {
        $$ = new Block($2);   
    }
    | '{' '}' {
        $$ = new Block(new std::vector<Statement*>());
    }
    | expression ';' {
        $$ = new ExprStatement($1);   
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
    | TOK_FOR '(' for_init ';' for_cond ';' for_inc ')' statement {
        $$ = new ForStatement($3, $5, $7, $9);
    }
    | TOK_BREAK ';' {
        $$ = new BreakStatement();
    }
    | TOK_CONTINUE ';' {
        $$ = new ContinueStatement();
    }
    | error ';' {
        yyerrok;
        $$ = nullptr;
    }
;

for_init:
    /* empty */ { $$ = nullptr; }
    | INT IDENTIFIER '=' expression {
        $$ = new VarDeclaration($2, $4);
        free($2);
    }
    | IDENTIFIER '=' expression {
        $$ = new Assignment($1, $3);
        free($1);
    }
    | expression {
        $$ = new ExprStatement($1);
    }
;

for_cond:
    /* empty */ { $$ = nullptr; }
    | expression { $$ = $1; }
;

for_inc:
    /* empty */ { $$ = nullptr; }
    | IDENTIFIER '=' expression {
        $$ = new Assignment($1, $3);
        free($1);
    }
    | IDENTIFIER ADD_ASSIGN expression {
        $$ = new Assignment($1, new BinaryExpr('+', new VariableExpr($1), $3));
        free($1);
    }
    | IDENTIFIER SUB_ASSIGN expression {
        $$ = new Assignment($1, new BinaryExpr('-', new VariableExpr($1), $3));
        free($1);
    }
    | IDENTIFIER TOK_INC {
        $$ = new Assignment($1, new BinaryExpr('+', new VariableExpr($1), new IntegerLiteral(1)));
        free($1);
    }
    | IDENTIFIER TOK_DEC {
        $$ = new Assignment($1, new BinaryExpr('-', new VariableExpr($1), new IntegerLiteral(1)));
        free($1);
    }
    | expression {
        $$ = new ExprStatement($1);
    }
;

expression:
    NUMBER {
        $$ = new IntegerLiteral($1);
    }
    | STRING_LITERAL {
        $$ = new StringLiteral($1);
        free($1);
    }
    | IDENTIFIER {
        $$ = new VariableExpr($1);   
        free($1);
    }
    | '(' expression ')' {
        $$ = $2;
    }
    | '-' expression %prec UMINUS {
        $$ = new UnaryExpr('-', $2);
    }
    | '!' expression {
        $$ = new UnaryExpr('!', $2);
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
    | expression '%' expression {
        $$ = new BinaryExpr('%', $1, $3);
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
    | expression AND expression {
        $$ = new LogicalExpr("&&", $1, $3);
    }
    | expression OR expression {
        $$ = new LogicalExpr("||", $1, $3);
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