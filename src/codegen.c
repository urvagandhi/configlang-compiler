/**
 * @file codegen.c
 * @brief Implementation of JSON and YAML configuration file generator.
 * @module codegen
 * @phase Code Generation
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "codegen.h"
#include <inttypes.h>
#include <string.h>

static void format_float(double val, char *buf, size_t max) {
    snprintf(buf, max, "%.15g", val);
    if (strchr(buf, '.') == NULL && strchr(buf, 'e') == NULL && strchr(buf, 'E') == NULL) {
        size_t len = strlen(buf);
        if (len + 3 < max) {
            buf[len] = '.';
            buf[len + 1] = '0';
            buf[len + 2] = '\0';
        }
    }
}

static void print_json_escaped_string(const char *str, FILE *out) {
    fputc('"', out);
    for (const unsigned char *p = (const unsigned char *)str; *p != '\0'; ++p) {
        switch (*p) {
            case '"':  fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\b': fputs("\\b", out); break;
            case '\f': fputs("\\f", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if (*p < 0x20) {
                    fprintf(out, "\\u%04x", *p);
                } else {
                    fputc(*p, out);
                }
                break;
        }
    }
    fputc('"', out);
}

static void print_indent(int level, FILE *out) {
    for (int i = 0; i < level; ++i) {
        fputs("  ", out);
    }
}

static void print_json_value(const sym_value_t *val, FILE *out) {
    if (!val->has_value) {
        fputs("null", out);
        return;
    }
    switch (val->type) {
        case TYPE_INT:
            fprintf(out, "%" PRId64, val->val.ival);
            break;
        case TYPE_FLOAT: {
            char fbuf[64];
            format_float(val->val.fval, fbuf, sizeof(fbuf));
            fputs(fbuf, out);
            break;
        }
        case TYPE_STRING:
            print_json_escaped_string(val->val.sval ? val->val.sval : "", out);
            break;
        case TYPE_BOOL:
            fputs(val->val.bval ? "true" : "false", out);
            break;
        default:
            fputs("null", out);
            break;
    }
}

static void print_json_object(const symbol_t *block_sym, int indent, FILE *out) {
    if (block_sym->children_head == NULL) {
        fputs("{}", out);
        return;
    }
    fputs("{\n", out);
    for (symbol_t *c = block_sym->children_head; c != NULL; c = c->next_child) {
        print_indent(indent + 1, out);
        print_json_escaped_string(c->name, out);
        fputs(": ", out);

        if (c->kind == SYM_BLOCK) {
            print_json_object(c, indent + 1, out);
        } else {
            print_json_value(&c->final_val, out);
        }

        if (c->next_child != NULL) {
            fputc(',', out);
        }
        fputc('\n', out);
    }
    print_indent(indent, out);
    fputs("}", out);
}

static void print_yaml_node(const symbol_t *sym, int indent, FILE *out) {
    if (sym == NULL) {
        return;
    }
    for (symbol_t *c = sym->children_head; c != NULL; c = c->next_child) {
        print_indent(indent, out);
        if (c->kind == SYM_BLOCK) {
            if (c->children_head == NULL) {
                fprintf(out, "%s: {}\n", c->name);
            } else {
                fprintf(out, "%s:\n", c->name);
                print_yaml_node(c, indent + 1, out);
            }
        } else {
            fprintf(out, "%s: ", c->name);
            print_json_value(&c->final_val, out);
            fputc('\n', out);
        }
    }
}

bool codegen_generate(const symtab_t *st, codegen_format_t format, FILE *out) {
    if (st == NULL || out == NULL) {
        return false;
    }
    if (format == CODEGEN_JSON) {
        print_json_object(st->root_block, 0, out);
        fputc('\n', out);
    } else {
        print_yaml_node(st->root_block, 0, out);
    }
    return true;
}

bool codegen_emit_file(const symtab_t *st, codegen_format_t format,
                       const char *output_path) {
    if (output_path == NULL) {
        return codegen_generate(st, format, stdout);
    }
    FILE *fp = fopen(output_path, "wb");
    if (fp == NULL) {
        return false;
    }
    bool ok = codegen_generate(st, format, fp);
    fclose(fp);
    return ok;
}
