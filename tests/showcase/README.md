# ConfigLang Final Test Cases Showcase (Faculty & Viva Presentation)

**Course:** Principles of Compiler Design (`4CS501CC25`) — Semester-VII B.Tech CSE, Nirma University  
**Authors:** Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)  

---

## 🎯 Overview

This directory contains the **6 definitive, non-trivial test cases** selected and crafted to showcase the complete spectrum of the **ConfigLang** compiler to evaluation faculty ("mam").

Instead of toy 1-line programs (`a = 1;`), these test cases simulate realistic, modern cloud and microservice configurations while systematically verifying every phase of the compiler pipeline:
1. **Lexical Analysis (Flex)**: Token recognition, literal parsing, comment stripping, escape handling.
2. **Syntax Analysis (Bison & AST)**: LALR(1) grammar, block nesting, precedence, empty statement lists.
3. **Symbol Table Management**: Dotted canonical naming (`cluster.node_pool.compute.cores`), scope resolution.
4. **Semantic Analysis**: Type checking, type inference, undeclared symbol detection, self-cycles, duplicate keys.
5. **Intermediate Code Generation (TAC)**: Linearizing expression trees into 3-address quadruples (`LOAD`, `STORE`, `ADD`, `SUB`, `MUL`, `DIV`).
6. **Machine-Independent Optimization**: Constant propagation across multi-hop dependency chains and compile-time constant folding.
7. **Target Code Generation**: Emitting cleanly formatted, valid standard JSON.

---

## 📊 Test Case Spectrum & Matrix

| Test Case | Filename | Category | Key Compiler Concepts Demonstrated | Expected Status |
|:---|:---|:---|:---|:---:|
| **TC1** | `tc1_simple_baseline.cfg` | **Simple / Baseline** | Primitive types (`int`, `float`, `string`, `bool`), block syntax, single-line comments, basic binary TAC addition. | ✅ Passes (Valid JSON) |
| **TC2** | `tc2_feature_rich.cfg` | **Medium / Feature-Rich** | Type inference (without explicit annotations), cross-block `@` references, compile-time string concatenation (`+`). | ✅ Passes (Valid JSON) |
| **TC3** | `tc3_complex_hierarchy.cfg` | **Complex / Deep Hierarchy** | 3-level block nesting, 4-hop reference dependency chains, operator precedence `()`, `*`, `/`, `+`, `-`, float widening, 8+ constants folded. | ✅ Passes (Valid JSON) |
| **TC4** | `tc4_edge_cases.cfg` | **Edge Cases & Boundaries** | Unary minus expressions (`-(100-80)`), float widening, zero in numerator (`0/100`), empty blocks (`{}`), escape sequences (`\"`, `\n`, `\t`), multi-line block comments. | ✅ Passes (Valid JSON) |
| **TC5** | `tc5_semantic_errors.cfg` | **Semantic Diagnostics** | Symbol Table & Semantic validation: Duplicate declaration, type mismatch, undefined cross-scope reference, circular dependency. | ❌ Fails (5 Semantic Errors) |
| **TC6** | `tc6_opt_div_zero.cfg` | **Optimization Phase Error** | Dynamic / propagated division by zero (`5000 / 0`). Code passes Lexer, Parser, Symbol Table, and Semantic checks; error is detected exclusively during TAC Constant Propagation! | ❌ Fails (Arithmetic Error) |

---

## 🚀 How to Run the Showcase

### 1. Run All 6 Test Cases Automatically
```bash
make showcase
# or
bash tests/showcase/run_showcase.sh
```

### 2. Run Any Individual Test Case (Clean JSON Output)
```bash
./build/configlang tests/showcase/tc1_simple_baseline.cfg
./build/configlang tests/showcase/tc2_feature_rich.cfg
./build/configlang tests/showcase/tc3_complex_hierarchy.cfg
./build/configlang tests/showcase/tc4_edge_cases.cfg
./build/configlang tests/showcase/tc5_semantic_errors.cfg
./build/configlang tests/showcase/tc6_opt_div_zero.cfg
```

### 3. Step-by-Step 8-Phase Deep Dive (`--phases`)
To impress faculty during viva, run any test case with `--phases` to display the intermediate representation at each compiler phase:
```bash
./build/configlang --phases tests/showcase/tc3_complex_hierarchy.cfg
./build/configlang --phases tests/showcase/tc6_opt_div_zero.cfg
```

---

## 🔍 Detailed Test Case Explanations

### Test Case 1: Simple / Baseline (`tc1_simple_baseline.cfg`)
* **What it looks like:** A standard authentication microservice configuration.
* **Why it is not trivial:** It defines a complete microservice schema rather than isolated variables. It validates:
  - All four language data types: `string`, `int`, `float`, and `bool`.
  - Block hierarchies: `server { ... }` and `monitoring { ... }`.
  - Expression evaluation: `port : int = 8000 + 80;` is parsed into an AST binary operation node, lowered into TAC `8000 ADD 80`, folded to `8080`, and emitted to JSON.

