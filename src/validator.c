/**
 * @file validator.c
 * @brief Implementation of range constraint and float finiteness validation.
 * @module validator
 * @phase Validation
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "validator.h"
#include <inttypes.h>
#include <math.h>

static void check_range_int(symbol_t *sym, const tac_instr_t *ins,
                            diag_collector_t *diags) {
    int64_t val = sym->final_val.val.ival;
    int64_t lo = (int64_t)sym->range.lo;
    int64_t hi = (int64_t)sym->range.hi;
    if (val < lo || val > hi) {
        diag_error(diags, DIAG_E303, ins->loc,
                   "value %" PRId64 " outside declared range [%" PRId64 "..%" PRId64 "] for '%s'",
                   val, lo, hi, sym->path);
    }
}

static void check_range_float(symbol_t *sym, const tac_instr_t *ins,
                              diag_collector_t *diags) {
    double val = (sym->final_val.type == TYPE_INT) ? (double)sym->final_val.val.ival
                                                   : sym->final_val.val.fval;
    if (val < sym->range.lo || val > sym->range.hi) {
        diag_error(diags, DIAG_E303, ins->loc,
                   "value %g outside declared range [%g..%g] for '%s'",
                   val, sym->range.lo, sym->range.hi, sym->path);
    }
}

bool validator_validate(const tac_program_t *prog, symtab_t *st, diag_collector_t *diags) {
    if (prog == NULL || st == NULL) {
        return false;
    }
    size_t err_start = diags->error_count;

    for (const tac_instr_t *ins = prog->head; ins != NULL; ins = ins->next) {
        if (ins->op != TAC_STORE || ins->result.name == NULL) {
            continue;
        }
        symbol_t *sym = symtab_lookup(st, ins->result.name);
        if (sym == NULL) {
            continue;
        }
        sym->final_val = ins->arg1.const_val;

        if (sym->final_val.type == TYPE_FLOAT && !isfinite(sym->final_val.val.fval)) {
            diag_error(diags, DIAG_E310, ins->loc,
                       "non-finite floating point value computed for '%s'", sym->path);
            continue;
        }

        if (sym->range.present) {
            if (sym->range.lo_is_float || sym->range.hi_is_float) {
                check_range_float(sym, ins, diags);
            } else {
                check_range_int(sym, ins, diags);
            }
        }
    }

    return (diags->error_count == err_start);
}
