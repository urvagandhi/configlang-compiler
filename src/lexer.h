/**
 * @file lexer.h
 * @brief Token definitions and lexical analysis interface.
 * @module lexer
 * @phase Lexical Analysis
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#ifndef CONFIGLANG_LEXER_H
#define CONFIGLANG_LEXER_H

#include "arena.h"
#include "diag.h"
#include <stdbool.h>
#include <stdint.h>

/** Token enumeration compatible with Bison LALR(1) numbering. */
typedef enum token_kind {
    TOK_EOF = 0,
    TOK_IDENT = 258,
    TOK_STRING_LIT = 259,
    TOK_INT_LIT = 260,
    TOK_FLOAT_LIT = 261,
    TOK_KW_INT = 262,
    TOK_KW_FLOAT = 263,
    TOK_KW_STRING = 264,
    TOK_KW_BOOL = 265,
    TOK_KW_TRUE = 266,
    TOK_KW_FALSE = 267,
    TOK_LBRACE = 268,
    TOK_RBRACE = 269,
    TOK_LPAREN = 270,
    TOK_RPAREN = 271,
    TOK_ASSIGN = 272,
    TOK_COLON = 273,
    TOK_SEMI = 274,
    TOK_DOT = 275,
    TOK_DOTDOT = 276,
    TOK_AT = 277,
    TOK_PLUS = 278,
    TOK_MINUS = 279,
    TOK_STAR = 280,
    TOK_SLASH = 281
} token_kind_t;

/** Semantic token value container. */
typedef union token_val {
    int64_t ival;
    double fval;
    char *sval;
} token_val_t;

/** Rich token instance carrying location and lexeme. */
typedef struct token {
    token_kind_t kind;
    diag_loc_t loc;
    char *lexeme;
    token_val_t val;
} token_t;

/** Context passed into pure/reentrant scanner. */
typedef struct lexer_extra {
    diag_collector_t *diags;
    arena_t *arena;
    const char *filename;
    size_t cur_line;
    size_t cur_col;
    diag_loc_t comment_start_loc;
} lexer_extra_t;

/**
 * @brief Get human-readable symbolic name for token kind.
 * @param kind Token type enumeration value.
 * @return String representation matching SRS §5.3.
 */
const char *lexer_token_name(token_kind_t kind);

/**
 * @brief Run lexical analysis over source text.
 * @param source Null-terminated source text.
 * @param filename File path for location reporting.
 * @param diags Diagnostic collector to record lexical errors.
 * @param arena Arena allocator for token string memory.
 * @param dump_tokens If true, prints token stream table to stderr per SRS §5.3.
 * @return True if no lexical errors were recorded, false otherwise.
 */
bool lexer_scan(const char *source, const char *filename,
                diag_collector_t *diags, arena_t *arena, bool dump_tokens);

#endif /* CONFIGLANG_LEXER_H */
