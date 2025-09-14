#ifndef SEPI_GARENA_H
#define SEPI_GARENA_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>

#include "base.h"
#include "walloc.h"

#ifndef GARENA_FIRST_CAP
#define GARENA_FIRST_CAP (256 * 1024) /* first block ~256 KiB */
#endif

typedef struct garena_block {
  struct garena_block* next;
  sz capacity;
  sz position;
  u8 data[];  // ok, this points to the memory the is assigned to a block
} garena_block;

typedef struct {
  garena_block* block;
  sz position;
} garena_mark;

typedef struct {
  garena_block* head;
  garena_block* curr;
  sz next_capacity;
} garena;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

void garena_create(garena* a, sz init_cap);
void garena_destroy(garena* a);
void* garena_alloc(garena* a, sz size, sz alignment);
void* garena_alloc_zero(garena* a, sz size, sz alignment);
garena_mark garena_get_mark(garena* a);
void garena_release(garena* a, garena_mark m);
void garena_reset(garena* a);

#define garena_push_struct(a, T) \
  (T*)garena_alloc((a), (sz)sizeof(T), (sz)alignof(T))

#define garena_push_array(a, T, N) \
  (T*)garena_alloc((a), (sz)(sizeof(T) * (N)), (sz)alignof(T))

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_GARENA_IMPLEMENTATION

static garena_block* garena_block_create(sz min_cap) {
  sz capacity = (min_cap < GARENA_FIRST_CAP) ? GARENA_FIRST_CAP : min_cap;
  sz size = sizeof(garena_block) + capacity;

  garena_block* b = (garena_block*)wmalloc((size_t)size);
  NOTNULL(b);

  b->next = NULL;
  b->capacity = capacity;
  b->position = 0;
  return b;
}

void garena_create(garena* a, sz init_cap) {
  NOTNULL(a);

  memset(a, 0, sizeof(*a));
  sz capacity = init_cap > 0 ? init_cap : (sz)GARENA_FIRST_CAP;
  a->head = a->curr = garena_block_create(capacity);
  a->next_capacity = capacity * 2;
}

void garena_destroy(garena* a) {
  NOTNULL(a);

  garena_block* block = a->head;
  while (block) {
    garena_block* next = block->next;
    wfree(block);
    block = next;
  }
  memset(a, 0, sizeof(*a));
}

void* garena_alloc(garena* a, sz size, sz alignment) {
  NOTNULL(a);
  NOTNULL(a->curr);
  NOTZERO(size);
  NOTZERO(alignment);
  ISVALID((alignment & (alignment - 1)) == 0);

  garena_block* block = a->curr;
  sz aligned = ALIGN_UP(block->position, alignment);

  if (aligned <= block->capacity && size <= (block->capacity - aligned)) {
    void* mem = (void*)(block->data + aligned);
    block->position = aligned + size;
    return mem;
  }

  sz need = size + alignment;
  sz capacity = a->next_capacity ? a->next_capacity : (sz)GARENA_FIRST_CAP;
  while (capacity < need)
    capacity *= 2;

  garena_block* next = garena_block_create(capacity);
  NOTNULL(next);

  block->next = next;
  a->curr = next;
  a->next_capacity = capacity * 2;

  sz aligned2 = ALIGN_UP(next->position, alignment);
  void* p = (void*)(next->data + aligned2);
  next->position = aligned2 + size;

  return p;
}

void* garena_alloc_zero(garena* a, sz size, sz alignment) {
  void* p = garena_alloc(a, size, alignment);
  if (p)
    memset(p, 0, (size_t)size);
  return p;
}

garena_mark garena_get_mark(garena* a) {
  garena_mark m = {0};
  m.block = a->curr;
  m.position = a->curr ? a->curr->position : 0;
  return m;
}

void garena_release(garena* a, garena_mark m) {
  NOTNULL(a);
  NOTNULL(a->head);

  if (m.block) {
    /* free everything after the marked block */
    garena_block* t = m.block->next;
    m.block->next = NULL;
    while (t) {
      garena_block* n = t->next;
      wfree(t);
      t = n;
    }
    a->curr = m.block;

#ifdef DEBUG_MODE
    if (a->curr->position > m.position) {
      memset(a->curr->data + m.position, 0xDD,
             (size_t)(a->curr->position - m.position));
    }
#endif /* DEBUG_MODE */

    a->curr->position = m.position;
    /* growth target can stay; it only affects future block sizes */
  } else {
    /* rewind to "empty": keep head, free rest */
    garena_block* head = a->head;
    if (head) {
#ifdef DEBUG_MODE
      if (head->position)
        memset(head->data, 0xDD, (size_t)head->position);
#endif /* DEBUG_MODE */

      garena_block* t = head->next;
      head->next = NULL;
      head->position = 0;
      while (t) {
        garena_block* n = t->next;
        wfree(t);
        t = n;
      }
      a->curr = head;
    }
  }
}

void garena_reset(garena* a) {
  NOTNULL(a);
  NOTNULL(a->head);

#ifdef DEBUG_MODE
  /* poison used bytes in all blocks */
  for (garena_block* b = a->head; b; b = b->next) {
    if (b->position)
      memset(b->data, 0xDD, (size_t)b->position);
  }
#endif /* DEBUG_MODE */

  /* keep the first block; free others */
  garena_mark m = {a->head, 0};
  garena_release(a, m);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_GARENA_IMPLEMENTATION */
#endif /* SEPI_GARENA_H */
