#ifndef SEPI_ARENA_H
#define SEPI_ARENA_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>

#include "base.h"
#include "walloc.h"

typedef struct arena {
  void* memory;
  sz size;
  sz offset;
  sz estimation;
  sz max_align;
  bool estimating;

#ifdef DEBUG_MODE
  sz user_requested_size;
#endif /* DEBUG_MODE */

} arena;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

#define ARENA_DEFAULT_ALIGNMENT NATIVE_ALIGNMENT
#define ARENA_MIN_ALIGNMENT 8
#define ARENA_MAX_ALIGNMENT 128

void arena_create(arena* a, sz size);
void arena_destroy(arena* a);
void* arena_alloc(arena* a, sz size, sz alignment);
void arena_reset(arena* a);
void arena_estimate_begin(arena* a);
void arena_estimate_add(arena* a, sz size, sz alignment);
void arena_estimate_end(arena* a);

#define arena_push_struct(A, T) \
  (T*)arena_alloc((A), (sz)sizeof(T), (sz)ALIGNOF(T))

#define arena_push_array(A, T, N) \
  (T*)arena_alloc((A), (sz)(sizeof(T) * (N)), (sz)ALIGNOF(T))

#define arena_talloc(A, T) arena_alloc((A), (sz)sizeof(T), (sz)ALIGNOF(T))
#define arena_testimate_add(A, T) \
  arena_estimate_add((A), (sz)sizeof(T), (sz)ALIGNOF(T))

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_ARENA_IMPLEMENTATION

void arena_create(arena* a, sz size) {
  NOTNULL(a);
  NOTZERO(size);
  MAKESURE(a->memory == NULL, "arena is already allocated");

  void* memory = wcalloc(1, size);
  NOTNULL(memory);

  a->memory = memory;
  a->size = size;
  a->offset = 0;
  a->estimating = false;
  a->estimation = 0;
  a->max_align = ARENA_DEFAULT_ALIGNMENT;

#ifdef DEBUG_MODE
  a->user_requested_size = size;
#endif /* DEBUG_MODE */
}

void arena_destroy(arena* a) {
  NOTNULL(a);

  if (a->memory)
    wfree(a->memory);

  a->memory = NULL;
  a->size = 0;
  a->offset = 0;
  a->estimating = false;
  a->estimation = 0;
  a->max_align = 0;

#ifdef DEBUG_MODE
  a->user_requested_size = 0;
#endif /* DEBUG_MODE */
}

void* arena_alloc(arena* a, sz size, sz alignment) {
  NOTNULL(a);
  NOTNULL(a->memory);
  NOTZERO(size);
  NOTZERO(alignment);
  ISVALID(IS_POW2(alignment));

  uintptr_t base = (uintptr_t)a->memory;
  uintptr_t cur = base + a->offset;
  uintptr_t aptr = ALIGN_UP(cur, alignment); /* ALIGN_UP takes integers */
  ISVALID(aptr >= base);

  sz used = (sz)(aptr - base);
  ISVALID(used <= a->size);
  ISVALID(size <= a->size - used);

  a->offset = used + size;
  if (a->max_align < alignment)
    a->max_align = alignment;

  return (void*)aptr;
}

void arena_reset(arena* a) {
  NOTNULL(a);
  NOTNULL(a->memory);

  a->offset = 0;
  a->estimating = false;

#ifdef DEBUG_MODE
  memset(a->memory, 0, a->size);
  a->user_requested_size = 0;
#endif /* DEBUG_MODE */
}

void arena_estimate_begin(arena* a) {
  NOTNULL(a);
  ISVALID(a->memory == NULL);

  a->memory = NULL;
  a->size = 0;
  a->offset = 0;
  a->estimating = true;
  a->estimation = 0;

#ifdef DEBUG_MODE
  a->user_requested_size = 0;
#endif /* DEBUG_MODE */
}

void arena_estimate_add(arena* a, sz size, sz alignment) {
  NOTNULL(a);
  NOTZERO(size);
  NOTZERO(alignment);
  ISVALID(IS_POW2(alignment));

  a->estimation += size;
  if (a->max_align < alignment) {
    a->max_align = alignment;
  }

#ifdef DEBUG_MODE
  a->user_requested_size += size;
#endif /* DEBUG_MODE */
}

void arena_estimate_end(arena* a) {
  NOTNULL(a);
  NOTZERO(a->estimating);
  NOTZERO(a->max_align);

  sz size = ALIGN_UP(a->estimation, a->max_align);
  void* memory = wcalloc(1, size);
  NOTNULL(memory);

  a->memory = memory;
  a->size = size;
  a->offset = 0;
  a->estimating = false;
  a->estimation = 0;

#ifdef DEBUG_MODE
  printf("============== ARENA.H ==============\n");
  printf(" user requested size: %zu\n", a->user_requested_size);
  printf(" real allocated size: %zu\n", a->size);
  printf("=====================================\n");
#endif /* DEBUG_MODE */
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_ARENA_IMPLEMENTATION */
#endif /* SEPI_ARENA_H */
