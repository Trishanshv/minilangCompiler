# MiniLang Compiler - Remaining Tasks & Project Status

**Generated:** September 2026  
**Status:** 🔴 Build Broken / Incomplete Feature Set  

---

## 📌 Where You Left Off

Your last commit was **`360747d` ("Maintained Markdown Files")**, where an attempt was made to resolve **Issue #1 (Semantic Analysis Integration)**. During that edit:
1. **`src/codegen.hpp` was corrupted**: A partial edit left syntax errors (`void registerVariab` cut off halfway, duplicated/broken `popScope()`, and missing `pushScope()` / `registerVariable` prototypes). Because of this, **the compiler does not compile in its current state**.
2. **`src/semantic.cpp` was left orphaned**: The `SymbolTable` implementation was duplicated inline into `src/codegen.hpp` rather than being extracted into a proper header (`semantic.hpp`), leaving `src/semantic.cpp` unused.
3. **Loop control (`break` / `continue`) is broken**: The AST nodes and lexer tokens exist, but loop context management (`setCurrentLoopBlocks`) is never called inside `WhileStatement::codegen`, causing immediate runtime failures whenever `break` or `continue` is used.
4. **Existing test samples in `tests/` don't match compiler capabilities**: Test files use features like `print(...)`, string literals `"..."`, and function definitions `int main() { ... }`, none of which are implemented in the parser or codegen.

---

## 📋 Comprehensive Checklist of Things Left To Do

All unfinished items are marked with `[ ]`. Items that are already functional are marked with `[x]`.

---

### 1. 🚨 Critical Compilation & Build Errors (Fix First)

- [x] **Fix syntax errors in `src/codegen.hpp`**:
  - [x] Repair truncated `void registerVariable(const std::string& name, llvm::AllocaInst* alloca);` prototype.
  - [x] Restore proper `void pushScope();` implementation (pushing both LLVM alloca map and `symbolTable.enterScope()`).
  - [x] Clean up duplicate and corrupted `popScope()` method in `src/codegen.hpp` (remove dangling `scopemplace_back(); }`).
- [x] **Fix CMake configuration issues in `CMakeLists.txt`**:
  - [x] Add `${LLVM_INCLUDE_DIRS}` and `${CMAKE_BINARY_DIR}` to `minilang_lib` target include directories so `codegen.cpp` and `ast.cpp` reliably resolve LLVM headers.
  - [x] Remove duplicate `target_link_libraries(minilang PRIVATE ${LLVM_LIBS})` calls (lines 64 and 73).
  - [x] Configure header include path for `include/` so include directories follow standard conventions.
- [x] **Fix `semantic.cpp` structure**:
  - [x] Create `include/semantic.hpp` (or `src/semantic.hpp`) with the `SymbolTable` class definition.
  - [x] Move implementation to `src/semantic.cpp`.
  - [x] Remove inline copy-pasted `SymbolTable` from `src/codegen.hpp` and `#include "semantic.hpp"` instead.

---

### 2. 🔤 Lexer Deficiencies (`src/lexer.l`)

- [x] **Add source location tracking**:
  - [x] Enable `%option yylineno` to track line numbers.
  - [x] Add column number tracking and pass location information (`YYLTYPE` / `yylloc`) to Bison.
- [x] **Add comment support**:
  - [x] Support single-line comments (`// ...`).
  - [x] Support multi-line block comments (`/* ... */`).
- [x] **Add string literal support**:
  - [x] Add regex for string literals (`\"([^\"\\]|\\.)*\"`).
  - [x] Add `STRING_LITERAL` token to `parser.y` and return escaped string values.
- [x] **Add missing operators & punctuation**:
  - [x] Explicitly define `{`, `}`, `,`, `=` tokens instead of letting them fall through to the catch-all dot rule (`.`).
  - [x] Compound assignment operators: `+=`, `-=`, `*=`, `/=`.
  - [x] Increment and decrement operators: `++`, `--` (needed for standard for-loops).
  - [x] Logical operators: `&&`, `||`, `!`.
  - [x] Modulo operator: `%`.
  - [ ] Bitwise operators: `&`, `|`, `^`, `~`, `<<`, `>>`.
- [ ] **Lexer error handling**:
  - [ ] Replace silent fallthrough rule with proper error reporting for unexpected characters.

---

### 3. 🌲 Parser & Grammar Gaps (`src/parser.y`)

