%{
#include <cstdlib>
#include <cstring>
#include <vector>
#include "parser.hpp" 
#include "ast.hpp"

extern FILE* yyin;
int yylex();
void yyerror(const char* s);
%}

%%
"if"       { return TOK_IF; }
"else"     { return TOK_ELSE; }
"while"    { return TOK_WHILE; }
"for"      { return TOK_FOR; }
"break"    { return TOK_BREAK; }
"continue" { return TOK_CONTINUE; }
"int"       { return INT; }
"return"    { return RETURN; }

[0-9]+      { yylval.ival = atoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.id = strdup(yytext); return IDENTIFIER; }

"+"         { return '+'; }
"-"         { return '-'; }
"*"         { return '*'; }
"/"         { return '/'; }

";"         { return ';'; }
"("         { return '('; }
")"         { return ')'; }

"=="        { return EQ; }
"!="        { return NE; }
"<="        { return LE; }
">="        { return GE; }
"<"         { return '<'; }
">"         { return '>'; }


[ \t\r\n]+  { /* skip whitespace */ }

.           { return yytext[0]; }

%%

int yywrap() {
    return 1;
}
