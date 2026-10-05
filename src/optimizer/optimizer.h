/**
 * ============================================================================
 * Code Optimizer Interface (optimizer.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * SYLLABUS MAPPING:
 *   - Experiment 9: Design and implementation of Code Optimizer for given
 *     Three Address Code.
 *
 * OPTIMIZATIONS PERFORMED:
 *   1. Constant Propagation: Substitutes known constant values into operands.
 *   2. Constant Folding: Precomputes arithmetic expressions at compile time:
 *        t1 = 2 + 3  -->  t1 = 5
 *   3. Dead Code Elimination: Drops unused temporary instructions once folded.
 * ============================================================================
 */

#ifndef CONFIGLANG_OPTIMIZER_H
#define CONFIGLANG_OPTIMIZER_H

#include "diag.h"
#include "tac.h"
#include <stdio.h>

/** Optimization pass statistics. */
typedef struct opt_stats {
    size_t instructions_before;
    size_t instructions_after;
    size_t folds_count;
    size_t propagations_count;
    size_t removed_count;
} opt_stats_t;

/**
 * @brief Run optimization passes over TAC program.
 * @param prog Input TAC program (modified in place).
 * @param diags Collector for E308 (div by zero) and E310 (overflow).
 * @return Struct with optimization statistics.
 */
opt_stats_t optimizer_run(tac_program_t *prog, diag_collector_t *diags);

/**
 * @brief Dump optimized TAC and statistics banner per SRS §5.3.
 * @param prog Optimized TAC program.
 * @param stats Optimization statistics.
 * @param out Output stream.
 */
void optimizer_dump(const tac_program_t *prog, const opt_stats_t *stats, FILE *out);

#endif /* CONFIGLANG_OPTIMIZER_H */
