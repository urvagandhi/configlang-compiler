/**
 * ============================================================================
 * Code Optimizer Implementation (optimizer.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * SYLLABUS MAPPING:
 *   - Experiment 9: Design and implementation of Code Optimizer for given
 *     Three Address Code.
 *
 * HOW THE OPTIMIZER RUNS IN A SINGLE PASS:
 *   1. Maintains a map of known constants (const_map_t).
 *   2. For each TAC instruction:
 *      - Replaces temporary or variable arguments with known constants (propagation).
 *      - If both arguments are constants:
 *          Calls fold_binary() to compute the result at compile time (folding).
 *          Checks for division by zero (E308) and integer overflow (E310).
 *          Records the result in the constant map and removes the instruction.
 *      - If a STORE has a constant value: records final value for the variable.
 * ============================================================================
 */

#include "optimizer.h"
#include <math.h>
#include <string.h>

#define CONST_MAP_SIZE 2048U

typedef struct const_entry {
    char *name;
    sym_value_t val;
    struct const_entry *next;
} const_entry_t;

typedef struct const_map {
    arena_t *arena;
    const_entry_t *buckets[CONST_MAP_SIZE];
} const_map_t;

static uint32_t map_hash(const char *s) {
    uint32_t h = 5381U;
    int c;
    while ((c = *s++) != 0) {
        h = ((h << 5) + h) + (uint32_t)c;
    }
    return h % CONST_MAP_SIZE;
}

static void map_set(const_map_t *map, const char *name, sym_value_t val) {
    uint32_t b = map_hash(name);
    const_entry_t *e = (const_entry_t *)arena_alloc(map->arena, sizeof(const_entry_t));
    e->name = arena_strdup(map->arena, name);
    e->val = val;
    e->next = map->buckets[b];
    map->buckets[b] = e;
}

static bool map_get(const const_map_t *map, const char *name, sym_value_t *out_val) {
    uint32_t b = map_hash(name);
    for (const_entry_t *e = map->buckets[b]; e != NULL; e = e->next) {
        if (strcmp(e->name, name) == 0) {
            *out_val = e->val;
            return true;
        }
    }
    return false;
}

static bool fold_binary_int(tac_op_t op, int64_t a, int64_t b, int64_t *out,
                            diag_collector_t *diags, diag_loc_t loc) {
    if (op == TAC_ADD) {
        if (__builtin_add_overflow(a, b, out)) {
            diag_error(diags, DIAG_E310, loc, "integer overflow in addition");
            return false;
        }
    } else if (op == TAC_SUB) {
        if (__builtin_sub_overflow(a, b, out)) {
            diag_error(diags, DIAG_E310, loc, "integer overflow in subtraction");
            return false;
        }
    } else if (op == TAC_MUL) {
        if (__builtin_mul_overflow(a, b, out)) {
            diag_error(diags, DIAG_E310, loc, "integer overflow in multiplication");
            return false;
        }
    } else if (op == TAC_DIV) {
        if (b == 0) {
            diag_error(diags, DIAG_E308, loc, "division by zero");
            return false;
        }
        if (a == INT64_MIN && b == -1) {
            diag_error(diags, DIAG_E310, loc, "integer overflow in division");
            return false;
        }
        *out = a / b;
    }
    return true;
}

static bool fold_binary_float(tac_op_t op, double a, double b, double *out,
                              diag_collector_t *diags, diag_loc_t loc) {
    if (op == TAC_DIV && b == 0.0) {
        diag_error(diags, DIAG_E308, loc, "division by zero");
        return false;
    }
    double res = (op == TAC_ADD) ? (a + b) : (op == TAC_SUB) ? (a - b) :
                 (op == TAC_MUL) ? (a * b) : (a / b);
    if (!isfinite(res)) {
        diag_error(diags, DIAG_E310, loc, "non-finite floating point arithmetic result");
        return false;
    }
    *out = res;
    return true;
}

