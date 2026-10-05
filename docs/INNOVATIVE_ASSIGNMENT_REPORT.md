# NIRMA UNIVERSITY
## Institute of Technology | Department of Computer Science & Engineering
### B.Tech. Semester-VII | Academic Year 2026–27
### Course: Principles of Compiler Design (4CS501CC25)

---

# INNOVATIVE ASSIGNMENT REPORT
## **ConfigLang: A Typed Configuration Language Compiler with Semantic Validation**

**Submitted by:**
- **Urva Gandhi** (Roll No: `23BCE078`)
- **Rakshit Gajnotar** (Roll No: `23BCE077`)

**Faculty Evaluator**: Course Coordinator & Lab Faculty, PCD (4CS501CC25)

---

## 1. Problem Statement & Motivation

Modern software systems depend on configuration formats such as JSON and YAML. However:
1. **JSON** has no comments, no variable references, no arithmetic expressions, and no type/range constraints.
2. **YAML** is prone to subtle indentation bugs and type confusion.
3. In both formats, syntax and semantic validation only happen at runtime when an application crashes.

### Our Solution: ConfigLang
We built **ConfigLang**, an educational yet robust compiler written from scratch in **C11**, **Flex 2.6**, and **Bison 3.8** (with GNU Make and zero external dependencies). It accepts strongly-typed, human-friendly `.cfg` files, performs full compiler analysis (lexing, parsing, symbol tables, type checking, cycle detection, three-address code generation, constant folding optimization, and range validation), and translates valid configurations into standard **JSON** and **YAML**.

---

## 2. Direct Mapping to Nirma University PCD Practical List (LOP)

This innovative project directly integrates and showcases all practical experiments from our **PCD Lab Syllabus (4CS501CC25)**:

| Lab Exp. No. | Practical Title from Course Syllabus | Corresponding ConfigLang Module | Key Implementation Features |
|:---:|:---|:---|:---|
| **Exp 1 & 2** | **Lexical Analyzer using Lex/Flex**<br>*(Recognize tokens, eliminate comments, report line numbers)* | `src/lexer/lexer.l`<br>`src/lexer/lexer.h` | Flex regular expressions recognize keywords (`int`, `float`, `string`, `bool`), literals, identifiers, and operators. Discards `//` and `/* ... */` comments. Reports lexical errors (`E101`–`E105`) with exact line and column numbers. |
| **Exp 3** | **Symbol Table Generation**<br>*(Store identifier name, data type, address/scope, check uniqueness)* | `src/symtab/symtab.c`<br>`src/symtab/symtab.h` | Implements a hierarchical scope tree preserving nested blocks (`server.port`) combined with a hash table for fast lookup. Rejects duplicate declarations (`E301`) in the same scope. |
| **Exp 4** | **Syntax Analyzer using YACC/Bison**<br>*(CFG definition and AST generation)* | `src/parser/parser.y`<br>`src/parser/ast.c` | Defines an unambiguous Context-Free Grammar (CFG) for declarations, blocks, and expressions. Bison generates a conflict-free LALR(1) parser (`%expect 0`) that builds an Abstract Syntax Tree (AST). |
| **Exp 5** | **Error Recovery in Syntax Analyzer**<br>*(Detect and report syntax errors with recovery)* | `src/parser/parser.y` | Employs panic-mode syntax error recovery using the `error SEMI` rule. When a syntax error occurs, it reports error `E201`, skips to the synchronizing semicolon, and continues parsing subsequent statements. |
| **Exp 6** | **Semantic Analyzer**<br>*(Type checking, undeclared variables, duplicate declarations)* | `src/semantic/semantic.c`<br>`src/semantic/validator.c` | Performs two-pass semantic analysis: validates types (`int`, `float`, `string`, `bool`), resolves `@references`, detects undeclared references (`E305`), and detects circular reference cycles (`E306`) via Depth-First Search (DFS) graph traversal. |
| **Exp 7** | **Three-Address Code (TAC) Generator**<br>*(Generate intermediate code representation)* | `src/tac/tac.c`<br>`src/tac/tac.h` | Linearizes high-level expressions into standard Three-Address Code quadruples (`LOAD`, `STORE`, `ADD`, `SUB`, `MUL`, `DIV`, `NEG`) in topological dependency order. |
| **Exp 8** | **Code Generation**<br>*(Translate intermediate code to target format)* | `src/codegen/codegen.c`<br>`src/codegen/codegen.h` | Traverses the evaluated symbol table to emit clean, formatted, 2-space indented standard JSON and YAML. |
| **Exp 9** | **Code Optimization Techniques**<br>*(Improve efficiency of intermediate code)* | `src/optimizer/optimizer.c`<br>`src/optimizer/optimizer.h` | Implements two classic machine-independent optimization techniques: **Constant Propagation** and **Constant Folding** (with checked arithmetic for division-by-zero `E308` and integer overflow `E310`), plus dead temporary elimination. |

---

## 3. Modular Project Architecture

The source code in `src/` is cleanly modularized according to the compiler pipeline stages:

```
src/
├── main.c                   # Compiler CLI driver & 8-phase pipeline orchestrator
├── common/                  # Memory & error utilities (Arena allocator, diagnostics, strbuf)
├── lexer/                   # [Exp 1 & 2] Lexical Analyzer (Flex pure scanner)
├── parser/                  # [Exp 4 & 5] Syntax Analyzer, AST & Error Recovery (Bison LALR-1)
├── symtab/                  # [Exp 3] Symbol Table Management (hierarchical scope + hash map)
├── semantic/                # [Exp 6] Semantic Analyzer & Range Validator
├── tac/                     # [Exp 7] Three-Address Code (TAC) Generator
├── optimizer/               # [Exp 9] Code Optimizer (Constant Propagation & Folding)
└── codegen/                 # [Exp 8] Code Generator (JSON & YAML output)
```

