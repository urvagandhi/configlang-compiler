/**
 * ============================================================================
 * Code Optimizer Implementation (optimizer.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * HOW IT WORKS (In 2 Simple Steps):
 *   Step 1 (Constant Propagation): If an operand is a temporary or variable
 *          whose value is already known, substitute that constant into place.
 *   Step 2 (Constant Folding): If both operands of an arithmetic instruction
 *          are constants, compute the result at compile time (e.g. 30 * 1000 -> 30000)
 *          and eliminate the instruction!
 * ============================================================================
 */

#include "optimizer.h"
#include "symtab.h"
#include <ctype.h>

/** Simple constant mapping table (var_name -> const_val) */
typedef struct ConstBinding {
    char name[64];
    char value[256];
    struct ConstBinding *next;
} ConstBinding;

static ConstBinding *bindings = NULL;
static int fold_count = 0;

static void set_binding(const char *name, const char *val) {
    ConstBinding *b = bindings;
    while (b) {
        if (strcmp(b->name, name) == 0) {
            strncpy(b->value, val, sizeof(b->value) - 1);
            return;
        }
        b = b->next;
    }
    b = (ConstBinding *)malloc(sizeof(ConstBinding));
    strncpy(b->name, name, sizeof(b->name) - 1);
    strncpy(b->value, val, sizeof(b->value) - 1);
    b->next = bindings;
    bindings = b;
}

static const char *get_binding(const char *name) {
    ConstBinding *b = bindings;
    while (b) {
        if (strcmp(b->name, name) == 0) {
            return b->value;
        }
        b = b->next;
    }
    return NULL;
}

/* Helper: Check if string is a numeric integer */
static bool is_integer(const char *str) {
    if (!str || !*str) return false;
    if (*str == '-' || *str == '+') str++;
    if (!*str) return false;
    while (*str) {
        if (!isdigit(*str)) return false;
        str++;
    }
    return true;
}

/* Helper: Check if string is a float */
static bool is_float(const char *str) {
    if (!str || !*str) return false;
    char *endptr = NULL;
    strtod(str, &endptr);
    return (endptr != str && *endptr == '\0' && strchr(str, '.') != NULL);
}

/* Helper: Check if string is a quoted string literal */
static bool is_string_literal(const char *str) {
    if (!str) return false;
    size_t len = strlen(str);
    return (len >= 2 && str[0] == '"' && str[len - 1] == '"');
}

