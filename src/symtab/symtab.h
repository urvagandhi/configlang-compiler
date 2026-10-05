/**
 * ============================================================================
 * Symbol Table (symtab.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Simple Symbol Table storing variable names, types, and values.
 * ============================================================================
 */

#ifndef CONFIGLANG_SYMTAB_H
#define CONFIGLANG_SYMTAB_H

#include "common.h"

/** A single record in our symbol table */
typedef struct Symbol {
    char name[64];           /* Variable name, e.g. "port" or "server.port" */
    DataType type;           /* Data type: int, float, string, bool */
    int ival;                /* Computed integer value */
    double fval;             /* Computed float value */
    char sval[128];          /* Computed string value */
    bool bval;               /* Computed boolean value */
    bool is_assigned;        /* Whether a value has been assigned */
    struct Symbol *next;     /* Next symbol in linked list */
} Symbol;

/* Initialize / clear symbol table */
void symtab_init(void);

/* Insert a new symbol; returns pointer to existing if duplicate */
Symbol *symtab_insert(const char *name, DataType type);

/* Look up a symbol by name; returns NULL if not found */
Symbol *symtab_lookup(const char *name);

/* Print all symbols in a clean ASCII table */
void symtab_print(void);

/* Get the head of the symbol list for code generation */
Symbol *symtab_get_all(void);

#endif /* CONFIGLANG_SYMTAB_H */
