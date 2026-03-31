# Known Issues & Bug Tracker

**Last Updated:** March 31, 2026  
**Total Issues:** 12  
**Priority:** 5 High | 5 Medium | 2 Low

---

## 🔴 CRITICAL / BLOCKING

### Issue #1: Semantic Analysis Not Integrated
- **Status:** ⚠️ **CRITICAL - MUST FIX**
- **Priority:** 🔴 P0 (Blocks feature completeness)
- **Component:** `src/semantic.cpp` / `src/codegen.cpp`
- **Description:**
  - `src/semantic.cpp` implements `SymbolTable` class with scope management
  - Declared in CMakeLists as part of `minilang_lib`
  - BUT never instantiated or used in main pipeline
  - Results in: **NO type checking**, **NO redeclaration detection**
- **Current Behavior:**
  - Variables can be redeclared without error
  - No type matching between operations
  - Undefined variable access not caught
- **Expected Behavior:**
  - Reject redeclarations in same scope
  - Allow shadowing in inner scopes
  - Type check all operations
- **Impact:** High - semantic analysis phase is missing
- **Files Affected:**
  - `src/semantic.cpp` (implement)
  - `src/codegen.hpp` (add SymbolTable member)
  - `src/codegen.cpp` (use SymbolTable)
  - `src/main.cpp` (potentially)
- **Fix Difficulty:** Medium (2-3 hours)
- **Test Case Needed:**
  ```minilang
  int x = 5;
  int x = 10;  // Should error: redeclaration
  ```

---

### Issue #2: Error Messages Lack Location Info
- **Status:** ⚠️ **HIGH - Important for usability**
- **Priority:** 🔴 P1 (Usability)
- **Component:** `src/lexer.l` / `src/parser.y` / `src/main.cpp`
- **Description:**
  - Parser errors don't include line numbers
  - Error context is minimal
  - User has no way to find where error occurred
- **Current Output Example:**
  ```
  Error: syntax error at token 'if'
  ```
- **Expected Output Example:**
  ```
  Error on line 5: syntax error at token 'if'
  Expected: '{', found: 'if'
  5 | if (x > 5) {
      | ^^
  ```
- **Files Affected:**
  - `src/lexer.l` (track line/column)
  - `src/parser.y` (yyerror function)
  - `src/main.cpp` (error reporting)
- **Fix Difficulty:** Medium (2-3 hours)

---

## 🟠 HIGH PRIORITY

### Issue #3: No Error Recovery in Parser
- **Status:** ⚠️ **HIGH**
- **Priority:** 🟠 P1
- **Component:** `src/parser.y`
- **Description:**
  - Any parse error stops immediately
  - User gets ONE error at a time
  - Ideal: collect multiple errors, then report all
- **Current Limitation:** Single error reporting only
- **Files Affected:** `src/parser.y` (error productions)
- **Fix Difficulty:** High (3-4 hours)

### Issue #4: For-Loop Not Fully Implemented
- **Status:** ⚠️ **HIGH - Feature incomplete**
- **Priority:** 🟠 P1
- **Component:** `src/parser.y` / `src/ast.hpp` / `src/codegen.cpp`
- **Description:**
  - Lexer recognizes FOR keyword
  - Parser has NO for-loop grammar rules
  - AST has NO ForStatement node defined
  - Result: For-loops cannot be parsed at all
- **Current Behavior:**
  ```cpp
  // Parser will reject this
  for (int i = 0; i < 10; i++) {
      return i;
  }
  ```
- **Expected Behavior:**
  - Parser accepts for-loop syntax
  - Creates ForStatement AST node
  - CodeGen generates appropriate LLVM IR
- **Files Affected:**
  - `src/parser.y` (add for-loop grammar rules)
  - `src/ast.hpp` (define ForStatement struct)
  - `src/ast.cpp` (implement ForStatement)
  - `src/codegen.cpp` (implement codegen for ForStatement)
