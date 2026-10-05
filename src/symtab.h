/**
 * @file symtab.h
 * @brief Symbol table with hash table lookup and ordered scope tree.
 * @module symtab
 * @phase Semantic Analysis
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#ifndef CONFIGLANG_SYMTAB_H
#define CONFIGLANG_SYMTAB_H

#include "arena.h"
#include "ast.h"
#include "diag.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SYMTAB_BUCKETS 1024U

/** Kind of symbol record. */
typedef enum symbol_kind {
    SYM_BLOCK,
    SYM_VALUE
} symbol_kind_t;

/** Symbol resolution/visit state for DFS cycle detection. */
typedef enum sym_state {
    STATE_UNVISITED = 0,
    STATE_VISITING = 1,
    STATE_DONE = 2
} sym_state_t;

/** Computed value holder for final evaluation. */
typedef struct sym_value {
    bool has_value;
    data_type_t type;
    union {
        int64_t ival;
        double fval;
        char *sval;
        bool bval;
    } val;
} sym_value_t;

typedef struct symbol symbol_t;

/** Symbol record matching SRS §4.3 FR-SYM-03. */
struct symbol {
    char *name;               /**< Local identifier. */
    char *path;               /**< Full dotted canonical path. */
    symbol_kind_t kind;       /**< SYM_BLOCK or SYM_VALUE. */
    data_type_t declared_type;
    data_type_t effective_type;
    ast_range_t range;
    ast_node_t *decl_node;
    size_t line;
    size_t col;
    sym_state_t state;
    sym_value_t final_val;

    symbol_t *parent;         /**< Enclosing scope block, or NULL for root. */
    symbol_t *children_head;  /**< Scope tree: first child in declaration order. */
    symbol_t *children_tail;  /**< Scope tree: last child in declaration order. */
    symbol_t *next_child;     /**< Next sibling in scope declaration order. */

    symbol_t *hash_next;      /**< Hash table collision chain. */
};

/** Symbol table container. */
typedef struct symtab {
    arena_t *arena;
    const char *filename;
    symbol_t *root_block;
    symbol_t *buckets[SYMTAB_BUCKETS];
    size_t symbol_count;
} symtab_t;

/**
 * @brief Initialize a new symbol table in arena memory.
 * @param arena Arena memory allocator.
 * @param filename Source file path.
 * @return Initialized symbol table pointer.
 */
symtab_t *symtab_create(arena_t *arena, const char *filename);

/**
 * @brief Look up a symbol by its full dotted path.
 * @param st Symbol table.
 * @param path Full dotted path string (e.g. "server.port").
 * @return Pointer to symbol record, or NULL if not found.
 */
symbol_t *symtab_lookup(const symtab_t *st, const char *path);

/**
 * @brief Look up a child symbol by local name within an enclosing block.
 * @param parent Enclosing block symbol.
 * @param name Local identifier.
 * @return Pointer to child symbol, or NULL if not found.
 */
symbol_t *symtab_lookup_local(const symbol_t *parent, const char *name);

/**
 * @brief Insert a new symbol into symbol table and scope tree.
 * @param st Symbol table.
 * @param parent Enclosing parent block symbol (or NULL for root).
 * @param name Local name.
 * @param kind SYM_BLOCK or SYM_VALUE.
 * @param decl_type Declared type.
 * @param range Range specification.
 * @param node AST node of declaration.
 * @param line Source line.
 * @param col Source column.
 * @param diags Collector to record E301 duplicate errors.
 * @return Newly created symbol pointer, or NULL on duplicate.
 */
symbol_t *symtab_insert(symtab_t *st, symbol_t *parent, const char *name,
                        symbol_kind_t kind, data_type_t decl_type,
                        ast_range_t range, ast_node_t *node, size_t line,
                        size_t col, diag_collector_t *diags);

/**
 * @brief Pass 1: Build the complete symbol table from AST.
 * @param st Symbol table.
 * @param root Root AST node (AST_PROGRAM).
 * @param diags Diagnostic collector.
 */
void symtab_build_pass1(symtab_t *st, const ast_node_t *root, diag_collector_t *diags);

/**
 * @brief Dump symbol table in SRS §5.3 format.
 * @param st Symbol table.
 * @param out Output stream.
 */
void symtab_dump(const symtab_t *st, FILE *out);

#endif /* CONFIGLANG_SYMTAB_H */
