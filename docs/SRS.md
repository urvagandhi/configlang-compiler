# Software Requirements Specification (SRS)

## ConfigLang: A Typed Configuration Language Compiler with Semantic Validation

| Field | Value |
|---|---|
| Course | PCD (Compiler Design), 4CS501CC25 |
| Document | SRS, version 1.0 |
| Student | Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077) |
| Status | Approved baseline. This file is the **source of truth for all technical behaviour**. |
| Companion | `docs/PRD.md` (scope, priorities, schedule) |

---

## 1. Introduction

### 1.1 Purpose
This document specifies, precisely and testably, the behaviour of **ConfigLang**, a small compiler that translates a custom, typed configuration language into standard JSON (and optionally YAML). It is written so that an implementer (human or AI agent) can build the system end to end without further clarification.

### 1.2 Scope
ConfigLang is a **real multi-phase compiler** (not a "config parser"). It performs:

1. Lexical analysis → 2. Syntax analysis → 3. AST construction → 4. Symbol-table construction → 5. Semantic analysis (types, references, cycles) → 6. Three-address-code (TAC) generation → 7. Optimization (constant propagation and folding) → 8. Validation (range and arithmetic checks) → 9. Code generation (JSON / YAML).

Out of scope: a runtime, environment-variable substitution, includes/imports, arrays, a web UI, an IDE plugin (see PRD §5).

### 1.3 Definitions
| Term | Meaning |
|---|---|
| Item | Either a *block* or a *declaration* |
| Block | `name { items }`, introduces a scope |
| Declaration | `name [: type [range]] = expr ;` |
| Path | Dotted full name of a symbol, e.g. `server.port` |
| Leaf symbol | A declaration (holds a value) |
| TAC | Three-address code, linear intermediate representation |
| Diagnostic | An error or warning message with code, location and text |

### 1.4 References
- Aho, Lam, Sethi, Ullman, *Compilers: Principles, Techniques, and Tools* (phases, SDT, TAC, optimization).
- Flex and Bison manuals (GNU).
- Course practical list 4CS501CC25 (lexical analysis, symbol table, syntax analysis, semantic analysis, TAC, code generation, optimization).

### 1.5 Overview
§2 describes the product context. §3 specifies the language. §4 specifies each compiler phase. §5 specifies the CLI and outputs. §6 gives the diagnostic catalogue. §7 gives non-functional requirements and coding standards. §8 gives verification and traceability. Appendices hold the grammar and worked examples.

---

## 2. Overall Description

### 2.1 Product perspective
A stand-alone command-line program `configlang`, written in **C (C11)** using **Flex** (lexer) and **Bison** (parser), built with **GNU Make** and **GCC/Clang**. No third-party libraries other than the C standard library.

### 2.2 Compiler pipeline
```
 source.cfg
     │
     ▼
 [1] Lexical Analysis      (Flex)           → token stream
     ▼
 [2] Syntax Analysis       (Bison, LALR(1)) → parse + error recovery
     ▼
 [3] AST Construction                       → abstract syntax tree
     ▼    ── gate A: stop if any E1xx/E2xx ──
 [4] Symbol Table Build    (pass 1)         → scopes, duplicates (E301)
     ▼
 [5] Semantic Analysis     (pass 2)         → types, refs, cycles
     ▼    ── gate B: stop if any semantic error ──
 [6] TAC Generation                         → quadruples in dependency order
     ▼
 [7] Optimization                           → constant propagation + folding
     ▼
 [8] Validation                             → ranges, div-by-zero, overflow
     ▼    ── gate C: stop if any error ──
 [9] Code Generation                        → JSON / YAML
```
Each gate collects **all** errors of its phase before stopping (no "first error only" behaviour).

### 2.3 Users
- **Config author**: writes `.cfg` files and wants typed, validated output.
- **Evaluator / professor**: inspects each compiler phase and the code quality.

### 2.4 Constraints
- Implementation language C11; Flex ≥ 2.6, Bison ≥ 3.0, GCC ≥ 9 or Clang ≥ 11.
- Works on Linux, macOS and Windows via WSL/MSYS2.
- Work must be **original**. Public repositories may inspire architecture only; no code is copied or vendored.

