# MiniLang Compiler - Development Guide

## Project Overview

The MiniLang Compiler is a complete implementation of a compiler for a minimal C-like language, built using:
- **Flex** for lexical analysis
- **Bison** for parsing
- **LLVM** for code generation
- **C++17** for implementation

## Development Setup

### Prerequisites

1. **Install LLVM 15+**
   - Windows: Download from [llvm.org](https://llvm.org/download/)
   - Or use package manager (vcpkg, Conan, etc.)

2. **Install Flex & Bison**
   - Windows: Use [GnuWin32](http://gnuwin32.sourceforge.net/) or [winflexbison](https://github.com/lexxmark/winflexbison)

3. **Install CMake 3.10+**
   - Download from [cmake.org](https://cmake.org/download/)

4. **C++ Compiler**
   - MSVC (Visual Studio 2019+) or Clang

### Clone & Setup

```bash
git clone <repo-url>
cd minilangCompiler
mkdir build
cd build
cmake ..
```

## Compiler Architecture

```
┌─────────────────┐
│  Source Code    │ (*.minilang)
└────────┬────────┘
         │
    ┌────▼─────┐
    │  LEXER    │ (lexer.l → Flex)
    │ Tokenize  │
    └────┬─────┘
         │
    ┌────▼─────┐
    │  PARSER   │ (parser.y → Bison)
    │  Build    │
    │   AST     │
    └────┬─────┘
         │
    ┌────▼──────────┐
    │  SEMANTIC     │
    │  ANALYSIS     │ (semantic.cpp)
    │  Symbol Table │  ⚠️ NOT INTEGRATED YET
    └────┬──────────┘
         │
    ┌────▼──────────┐
    │  CODE GEN     │
    │ LLVM IR Gen   │ (codegen.cpp)
    └────┬──────────┘
         │
    ┌────▼─────────────┐
    │  LLVM Backend    │
    │  Native Code     │
    └──────────────────┘
```

## File Organization

### Source Files (`src/`)

| File | Purpose | Lines | Status |
|------|---------|-------|--------|
| **main.cpp** | Entry point; orchestrates pipeline | 30-40 | [DONE] |
| **lexer.l** | Flex lexical rules | 40-50 | [DONE] |
| **parser.y** | Bison grammar rules | 100-120 | [DONE] |
| **ast.hpp** | AST node class definitions | 150+ | [DONE] |
| **ast.cpp** | AST node implementations | 150+ | [DONE] |
| **codegen.hpp** | Code generation interface | 50+ | [DONE] |
| **codegen.cpp** | LLVM IR codegen impl | 200+ | [DONE] |
| **semantic.cpp** | Symbol table & semantic checking | 50+ | [TODO] Not integrated |

### Test Files

- `test.minilang` - Simple return statement
- `test_if.minilang` - If/else logic test
- `tests/test_input.minilang` - Additional test
- `tests/test.minilang` - Generic test

## Current Implementation Status

### Complete Components

1. **Lexer (src/lexer.l)**
   - Tokenizes all keywords, operators, identifiers, numbers
   - Skips whitespace
   - Ready for use

2. **Parser (src/parser.y)**
   - Bison grammar complete
   - Handles statements: return, declarations, assignments, blocks
   - Handles expressions: binary ops, comparisons, literals, variables
   - Operator precedence properly defined
   - AST creation working

3. **AST (src/ast.hpp, src/ast.cpp)**
   - All node types implemented
   - Virtual methods for codegen
   - Proper memory management with unique_ptr

4. **Code Generation (src/codegen.cpp)**
   - LLVM IR generation working
   - Supports:
     - Variable declarations (alloca)
     - Binary operations (+, -, *, /)
     - Comparisons (==, !=, <, >, <=, >=)
     - If/else branching
     - While loops
     - Return statements
     - Scope management (push/pop scopes)

### Incomplete/Issues

1. **Semantic Analysis Not Integrated**
   - `src/semantic.cpp` has SymbolTable implementation but NOT used
   - **Action Required:** Connect to CodeGenContext in main pipeline
   - **Impact:** No type checking, no redeclaration detection

2. **Missing Features**
   - [ ] Function declarations & calls
   - [ ] For-loop (parsed but not codegened)
   - [ ] Break/Continue statements
   - [ ] Array types
   - [ ] Pointers
   - [ ] Structs/Records

3. **Testing**
   - Only manual testing
   - No unit test framework integrated
   - No regression test suite

4. **Error Handling**
   - Parser errors don't include line numbers
   - No error recovery
   - Limited error context

## Typical Development Workflow

### Adding a New Feature

1. **Add Lexer Tokens** (src/lexer.l)
   ```
   "keyword"    { return TOK_KEYWORD; }
   ```

2. **Update Parser Grammar** (src/parser.y)
   ```
   %token TOK_KEYWORD
   
   statement:
       TOK_KEYWORD ... { $$ = new YourNode(...); }
   ```

3. **Create AST Node** (src/ast.hpp + src/ast.cpp)
   ```cpp
   struct YourNode : Statement {
       // fields
       llvm::Value* codegen(CodeGenContext& ctx) override;
   };
   ```

4. **Implement Code Generation** (src/codegen.cpp)
   ```cpp
   llvm::Value* YourNode::codegen(CodeGenContext& ctx) {
       // LLVM IR generation
   }
   ```

5. **Build & Test**
   ```bash
   cd build && make && ./minilang ../test_yourfeature.minilang
   ```

### Building

```bash
cd build
cmake ..     # Configure (only needed if CMakeLists changed)
make         # Build (or ninja)
```

Generated executables:
- `./minilang` - Main compiler executable

### Running Tests

```bash
cd build
./minilang ../test.minilang              # Simple test
./minilang ../test_if.minilang           # If/else test
./minilang ../tests/test_input.minilang  # Another test
```

To see LLVM IR output:
```bash
./minilang ../test.minilang > output.ll
cat output.ll
```

## Important Notes

### Memory Management
- Use `std::unique_ptr<>` for AST nodes
- Use `strdup()` for C strings (remember to `free()` in parser)
- LLVM manages its own memory (LLVMContext, Module, etc.)

### Parser Development Tips
- `%debug 1` to enable debug output in Bison
- Use `$$` for result, `$1, $2...` for production items
- Remember precedence declarations: `%left`, `%right`, `%nonassoc`
- Test incrementally; add one rule at a time

### Lexer Development Tips
- Order matters in Flex! Longer patterns should come first
- Use `yylval` to pass semantic values
- Use `return <token>;` to emit tokens

### Debugging

**Enable verbose output:**
- In parser default is on: `yydebug=1;` in main.cpp
- Set to 0 to disable

**Debug LLVM IR:**
```cpp
module->print(llvm::outs(), nullptr);
```

## Known Pitfalls

1. CIRCULAR INCLUDES - Be careful with forward declarations
2. MEMORY LEAKS - Check with valgrind in Linux
3. LLVM VERSION MISMATCH - Check LLVM version compatibility
4. PARSER CONFLICTS - Use Bison warnings (`-Werror=conflicts`)
5. FLEX/BISON DEPENDENCIES - Regenerate if you modify .l or .y files

## Next Steps

### Immediate (v0.2.0)
- [ ] Integrate semantic.cpp into pipeline
- [ ] Add SymbolTable to CodeGenContext
- [ ] Create 10+ unit tests with Catch2
- [ ] Add proper error messages with line numbers

### Short-term (v0.3.0)
- [ ] Complete for-loop codegen
- [ ] Add break/continue support
- [ ] Implement function declarations
- [ ] Add more data types (float, double, bool)

### Long-term (v0.4.0+)
- [ ] Generic type system
- [ ] Array/Pointer types
- [ ] Struct definitions
- [ ] Optimization passes
- [ ] Debug symbols (DWARF)

## Resources

- [LLVM C++ API Tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/)
- [Flex & Bison Book](https://www.oreilly.com/library/view/flex-a-lexical/9780596805418/)
- [Compiler Construction (Louden)](https://www.elsevier.com/products/compiler-construction)
- [LLVM Language Reference](https://llvm.org/docs/LangRef/)

---

**Last Updated:** March 31, 2026  
**Maintained by:** Developer
