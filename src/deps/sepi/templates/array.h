#ifndef SEPI_ARRAY__CLASS__H
#define SEPI_ARRAY__CLASS__H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#define _EXTRA_DEFINES_

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/arena.h>
#include <sepi/platform.h>

#define _EXTRA_HEADERS_

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define ARRAY__CLASS__MIN_SEGMENT 6
#define ARRAY__CLASS__MAX_SEGMENT 26
#define ARRAY__CLASS__64_BITS 8 * sizeof(U64)

#define array__class__get_segment_capacity(segment) \
  ((1 << ARRAY__CLASS__MIN_SEGMENT) << (segment)) - \
      (1 << ARRAY__CLASS__MIN_SEGMENT)

#define array__class__get_segment_from_index_(index) \
  ((U32)(ARRAY__CLASS__64_BITS - get_leading_0_bits(index)))

#define array__class__get_segment_from_index(index) \
  array__class__get_segment_from_index_(            \
      ((index) >> ARRAY__CLASS__MIN_SEGMENT) + 1)

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct _Class_ _Class_;
typedef struct Array_Class_ Array_Class_;
struct Array_Class_ {
  U64 capacity;
  U64 offset;
  U64 used_segments;
  _Type_* segments[ARRAY__CLASS__MAX_SEGMENT];
  Arena* arena;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

fn Array_Class_* array__class__create(Arena* arena);
fn _Type_* array__class__get(Array_Class_* array, U64 index);
fn _Type_* array__class__push(Array_Class_* array, _Type_* ptr);
fn U32 array__class__length(Array_Class_* array);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_ARRAY__CLASS__IMPLEMENTATION

mount_slave_profiling_context();

fn Array_Class_*
array__class__create(Arena* arena) {
  start_profiling(1);

  assert(arena != 0);

  U64 used_segments = 1;

  Array_Class_* array =
      arena_push(arena, sizeof(Array_Class_), alignof(Array_Class_), TRUE);

  array->arena = arena, array->used_segments = used_segments,
  array->capacity = array__class__get_segment_capacity(used_segments),
  array->offset = 0,

  array->segments[used_segments - 1] = arena_push(
      arena, sizeof(_Type_) * array->capacity, alignof(_Type_), TRUE);

  end_profiling();
  return array;
}

fn _Type_*
array__class__get(Array_Class_* array, U64 index) {
  start_profiling(1);

  assert(array != 0);
  assert(index < array->capacity);

  _Type_* res = 0;
  U64 seg = array__class__get_segment_from_index(index);
  U64 base = (seg > 1) ? array__class__get_segment_capacity(seg - 1) : 0;
  U64 slot = index - base;
  res = array->segments[seg - 1] + slot;

  end_profiling();
  return res;
}

fn _Type_*
array__class__push(Array_Class_* array, _Type_* ptr) {
  start_profiling(1);

  assert(array != 0);
  assert(ptr != 0);

  if (array->offset >=
      (U64)array__class__get_segment_capacity(array->used_segments)) {
    U64 old_cap = array__class__get_segment_capacity(array->used_segments);
    U64 new_cap = array__class__get_segment_capacity(array->used_segments + 1);
    U64 seg_size = new_cap - old_cap;

    array->segments[array->used_segments] = arena_push(
        array->arena, sizeof(_Type_) * seg_size, alignof(_Type_), TRUE);

    array->used_segments++;
    array->capacity = array__class__get_segment_capacity(array->used_segments);
  }

  _Type_* res = array__class__get(array, array->offset);
  copy_memory(res, ptr, sizeof(_Type_));
  array->offset++;

  end_profiling();
  return res;
}

fn U32
array__class__length(Array_Class_* array) {
  /* NOTE: no place for profiling! */
  return array->offset;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_ARRAY__CLASS__IMPLEMENTATION */
#endif /* SEPI_ARRAY__CLASS__H */