- **Fix Difficulty:** High (4-5 hours - complete feature)
- **Test Case:**
  ```minilang
  for (int i = 0; i < 5; i = i + 1) {
      return i;
  }
  ```

### Issue #5: No Function Declarations/Calls
- **Status:** ⚠️ **HIGH - Core feature missing**
- **Priority:** 🟠 P1
- **Component:** Entire pipeline
- **Description:**
  - Current implementation: only generates main() function
  - Cannot declare user functions
  - Cannot call functions
  - No function symbol resolution
- **Requested Feature:**
  ```minilang
  int double(int x) {
      return x * 2;
  }
  int main() {
      return double(21);
  }
  ```
- **Impact:** Very high - limits language expressiveness
- **Files Affected:** All pipeline files
- **Fix Difficulty:** High (6-8 hours)

### Issue #6: Limited Data Types
- **Status:** ⚠️ **HIGH - Core feature limited**
- **Priority:** 🟠 P1
- **Component:** `src/lexer.l` / `src/parser.y` / `src/codegen.cpp`
- **Description:**
  - Only `int` type supported
  - No float, double, bool, char, string
  - No type system infrastructure
- **Limitation:** Severely restricts what programs can do
- **Future Needs:**
  - Type inference
  - Type coercion
  - Type checking
- **Fix Difficulty:** High (4-6 hours for basic types)

### Issue #7: No Testing Framework
- **Status:** ⚠️ **HIGH - Maintenance risk**
- **Priority:** 🟠 P1
- **Component:** Project-wide
- **Description:**
  - Zero unit tests
  - Zero integration tests
  - Zero regression tests
  - Manual testing only (error-prone)
- **Current Test Count:** 0
- **Needed:** 20+ tests covering all features
- **Catch2 Available:** Yes, in CMakeLists but not used
- **Fix Difficulty:** Medium (depends on test count)

---

## 🟡 MEDIUM PRIORITY

### Issue #8: Include Directory Empty
- **Status:** ⚠️ **MEDIUM - Code organization**
- **Priority:** 🟡 P2
- **Component:** `include/` directory
- **Description:**
  - `include/` directory exists but is empty
  - All headers in `src/`
  - Standard practice: headers in include/
- **Current Structure:**
  ```
  src/
  ├── ast.hpp
  ├── codegen.hpp
  └── ...
  include/  ← UNUSED
  ```
- **Recommended:**
  ```
  include/
  ├── ast.hpp
  ├── codegen.hpp
  ├── semantic.hpp
  └── ast/
      ├── expression.hpp
      ├── statement.hpp
      └── program.hpp
  src/
  ├── ast.cpp
  ├── codegen.cpp
  └── main.cpp
  ```
- **Files Affected:** CMakeLists.txt, all source files (include paths)
- **Fix Difficulty:** Medium (organizing + path updates)

### Issue #9: Break/Continue Not Fully Implemented
- **Status:** ⚠️ **MEDIUM - Feature incomplete**
- **Priority:** 🟡 P2
- **Component:** `src/codegen.cpp`
- **Description:**
  - Lexer recognizes `break` and `continue`
  - Parser accepts them
  - AST nodes might exist
  - BUT: codegen likely incomplete
- **Test Case:**
  ```minilang
  while (x > 0) {
      if (x == 5) break;
      if (x == 3) continue;
      x = x - 1;
  }
  ```
- **Files Affected:**
  - `src/codegen.cpp` (BreakStatement, ContinueStatement codegen)
  - `src/codegen.hpp` (track loop contexts)
- **Fix Difficulty:** Medium (2-3 hours)

### Issue #10: Memory Leak Not Checked
- **Status:** ⚠️ **MEDIUM - Maintenance**
- **Priority:** 🟡 P2
- **Component:** Entire project
- **Description:**
  - No valgrind/sanitizer runs
  - No memory profiling
  - Using strdup() in parser without confirmed cleanup
  - AST node cleanup could have leaks
