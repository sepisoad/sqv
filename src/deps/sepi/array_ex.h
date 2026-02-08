#ifndef SEPI_ARRAY_EX_H
#define SEPI_ARRAY_EX_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../tracy/tracy.h"

#include "base.h"
#include "arena.h"
#include "platform.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define ARRAY_EX_MIN_SEGMENT 6
#define ARRAY_EX_MAX_SEGMENT 26
#define ARRAY_EX_64_BITS 8 * sizeof(U64)

#define ARRAY_EX_SEGMENT_CAPACITY(SEG) \
  ((1 << ARRAY_EX_MIN_SEGMENT) << (SEG)) - (1 << ARRAY_EX_MIN_SEGMENT)
#define ARRAY_EX_GET_IDX_SEGMENT(VAL) \
  ((U32)(ARRAY_EX_64_BITS - LeadingZeroBits(VAL)))
#define ARRAY_EX_GET_IDX_SEGMENT_NORMALIZED(IDX) \
  ARRAY_EX_GET_IDX_SEGMENT(((IDX) >> ARRAY_EX_MIN_SEGMENT) + 1)

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

#define DefineArray(TYPE, CLASS, METHOD)                                       \
  typedef struct Array##CLASS Array##CLASS;                                    \
  struct Array##CLASS {                                                        \
    U64 capacity;                                                              \
    U64 offset;                                                                \
    U64 used_segments;                                                         \
    TYPE* segments[ARRAY_EX_MAX_SEGMENT];                                      \
    Arena* arena;                                                              \
  };                                                                           \
                                                                               \
  Array##CLASS array_##METHOD##_make(Arena* arena) {                           \
    START_PROFILING(1);                                                        \
                                                                               \
    Assert(arena != 0);                                                        \
                                                                               \
    U64 used_segments = 1;                                                     \
    Array##CLASS array = {                                                     \
        .arena = arena,                                                        \
        .used_segments = used_segments,                                        \
        .capacity = ARRAY_EX_SEGMENT_CAPACITY(used_segments),                  \
        .offset = 0,                                                           \
    };                                                                         \
    array.segments[used_segments - 1] =                                        \
        arena_push(arena, sizeof(TYPE) * array.capacity, AlignOf(TYPE), TRUE); \
                                                                               \
    END_PROFILING();                                                           \
    return array;                                                              \
  }                                                                            \
                                                                               \
  Nothing array_##METHOD##_init(Arena* arena, Array##CLASS* array) {           \
    START_PROFILING(1);                                                        \
                                                                               \
    Assert(arena != 0);                                                        \
    Assert(array != 0);                                                        \
    Assert(array->arena == 0);                                                 \
    Assert(array->used_segments == 0);                                         \
    Assert(array->capacity == 0);                                              \
    Assert(array->offset == 0);                                                \
                                                                               \
    U64 used_segments = 1;                                                     \
    array->arena = arena;                                                      \
    array->used_segments = used_segments;                                      \
    array->capacity = ARRAY_EX_SEGMENT_CAPACITY(used_segments);                \
    array->offset = 0;                                                         \
                                                                               \
    array->segments[used_segments - 1] = arena_push(                           \
        arena, sizeof(TYPE) * array->capacity, AlignOf(TYPE), TRUE);           \
                                                                               \
    END_PROFILING();                                                           \
  }                                                                            \
                                                                               \
  TYPE* array_##METHOD##_get(Array##CLASS* array, U64 index) {                 \
    START_PROFILING(1);                                                        \
                                                                               \
    Assert(array != 0);                                                        \
    Assert(index < array->capacity);                                           \
                                                                               \
    TYPE* res = 0;                                                             \
    U64 seg = ARRAY_EX_GET_IDX_SEGMENT_NORMALIZED(index);                      \
    U64 base = (seg > 1) ? ARRAY_EX_SEGMENT_CAPACITY(seg - 1) : 0;             \
    U64 slot = index - base;                                                   \
    res = array->segments[seg - 1] + slot;                                     \
                                                                               \
    END_PROFILING();                                                           \
    return res;                                                                \
  }                                                                            \
                                                                               \
  TYPE* array_##METHOD##_push(Array##CLASS* array, TYPE* ptr) {                \
    START_PROFILING(1);                                                        \
                                                                               \
    Assert(array != 0);                                                        \
    Assert(ptr != 0);                                                          \
                                                                               \
    if (array->offset >=                                                       \
        (U64)ARRAY_EX_SEGMENT_CAPACITY(array->used_segments)) {                \
      U64 old_cap = ARRAY_EX_SEGMENT_CAPACITY(array->used_segments);           \
      U64 new_cap = ARRAY_EX_SEGMENT_CAPACITY(array->used_segments + 1);       \
      U64 seg_size = new_cap - old_cap;                                        \
                                                                               \
      array->segments[array->used_segments] = arena_push(                      \
          array->arena, sizeof(TYPE) * seg_size, AlignOf(TYPE), TRUE);         \
                                                                               \
      array->used_segments++;                                                  \
      array->capacity = ARRAY_EX_SEGMENT_CAPACITY(array->used_segments);       \
    }                                                                          \
                                                                               \
    TYPE* res = array_##METHOD##_get(array, array->offset);                    \
    MemCopy(res, ptr, sizeof(TYPE));                                           \
    array->offset++;                                                           \
                                                                               \
    END_PROFILING();                                                           \
    return res;                                                                \
  }                                                                            \
                                                                               \
  U32 array_##METHOD##_length(Array##CLASS* array) {                           \
    /* NOTE: no place for profiling! */                                        \
    return array->offset;                                                      \
  }

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// --

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_ARRAY_EX_IMPLEMENTATION

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_ARRAY_EX_IMPLEMENTATION */
#endif /* SEPI_ARRAY_EX_H */
