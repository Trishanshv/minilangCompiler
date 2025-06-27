#include "ast.hpp"
#include<cstdio>

extern int yydebug;
extern int yyparse();
extern std::unique_ptr<Program> root;
extern FILE* yyin;

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Using: %s <source file>\n", argv[0]);
        return 1;
    }
    
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("fopen");
        return 1;
    }
    
    yydebug=1;
    
    if (yyparse() == 0) {
        if (root) {
            root->print();
        } else {
            fprintf(stderr, "AST not generated.\n");
        }
    } else {
        fprintf(stderr, "Parsing failed.\n");
    }

    fclose(yyin);
    return 0;
}
