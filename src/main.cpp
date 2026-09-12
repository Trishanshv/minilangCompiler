#include "ast.hpp"
#include "semantic.hpp"
#include "codegen.hpp"
#include "parser.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

extern int yydebug;
extern int yyparse();
extern std::unique_ptr<Program> root;
extern FILE* yyin;

enum class OutputFormat {
    LLVM_IR,
    BITCODE,
    ASSEMBLY,
    OBJECT
};

struct CommandLineOptions {
    std::string inputFile;
    std::string outputFile;
    OutputFormat format = OutputFormat::LLVM_IR;
    bool formatExplicit = false;
    int optLevel = 0;
    bool verbose = false;
    bool debug = false;
    bool showHelp = false;
};

static void printUsage(const char* progName) {
    std::cout << "MiniLang Compiler (minilang) - Version 1.0\n"
              << "Usage: " << progName << " [options] <source-file>\n\n"
              << "Options:\n"
              << "  -o <file>          Write output to <file>\n"
              << "  -O0, -O1, -O2, -O3 Set optimization level (default: -O0)\n"
              << "  --emit-llvm        Emit human-readable LLVM IR (.ll) [default to stdout]\n"
              << "  --emit-bc          Emit LLVM bitcode (.bc)\n"
              << "  -S, --emit-asm     Emit native assembly (.s)\n"
              << "  -c, --emit-obj     Emit native relocatable object file (.o)\n"
              << "  -v, --verbose      Enable verbose logging and pass progression\n"
              << "  --debug            Enable parser debugging trace (yydebug)\n"
              << "  -h, --help         Display this help menu\n\n"
              << "Examples:\n"
              << "  " << progName << " input.minilang                    # LLVM IR to stdout\n"
              << "  " << progName << " input.minilang -o out.ll          # LLVM IR to file\n"
              << "  " << progName << " input.minilang -O2 -o out.bc      # Optimized bitcode\n"
              << "  " << progName << " input.minilang -c -o out.o        # Relocatable object\n"
              << "  " << progName << " input.minilang -S -o out.s        # Native assembly\n";
}

static std::string getFileBaseName(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        return filename.substr(0, lastDot);
    }
    return filename;
}

static bool parseArguments(int argc, char** argv, CommandLineOptions& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
            return true;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--debug") {
            opts.debug = true;
        } else if (arg == "--emit-llvm") {
            opts.format = OutputFormat::LLVM_IR;
            opts.formatExplicit = true;
        } else if (arg == "--emit-bc") {
            opts.format = OutputFormat::BITCODE;
            opts.formatExplicit = true;
        } else if (arg == "-S" || arg == "--emit-asm") {
            opts.format = OutputFormat::ASSEMBLY;
            opts.formatExplicit = true;
        } else if (arg == "-c" || arg == "--emit-obj") {
            opts.format = OutputFormat::OBJECT;
            opts.formatExplicit = true;
        } else if (arg == "-O0") {
            opts.optLevel = 0;
        } else if (arg == "-O1") {
            opts.optLevel = 1;
        } else if (arg == "-O2") {
            opts.optLevel = 2;
        } else if (arg == "-O3") {
            opts.optLevel = 3;
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Error: -o option requires an output filename argument.\n";
                return false;
            }
            opts.outputFile = argv[++i];
        } else if (arg.rfind("-o", 0) == 0 && arg.length() > 2) {
            opts.outputFile = arg.substr(2);
        } else if (arg[0] == '-') {
            std::cerr << "Error: Unrecognized option '" << arg << "'. Use --help for usage.\n";
            return false;
        } else {
            if (!opts.inputFile.empty()) {
                std::cerr << "Error: Multiple input files provided ('" << opts.inputFile << "' and '" << arg << "').\n";
                return false;
            }
            opts.inputFile = arg;
        }
    }

    if (opts.inputFile.empty() && !opts.showHelp) {
        std::cerr << "Error: No input source file provided.\n";
        return false;
    }

    // Deduce format from output filename extension if not explicitly given
    if (!opts.formatExplicit && !opts.outputFile.empty()) {
        size_t dot = opts.outputFile.find_last_of('.');
        if (dot != std::string::npos) {
            std::string ext = opts.outputFile.substr(dot);
            if (ext == ".ll") {
                opts.format = OutputFormat::LLVM_IR;
            } else if (ext == ".bc") {
                opts.format = OutputFormat::BITCODE;
            } else if (ext == ".s" || ext == ".asm") {
                opts.format = OutputFormat::ASSEMBLY;
            } else if (ext == ".o" || ext == ".obj") {
                opts.format = OutputFormat::OBJECT;
            }
        }
    }

    // Default output filenames for target formats if -o was omitted
    if (opts.outputFile.empty() && !opts.inputFile.empty()) {
        std::string base = getFileBaseName(opts.inputFile);
        if (opts.format == OutputFormat::BITCODE) {
            opts.outputFile = base + ".bc";
        } else if (opts.format == OutputFormat::ASSEMBLY) {
            opts.outputFile = base + ".s";
        } else if (opts.format == OutputFormat::OBJECT) {
            opts.outputFile = base + ".o";
        }
        // For LLVM_IR without -o, outputFile stays empty so it streams to stdout
    }

    return true;
}

