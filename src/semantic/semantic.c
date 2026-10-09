/**
 * ============================================================================
 * Semantic Analyzer Implementation (semantic.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * HOW IT WORKS (In 2 Simple Steps):
 *   Step 1: Walk the AST and register all variables in the Symbol Table.
 *   Step 2: Check each expression:
 *           - Are @referenced variables defined? (Undefined check)
 *           - Does a variable reference itself? (Circular check)
 *           - Are operand types compatible with operators? (Type check)
 * ============================================================================
 */

#include "semantic.h"

static int sem_errors = 0;

static void build_canonical_name(char *dest, size_t max, const char *prefix, const char *name) {
    if (prefix && prefix[0] != '\0') {
        snprintf(dest, max, "%s.%s", prefix, name);
    } else {
        snprintf(dest, max, "%s", name);
    }
}

/* Step 1: Register all variable declarations in Symbol Table */
static void pass1_register_symbols(ASTNode *node, const char *prefix) {
    if (!node) return;

    if (node->type == NODE_BLOCK) {
        char block_prefix[64];
        build_canonical_name(block_prefix, sizeof(block_prefix), prefix, node->name);
        pass1_register_symbols(node->left, block_prefix);
    } else if (node->type == NODE_DECL) {
        char full_name[64];
        build_canonical_name(full_name, sizeof(full_name), prefix, node->name);
        if (symtab_lookup(full_name)) {
            fprintf(stderr, "Semantic Error at line %d: Duplicate declaration of '%s'\n",
                    node->line, full_name);
            sem_errors++;
        } else {
            symtab_insert(full_name, node->data_type);
        }
    }

    if (node->next) {
        pass1_register_symbols(node->next, prefix);
    }
}

/* Helper to evaluate expression type */
static DataType check_expr_type(ASTNode *expr, const char *current_var) {
    if (!expr) return TYPE_UNKNOWN;

    switch (expr->type) {
        case NODE_INT:    return TYPE_INT;
        case NODE_FLOAT:  return TYPE_FLOAT;
        case NODE_STRING: return TYPE_STRING;
        case NODE_BOOL:   return TYPE_BOOL;

        case NODE_REF: {
            /* Check if variable references itself */
            if (current_var && strcmp(current_var, expr->name) == 0) {
                fprintf(stderr, "Semantic Error at line %d: Circular reference detected for '%s'\n",
                        expr->line, current_var);
                sem_errors++;
                return TYPE_UNKNOWN;
            }

            /* Check if referenced variable is declared in symbol table */
            Symbol *sym = symtab_lookup(expr->name);
            if (!sym) {
                fprintf(stderr, "Semantic Error at line %d: Undefined reference '@%s'\n",
                        expr->line, expr->name);
                sem_errors++;
                return TYPE_UNKNOWN;
            }
            return sym->type;
        }

        case NODE_BINARY: {
            DataType lt = check_expr_type(expr->left, current_var);
            DataType rt = check_expr_type(expr->right, current_var);

            /* String concatenation with '+' */
            if (expr->op == '+' && (lt == TYPE_STRING || rt == TYPE_STRING)) {
                return TYPE_STRING;
            }

            /* Numeric operations */
            if ((lt == TYPE_INT || lt == TYPE_FLOAT) && (rt == TYPE_INT || rt == TYPE_FLOAT)) {
                if (expr->op == '/') {
                    if ((expr->right->type == NODE_INT && expr->right->ival == 0) ||
                        (expr->right->type == NODE_FLOAT && expr->right->fval == 0.0)) {
                        fprintf(stderr, "Semantic Error at line %d: Division by zero\n", expr->line);
                        sem_errors++;
                        return TYPE_UNKNOWN;
                    }
                }
                if (lt == TYPE_FLOAT || rt == TYPE_FLOAT) return TYPE_FLOAT;
                return TYPE_INT;
            }

            fprintf(stderr, "Semantic Error at line %d: Invalid operand types for operator '%c'\n",
                    expr->line, expr->op);
            sem_errors++;
            return TYPE_UNKNOWN;
        }

        default:
            return TYPE_UNKNOWN;
    }
}

/* Step 2: Validate all expressions */
static void pass2_check_expressions(ASTNode *node, const char *prefix) {
    if (!node) return;

    if (node->type == NODE_BLOCK) {
        char block_prefix[64];
        build_canonical_name(block_prefix, sizeof(block_prefix), prefix, node->name);
        pass2_check_expressions(node->left, block_prefix);
    } else if (node->type == NODE_DECL) {
        char full_name[64];
        build_canonical_name(full_name, sizeof(full_name), prefix, node->name);

        DataType expr_type = check_expr_type(node->left, full_name);
        Symbol *sym = symtab_lookup(full_name);
        if (sym && sym->type == TYPE_UNKNOWN) {
            sym->type = expr_type;
        } else if (sym && sym->type != TYPE_UNKNOWN && expr_type != TYPE_UNKNOWN) {
            if (sym->type != expr_type && !(sym->type == TYPE_FLOAT && expr_type == TYPE_INT)) {
                fprintf(stderr, "Semantic Error at line %d: Type mismatch in declaration of '%s'\n",
                        node->line, full_name);
                sem_errors++;
            }
        }
    }

    if (node->next) {
        pass2_check_expressions(node->next, prefix);
    }
}

int semantic_check(ASTNode *root) {
    sem_errors = 0;
    pass1_register_symbols(root, "");
    pass2_check_expressions(root, "");
    return sem_errors;
}