### 2.5 Assumptions
- Input is UTF-8 text; non-ASCII bytes are legal only inside string literals and comments.
- A source file is processed in one run; there is no incremental compilation.

---

## 3. The ConfigLang Language

### 3.1 Informal description
```
// comment
/* block comment */

app_name : string = "ConfigLang Demo";
base_port : int(1024..65535) = 8000;

server {
    port : int(1..65535) = @base_port + 80;   // reference + arithmetic
    workers : int(1..64) = 4;
    timeout_ms : int = 30 * 1000;             // constant folded
    max_connections : int = @server.workers * 250;
    debug : bool = false;
    region = "ap-south-1";                    // type inferred: string
}
```
Key ideas: **typed declarations with optional numeric ranges**, **references** (`@path`) with forward references allowed, **constant expressions**, **nested blocks**.

> Design note: an untyped assignment such as `workers = "four";` has no declared type, so it cannot be an error. Type safety therefore comes from the **annotation** `workers : int = "four";` → `E302`. This is the core "typed configuration" innovation.

### 3.2 Lexical specification

**Ignored:** space, tab, CR, LF; line comments `// … EOL`; block comments `/* … */` (not nested).

| Token | Pattern / spelling | Notes |
|---|---|---|
| `KW_INT KW_FLOAT KW_STRING KW_BOOL` | `int float string bool` | reserved |
| `KW_TRUE KW_FALSE` | `true false` | reserved |
| `IDENT` | `[A-Za-z_][A-Za-z0-9_]*` | not a keyword |
| `INT_LIT` | `[0-9]+` | must fit in signed 64-bit, else `E103` |
| `FLOAT_LIT` | `[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?` | IEEE double |
| `STRING_LIT` | `"` … `"` | single line; escapes `\" \\ \n \t \r`; others `E104` |
| Punctuation | `{ } ( ) = : ; . .. + - * / @` | `..` is `DOTDOT` |

Rules:
- **Longest match** applies. Therefore `1..65535` lexes as `INT_LIT DOTDOT INT_LIT` (a float needs a digit after the dot) and `server.port` as `IDENT DOT IDENT`.
- No negative literals in the lexer; `-` is an operator handled by the parser.
- Every token carries `first_line, first_column, last_line, last_column` (1-based). Column counts bytes; a tab counts as 1.
- On a lexical error the lexer records the diagnostic, skips the offending input, and continues.

### 3.3 Syntactic specification
Full Bison-style grammar is in **Appendix A**. Summary (EBNF):

```
program      = { item } ;
item         = block | declaration ;
block        = IDENT "{" { item } "}" ;
declaration  = IDENT ":" type [ range ] "=" expr ";"
             | IDENT "=" expr ";" ;
type         = "int" | "float" | "string" | "bool" ;
range        = "(" number ".." number ")" ;           (* number may be preceded by "-" *)
expr         = term { ("+" | "-") term } ;
term         = unary { ("*" | "/") unary } ;
unary        = "-" unary | primary ;
primary      = INT_LIT | FLOAT_LIT | STRING_LIT | "true" | "false"
             | reference | "(" expr ")" ;
reference    = "@" IDENT { "." IDENT } ;
```
Precedence (high → low): unary `-`; `* /`; `+ -`. All binary operators are left-associative.

The grammar must be **LALR(1) with zero shift/reduce and zero reduce/reduce conflicts** (`%expect 0`).

### 3.4 Static semantics

**Scopes.** The file is the root scope. Each block creates a child scope. The **path** of a symbol is the dot-joined names of enclosing blocks plus its own name (`server.port`). Root-level symbols have no prefix.

**Types.** `int` (signed 64-bit), `float` (IEEE double), `string`, `bool`. Plus the internal type `ERROR` used to suppress cascading diagnostics.

