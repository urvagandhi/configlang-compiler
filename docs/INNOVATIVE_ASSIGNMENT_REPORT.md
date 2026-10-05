# NIRMA UNIVERSITY
## Institute of Technology | Department of Computer Science & Engineering
### B.Tech. Semester-VII | Academic Year 2026–27
### Course: Principles of Compiler Design (4CS501CC25)

---

# INNOVATIVE ASSIGNMENT REPORT
## **ConfigLang: A Typed Configuration Language Compiler**

**Submitted by:**
- **Urva Gandhi** (Roll No: `23BCE078`)
- **Rakshit Gajnotar** (Roll No: `23BCE077`)

**Faculty Evaluator**: Course Coordinator & Lab Faculty, PCD (4CS501CC25)

---

## 1. Problem Statement & Motivation

Modern software infrastructure relies heavily on configuration files (such as JSON or YAML). However, existing formats suffer from critical limitations:
1. **JSON** lacks comments, variable references, arithmetic evaluation, and compile-time type verification.
2. **YAML** is prone to subtle indentation bugs and unintended type coercions.
3. Errors in both formats are typically detected only at runtime during deployment, causing server downtime.

### Our Solution: ConfigLang
We developed **ConfigLang**, an educational yet robust compiler written from scratch in **C11**, **GNU Flex**, and **GNU Bison**. ConfigLang takes human-friendly `.cfg` files, executes all 8 classical compiler phases, evaluates expressions, validates types and references, optimizes arithmetic via constant folding, and compiles the configuration into standard **JSON**.

---

## 2. Mapping to Nirma University PCD Syllabus (Experiments 1–9)

| Lab Exp. | Practical Syllabus Topic | ConfigLang Implementation | Module / Location |
|:---:|:---|:---|:---|
| **Exp 1 & 2** | **Lexical Analyzer using Lex/Flex** | Scans identifiers, literals, keywords; strips comments; handles line numbering | `src/lexer/lexer.l` |
| **Exp 3** | **Symbol Table Generation** | Linked-list table storing identifier name, type, and evaluated values; duplicate checking | `src/symtab/symtab.c` |
| **Exp 4 & 5** | **Syntax Analyzer using YACC/Bison & AST** | LALR(1) context-free grammar with precedence rules; generates clean Abstract Syntax Tree | `src/parser/parser.y`<br>`src/parser/ast.c` |
| **Exp 6** | **Semantic Analyzer** | Two-pass analysis: type compatibility checking, undefined references, and cycle detection | `src/semantic/semantic.c` |
| **Exp 7** | **Three-Address Code (TAC) Generator** | Linearizes expression trees into quadruples (`LOAD`, `STORE`, `ADD`, `SUB`, `MUL`, `DIV`) | `src/tac/tac.c` |
| **Exp 8** | **Code Generation** | Emits clean, 2-space indented standard JSON from AST and symbol table | `src/codegen/codegen.c` |
| **Exp 9** | **Code Optimization Techniques** | Machine-independent optimization: Constant Propagation and compile-time Constant Folding | `src/optimizer/optimizer.c` |

---

## 3. Architecture of the 8 Compiler Phases

```
                     +-----------------------------------+
                     |      Source File (*.cfg)          |
                     +-----------------------------------+
                                       |
                                       v
   [Phase 1]          Lexical Analysis (Flex: src/lexer/lexer.l)
                      Tokenizes character stream into keywords, literals, and identifiers
                                       |
                                       v
   [Phase 2 & 3]      Syntax Analysis & AST (Bison: src/parser/parser.y, ast.c)
                      Constructs an Abstract Syntax Tree representing blocks and declarations
                                       |
                                       v
   [Phase 4]          Symbol Table Management (src/symtab/symtab.c)
                      Records declared variables and canonical nested names (server.port)
                                       |
                                       v
   [Phase 5]          Semantic Analysis (src/semantic/semantic.c)
                      Pass 1: Symbol registration & duplicate detection
                      Pass 2: Type checking, undefined references, and circular references
                                       |
                                       v
   [Phase 6]          Three-Address Code Generation (src/tac/tac.c)
                      Linearizes nested expressions into atomic 3-address quadruples
                                       |
                                       v
   [Phase 7]          Code Optimization (src/optimizer/optimizer.c)
                      Constant Propagation & compile-time Constant Folding (30 * 1000 -> 30000)
                                       |
                                       v
   [Phase 8]          Target Code Generation (src/codegen/codegen.c)
                      Traverses evaluated AST & Symbol Table to emit formatted JSON
```

---

## 4. Experimental Demonstration & Results

### 4.1 Sample Input Configuration (`examples/server.cfg`)
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

### 4.2 Phase-by-Phase Intermediate Outputs (`make demo`)
* **AST**: Successfully builds nested blocks for `server` and `database`.
* **Symbol Table**: Correctly registers dotted canonical names (`server.port`, `database.url`).
* **TAC**: Emits linear quadruples:
  ```
  t1 = LOAD @base_port
  t2 = t1 ADD 80
  STORE server.port = t2
  t3 = 30 MUL 1000
  STORE server.timeout_ms = t3
  ```
* **Optimization**: Constant folding precomputes `server.port = 8080`, `server.timeout_ms = 30000`, `server.max_connections = 1000`, and `database.url = "postgres://localhost/app"`.
* **Generated JSON**:
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

---

## 5. Automated Verification & Testing
The project includes an automated test runner script (`tests/run_tests.sh`) invoked via `make test`:
- **CLI Options**: Validates `--version`, `--help`, and graceful handling of missing files.
- **Golden Comparison**: Verifies that `examples/server.cfg` output matches `examples/server.json` byte-for-byte.
- **Valid Suite**: 14 valid configuration test cases covering comments, negative numbers, expressions, nested blocks, and string concatenations.
- **Negative Suite**: Tests error handling for syntax errors, undefined references, type mismatches, and circular references.
- **Result**: **24 out of 24 tests passing with zero failures.**

---

## 6. Conclusion
ConfigLang successfully demonstrates the practical implementation of an end-to-end compiler as part of the 7th-semester Principles of Compiler Design curriculum. By keeping the design modular, readable, and free of unnecessary industrial complexities, the project effectively highlights every core theoretical concept taught in the classroom.
