/**
 * ============================================================================
 * Target Code Generator Interface (codegen.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * SYLLABUS MAPPING:
 *   - Experiment 8: Design and implementation of Code Generator for target
 *     machine / target format (JSON and YAML).
 *
 * WHAT THIS FILE DOES:
 *   Takes the evaluated symbols and emits valid JSON or YAML configurations.
 * ============================================================================
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