**Declared vs expression type.**
| Declared | Allowed expression type |
|---|---|
| `int` | `int` |
| `float` | `float` or `int` (implicit widening, warning `W402`) |
| `string` | `string` |
| `bool` | `bool` |
| *(none)* | any; the symbol's type becomes the expression type |

**Operator typing.**
| Operator | Operands → result |
|---|---|
| `+` | `int+int→int`; `int/float` mixed or `float+float→float`; `string+string→string` |
| `-` `*` `/` | numeric only; `int op int→int`; any `float` operand → `float` |
| unary `-` | `int→int`, `float→float` |
| anything else | `E307` |

`int / int` truncates toward zero. `bool` never participates in arithmetic.

**References.** `@a.b.c` must name an existing **leaf symbol** (`E305` if missing, `E309` if it names a block). Forward references are legal. Absolute paths only.

**Cycles.** Any dependency cycle (including self-reference) is `E306`. Each distinct cycle is reported once, with the chain, e.g. `a -> b -> c -> a`.

**Ranges.** `int(lo..hi)` / `float(lo..hi)` is inclusive. Requirements: only on `int`/`float` declarations; `lo <= hi`; bound literal kind must match the declared type (`int` needs integer bounds). Violations are `E304`. The **value** must lie in range, checked after optimization (`E303`).

**Duplicates.** Within one scope, every name (block or declaration) must be unique (`E301`). The diagnostic points at the second definition and names the line of the first.

**Warnings.** `W401` empty block; `W402` implicit `int → float` widening. Warnings never change the exit code.

---

## 4. Functional Requirements (by phase)

Priority: **M** = must, **S** = should, **C** = could. Every **M** is required for acceptance.

### 4.1 Lexical analysis (`src/lexer.l`)
| ID | Requirement | P |
|---|---|---|
| FR-LEX-01 | Recognise all tokens in §3.2 with correct longest-match behaviour. | M |
| FR-LEX-02 | Track line and column for every token, including across comments and strings. | M |
| FR-LEX-03 | Report `E101`–`E105` with accurate location, then continue lexing. | M |
| FR-LEX-04 | Decode string escapes into the token value; reject raw newlines inside strings. | M |
| FR-LEX-05 | Provide a token-dump facility used by `--tokens` (format in §5.3). | M |
| FR-LEX-06 | Reject integer literals above `INT64_MAX` (`E103`) without undefined behaviour. | M |

### 4.2 Syntax analysis and AST (`src/parser.y`, `src/ast.c`)
| ID | Requirement | P |
|---|---|---|
| FR-SYN-01 | Implement the grammar in Appendix A with `%expect 0`. | M |
| FR-SYN-02 | Use `%locations`; every AST node stores its source span. | M |
| FR-SYN-03 | Syntax errors are `E201` with an *unexpected token* and, when available, an *expected* list (`%define parse.error verbose`, post-processed to readable text). | M |
| FR-SYN-04 | Error recovery: `item : error ';'` so later errors are still found. Never loop forever on bad input. | M |
| FR-SYN-05 | AST node kinds: `PROGRAM, BLOCK, DECL, INT, FLOAT, STRING, BOOL, REF, BINARY, UNARY`. Tagged-union struct, arena allocated. | M |
| FR-SYN-06 | `--ast` prints an indented tree (format in §5.3). | M |

### 4.3 Symbol table (`src/symtab.c`)
| ID | Requirement | P |
|---|---|---|
| FR-SYM-01 | Hash table with **separate chaining**, keyed by full dotted path. | M |
| FR-SYM-02 | A scope tree preserving **declaration order** (needed for output ordering). | M |
| FR-SYM-03 | Symbol record: `name, path, kind(BLOCK/VALUE), declared_type, effective_type, range(lo,hi,present), decl_node, line, col, state(UNVISITED/VISITING/DONE), final_value`. | M |
| FR-SYM-04 | Pass 1 inserts every symbol, reporting `E301` for duplicates (second definition is not inserted). | M |
| FR-SYM-05 | `--symtab` prints the table (format in §5.3), including final values after optimization. | M |
| FR-SYM-06 | Lookup by path in O(1) average. | M |

