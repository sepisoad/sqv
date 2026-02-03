#ifndef SEPI_ARRAY_H
#define SEPI_ARRAY_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../tracy/tracy.h"

#include "base.h"
#include "string.h"
#include "arena.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define ARRAY_MIN_SEGMENT 6
#define ARRAY_MAX_SEGMENT 26
#define ARRAY_64_BITS 8 * sizeof(U64)

#define ARRAY_SEGMENT_CAPACITY(SEG) \
  ((1 << ARRAY_MIN_SEGMENT) << (SEG)) - (1 << ARRAY_MIN_SEGMENT)
#define ARRAY_GET_IDX_SEGMENT(VAL) ((U32)(ARRAY_64_BITS - LeadingZeroBits(VAL)))
#define ARRAY_GET_IDX_SEGMENT_NORMALIZED(IDX) \
  ARRAY_GET_IDX_SEGMENT(((IDX) >> ARRAY_MIN_SEGMENT) + 1)

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct Array Array;
struct Array {
  Arena* arena;
  U64 capacity;
  U64 offset;
  U64 used_segments;
  struct {
    Sz item_size;
    Sz item_alignment;
  } meta;
  RawPtr segments[ARRAY_MAX_SEGMENT];
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Array* array_create(Arena* arena, Sz item_size, Sz item_alignment);
RawPtr array_push(Array* array, RawPtr ptr);
RawPtr array_get(Array* array, U64 index);

#define ArrayOf(T) Array*

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_ARRAY_IMPLEMENTATION

SLAVE_PROFILING_CONTEXT;

Array*
array_create(Arena* arena, Sz item_size, Sz item_alignment) {
  START_PROFILING(1);

  Assert(arena != 0);
  Assert(item_size > 0);
  Assert(item_alignment > 0);

  Array* array = arena_push(arena, sizeof(Array), AlignOf(Array), TRUE);

  array->arena = arena;
  array->used_segments = 1;
  array->capacity = ARRAY_SEGMENT_CAPACITY(array->used_segments);
  array->offset = 0;
  array->meta.item_size = item_size;
  array->meta.item_alignment = item_alignment;

  array->segments[array->used_segments - 1] =
      arena_push(arena, item_size * array->capacity, item_alignment, TRUE);

  END_PROFILING();
  return array;
}

RawPtr
array_push(Array* array, RawPtr ptr) {
  START_PROFILING(1);

  Assert(array != 0);
  Assert(ptr != 0);

  if (array->offset >= (U64)ARRAY_SEGMENT_CAPACITY(array->used_segments)) {
    U64 old_cap = ARRAY_SEGMENT_CAPACITY(array->used_segments);
    U64 new_cap = ARRAY_SEGMENT_CAPACITY(array->used_segments + 1);
    U64 seg_size = new_cap - old_cap;

    array->segments[array->used_segments] = arena_push(
        array->arena, array->meta.item_size * seg_size, array->meta.item_alignment, TRUE);

    array->used_segments++;
    array->capacity = ARRAY_SEGMENT_CAPACITY(array->used_segments);
  }

  // TODO:
  // it's late night and i came across this line of code
  // and i don't remember why i added it! my brain is not
  // working ATM, but i would like to come back to this
  // line later and remember what the fuck is going on!
  RawPtr res = array_get(array, array->offset);
  MemCopy(res, ptr, array->meta.item_size);
  array->offset++;

  END_PROFILING();
  return res;
}

RawPtr
array_get(Array* array, U64 index) {
  START_PROFILING(1);

  Assert(array != 0);
  Assert(index < array->capacity);

  RawPtr res = 0;
  U64 seg = ARRAY_GET_IDX_SEGMENT_NORMALIZED(index);
  U64 base = (seg > 1) ? ARRAY_SEGMENT_CAPACITY(seg - 1) : 0;
  U64 slot = index - base;
  res = array->segments[seg - 1] + (array->meta.item_size * slot);

  END_PROFILING();

  return res;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_ARRAY_IMPLEMENTATION */
#endif /* SEPI_ARRAY_H */
