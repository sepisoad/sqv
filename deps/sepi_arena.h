#ifndef SEPI_ARENA_HEADER_
#define SEPI_ARENA_HEADER_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "sepi_types.h"
#include "sepi_macros.h"

#define ARENA_DEFAULT_SIZE (1024 * 1024)  // 1MB default size
#define ALIGNMENT 16                      // Default alignment

typedef struct {
  u8* base;     // Start of allocated memory (NULL during estimation phase)
  sz offset;    // Current offset in arena
  sz size;      // Total size of the arena
  sz estimate;  // Memory estimate during pre-allocation phase
} arena;

/* ****************** API ****************** */
void arena_create(arena* a, sz size);
void arena_destroy(arena* a);
void* arena_alloc(arena* a, sz size, sz alignment);
void arena_reset(arena* a);
void arena_begin_estimate(arena* a);
void arena_estimate_add(arena* a, sz size, sz alignment);
sz arena_end_estimate(arena* a);
void arena_print(arena* a);
/* ****************** API ****************** */

#ifdef SEPI_ARENA_IMPLEMENTATION

// .--------------------------------------------------------------------------.
// | _                 _                           _        _   _             |
// |(_)               | |                         | |      | | (_)            |
// | _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __  |
// || | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \ |
// || | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | ||
// ||_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_||
// |            | |                                                           |
// |            |_|                                                           |
// '--------------------------------------------------------------------------'

// Helper function to align memory addresses
static inline sz align_up(sz ptr, sz alignment) {
  sz align = (ptr + (alignment - 1)) & ~(alignment - 1);
  return align;
}

/* ****************** Arena Initialization & Destruction ****************** */

void arena_create(arena* a, sz size) {
  sz aligned_size = align_up(size, ALIGNMENT);
  a->base = (u8*)calloc(1, aligned_size);  // Allocate and zero-initialize
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

void* arena_alloc(arena* a, sz size, sz alignment) {
  sz aligned_offset = align_up((sz)(a->base + a->offset), alignment);
  sz padding = aligned_offset - (sz)(a->base + a->offset);

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

void arena_estimate_add(arena* a, sz size, sz alignment) {
  sz aligned_offset = align_up(a->estimate, alignment);
  a->estimate = aligned_offset + size;
}

sz arena_end_estimate(arena* a) {
  sz final_size = align_up(a->estimate, ALIGNMENT);
  a->base = (u8*)calloc(1, final_size);
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
#endif  // SEPI_ARENA_HEADER_