### 4.4 Semantic analysis (`src/semantic.c`)
| ID | Requirement | P |
|---|---|---|
| FR-SEM-01 | Infer the type of every expression per §3.4; check against the declared type (`E302`). | M |
| FR-SEM-02 | Resolve each reference (`E305`, `E309`). | M |
| FR-SEM-03 | Detect dependency cycles with a visiting-state DFS (`E306`), reporting the chain. | M |
| FR-SEM-04 | Validate range declarations (`E304`). | M |
| FR-SEM-05 | Operand type checks (`E307`). | M |
| FR-SEM-06 | Use type `ERROR` to avoid cascading duplicates of the same root cause. | M |
| FR-SEM-07 | Record dependency edges; `--deps` prints `a -> b` lines. | S |
| FR-SEM-08 | Emit `W401`, `W402`. | S |

### 4.5 TAC generation (`src/tac.c`)
| ID | Requirement | P |
|---|---|---|
| FR-TAC-01 | Quadruple form `{op, arg1, arg2, result, loc}`. Operands: constant, temporary (`t1…`), symbol path. | M |
| FR-TAC-02 | Ops: `ADD SUB MUL DIV NEG CONCAT LOAD STORE`. | M |
| FR-TAC-03 | Emit instructions in **dependency (topological) order** so every `LOAD` follows its `STORE`. | M |
| FR-TAC-04 | `--tac` prints the unoptimized listing (format in §5.3). | M |

### 4.6 Optimization (`src/optimizer.c`)
| ID | Requirement | P |
|---|---|---|
| FR-OPT-01 | Constant folding for all arithmetic and `CONCAT`. | M |
| FR-OPT-02 | Constant propagation: `LOAD` of a constant-valued symbol is replaced by the constant. | M |
| FR-OPT-03 | Dead-temporary elimination. | M |
| FR-OPT-04 | After optimization every value declaration is a single `STORE path, constant`. | M |
| FR-OPT-05 | Checked 64-bit arithmetic; overflow → `E310`; division by zero → `E308`. No undefined behaviour. | M |
| FR-OPT-06 | `--tac-opt` prints the optimized listing plus statistics (instructions before/after, folds, propagations). | M |

### 4.7 Validation (`src/validator.c`)
| ID | Requirement | P |
|---|---|---|
| FR-VAL-01 | Check each final value against its declared range (`E303`), inclusive bounds. | M |
| FR-VAL-02 | Float values must be finite; otherwise `E310`. | M |

### 4.8 Code generation (`src/codegen.c`)
| ID | Requirement | P |
|---|---|---|
| FR-GEN-01 | JSON output, 2-space indentation, nested objects mirroring blocks, **source order preserved**. | M |
| FR-GEN-02 | JSON string escaping: `" \ \b \f \n \r \t` and `\u00XX` for other control characters; UTF-8 passes through. | M |
| FR-GEN-03 | Floats printed with `%.15g`; if the result lacks `.`, `e` or `E`, append `.0`. | M |
| FR-GEN-04 | Empty block → `{}`; empty file → `{}`. | M |
| FR-GEN-05 | YAML output (`--format yaml`): 2-space indentation, strings always double-quoted, empty block `key: {}`. | S |
| FR-GEN-06 | No output file is created or truncated when any error occurred. | M |

### 4.9 Diagnostics (`src/diag.c`)
| ID | Requirement | P |
|---|---|---|
| FR-DIA-01 | Central collector: add, count by severity, sort by (line, column), print. | M |
| FR-DIA-02 | Format exactly as §5.4, including source line and caret span. | M |
| FR-DIA-03 | Cap output at 50 diagnostics, then print `too many errors`. | S |
| FR-DIA-04 | Colour only when stderr is a TTY and `--no-color` is not set. | S |

---

## 5. External Interfaces

