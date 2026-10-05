/**
 * ============================================================================
 * Code Optimizer (optimizer.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Implements Constant Folding and Constant Propagation on Three-Address Code.
 * ============================================================================
 */

#ifndef CONFIGLANG_OPTIMIZER_H
#define CONFIGLANG_OPTIMIZER_H

#include "tac.h"

/* Run optimization pass. Returns count of folded constants. */
int optimize_tac(void);

/* Print optimized TAC stream */
void optimizer_print(void);

#endif /* CONFIGLANG_OPTIMIZER_H */
