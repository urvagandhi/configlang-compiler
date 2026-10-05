/**
 * @file parser.h
 * @brief Public interface to the ConfigLang Bison LALR(1) parser.
 * @module parser
 * @phase Syntax Analysis
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#ifndef CONFIGLANG_PARSER_H
#define CONFIGLANG_PARSER_H

#include "arena.h"
#include "ast.h"
#include "diag.h"

/**
 * @brief Parse source text and construct AST.
 * @param source Null-terminated source code string.
 * @param filename File path for diagnostic locations.
 * @param diags Collector for syntax errors (E201).
 * @param arena Memory arena for AST allocation.
 * @return Root AST node on success, or NULL if parsing failed with errors.
 */
ast_node_t *parser_parse(const char *source, const char *filename,
                         diag_collector_t *diags, arena_t *arena);

#endif /* CONFIGLANG_PARSER_H */