### Test Case 2: Medium / Feature-Rich (`tc2_feature_rich.cfg`)
* **What it looks like:** Realistic database connection strings and API gateway bindings.
* **Compiler concepts:**
  - **Type Inference:** Declarations like `service = "analytics-engine";` omit the `: type` annotation. The semantic analyzer infers `string` from the expression.
  - **Cross-Block Variable References:** `@base_domain` is resolved from root scope into `database.host`, and `@database.host` is then referenced inside `database.url`.
  - **Compile-Time String Concatenation:** Demonstrates constant folding on strings:
    `"postgresql://" + @database.host + ":" + "5432/analytics"` folds cleanly into `"postgresql://db-primary.internal.net:5432/analytics"`.

### Test Case 3: Complex / Enterprise Cluster Hierarchy (`tc3_complex_hierarchy.cfg`)
* **What it looks like:** Kubernetes cloud cluster resource sizing and pod capacity calculation.
* **Compiler concepts:**
  - **Deep Structural Nesting:** 3 levels of blocks (`cluster -> node_pool -> compute`).
  - **Multi-Hop Reference Dependency Chains:**
    $$\text{cores\_per\_node} (16) \times \text{ram\_per\_core\_mb} (4096) \longrightarrow \text{total\_node\_ram\_mb} (65536)$$
    $$\text{total\_node\_ram\_mb} (65536) - 2048 \longrightarrow \text{usable\_ram\_mb} (63488)$$
    $$(\text{usable\_ram\_mb} (63488) / 1024) - 2 \longrightarrow \text{max\_pods\_per\_node} (60)$$
    $$\text{node\_count} (8) \times \text{max\_pods\_per\_node} (60) \longrightarrow \text{cluster\_pod\_capacity} (480)$$
  - **Multi-Pass Optimizer Convergence:** The optimizer runs multiple substitution passes until all dependent temporaries in TAC are fully resolved.
  - **Mixed-Type Widening:** `@total_cluster_cores * @base_core_cost + 2.50` widens `int` into `float` (`8.26`).

### Test Case 4: Edge Cases & Boundary Matrix (`tc4_edge_cases.cfg`)
* **What it looks like:** Boundary condition stress test.
* **Compiler concepts:**
  - **Unary Minus:** Tested both as a literal (`-25`) and over compound parenthesized expressions (`-(100 - 80)`). Lowered in TAC as `0 SUB (100 SUB 80) = -20`.
  - **Zero in Numerator:** `0 / 100` evaluates safely to `0` without triggering division-by-zero checks.
  - **Empty Blocks:** `empty_section { }` tests Bison grammar handling of empty `stmt_list` epsilon transitions.
  - **String Escapes:** Verifies lexer escape sequence preservation: `\"`, `\n`, `\t`, `\\`.
  - **Comments:** Verifies single-line `//` and multi-line `/* ... */` comment stripping without corrupting line numbers.

### Test Case 5: Semantic Analysis Diagnostics (`tc5_semantic_errors.cfg`)
* **What it looks like:** A malformed configuration containing standard developer errors.
* **Compiler concepts:**
  - **Type Narrowing Mismatch (`int = 30 * 100.00`):** `30` (int) $\times$ `100.00` (float) produces a `float` expression. Assigning this `float` to an `int` variable is rejected at compile time as a **Type Mismatch** to prevent unintended truncation/loss of precision (while the reverse widening `float = int` is permitted).
  - **Primitive Type Safety Violation:** `port : int = "eight-zero-eight-zero";` triggers type mismatch between string literal and int variable.
  - **Symbol Table Collision:** `max_threads : int = 8;` followed by `max_threads : int = 16;` triggers duplicate declaration error in the same scope.
  - **Scope Resolution Failure:** Referencing `@cache_cluster.primary_node` (undeclared symbol) triggers undefined reference error.
  - **Circular Dependency Detection:** `self_retry_limit : int = @server.self_retry_limit + 1;` detects cycle on self-reference.
  - **Graceful Error Recovery:** All errors are reported with exact line numbers before halting compilation.

### Test Case 6: Optimization Phase Error (`tc6_opt_div_zero.cfg`)
* **What it looks like:** An insidious bug where dynamic resource calculation results in division by zero.
* **Why this is the ultimate compiler showcase:**
  - In source code, the division is written as:
    `traffic_per_free_unit : int = 5000 / @gateway.free_units;`
  - It **passes Lexical Analysis** (valid tokens).
  - It **passes Syntax Analysis & AST generation** (valid grammar).
  - It **passes Symbol Table registration** (all symbols exist).
  - It **passes Semantic Analysis** (both operands are `int`, no type mismatch, no undefined references).
  - It **generates valid TAC**: `t8 = 5000 DIV t7`.
  - In **Phase 6 (Code Optimization)**, constant propagation resolves:
    `free_units = 100 - (50 + 50) = 0`.
  - When evaluating `5000 DIV 0`, the optimizer halts compilation with:
    `Semantic Error at line 18: Division by zero in expression '5000 / 0'`!
* **Viva Value:** Proves to faculty why intermediate representations and multi-phase optimization pipelines are necessary in modern compilers.
