/**
 * ============================================================================
 * Symbol Table Implementation (symtab.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Simple linked-list Symbol Table with O(n) search and insertion.
 * ============================================================================
 */

#include "symtab.h"

static Symbol *sym_head = NULL;
static Symbol *sym_tail = NULL;

void symtab_init(void) {
    Symbol *cur = sym_head;
    while (cur) {
        Symbol *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    sym_head = NULL;
    sym_tail = NULL;
}

Symbol *symtab_lookup(const char *name) {
    if (!name) return NULL;
    Symbol *cur = sym_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

Symbol *symtab_insert(const char *name, DataType type) {
    Symbol *existing = symtab_lookup(name);
    if (existing) {
        return existing;
    }

    Symbol *s = (Symbol *)malloc(sizeof(Symbol));
    memset(s, 0, sizeof(Symbol));
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->type = type;
    s->is_assigned = false;
    s->next = NULL;

    if (!sym_head) {
        sym_head = s;
        sym_tail = s;
    } else {
        sym_tail->next = s;
        sym_tail = s;
    }
    return s;
}

Symbol *symtab_get_all(void) {
    return sym_head;
}

void symtab_print(void) {
    printf("----------------------------------------------------------\n");
    printf("%-24s %-10s %-10s %s\n", "NAME", "TYPE", "STATUS", "VALUE");
    printf("----------------------------------------------------------\n");

    Symbol *cur = sym_head;
    while (cur) {
        char val_str[256] = "-";
        if (cur->is_assigned) {
            switch (cur->type) {
                case TYPE_INT:
                    snprintf(val_str, sizeof(val_str), "%d", cur->ival);
                    break;
                case TYPE_FLOAT:
                    snprintf(val_str, sizeof(val_str), "%g", cur->fval);
                    break;
                case TYPE_STRING:
                    snprintf(val_str, sizeof(val_str), "\"%s\"", cur->sval);
                    break;
                case TYPE_BOOL:
                    snprintf(val_str, sizeof(val_str), "%s", cur->bval ? "true" : "false");
                    break;
                default:
                    break;
            }
        }
        printf("%-24s %-10s %-10s %s\n",
               cur->name,
               type_name(cur->type),
               cur->is_assigned ? "assigned" : "declared",
               val_str);
        cur = cur->next;
    }
    printf("----------------------------------------------------------\n");
}
