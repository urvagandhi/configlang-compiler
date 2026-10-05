/**
 * @file util.h
 * @brief Common utility functions and dynamic string buffer.
 * @module util
 * @phase Common
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#ifndef CONFIGLANG_UTIL_H
#define CONFIGLANG_UTIL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** Initial default capacity for dynamic string buffers. */
#define UTIL_STRBUF_INIT_CAP 64U

/**
 * @brief Dynamic string buffer for formatting and token accumulation.
 */
typedef struct strbuf {
    char *data;         /**< Pointer to heap-allocated buffer. */
    size_t length;      /**< Current string length (excluding null byte). */
    size_t capacity;    /**< Total allocated capacity in bytes. */
} strbuf_t;

/**
 * @brief Allocate memory or abort with an error message on failure.
 * @param size Number of bytes to allocate.
 * @return Non-null pointer to allocated memory.
 */
void *util_xmalloc(size_t size);

/**
 * @brief Allocate zeroed memory or abort on failure.
 * @param count Number of elements.
 * @param size Size of each element in bytes.
 * @return Non-null pointer to zeroed memory.
 */
void *util_xcalloc(size_t count, size_t size);

/**
 * @brief Reallocate memory or abort on failure.
 * @param ptr Pointer to previously allocated block, or NULL.
 * @param size New size in bytes.
 * @return Non-null pointer to reallocated memory.
 */
void *util_xrealloc(void *ptr, size_t size);

/**
 * @brief Duplicate a null-terminated string using util_xmalloc.
 * @param str Source string to duplicate.
 * @return Pointer to freshly allocated copy.
 */
char *util_xstrdup(const char *str);

/**
 * @brief Initialize a new dynamic string buffer.
 * @return Initialized string buffer struct.
 */
strbuf_t util_strbuf_create(void);

/**
 * @brief Append a null-terminated string to the buffer.
 * @param sb Pointer to string buffer.
 * @param str Null-terminated string to append.
 */
void util_strbuf_append(strbuf_t *sb, const char *str);

/**
 * @brief Append a single character to the buffer.
 * @param sb Pointer to string buffer.
 * @param ch Character to append.
 */
void util_strbuf_append_char(strbuf_t *sb, char ch);

/**
 * @brief Free heap memory used by the string buffer and reset fields.
 * @param sb Pointer to string buffer.
 */
void util_strbuf_destroy(strbuf_t *sb);

/**
 * @brief Read an entire text file into a heap-allocated buffer.
 * @param path Path to file.
 * @param out_size Optional pointer to receive size in bytes (can be NULL).
 * @return Null-terminated string buffer, or NULL on error.
 */
char *util_read_file(const char *path, size_t *out_size);

#endif /* CONFIGLANG_UTIL_H */
