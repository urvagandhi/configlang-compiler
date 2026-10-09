/**
 * ============================================================================
 * Three-Address Code (tac.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Intermediate Representation using 3-operand quadruples:
 *   Result = Arg1 OP Arg2
 * ============================================================================
 */

#ifndef CONFIGLANG_TAC_H
#define CONFIGLANG_TAC_H

#include "ast.h"

/** A single Three-Address Code instruction (Quadruple) */
typedef struct TAC {
    char op[16];          /* "ADD", "SUB", "MUL", "DIV", "STORE", "LOAD" */
    char arg1[64];        /* First operand */
    char arg2[64];        /* Second operand */
    char result[64];      /* Target variable or temporary: t1, t2, ... */
    int line;             /* Source line number */
    struct TAC *next;     /* Next instruction */
} TAC;

/* Initialize / clear TAC instruction list */
void tac_init(void);

/* Generate a fresh temporary variable name: "t1", "t2", ... */
char *tac_new_temp(void);

/* Append an instruction to the TAC list */
void tac_emit(const char *op, const char *arg1, const char *arg2, const char *result, int line);

/* Generate TAC instructions from the AST */
void tac_generate(ASTNode *root, const char *prefix);

/* Print all TAC instructions */
void tac_print(void);

/* Get the first TAC instruction */
TAC *tac_get_head(void);

/* Set the first TAC instruction */
void tac_set_head(TAC *head);

#endif /* CONFIGLANG_TAC_H */
