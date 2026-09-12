# MiniLang Compiler

A **production-grade compiler** for MiniLang — a C-like systems programming language — built from scratch in **C++17** using **Flex**, **Bison**, and the **LLVM** compiler infrastructure.

The compiler implements the **complete compilation pipeline**: lexical analysis → parsing → AST construction → semantic analysis → LLVM IR generation → optimization → multi-target code emission (`.ll`, `.bc`, `.s`, `.o`).

---

## ✨ Features

### Language Features
| Feature | Syntax | Status |
|---|---|---|
| Integer variables | `int x = 42;` | ✅ |
| String literals | `"hello world"` | ✅ |
| Arithmetic | `+`, `-`, `*`, `/`, `%` | ✅ |
| Comparisons | `<`, `>`, `<=`, `>=`, `==`, `!=` | ✅ |
| Logical operators | `&&`, `\|\|`, `!` (short-circuit) | ✅ |
| Unary operators | `-x`, `!x` | ✅ |
| Compound assignment | `+=`, `-=`, `*=`, `/=`, `++`, `--` | ✅ |
| Control flow | `if` / `else`, `while`, `for` | ✅ |
| Loop control | `break`, `continue` (nested) | ✅ |
| Functions | `int foo(int a, int b) { ... }` | ✅ |
| Void functions | `void bar() { ... }` | ✅ |
| Built-in I/O | `print(expr)` — polymorphic (int/string) | ✅ |
| Comments | `// line` and `/* block */` | ✅ |
| Return statements | `return expr;` / `return;` | ✅ |

### Compiler Features
- **Dedicated semantic analysis pass** with 15+ diagnostic checks, flow-sensitive definite-assignment analysis, and sound missing-return detection
- **Rich diagnostics** with line/column source locations: `[Line 5, Col 12] Semantic Error: ...`
- **LLVM optimization pipeline** (`-O0` through `-O3`) using the modern PassBuilder API
- **Multi-target emission**: LLVM IR (`.ll`), LLVM bitcode (`.bc`), native assembly (`.s`), and relocatable object files (`.o`)
- **Automated test suite** with positive, negative, and warning test categories

---

## 🏗️ Architecture

```
 ┌───────────────────────────────────────────────────────────────┐
 │                      CLI Driver (main.cpp)                   │
 │          -o, -O0..3, --emit-llvm/bc/asm/obj, -v              │
 └──────────────────────────┬────────────────────────────────────┘
                            ▼
 ┌────────────────────────────────────────────────────────────────┐
 │  Lexer (Flex)  ──▸  Parser (Bison)  ──▸  AST (C++ nodes)     │
 │  lexer.l            parser.y             ast.hpp / ast.cpp    │
 └──────────────────────────┬─────────────────────────────────────┘
                            ▼
 ┌────────────────────────────────────────────────────────────────┐
 │              Semantic Analyzer (semantic.cpp)                  │
 │  • Type checking         • Use-before-init (flow-sensitive)   │
 │  • Scope validation      • Missing return detection           │
 │  • Function arity/types  • Unreachable code warnings          │
 └──────────────────────────┬─────────────────────────────────────┘
                            ▼
 ┌────────────────────────────────────────────────────────────────┐
 │             LLVM IR Code Generation (codegen.cpp)             │
 │  AST visitor → IRBuilder → LLVM Module                        │
 └──────────────────────────┬─────────────────────────────────────┘
                            ▼
 ┌────────────────────────────────────────────────────────────────┐
 │         LLVM PassBuilder Optimization Pipeline                │
 │         (mem2reg, DCE, CSE, inlining, vectorization)          │
 └──────────────────────────┬─────────────────────────────────────┘
                            ▼
         ┌──────────┬───────────────┬──────────────┐
         ▼          ▼               ▼              ▼
     .ll (IR)    .bc (bitcode)   .s (assembly)   .o (object)
```

---

## 📦 Dependencies

| Dependency | Version | Purpose |
|---|---|---|
| C++17 compiler | GCC 7+ / Clang 5+ | Core language |
| Flex | 2.6+ | Lexer generator |
| Bison | 3.0+ | Parser generator |
| LLVM | 10+ (tested 14–18) | IR generation, optimization, target emission |
| CMake | 3.10+ | Build system |
| Python 3 | 3.6+ | Test runner (`run_tests.py`) |

