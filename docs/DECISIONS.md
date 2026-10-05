# Design Decisions and Architecture Log

## ConfigLang: A Typed Configuration Language Compiler with Semantic Validation
**Course:** PCD (Principles of Compiler Design), 4CS501CC25  
**Baseline Date:** 5 October 2026

---

### Decision 1: Foundation Strategy (Clean Specification vs. External Repository)
* **Date:** 5 October 2026
* **Status:** Approved
* **Context:** Evaluated existing open-source configuration and educational compiler repositories (Orbit in Rust, Corn in Rust, ICL in C++, Toy-C in Flex/Bison) to assess if one can serve as a direct foundation.
* **Decision:** Implement ConfigLang from a clean modular C11 foundation directly conforming to `docs/SRS.md` rather than cloning or vendoring third-party repositories.
* **Rationale:**
  1. **Academic Originality:** `SRS.md` §2.4, §7.3(12) and `PRD.md` §10 strictly require original code to avoid plagiarism penalties during university evaluation.
  2. **Specification Match:** The grammar in `SRS.md` Appendix A is already fully specified and conflict-free (`%expect 0`). Repositories designed for general programming languages (loops, functions, pointers) require more effort to dismantle than building the exact typed configuration DSL.
  3. **Quality Gates:** Third-party educational repositories almost universally fail strict `-Wall -Wextra -Wpedantic -Wconversion -Werror` and leak memory under Valgrind.
  4. **Viva Defensibility:** The student must explain every module, AST structure, and pass in the oral exam.

---

### Decision 2: Toolchain & Build Environment
* **Date:** 5 October 2026
* **Status:** Approved
* **Context:** Verified host environment (Linux x86_64).
* **Findings:**
  - `flex 2.6.4` (supported, $\ge 2.6$)
  - `bison 3.8.2` (supported, $\ge 3.0$)
  - `gcc 13.3.0` (supported, $\ge 9.0$)
  - `GNU Make 4.3` (supported)
  - `valgrind` needs installation via `sudo apt install valgrind` for M5 memory verification.

---

### Decision 3: Memory Management (Arena Allocator)
* **Date:** 5 October 2026
* **Status:** Approved
* **Decision:** Use a single contiguous block / chunked arena allocator (`src/arena.c`, `src/arena.h`) for all AST nodes, symbol records, and TAC instructions.
* **Rationale:** Eliminates manual recursive tree frees, prevents memory leaks by design, and guarantees clean Valgrind runs (`make memcheck`). Individual allocations do not require per-node `free()`; the entire arena is destroyed once at compiler shutdown.

---

### Decision 4: Lexer & Parser Reentrancy
* **Date:** 5 October 2026
* **Status:** Approved
* **Decision:** Use pure/reentrant Flex and Bison options (`%define api.pure full`, `%locations`, `%option reentrant bison-bridge bison-locations`).
* **Rationale:** Prevents global state pollution, enables clean error collection, and satisfies SRS §7.3 Rule 9.
