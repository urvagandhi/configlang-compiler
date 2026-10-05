# ConfigLang: Code Explanation & Viva Cheat Sheet
**Principles of Compiler Design (4CS501CC25) — Semester VII, B.Tech CSE, Nirma University**  
**Authors**: Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)

---

## 1. Quick Project Summary (30-Second Viva Pitch)
> *"ConfigLang is an educational typed configuration language compiler built from scratch in C11 using Flex and Bison. It takes a typed configuration file (`.cfg`), runs it sequentially through the core compiler phases: Lexical Analysis, Syntax Analysis (LALR(1) Parsing), AST Construction, Symbol Table Management, Semantic Analysis, Three-Address Code (TAC) Generation, Code Optimization (Constant Folding & Propagation), and Target Code Generation (JSON). It directly demonstrates Experiments 1 through 9 of our Nirma University PCD syllabus."*

---

## 2. Architecture Map: How `src/` Maps to Compiler Phases

```
                  +-----------------------------------+
                  |  Source File (.cfg)               |
                  +-----------------------------------+
                                    |
                                    v
[Phase 1]       src/lexer/lexer.l       (Flex Scanner)
                Exp 1 & 2: Tokenizes input into INT_LIT, IDENT, STRING_LIT, keywords
                                    |
                                    v
[Phase 2 & 3]   src/parser/parser.y     (Bison Parser + AST in src/parser/ast.c)
                Exp 4 & 5: Parses grammar & builds Abstract Syntax Tree
                                    |
                                    v
[Phase 4]       src/symtab/symtab.c     (Symbol Table)
                Exp 3: Populates linked-list table storing names, types, and values
                                    |
                                    v
[Phase 5]       src/semantic/semantic.c (Semantic Analyzer)
                Exp 6: Validates types, checks undefined variables & duplicate symbols
                                    |
                                    v
[Phase 6]       src/tac/tac.c           (Three-Address Code Generator)
                Exp 7: Converts expressions into quadruples: LOAD, STORE, ADD, MUL
                                    |
                                    v
[Phase 7]       src/optimizer/optimizer.c (Constant Folding & Propagation)
                Exp 9: Evaluates 30*1000->30000 at compile time & propagates constants
                                    |
                                    v
[Phase 8]       src/codegen/codegen.c   (Target Code Generator)
                Exp 8: Traverses evaluated AST & Symbol Table to emit formatted JSON
```

---

## 3. Detailed File Walkthrough & Key Implementation Details

### `src/common.h` — Shared Type Definitions
- **What it does**: Defines the `DataType` enum (`TYPE_INT`, `TYPE_FLOAT`, `TYPE_STRING`, `TYPE_BOOL`, `TYPE_UNKNOWN`) and the `type_name()` helper.
- **Why it matters**: Shared across all phases so that the lexer, parser, symbol table, semantic checker, and codegen use the exact same type representation.

---

### `src/lexer/lexer.l` — Lexical Analysis (Exp 1 & 2)
- **Tool**: Flex (Fast Lexical Analyzer Generator).
- **Key Concepts**:
  - Regular expressions match keywords (`int`, `float`, `string`, `bool`, `true`, `false`).
  - Integer literals: `[0-9]+` (uses `atoi`).
  - Float literals: `[0-9]+\.[0-9]+` (uses `atof`).
  - String literals: `\"([^"\\]|\\.)*\"` (supports escaped quotes and backslashes).
  - Identifiers and dotted access: `[a-zA-Z_][a-zA-Z0-9_]*(\.[a-zA-Z_][a-zA-Z0-9_]*)*`.
  - Comments: Single-line `//...` and multi-line C-style `/* ... */`.
  - Automatic line numbering via `%option yylineno`.
- **Viva Questions & Answers**:
  - *Q: How does Flex know when a token ends?*  
    *A: Flex follows the Maximal Munch (Longest Match) rule: it matches the longest sequence of characters that satisfies any regular expression.*
  - *Q: How do you handle line numbers for error reporting?*  
    *A: We use `%option yylineno`, which instructs Flex to automatically maintain the global variable `yylineno` every time a newline character is encountered.*

