/**
 * @file tac.c
 * @brief Three-Address Code generation in dependency topological order.
 * @module tac
 * @phase Intermediate Code Generation
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "tac.h"
#include <inttypes.h>
#include <string.h>

tac_program_t *tac_create_program(arena_t *arena) {
    tac_program_t *p = (tac_program_t *)arena_alloc(arena, sizeof(tac_program_t));
    p->arena = arena;
    p->head = NULL;
    p->tail = NULL;
    p->count = 0;
    p->next_temp_id = 1;
    return p;
}

tac_operand_t tac_new_temp(tac_program_t *prog) {
    tac_operand_t op;
    op.kind = TAC_OPERAND_TEMP;
    char buf[32];
    snprintf(buf, sizeof(buf), "t%zu", prog->next_temp_id++);
    op.name = arena_strdup(prog->arena, buf);
    memset(&op.const_val, 0, sizeof(op.const_val));
    return op;
}

tac_operand_t tac_const_int(int64_t val) {
    tac_operand_t op;
    op.kind = TAC_OPERAND_CONST;
    op.name = NULL;
    op.const_val.has_value = true;
    op.const_val.type = TYPE_INT;
    op.const_val.val.ival = val;
    return op;
}

tac_operand_t tac_const_float(double val) {
    tac_operand_t op;
    op.kind = TAC_OPERAND_CONST;
    op.name = NULL;
    op.const_val.has_value = true;
    op.const_val.type = TYPE_FLOAT;
    op.const_val.val.fval = val;
    return op;
}

tac_operand_t tac_const_string(arena_t *arena, const char *str) {
    tac_operand_t op;
    op.kind = TAC_OPERAND_CONST;
    op.name = NULL;
    op.const_val.has_value = true;
    op.const_val.type = TYPE_STRING;
    op.const_val.val.sval = arena_strdup(arena, str);
    return op;
}

tac_operand_t tac_const_bool(bool val) {
    tac_operand_t op;
    op.kind = TAC_OPERAND_CONST;
    op.name = NULL;
    op.const_val.has_value = true;
    op.const_val.type = TYPE_BOOL;
    op.const_val.val.bval = val;
    return op;
}

tac_operand_t tac_var(arena_t *arena, const char *path) {
    tac_operand_t op;
    op.kind = TAC_OPERAND_VAR;
    op.name = arena_strdup(arena, path);
    memset(&op.const_val, 0, sizeof(op.const_val));
    return op;
}

void tac_emit(tac_program_t *prog, tac_op_t op, tac_operand_t arg1,
              tac_operand_t arg2, tac_operand_t result, diag_loc_t loc) {
    tac_instr_t *ins = (tac_instr_t *)arena_alloc(prog->arena, sizeof(tac_instr_t));
    ins->op = op;
    ins->arg1 = arg1;
    ins->arg2 = arg2;
    ins->result = result;
    ins->loc = loc;
    ins->next = NULL;

    if (prog->tail == NULL) {
        prog->head = ins;
    } else {
        prog->tail->next = ins;
    }
    prog->tail = ins;
    prog->count++;
}

static tac_operand_t tac_emit_expr(tac_program_t *prog, const symtab_t *st,
                                  const ast_node_t *expr);

static tac_operand_t tac_emit_binary(tac_program_t *prog, const symtab_t *st,
                                    const ast_node_t *expr) {
    tac_operand_t left = tac_emit_expr(prog, st, expr->data.binary.left);
    tac_operand_t right = tac_emit_expr(prog, st, expr->data.binary.right);
    tac_operand_t temp = tac_new_temp(prog);

    tac_op_t top = TAC_ADD;
    switch (expr->data.binary.op) {
        case BIN_ADD: top = TAC_ADD; break;
        case BIN_SUB: top = TAC_SUB; break;
        case BIN_MUL: top = TAC_MUL; break;
        case BIN_DIV: top = TAC_DIV; break;
    }
    tac_emit(prog, top, left, right, temp, expr->loc);
    return temp;
}

static tac_operand_t tac_emit_expr(tac_program_t *prog, const symtab_t *st,
                                  const ast_node_t *expr) {
    if (expr == NULL) {
        return (tac_operand_t){0};
    }
    switch (expr->kind) {
        case AST_LIT_INT:
            return tac_const_int(expr->data.lit_int.ival);
        case AST_LIT_FLOAT:
            return tac_const_float(expr->data.lit_float.fval);
        case AST_LIT_STRING:
            return tac_const_string(prog->arena, expr->data.lit_string.sval);
        case AST_LIT_BOOL:
            return tac_const_bool(expr->data.lit_bool.bval);
        case AST_REF: {
            tac_operand_t ref_var = tac_var(prog->arena, expr->data.ref.path);
            tac_operand_t temp = tac_new_temp(prog);
            tac_emit(prog, TAC_LOAD, ref_var, (tac_operand_t){0}, temp, expr->loc);
            return temp;
        }
        case AST_BINARY:
            return tac_emit_binary(prog, st, expr);
        case AST_UNARY: {
            tac_operand_t sub = tac_emit_expr(prog, st, expr->data.unary.operand);
            tac_operand_t temp = tac_new_temp(prog);
            tac_emit(prog, TAC_NEG, sub, (tac_operand_t){0}, temp, expr->loc);
            return temp;
        }
        default:
            return (tac_operand_t){0};
    }
}

static void tac_emit_symbol(tac_program_t *prog, const symtab_t *st, symbol_t *sym);

static void tac_emit_referenced_symbols(tac_program_t *prog, const symtab_t *st,
                                       const ast_node_t *expr) {
    if (expr == NULL) {
        return;
    }
    if (expr->kind == AST_REF) {
        symbol_t *target = symtab_lookup(st, expr->data.ref.path);
        if (target != NULL && target->kind == SYM_VALUE) {
            tac_emit_symbol(prog, st, target);
        }
    } else if (expr->kind == AST_BINARY) {
        tac_emit_referenced_symbols(prog, st, expr->data.binary.left);
        tac_emit_referenced_symbols(prog, st, expr->data.binary.right);
    } else if (expr->kind == AST_UNARY) {
        tac_emit_referenced_symbols(prog, st, expr->data.unary.operand);
    }
}

static void tac_emit_symbol(tac_program_t *prog, const symtab_t *st, symbol_t *sym) {
    if (sym == NULL || sym->kind != SYM_VALUE || sym->state != STATE_UNVISITED) {
        return;
    }
    sym->state = STATE_VISITING;
    /* First ensure all dependencies are emitted in topological order */
    if (sym->decl_node != NULL && sym->decl_node->kind == AST_DECL) {
        tac_emit_referenced_symbols(prog, st, sym->decl_node->data.decl.expr);
    }

    sym->state = STATE_DONE; /* Mark emitted */
    if (sym->decl_node != NULL && sym->decl_node->kind == AST_DECL) {
        tac_operand_t res = tac_emit_expr(prog, st, sym->decl_node->data.decl.expr);
        tac_operand_t target_var = tac_var(prog->arena, sym->path);
        diag_loc_t sloc = (sym->decl_node->data.decl.expr != NULL)
                              ? sym->decl_node->data.decl.expr->loc
                              : sym->decl_node->loc;
        tac_emit(prog, TAC_STORE, res, (tac_operand_t){0}, target_var, sloc);
    }
}

