# ConfigLang: Viva & Oral Examination Preparation Guide

**Course**: PCD (Principles of Compiler Design - 4CS501CC25)  
**Authors**: Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)  
**Institution**: Institute of Technology, Nirma University  

---

## Part 1: Architecture & Pipeline Overview

### Q1: What is ConfigLang, and what is its compiler pipeline?
**Answer**:  
ConfigLang is an end-to-end, ahead-of-time compiler written in standard C11 from scratch that compiles a strongly-typed configuration language (`.cfg`) into standard JSON and YAML.  
Its architecture follows the classic 8-phase compiler pipeline:
1. **Lexical Analysis**: Flex pure reentrant scanner converts character streams into tokens with precise line and column spans.
2. **Syntax Analysis**: Bison LALR(1) parser verifies the context-free grammar and produces an Abstract Syntax Tree (AST).
3. **Symbol Table Pass 1**: Traverses the AST to construct a hierarchical scope tree and detects duplicate identifiers.
4. **Semantic Analysis Pass 2**: Resolves variable references, performs type checking/inference, checks range declarations, and detects circular dependency cycles.
5. **Intermediate Code Generation**: Emits Three-Address Code (TAC) quadruples in topological dependency order.
6. **Code Optimization**: Performs constant propagation, compile-time constant folding (with division-by-zero and overflow detection), and dead temporary elimination.
7. **Constraint Validation**: Enforces post-optimization numeric range constraints and float finiteness.
8. **Target Code Generation**: Emits formatted, 2-space indented JSON or YAML.

---

### Q2: Why did you build ConfigLang in C11 instead of Python or Go?
**Answer**:  
Building the compiler in standard C11 directly addresses the core learning objectives of Principles of Compiler Design:
- Low-level control over memory layouts and pointer mechanics.
- First-hand implementation of production tools like Flex and Bison.
- Mastery of custom memory architectures (Arena allocators) rather than relying on automatic garbage collection.
- Achieving zero compiler warnings under strict flags (`-Wall -Wextra -Wpedantic -Wconversion -Werror`) and zero memory leaks under Valgrind.

---

## Part 2: Lexical Analysis & Flex

### Q3: What makes your Flex scanner "pure reentrant", and why is that important?
**Answer**:  
In `src/lexer.l`, we use the directive `%option reentrant`. Non-reentrant Flex scanners store their state in global static variables (`yyin`, `yytext`, `yylineno`), which makes them thread-unsafe and prevents multiple scanner instances from coexisting.  
A reentrant scanner encapsulates its entire state inside an opaque pointer (`yyscan_t`). State is passed explicitly via function parameters, enabling modularity and thread-safety.

---

### Q4: How did you track multi-character source spans and exact caret locations?
**Answer**:  
We configured `%option bison-locations` and defined `YY_USER_ACTION` in `src/lexer.l`.  
For every matched pattern:
- `yylloc->first_line` and `yylloc->first_column` record the starting position.
- `yylloc->last_line` and `yylloc->last_column` record the end position based on `yyleng`.
- For multi-line constructs or strings, we store the start coordinate upon entering the start condition (`<STR>`) so that the diagnostic engine can underline the entire span (`^^^^^`).

---

### Q5: How does the lexer distinguish between a float literal (`1.5`) and a range slice (`1..65535`)?
**Answer**:  
In Flex, token matching follows two rules:
1. **Longest Match (Maximal Munch)**.
2. **First Rule Listed** in case of equal length.  
By defining `DOTDOT` (`\.\.`) as a dedicated token before any single dot pattern, Flex reads `..` as a two-character token rather than a decimal point followed by an integer.

---

## Part 3: Syntax Analysis & Bison

### Q6: What does `%expect 0` in Bison mean, and how did you eliminate conflicts?
**Answer**:  
`%expect 0` instructs Bison to treat any shift/reduce or reduce/reduce conflict as a fatal build failure.  
Conflicts in programming language grammars typically arise from:
1. Operator precedence ambiguity (e.g. `a + b * c`).
2. Grammar ambiguities such as the "dangling else" problem or overlapping production prefixes.  
We eliminated all conflicts by declaring operator precedence explicitly (`%left PLUS MINUS`, `%left STAR SLASH`, `%right UMINUS`) and designing distinct grammar non-terminals for declarations and blocks.