int main(int argc, char** argv) {
    CommandLineOptions opts;
    if (!parseArguments(argc, argv, opts)) {
        printUsage(argv[0]);
        return 1;
    }

    if (opts.showHelp) {
        printUsage(argv[0]);
        return 0;
    }

    yyin = fopen(opts.inputFile.c_str(), "r");
    if (!yyin) {
        perror(("Error opening input file " + opts.inputFile).c_str());
        return 1;
    }

    yydebug = opts.debug ? 1 : 0;

    if (opts.verbose) {
        std::cout << "[MiniLang] Parsing " << opts.inputFile << "...\n";
    }

    if (yyparse() != 0 || !root) {
        std::cerr << "Parsing failed.\n";
        fclose(yyin);
        return 1;
    }
    fclose(yyin);

    if (opts.verbose) {
        std::cout << "[MiniLang] Running semantic analysis pass...\n";
    }

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

    if (opts.verbose) {
        std::cout << "[MiniLang] Generating LLVM IR...\n";
    }

    // Code generation
    CodeGenContext context;
    if (!context.generateCode(root.get())) {
        std::cerr << "Code generation failed.\n";
        return 1;
    }

    // Optimization pipeline
    if (!context.optimizeModule(opts.optLevel, opts.verbose)) {
        std::cerr << "Optimization failed.\n";
        return 1;
    }

    // Target emission
    bool success = false;
    switch (opts.format) {
        case OutputFormat::LLVM_IR:
            if (opts.verbose && !opts.outputFile.empty()) {
                std::cout << "[MiniLang] Emitting LLVM IR to " << opts.outputFile << "...\n";
            }
            success = context.emitLLVMIR(opts.outputFile);
            break;

        case OutputFormat::BITCODE:
            if (opts.verbose) {
                std::cout << "[MiniLang] Emitting LLVM bitcode to " << opts.outputFile << "...\n";
            }
            success = context.emitBitcode(opts.outputFile);
            break;

        case OutputFormat::ASSEMBLY:
            if (opts.verbose) {
                std::cout << "[MiniLang] Emitting native assembly to " << opts.outputFile << "...\n";
            }
            success = context.emitAssembly(opts.outputFile);
            break;

        case OutputFormat::OBJECT:
            if (opts.verbose) {
                std::cout << "[MiniLang] Emitting native object file to " << opts.outputFile << "...\n";
            }
            success = context.emitObjectFile(opts.outputFile);
            break;
    }

    if (!success) {
        std::cerr << "Target emission failed.\n";
        return 1;
    }

    if (opts.verbose && !opts.outputFile.empty()) {
        std::cout << "[MiniLang] Compilation finished successfully -> " << opts.outputFile << "\n";
    }

    return 0;
}
