/**
 * ============================================================================
 * Abstract Syntax Tree Data Structures (ast.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * WHAT IS AN ABSTRACT SYNTAX TREE (AST)?
 *   After the parser recognizes tokens according to grammar rules, it builds
 *   an in-memory tree representation of the program:
 *     - AST_PROGRAM  : Top-level root holding a list of items
 *     - AST_BLOCK    : Scope block with child items, e.g. server { ... }
 *     - AST_DECL     : Variable declaration, e.g. port: int(1024..65535) = 8080;
 *     - AST_BINARY   : Binary math expression (+, -, *, /)
 *     - AST_UNARY    : Unary negation (-x)
 *     - AST_REF      : Variable reference (@server.host)
 *     - AST_LIT_*    : Literal values (integers, floats, strings, booleans)
 * ============================================================================
 */

#ifndef CONFIGLANG_AST_H
#define CONFIGLANG_AST_H

#include "arena.h"
#include "diag.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/** Supported data types in ConfigLang. */
typedef enum data_type {
    TYPE_NONE = 0,    /**< Untyped assignment (inferred from expression) */
    TYPE_INT = 1,     /**< Signed 64-bit integer */
    TYPE_FLOAT = 2,   /**< IEEE double-precision float */
    TYPE_STRING = 3,  /**< UTF-8 string */
    TYPE_BOOL = 4,    /**< Boolean true/false */
    TYPE_ERROR = 5    /**< Sentinel poison type for error recovery */
} data_type_t;

/** Binary operators. */
typedef enum bin_op {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV
} bin_op_t;

/** Unary operators. */
typedef enum un_op {
    UN_NEG
} un_op_t;

/** Node kinds matching SRS §4.2. */
typedef enum ast_kind {
    AST_PROGRAM,
    AST_BLOCK,
    AST_DECL,
    AST_LIT_INT,
    AST_LIT_FLOAT,
    AST_LIT_STRING,
    AST_LIT_BOOL,
    AST_REF,
    AST_BINARY,
    AST_UNARY
} ast_kind_t;

/** Forward declaration of AST node. */
typedef struct ast_node ast_node_t;

/** Linked list element of items within a block or program. */
typedef struct ast_item {
    ast_node_t *node;
    struct ast_item *next;
} ast_item_t;

/** Range constraint bounds. */
typedef struct ast_range {
    bool present;
    bool lo_is_float;
    bool hi_is_float;
    double lo;
    double hi;
    diag_loc_t loc;
} ast_range_t;

/** AST Node structure with tagged union. */
struct ast_node {
    ast_kind_t kind;
    diag_loc_t loc;
    union {
        struct {
            ast_item_t *items_head;
            ast_item_t *items_tail;
        } program;
        struct {
            char *name;
            ast_item_t *items_head;
            ast_item_t *items_tail;
        } block;
        struct {
            char *name;
            data_type_t declared_type;
            ast_range_t range;
            ast_node_t *expr;
        } decl;
        struct {
            int64_t ival;
        } lit_int;
        struct {
            double fval;
        } lit_float;
        struct {
            char *sval;
        } lit_string;
        struct {
            bool bval;
        } lit_bool;
        struct {
            char *path;
        } ref;
        struct {
            bin_op_t op;
            ast_node_t *left;
            ast_node_t *right;
        } binary;
        struct {
            un_op_t op;
            ast_node_t *operand;
        } unary;
    } data;
};

const char *ast_type_to_string(data_type_t type);

ast_node_t *ast_create_program(arena_t *arena, diag_loc_t loc);
ast_node_t *ast_create_block(arena_t *arena, const char *name, diag_loc_t loc);
ast_node_t *ast_create_decl(arena_t *arena, const char *name, data_type_t type,
                            ast_range_t range, ast_node_t *expr, diag_loc_t loc);
ast_node_t *ast_create_int(arena_t *arena, int64_t val, diag_loc_t loc);
ast_node_t *ast_create_float(arena_t *arena, double val, diag_loc_t loc);
ast_node_t *ast_create_string(arena_t *arena, const char *str, diag_loc_t loc);
ast_node_t *ast_create_bool(arena_t *arena, bool val, diag_loc_t loc);
ast_node_t *ast_create_ref(arena_t *arena, const char *path, diag_loc_t loc);
ast_node_t *ast_create_binary(arena_t *arena, bin_op_t op, ast_node_t *left,
                              ast_node_t *right, diag_loc_t loc);
ast_node_t *ast_create_unary(arena_t *arena, un_op_t op, ast_node_t *operand,
                             diag_loc_t loc);

void ast_append_item(arena_t *arena, ast_node_t *container, ast_node_t *item);
void ast_dump(const ast_node_t *root, FILE *out);

#endif /* CONFIGLANG_AST_H */
