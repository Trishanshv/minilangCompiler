%option yylineno
%option noyywrap

%{
#include <cstdlib>
#include <cstring>
#include <vector>
#include "ast.hpp"
#include "parser.hpp"

extern FILE* yyin;
int yylex();
void yyerror(const char* s);

int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_line = yylineno; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

static char* parse_string_literal(const char* text, int len) {
    char* buf = (char*)malloc(len);
    int j = 0;
    for (int i = 1; i < len - 1; ++i) {
        if (text[i] == '\\' && i + 1 < len - 1) {
            i++;
            switch (text[i]) {
                case 'n': buf[j++] = '\n'; break;
                case 't': buf[j++] = '\t'; break;
                case 'r': buf[j++] = '\r'; break;
                case '\\': buf[j++] = '\\'; break;
                case '\"': buf[j++] = '\"'; break;
                default: buf[j++] = text[i]; break;
            }
        } else {
            buf[j++] = text[i];
        }
    }
    buf[j] = '\0';
    return buf;
}
%}

%%

"//"[^\r\n]* {
    /* Skip single-line comments */
}

"/*"([^*]|\*+[^*/])*\*+"/" {
    /* Skip multi-line comments and update column for newlines */
    for (int i = 0; yytext[i] != '\0'; ++i) {
        if (yytext[i] == '\n') {
            yycolumn = 1;
        }
    }
}

\n {
    yycolumn = 1;
}

[ \t\r]+ {
    /* Skip whitespace */
}

\"([^\"\\]|\\.)*\" {
    yylval.id = parse_string_literal(yytext, yyleng);
    return STRING_LITERAL;
}

"if"       { return TOK_IF; }
"else"     { return TOK_ELSE; }
"while"    { return TOK_WHILE; }
"for"      { return TOK_FOR; }
"break"    { return TOK_BREAK; }
"continue" { return TOK_CONTINUE; }
"int"      { return INT; }
"void"     { return TOK_VOID; }
"return"   { return RETURN; }

"=="       { return EQ; }
"!="       { return NE; }
"<="       { return LE; }
">="       { return GE; }
"<"        { return '<'; }
">"        { return '>'; }

"&&"       { return AND; }
"||"       { return OR; }
"!"        { return '!'; }

"++"       { return TOK_INC; }
"--"       { return TOK_DEC; }

"+="       { return ADD_ASSIGN; }
"-="       { return SUB_ASSIGN; }
"*="       { return MUL_ASSIGN; }
"/="       { return DIV_ASSIGN; }

"="        { return '='; }
"+"        { return '+'; }
"-"        { return '-'; }
"*"        { return '*'; }
"/"        { return '/'; }
"%"        { return '%'; }

";"        { return ';'; }
","        { return ','; }
"{"        { return '{'; }
"}"        { return '}'; }
"("        { return '('; }
")"        { return ')'; }

[0-9]+ {
    yylval.ival = atoi(yytext);
    return NUMBER;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.id = strdup(yytext);
    return IDENTIFIER;
}

. {
    return yytext[0];
}

%%
