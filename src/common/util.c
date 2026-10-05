/**
 * ============================================================================
 * Helper Utilities Implementation (util.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * WHAT THIS FILE IMPLEMENTS:
 *   1. Memory wrappers (util_xmalloc, util_xcalloc, util_xrealloc):
 *      Standard malloc/calloc/realloc with automatic out-of-memory checks.
 *   2. Dynamic String Buffer (strbuf_t):
 *      Grows dynamically like a C++ std::string for accumulating string characters.
 *   3. File Reader (util_read_file):
 *      Loads the entire source file into an in-memory null-terminated buffer.
 * ============================================================================
 */

#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Maximum supported source file size: 16 MB. */
#define UTIL_MAX_FILE_SIZE (16U * 1024U * 1024U)

void *util_xmalloc(size_t size) {
    if (size == 0) {
        size = 1;
    }
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "fatal: out of memory allocating %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *util_xcalloc(size_t count, size_t size) {
    if (count == 0 || size == 0) {
        count = 1;
        size = 1;
    }
    void *ptr = calloc(count, size);
    if (ptr == NULL) {
        fprintf(stderr, "fatal: out of memory zero-allocating %zu items of %zu bytes\n",
                count, size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *util_xrealloc(void *ptr, size_t size) {
    if (size == 0) {
        size = 1;
    }
    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        fprintf(stderr, "fatal: out of memory reallocating %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

char *util_xstrdup(const char *str) {
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    char *copy = (char *)util_xmalloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

strbuf_t util_strbuf_create(void) {
    strbuf_t sb;
    sb.capacity = UTIL_STRBUF_INIT_CAP;
    sb.length = 0;
    sb.data = (char *)util_xmalloc(sb.capacity);
    sb.data[0] = '\0';
    return sb;
}

void util_strbuf_append(strbuf_t *sb, const char *str) {
    if (sb == NULL || str == NULL) {
        return;
    }
    size_t add_len = strlen(str);
    size_t needed = sb->length + add_len + 1;
    if (needed > sb->capacity) {
        while (sb->capacity < needed) {
            sb->capacity *= 2U;
        }
        sb->data = (char *)util_xrealloc(sb->data, sb->capacity);
    }
    memcpy(sb->data + sb->length, str, add_len);
    sb->length += add_len;
    sb->data[sb->length] = '\0';
}

void util_strbuf_append_char(strbuf_t *sb, char ch) {
    if (sb == NULL) {
        return;
    }
    if (sb->length + 2U > sb->capacity) {
        sb->capacity *= 2U;
        sb->data = (char *)util_xrealloc(sb->data, sb->capacity);
    }
    sb->data[sb->length] = ch;
    sb->length += 1U;
    sb->data[sb->length] = '\0';
}

void util_strbuf_destroy(strbuf_t *sb) {
    if (sb == NULL) {
        return;
    }
    if (sb->data != NULL) {
        free(sb->data);
        sb->data = NULL;
    }
    sb->length = 0;
    sb->capacity = 0;
}

char *util_read_file(const char *path, size_t *out_size) {
    if (path == NULL) {
        return NULL;
    }
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    long size_signed = ftell(fp);
    if (size_signed < 0 || (size_t)size_signed > UTIL_MAX_FILE_SIZE) {
        fclose(fp);
        return NULL;
    }
    size_t size = (size_t)size_signed;
    rewind(fp);

    char *buffer = (char *)util_xmalloc(size + 1U);
    size_t read_bytes = fread(buffer, 1U, size, fp);
    fclose(fp);

    buffer[read_bytes] = '\0';
    if (out_size != NULL) {
        *out_size = read_bytes;
    }
    return buffer;
}
