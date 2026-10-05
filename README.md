# ConfigLang: A Simple Typed Configuration Language Compiler

[![Language: C11](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Tools: Flex & Bison](https://img.shields.io/badge/Tools-Flex%20%7C%20Bison-orange.svg)](https://www.gnu.org/software/bison/)
[![Build: GNU Make](https://img.shields.io/badge/Build-GNU%20Make-brightgreen.svg)](https://www.gnu.org/software/make/)

**ConfigLang** is an educational, end-to-end configuration language compiler built from scratch in C11 using **Flex** and **Bison**. It translates human-friendly, strongly-typed configuration files (`.cfg`) into standard **JSON**.

Developed as the **Innovative Assignment** for **Principles of Compiler Design (PCD - 4CS501CC25)**, Semester-VII B.Tech CSE at **Nirma University**.

---

## 👥 Authors & Team
- **Urva Gandhi** (Roll No: `23BCE078`)
- **Rakshit Gajnotar** (Roll No: `23BCE077`)

---

## 🎯 Project Design Philosophy
- **Simple > Complete > Industry-Level**: Designed specifically as a clear, student-level compiler project that is easy to understand, debug, demonstrate, and explain during viva examinations.
- **Clear 8-Phase Pipeline**: Every classical compiler phase lives in its own dedicated, clean directory with step-by-step comments.
- **Zero Heavy Abstractions**: Standard C11 memory management, no complex arena allocators, no complicated dependency graphs, and no confusing reentrant scanner pointers.

---

## 🏗️ Compiler Architecture & Pipeline

```
                     +-----------------------------------+
                     |      Source Code (*.cfg)          |
                     +-----------------------------------+
                                       |
                                       v
   [Phase 1]          Lexical Analysis (Flex: src/lexer/lexer.l)
                      Converts character stream into tokens (IDENT, INT, STRING, etc.)
                                       |
                                       v
   [Phase 2 & 3]      Syntax Analysis & AST (Bison: src/parser/parser.y, ast.c)
                      LALR(1) parsing builds the Abstract Syntax Tree
                                       |
                                       v
   [Phase 4]          Symbol Table Management (src/symtab/symtab.c)
                      Simple linked list storing variables, types, and values
                                       |
                                       v
   [Phase 5]          Semantic Analysis (src/semantic/semantic.c)
                      Two-pass validation: type checking, undeclared/duplicate checks
                                       |
                                       v
   [Phase 6]          Three-Address Code Generation (src/tac/tac.c)
                      Linear quadruples: LOAD, STORE, ADD, SUB, MUL, DIV
                                       |
                                       v
   [Phase 7]          Code Optimization (src/optimizer/optimizer.c)
                      Compile-time Constant Propagation & Constant Folding
                                       |
                                       v
   [Phase 8]          Target Code Generation (src/codegen/codegen.c)
                      Traverses evaluated AST & Symbol Table to emit formatted JSON
```

---

## 📂 Source Code Layout

| Directory / File | Description | Lines of Code |
|---|---|---|
| `src/common.h` | Shared data types (`int`, `float`, `string`, `bool`) | ~35 lines |
| `src/lexer/lexer.l` | Flex scanner recognizing keywords, literals, and comments | ~70 lines |
| `src/parser/parser.y` | Bison LALR(1) grammar for declarations, blocks, expressions | ~110 lines |
| `src/parser/ast.c` | Abstract Syntax Tree node constructors and indented printer | ~95 lines |
| `src/symtab/symtab.c` | Linked-list Symbol Table with `insert`, `lookup`, and table print | ~95 lines |
| `src/semantic/semantic.c` | 2-pass semantic validator (type check, duplicate & cycle checks) | ~140 lines |
| `src/tac/tac.c` | Three-Address Code generation and quadruple printer | ~125 lines |
| `src/optimizer/optimizer.c` | Constant propagation and compile-time arithmetic folding | ~180 lines |
| `src/codegen/codegen.c` | JSON code generator emitting indented JSON output | ~75 lines |
| `src/main.c` | Driver program orchestrating the 8 pipeline phases | ~170 lines |

---

## 🚀 Quick Start Guide

### Prerequisites
Install standard compiler build tools:
```bash
sudo apt update
sudo apt install -y build-essential flex bison
```

### 1. Build the Compiler
```bash
make clean && make
```
The compiled executable is created at `build/configlang`.

### 2. Run the Full 8-Phase Demonstration
```bash
make demo
```
This executes `./build/configlang --phases examples/server.cfg` and prints the intermediate output of every phase (AST, Symbol Table, Raw TAC, Optimized TAC, and Final JSON).

### 3. Run the Automated Test Suite
```bash
make test
```
Runs 24 automated test cases covering CLI flags, valid configs, golden JSON verification, and error detection.

---

## 💻 Usage & CLI Options

```bash
# Basic compilation (prints JSON to terminal)
./build/configlang examples/server.cfg

# Compile and save JSON to a file
./build/configlang -o output.json examples/server.cfg

# Display all 8 compiler pipeline phases step-by-step
./build/configlang --phases examples/server.cfg

# Show help message
./build/configlang --help

# Show version
./build/configlang --version
```

---

## 📝 Example

### Input Configuration (`examples/server.cfg`)
```cfg
app_name : string = "ConfigLang Demo";
base_port : int = 8000;

server {
    port : int = @base_port + 80;
    workers : int = 4;
    timeout_ms : int = 30 * 1000;
    max_connections : int = @server.workers * 250;
    debug : bool = false;
}

database {
    host : string = "localhost";
    port : int = 5432;
    url : string = "postgres://" + @database.host + "/app";
}
```

### Output JSON (`build/server.json`)
```json
{
  "app_name": "ConfigLang Demo",
  "base_port": 8000,
  "server": {
    "port": 8080,
    "workers": 4,
    "timeout_ms": 30000,
    "max_connections": 1000,
    "debug": false
  },
  "database": {
    "host": "localhost",
    "port": 5432,
    "url": "postgres://localhost/app"
  }
}
```

Notice how `@base_port + 80` was folded to `8080`, `30 * 1000` to `30000`, and strings concatenated to `"postgres://localhost/app"` at compile time!

---

## 🎓 Mapping to PCD Syllabus (Nirma University)

| Experiment | Syllabus Topic | Implementation in ConfigLang |
|---|---|---|
| **Exp 1 & 2** | Lexical Analysis using Flex | `src/lexer/lexer.l` |
| **Exp 3** | Symbol Table Management | `src/symtab/symtab.c` & `symtab.h` |
| **Exp 4 & 5** | Syntax Analysis using Bison & AST | `src/parser/parser.y` & `src/parser/ast.c` |
| **Exp 6** | Semantic Analysis & Type Checking | `src/semantic/semantic.c` |
| **Exp 7** | Intermediate Code Generation (TAC) | `src/tac/tac.c` & `src/tac/tac.h` |
| **Exp 8** | Code Generation | `src/codegen/codegen.c` (JSON emitter) |
| **Exp 9** | Code Optimization | `src/optimizer/optimizer.c` (Constant Folding & Propagation) |
