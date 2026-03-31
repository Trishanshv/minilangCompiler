# Project Snapshot - Quick Reference

**Date:** March 31, 2026  
**Status:** 🚀 **In Development** | π = 50% Complete

---

## 🎯 One-Minute Summary

**MiniLang Compiler** is a C++/LLVM compiler for a minimal C-like language.

```
Source -> Lexer -> Parser -> AST -> [Semantic] -> CodeGen -> LLVM IR
([OK])    ([OK])    ([OK])   ([OK])  ([MISSING])  ([OK])     ([OK])
```

**Current:** Lexing, parsing, and basic codegen work. Semantic analysis not integrated.

---

## 📊 Quick Stats

| Metric | Value | Status |
|--------|-------|--------|
| Total Lines of Code | ~800 | - |
| Generated Code | ~1,500+ | (Flex/Bison generated) |
| Files | 9 source | [DONE] Core complete |
| Build System | CMake | [DONE] Working |
| Tests | 0 unit | [TODO] Need 20+ |
| Features | 7/15 | 47% complete |
| Bugs | 12 known | [TODO] See ISSUES.md |

---

## What Works

[DONE] Lexical analysis (Flex)
[DONE] Parsing (Bison) - constructs AST
- ✅ Variable declarations: `int x = 5;`
- ✅ Arithmetic: `+`, `-`, `*`, `/`
- ✅ Comparisons: `==`, `!=`, `<`, `>`, `<=`, `>=`
- ✅ Control flow: `if/else`, `while`
- ✅ Return statements: `return 42;`
- ✅ LLVM IR generation
- ✅ Build configuration

---

## What Needs Work

[TODO] **Semantic analysis** - not integrated yet
- ⚠️ **For-loop** - parsed but not codegened
- ⚠️ **Function calls** - not supported
- ⚠️ **Error messages** - no line numbers
- ⚠️ **Break/Continue** - incomplete
- ⚠️ **Testing** - no automated tests
- ⚠️ **Type system** - only `int` supported

---

## 📁 File Organization

```
src/                    (Implementation)
├── lexer.l            (40 lines) ✅
├── parser.y           (100 lines) ✅
├── ast.hpp            (150 lines) ✅
├── ast.cpp            (150 lines) ✅
├── codegen.hpp        (50 lines) ✅
├── codegen.cpp        (200 lines) ✅
├── semantic.cpp       (50 lines) ⚠️ Not used
└── main.cpp           (30 lines) ✅

tests/                  (Samples)
├── test.minilang      (1 line)
├── test_if.minilang   (?)
└── test_input.minilang

build/                  (Artifacts)
├── parser.cpp         (Generated)
├── lexer.cpp          (Generated)
└── CMakeCache.txt     (Configured)

Documentation:
├── README.md          (Project info)
├── TRACKER.md         (This project! Overview)
├── DEVELOPMENT.md     (Dev guidelines)
├── BUILD_STATUS.md    (Build details)
├── ISSUES.md          (Bug tracker)
└── PROJECT_SNAPSHOT.md (This file)
```

---

## 🔄 Development Workflow

### Adding a Feature (5-Step Process)

```
1. Update Lexer        (lexer.l) - Add token
   ↓
2. Update Parser       (parser.y) - Add grammar rule
   ↓
3. Update AST          (ast.hpp + ast.cpp) - Add node class
   ↓
4. Update CodeGen      (codegen.cpp) - Add IR generation
   ↓
5. Test                (test.minilang) - Verify it works
```

### Building

```bash
cd build
cmake ..              # (only if CMakeLists changed)
make                  # Build
./minilang test.minilang  # Run
```

---

## 🧪 Running Tests

```bash
cd build
./minilang ../test.minilang           # Test 1: Simple return
./minilang ../test_if.minilang        # Test 2: If/else
./minilang ../tests/test_input.minilang  # Test 3: Other
```

**Expected Output:** LLVM IR code (starts with `; ModuleID = 'main'`)

---

## 🔴 CRITICAL Issues (Must Fix)

| # | Issue | Impact | Time |
|---|-------|--------|------|
| 1 | Semantic analysis not integrated | No type checking | 2-3 hrs |
| 2 | Error messages lack line numbers | User confusion | 2-3 hrs |
| 4 | For-loop codegen missing | Feature broken | 2 hrs |

---

## 🎓 Learning Resources

- **Understanding the Pipeline:** See DEVELOPMENT.md
- **Build Details:** See BUILD_STATUS.md
- **Known Issues:** See ISSUES.md
- **Full Documentation:** See README.md

---

## 📋 Checklist for New Contributors

- [ ] Read README.md (project overview)
- [ ] Read DEVELOPMENT.md (architecture & workflow)
- [ ] Try building: `cd build && cmake .. && make`
- [ ] Run a test: `./minilang ../test.minilang`
- [ ] Review ISSUES.md (pick a task)
- [ ] Make changes following workflow steps
- [ ] Update TRACKER.md when done

---

## 📞 Questions?

**"How do I build?"** → See DEVELOPMENT.md section "Building"  
**"What's broken?"** → See ISSUES.md for all 12 known issues  
**"What should I work on?"** → See ISSUES.md "Fix Roadmap"  
**"How do I add a feature?"** → See DEVELOPMENT.md "Adding New Feature"  
**"Where's the architecture?"** → See DEVELOPMENT.md "Compiler Architecture"

---

## 🚀 Next Milestones

### v0.1.0 (Current - Alpha)
- [x] Lexer complete
- [x] Parser complete
- [x] AST complete
- [ ] Semantic analysis (in progress)

### v0.2.0 (Next Sprint)
- [ ] Semantic analysis integrated
- [ ] Comprehensive error messages
- [ ] For-loop codegen
- [ ] Test suite (20+ tests)

### v0.3.0 (Future)
- [ ] Function declarations & calls
- [ ] Breaking/Continue statements
- [ ] Multiple data types

### v1.0.0 (Goal)
- Full feature language
- Comprehensive test suite
- Production-ready compiler

---

**Status:** 🚀 Making steady progress  
**Maintenance:** Weekly reviews  
**Next Update:** April 7, 2026
