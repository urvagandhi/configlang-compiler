# ConfigLang: Code Explanation & Viva Cheat Sheet
**Principles of Compiler Design (4CS501CC25) — Semester VII, B.Tech CSE, Nirma University**  
**Authors**: Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)

---

## 1. Quick Project Summary (30-Second Viva Pitch)
> *"ConfigLang is a typed configuration language compiler built from scratch in C11 using Flex and Bison. It takes a typed configuration file (`.cfg`), runs it through the 6 classic compiler phases (Lexical Analysis, Syntax Analysis, Symbol Table Management, Semantic Analysis, Intermediate Code Generation, and Code Optimization), and generates production-ready JSON or YAML output. It directly implements Experiments 1 through 9 of our Nirma University PCD syllabus."*

---

## 2. Architecture Map: How `src/` Maps to Compiler Phases

```
                  +-----------------------------------+
                  |  Source File (.cfg)               |
                  +-----------------------------------+
                                    |
                                    v
[Phase 1]       src/lexer/lexer.l       (Flex Scanner)
                Exp 1 & 2: Tokenizes input into INT_LIT, IDENT, keywords
                                    |
                                    v
[Phase 2]       src/parser/parser.y     (Bison Parser + AST in ast.c)
                Exp 4 & 5: Parses grammar & builds Abstract Syntax Tree
                                    |
                                    v
[Phase 3]       src/symtab/symtab.c     (Symbol Table - Pass 1)
                Exp 3: Populates 1024-bucket hash table & scope tree
                                    |
                                    v
[Phase 4]       src/semantic/semantic.c (Type & Reference Check - Pass 2)
                Exp 6: Type checks, resolves @refs, detects cycles with DFS
                                    |
                                    v
[Phase 5]       src/tac/tac.c           (Three-Address Code Generator)
                Exp 7: Converts expressions into 3-operand quadruples
                                    |
                                    v
[Phase 6]       src/optimizer/optimizer.c (Constant Folding & Propagation)
                Exp 9: Evaluates 2+3->5 at compile time & drops dead temps
                                    |
                                    v
[Phase 7]       src/semantic/validator.c (Constraint Validator)
                Checks user range bounds like port(1024..65535)
                                    |
                                    v
[Phase 8]       src/codegen/codegen.c   (Target Code Generator)
                Exp 8: Emits clean, formatted JSON or YAML output
```

---

## 3. Detailed Walkthrough of Every File in `src/`

### `src/main.c` — The Compiler Driver
- **Purpose**: Command-line interface and orchestrator.
- **How it works**:
  1. Parses CLI flags (`--tokens`, `--ast`, `--symtab`, `--tac`, `--tac-opt`, `--phases`, `-f yaml`).
  2. Reads the input file using `util_read_file()`.
  3. Sequentially invokes each phase:
     `lexer_scan` -> `parser_parse` -> `symtab_build_pass1` -> `semantic_analyze` -> `tac_generate` -> `optimizer_run` -> `validator_validate` -> `codegen_emit_file`.
  4. If any phase reports errors, it prints pretty colored diagnostics with line/column pointers and exits with code 1.
  5. Frees all memory via `arena_destroy()` and exits with code 0 on success.

---

### `src/lexer/` — Lexical Analyzer (Exp 1 & 2)
- **Files**: `lexer.l` (Flex definition), `lexer.h` (Token declarations).
- **Key Concepts**:
  - Regular expressions for tokens:
    - Identifiers: `[A-Za-z_][A-Za-z0-9_]*`
    - Integers: `[0-9]+`
    - Floats: `[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?`
    - Strings: `"..."` with escape sequences (`\n`, `\t`, `\\`, `\"`).
    - Comments: Single-line (`//...`) and multi-line (`/* ... */`).
  - Error detection:
    - `E101`: Illegal character (e.g. `$`, `~`).
    - `E102`: Unterminated string literal.
    - `E103`: 64-bit integer overflow.
    - `E104`: Invalid escape sequence.
    - `E105`: Unterminated block comment.
- **Viva Q&A**:
  - *Q: How does the lexer track line and column numbers?*  
    *A: In `YY_USER_ACTION`, we loop through the matched text (`yytext`), incrementing `cur_col` for normal characters and incrementing `cur_line` and resetting `cur_col=1` whenever a `\n` is encountered.*

---

### `src/parser/` — Syntax Analyzer & AST (Exp 4 & 5)
- **Files**: `parser.y` (Bison grammar), `parser.h`, `ast.h`, `ast.c`.
- **Key Concepts**:
  - **LALR(1) Grammar**: Defines production rules for blocks, declarations, and arithmetic expressions with standard operator precedence (`*`, `/` higher than `+`, `-`).
  - **AST Nodes** (`ast_node_t`): Tagged union representing:
    - `AST_PROGRAM`: Root node containing top-level statements.
    - `AST_BLOCK`: Container for nested blocks (e.g. `server { ... }`).
    - `AST_DECL`: Variable declaration (`name : type = expr;`).
    - `AST_BINARY`: Binary operations (`+`, `-`, `*`, `/`).
    - `AST_UNARY`: Unary negation (`-expr`).
    - `AST_REF`: Reference to another variable (`@server.port`).
    - `AST_LIT_*`: Literal values (int, float, string, boolean).
  - **Syntax Error Recovery (Exp 5)**:
    - `item : error SEMI { $$ = NULL; yyerrok; }`
    - When a syntax error occurs, Bison discards bad tokens until it reaches a semicolon (`;`), reports error `E201`, and resumes parsing without crashing.