- **Needed:** Run memory checker regularly
- **Files Affected:** All files using dynamic memory
- **Fix Difficulty:** Low (analysis) / Medium (fixes if needed)

---

## 🔵 LOW PRIORITY / NICE-TO-HAVE

### Issue #11: No Debug Symbols
- **Status:** ℹ️ **LOW - Nice feature**
- **Priority:** 🔵 P3
- **Component:** `src/codegen.cpp`
- **Description:**
  - LLVM IR generated without debug metadata
  - Can't step through native code with debugger
  - Line number mapping missing
- **Files Affected:** `src/codegen.cpp` (add DWARF generation)
- **Fix Difficulty:** High (LLVM debug APIs)

### Issue #12: No Optimization Passes
- **Status:** ℹ️ **LOW - Performance feature**
- **Priority:** 🔵 P3
- **Component:** `src/main.cpp` / `src/codegen.cpp`
- **Description:**
  - LLVM IR generated but no optimization passes
  - Generated code likely suboptimal
  - Could add standard passes (DCE, CSE, inlining)
- **Files Affected:** `src/main.cpp` (pass manager)
- **Fix Difficulty:** High (LLVM pass pipeline)

---

## 📊 Issue Summary Matrix

### By Priority
| Priority | Count | Examples |
|----------|-------|----------|
| 🔴 Critical (P0) | 2 | Semantic analysis, Error messages |
| 🟠 High (P1) | 5 | For-loops, Functions, Testing |
| 🟡 Medium (P2) | 3 | Headers, Break/Continue, Memory leaks |
| 🔵 Low (P3) | 2 | Debug symbols, Optimization |

### By Status
| Status | Count |
|--------|-------|
| ⚠️ Open | 12 |
| 🆙 In Progress | 0 |
| ✅ Fixed | 0 |

### By Component
| Component | Issues |
|-----------|--------|
| Code Generation | 5 |
| Parser/Language | 3 |
| Project Organization | 2 |
| Testing/Quality | 2 |

---

## 🔧 Fix Roadmap

### Immediate (This Week)
- [ ] **Issue #1:** Integrate semantic analysis
- [ ] **Issue #2:** Add error location info
- [ ] **Issue #4:** Complete for-loop codegen

### Short-term (Next Week)
- [ ] **Issue #7:** Set up unit testing with Catch2
- [ ] **Issue #9:** Complete break/continue codegen
- [ ] **Issue #3:** Implement error recovery

### Medium-term (2-3 Weeks)
- [ ] **Issue #5:** Function declarations & calls
- [ ] **Issue #6:** Add more data types
- [ ] **Issue #8:** Reorganize include directories

### Long-term (Future)
- [ ] **Issue #10:** Regular memory profiling
- [ ] **Issue #11:** Debug symbols support
- [ ] **Issue #12:** Optimization passes

---

## 📝 How to Report New Issues

When discovering new issues:

1. **Title:** Clear one-liner description
2. **Priority:** 🔴 🟠  🟡 🔵 (choose one)
3. **Component:** Which file/module affected
4. **Description:** What's broken, how to reproduce
5. **Expected:** What should happen instead
6. **Impact:** How bad is it
7. **Files Affected:** Which source files need changes
8. **Difficulty:** Low/Medium/High estimate
9. **Test Case:** Example that triggers the issue

---

## ✅ Test Cases for Issues

### For Issue #1 (Semantic): Variable Redeclaration
```minilang
int x = 5;
int x = 10;  // ERROR: redeclaration
return x;
```

### For Issue #2 (Error Messages): Parse Error
```minilang
int x = ;  // ERROR: missing expression
```

### For Issue #4 (For-Loop): For Loop Execution
```minilang
for (int i = 0; i < 3; i = i + 1) {
    return i;
}
```

### For Issue #5 (Functions): Function Call
```minilang
int add(int a, int b) {
    return a + b;
}
return add(2, 3);
```

---

**Maintained by:** Developer  
**Next Review:** [Weekly]  
**Last Verified:** March 31, 2026
