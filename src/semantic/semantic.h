/**
 * @file semantic.h
 * @brief Semantic analysis pass: type checking, references, and cycle detection.
 * @module semantic
 * @phase Semantic Analysis
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#ifndef CONFIGLANG_SEMANTIC_H
#define CONFIGLANG_SEMANTIC_H

#include "ast.h"
#include "diag.h"
#include "symtab.h"
#include <stdio.h>

/** Directed dependency edge in symbol dependency graph. */
typedef struct dep_edge {
    symbol_t *from;
    symbol_t *to;
    struct dep_edge *next;
} dep_edge_t;

/** Semantic analysis context. */
typedef struct sem_context {
    symtab_t *symtab;
    diag_collector_t *diags;
    arena_t *arena;
    dep_edge_t *deps_head;
    dep_edge_t *deps_tail;
} sem_context_t;

/**
 * @brief Run full semantic analysis (Pass 2) over symbol table and AST.
 * @param ctx Pointer to semantic analysis context.
 * @param root Root AST node.
 * @return True if semantic checks passed with zero errors, false otherwise.
 */
bool semantic_analyze(sem_context_t *ctx, const ast_node_t *root);

/**
 * @brief Dump recorded dependency graph edges to output stream.
 * @param ctx Semantic analysis context.
 * @param out Output stream.
 */
void semantic_dump_deps(const sem_context_t *ctx, FILE *out);

#endif /* CONFIGLANG_SEMANTIC_H */