- [x] **Implement For-Loop grammar**:
  - [x] Token `TOK_FOR` exists but has no grammar rule. Add rule:
    ```yacc
    TOK_FOR '(' var_decl_or_expr ';' expression ';' assignment_or_expr ')' statement
    ```
- [x] **Implement Function Definitions & Declarations**:
  - [x] Grammar allows top-level list of statements and functions.
  - [x] Add grammar rules for defining functions:
    ```yacc
    type IDENTIFIER '(' parameter_list ')' block
    ```
  - [x] Add parameter lists and empty parameter list support.
  - [x] Support global variable declarations alongside function bodies.
- [ ] **Support more data types in grammar**:
  - [x] `int` and `void` supported.
  - [ ] Add remaining types: `bool`, `float`, `char`, `string`.
- [x] **Support void return**:
  - [x] Allow `RETURN ';'` without requiring an expression.
- [x] **Add unary expressions in grammar**:
  - [x] Unary negation: `-expr`.
  - [x] Logical NOT: `!expr`.
- [x] **Add logical binary expressions**:
  - [x] Logical AND: `expr && expr`.
  - [x] Logical OR: `expr || expr`.
- [x] **Parser error handling & recovery**:
  - [x] Integrate line and column numbers into `yyerror(const char* s)`.
  - [x] Add Bison `error` tokens to statement rules so parsing can recover and report multiple errors instead of halting on the first one.
- [ ] **Fix AST memory management during parsing**:
  - [ ] Avoid memory leaks when parse errors abort compilation before AST is attached to `root`.

---

### 4. 🧱 AST Architecture & Memory Management (`src/ast.hpp`, `src/ast.cpp`)

- [x] **Unify codegen architecture**:
  - [x] Resolve dual implementation: AST virtual `codegen` methods delegate directly to `CodeGenContext::codegen(...)` as single source of truth.
- [x] **Add missing AST nodes**:
  - [x] `ForStatement`: initializer, condition, step, body.
  - [x] `FunctionDef`: return type, function name, parameter list, body block.
  - [x] `Parameter`: type, name.
  - [x] `UnaryExpr`: operator (`-`, `!`), operand.
  - [x] `LogicalExpr`: `&&`, `||` with short-circuiting support.
  - [x] `StringLiteral`: string value.
  - [x] `ExprStatement`: already exists in `ast.hpp`, handled in `CodeGenContext::codegen(Statement*)`.
- [x] **Fix memory leaks in existing AST nodes**:
  - [x] `FunctionCall`: destructor frees `args` vector and individual expression elements.
  - [x] Ensure all container nodes use `std::unique_ptr` for child expressions and statements.

---

### 5. 🔍 Semantic Analysis (`src/semantic.cpp`, `src/codegen.hpp`)

- [ ] **Establish dedicated semantic analysis pass**:
  - [ ] Perform semantic analysis as a dedicated AST traversal pass *before* code generation rather than inline inside LLVM IR emission.
- [ ] **Variable validation**:
  - [x] Variable redeclaration detection in the same scope (partially added in `codegen.cpp`).
  - [x] Undeclared variable use detection (partially added in `codegen.cpp`).
  - [ ] Check assignment to undeclared variables in `Assignment` (currently checks LLVM alloca rather than symbol table).
  - [ ] Detect use of uninitialized variables.
  - [ ] Detect unused variables (warning).
- [ ] **Type checking & validation**:
  - [ ] Type checking for binary arithmetic operands.
  - [ ] Type checking for comparisons.
  - [ ] Condition type validation for `if`, `while`, and `for` (ensure boolean/integer).
  - [ ] Type checking on variable initialization and assignment.
- [ ] **Function validation**:
  - [ ] Function signature registration in symbol table.
  - [ ] Function call arity check (argument count matches parameter count).
  - [ ] Function call argument type compatibility check.
  - [ ] Verify non-void functions always return a value along all execution paths.
- [ ] **Control flow validation**:
  - [ ] Validate `break` and `continue` statements at semantic check time (must be lexically enclosed within a loop).
  - [ ] Detect unreachable code after unconditional `return`, `break`, or `continue`.

---

### 6. ⚙️ Code Generation & LLVM Backend (`src/codegen.cpp`)

- [x] **Fix loop jump statements (`break` / `continue`)**:
  - [x] Maintain a stack of loop contexts (`std::vector<LoopContext> loopStack`) instead of a single `currentLoopEnd` / `currentLoopContinue` pointer.
  - [x] Push loop exit and continue blocks before codegening loop bodies, and pop on exit.
  - [x] Call `pushLoop` / `popLoop` in `WhileStatement::codegen`.
  - [x] Implement codegen for `ForStatement`.
