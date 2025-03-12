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
  uint8_t* base;    // Start of allocated memory (NULL during estimation phase)
  size_t offset;    // Current offset in arena
  size_t size;      // Total size of the arena
  size_t estimate;  // Memory estimate during pre-allocation phase
} arena;

/* ****************** utils::arena API ****************** */

// Arena initialization and destruction
void arena_create(arena* a, size_t size);
void arena_destroy(arena* a);

// Arena allocation
void* arena_alloc(arena* a, size_t size, size_t alignment);
void arena_reset(arena* a);

// Memory estimation API
void arena_begin_estimate(arena* a);
void arena_estimate_add(arena* a, size_t size, size_t alignment);
size_t arena_end_estimate(arena* a);

// Debug API
void arena_print(arena* a);

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

// Helper function to align memory addresses
static inline size_t align_up(size_t ptr, size_t alignment) {
  size_t align = (ptr + (alignment - 1)) & ~(alignment - 1);
  return align;
}

/* ****************** Arena Initialization & Destruction ****************** */

void arena_create(arena* a, size_t size) {
  size_t aligned_size = align_up(size, ALIGNMENT);
  a->base = (uint8_t*)calloc(1, aligned_size);  // Allocate and zero-initialize
  makesure(a->base != NULL, "arena_create failed");

  a->offset = 0;
  a->size = aligned_size;
  a->estimate = 0;  // Not used after allocation phase
}

void arena_destroy(arena* a) {
  if (a->base)
    free(a->base);
  a->base = NULL;
  a->offset = 0;
  a->size = 0;
  a->estimate = 0;
}

/* ****************** Arena Allocation API ****************** */

void* arena_alloc(arena* a, size_t size, size_t alignment) {
  size_t aligned_offset = align_up((size_t)(a->base + a->offset), alignment);
  size_t padding = aligned_offset - (size_t)(a->base + a->offset);

  if (a->offset + padding + size > a->size) {
    return NULL;
  }

  void* ptr = a->base + a->offset + padding;
  a->offset += padding + size;
  return ptr;
}

void arena_reset(arena* a) {
  a->offset = 0;
}

/* ****************** Memory Estimation API ****************** */

void arena_begin_estimate(arena* a) {
  a->offset = 0;
  a->estimate = 0;
  a->base = NULL;
}

void arena_estimate_add(arena* a, size_t size, size_t alignment) {
  size_t aligned_offset = align_up(a->estimate, alignment);
  a->estimate = aligned_offset + size;
}

size_t arena_end_estimate(arena* a) {
  size_t final_size = align_up(a->estimate, ALIGNMENT);
  a->base = (uint8_t*)calloc(1, final_size);
  makesure(a->base != NULL, "arena_end_estimate failed");

  a->offset = 0;
  a->size = final_size;
  return final_size;
}

/* ****************** Debug API ****************** */

void arena_print(arena* a) {
  printf("==========================\n");
  printf("== size:     %zu \n", a->size);
  printf("== estimate: %zu \n", a->estimate);
  printf("== offset:   %zu \n", a->offset);
  printf("==========================\n");
}

#endif  // UTILS_ARENA_IMPLEMENTATION
#endif  // UTILS_ARENA_HEADER_
