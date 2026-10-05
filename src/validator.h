/**
 * @file validator.h
 * @brief Range constraint validation and final value recording.
 * @module validator
 * @phase Validation
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#ifndef CONFIGLANG_VALIDATOR_H
#define CONFIGLANG_VALIDATOR_H

#include "diag.h"
#include "symtab.h"
#include "tac.h"
#include <stdbool.h>

/**
 * @brief Validate optimized TAC values against declared symbol ranges.
 * @param prog Optimized TAC program.
 * @param st Symbol table (records final values into symbols).
 * @param diags Collector for E303 range validation errors.
 * @return True if all values are valid and within range, false otherwise.
 */
bool validator_validate(const tac_program_t *prog, symtab_t *st, diag_collector_t *diags);

#endif /* CONFIGLANG_VALIDATOR_H */