### 5.1 Command line
```
configlang [options] <input-file>

  -o, --output <file>       write result to file (default: stdout)
  -f, --format <json|yaml>  output format (default: json)
      --tokens              dump token stream
      --ast                 dump abstract syntax tree
      --symtab              dump symbol table
      --deps                dump dependency edges
      --tac                 dump unoptimized TAC
      --tac-opt             dump optimized TAC and statistics
      --phases              enable all dump flags above, with phase banners
      --no-color            disable ANSI colour
  -h, --help                usage
  -v, --version             print "configlang 1.0.0"
```
**Streams:** the generated artifact goes to **stdout** (or `-o`). Diagnostics and all dumps go to **stderr**.
**Exit codes:** `0` success (warnings allowed) · `1` compile errors · `2` usage or I/O error (`E901` unreadable file).

### 5.2 Input / output files
Input extension `.cfg` (not enforced). Output is UTF-8, ends with a single newline.

### 5.3 Dump formats (stable, used in demos and tests)

**Tokens**
```
LINE:COL   TOKEN         LEXEME
2:5        IDENT         workers
2:13       COLON         :
2:15       KW_INT        int
```
**AST** (two spaces per level)
```
Program
  Decl workers : int            [2:5]
    String "four"               [2:21]
```
**Symbol table**
```
PATH             KIND   DECL    TYPE    RANGE        LINE  VALUE
server           block  -       -       -            3     -
server.port      value  int     int     1..65535     4     8080
```
**TAC**
```
t1 = LOAD base_port
t2 = t1 ADD 80
STORE server.port, t2
```
**Optimized TAC**
```
STORE base_port, 8000
STORE server.port, 8080
; before: 9 instructions, after: 5 | folds: 3 | propagations: 2 | removed: 4
```

### 5.4 Diagnostic format
```
<file>:<line>:<col>: <error|warning>[<CODE>]: <message>
<line number right-aligned> | <source line>
<blank gutter> | <spaces to column><^ repeated over the span>
```
Example:
```
bad.cfg:2:21: error[E302]: type mismatch for 'server.workers': declared int, expression has type string
    2 |     workers : int = "four";
      |                     ^^^^^^
```
Final line: `1 error, 0 warnings generated.` (pluralised correctly). The first line of every diagnostic must match the regex  
`^[^:]+:[0-9]+:[0-9]+: (error|warning)\[[EW][0-9]{3}\]: .+$` — the test suite relies on it.

---

## 6. Diagnostic Catalogue

| Code | Phase | Meaning | Reported at |
|---|---|---|---|
| E101 | Lexer | Illegal character | the character |
| E102 | Lexer | Unterminated string literal | opening quote |
| E103 | Lexer | Integer literal out of range | the literal |
| E104 | Lexer | Invalid escape sequence | the backslash |
| E105 | Lexer | Unterminated block comment | the `/*` |
| E201 | Parser | Syntax error (unexpected token, expected list) | offending token |
| E301 | Semantic | Duplicate name in scope (cites first definition line) | second name |
| E302 | Semantic | Declared type does not match expression type | expression |
| E303 | Validation | Value outside declared range | expression |
| E304 | Semantic | Invalid range (lo>hi, non-numeric type, bound kind mismatch) | the range |
| E305 | Semantic | Undefined reference | the reference |
| E306 | Semantic | Circular reference (shows chain) | declaration that closes the cycle |
| E307 | Semantic | Invalid operand types for operator | the operator expression |
| E308 | Optimizer | Division by zero | the divisor expression |
| E309 | Semantic | Reference names a block, not a value | the reference |
| E310 | Optimizer/Validator | Arithmetic overflow, or non-finite float | the expression |
| E901 | Driver | Cannot read input file | (no source line) |
| W401 | Semantic | Empty block | block name |
| W402 | Semantic | Implicit int → float conversion | expression |

Messages must name the offending symbol path or token, the expectation, and what was found.

---

## 7. Non-Functional Requirements and Coding Standards