static void tac_traverse_scope(tac_program_t *prog, const symtab_t *st, symbol_t *sym) {
    if (sym == NULL) {
        return;
    }
    if (sym->kind == SYM_VALUE) {
        tac_emit_symbol(prog, st, sym);
    }
    for (symbol_t *c = sym->children_head; c != NULL; c = c->next_child) {
        tac_traverse_scope(prog, st, c);
    }
}

static void reset_visit_states(symbol_t *sym) {
    if (sym == NULL) {
        return;
    }
    sym->state = STATE_UNVISITED;
    for (symbol_t *c = sym->children_head; c != NULL; c = c->next_child) {
        reset_visit_states(c);
    }
}

tac_program_t *tac_generate(arena_t *arena, const symtab_t *st) {
    if (arena == NULL || st == NULL) {
        return NULL;
    }
    tac_program_t *prog = tac_create_program(arena);
    reset_visit_states(st->root_block);
    tac_traverse_scope(prog, st, st->root_block);
    return prog;
}

static void print_operand(const tac_operand_t *op, FILE *out) {
    if (op->kind == TAC_OPERAND_CONST) {
        if (!op->const_val.has_value) {
            fputs("?", out);
            return;
        }
        switch (op->const_val.type) {
            case TYPE_INT:
                fprintf(out, "%" PRId64, op->const_val.val.ival);
                break;
            case TYPE_FLOAT:
                fprintf(out, "%g", op->const_val.val.fval);
                break;
            case TYPE_STRING:
                fprintf(out, "\"%s\"", op->const_val.val.sval);
                break;
            case TYPE_BOOL:
                fprintf(out, "%s", op->const_val.val.bval ? "true" : "false");
                break;
            default:
                fputs("?", out);
                break;
        }
    } else if (op->name != NULL) {
        fputs(op->name, out);
    }
}

void tac_dump(const tac_program_t *prog, FILE *out) {
    if (prog == NULL || out == NULL) {
        return;
    }
    for (const tac_instr_t *i = prog->head; i != NULL; i = i->next) {
        if (i->op == TAC_LOAD) {
            print_operand(&i->result, out);
            fputs(" = LOAD ", out);
            print_operand(&i->arg1, out);
            fputc('\n', out);
        } else if (i->op == TAC_STORE) {
            fputs("STORE ", out);
            print_operand(&i->result, out);
            fputs(", ", out);
            print_operand(&i->arg1, out);
            fputc('\n', out);
        } else if (i->op == TAC_NEG) {
            print_operand(&i->result, out);
            fputs(" = -", out);
            print_operand(&i->arg1, out);
            fputc('\n', out);
        } else {
            const char *op_str = (i->op == TAC_ADD) ? "ADD" :
                                 (i->op == TAC_SUB) ? "SUB" :
                                 (i->op == TAC_MUL) ? "MUL" :
                                 (i->op == TAC_DIV) ? "DIV" : "CONCAT";
            print_operand(&i->result, out);
            fputs(" = ", out);
            print_operand(&i->arg1, out);
            fprintf(out, " %s ", op_str);
            print_operand(&i->arg2, out);
            fputc('\n', out);
        }
    }
}
