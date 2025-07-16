# MiniLang Compiler

A **minimal programming language compiler** written in **C++** using **Flex**, **Bison**, and **LLVM**.  
This project is built from scratch to explore and learn all stages of compiler design, including lexical analysis, parsing, semantic analysis, and code generation to native x86-64 assembly.

---

## 🧠 Motivation

The goal of this project is **deep understanding of compiler theory and implementation**, not just completing a working compiler. Each stage is implemented incrementally and thoroughly tested to reinforce concepts like:

- Tokenization with Flex (lexical analysis)
- Parsing with Bison (grammar + AST generation)
- AST representation in C++
- Semantic checking and transformations
- LLVM IR generation and native code emission

---

## 🛠 Features

- Simple C-like syntax: `int x = 1 + 2 * 3;`
- Arithmetic expressions (`+`, `-`, `*`, `/`)
- Comparison operators (`<`, `>`, `<=`, `>=`, `==`, `!=`)
- Variable declarations and assignments
- Control flow: `if`, `else`, `while`
- Return statements
- LLVM IR generation (via LLVM C++ API)
- Generates native x86-64 machine code

---


## 📄 Example

**Input (`test.minilang`)**:

```c
int x = 10;
x = x - 1;
if (x > 5) {
    return 1;
} else {
    return 0;
}
```

**Output**:

```
return 1;
```

---

## 🔧 Compilation Pipeline

```text
[ Source Code ]
      |
   [Lexer.l]   →  Flex     → Tokens
      |
   [Parser.y]  →  Bison    → AST
      |
   [AST.hpp]   →  C++ AST Nodes
      |
   [CodeGen]   →  LLVM IR Generation
      |
 [LLVM Backend] → Native Code
```

---

## 📦 Dependencies

* **C++17 / C++20**
* **Flex**
* **Bison**
* **LLVM** (tested with version 15+)
* **Catch2** (for unit testing)
* **CMake** (build system)

---

## ⚒️ Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Run CMake
cmake ..

# Build project
make

# Run
./minilang ../test.minilang
```

---

## 📝 Roadmap

* [x] Basic expressions and types
* [x] Variable declarations and assignments
* [x] Return statements
* [x] Control flow (if, else, while)
* [x] Comparison operators
* [x] Scoping and local variables
* [ ] Functions and parameters
* [ ] Type checking and semantic analysis
* [ ] Code optimization passes
* [ ] Full IR generation for control structures
* [ ] Emit `.ll` or `.s` output

---

## 📂 Project Structure

```text
├── CMakeLists.txt
├── README.md
├── src/
│   ├── lexer.l
│   ├── parser.y
│   ├── ast.hpp
│   ├── main.cpp
│   ├── codegen.cpp
│   └── codegen.hpp
├── test.minilang
└── build/
```