---

### `src/parser/parser.y` & `src/parser/ast.c` — Syntax Analysis & AST (Exp 4 & 5)
- **Tool**: Bison (LALR(1) Parser Generator).
- **Key Concepts**:
  - Grammar rules define programs as a list of statements (`decl` or nested `block`).
  - Precedence declarations resolve operator ambiguity without conflicts:
    ```yacc
    %left '+' '-'
    %left '*' '/'
    %nonassoc UMINUS
    ```
  - Semantic actions (`{ $$ = ... }`) construct AST nodes dynamically using `ast_alloc()`.
  - Node types:
    - `NODE_BLOCK`: Represents nested scopes (e.g. `server { ... }`).
    - `NODE_DECL`: Variable declarations (`name : type = expr;` or `name = expr;`).
    - `NODE_BINARY`: Binary operators (`+`, `-`, `*`, `/`).
    - `NODE_INT`, `NODE_FLOAT`, `NODE_STRING`, `NODE_BOOL`: Literal leaves.
    - `NODE_REF`: Reference to another variable (`@variable_name`).
- **Viva Questions & Answers**:
  - *Q: What parsing algorithm does Bison use?*  
    *A: Bison generates an LALR(1) (Look-Ahead Left-to-right, Rightmost derivation) bottom-up parser using shift-reduce tables.*
  - *Q: Why do we build an Abstract Syntax Tree (AST) instead of evaluating directly in parser actions?*  
    *A: Decoupling parsing from evaluation allows multi-pass semantic analysis (checking forward references), intermediate code generation (TAC), and independent code optimization.*

---

### `src/symtab/symtab.c` — Symbol Table Management (Exp 3)
- **Structure**: A clean linked list of `Symbol` structures.
- **Fields**:
  ```c
  typedef struct Symbol {
      char name[64];      // e.g. "port" or "server.port"
      DataType type;      // int, float, string, bool
      int ival;           // evaluated int value
      double fval;        // evaluated float value
      char sval[128];     // evaluated string value
      bool bval;          // evaluated bool value
      bool is_assigned;   // whether value is resolved
      struct Symbol *next;
  } Symbol;
  ```
- **Functions**:
  - `symtab_init()`: Cleans and resets the table.
  - `symtab_insert(name, type)`: Adds a new variable.
  - `symtab_lookup(name)`: Searches the table by name in $O(n)$ time.
  - `symtab_print()`: Prints a clean ASCII table showing `NAME`, `TYPE`, `STATUS`, and `VALUE`.
- **Viva Questions & Answers**:
  - *Q: What is the purpose of the Symbol Table?*  
    *A: It acts as the central dictionary of the compiler, preserving identifiers, their declared/inferred data types, and their computed values across semantic analysis, optimization, and code generation.*

---

### `src/semantic/semantic.c` — Semantic Analysis (Exp 6)
- **How it Works (2 Clean Passes)**:
  1. **Pass 1 (`pass1_register_symbols`)**:
     - Traverses the AST.
     - Detects duplicate variable declarations within the same scope.
     - Registers all variable names and declared types in the Symbol Table.
  2. **Pass 2 (`pass2_check_expressions`)**:
     - Resolves `@referenced` variables.
     - Flags **Undefined References** if a referenced variable does not exist in the Symbol Table.
     - Flags **Circular References** if a variable attempts to reference itself (e.g. `a = @a;`).
     - Performs **Type Checking**:
       - Ensures numbers only add/multiply with numbers.
       - Allows string concatenation via `+`.
       - Enforces type matching between declared variable type and expression value.
- **Viva Questions & Answers**:
  - *Q: Why are two passes required for semantic analysis?*  
    *A: Two passes allow forward references. Pass 1 discovers all declared variables first. Pass 2 can then resolve references to variables declared later in the file without encountering false "undefined reference" errors.*

---

### `src/tac/tac.c` — Three-Address Code Generation (Exp 7)
- **Concept**: Lowers nested AST expressions into linear quadruples with at most 3 addresses:
  ```
  Result = Arg1 OP Arg2
  ```
