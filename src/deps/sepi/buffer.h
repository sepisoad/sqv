#ifndef SEPI_BUFFER_H
#define SEPI_BUFFER_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdlib.h>

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/endian.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct Buf Buf;
struct Buf {
  Sz size;
  Sz offset;
  const U8* base;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

static inline Buf buf(const U8* base, Sz size);
static inline Nothing buf_reset(Buf buf);
static inline Nothing buf_set_position(Buf buf, Sz offset);
static inline Nothing buf_move_offset(Buf buf, Sz amount);
static inline const U8* buf_get_position(Buf buf);
static inline Nothing buf_read_i16(Buf buf, I16* num);
static inline Nothing buf_read_i32(Buf buf, I32* num);
static inline Nothing buf_read_i64(Buf buf, I64* num);
static inline Nothing buf_read_f32(Buf buf, F32* num);
static inline Nothing buf_read_f64(Buf buf, F64* num);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

mount_slave_profiling_context();

static inline Buf
buf(const U8* base, Sz size) {
  start_profiling(1);

  assert(base != 0);
  assert(size > 0);

  Buf result = {.base = base, .size = size};

  end_profiling();
  return result;
}

static inline Nothing
buf_reset(Buf buf) {
  buf.base = 0;
  buf.offset = 0;
  buf.size = 0;
}

static inline Nothing
buf_set_position(Buf buf, Sz offset) {
  start_profiling(1);

  assert(buf.base != 0);
  assert(buf.size > 0);
  assert(offset <= buf.size);

  buf.offset = offset;

  end_profiling();
}

static inline Nothing
buf_move_offset(Buf buf, Sz amount) {
  start_profiling(1);

  assert(buf.base != 0);
  assert(buf.size > 0);
  assert(buf.size >= buf.offset + amount);

  buf.offset += amount;

  end_profiling();
}

static inline const U8*
buf_get_position(Buf buf) {
  start_profiling(1);

  assert(buf.base != 0);
  assert(buf.size > 0);

  const U8* position = buf.base + buf.offset;

  end_profiling();
  return position;
}

static inline Nothing
buf_read_i16(Buf buf, I16* num) {
  start_profiling(1);

  assert(buf.base != 0);
  assert(buf.size > 0);
  assert(sizeof(I16) + buf.offset <= buf.size);

  memcpy(&num, buf_get_position(buf), sizeof(I16));
  *num = nd_i16(*num);
  buf_move_offset(buf, sizeof(I16));

  end_profiling();
}

static inline Nothing
buf_read_i32(Buf buf, I32* num) {
  start_profiling(1);

  assert(buf.base != 0);
  assert(buf.size > 0);
  assert(sizeof(I32) + buf.offset <= buf.size);

  memcpy(&num, buf_get_position(buf), sizeof(I32));
  *num = nd_i32(*num);
  buf_move_offset(buf, sizeof(I32));

  end_profiling();
}

static inline Nothing
buf_read_i64(Buf buf, I64* num) {
  start_profiling(1);

  assert(buf.base != 0);
  assert(buf.size > 0);
  assert(sizeof(I64) + buf.offset <= buf.size);

  memcpy(&num, buf_get_position(buf), sizeof(I64));
  *num = nd_i64(*num);
  buf_move_offset(buf, sizeof(I64));

  end_profiling();
}

static inline Nothing
buf_read_f32(Buf buf, F32* num) {
  start_profiling(1);

  assert(buf.base != 0);
  assert(buf.size > 0);
  assert(sizeof(F32) + buf.offset <= buf.size);

  memcpy(&num, buf_get_position(buf), sizeof(F32));
  *num = nd_f32(*num);
  buf_move_offset(buf, sizeof(F32));

  end_profiling();
}

static inline Nothing
buf_read_f64(Buf buf, F64* num) {
  start_profiling(1);

  assert(buf.base != 0);
  assert(buf.size > 0);
  assert(sizeof(F64) + buf.offset <= buf.size);

  memcpy(&num, buf_get_position(buf), sizeof(F64));
  *num = nd_f64(*num);
  buf_move_offset(buf, sizeof(F64));

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_BUFFER_H
