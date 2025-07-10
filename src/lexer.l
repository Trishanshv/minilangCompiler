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
"while"     { return WHILE;}
"else"      { return ELSE; } 
"if"        { return IF; }
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
