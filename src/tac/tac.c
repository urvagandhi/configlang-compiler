/**
 * ============================================================================
 * Three-Address Code Implementation (tac.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Intermediate Code Generation: decomposes expression trees into linear quadruples.
 * ============================================================================
 */

#include "tac.h"

static TAC *tac_head = NULL;
static TAC *tac_tail = NULL;
static int temp_count = 1;

void tac_init(void) {
    TAC *cur = tac_head;
    while (cur) {
        TAC *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    tac_head = NULL;
    tac_tail = NULL;
    temp_count = 1;
}

char *tac_new_temp(void) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "t%d", temp_count++);
    return strdup(buf);
}

void tac_emit(const char *op, const char *arg1, const char *arg2, const char *result, int line) {
    TAC *t = (TAC *)malloc(sizeof(TAC));
    memset(t, 0, sizeof(TAC));
    strncpy(t->op, op, sizeof(t->op) - 1);
    if (arg1) strncpy(t->arg1, arg1, sizeof(t->arg1) - 1);
    if (arg2) strncpy(t->arg2, arg2, sizeof(t->arg2) - 1);
    if (result) strncpy(t->result, result, sizeof(t->result) - 1);
    t->line = line;
    t->next = NULL;

    if (!tac_head) {
        tac_head = t;
        tac_tail = t;
    } else {
        tac_tail->next = t;
        tac_tail = t;
    }
}

TAC *tac_get_head(void) {
    return tac_head;
}

void tac_set_head(TAC *head) {
    tac_head = head;
}

static char *tac_gen_expr(ASTNode *expr) {
    if (!expr) return "";
    char buf[64];

    switch (expr->type) {
        case NODE_INT:
            snprintf(buf, sizeof(buf), "%d", expr->ival);
            return strdup(buf);

        case NODE_FLOAT:
            snprintf(buf, sizeof(buf), "%g", expr->fval);
            return strdup(buf);

        case NODE_STRING:
            snprintf(buf, sizeof(buf), "\"%s\"", expr->sval);
            return strdup(buf);

        case NODE_BOOL:
            return expr->bval ? "true" : "false";

        case NODE_REF: {
            char *temp = tac_new_temp();
            tac_emit("LOAD", expr->name, "", temp, expr->line);
            return temp;
        }

        case NODE_BINARY: {
            char *left = tac_gen_expr(expr->left);
            char *right = tac_gen_expr(expr->right);
            char *temp = tac_new_temp();

            const char *op_name = "ADD";
            if (expr->op == '-') op_name = "SUB";
            else if (expr->op == '*') op_name = "MUL";
            else if (expr->op == '/') op_name = "DIV";

            tac_emit(op_name, left, right, temp, expr->line);
            return temp;
        }

        default:
            return "";
    }
}

void tac_generate(ASTNode *node, const char *prefix) {
    if (!node) return;

    if (node->type == NODE_BLOCK) {
        char block_prefix[64];
        if (prefix && prefix[0] != '\0') {
            snprintf(block_prefix, sizeof(block_prefix), "%s.%s", prefix, node->name);
        } else {
            snprintf(block_prefix, sizeof(block_prefix), "%s", node->name);
        }
        tac_generate(node->left, block_prefix);
    } else if (node->type == NODE_DECL) {
        char full_name[64];
        if (prefix && prefix[0] != '\0') {
            snprintf(full_name, sizeof(full_name), "%s.%s", prefix, node->name);
        } else {
            snprintf(full_name, sizeof(full_name), "%s", node->name);
        }

        char *val = tac_gen_expr(node->left);
        tac_emit("STORE", val, "", full_name, node->line);
    }

    if (node->next) {
        tac_generate(node->next, prefix);
    }
}

void tac_print(void) {
    TAC *cur = tac_head;
    while (cur) {
        if (strcmp(cur->op, "STORE") == 0) {
            printf("STORE %s = %s\n", cur->result, cur->arg1);
        } else if (strcmp(cur->op, "LOAD") == 0) {
            printf("%s = LOAD @%s\n", cur->result, cur->arg1);
        } else {
            printf("%s = %s %s %s\n", cur->result, cur->arg1, cur->op, cur->arg2);
        }
        cur = cur->next;
    }
}
