/**
 * ============================================================================
 * Arena Memory Allocator Implementation (arena.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * HOW IT WORKS IN 3 SIMPLE STEPS:
 *   1. arena_create() allocates the first memory chunk (e.g. 64 KB).
 *   2. arena_alloc() returns a pointer to the next free bytes inside the chunk.
 *      If the current chunk runs out of space, it allocates a new chunk and
 *      links it in a simple linked list.
 *   3. arena_destroy() walks through the linked list and frees every chunk
 *      at once when the compiler is done.
 * ============================================================================
 */

#include "arena.h"
#include "util.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** Byte alignment (8 bytes) so 64-bit ints and pointers are properly aligned */
#define ARENA_ALIGNMENT 8U

/** A single memory block (chunk) in our linked list */
typedef struct arena_chunk {
    struct arena_chunk *next; /* Next memory chunk in list */
    size_t capacity;          /* Total size of this chunk in bytes */
    size_t offset;            /* How many bytes are currently used */
    char *data;               /* The actual memory buffer */
} arena_chunk_t;

/** Arena manager keeping track of the chunk linked list */
struct arena {
    arena_chunk_t *head;      /* First chunk in the list */
    arena_chunk_t *current;   /* Chunk currently being allocated from */
    size_t default_chunk_sz;  /* Default chunk size (64 KB) */
};

/** Create a new memory chunk of specified capacity */
static arena_chunk_t *arena_chunk_create(size_t capacity) {
    arena_chunk_t *chunk = (arena_chunk_t *)util_xmalloc(sizeof(arena_chunk_t));
    chunk->next = NULL;
    chunk->capacity = capacity;
    chunk->offset = 0;
    chunk->data = (char *)util_xmalloc(capacity);
    return chunk;
}

/** Initialize a new arena memory pool */
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