static bool fold_binary(arena_t *arena, tac_op_t op, sym_value_t v1, sym_value_t v2,
                        sym_value_t *out, diag_collector_t *diags, diag_loc_t loc) {
    if (v1.type == TYPE_INT && v2.type == TYPE_INT) {
        int64_t ires = 0;
        if (!fold_binary_int(op, v1.val.ival, v2.val.ival, &ires, diags, loc)) {
            return false;
        }
        out->has_value = true;
        out->type = TYPE_INT;
        out->val.ival = ires;
        return true;
    }
    if ((v1.type == TYPE_INT || v1.type == TYPE_FLOAT) &&
        (v2.type == TYPE_INT || v2.type == TYPE_FLOAT)) {
        double d1 = (v1.type == TYPE_INT) ? (double)v1.val.ival : v1.val.fval;
        double d2 = (v2.type == TYPE_INT) ? (double)v2.val.ival : v2.val.fval;
        double fres = 0.0;
        if (!fold_binary_float(op, d1, d2, &fres, diags, loc)) {
            return false;
        }
        out->has_value = true;
        out->type = TYPE_FLOAT;
        out->val.fval = fres;
        return true;
    }
    if (v1.type == TYPE_STRING && v2.type == TYPE_STRING && op == TAC_ADD) {
        size_t l1 = strlen(v1.val.sval);
        size_t l2 = strlen(v2.val.sval);
        char *cat = (char *)arena_alloc(arena, l1 + l2 + 1);
        memcpy(cat, v1.val.sval, l1);
        memcpy(cat + l1, v2.val.sval, l2 + 1);
        out->has_value = true;
        out->type = TYPE_STRING;
        out->val.sval = cat;
        return true;
    }
    return false;
}

static void try_propagate_operand(const_map_t *map, tac_operand_t *op,
                                 size_t *prop_count) {
    if (op->kind == TAC_OPERAND_TEMP || op->kind == TAC_OPERAND_VAR) {
        sym_value_t cval;
        if (op->name != NULL && map_get(map, op->name, &cval)) {
            op->kind = TAC_OPERAND_CONST;
            op->const_val = cval;
            (*prop_count)++;
        }
    }
}

opt_stats_t optimizer_run(tac_program_t *prog, diag_collector_t *diags) {
    opt_stats_t stats = {0};
    if (prog == NULL) {
        return stats;
    }
    stats.instructions_before = prog->count;

    const_map_t map;
    map.arena = prog->arena;
    memset(map.buckets, 0, sizeof(map.buckets));

    tac_instr_t *prev = NULL;
    tac_instr_t *cur = prog->head;

    while (cur != NULL) {
        try_propagate_operand(&map, &cur->arg1, &stats.propagations_count);
        try_propagate_operand(&map, &cur->arg2, &stats.propagations_count);

        if (cur->op == TAC_LOAD && cur->arg1.kind == TAC_OPERAND_CONST) {
            map_set(&map, cur->result.name, cur->arg1.const_val);
            stats.removed_count++;
            if (prev == NULL) {
                prog->head = cur->next;
            } else {
                prev->next = cur->next;
            }
            cur = cur->next;
            continue;
        }

        if (cur->op == TAC_STORE && cur->arg1.kind == TAC_OPERAND_CONST) {
            map_set(&map, cur->result.name, cur->arg1.const_val);
            prev = cur;
            cur = cur->next;
            continue;
        }

        if (cur->op == TAC_NEG && cur->arg1.kind == TAC_OPERAND_CONST) {
            sym_value_t neg_val = cur->arg1.const_val;
            if (neg_val.type == TYPE_INT) {
                neg_val.val.ival = -neg_val.val.ival;
            } else if (neg_val.type == TYPE_FLOAT) {
                neg_val.val.fval = -neg_val.val.fval;
            }
            map_set(&map, cur->result.name, neg_val);
            stats.folds_count++;
            stats.removed_count++;
            if (prev == NULL) {
                prog->head = cur->next;
            } else {
                prev->next = cur->next;
            }
            cur = cur->next;
            continue;
        }

        if (cur->arg1.kind == TAC_OPERAND_CONST && cur->arg2.kind == TAC_OPERAND_CONST) {
            sym_value_t folded = {0};
            if (fold_binary(prog->arena, cur->op, cur->arg1.const_val,
                            cur->arg2.const_val, &folded, diags, cur->loc)) {
                map_set(&map, cur->result.name, folded);
                stats.folds_count++;
                stats.removed_count++;
                if (prev == NULL) {
                    prog->head = cur->next;
                } else {
                    prev->next = cur->next;
                }
                cur = cur->next;
                continue;
            }
        }

        prev = cur;
        cur = cur->next;
    }

    size_t remaining = 0;
    prog->tail = NULL;
    for (tac_instr_t *i = prog->head; i != NULL; i = i->next) {
        remaining++;
        prog->tail = i;
    }
    prog->count = remaining;
    stats.instructions_after = remaining;
    return stats;
}

void optimizer_dump(const tac_program_t *prog, const opt_stats_t *stats, FILE *out) {
    if (prog == NULL || stats == NULL || out == NULL) {
        return;
    }
    tac_dump(prog, out);
    fprintf(out, "; before: %zu instructions, after: %zu | folds: %zu | propagations: %zu | removed: %zu\n",
            stats->instructions_before, stats->instructions_after,
            stats->folds_count, stats->propagations_count, stats->removed_count);
}
