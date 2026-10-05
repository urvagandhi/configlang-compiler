# Product Requirements Document (PRD)

## ConfigLang: A Typed Configuration Language Compiler with Semantic Validation

| Field | Value |
|---|---|
| Course | PCD (Compiler Design), 4CS501CC25, Innovative Assignment |
| Version | 1.0 |
| Owner | Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077) |
| Companion | `docs/SRS.md` is the technical source of truth. If this PRD and the SRS disagree on behaviour, the SRS wins; on scope and priority, this PRD wins. |
| Submission deadline | **10 October 2026** |

---

## 1. One-line description (for the professor)
A lightweight compiler for a custom, typed configuration language that performs lexical analysis, syntax analysis, AST generation, symbol-table construction, semantic validation, three-address-code generation, optimization, and emits standard JSON/YAML configuration files.

## 2. Problem statement
Configuration files (JSON, YAML, TOML) are only data. They cannot declare a type or a valid range for a value, cannot reference other values, and cannot compute derived values. Mistakes such as `workers: "four"`, a port of `70000`, or two keys with the same name are found only at runtime, often in production.

## 3. Proposed solution and novelty
**ConfigLang** adds compiler technology to configuration management. A config author writes a typed source file; the compiler validates it *before* anything runs and then generates a clean standard file.

**Novelty statement (use in the report):**
> ConfigLang applies compiler-design techniques to configuration management. It introduces a typed configuration language with range constraints, forward references and constant expressions. A symbol table with dependency tracking drives semantic validation, including type checking and circular-reference detection. An intermediate representation (TAC) is optimized by constant propagation and folding, and the result is generated as standard JSON or YAML.

**What is genuinely different from a "config parser":**
| Feature | Plain JSON/YAML parser | ConfigLang |
|---|---|---|
| Type annotations (`workers : int`) | No | Yes, checked at compile time |
| Range constraints (`int(1..65535)`) | No | Yes |
| References between values (`@server.port`) | No | Yes, with cycle detection |
| Derived values (`@a * 250`) | No | Yes, folded at compile time |
| Duplicate-key detection | Often silently accepted | Compile error with both locations |
| Multi-error diagnostics with source caret | Rarely | Yes |
| Multi-phase compiler, each phase inspectable | No | Yes (`--phases`) |

Honest positioning: tools such as HCL, CUE, Dhall and Jsonnet exist. ConfigLang is an **educational compiler**, deliberately small enough to be completely understood, and its value is demonstrating every classical compiler phase on a real, useful domain. The existing project list already contains a Domain-Specific Language for expense rules, so ConfigLang's domain (configuration validation and generation, with types and references) is clearly different.

## 4. Goals and success metrics
| Goal | Metric (measurable) |
|---|---|
| Demonstrate every classical compiler phase | `--phases` prints token stream, AST, symbol table, dependencies, TAC, optimized TAC, output |
| Correctness | 100% of tests pass (`make test`), at least 12 valid and 28 invalid |
| Code quality (strict evaluator) | Zero compiler warnings with `-Wall -Wextra -Wpedantic -Werror`; zero Bison conflicts; follows SRS §7.3 |
| Memory safety | `make memcheck` clean (no leaks, no invalid accesses) |
| Quality of errors | Every error shows file, line, column, code, message, source line and caret |
| Viva readiness | `docs/VIVA_NOTES.md` covers every phase; the student can explain each file |

## 5. Non-goals (explicitly out of scope)
Arrays and lists; imports/includes; environment-variable substitution; a runtime or interpreter; web UI; IDE plugin; schema files; TOML or `.env` output; Unicode identifiers; multi-file compilation. These may be listed as **future work** in the report only.

## 6. Users and stories
| Persona | Story | Acceptance |
|---|---|---|
| Config author | I write `port : int(1..65535) = 70000;` and get a precise error with line, column and caret. | `E303` shown, exit code 1, no output file |
| Config author | I define `max_connections = @server.workers * 250;` and the output has the computed number. | JSON contains `1000` |
| Config author | I reference a value defined later in the file. | Compiles successfully |
| Config author | I accidentally create `a = @b; b = @a;`. | One `E306` showing the chain |
| Config author | I get several errors in one run, not just the first. | All reported, sorted by position |
| Evaluator | I see each compiler phase's output separately. | `--tokens`, `--ast`, `--symtab`, `--tac`, `--tac-opt` each work |
| Evaluator | I can build and run with two commands. | `make && make test` |
| Student | I can explain the whole project in a viva. | `VIVA_NOTES.md` and `DESIGN.md` exist and match the code |