### 7.1 Quality attributes
| ID | Requirement |
|---|---|
| NFR-01 | **Correctness:** all tests in `tests/` pass via `make test`. |
| NFR-02 | **Robustness:** never crashes, hangs or reads out of bounds on any input (including empty, binary, huge, deeply nested). Verified with a fuzz-style test of random bytes. |
| NFR-03 | **Memory:** `valgrind --leak-check=full --error-exitcode=1` clean on every test (`make memcheck`). |
| NFR-04 | **Performance:** a 10,000-line valid file compiles in under 1 s. |
| NFR-05 | **Portability:** builds on Linux, macOS, WSL with `make`; no platform-specific code except GCC/Clang overflow builtins. |
| NFR-06 | **Determinism:** same input → byte-identical output and diagnostics order. |
| NFR-07 | **Nesting:** supports at least 100 nested blocks; deeper input yields a diagnostic, not a crash. |

### 7.2 Build requirements
- `make` builds `build/configlang`; `make test`, `make memcheck`, `make clean`, `make demo` (runs `examples/` with `--phases`).
- Hand-written code compiled with `-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror -g`.
- Flex/Bison-generated sources are compiled with the warnings that cannot be avoided relaxed *only for those files*.
- Bison run with `-Wall` and **no conflicts**.

### 7.3 Coding standards (strictly enforced; the evaluator is strict)
1. **File header** on every `.c`, `.h`, `.l`, `.y`: file name, module, compiler phase, purpose, course, author placeholders.
2. **Function comment** on every function: purpose, parameters, return value, ownership, errors.
3. Naming: `snake_case` functions and variables; `UPPER_SNAKE_CASE` macros and enum values; types end in `_t`; module prefixes `lex_ ast_ sym_ sem_ tac_ opt_ val_ gen_ diag_`.
4. Formatting: 4 spaces, no tabs (except Makefile), ≤ 100 columns, braces on every `if/else/for/while`.
5. Function length ≤ 50 lines; one responsibility per function; no deep nesting (> 4 levels).
6. **No magic numbers or strings:** use named constants or enums.
7. Memory: all AST and symbol data from a single **arena allocator** freed once; allocation failure handled via checked wrappers (`xmalloc`).
8. Safety: never `gets`, `strcpy`, `sprintf`, `strcat`; use bounded alternatives; check every return value.
9. Encapsulation: `static` for file-local helpers; headers expose only the public API; include guards; no global mutable state except what Flex/Bison force (wrap and document it) — preferred: reentrant/pure parser (`%define api.pure full`, `%option reentrant bison-bridge bison-locations`).
10. Phases talk only through public headers; no phase reaches into another phase's internals.
11. Comments explain *why*, not *what*; no dead or commented-out code; no TODO left in the final version.
12. Originality: written from scratch; no copied code; no vendored dependencies.

### 7.4 Required project deliverables
```
configlang/
├── Makefile
├── README.md                 build, usage, examples, phase-to-syllabus mapping
├── docs/
│   ├── SRS.md  PRD.md        (given)
│   ├── DESIGN.md             architecture, data structures, algorithms
│   ├── GRAMMAR.md            token table + grammar + precedence
│   ├── TEST_PLAN.md          test catalogue mapped to requirement IDs
│   ├── VIVA_NOTES.md         likely viva questions with short answers
│   └── DECISIONS.md          assumptions and design decisions log
├── src/
│   ├── main.c  lexer.l  parser.y
│   ├── ast.c/.h  symtab.c/.h  semantic.c/.h
│   ├── tac.c/.h  optimizer.c/.h  validator.c/.h
│   ├── codegen.c/.h  diag.c/.h  arena.c/.h  util.c/.h
├── tests/
│   ├── valid/      *.cfg + *.json (golden output, and *.yaml where applicable)
│   ├── invalid/    *.cfg + *.expected (one "LINE:COL CODE" per line)
│   └── run_tests.sh
└── examples/       server.cfg, bad_types.cfg, cycle.cfg, …
```

---

## 8. Verification

### 8.1 Test strategy
- **Golden tests (valid):** compile and diff against stored JSON (and YAML).
- **Negative tests (invalid):** extract `LINE:COL CODE` from stderr, diff against `.expected`.
- **Every diagnostic code E101–E310 and both warnings has at least one dedicated test; E201, E301, E302, E306 have at least two.**
- **Minimum counts:** 12 valid, 28 invalid, 1 empty-file test, 1 random-byte robustness test.
- Boundary cases: `INT64_MAX`, `INT64_MAX + 1`, range edges `lo` and `hi`, `lo-1`, `hi+1`, 100-level nesting, forward references, unary minus chains, `1..65535` lexing.

