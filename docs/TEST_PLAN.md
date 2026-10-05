# ConfigLang: Verification Matrix & Test Plan

**Course**: PCD (Principles of Compiler Design - 4CS501CC25)  
**Authors**: Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)  

---

## 1. Test Strategy & Philosophy

Verification of the ConfigLang compiler is automated via `tests/run_tests.sh` and GNU Make targets (`make test`, `make memcheck`, `make demo`). The test strategy comprises:

1. **Golden Output Verification**: Comparing compiler-generated JSON output against reference specifications byte-for-byte.
2. **Diagnostic Differential Testing**: Comparing line, column, severity, and error ID against `.expected` baseline files for every error code.
3. **Panic-Mode Recovery Verification**: Ensuring the parser recovers after invalid statements and continues scanning subsequent declarations.
4. **Memory Hygiene (Valgrind)**: Verifying zero memory leaks and zero invalid reads/writes on both valid and invalid compiler inputs.
5. **Stress & Fuzz Testing**: Submitting 100-level nested block structures and randomly generated binary byte sequences to guarantee parser robustness without segmentation faults.

---

## 2. Traceability Matrix

| Requirement ID | Requirement Description | Test File(s) | Status |
|:---|:---|:---|:---:|
| **FR-1** | Lexical tokenization & regex rules | `tests/valid/scalars.cfg`, `tests/valid/comments.cfg` | **PASS** |
| **FR-2** | String unescaping & escape error checking | `tests/valid/escape_sequences.cfg`, `tests/invalid/lex_bad_escape.cfg` | **PASS** |
| **FR-3** | Number parsing & 64-bit bounds checking | `tests/invalid/lex_int_overflow.cfg` | **PASS** |
| **FR-4** | Conflict-free LALR(1) parsing | `src/parser.y` (`%expect 0`), `tests/valid/nested_blocks.cfg` | **PASS** |
| **FR-5** | Syntax error recovery | `tests/invalid/syn_recovery.cfg`, `tests/invalid/syn_missing_semi.cfg` | **PASS** |
| **FR-6** | AST construction | `examples/server.cfg` (`--ast`) | **PASS** |
| **FR-7** | Scoped symbol table & duplicate rejection | `tests/invalid/sem_duplicate.cfg`, `tests/invalid/sem_duplicate_in_block.cfg` | **PASS** |
| **FR-8** | Type checking & inference | `tests/valid/inferred_types.cfg`, `tests/invalid/sem_type_mismatch.cfg` | **PASS** |
| **FR-9** | Range constraint validation | `tests/valid/range_checks.cfg`, `tests/invalid/val_range_exceeded.cfg`, `sem_bad_range.cfg` | **PASS** |
| **FR-10** | Reference resolution & block reference check | `tests/valid/references.cfg`, `tests/invalid/sem_undefined_ref.cfg`, `sem_block_ref.cfg` | **PASS** |
| **FR-11** | Dependency cycle detection | `tests/invalid/sem_cycle.cfg`, `tests/invalid/sem_cycle_self.cfg`, `sem_cycle_3node.cfg` | **PASS** |
| **FR-12** | Three-Address Code (TAC) generation | `examples/server.cfg` (`--tac`) | **PASS** |
| **FR-13** | Constant propagation & folding optimization | `tests/valid/optimizer_folding.cfg`, `examples/server.cfg` (`--tac-opt`) | **PASS** |
| **FR-14** | Compile-time division by zero detection | `tests/invalid/opt_div_zero.cfg`, `tests/invalid/opt_div_zero_float.cfg` | **PASS** |
| **FR-15** | Compile-time integer arithmetic overflow | `tests/invalid/opt_overflow.cfg`, `tests/invalid/opt_overflow_mul.cfg` | **PASS** |
| **FR-16** | JSON code generation | `examples/server.cfg` (`diff examples/server.json`) | **PASS** |
| **FR-17** | YAML code generation | `examples/server.cfg` (`-f yaml`) | **PASS** |
| **FR-18** | Clang/Rust diagnostic formatting | All `tests/invalid/*.expected` | **PASS** |
| **NFR-1** | Zero compiler warnings with strict flags | `Makefile` (`-Wall -Wextra -Wpedantic -Wconversion -Werror`) | **PASS** |
| **NFR-2** | Zero memory leaks under Valgrind | `make memcheck` | **PASS** |
| **NFR-3** | Functions $\le 50$ lines | Source code audit | **PASS** |
| **NFR-4** | Pure C11 (zero third-party dependencies) | Standard library only (`<stdio.h>`, `<stdlib.h>`, etc.) | **PASS** |
| **NFR-5** | Deterministic crash resilience under fuzzing | 30 random byte fuzz tests in `tests/run_tests.sh` | **PASS** |

---

## 3. Test Suite Inventory

### Valid Test Cases (14 Cases)
1. `examples/server.cfg`: Comprehensive multi-block server configuration.
2. `tests/valid/empty.cfg`: Empty file compiling to `{}`.
3. `tests/valid/empty_block.cfg`: Empty block generating warning `W401`.
4. `tests/valid/scalars.cfg`: All primitive scalar types (`int`, `float`, `string`, `bool`).
5. `tests/valid/arithmetic.cfg`: Arithmetic precedence, associativity, and parentheses.
6. `tests/valid/string_concat.cfg`: Multi-part string concatenation.
7. `tests/valid/references.cfg`: Local and root-scoped variable references.
8. `tests/valid/inferred_types.cfg`: Untyped assignment type inference.
9. `tests/valid/nested_blocks.cfg`: Multi-level block hierarchy.
10. `tests/valid/range_checks.cfg`: Numeric constraints satisfied within range.
11. `tests/valid/comments.cfg`: Single-line `//` and block `/* ... */` comments.
12. `tests/valid/escape_sequences.cfg`: Supported string escape characters.
13. `tests/valid/optimizer_folding.cfg`: Complex expressions fully folded by TAC optimizer.
14. `tests/valid/widening.cfg`: Implicit int-to-float widening warning `W402`.
15. `tests/valid/deep_nesting.cfg`: 100 levels of nested blocks.

### Invalid Test Cases (35 Cases)
- **Lexical (E101–E105)**: `lex_illegal_char`, `lex_illegal_char2`, `lex_unterminated_str`, `lex_int_overflow`, `lex_bad_escape`, `lex_bad_escape2`, `lex_unterminated_comment`.
- **Syntax (E201)**: `syn_bad_token`, `syn_missing_semi`, `syn_missing_brace`, `syn_missing_equal`, `syn_bad_expr`, `syn_recovery`.
- **Semantic (E301–E309)**: `sem_duplicate`, `sem_duplicate_in_block`, `sem_type_mismatch`, `sem_type_mismatch_bool`, `sem_type_mismatch_float`, `sem_bad_range`, `sem_bad_range_type`, `sem_undefined_ref`, `sem_undefined_nested`, `sem_cycle`, `sem_cycle_self`, `sem_cycle_3node`, `sem_invalid_operand`, `sem_invalid_operand_unary`, `sem_block_ref`.
- **Validation & Optimizer (E303, E308, E310)**: `val_range_exceeded`, `val_range_exceeded_lo`, `val_range_exceeded_float`, `opt_div_zero`, `opt_div_zero_float`, `opt_overflow`, `opt_overflow_mul`.
