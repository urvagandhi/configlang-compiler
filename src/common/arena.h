/**
 * @file arena.h
 * @brief Arena (region-based) memory allocator for AST and symbol table.
 * @module arena
 * @phase Common
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#ifndef CONFIGLANG_ARENA_H
#define CONFIGLANG_ARENA_H

#include <stddef.h>

/** Default size for each arena block chunk (64 KB). */
#define ARENA_DEFAULT_CHUNK_SIZE (64U * 1024U)

/** Opaque pointer to arena allocator instance. */
typedef struct arena arena_t;

/**
 * @brief Create a new arena allocator with an optional initial chunk size.
 * @param chunk_size Chunk size in bytes (pass 0 for ARENA_DEFAULT_CHUNK_SIZE).
 * @return Pointer to freshly initialized arena instance.
 */
arena_t *arena_create(size_t chunk_size);

/**
 * @brief Allocate a zeroed memory block aligned to 8 bytes from the arena.
 * @param arena Pointer to arena instance.
 * @param size Number of bytes to allocate.
 * @return Aligned pointer to allocated memory block.
 */
void *arena_alloc(arena_t *arena, size_t size);

/**
 * @brief Duplicate a null-terminated string into arena memory.
 * @param arena Pointer to arena instance.
 * @param str Source string to copy.
 * @return Pointer to string in arena, or NULL if source is NULL.
 */
char *arena_strdup(arena_t *arena, const char *str);

/**
 * @brief Free all memory chunks associated with the arena and the arena itself.
 * @param arena Pointer to arena instance to destroy.
 */
void arena_destroy(arena_t *arena);

#endif /* CONFIGLANG_ARENA_H */
