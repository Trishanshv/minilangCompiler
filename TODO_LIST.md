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

- [ ] **Fix syntax errors in `src/codegen.hpp`**:
  - [ ] Repair truncated `void registerVariable(const std::string& name, llvm::AllocaInst* alloca);` prototype.
  - [ ] Restore proper `void pushScope();` implementation (pushing both LLVM alloca map and `symbolTable.enterScope()`).
  - [ ] Clean up duplicate and corrupted `popScope()` method in `src/codegen.hpp` (remove dangling `scopemplace_back(); }`).
- [ ] **Fix CMake configuration issues in `CMakeLists.txt`**:
  - [ ] Add `${LLVM_INCLUDE_DIRS}` and `${CMAKE_BINARY_DIR}` to `minilang_lib` target include directories so `codegen.cpp` and `ast.cpp` reliably resolve LLVM headers.
  - [ ] Remove duplicate `target_link_libraries(minilang PRIVATE ${LLVM_LIBS})` calls (lines 64 and 73).
  - [ ] Configure header include path for `include/` so include directories follow standard conventions.
- [ ] **Fix `semantic.cpp` structure**:
  - [ ] Create `include/semantic.hpp` (or `src/semantic.hpp`) with the `SymbolTable` class definition.
  - [ ] Move implementation to `src/semantic.cpp`.
  - [ ] Remove inline copy-pasted `SymbolTable` from `src/codegen.hpp` and `#include "semantic.hpp"` instead.

---

### 2. 🔤 Lexer Deficiencies (`src/lexer.l`)

- [ ] **Add source location tracking**:
  - [ ] Enable `%option yylineno` to track line numbers.
  - [ ] Add column number tracking and pass location information (`YYLTYPE` / `yylloc`) to Bison.
- [ ] **Add comment support**:
  - [ ] Support single-line comments (`// ...`).
  - [ ] Support multi-line block comments (`/* ... */`).
- [ ] **Add string literal support**:
  - [ ] Add regex for string literals (`\"([^\"\\]|\\.)*\"`).
  - [ ] Add `STRING_LITERAL` token to `parser.y` and return escaped string values.
- [ ] **Add missing operators & punctuation**:
  - [ ] Explicitly define `{`, `}`, `,`, `=` tokens instead of letting them fall through to the catch-all dot rule (`.`).
  - [ ] Compound assignment operators: `+=`, `-=`, `*=`, `/=`.
  - [ ] Increment and decrement operators: `++`, `--` (needed for standard for-loops).
  - [ ] Logical operators: `&&`, `||`, `!`.
  - [ ] Modulo operator: `%`.
  - [ ] Bitwise operators: `&`, `|`, `^`, `~`, `<<`, `>>`.
- [ ] **Lexer error handling**:
  - [ ] Replace silent fallthrough rule with proper error reporting for unexpected characters.

---

### 3. 🌲 Parser & Grammar Gaps (`src/parser.y`)

- [ ] **Implement For-Loop grammar**:
  - [ ] Token `TOK_FOR` exists but has no grammar rule. Add rule:
    ```yacc
    TOK_FOR '(' var_decl_or_expr ';' expression ';' assignment_or_expr ')' statement
    ```
- [ ] **Implement Function Definitions & Declarations**:
  - [ ] Grammar currently only allows a top-level list of statements inside an implicit `main()`.
  - [ ] Add grammar rules for defining functions:
    ```yacc
    type IDENTIFIER '(' parameter_list ')' block
    ```
  - [ ] Add parameter lists and empty parameter list support.
  - [ ] Support global variable declarations vs local function bodies.
- [ ] **Support more data types in grammar**:
  - [ ] Add tokens and type specifiers for `void`, `bool`, `float`, `char`, `string`.
- [ ] **Support void return**:
  - [ ] Allow `RETURN ';'` without requiring an expression.
- [ ] **Add unary expressions in grammar**:
  - [ ] Unary negation: `-expr`.
  - [ ] Logical NOT: `!expr`.
- [ ] **Add logical binary expressions**:
  - [ ] Logical AND: `expr && expr`.
  - [ ] Logical OR: `expr || expr`.
- [ ] **Parser error handling & recovery**:
  - [ ] Integrate line and column numbers into `yyerror(const char* s)`.
  - [ ] Print snippet or pointer to the line where syntax failed.
  - [ ] Add Bison `error` tokens to statement rules so parsing can recover and report multiple errors instead of halting on the first one.
- [ ] **Fix AST memory management during parsing**:
  - [ ] Avoid memory leaks when parse errors abort compilation before AST is attached to `root`.

---

### 4. 🧱 AST Architecture & Memory Management (`src/ast.hpp`, `src/ast.cpp`)

