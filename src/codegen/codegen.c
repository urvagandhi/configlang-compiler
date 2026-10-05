/**
 * ============================================================================
 * Code Generator Implementation (codegen.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Emits clean, human-readable JSON output by traversing the AST and Symbol Table.
 * ============================================================================
 */

#include "codegen.h"
#include "symtab.h"

static void print_indent(int level, FILE *out) {
    for (int i = 0; i < level; i++) {
        fprintf(out, "  ");
    }
}

static void codegen_stmt_list(ASTNode *node, const char *prefix, int indent, FILE *out) {
    ASTNode *cur = node;
    while (cur) {
        char full_name[64];
        if (prefix && prefix[0] != '\0') {
            snprintf(full_name, sizeof(full_name), "%s.%s", prefix, cur->name);
        } else {
            snprintf(full_name, sizeof(full_name), "%s", cur->name);
        }

        print_indent(indent, out);
        fprintf(out, "\"%s\": ", cur->name);

        if (cur->type == NODE_BLOCK) {
            fprintf(out, "{\n");
            codegen_stmt_list(cur->left, full_name, indent + 1, out);
            print_indent(indent, out);
            fprintf(out, "}");
        } else if (cur->type == NODE_DECL) {
            Symbol *sym = symtab_lookup(full_name);
            if (sym && sym->is_assigned) {
                switch (sym->type) {
                    case TYPE_INT:
                        fprintf(out, "%d", sym->ival);
                        break;
                    case TYPE_FLOAT:
                        fprintf(out, "%g", sym->fval);
                        break;
                    case TYPE_STRING:
                        fprintf(out, "\"%s\"", sym->sval);
                        break;
                    case TYPE_BOOL:
                        fprintf(out, "%s", sym->bval ? "true" : "false");
                        break;
                    default:
                        fprintf(out, "null");
                        break;
                }
            } else {
                fprintf(out, "null");
            }
        }

        if (cur->next) {
            fprintf(out, ",");
        }
        fprintf(out, "\n");

        cur = cur->next;
    }
}

void codegen_json(ASTNode *root, FILE *out) {
    if (!out) out = stdout;
    fprintf(out, "{\n");
    codegen_stmt_list(root, "", 1, out);
    fprintf(out, "}\n");
}
