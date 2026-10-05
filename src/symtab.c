/**
 * @file symtab.c
 * @brief Implementation of symbol table hash lookups and scope tree management.
 * @module symtab
 * @phase Semantic Analysis
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "symtab.h"
#include <inttypes.h>
#include <string.h>

static uint32_t symtab_hash(const char *str) {
    uint32_t hash = 5381U;
    int c;
    while ((c = *str++) != 0) {
        hash = ((hash << 5) + hash) + (uint32_t)c;
    }
    return hash;
}

symtab_t *symtab_create(arena_t *arena, const char *filename) {
    symtab_t *st = (symtab_t *)arena_alloc(arena, sizeof(symtab_t));
    st->arena = arena;
    st->filename = filename;
    st->symbol_count = 0;
    memset(st->buckets, 0, sizeof(st->buckets));

    symbol_t *root = (symbol_t *)arena_alloc(arena, sizeof(symbol_t));
    root->name = arena_strdup(arena, "");
    root->path = arena_strdup(arena, "");
    root->kind = SYM_BLOCK;
    root->declared_type = TYPE_NONE;
    root->effective_type = TYPE_NONE;
    root->line = 1;
    root->col = 1;
    st->root_block = root;
    return st;
}

symbol_t *symtab_lookup(const symtab_t *st, const char *path) {
    if (st == NULL || path == NULL) {
        return NULL;
    }
    uint32_t bucket = symtab_hash(path) % SYMTAB_BUCKETS;
    symbol_t *cur = st->buckets[bucket];
    while (cur != NULL) {
        if (strcmp(cur->path, path) == 0) {
            return cur;
        }
        cur = cur->hash_next;
    }
    return NULL;
}

symbol_t *symtab_lookup_local(const symbol_t *parent, const char *name) {
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    symbol_t *cur = parent->children_head;
    while (cur != NULL) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next_child;
    }
    return NULL;
}

static char *build_canonical_path(arena_t *arena, const symbol_t *parent,
                                  const char *name) {
    if (parent == NULL || parent->path[0] == '\0') {
        return arena_strdup(arena, name);
    }
    size_t plen = strlen(parent->path);
    size_t nlen = strlen(name);
    char *buf = (char *)arena_alloc(arena, plen + nlen + 2);
    memcpy(buf, parent->path, plen);
    buf[plen] = '.';
    memcpy(buf + plen + 1, name, nlen + 1);
    return buf;
}

symbol_t *symtab_insert(symtab_t *st, symbol_t *parent, const char *name,
                        symbol_kind_t kind, data_type_t decl_type,
                        ast_range_t range, ast_node_t *node, size_t line,
                        size_t col, diag_collector_t *diags) {
    if (parent == NULL) {
        parent = st->root_block;
    }
    symbol_t *existing = symtab_lookup_local(parent, name);
    if (existing != NULL) {
        diag_loc_t loc = { st->filename, line, col, strlen(name) };
        diag_error(diags, DIAG_E301, loc,
                   "duplicate name '%s' in scope (first defined at line %zu)",
                   name, existing->line);
        return NULL;
    }

    symbol_t *sym = (symbol_t *)arena_alloc(st->arena, sizeof(symbol_t));
    sym->name = arena_strdup(st->arena, name);
    sym->path = build_canonical_path(st->arena, parent, name);
    sym->kind = kind;
    sym->declared_type = decl_type;
    sym->effective_type = decl_type;
    sym->range = range;
    sym->decl_node = node;
    sym->line = line;
    sym->col = col;
    sym->state = STATE_UNVISITED;
    sym->parent = parent;

    /* Append to parent child chain (declaration order) */
    if (parent->children_tail == NULL) {
        parent->children_head = sym;
    } else {
        parent->children_tail->next_child = sym;
    }
    parent->children_tail = sym;

    /* Insert into hash table */
    uint32_t bucket = symtab_hash(sym->path) % SYMTAB_BUCKETS;
    sym->hash_next = st->buckets[bucket];
    st->buckets[bucket] = sym;
    st->symbol_count++;

    return sym;
}