- **Viva Q&A**:
  - *Q: How is operator precedence handled in Bison?*  
    *A: We declare `%left PLUS MINUS` followed by `%left STAR SLASH` and `%precedence UMINUS`. Bison gives higher precedence to declarations listed later, ensuring `*` binds tighter than `+`.*

---

### `src/symtab/` — Symbol Table (Exp 3)
- **Files**: `symtab.c`, `symtab.h`.
- **Key Concepts**:
  - **Hybrid Data Structure**:
    1. **Hash Table** with 1024 buckets and separate chaining (linked list). Provides fast $O(1)$ lookup for canonical dotted paths (e.g. `database.primary.port`).
    2. **Scope Tree**: Each symbol points to its `parent` scope and has a linked list of `children` to preserve hierarchical nesting and original declaration order.
  - **Hash Function**: Standard `djb2` string hash (`hash = ((hash << 5) + hash) + c`).
  - **Duplicate Detection**: `symtab_lookup_local()` checks if a name was already declared in the current block, reporting `E301` if a duplicate is found.
- **Viva Q&A**:
  - *Q: Why do you need both a hash table and a scope tree?*  
    *A: The hash table gives $O(1)$ fast lookup when resolving references like `@server.port`, while the scope tree preserves parent-child block relationships and declaration order for code generation.*

---

### `src/semantic/` — Semantic Analyzer & Validator (Exp 6)
- **Files**: `semantic.c`, `semantic.h`, `validator.c`, `validator.h`.
- **Key Concepts**:
  - **Pass 2 of Compilation**:
    1. **Type Checking**:
       - `int + int -> int`, `int + float -> float`, `string + string -> string`.
       - Emits error `E307` for invalid operands (e.g. `string - int` or `bool + int`).
       - Emits error `E302` for type mismatch on assignment (`x: int = "hello"`).
       - Emits warning `W402` on implicit widening (`float x = 42`).
    2. **Reference Resolution**:
       - Resolves `@path` references. Emits `E305` if undefined, or `E309` if pointing to a block instead of a value.
    3. **Circular Reference Detection (Cycle Detection)**:
       - Uses 3-state Depth First Search (DFS):
         - `STATE_UNVISITED (0)`: Node not yet examined.
         - `STATE_VISITING (1)`: Node is currently on the call stack.
         - `STATE_DONE (2)`: Node and its dependencies are verified.
       - If a reference reaches a node in `STATE_VISITING`, a circular dependency exists! Emits `E306` with the full cycle path (e.g. `a -> b -> a`).
    4. **Range Validation** (`validator.c`):
       - Verifies that user-defined numeric constraints (e.g. `port(1024..65535)`) are valid (`lo <= hi`, `E304`).
       - Validates that the evaluated final value falls inside the range (`E303`).
- **Viva Q&A**:
  - *Q: How do you detect circular references like `a = @b; b = @a;`?*  
    *A: We mark variable `a` as `VISITING` and recursively evaluate its dependency `b`. While evaluating `b`, it references `a` which is still marked `VISITING`. Detecting `VISITING` immediately flags a cycle (E306).*

---

### `src/tac/` — Three-Address Code Generator (Exp 7)
- **Files**: `tac.c`, `tac.h`.
- **Key Concepts**:
  - Lowers tree-structured expressions into a linear sequence of instructions with at most 3 operands:
    `Result = Arg1 OP Arg2`
  - Opcodes: `TAC_ADD`, `TAC_SUB`, `TAC_MUL`, `TAC_DIV`, `TAC_NEG`, `TAC_LOAD`, `TAC_STORE`.
  - Generates unique temporary variables: `t1`, `t2`, `t3`, ...
  - Emits instructions in **topological dependency order**, so prerequisites are computed before dependent variables.
- **Viva Q&A**:
  - *Q: Why is Three-Address Code useful before code generation?*  
    *A: TAC linearizes complex nested expression trees into simple, atomic operations. This makes optimization (like constant folding) and target code generation much simpler.*

---

