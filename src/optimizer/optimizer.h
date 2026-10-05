/**
 * @file optimizer.h
 * @brief TAC optimizer: constant propagation, folding, and dead temporary removal.
 * @module optimizer
 * @phase Optimization
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
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