static void symtab_build_node(symtab_t *st, symbol_t *current_scope,
                             const ast_node_t *node, diag_collector_t *diags) {
    if (node == NULL) {
        return;
    }
    if (node->kind == AST_BLOCK) {
        symbol_t *block_sym = symtab_insert(st, current_scope, node->data.block.name,
                                            SYM_BLOCK, TYPE_NONE, (ast_range_t){0},
                                            (ast_node_t *)node, node->loc.line,
                                            node->loc.col, diags);
        if (block_sym != NULL) {
            for (ast_item_t *it = node->data.block.items_head; it; it = it->next) {
                symtab_build_node(st, block_sym, it->node, diags);
            }
        }
    } else if (node->kind == AST_DECL) {
        symtab_insert(st, current_scope, node->data.decl.name, SYM_VALUE,
                      node->data.decl.declared_type, node->data.decl.range,
                      (ast_node_t *)node, node->loc.line, node->loc.col, diags);
    }
}

void symtab_build_pass1(symtab_t *st, const ast_node_t *root, diag_collector_t *diags) {
    if (st == NULL || root == NULL) {
        return;
    }
    for (ast_item_t *cur = root->data.program.items_head; cur; cur = cur->next) {
        symtab_build_node(st, st->root_block, cur->node, diags);
    }
}

static void dump_value_repr(const sym_value_t *val, char *buf, size_t max) {
    if (!val->has_value) {
        snprintf(buf, max, "-");
        return;
    }
    switch (val->type) {
        case TYPE_INT:
            snprintf(buf, max, "%" PRId64, val->val.ival);
            break;
        case TYPE_FLOAT:
            snprintf(buf, max, "%g", val->val.fval);
            break;
        case TYPE_STRING:
            snprintf(buf, max, "\"%s\"", val->val.sval);
            break;
        case TYPE_BOOL:
            snprintf(buf, max, "%s", val->val.bval ? "true" : "false");
            break;
        default:
            snprintf(buf, max, "-");
            break;
    }
}

static void symtab_dump_tree(const symbol_t *sym, FILE *out) {
    if (sym == NULL) {
        return;
    }
    if (sym->path[0] != '\0') {
        const char *kstr = (sym->kind == SYM_BLOCK) ? "block" : "value";
        const char *dstr = (sym->declared_type != TYPE_NONE)
                               ? ast_type_to_string(sym->declared_type)
                               : "-";
        const char *tstr = (sym->effective_type != TYPE_NONE && sym->effective_type != TYPE_ERROR)
                               ? ast_type_to_string(sym->effective_type)
                               : "-";
        char rstr[32] = "-";
        if (sym->range.present) {
            if (sym->range.lo_is_float || sym->range.hi_is_float) {
                snprintf(rstr, sizeof(rstr), "%g..%g", sym->range.lo, sym->range.hi);
            } else {
                snprintf(rstr, sizeof(rstr), "%" PRId64 "..%" PRId64,
                         (int64_t)sym->range.lo, (int64_t)sym->range.hi);
            }
        }
        char val_str[64];
        dump_value_repr(&sym->final_val, val_str, sizeof(val_str));

        fprintf(out, "%-16s %-6s %-7s %-7s %-12s %-5zu %s\n",
                sym->path, kstr, dstr, tstr, rstr, sym->line, val_str);
    }
    for (symbol_t *c = sym->children_head; c != NULL; c = c->next_child) {
        symtab_dump_tree(c, out);
    }
}

void symtab_dump(const symtab_t *st, FILE *out) {
    if (st == NULL || out == NULL) {
        return;
    }
    fprintf(out, "PATH             KIND   DECL    TYPE    RANGE        LINE  VALUE\n");
    symtab_dump_tree(st->root_block, out);
}