int optimize_tac(void) {
    fold_count = 0;
    bindings = NULL;

    bool progress = true;
    int pass = 0;

    while (progress && pass < 20) {
        progress = false;
        pass++;

        TAC *prev = NULL;
        TAC *cur = tac_get_head();

        while (cur) {
            /* Handle LOAD instruction: propagate variable's known value into temporary */
            if (strcmp(cur->op, "LOAD") == 0) {
                const char *val = get_binding(cur->arg1);
                if (val && !get_binding(cur->result)) {
                    set_binding(cur->result, val);
                    progress = true;
                }
                prev = cur;
                cur = cur->next;
                continue;
            }

            /* Step 1: Constant Propagation */
            const char *c1 = get_binding(cur->arg1);
            if (c1 && strcmp(cur->arg1, c1) != 0) {
                strncpy(cur->arg1, c1, sizeof(cur->arg1) - 1);
                progress = true;
            }

            const char *c2 = get_binding(cur->arg2);
            if (c2 && strcmp(cur->arg2, c2) != 0) {
                strncpy(cur->arg2, c2, sizeof(cur->arg2) - 1);
                progress = true;
            }

            /* Detect compile-time division by zero */
            if (strcmp(cur->op, "DIV") == 0) {
                bool is_zero = false;
                if (is_integer(cur->arg2) && atoi(cur->arg2) == 0) {
                    is_zero = true;
                } else if (is_float(cur->arg2) && atof(cur->arg2) == 0.0) {
                    is_zero = true;
                }
                if (is_zero) {
                    if (cur->line > 0) {
                        fprintf(stderr, "Semantic Error at line %d: Division by zero in expression '%s / %s'\n",
                                cur->line, cur->arg1, cur->arg2);
                    } else {
                        fprintf(stderr, "Semantic Error: Division by zero in expression '%s / %s'\n",
                                cur->arg1, cur->arg2);
                    }
                    return -1;
                }
            }

            /* Step 2: Constant Folding on Binary Operations */
            bool folded = false;
            char result_buf[512] = "";

            if ((is_integer(cur->arg1) || is_float(cur->arg1)) &&
                (is_integer(cur->arg2) || is_float(cur->arg2))) {
                bool has_float = is_float(cur->arg1) || is_float(cur->arg2);
                if (has_float) {
                    double a = atof(cur->arg1);
                    double b = atof(cur->arg2);
                    double res = 0.0;
                    if (strcmp(cur->op, "ADD") == 0) res = a + b;
                    else if (strcmp(cur->op, "SUB") == 0) res = a - b;
                    else if (strcmp(cur->op, "MUL") == 0) res = a * b;
                    else if (strcmp(cur->op, "DIV") == 0 && b != 0.0) res = a / b;

                    snprintf(result_buf, sizeof(result_buf), "%g", res);
                    folded = true;
                } else {
                    int a = atoi(cur->arg1);
                    int b = atoi(cur->arg2);
                    int res = 0;
                    if (strcmp(cur->op, "ADD") == 0) res = a + b;
                    else if (strcmp(cur->op, "SUB") == 0) res = a - b;
                    else if (strcmp(cur->op, "MUL") == 0) res = a * b;
                    else if (strcmp(cur->op, "DIV") == 0 && b != 0) res = a / b;

                    snprintf(result_buf, sizeof(result_buf), "%d", res);
                    folded = true;
                }
            } else if (is_string_literal(cur->arg1) && is_string_literal(cur->arg2) && strcmp(cur->op, "ADD") == 0) {
                /* String concatenation */
                size_t l1 = strlen(cur->arg1);
                size_t l2 = strlen(cur->arg2);
                char s1[128] = "", s2[128] = "";
                if (l1 >= 2) {
                    strncpy(s1, cur->arg1 + 1, l1 - 2);
                    s1[l1 - 2] = '\0';
                }
                if (l2 >= 2) {
                    strncpy(s2, cur->arg2 + 1, l2 - 2);
                    s2[l2 - 2] = '\0';
                }
                snprintf(result_buf, sizeof(result_buf), "\"%s%s\"", s1, s2);
                folded = true;
            }

            if (folded) {
                set_binding(cur->result, result_buf);
                fold_count++;
                progress = true;
                /* Remove this folded temporary instruction */
                if (prev) {
                    prev->next = cur->next;
                } else {
                    tac_set_head(cur->next);
                }
                cur = cur->next;
                continue;
            }

            /* Handle STORE into variable */
            if (strcmp(cur->op, "STORE") == 0) {
                const char *final_val = cur->arg1;
                const char *b = get_binding(cur->arg1);
                if (b) {
                    final_val = b;
                    strncpy(cur->arg1, b, sizeof(cur->arg1) - 1);
                    progress = true;
                }

                /* Record variable value in symbol table */
                Symbol *sym = symtab_lookup(cur->result);
                if (sym) {
                    sym->is_assigned = true;
                    if (is_string_literal(final_val) || sym->type == TYPE_STRING) {
                        sym->type = TYPE_STRING;
                        size_t len = strlen(final_val);
                        if (len >= 2 && final_val[0] == '"' && final_val[len - 1] == '"') {
                            strncpy(sym->sval, final_val + 1, len - 2);
                            sym->sval[len - 2] = '\0';
                        } else {
                            strncpy(sym->sval, final_val, sizeof(sym->sval) - 1);
                            sym->sval[sizeof(sym->sval) - 1] = '\0';
                        }
                    } else if (strcmp(final_val, "true") == 0 || strcmp(final_val, "false") == 0 || sym->type == TYPE_BOOL) {
                        sym->type = TYPE_BOOL;
                        sym->bval = (strcmp(final_val, "true") == 0);
                    } else if (is_float(final_val) || sym->type == TYPE_FLOAT) {
                        sym->type = TYPE_FLOAT;
                        sym->fval = atof(final_val);
                    } else if (is_integer(final_val) || sym->type == TYPE_INT) {
                        sym->type = TYPE_INT;
                        sym->ival = atoi(final_val);
                    }
                }
                set_binding(cur->result, final_val);
            }

            prev = cur;
            cur = cur->next;
        }
    }

    return fold_count;
}

void optimizer_print(void) {
    tac_print();
}
