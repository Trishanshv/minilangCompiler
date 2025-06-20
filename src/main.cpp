#include "ast.hpp"

extern int yyparse();
extern std::unique_ptr<Program> root;
extern FILE* yyin;

int main(int argc, char** argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
    }
    if (yyparse() == 0 && root) {
        root->print();
    }
    return 0;
}