---

### Q7: How does your parser implement syntax error recovery?
**Answer**:  
We implemented panic-mode error recovery using Bison's built-in `error` pseudo-token:
```bison
item: declaration | block | error SEMI ;
```
When an unexpected token is encountered, Bison reports error `E201`, discards tokens until it finds a synchronizing token (the semicolon `;`), and smoothly resumes parsing subsequent configuration items.

---

## Part 4: AST & Memory Management

### Q8: What is an Arena Allocator, and why is it superior to `malloc()`/`free()` in a compiler?
**Answer**:  
An arena allocator (`src/arena.c`) allocates a large continuous memory block (e.g., 64 KB chunks) and satisfies allocation requests by bumping a pointer forward ($O(1)$ complexity).  
Benefits for compilers:
- **Zero Fragmentation**: Small nodes (AST nodes, tokens, symbols) are tightly packed.
- **Cache Locality**: Contiguous memory improves CPU L1/L2 cache hits during tree traversals.
- **Zero Memory Leaks**: Compilers create deeply nested, cyclic, or interconnected graphs that are tedious and error-prone to free node-by-node. With an arena, the entire compiler lifetime memory is reclaimed in a single `arena_destroy()` call.

---

### Q9: How is your AST represented in C?
**Answer**:  
In `src/ast.h`, the AST uses a **tagged union** pattern:
```c
typedef struct ast_node {
    ast_kind_t kind;      /* Tag: AST_DECL, AST_BLOCK, AST_BINARY, etc. */
    diag_loc_t loc;       /* Source location for diagnostics */
    union {
        ast_decl_t decl;
        ast_block_t block;
        ast_binary_t binary;
        ast_unary_t unary;
        ast_ref_t ref;
        val_literal_t lit;
    } data;
} ast_node_t;
```
This allows homogenous manipulation of heterogenous expression and statement trees.

---

## Part 5: Symbol Table & Semantic Analysis

### Q10: Why does ConfigLang use two passes for semantic analysis instead of one?
**Answer**:  
In configuration languages, declarations may reference variables declared later in the file (forward references) or within sibling blocks:
```cfg
server { port = @base_port + 80; }
base_port = 8000;
```
- **Pass 1 (Symbol Registration)**: Traverses the AST to collect all block scopes and variable declarations into the symbol table, checking for duplicate names (`E301`).
- **Pass 2 (Type Checking & Semantic Validation)**: Once all symbols are registered and visible, Pass 2 evaluates expressions, resolves references (`@base_port`), validates types, and detects cyclic dependencies.

---

### Q11: How do you detect circular references (e.g. `a = @b; b = @a;`)?
**Answer**:  
We model symbol dependencies as a directed graph and perform Depth-First Search (DFS) with **3-color marking**:
- `STATE_UNVISITED (0)`: Node has not been visited.
- `STATE_VISITING (1)`: Node is currently on the active DFS recursion stack.
- `STATE_DONE (2)`: Node and all its dependencies have been fully resolved.  
If during the traversal of $A$'s expression we reference $B$, and $B$ is already in `STATE_VISITING`, a back-edge in the dependency graph is detected. We immediately log diagnostic `E306` with the cycle chain ($A \to B$) and abort recursion.

---

### Q12: How are scoped identifiers resolved (e.g. `@server.workers`)?
**Answer**:  
The symbol table maintains both a tree of scopes and a 256-bucket hash table of fully qualified canonical paths (`djb2` hash).  
When resolving `@server.workers`:
1. It first checks the local scope enclosing the reference.
2. If not found, it queries the hash table with the full canonical dot-path `server.workers`.
3. If the path does not exist, diagnostic `E305` (undefined reference) is emitted.
4. If the path resolves to a block rather than a scalar value, diagnostic `E309` is emitted.

---

## Part 6: Intermediate Representation & Optimization