- **Instruction Set**:
  - `LOAD`: Loads a referenced variable into a temporary (`t1 = LOAD @port`).
  - `STORE`: Assigns a computed value or temporary into a variable (`STORE server.port = t2`).
  - `ADD`, `SUB`, `MUL`, `DIV`: Binary arithmetic on temporaries/literals (`t3 = 30 MUL 1000`).
- **Temporary Generation**: Generates sequential temporary labels `t1`, `t2`, `t3`, ...
- **Viva Questions & Answers**:
  - *Q: Why do we generate Three-Address Code instead of targeting machine code directly?*  
    *A: TAC serves as a machine-independent intermediate representation (IR). It linearizes expression trees so that optimizations (like constant folding and common subexpression elimination) can be performed uniformly.*

---

### `src/optimizer/optimizer.c` — Code Optimization (Exp 9)
- **Two Classical Compiler Optimizations**:
  1. **Constant Propagation**:
     - Tracks known constant values for variables and temporaries.
     - When an operand is a variable whose constant value is known (e.g. `base_port = 8000`), the optimizer replaces the variable use with `8000`.
  2. **Constant Folding**:
     - When both operands of an arithmetic instruction are constant values, the operation is computed at compile time:
       - `30 MUL 1000`  -->  `30000`
       - `8000 ADD 80`  -->  `8080`
       - `"postgres://" ADD "localhost"`  -->  `"postgres://localhost"`
     - The intermediate temporary instruction is removed from the TAC instruction list!
- **Viva Questions & Answers**:
  - *Q: What is the difference between Constant Folding and Constant Propagation?*  
    *A: Constant Propagation substitutes known constant values into variable uses (`t1 = LOAD @x` becomes `t1 = 10`). Constant Folding executes the operation between two constants at compile time (`10 + 20` becomes `30`), eliminating runtime arithmetic.*

---

### `src/codegen/codegen.c` — Target Code Generation (Exp 8)
- **Target**: Formatted, human-readable standard **JSON**.
- **How it Works**:
  - Traverses the AST structure to maintain proper hierarchical nesting and indentation.
  - Queries the Symbol Table to fetch the final, optimized values.
  - Correctly outputs numbers, booleans (`true`/`false`), and quoted strings.
- **Viva Questions & Answers**:
  - *Q: Why is JSON chosen as the target code?*  
    *A: For a configuration language compiler, JSON is the industry-standard interchange format used by web servers, databases, and microservices. Generating JSON proves complete end-to-end compilation.*

---

### `src/main.c` — The Compiler Driver
- **Orchestration**:
  1. Parses command-line flags (`-o`, `--phases`, `-h`, `-v`).
  2. Opens the input file and passes it to `yyin`.
  3. Executes each phase sequentially, printing clear ASCII section banners when `--phases` is specified.
  4. Returns exit code `0` on success, or `1` on error.

---

## 4. Viva Cheat Sheet: Key Questions & Rapid Answers

| Question | Rapid Viva Answer |
|---|---|
| **What language is this implemented in?** | Pure C11 using standard POSIX libraries, GNU Flex 2.6+, and GNU Bison 3.8+. |
| **What grammar class does your parser belong to?** | LALR(1) — Look-Ahead Left-to-right with 1 token lookahead. |
| **How do you resolve shift-reduce conflicts?** | By assigning associativity and precedence `%left '+' '-'`, `%left '*' '/'`, and `%nonassoc UMINUS`. |
| **How does your compiler handle nested blocks?** | Blocks like `server { port = 8080; }` are stored in the AST as `NODE_BLOCK` and registered in the symbol table as canonical dot paths (`server.port`). |
| **What intermediate representation is used?** | Quadruple-based Three-Address Code (TAC), represented as a linked list of `TAC` structs. |
| **What optimizations are implemented?** | Constant Propagation and compile-time Constant Folding on numeric arithmetic and string concatenations. |
| **How do you demonstrate all phases during examination?** | By running `make demo` or `./build/configlang --phases examples/server.cfg`. |
