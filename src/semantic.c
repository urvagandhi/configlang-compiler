/**
 * @file semantic.c
 * @brief Implementation of type checking, reference resolution, and cycle detection.
 * @module semantic
 * @phase Semantic Analysis
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "semantic.h"
#include <string.h>

static void add_dep_edge(sem_context_t *ctx, symbol_t *from, symbol_t *to) {
    if (from == NULL || to == NULL) {
        return;
    }
    dep_edge_t *edge = (dep_edge_t *)arena_alloc(ctx->arena, sizeof(dep_edge_t));
    edge->from = from;
    edge->to = to;
    edge->next = NULL;
    if (ctx->deps_tail == NULL) {
        ctx->deps_head = edge;
    } else {
        ctx->deps_tail->next = edge;
    }
    ctx->deps_tail = edge;
}

static void validate_range_decl(sem_context_t *ctx, symbol_t *sym) {
    if (!sym->range.present) {
        return;
    }
    data_type_t dt = sym->declared_type;
    if (dt != TYPE_INT && dt != TYPE_FLOAT) {
        diag_error(ctx->diags, DIAG_E304, sym->range.loc,
                   "range constraint only allowed on numeric types (found on '%s')",
                   sym->path);
        return;
    }
    if (sym->range.lo > sym->range.hi) {
        diag_error(ctx->diags, DIAG_E304, sym->range.loc,
                   "invalid range for '%s': lower bound exceeds upper bound",
                   sym->path);
        return;
    }
    if (dt == TYPE_INT && (sym->range.lo_is_float || sym->range.hi_is_float)) {
        diag_error(ctx->diags, DIAG_E304, sym->range.loc,
                   "type mismatch: integer range bounds cannot be floats on '%s'",
                   sym->path);
    }
}

static data_type_t check_expr_type(sem_context_t *ctx, symbol_t *enclosing_sym,
                                  const ast_node_t *expr);

static data_type_t check_binary_type(sem_context_t *ctx, symbol_t *enclosing,
                                    const ast_node_t *expr) {
    data_type_t lt = check_expr_type(ctx, enclosing, expr->data.binary.left);
    data_type_t rt = check_expr_type(ctx, enclosing, expr->data.binary.right);

    if (lt == TYPE_ERROR || rt == TYPE_ERROR) {
        return TYPE_ERROR;
    }
    bin_op_t op = expr->data.binary.op;
    if (op == BIN_ADD) {
        if (lt == TYPE_INT && rt == TYPE_INT) {
            return TYPE_INT;
        }
        if ((lt == TYPE_INT || lt == TYPE_FLOAT) && (rt == TYPE_INT || rt == TYPE_FLOAT)) {
            return TYPE_FLOAT;
        }
        if (lt == TYPE_STRING && rt == TYPE_STRING) {
            return TYPE_STRING;
        }
    } else {
        if (lt == TYPE_INT && rt == TYPE_INT) {
            return TYPE_INT;
        }
        if ((lt == TYPE_INT || lt == TYPE_FLOAT) && (rt == TYPE_INT || rt == TYPE_FLOAT)) {
            return TYPE_FLOAT;
        }
    }

    const char *op_str = (op == BIN_ADD) ? "+" : (op == BIN_SUB) ? "-" :
                         (op == BIN_MUL) ? "*" : "/";
    diag_error(ctx->diags, DIAG_E307, expr->loc,
               "invalid operand types for operator '%s': %s and %s",
               op_str, ast_type_to_string(lt), ast_type_to_string(rt));
    return TYPE_ERROR;
}

static data_type_t check_ref_type(sem_context_t *ctx, symbol_t *enclosing,
                                 const ast_node_t *expr) {
    const char *ref_path = expr->data.ref.path;
    symbol_t *target = symtab_lookup(ctx->symtab, ref_path);
    if (target == NULL) {
        diag_error(ctx->diags, DIAG_E305, expr->loc,
                   "undefined reference '@%s'", ref_path);
        return TYPE_ERROR;
    }
    if (target->kind == SYM_BLOCK) {
        diag_error(ctx->diags, DIAG_E309, expr->loc,
                   "reference '@%s' names a block, not a value", ref_path);
        return TYPE_ERROR;
    }

    add_dep_edge(ctx, enclosing, target);

    if (target->state == STATE_VISITING) {
        diag_error(ctx->diags, DIAG_E306, expr->loc,
                   "circular reference detected: %s -> %s",
                   enclosing ? enclosing->path : "?", target->path);
        target->effective_type = TYPE_ERROR;
        return TYPE_ERROR;
    }
    if (target->state == STATE_UNVISITED) {
        if (target->decl_node != NULL && target->decl_node->kind == AST_DECL) {
            target->state = STATE_VISITING;
            data_type_t et = check_expr_type(ctx, target,
                                             target->decl_node->data.decl.expr);
            target->effective_type = (target->declared_type != TYPE_NONE)
                                         ? target->declared_type
                                         : et;
            target->state = STATE_DONE;
        }
    }
    return target->effective_type;
}

static data_type_t check_expr_type(sem_context_t *ctx, symbol_t *enclosing_sym,
                                  const ast_node_t *expr) {
    if (expr == NULL) {
        return TYPE_ERROR;
    }
    switch (expr->kind) {
        case AST_LIT_INT:    return TYPE_INT;
        case AST_LIT_FLOAT:  return TYPE_FLOAT;
        case AST_LIT_STRING: return TYPE_STRING;
        case AST_LIT_BOOL:   return TYPE_BOOL;
        case AST_REF:
            return check_ref_type(ctx, enclosing_sym, expr);
        case AST_BINARY:
            return check_binary_type(ctx, enclosing_sym, expr);
        case AST_UNARY: {
            data_type_t t = check_expr_type(ctx, enclosing_sym, expr->data.unary.operand);
            if (t == TYPE_INT || t == TYPE_FLOAT) {
                return t;
            }
            if (t != TYPE_ERROR) {
                diag_error(ctx->diags, DIAG_E307, expr->loc,
                           "invalid operand for unary minus: %s", ast_type_to_string(t));
            }
            return TYPE_ERROR;
        }
        default:
            return TYPE_ERROR;
    }
}

static void check_symbol_semantics(sem_context_t *ctx, symbol_t *sym) {
    if (sym->kind == SYM_BLOCK) {
        if (sym->children_head == NULL && sym->path[0] != '\0') {
            diag_loc_t loc = { ctx->symtab->filename, sym->line, sym->col, strlen(sym->name) };
            diag_warning(ctx->diags, DIAG_W401, loc, "empty block '%s'", sym->path);
        }
    } else if (sym->kind == SYM_VALUE && sym->state == STATE_UNVISITED) {
        validate_range_decl(ctx, sym);
        sym->state = STATE_VISITING;
        data_type_t expr_type = check_expr_type(ctx, sym,
                                                sym->decl_node->data.decl.expr);
        sym->state = STATE_DONE;

        if (expr_type != TYPE_ERROR) {
            if (sym->declared_type == TYPE_NONE) {
                sym->effective_type = expr_type;
            } else if (sym->declared_type == TYPE_FLOAT && expr_type == TYPE_INT) {
                sym->effective_type = TYPE_FLOAT;
                diag_warning(ctx->diags, DIAG_W402, sym->decl_node->data.decl.expr->loc,
                             "implicit int to float conversion for '%s'", sym->path);
            } else if (sym->declared_type != expr_type) {
                diag_error(ctx->diags, DIAG_E302, sym->decl_node->data.decl.expr->loc,
                           "type mismatch for '%s': declared %s, expression has type %s",
                           sym->path, ast_type_to_string(sym->declared_type),
                           ast_type_to_string(expr_type));
                sym->effective_type = TYPE_ERROR;
            } else {
                sym->effective_type = expr_type;
            }
        } else {
            sym->effective_type = TYPE_ERROR;
        }
    }
}

static void traverse_scope_tree(sem_context_t *ctx, symbol_t *sym) {
    if (sym == NULL) {
        return;
    }
    check_symbol_semantics(ctx, sym);
    for (symbol_t *c = sym->children_head; c != NULL; c = c->next_child) {
        traverse_scope_tree(ctx, c);
    }
}

bool semantic_analyze(sem_context_t *ctx, const ast_node_t *root) {
    (void)root;
    if (ctx == NULL || ctx->symtab == NULL) {
        return false;
    }
    size_t err_start = ctx->diags->error_count;
    traverse_scope_tree(ctx, ctx->symtab->root_block);
    return (ctx->diags->error_count == err_start);
}

void semantic_dump_deps(const sem_context_t *ctx, FILE *out) {
    if (ctx == NULL || out == NULL) {
        return;
    }
    for (const dep_edge_t *e = ctx->deps_head; e != NULL; e = e->next) {
        fprintf(out, "%s -> %s\n", e->from ? e->from->path : "?",
                e->to ? e->to->path : "?");
    }
}