### `src/optimizer/` — Code Optimizer (Exp 9)
- **Files**: `optimizer.c`, `optimizer.h`.
- **Key Concepts**:
  - **Constant Folding**: Evaluates constant arithmetic at compile time:
    - `t1 = 2 + 3`  -->  `t1 = 5`
    - `t2 = "a" + "b"` --> `t2 = "ab"`
    - Checks for division by zero (`E308`) and integer overflow (`E310`).
  - **Constant Propagation**: Replaces uses of variables/temporaries with their known folded values:
    - If `t1 = 5`, then `t2 = t1 * 2` becomes `t2 = 5 * 2`, which folds into `t2 = 10`.
  - **Dead Code Elimination**: Removes intermediate temporary calculations that were folded away, reducing instruction count.
- **Viva Q&A**:
  - *Q: What is the difference between Constant Folding and Constant Propagation?*  
    *A: Constant Folding computes an operation between constants at compile time (`2 + 3 -> 5`). Constant Propagation replaces variable references with their known constant values so subsequent operations can be folded.*

---

### `src/codegen/` — Code Generator (Exp 8)
- **Files**: `codegen.c`, `codegen.h`.
- **Key Concepts**:
  - Walks the evaluated symbol table scope tree and writes out formatted data.
  - Supports two target formats:
    - **JSON** (`-f json`): Emits valid `{ "key": value }` syntax with commas, indentation, and escaped strings.
    - **YAML** (`-f yaml`): Emits clean YAML with 2-space indentation.
  - Handles string escaping (`\"`, `\n`, `\t`, `\\`) and numeric formatting.
- **Viva Q&A**:
  - *Q: How does code generation handle nested blocks?*  
    *A: It uses a recursive helper (`print_json_object` or `print_yaml_node`). When a child symbol is of kind `SYM_BLOCK`, it indents and recursively calls itself to emit the nested block.*

---

### `src/common/` — Shared Infrastructure
- **Files**: `arena.c`/`arena.h`, `diag.c`/`diag.h`, `util.c`/`util.h`.
- **Key Concepts**:
  - **Arena Allocator** (`arena.c`): Allocates memory in 64 KB chunks. Eliminates individual `free()` calls and guarantees zero memory leaks.
  - **Diagnostic Collector** (`diag.c`): Stores errors and warnings, sorts them by source line/column, and prints Clang-style caret underlines (`^^^^`).
  - **Utilities** (`util.c`): Checked `malloc`/`calloc` wrappers and dynamic string buffer (`strbuf_t`).

---

## 4. Viva Speaking Script (2-Minute Project Presentation)

### Speaker 1 (Urva Gandhi - 23BCE078):
> *"Good morning respected faculty. For our PCD Innovative Assignment, Rakshit and I designed and implemented **ConfigLang**, a typed configuration language compiler.*  
> *In real-world software, bad configuration files cause major production outages because formats like JSON or YAML lack type checking, range validation, and circular dependency checks.*  
> *ConfigLang solves this by treating configuration as a compiled language.*  
> *I worked on the front-end phases: the Lexical Analyzer using Flex in `src/lexer/` for Experiment 1 and 2, the Syntax Analyzer using Bison in `src/parser/` for Experiment 4 and 5 with error recovery, and the Symbol Table in `src/symtab/` for Experiment 3 which uses a 1024-bucket hash table and a scope tree."*

### Speaker 2 (Rakshit Gajnotar - 23BCE077):
> *"Building on the AST and Symbol Table, I worked on the semantic and back-end phases:*  
> *In `src/semantic/`, we implemented type checking, reference resolution, and a 3-state Depth-First Search algorithm to detect circular dependencies, fulfilling Experiment 6.*  
> *Next, in `src/tac/`, we generate Three-Address Code quadruples in topological order for Experiment 7.*  
> *In `src/optimizer/`, we implemented compile-time Constant Folding, Constant Propagation, and Dead Temporary Elimination for Experiment 9.*  
> *Finally, in `src/codegen/`, we emit production-ready JSON or YAML for Experiment 8.*  
> *Our entire test suite of 79 test cases passes with zero failures, and Valgrind verifies zero memory leaks across all phases."*

---

## 5. Top 5 Viva Questions & Exact 1-Sentence Answers

| # | Question Asked by Faculty | Exact 1-Sentence Answer |
|---|---------------------------|-------------------------|
| 1 | **What is the role of Three-Address Code (TAC)?** | TAC linearizes expression trees into single-operator quadruples (`t = a op b`), making optimizations like constant folding much easier to implement. |
| 2 | **How do you handle syntax error recovery?** | In `parser.y`, we use the error production `item : error SEMI`, which skips tokens until a semicolon and calls `yyerrok` to resume parsing. |
| 3 | **How is cycle detection implemented?** | We perform a 3-state DFS traversal (`UNVISITED`, `VISITING`, `DONE`); encountering a symbol in `VISITING` state indicates a cycle. |
| 4 | **Why use an Arena Allocator?** | Compilers allocate thousands of small AST nodes; allocating from 64 KB chunks is faster and freeing the arena at the end guarantees zero memory leaks. |
| 5 | **What is the time complexity of symbol lookup?** | $O(1)$ average time complexity using a 1024-bucket hash table with the `djb2` hashing algorithm. |