- [x] **Fix Statement dispatch in `CodeGenContext::codegen(Statement*)`**:
  - [x] Add handlers for `ExprStatement`, `Block`, `BreakStatement`, `ContinueStatement`, and `FunctionDef`.
- [x] **Fix multi-statement Block codegen**:
  - [x] Handle blocks where a statement terminates execution (e.g. `return` in the middle of a block). Do not emit instructions into a closed basic block.
- [x] **Implement user function code generation**:
  - [x] Generate LLVM `Function` for user-defined functions with arguments allocated to local variables.
  - [x] Retain `main()` as either explicit user function or default entry point.
- [x] **Built-in runtime / I/O functions**:
  - [x] Provide built-in `print` helper polymorphic for string literals (`%s\n`) and integers (`%d\n`) via `printf`.
- [x] **Fix comparison in `BinaryExpr`**:
  - [x] Remove `case '=':` from `BinaryExpr::codegen` in `ast.cpp` and `codegen.cpp` (assignment should not be evaluated as equality comparison).

---

### 7. 🚀 Driver, Output Formats & CLI (`src/main.cpp`)

- [ ] **Remove debug prints in production flow**:
  - [ ] Remove hardcoded `yydebug = 1;` and debug `std::cout` prints ("Starting parsing", "Parsed expression statement") unless `-v` / `--debug` flag is passed.
- [ ] **Add Command Line Options**:
  - [ ] Output flag: `-o <filename>`.
  - [ ] Target emission flags:
    - [ ] `--emit-llvm`: Output human-readable LLVM IR (`.ll`).
    - [ ] `--emit-bc`: Output LLVM bitcode (`.bc`).
    - [ ] `-c` / `--emit-obj`: Output native object file (`.o` / `.obj`).
    - [ ] `-S` / `--emit-asm`: Output native assembly (`.s`).
  - [ ] Optimization level flag (`-O0`, `-O1`, `-O2`, `-O3`).
- [ ] **Add LLVM Optimization Pipeline**:
  - [ ] Set up LLVM `PassBuilder` / `FunctionPassManager` / `ModulePassManager` to run optimization passes (constant propagation, dead code elimination, mem2reg).
- [ ] **Native compilation / linking**:
  - [ ] Initialize LLVM native targets (`InitializeNativeTarget()`, `InitializeNativeTargetAsmPrinter()`).
  - [ ] Output native executable by invoking host linker (lld, clang, or gcc).

---

### 8. 🧪 Testing & Test Suite

- [x] **Fix existing broken test cases**:
  - [x] `tests/test.minilang`: Updated to valid variable declaration and string `print(...)`.
  - [x] `tests/test_input.minilang`: Fully supported with `int main() { ... }` function definitions.
  - [x] `src/test_if.minilang`: Fixed variable declaration and verified.
- [ ] **Setup automated testing framework**:
  - [ ] Integrate Catch2 or a Python-based end-to-end test runner (`run_tests.py`).
  - [ ] Add unit tests for AST nodes and SymbolTable.
  - [x] Add integration sample tests for:
    - [x] Arithmetic evaluation, modulo, and operator precedence.
    - [x] If-else branching and nested conditionals.
    - [x] While loops, for loops, and nested while loops.
    - [x] User-defined functions with parameters and calls.
    - [x] String literals, single-line/multi-line comments, and polymorphic `print`.
- [ ] **Add negative test cases**:
  - [ ] Tests that assert compilation fails with appropriate error messages on invalid syntax or invalid types.

---

### 9. 🧹 Codebase Cleanup & Maintenance

- [ ] **Delete duplicate/junk files**:
  - [x] Delete accidental typo file `,gitignore` in project root.
  - [ ] Consolidate empty/redundant forwarding header `include/ast.hpp`.
- [ ] **Reconcile outdated documentation**:
  - [ ] `ISSUE_1_RESOLUTION.md`, `TRACKER.md`, and `QA_REPORT.md` claim Issue #1 is completely resolved and verified, but the code was committed in a broken state. Reconcile docs to reflect real state.
  - [ ] Remove obsolete files or consolidate `DOCUMENTATION_INDEX.md`, `MAINTENANCE_SCHEDULE.md`, `CLEANUP_REPORT.md` into a single concise `README.md`.