## 7. Feature list (MoSCoW)
| ID | Feature | Priority |
|---|---|---|
| F1 | Lexer with accurate positions and lexical errors | Must |
| F2 | LALR(1) parser, zero conflicts, error recovery, multi-error | Must |
| F3 | AST with dump | Must |
| F4 | Symbol table (hash with chaining + scope tree), duplicate detection, dump | Must |
| F5 | Semantic analysis: types, references, cycles, ranges, operator typing | Must |
| F6 | TAC generation in dependency order, dump | Must |
| F7 | Optimizer: constant propagation + folding + dead temp removal, dump with statistics | Must |
| F8 | Validator: range, overflow, div-by-zero | Must |
| F9 | JSON generator | Must |
| F10 | Diagnostics with caret rendering | Must |
| F11 | Test suite, `make test`, `make memcheck` | Must |
| F12 | Documentation: README, DESIGN, GRAMMAR, TEST_PLAN, VIVA_NOTES, DECISIONS | Must |
| F13 | YAML generator | Should |
| F14 | Warnings W401/W402, `--deps` dump | Should |
| F15 | Coloured diagnostics | Should |
| F16 | Arrays (`[1,2,3]`) with homogeneous type check | Could, *only if everything above is complete and the student asks* |

## 8. Plan (5 working days; today is 5 Oct 2026)
| Day | Date | Milestone | Exit gate |
|---|---|---|---|
| 1 | Mon 5 Oct | **M0** Scaffold, Makefile, arena, diag, utilities. **M1** Lexer + `--tokens` + lexer tests. | Lexer tests pass |
| 2 | Tue 6 Oct | **M2** Parser + AST + `--ast`, error recovery, syntax tests. | Zero Bison conflicts; syntax tests pass |
| 3 | Wed 7 Oct | **M3** Symbol table + semantic analysis + `--symtab`/`--deps`. | Semantic tests pass |
| 4 | Thu 8 Oct | **M4** TAC + optimizer + validator + JSON/YAML codegen. | All valid tests produce golden output |
| 5 | Fri 9 Oct | **M5** Full test suite, memcheck, docs, demo rehearsal. Buffer for fixes. | Definition of Done met |
| – | Sat 10 Oct | Submission. | – |

If a milestone slips, cut **Should** items first (F13–F15). **Must** items are never cut.

## 9. Demo and viva plan (5 minutes)
1. Show `examples/server.cfg`. Run `configlang examples/server.cfg --phases` and narrate each phase.
2. Show the generated JSON (computed values `8080`, `1000`).
3. Run `examples/bad_types.cfg`: multi-error output with carets.
4. Run `examples/cycle.cfg`: circular reference chain.
5. Run `make test` and `make memcheck`.
6. Open `docs/GRAMMAR.md`, explain the grammar and why it is LALR(1) with zero conflicts.

## 10. Risks and mitigations
| Risk | Impact | Mitigation |
|---|---|---|
| Flex/Bison not installed (especially on Windows) | Cannot build | Agent checks the toolchain first; use WSL2 or MSYS2; the agent gives the install command |
| Grammar conflicts | Time loss | `%expect 0` from the start; fix conflicts before moving on |
| Column tracking wrong with tabs/comments | Bad diagnostics | Central `YY_USER_ACTION`; dedicated position tests |
| Scope creep (arrays, web UI) | Misses deadline | Non-goals in §5; F16 gated |
| Hidden undefined behaviour in 64-bit arithmetic | Wrong output/crash | Checked arithmetic; boundary tests |
| Code looks "AI-generated" or copied | Originality penalty | Student reads and understands every file; `VIVA_NOTES.md`; no copied repository code |
| Strict evaluator rejects style | Marks lost | SRS §7.3 enforced by warnings-as-errors and a self-review checklist |

## 11. Deliverables
Source code and Makefile; tests with golden files; `README.md`; `docs/` (SRS, PRD, DESIGN, GRAMMAR, TEST_PLAN, VIVA_NOTES, DECISIONS); `examples/`; a short report using §1 and §3 text (the student writes the final report).

## 12. Definition of Done
- [ ] Every **Must** feature implemented per the SRS
- [ ] `make` builds with zero warnings; Bison reports zero conflicts
- [ ] `make test` passes (≥ 12 valid, ≥ 28 invalid, empty-file, random-byte tests)
- [ ] `make memcheck` clean
- [ ] `--phases` demo runs on `examples/server.cfg` exactly as in §9
- [ ] All six documents exist and match the code
- [ ] Every file has the required header; no TODOs, no dead code
- [ ] The student has read `VIVA_NOTES.md` and can answer from it

## 13. Assumptions and open items
- Host OS is assumed Linux, macOS, or WSL; if Windows-native, use WSL2.
- Report format (Word/PDF) is the student's responsibility.
- If the professor requires a different tool (for example Python/PLY), this PRD still applies; only SRS §7.2 changes.
