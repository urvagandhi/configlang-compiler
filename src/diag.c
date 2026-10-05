/**
 * @file diag.c
 * @brief Implementation of diagnostic collector and caret formatter.
 * @module diag
 * @phase Common
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "diag.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANSI_RED "\033[1;31m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_BOLD "\033[1m"
#define ANSI_RESET "\033[0m"

#define DIAG_INIT_CAPACITY 16U
#define DIAG_MAX_MSG_LEN 512U

diag_collector_t diag_create(const char *source_text, bool color_enabled) {
    diag_collector_t dc;
    dc.items = (diag_item_t *)util_xcalloc(DIAG_INIT_CAPACITY, sizeof(diag_item_t));
    dc.count = 0;
    dc.capacity = DIAG_INIT_CAPACITY;
    dc.error_count = 0;
    dc.warning_count = 0;
    dc.color_enabled = color_enabled;
    dc.source_text = source_text;
    return dc;
}

static void diag_add_item(diag_collector_t *dc, diag_severity_t sev,
                          diag_code_t code, diag_loc_t loc, const char *msg) {
    if (dc->count >= DIAG_MAX_REPORTS) {
        return;
    }
    if (dc->count >= dc->capacity) {
        dc->capacity *= 2U;
        dc->items = (diag_item_t *)util_xrealloc(dc->items,
                                                 dc->capacity * sizeof(diag_item_t));
    }
    diag_item_t *item = &dc->items[dc->count++];
    item->severity = sev;
    item->code = code;
    item->loc = loc;
    item->message = util_xstrdup(msg);
    if (sev == DIAG_SEV_ERROR) {
        dc->error_count++;
    } else {
        dc->warning_count++;
    }
}

void diag_error(diag_collector_t *dc, diag_code_t code, diag_loc_t loc,
                const char *fmt, ...) {
    if (dc == NULL || fmt == NULL) {
        return;
    }
    char buf[DIAG_MAX_MSG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    diag_add_item(dc, DIAG_SEV_ERROR, code, loc, buf);
}

void diag_warning(diag_collector_t *dc, diag_code_t code, diag_loc_t loc,
                  const char *fmt, ...) {
    if (dc == NULL || fmt == NULL) {
        return;
    }
    char buf[DIAG_MAX_MSG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    diag_add_item(dc, DIAG_SEV_WARNING, code, loc, buf);
}

static int diag_compare(const void *a, const void *b) {
    const diag_item_t *ia = (const diag_item_t *)a;
    const diag_item_t *ib = (const diag_item_t *)b;
    if (ia->loc.line != ib->loc.line) {
        return (ia->loc.line < ib->loc.line) ? -1 : 1;
    }
    if (ia->loc.col != ib->loc.col) {
        return (ia->loc.col < ib->loc.col) ? -1 : 1;
    }
    return 0;
}

void diag_sort(diag_collector_t *dc) {
    if (dc != NULL && dc->count > 1) {
        qsort(dc->items, dc->count, sizeof(diag_item_t), diag_compare);
    }
}

/**
 * @brief Extract the 1-based source line string from full source text.
 */
static void extract_source_line(const char *src, size_t target_line,
                                char *out_buf, size_t out_max) {
    out_buf[0] = '\0';
    if (src == NULL || target_line == 0) {
        return;
    }
    size_t cur_line = 1;
    const char *p = src;
    while (*p != '\0' && cur_line < target_line) {
        if (*p == '\n') {
            cur_line++;
        }
        p++;
    }
    if (cur_line != target_line || *p == '\0') {
        return;
    }
    const char *start = p;
    while (*p != '\0' && *p != '\n' && *p != '\r') {
        p++;
    }
    size_t line_len = (size_t)(p - start);
    if (line_len >= out_max) {
        line_len = out_max - 1U;
    }
    memcpy(out_buf, start, line_len);
    out_buf[line_len] = '\0';
}

static void print_caret_lines(const diag_collector_t *dc, const diag_item_t *it) {
    if (dc->source_text == NULL || it->loc.line == 0) {
        return;
    }
    char line_buf[1024];
    extract_source_line(dc->source_text, it->loc.line, line_buf, sizeof(line_buf));
    if (line_buf[0] == '\0') {
        return;
    }
    fprintf(stderr, "%5zu | %s\n", it->loc.line, line_buf);
    fprintf(stderr, "      | ");

    size_t col = (it->loc.col > 0) ? it->loc.col : 1U;
    for (size_t i = 1; i < col; ++i) {
        fputc((i <= strlen(line_buf) && line_buf[i - 1] == '\t') ? '\t' : ' ', stderr);
    }
    size_t span = (it->loc.length > 0) ? it->loc.length : 1U;
    if (dc->color_enabled) {
        fprintf(stderr, "%s", (it->severity == DIAG_SEV_ERROR) ? ANSI_RED : ANSI_YELLOW);
    }
    for (size_t i = 0; i < span; ++i) {
        fputc('^', stderr);
    }
    if (dc->color_enabled) {
        fprintf(stderr, "%s", ANSI_RESET);
    }
    fputc('\n', stderr);
}

void diag_render_all(diag_collector_t *dc) {
    if (dc == NULL) {
        return;
    }
    diag_sort(dc);
    for (size_t i = 0; i < dc->count; ++i) {
        diag_item_t *it = &dc->items[i];
        const char *sev_str = (it->severity == DIAG_SEV_ERROR) ? "error" : "warning";
        char code_pfx = (it->severity == DIAG_SEV_ERROR) ? 'E' : 'W';
        const char *fname = (it->loc.filename != NULL) ? it->loc.filename : "<input>";

        fprintf(stderr, "%s:%zu:%zu: %s[%c%03d]: %s\n",
                fname, it->loc.line, it->loc.col, sev_str, code_pfx,
                (int)it->code, it->message);

        print_caret_lines(dc, it);
    }

    if (dc->count >= DIAG_MAX_REPORTS) {
        fprintf(stderr, "fatal: too many errors (> %u), stopping.\n", DIAG_MAX_REPORTS);
    }

    const char *err_word = (dc->error_count == 1) ? "error" : "errors";
    const char *warn_word = (dc->warning_count == 1) ? "warning" : "warnings";
    fprintf(stderr, "%zu %s, %zu %s generated.\n",
            dc->error_count, err_word, dc->warning_count, warn_word);
}

void diag_destroy(diag_collector_t *dc) {
    if (dc == NULL) {
        return;
    }
    for (size_t i = 0; i < dc->count; ++i) {
        if (dc->items[i].message != NULL) {
            free(dc->items[i].message);
        }
    }
    free(dc->items);
    dc->items = NULL;
    dc->count = 0;
    dc->capacity = 0;
}