### 8.2 Traceability matrix (summary)
| Area | Requirements | Verified by |
|---|---|---|
| Lexer | FR-LEX-01…06 | `tests/invalid/lex_*`, `--tokens` golden |
| Parser/AST | FR-SYN-01…06 | `tests/invalid/syn_*`, `--ast` golden |
| Symbol table | FR-SYM-01…06 | `tests/invalid/dup_*`, `--symtab` golden |
| Semantics | FR-SEM-01…08 | `tests/invalid/sem_*` |
| TAC/Opt | FR-TAC, FR-OPT | `--tac`, `--tac-opt` golden, overflow tests |
| Validation | FR-VAL | `tests/invalid/range_*` |
| Codegen | FR-GEN | `tests/valid/*` |
| NFRs | NFR-01…07 | `make test`, `make memcheck`, fuzz test |

`docs/TEST_PLAN.md` must expand this into one row per test.

### 8.3 Acceptance
The product is accepted when every **M** requirement is implemented, all tests pass, `make memcheck` is clean, the build has zero warnings, and the demo script in PRD §9 runs cleanly.

---

## Appendix A. Bison grammar (normative)

```yacc
%token <str> IDENT STRING_LIT
%token <ival> INT_LIT
%token <fval> FLOAT_LIT
%token KW_INT KW_FLOAT KW_STRING KW_BOOL KW_TRUE KW_FALSE
%token LBRACE RBRACE LPAREN RPAREN ASSIGN COLON SEMI DOT DOTDOT AT
%token PLUS MINUS STAR SLASH

%left PLUS MINUS
%left STAR SLASH
%right UMINUS

%expect 0
%%
program     : item_list ;
item_list   : /* empty */ | item_list item ;
item        : block | declaration | error SEMI ;
block       : IDENT LBRACE item_list RBRACE ;
declaration : IDENT COLON type opt_range ASSIGN expr SEMI
            | IDENT ASSIGN expr SEMI ;
type        : KW_INT | KW_FLOAT | KW_STRING | KW_BOOL ;
opt_range   : /* empty */ | LPAREN bound DOTDOT bound RPAREN ;
bound       : INT_LIT | FLOAT_LIT | MINUS INT_LIT | MINUS FLOAT_LIT ;
expr        : expr PLUS expr | expr MINUS expr
            | expr STAR expr | expr SLASH expr
            | MINUS expr %prec UMINUS
            | LPAREN expr RPAREN
            | INT_LIT | FLOAT_LIT | STRING_LIT | KW_TRUE | KW_FALSE
            | reference ;
reference   : AT path ;
path        : IDENT | path DOT IDENT ;
```
The implementer may refactor non-terminal names but must keep the language and the zero-conflict property.

## Appendix B. Worked example (valid)

`examples/server.cfg`
```
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
Expected JSON:
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

## Appendix C. Worked examples (invalid)

| Source (relevant line) | Expected diagnostic |
|---|---|
| `workers : int = "four";` | `E302` at the string |
| `p : int(1..10) = 11;` | `E303` at `11` |
| `p : int(10..1) = 5;` | `E304` at the range |
| `p : string(1..2) = "x";` | `E304` at the range |
| `a = @missing;` | `E305` |
| `a = @b; b = @a;` | `E306`, chain `a -> b -> a` (reported once) |
| `a = "x" * 2;` | `E307` |
| `a = 1 / 0;` | `E308` |
| `s { x = 1; } y = @s;` | `E309` |
| `a = 9223372036854775807 + 1;` | `E310` |
| `a = 1; a = 2;` | `E301`, cites first line |
| `a = 99999999999999999999;` | `E103` |
| `a = "abc` | `E102` |
| `a = 1` (missing `;`) followed by another item | `E201`, then parsing recovers |
| `a = #;` | `E101` |
