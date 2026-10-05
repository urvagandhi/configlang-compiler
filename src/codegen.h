/**
 * @file codegen.h
 * @brief Code generator interface for standard JSON and YAML configuration output.
 * @module codegen
 * @phase Code Generation
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#ifndef CONFIGLANG_CODEGEN_H
#define CONFIGLANG_CODEGEN_H

#include "symtab.h"
#include <stdbool.h>
#include <stdio.h>

/** Target format selector. */
typedef enum codegen_format {
    CODEGEN_JSON,
    CODEGEN_YAML
} codegen_format_t;

/**
 * @brief Generate JSON or YAML formatted output from evaluated symbol tree.
 * @param st Symbol table containing evaluated values and ordered scope tree.
 * @param format Output format (JSON or YAML).
 * @param out Destination stream.
 * @return True on successful generation, false on error.
 */
bool codegen_generate(const symtab_t *st, codegen_format_t format, FILE *out);

/**
 * @brief Emit code to target file path (or stdout if path is NULL).
 * @param st Symbol table.
 * @param format Output format.
 * @param output_path File path, or NULL for stdout.
 * @return True on success, false on I/O error.
 */
bool codegen_emit_file(const symtab_t *st, codegen_format_t format,
                       const char *output_path);

#endif /* CONFIGLANG_CODEGEN_H */
