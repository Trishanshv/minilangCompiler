#include "ast.hpp"
#include "semantic.hpp"
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
        fprintf(stderr, "Usage: %s <source file>\n", argv[0]);
        return 1;
    }
    
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("fopen");
        return 1;
    }
    
    yydebug = 0;
    
    if (yyparse() != 0 || !root) {
        std::cerr << "Parsing failed.\n";
        fclose(yyin);
        return 1;
    }
    fclose(yyin);

    // Run dedicated semantic analysis pass
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(root.get())) {
        analyzer.printDiagnostics();
        std::cerr << "Semantic analysis failed with " << analyzer.getErrorCount() << " error(s).\n";
        return 1;
    }

    if (analyzer.getWarningCount() > 0) {
        analyzer.printDiagnostics();
    }

    // Code generation
    CodeGenContext context;
    if (!context.generateCode(root.get())) {
        std::cerr << "Code generation failed.\n";
        return 1;
    }

    context.module->print(llvm::outs(), nullptr);

    return 0;
}
