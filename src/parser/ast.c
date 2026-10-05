/**
 * @file ast.c
 * @brief Implementation of Abstract Syntax Tree node constructors and dumper.
 * @module ast
 * @phase Syntax Analysis
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "ast.h"
#include <inttypes.h>
#include <string.h>

const char *ast_type_to_string(data_type_t type) {
    switch (type) {
        case TYPE_INT:    return "int";
        case TYPE_FLOAT:  return "float";
        case TYPE_STRING: return "string";
        case TYPE_BOOL:   return "bool";
        case TYPE_ERROR:  return "error";
        default:          return "untyped";
    }
}

static ast_node_t *ast_alloc_node(arena_t *arena, ast_kind_t kind, diag_loc_t loc) {
    ast_node_t *node = (ast_node_t *)arena_alloc(arena, sizeof(ast_node_t));
    node->kind = kind;
    node->loc = loc;
    return node;
}

ast_node_t *ast_create_program(arena_t *arena, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_PROGRAM, loc);
    n->data.program.items_head = NULL;
    n->data.program.items_tail = NULL;
    return n;
}

ast_node_t *ast_create_block(arena_t *arena, const char *name, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_BLOCK, loc);
    n->data.block.name = arena_strdup(arena, name);
    n->data.block.items_head = NULL;
    n->data.block.items_tail = NULL;
    return n;
}

ast_node_t *ast_create_decl(arena_t *arena, const char *name, data_type_t type,
                            ast_range_t range, ast_node_t *expr, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_DECL, loc);
    n->data.decl.name = arena_strdup(arena, name);
    n->data.decl.declared_type = type;
    n->data.decl.range = range;
    n->data.decl.expr = expr;
    return n;
}

ast_node_t *ast_create_int(arena_t *arena, int64_t val, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_LIT_INT, loc);
    n->data.lit_int.ival = val;
    return n;
}

ast_node_t *ast_create_float(arena_t *arena, double val, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_LIT_FLOAT, loc);
    n->data.lit_float.fval = val;
    return n;
}

ast_node_t *ast_create_string(arena_t *arena, const char *str, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_LIT_STRING, loc);
    n->data.lit_string.sval = arena_strdup(arena, str);
    return n;
}

ast_node_t *ast_create_bool(arena_t *arena, bool val, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_LIT_BOOL, loc);
    n->data.lit_bool.bval = val;
    return n;
}

ast_node_t *ast_create_ref(arena_t *arena, const char *path, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_REF, loc);
    n->data.ref.path = arena_strdup(arena, path);
    return n;
}

ast_node_t *ast_create_binary(arena_t *arena, bin_op_t op, ast_node_t *left,
                              ast_node_t *right, diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_BINARY, loc);
    n->data.binary.op = op;
    n->data.binary.left = left;
    n->data.binary.right = right;
    return n;
}

ast_node_t *ast_create_unary(arena_t *arena, un_op_t op, ast_node_t *operand,
                             diag_loc_t loc) {
    ast_node_t *n = ast_alloc_node(arena, AST_UNARY, loc);
    n->data.unary.op = op;
    n->data.unary.operand = operand;
    return n;
}

void ast_append_item(arena_t *arena, ast_node_t *container, ast_node_t *item) {
    if (container == NULL || item == NULL) {
        return;
    }
    ast_item_t *cell = (ast_item_t *)arena_alloc(arena, sizeof(ast_item_t));
    cell->node = item;
    cell->next = NULL;

    ast_item_t **head_ptr = (container->kind == AST_PROGRAM)
                                ? &container->data.program.items_head
                                : &container->data.block.items_head;
    ast_item_t **tail_ptr = (container->kind == AST_PROGRAM)
                                ? &container->data.program.items_tail
                                : &container->data.block.items_tail;

    if (*head_ptr == NULL) {
        *head_ptr = cell;
    } else {
        (*tail_ptr)->next = cell;
    }
    *tail_ptr = cell;
}

static void ast_print_indent(int indent, FILE *out) {
    for (int i = 0; i < indent; ++i) {
        fputs("  ", out);
    }
}

static void ast_dump_node(const ast_node_t *node, int indent, FILE *out);

static void ast_dump_binary(const ast_node_t *node, int indent, FILE *out) {
    const char *op_str = "+";
    switch (node->data.binary.op) {
        case BIN_ADD: op_str = "+"; break;
        case BIN_SUB: op_str = "-"; break;
        case BIN_MUL: op_str = "*"; break;
        case BIN_DIV: op_str = "/"; break;
    }
    ast_print_indent(indent, out);
    fprintf(out, "Binary '%s' [%zu:%zu]\n", op_str, node->loc.line, node->loc.col);
    ast_dump_node(node->data.binary.left, indent + 1, out);
    ast_dump_node(node->data.binary.right, indent + 1, out);
}

static void ast_dump_decl(const ast_node_t *node, int indent, FILE *out) {
    ast_print_indent(indent, out);
    fprintf(out, "Decl %s", node->data.decl.name);
    if (node->data.decl.declared_type != TYPE_NONE) {
        fprintf(out, " : %s", ast_type_to_string(node->data.decl.declared_type));
    }
    if (node->data.decl.range.present) {
        if (node->data.decl.range.lo_is_float || node->data.decl.range.hi_is_float) {
            fprintf(out, "(%g..%g)", node->data.decl.range.lo, node->data.decl.range.hi);
        } else {
            fprintf(out, "(%" PRId64 "..%" PRId64 ")",
                    (int64_t)node->data.decl.range.lo, (int64_t)node->data.decl.range.hi);
        }
    }
    fprintf(out, " [%zu:%zu]\n", node->loc.line, node->loc.col);
    if (node->data.decl.expr != NULL) {
        ast_dump_node(node->data.decl.expr, indent + 1, out);
    }
}

static void ast_dump_node(const ast_node_t *node, int indent, FILE *out) {
    if (node == NULL) {
        return;
    }
    switch (node->kind) {
        case AST_PROGRAM:
            ast_print_indent(indent, out);
            fputs("Program\n", out);
            for (ast_item_t *cur = node->data.program.items_head; cur; cur = cur->next) {
                ast_dump_node(cur->node, indent + 1, out);
            }
            break;
        case AST_BLOCK:
            ast_print_indent(indent, out);
            fprintf(out, "Block %s [%zu:%zu]\n", node->data.block.name,
                    node->loc.line, node->loc.col);
            for (ast_item_t *cur = node->data.block.items_head; cur; cur = cur->next) {
                ast_dump_node(cur->node, indent + 1, out);
            }
            break;
        case AST_DECL:
            ast_dump_decl(node, indent, out);
            break;
        case AST_LIT_INT:
            ast_print_indent(indent, out);
            fprintf(out, "Int %" PRId64 " [%zu:%zu]\n", node->data.lit_int.ival,
                    node->loc.line, node->loc.col);
            break;
        case AST_LIT_FLOAT:
            ast_print_indent(indent, out);
            fprintf(out, "Float %g [%zu:%zu]\n", node->data.lit_float.fval,
                    node->loc.line, node->loc.col);
            break;
        case AST_LIT_STRING:
            ast_print_indent(indent, out);
            fprintf(out, "String \"%s\" [%zu:%zu]\n", node->data.lit_string.sval,
                    node->loc.line, node->loc.col);
            break;
        case AST_LIT_BOOL:
            ast_print_indent(indent, out);
            fprintf(out, "Bool %s [%zu:%zu]\n",
                    node->data.lit_bool.bval ? "true" : "false",
                    node->loc.line, node->loc.col);
            break;
        case AST_REF:
            ast_print_indent(indent, out);
            fprintf(out, "Ref @%s [%zu:%zu]\n", node->data.ref.path,
                    node->loc.line, node->loc.col);
            break;
        case AST_BINARY:
            ast_dump_binary(node, indent, out);
            break;
        case AST_UNARY:
            ast_print_indent(indent, out);
            fprintf(out, "Unary - [%zu:%zu]\n", node->loc.line, node->loc.col);
            ast_dump_node(node->data.unary.operand, indent + 1, out);
            break;
    }
}

void ast_dump(const ast_node_t *root, FILE *out) {
    if (root != NULL && out != NULL) {
        ast_dump_node(root, 0, out);
    }
}
