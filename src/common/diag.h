/**
 * ============================================================================
 * Compiler Diagnostic Engine (diag.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * WHAT IS THIS & WHY DO WE NEED IT?
 *   When a user writes an incorrect configuration (e.g., misspelled word,
 *   type mismatch, or division by zero), the compiler must report:
 *     1. The filename, line number, and column number
 *     2. A clear error code (E101 for lexer, E201 for parser, etc.)
 *     3. A visual caret underline (^^^^) pointing directly to the error
 *   This file defines the diagnostic collector that records all errors.
 * ============================================================================
 */

#ifndef CONFIGLANG_DIAG_H
#define CONFIGLANG_DIAG_H

#include <stddef.h>
#include <stdbool.h>

/** Maximum number of errors reported before stopping compilation. */
#define DIAG_MAX_REPORTS 50U

/** Diagnostic severity: Error (stops code generation) or Warning. */
typedef enum diag_severity {
    DIAG_SEV_ERROR = 0,
    DIAG_SEV_WARNING = 1
} diag_severity_t;

/** Standard diagnostic codes matching SRS §6. */
typedef enum diag_code {
    DIAG_E101 = 101, /**< Illegal character */
    DIAG_E102 = 102, /**< Unterminated string literal */
    DIAG_E103 = 103, /**< Integer literal out of range */
    DIAG_E104 = 104, /**< Invalid escape sequence */
    DIAG_E105 = 105, /**< Unterminated block comment */
    DIAG_E201 = 201, /**< Syntax error */
    DIAG_E301 = 301, /**< Duplicate name in scope */
    DIAG_E302 = 302, /**< Type mismatch */
    DIAG_E303 = 303, /**< Value outside range */
    DIAG_E304 = 304, /**< Invalid range */
    DIAG_E305 = 305, /**< Undefined reference */
    DIAG_E306 = 306, /**< Circular reference */
    DIAG_E307 = 307, /**< Invalid operand types */
    DIAG_E308 = 308, /**< Division by zero */
    DIAG_E309 = 309, /**< Reference to block */
    DIAG_E310 = 310, /**< Arithmetic overflow / non-finite float */
    DIAG_E901 = 901, /**< Driver / I/O error */
    DIAG_W401 = 401, /**< Empty block warning */
    DIAG_W402 = 402  /**< Implicit widening warning */
} diag_code_t;

/** Source span location information. */
typedef struct diag_loc {
    const char *filename; /**< Name of source file. */
    size_t line;          /**< 1-based line number. */
    size_t col;           /**< 1-based column number. */
    size_t length;        /**< Length in characters for caret span. */
} diag_loc_t;

/** Single diagnostic record. */
typedef struct diag_item {
    diag_severity_t severity;
    diag_code_t code;
    diag_loc_t loc;
    char *message;
} diag_item_t;

/** Diagnostic collection container. */
typedef struct diag_collector {
    diag_item_t *items;
    size_t count;
    size_t capacity;
    size_t error_count;
    size_t warning_count;
    bool color_enabled;
    const char *source_text; /**< Pointer to entire source buffer for lines. */
} diag_collector_t;

/**
 * @brief Initialize a diagnostic collector.
 * @param source_text Null-terminated source file contents for caret rendering.
 * @param color_enabled True to use ANSI color codes on terminal.
 * @return Initialized diagnostic collector instance.
 */
diag_collector_t diag_create(const char *source_text, bool color_enabled);

/**
 * @brief Add an error diagnostic to the collector.
 * @param dc Pointer to diagnostic collector.
 * @param code Diagnostic error code (e.g. DIAG_E302).
 * @param loc Source location of error.
 * @param fmt Printf-style format string for error message.
 * @param ... Format arguments.
 */
void diag_error(diag_collector_t *dc, diag_code_t code, diag_loc_t loc,
                const char *fmt, ...);

/**
 * @brief Add a warning diagnostic to the collector.
 * @param dc Pointer to diagnostic collector.
 * @param code Diagnostic warning code (e.g. DIAG_W401).
 * @param loc Source location of warning.
 * @param fmt Printf-style format string for warning message.
 * @param ... Format arguments.
 */
void diag_warning(diag_collector_t *dc, diag_code_t code, diag_loc_t loc,
                  const char *fmt, ...);

/**
 * @brief Sort all collected diagnostics by (line, column).
 * @param dc Pointer to diagnostic collector.
 */
void diag_sort(diag_collector_t *dc);

/**
 * @brief Render and print all diagnostics to stderr in SRS §5.4 format.
 * @param dc Pointer to diagnostic collector.
 */
void diag_render_all(diag_collector_t *dc);

/**
 * @brief Free resources associated with diagnostic collector.
 * @param dc Pointer to diagnostic collector.
 */
void diag_destroy(diag_collector_t *dc);

#endif /* CONFIGLANG_DIAG_H */