### Q13: What is Three-Address Code (TAC), and why is it used?
**Answer**:  
Three-Address Code is an intermediate representation where each instruction has at most one operator and at most three operands (two sources, one destination).  
Example:
```cfg
timeout_ms = 30 * 1000 + 50;
```
Translates to:
```
t1 = 30 MUL 1000
t2 = t1 ADD 50
STORE timeout_ms, t2
```
TAC bridges high-level AST trees and low-level code generation, simplifying optimizations like constant folding and dead code elimination.

---

### Q14: What optimizations does ConfigLang implement?
**Answer**:  
`src/optimizer.c` implements three dataflow optimizations:
1. **Constant Propagation**: Tracks values assigned to temporary registers and variables, substituting literal values into subsequent expressions.
2. **Constant Folding**: Evaluates constant expressions at compile time.
3. **Dead Temporary Elimination**: Discards temporary registers (`t1`, `t2`) that were completely folded into downstream constants.

---

### Q15: How does your optimizer prevent undefined behavior (UB) in C during constant folding?
**Answer**:  
In C, signed integer overflow and division by zero cause Undefined Behavior. ConfigLang actively protects against this:
- **Division by Zero (`E308`)**: Checked explicitly before evaluating `/` on integer or floating point operands.
- **Arithmetic Overflow (`E310`)**: We use compiler built-in functions:
  - `__builtin_add_overflow(a, b, &res)`
  - `__builtin_sub_overflow(a, b, &res)`
  - `__builtin_mul_overflow(a, b, &res)`
  If overflow occurs on 64-bit integers (`int64_t`), error `E310` is emitted cleanly.

---

## Part 7: Validation & Code Generation

### Q16: When are range constraints validated, and why?
**Answer**:  
Range constraints (e.g. `port : int(1..65535) = @base_port + 80;`) are validated in **Phase 7 (Post-Optimization)**.  
Validation cannot occur during AST parsing because the value of `@base_port + 80` is not yet computed. After TAC optimization has folded all expressions into finalized constants, the validator inspects the symbol's `final_val` and flags `E303` if `val < lo || val > hi`.

---

### Q17: How is YAML generated without an external library?
**Answer**:  
`src/codegen.c` recursively traverses the hierarchical scope tree (`root_block`).
- For each child symbol:
  - If it is a scalar: it prints the key, colon, space, and formatted value.
  - If it is a block: it prints `key:\n`, increases the indentation level, and recurses through the child list.
  - Empty blocks are printed as `{}`.

---

## Part 8: Error Handling & Quality Assurance

### Q18: What is your diagnostic formatting standard?
**Answer**:  
We follow the Clang/Rust compiler diagnostic standard:
```
<file>:<line>:<col>: <severity>[<code_id>]: <message>
    <line_no> | <source_line>
              | <carets>
```
The diagnostic engine extracts the source line from memory, aligns the line number gutter, and prints carets (`^^^^`) under the offending token span.

---

### Q19: How did you verify memory safety?
**Answer**:  
We ran Valgrind with `--leak-check=full --error-exitcode=42` on both valid files and invalid error cases.  
Every run reported:
```
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts
```

---

### Q20: How did you test compiler robustness against corrupted inputs?
**Answer**:  
In `tests/run_tests.sh`, we executed a fuzzing loop generating 30 random byte sequences from `/dev/urandom` and fed them directly to `configlang`. The compiler caught all lexical and syntax errors without crashing or exceeding exit code 2.

---

## Summary of PCD Concepts Demonstrated in ConfigLang
- **Chomsky Hierarchy**: Regular expressions (Type 3) in Flex, Context-Free Grammar (Type 2) in Bison.
- **Parsing Automata**: Deterministic LALR(1) shift-reduce parsing table generation.
- **Syntax-Directed Definitions (SDD)**: Synthesized attributes constructing AST nodes.
- **Static Semantic Checks**: Type systems, scope tables, symbol resolution.
- **Graph Algorithms in Compilers**: Directed acyclic graphs (DAGs), topological sort, DFS cycle detection.
- **Intermediate Representations**: Quadruple-based Three-Address Code.
- **Machine-Independent Code Optimization**: Constant propagation, folding, and dead code elimination.
