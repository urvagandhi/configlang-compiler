# ConfigLang: Architectural Design Document

**Course**: PCD (Principles of Compiler Design - 4CS501CC25)  
**Institution**: Institute of Technology, Nirma University  
**Authors**: Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)  

---

## 1. Executive Summary

ConfigLang is an educational yet production-disciplined compiler engineered in standard C11 that translates typed configuration declarations (`.cfg`) into standard JSON and YAML. This document specifies the underlying software architecture, memory lifecycle, intermediate representation semantics, data structures, and optimization algorithms.

---

## 2. Compiler Pipeline Architecture

ConfigLang executes as an 8-phase pipeline orchestrated by the compiler driver (`src/main.c`). Execution between phases is strictly gated: if earlier phases produce fatal errors, subsequent phases are bypassed to avoid cascading or undefined behaviors.

```
 +-----------------------------------------------------------------------+
 |                            Source File (.cfg)                         |
 +-----------------------------------------------------------------------+
                                     |
                                     v
 [Phase 1: Lexical Analysis] (Flex 2.6 Pure Reentrant Scanner)
   - Dispatches tokens with exact byte & character spans (line, col, len)
   - Tracks state for string escapes, line/block comments
   - HALTS on E101-E105
                                     |
                                     v
 [Phase 2: Syntax Analysis & AST Construction] (Bison 3.8 LALR(1) Parser)
   - Validates grammar with %expect 0 (0 shift/reduce, 0 reduce/reduce)
   - Constructs tagged-union AST nodes in Arena memory
   - Implements panic-mode error recovery across statements
   - HALTS on E201
                                     |
                                     v
 [Phase 3: Symbol Table Construction (Pass 1)] (Scope Tree & Chained Hash Map)
   - Traverses AST declarations and nested blocks
   - Maps full canonical dot-paths (e.g. `database.port`)
   - Detects duplicate keys in identical scopes (E301)
                                     |
                                     v
 [Phase 4: Semantic Analysis (Pass 2)] (Type Checker & Cycle Detector)
   - Resolves `@references` across local and global scopes (E305, E309)
   - Performs static type checking and type inference (E302, E307)
   - Emits implicit widening warning (W402) and empty block warning (W401)
   - Detects circular dependency cycles via DFS 3-color graph traversal (E306)
   - HALTS on E301-E309
                                     |
                                     v
 [Phase 5: Intermediate Code Generation] (Three-Address Code Emitter)
   - Linearizes expressions into TAC quadruples
   - Emits variable definitions in topological dependency order
                                     |
                                     v
 [Phase 6: Optimization] (Constant Propagation & Folding)
   - Iterative dataflow pass: propagates known constant operands
   - Evaluates constant expressions at compile-time with checked 64-bit math
   - Detects division by zero (E308) and integer arithmetic overflow (E310)
   - Eliminates dead temporary registers
                                     |
                                     v
 [Phase 7: Validation] (Constraint Validator)
   - Validates evaluated final values against declared ranges `[lo..hi]` (E303)
   - Validates floating point finiteness (`isfinite`)
   - Stores finalized values in Symbol records
   - HALTS on E303, E310
                                     |
                                     v
 [Phase 8: Target Code Generation] (JSON & YAML Serializers)
   - Serializes evaluated symbol tree with 2-space indentation
   - Handles string escaping and numeric formatting
```

---

## 3. Memory Architecture: Chunked Arena Allocator

To guarantee high performance and absolute memory safety under Valgrind (zero leaks, zero dangling pointers), ConfigLang employs an **Arena Allocator** (`src/arena.h`, `src/arena.c`).

### 3.1 Design Principles
1. **$O(1)$ Allocation**: Fast pointer bump within the active memory chunk.
2. **8-Byte Alignment**: Every allocated pointer satisfies `(uintptr_t)ptr % 8 == 0`, ensuring optimal CPU access and preventing alignment faults on 64-bit architectures.
3. **Single Bulk Deallocation**: The entire compiler AST, Symbol Table, and TAC instructions are reclaimed simultaneously in `arena_destroy()`. Individual `free()` operations on AST nodes are entirely eliminated.
4. **Chunk Growth**: If an allocation exceeds the remaining capacity of the current 64 KB chunk, a new chunk is allocated and linked onto the chunk list.

```
Arena Structure:
 [arena_t]
    |
    +---> [chunk_t (64KB)] ---> [chunk_t (64KB)] ---> NULL
             |
             +-- current pointer (bump allocation)
             +-- end pointer
```

---

## 4. Symbol Table Architecture

The symbol table (`src/symtab.h`, `src/symtab.c`) supports two simultaneous requirements:
1. **Hierarchical Scope Tree**: Preserves structural containment and declaration order for nested configuration blocks and YAML/JSON serialization.
2. **Chained Hash Map**: A 256-bucket hash table using the djb2 hash algorithm provides $O(1)$ lookup by canonical dot-path (e.g. `server.database.port`).

