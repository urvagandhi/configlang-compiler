/**
 * ============================================================================
 * Syntax Analyzer Interface (parser.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * SYLLABUS MAPPING:
 *   - Experiment 4: Design and implementation of Syntax Analyzer using Yacc / Bison
 *   - Experiment 5: Syntax Error Handling and Recovery using Error Productions
 *
 * WHAT THIS FILE DOES:
 *   Provides the entry point parser_parse() which invokes the Bison LALR(1)
 *   parser to construct the Abstract Syntax Tree (AST).
 * ============================================================================
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
