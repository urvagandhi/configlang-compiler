/**
 * @file arena.c
 * @brief Arena memory allocator implementation with 8-byte alignment.
 * @module arena
 * @phase Common
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "arena.h"
#include "util.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** Byte alignment requirement for all arena allocations. */
#define ARENA_ALIGNMENT 8U

/**
 * @brief Linked list chunk of arena memory.
 */
typedef struct arena_chunk {
    struct arena_chunk *next; /**< Pointer to subsequent chunk. */
    size_t capacity;          /**< Total capacity of this chunk. */
    size_t offset;            /**< Current allocation offset. */
    char *data;               /**< Pointer to payload data buffer. */
} arena_chunk_t;

/**
 * @brief Arena allocator state structure.
 */
struct arena {
    arena_chunk_t *head;      /**< Head of chunk linked list. */
    arena_chunk_t *current;   /**< Currently active chunk for new allocations. */
    size_t default_chunk_sz;  /**< Configured default chunk size. */
};

/**
 * @brief Helper to allocate a new arena chunk.
 * @param capacity Minimum byte capacity for chunk.
 * @return Freshly allocated and zero-initialized chunk.
 */
static arena_chunk_t *arena_chunk_create(size_t capacity) {
    arena_chunk_t *chunk = (arena_chunk_t *)util_xmalloc(sizeof(arena_chunk_t));
    chunk->next = NULL;
    chunk->capacity = capacity;
    chunk->offset = 0;
    chunk->data = (char *)util_xmalloc(capacity);
    return chunk;
}

arena_t *arena_create(size_t chunk_size) {
    arena_t *arena = (arena_t *)util_xmalloc(sizeof(arena_t));
    arena->default_chunk_sz = (chunk_size > 0) ? chunk_size : ARENA_DEFAULT_CHUNK_SIZE;
    arena->head = arena_chunk_create(arena->default_chunk_sz);
    arena->current = arena->head;
    return arena;
}

void *arena_alloc(arena_t *arena, size_t size) {
    if (arena == NULL || size == 0) {
        return NULL;
    }
    /* Align size up to ARENA_ALIGNMENT */
    size_t aligned_size = (size + (ARENA_ALIGNMENT - 1U)) & ~(ARENA_ALIGNMENT - 1U);
    arena_chunk_t *cur = arena->current;

    if (cur->offset + aligned_size > cur->capacity) {
        size_t next_cap = (aligned_size > arena->default_chunk_sz)
                              ? aligned_size
                              : arena->default_chunk_sz;
        arena_chunk_t *new_chunk = arena_chunk_create(next_cap);
        cur->next = new_chunk;
        arena->current = new_chunk;
        cur = new_chunk;
    }

    void *ptr = cur->data + cur->offset;
    cur->offset += aligned_size;
    memset(ptr, 0, aligned_size);
    return ptr;
}

char *arena_strdup(arena_t *arena, const char *str) {
    if (arena == NULL || str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    char *copy = (char *)arena_alloc(arena, len + 1U);
    memcpy(copy, str, len + 1U);
    return copy;
}

void arena_destroy(arena_t *arena) {
    if (arena == NULL) {
        return;
    }
    arena_chunk_t *chunk = arena->head;
    while (chunk != NULL) {
        arena_chunk_t *next = chunk->next;
        free(chunk->data);
        free(chunk);
        chunk = next;
    }
    free(arena);
}
