/**
 * ============================================================================
 * Three-Address Code (TAC) Interface (tac.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * SYLLABUS MAPPING:
 *   - Experiment 7: Design and implementation of Intermediate Code Generator
 *     (Three Address Code) for given expressions.
 *
 * WHAT IS THREE-ADDRESS CODE (TAC)?
 *   TAC is a linear intermediate representation where each instruction has
 *   at most one operator and at most three operands:
 *     Example: x = (a + b) * c
 *     Becomes:
 *       t1 = a + b      (arg1 + arg2 -> result)
 *       t2 = t1 * c     (arg1 * arg2 -> result)
 *       STORE x, t2     (store result into variable)
 * ============================================================================
 */

#ifndef CONFIGLANG_TAC_H
#define CONFIGLANG_TAC_H

#include "arena.h"
#include "ast.h"
#include "diag.h"
#include "symtab.h"
#include <stdbool.h>
#include <stdio.h>

/** TAC opcode matching SRS §4.5 FR-TAC-02. */
typedef enum tac_op {
    TAC_ADD,
    TAC_SUB,
    TAC_MUL,
    TAC_DIV,
    TAC_NEG,
    TAC_CONCAT,
    TAC_LOAD,
    TAC_STORE
} tac_op_t;

/** TAC operand kind. */
typedef enum tac_operand_kind {
    TAC_OPERAND_NONE = 0,
    TAC_OPERAND_CONST,
    TAC_OPERAND_TEMP,
    TAC_OPERAND_VAR
} tac_operand_kind_t;

/** Operand structure. */
typedef struct tac_operand {
    tac_operand_kind_t kind;
    char *name;               /**< Variable name or temp id (e.g. "t1"). */
    sym_value_t const_val;    /**< Literal constant value. */
} tac_operand_t;

/** Single Three-Address Code instruction. */
typedef struct tac_instr {
    tac_op_t op;
    tac_operand_t arg1;
    tac_operand_t arg2;
    tac_operand_t result;
    diag_loc_t loc;
    struct tac_instr *next;
} tac_instr_t;

/** Linear sequence of TAC instructions. */
typedef struct tac_program {
    arena_t *arena;
    tac_instr_t *head;
    tac_instr_t *tail;
    size_t count;
    size_t next_temp_id;
} tac_program_t;

tac_program_t *tac_create_program(arena_t *arena);
tac_operand_t tac_new_temp(tac_program_t *prog);
tac_operand_t tac_const_int(int64_t val);
tac_operand_t tac_const_float(double val);
tac_operand_t tac_const_string(arena_t *arena, const char *str);
tac_operand_t tac_const_bool(bool val);
tac_operand_t tac_var(arena_t *arena, const char *path);

void tac_emit(tac_program_t *prog, tac_op_t op, tac_operand_t arg1,
              tac_operand_t arg2, tac_operand_t result, diag_loc_t loc);

tac_program_t *tac_generate(arena_t *arena, const symtab_t *st);
void tac_dump(const tac_program_t *prog, FILE *out);

#endif /* CONFIGLANG_TAC_H */