---

## ⚒️ Build Instructions

### Linux / macOS

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install flex bison llvm-dev cmake g++ python3

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Verify
./minilang --help
```

### Windows (MSYS2 / MinGW)

```bash
pacman -S mingw-w64-x86_64-llvm mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-gcc flex bison

mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j4
```

---

## 🚀 Usage

```bash
# Default: emit LLVM IR to stdout
./minilang input.minilang

# Emit LLVM IR to a file
./minilang input.minilang -o output.ll

# Emit optimized LLVM bitcode
./minilang input.minilang -O2 -o output.bc --emit-bc

# Emit native assembly
./minilang input.minilang -S -o output.s

# Emit relocatable object file
./minilang input.minilang -c -o output.o

# Verbose mode
./minilang input.minilang -v -O3 -c -o output.o
```

### CLI Options

| Flag | Description |
|---|---|
| `-o <file>` | Write output to `<file>` |
| `-O0` to `-O3` | Optimization level (default: `-O0`) |
| `--emit-llvm` | Emit human-readable LLVM IR (`.ll`) — **default** |
| `--emit-bc` | Emit LLVM bitcode (`.bc`) |
| `-S`, `--emit-asm` | Emit native assembly (`.s`) |
| `-c`, `--emit-obj` | Emit native object file (`.o`) |
| `-v`, `--verbose` | Print pipeline progress and pass timing |
| `--debug` | Enable Bison parser debug trace |
| `-h`, `--help` | Show help menu |

---

## 📄 Example

**`hello.minilang`:**
```c
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(10);
    print(result);
    return 0;
}
```

**Compile and run:**
```bash
./minilang hello.minilang -O2 -c -o hello.o
gcc hello.o -o hello
./hello
# Output: 3628800
```

---

## 🧪 Testing

The project ships with an automated test suite covering three categories:

```bash
# Run the full test suite
python3 run_tests.py --compiler ./build/minilang --verbose

# Run via CTest (after cmake build)
cd build && ctest --output-on-failure
```

| Suite | Directory | Validates |
|---|---|---|
| **Positive** | `tests/*.minilang` | Successful compilation |
| **Negative** | `tests/negative/*.minilang` | Expected compile errors |
| **Warning** | `tests/warnings/*.minilang` | Compilation succeeds with expected warnings |

**Annotation support:** Add `// EXPECT-ERROR: <text>` or `// EXPECT-WARNING: <text>` comments in test files for precise diagnostic validation.

---

## 📂 Project Structure

```
minilangCompiler/
├── CMakeLists.txt              # Build configuration with CTest
├── README.md                   # This file
├── TODO_LIST.md                # Development roadmap tracker
├── run_tests.py                # Automated test runner
├── include/
│   ├── ast.hpp                 # AST forwarding header
│   └── semantic.hpp            # Symbol table & semantic types
├── src/
│   ├── lexer.l                 # Flex lexer (tokenizer)
│   ├── parser.y                # Bison grammar (parser)
│   ├── ast.hpp                 # AST node definitions
│   ├── ast.cpp                 # AST codegen dispatch
│   ├── semantic.cpp            # Dedicated semantic analysis pass
│   ├── codegen.hpp             # CodeGenContext class
│   ├── codegen.cpp             # LLVM IR generation + optimization + emission
│   └── main.cpp                # CLI driver
├── tests/
│   ├── *.minilang              # Positive test cases
│   ├── negative/               # Expected-failure tests
│   └── warnings/               # Expected-warning tests
└── build/                      # CMake build output (gitignored)
```

---

## 🔬 Semantic Analysis Checks

The compiler performs a comprehensive, dedicated semantic analysis pass before code generation:

| Check | Severity |
|---|---|
| Undeclared variable use | Error |
| Variable redeclaration in same scope | Error |
| Type mismatch in expressions / assignments | Error |
| Function call arity mismatch | Error |
| Function argument type mismatch | Error |
| `break`/`continue` outside loop | Error |
| Missing return on all paths (non-void) | Error |
| Use of uninitialized variable (flow-sensitive) | Warning |
| Unreachable code after return/break/continue | Warning |
| Unused variable declaration | Warning |

---

## 📜 License

This project is for educational and portfolio purposes. MIT License.