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
  U64 item_size;
  U64 item_alignment;
  U64 capacity;
  U64 offset;
  U64 used_segments;
  RawPtr segments[ARRAY_MAX_SEGMENT];
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Array* array_create(Arena* a, U64 item_size, U64 item_alignment);
RawPtr array_push(Array* a, RawPtr ptr);
RawPtr array_get(Array* a, U64 index);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_ARRAY_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

Array*
array_create(Arena* a, U64 item_size, U64 item_alignment) {
  TracyCZoneN(tracyctx, "array_create", 1);

  Assert(a != 0);
  Assert(item_size > 0);
  Assert(item_alignment > 0);

  Array* array = arena_push(a, sizeof(Array), AlignOf(Array), TRUE);

  array->arena = a;
  array->item_size = item_size;
  array->item_alignment = item_alignment;
  array->used_segments = 1;
  array->capacity = ARRAY_SEGMENT_CAPACITY(array->used_segments);
  array->offset = 0;

  array->segments[array->used_segments - 1] =
      arena_push(a, item_size * array->capacity, item_alignment, TRUE);

  TracyCZoneEnd(tracyctx);
  return array;
}

RawPtr
array_push(Array* a, RawPtr ptr) {
  TracyCZoneN(tracyctx, "array_push", 1);

  Assert(a != 0);
  Assert(ptr != 0);

  if (a->offset >= (U64)ARRAY_SEGMENT_CAPACITY(a->used_segments)) {
    U64 old_cap = ARRAY_SEGMENT_CAPACITY(a->used_segments);
    U64 new_cap = ARRAY_SEGMENT_CAPACITY(a->used_segments + 1);
    U64 seg_size = new_cap - old_cap;

    a->segments[a->used_segments] =
        arena_push(a->arena, a->item_size * seg_size, a->item_alignment, TRUE);

    a->used_segments++;
    a->capacity = ARRAY_SEGMENT_CAPACITY(a->used_segments);
  }

  RawPtr res = array_get(a, a->offset);
  MemCopy(res, ptr, a->item_size);
  a->offset++;

  TracyCZoneEnd(tracyctx);

  return res;
}

RawPtr
array_get(Array* a, U64 index) {
  TracyCZoneN(tracyctx, "array_get", 1);

  Assert(a != 0);
  Assert(index < a->capacity);

  RawPtr res = 0;
  U64 seg = ARRAY_GET_IDX_SEGMENT_NORMALIZED(index);
  U64 base = (seg > 1) ? ARRAY_SEGMENT_CAPACITY(seg - 1) : 0;
  U64 slot = index - base;
  res = a->segments[seg - 1] + (a->item_size * slot);

  TracyCZoneEnd(tracyctx);

  return res;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_ARRAY_IMPLEMENTATION */
#endif /* SEPI_ARRAY_H */