### 4.1 Node Data Structure
Each `symbol_t` contains:
- `name`: Local identifier name (e.g. `port`).
- `path`: Fully qualified canonical path (e.g. `server.port`).
- `kind`: `SYM_VALUE` (scalar setting) or `SYM_BLOCK` (scope container).
- `declared_type` & `effective_type`: `TYPE_INT`, `TYPE_FLOAT`, `TYPE_STRING`, `TYPE_BOOL`.
- `range`: Optional numeric bounds constraint `[lo..hi]`.
- `final_val`: Evaluated literal value stored after optimization and validation.
- `parent`, `children_head`, `children_tail`, `next_child`: Ordered hierarchy pointers.
- `hash_next`: Chaining bucket pointer for hash collisions.

---

## 5. Semantic Analysis & Cycle Detection

Semantic analysis runs as Pass 2 over the symbol table and AST.

### 5.1 Reference Resolution Algorithm
References take the form `@identifier` or `@block.sub_ident`.
1. The resolver strips the leading `@`.
2. First, it attempts a local lookup within the current enclosing block scope (`symtab_lookup_local`).
3. If not found locally, it attempts a lookup by canonical path starting from the root scope (`symtab_lookup`).
4. If the target is unresolved, error `E305` is emitted.
5. If the target is a block rather than a value, error `E309` is emitted.

### 5.2 Dependency Ordering & Cycle Detection
To support forward references and arbitrary reference topology, ConfigLang tracks directed dependency edges $A \to B$ ($A$ depends on $B$).
- Node states during DFS traversal:
  - `STATE_UNVISITED (0)`: Node not yet examined.
  - `STATE_VISITING (1)`: Node currently on the DFS call stack.
  - `STATE_DONE (2)`: Node and all descendants completely evaluated.
- If during recursive traversal of $A$'s initialization expression, a reference to $B$ is encountered where $B$ is `STATE_VISITING`, a circular dependency exists. Diagnostic `E306` is reported, pinpointing the cycle ($A \to B$), and evaluation aborts without recursion.

---

## 6. Intermediate Representation: Three-Address Code (TAC)

ConfigLang compiles high-level expressions into linear Three-Address Code quadruples (`src/tac.h`, `src/tac.c`).

### 6.1 Instruction Format
```
[OPCODE] [ARG1], [ARG2] -> [RESULT]
```
Supported TAC operations:
- `TAC_STORE target, value`: Stores a value into a canonical configuration key.
- `TAC_LOAD source -> tN`: Loads a referenced variable value into temporary register `tN`.
- `TAC_ADD`, `TAC_SUB`, `TAC_MUL`, `TAC_DIV`: Binary arithmetic operations.
- `TAC_NEG`: Unary negation.

### 6.2 Topological Emission
TAC instructions are emitted in topological dependency order. Independent constants are defined first, followed by computed variables whose dependencies have already been linearized.

---

## 7. Optimizer Architecture

The optimizer (`src/optimizer.h`, `src/optimizer.c`) executes iterative passes until a fixpoint is achieved:

1. **Constant Propagation**:
   - Maintains an in-memory mapping from temporary registers `tN` and variables to known evaluated literals (`val_literal_t`).
   - Replaces `TAC_LOAD` operands with their propagated constants.
2. **Checked Constant Folding**:
   - When all operands to a binary or unary TAC operation are constants, the operation is computed at compile-time.
   - **Division by Zero Protection (`E308`)**: If an integer or floating point divisor is `0`, error `E308` is emitted.
   - **Arithmetic Overflow Protection (`E310`)**: Integer addition, subtraction, and multiplication utilize GCC built-ins (`__builtin_add_overflow`, `__builtin_sub_overflow`, `__builtin_mul_overflow`) on 64-bit signed integers (`int64_t`). If overflow occurs, error `E310` is emitted.
   - String concatenation (`+`) is folded into an arena-allocated concatenated string.
3. **Dead Temporary Elimination**:
   - `TAC_LOAD` and binary operations whose temporary outputs have been propagated into subsequent expressions are eliminated from the instruction list.

---

## 8. Diagnostic Engine

The diagnostic subsystem (`src/diag.h`, `src/diag.c`) strictly complies with the Clang / Rust compiler style specified in SRS §5.4:

```
tests/invalid/val_range_exceeded.cfg:1:18: error[E303]: value 11 outside declared range [1..10] for 'p'
    1 | p : int(1..10) = 11;
      |                  ^^
1 error, 0 warnings generated.
```

- Automatic column gutter alignment (`    1 | `).
- Exact multi-column carets matching token width (`^^`).
- ANSI terminal color coding (red for errors, yellow for warnings, bold headers) disabled automatically when `--no-color` is set or stdout is not a TTY.