- [ ] **Unify codegen architecture**:
  - [ ] Resolve dual implementation: Currently code generation logic is duplicated between methods in `ast.cpp` (`Node::codegen`) and `codegen.cpp` (`CodeGenContext::codegen(Node*)`).
  - [ ] Decide on one pattern: either the Visitor pattern (clean separation of AST and backend) or virtual AST methods (direct dispatch), but do not mix both.
- [ ] **Add missing AST nodes**:
  - [ ] `ForStatement`: initializer, condition, step, body.
  - [ ] `FunctionDef`: return type, function name, parameter list, body block.
  - [ ] `Parameter`: type, name.
  - [ ] `UnaryExpr`: operator (`-`, `!`, `++`, `--`), operand.
  - [ ] `LogicalExpr`: `&&`, `||` with short-circuiting support.
  - [ ] `StringLiteral`: string value.
  - [ ] `ExprStatement`: already exists in `ast.hpp`, but missing in `CodeGenContext::codegen(Statement*)`.
- [ ] **Fix memory leaks in existing AST nodes**:
  - [ ] `FunctionCall`: `std::vector<Expression*>* args` is a raw pointer holding raw pointers. Destructor does not clean them up. Convert to `std::vector<std::unique_ptr<Expression>>`.
  - [ ] Ensure all container nodes use `std::unique_ptr` for child expressions and statements.

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

- [ ] **Fix loop jump statements (`break` / `continue`)**:
  - [ ] Maintain a stack of loop contexts (`std::vector<LoopContext> loopStack`) instead of a single `currentLoopEnd` / `currentLoopContinue` pointer.
  - [ ] Push loop exit and continue blocks before codegening loop bodies, and pop on exit.
  - [ ] Call `setCurrentLoopBlocks` in `WhileStatement::codegen`.
  - [ ] Implement codegen for `ForStatement`.
- [ ] **Fix Statement dispatch in `CodeGenContext::codegen(Statement*)`**:
  - [ ] Add handlers for `ExprStatement`, `Block`, `BreakStatement`, and `ContinueStatement`. Currently these return `nullptr` and fail the build if present at the top level or inside conditionals.
- [ ] **Fix multi-statement Block codegen**:
  - [ ] Handle blocks where a statement terminates execution (e.g. `return` in the middle of a block). Do not emit instructions into a closed basic block.
- [ ] **Implement user function code generation**:
  - [ ] Generate LLVM `Function` for user-defined functions with arguments allocated to local variables.
  - [ ] Retain `main()` as either explicit user function or default entry point.
- [ ] **Built-in runtime / I/O functions**:
  - [ ] Provide or declare built-in `print` / `printInt` / `printf` so test samples can print values and output results.
- [ ] **Fix comparison in `BinaryExpr`**:
  - [ ] Remove `case '=':` from `BinaryExpr::codegen` in `ast.cpp` (assignment should not be evaluated as equality comparison).

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

- [ ] **Fix existing broken test cases**:
  - [ ] `tests/test.minilang`: Uses string literals and `print(...)`, which fail. Either support strings/print or simplify test case.
  - [ ] `tests/test_input.minilang`: Uses `int main() { ... }`, which fails parser. Update to supported syntax or implement function defs.
  - [ ] `src/test_if.minilang`: Uses `print(...)`. Fix or replace.
- [ ] **Setup automated testing framework**:
  - [ ] Integrate Catch2 or a Python-based end-to-end test runner (`run_tests.py`).
  - [ ] Add unit tests for AST nodes and SymbolTable.
  - [ ] Add integration tests for:
    - [ ] Arithmetic evaluation and operator precedence.
    - [ ] If-else branching and nested conditionals.
    - [ ] While loops and nested while loops.
    - [ ] Variable scoping and shadowing.
    - [ ] Semantic error diagnostics (redeclaration, undeclared access).
- [ ] **Add negative test cases**:
  - [ ] Tests that assert compilation fails with appropriate error messages on invalid syntax or invalid types.

---

### 9. 🧹 Codebase Cleanup & Maintenance

- [ ] **Delete duplicate/junk files**:
  - [ ] Delete accidental typo file `,gitignore` in project root.
  - [ ] Consolidate empty/redundant forwarding header `include/ast.hpp`.
- [ ] **Reconcile outdated documentation**:
  - [ ] `ISSUE_1_RESOLUTION.md`, `TRACKER.md`, and `QA_REPORT.md` claim Issue #1 is completely resolved and verified, but the code was committed in a broken state. Reconcile docs to reflect real state.
  - [ ] Remove obsolete files or consolidate `DOCUMENTATION_INDEX.md`, `MAINTENANCE_SCHEDULE.md`, `CLEANUP_REPORT.md` into a single concise `README.md`.