---

## 4. Worked Example: Input to Output

### 4.1 Input Source (`examples/server.cfg`)
```cfg
app_name : string = "ConfigLang Demo";
base_port : int(1024..65535) = 8000;

server {
    port : int(1..65535) = @base_port + 80;
    workers : int(1..64) = 4;
    timeout_ms : int = 30 * 1000;
    max_connections : int = @server.workers * 250;
    debug : bool = false;
}

database {
    host : string = "localhost";
    port : int(1..65535) = 5432;
    url : string = "postgres://" + @database.host + "/app";
}
```

### 4.2 Generated Three-Address Code (TAC) - Before Optimization (`--tac`)
```
STORE app_name, "ConfigLang Demo"
STORE base_port, 8000
t1 = LOAD base_port
t2 = t1 ADD 80
STORE server.port, t2
STORE server.workers, 4
t3 = 30 MUL 1000
STORE server.timeout_ms, t3
t4 = LOAD server.workers
t5 = t4 MUL 250
STORE server.max_connections, t5
STORE server.debug, false
STORE database.host, "localhost"
STORE database.port, 5432
t6 = LOAD database.host
t7 = "postgres://" ADD t6
t8 = t7 ADD "/app"
STORE database.url, t8
```

### 4.3 Optimized Three-Address Code (`--tac-opt`)
```
STORE app_name, "ConfigLang Demo"
STORE base_port, 8000
STORE server.port, 8080
STORE server.workers, 4
STORE server.timeout_ms, 30000
STORE server.max_connections, 1000
STORE server.debug, false
STORE database.host, "localhost"
STORE database.port, 5432
STORE database.url, "postgres://localhost/app"
; before: 18 instructions, after: 10 | folds: 5 | propagations: 11 | removed: 8
```

### 4.4 Final Target Code: JSON Output (`build/configlang examples/server.cfg`)
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

### 4.5 Final Target Code: YAML Output (`build/configlang -f yaml examples/server.cfg`)
```yaml
app_name: "ConfigLang Demo"
base_port: 8000
server:
  port: 8080
  workers: 4
  timeout_ms: 30000
  max_connections: 1000
  debug: false
database:
  host: "localhost"
  port: 5432
  url: "postgres://localhost/app"
```

---

## 5. Verification & Testing

Our compiler was verified using an automated bash test suite (`tests/run_tests.sh`):
1. **79 Automated Test Assertions Passed (0 Failed)**:
   - **Valid Configuration Cases (14 tests)**: Primitive types, arithmetic precedence, string concatenation, scoped variable references, comments, escape sequences, constant folding, and 100-level block nesting.
   - **Negative Diagnostic Cases (35 tests)**: Verifies exact line number, column number, and error code for all errors (`E101`–`E310`).
   - **Stress & Fuzz Testing**: 30 random byte fuzz test iterations survived without crashes.
2. **Memory Safety (Valgrind)**:
   - Command: `make memcheck`
   - Result: **0 memory errors, 0 bytes leaked** across all test runs.

---

## 6. How to Run the Demonstration

```bash
# 1. Compile the project
make clean && make

# 2. Run the 8-phase pipeline demonstration on server.cfg
make demo

# 3. Run the automated test suite
make test

# 4. Run Valgrind memory leak verification
make memcheck
```

---

## 7. Viva Q&A Cheat Sheet (Quick Reference for Students)

### Q1: What tools are used in this project?
> **Answer**: Standard C11, GNU Flex (for lexical analysis), GNU Bison (for LALR(1) syntax analysis), and GNU Make. No external third-party libraries were used.

### Q2: How does the Lexical Analyzer work?
> **Answer**: In `src/lexer/lexer.l`, Flex matches regular expressions to recognize tokens (keywords, literals, identifiers, symbols). It skips whitespace and comments, and tracks line and column numbers using `YY_USER_ACTION`.

### Q3: How is the Symbol Table structured?
> **Answer**: In `src/symtab/symtab.c`, the symbol table is a tree of scopes (to preserve nested blocks like `server.port`) combined with a hash table for fast lookups.

### Q4: How does error recovery work in the parser?
> **Answer**: In `src/parser/parser.y`, we use Bison's `error SEMI` rule. When an error occurs in a statement, Bison discards tokens until it finds a semicolon `;` and then resumes parsing normal statements.

### Q5: What semantic checks are performed?
> **Answer**: In `src/semantic/`, we check for duplicate variable declarations in the same scope (`E301`), type mismatches (`E302`), undeclared references (`E305`), circular references (`E306`), and values exceeding declared numeric ranges (`E303`).

### Q6: What optimizations are performed?
> **Answer**: In `src/optimizer/optimizer.c`, we perform **Constant Folding** (computing expressions like `30 * 1000` at compile-time) and **Constant Propagation** (replacing references to known constants with their values).

### Q7: How does memory management work?
> **Answer**: In `src/common/arena.c`, we use an Arena Allocator that allocates memory in 64 KB chunks. When the compiler finishes, all memory is freed at once in `arena_destroy()`, preventing memory leaks (0 leaks in Valgrind).
