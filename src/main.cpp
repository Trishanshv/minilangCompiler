#include "ast.hpp"
#include "codegen.hpp"
#include "parser.hpp"
#include <iostream>
#include <fstream>

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
    std::cout<<"Starting parsing\n";
    
    if (yyparse() != 0) {
        std::cerr << "Parsing failed.\n";
        return 1;
    }

    std::cout << "Generating LLVM IR...\n";
    CodeGenContext context;
    context.generateCode(root.get());

    context.module->print(llvm::outs(), nullptr);

    return 0;
}
