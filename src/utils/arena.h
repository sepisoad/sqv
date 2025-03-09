#ifndef UTILS_ARENA_HEADER_
#define UTILS_ARENA_HEADER_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "macros.h"

#define ARENA_DEFAULT_SIZE (1024 * 1024)  // 1MB default size
#define ALIGNMENT 16                      // Default alignment

typedef struct {
  uint8_t* base;  // Start of allocated memory
  size_t offset;  // Current offset in arena
  size_t size;    // Total size of the arena
} arena;

/* ****************** utils::arena API ****************** */
arena arena_create(size_t size);
void* arena_alloc(arena* a, size_t size, size_t alignment);
void arena_reset(arena* a);
void arena_destroy(arena* a);
/* ****************** utils::arena API ****************** */

#ifdef UTILS_ARENA_IMPLEMENTATION

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

static inline size_t align_up(size_t ptr, size_t alignment) {
  return (ptr + (alignment - 1)) & ~(alignment - 1);
}

arena arena_create(size_t size) {
  size_t aligned_size = align_up(size, ALIGNMENT);  // Ensure alignment

  void* mem = calloc(1, aligned_size);  // Zero-initialized memory
  makesure(mem != NULL, "arena_creat failed");

  arena a = {.base = (uint8_t*)mem, .offset = 0, .size = aligned_size};
  return a;
}

void* arena_alloc(arena* a, size_t size, size_t alignment) {
  size_t aligned_offset = align_up((size_t)(a->base + a->offset), alignment);
  size_t padding = aligned_offset - (size_t)(a->base + a->offset);
  makesure(a->offset + padding + size <= a->size, "arena out of memory !");

  void* ptr = a->base + a->offset + padding;
  a->offset += padding + size;
  return ptr;
}

void arena_reset(arena* a) {
  a->offset = 0;
}

void arena_destroy(arena* a) {
  free(a->base);
  a->base = NULL;
  a->offset = 0;
  a->size = 0;
}

#endif  // UTILS_ARENA_IMPLEMENTATION
#endif  // UTILS_ARENA_HEADER_