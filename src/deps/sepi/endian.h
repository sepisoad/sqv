#ifndef SEPI_ENDIAN_H
#define SEPI_ENDIAN_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdlib.h>
#include "base.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

// N/A

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct {
  CBuf base;
  U32  offset;
} NDBuffer;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

I16 nd_i16(I16 num); /* works on unsigned intergers as well */
I32 nd_i32(I32 num); /* works on unsigned intergers as well */
I64 nd_i64(I64 num); /* works on unsigned intergers as well */
F32 nd_f32(F32 num);
F64 nd_f64(F64 num);

#define ND_ADDR(ndb) ((ndb)->base + (ndb)->offset)
#define ND_MOVE(ndb, sz) ((ndb)->offset += (sz))

#define ND_I16(num /* I16* */, ndb /* NDBuffer* */) \
  {                                                 \
    I16 tmp;                                        \
    memcpy(&tmp, ND_ADDR((ndb)), sizeof(I16));      \
    tmp = nd_i16(tmp);                              \
    *(num) = tmp;                                   \
    ND_MOVE((ndb), sizeof(I16));                    \
  }

#define ND_I32(num /* I32* */, ndb /* NDBuffer* */) \
  {                                                 \
    I32 tmp;                                        \
    memcpy(&tmp, ND_ADDR((ndb)), sizeof(I32));      \
    tmp = nd_i32(tmp);                              \
    *(num) = tmp;                                   \
    ND_MOVE((ndb), sizeof(I32));                    \
  }

#define ND_I64(num /* I64* */, ndb /* NDBuffer* */) \
  {                                                 \
    I64 tmp;                                        \
    memcpy(&tmp, ND_ADDR((ndb)), sizeof(I64));      \
    tmp = nd_i64(tmp);                              \
    *(num) = tmp;                                   \
    ND_MOVE((ndb), sizeof(I64));                    \
  }

#define ND_F32(num /* F32* */, ndb /* NDBuffer* */) \
  {                                                 \
    F32 tmp;                                        \
    memcpy(&tmp, ND_ADDR((ndb)), sizeof(F32));      \
    tmp = nd_f32(tmp);                              \
    *(num) = tmp;                                   \
    ND_MOVE((ndb), sizeof(F32));                    \
  }

#define ND_F64(num /* F64* */, ndb /* NDBuffer* */) \
  {                                                 \
    F64 tmp;                                        \
    memcpy(&tmp, ND_ADDR((ndb)), sizeof(F64));      \
    tmp = nd_f64(tmp);                              \
    *(num) = tmp;                                   \
    ND_MOVE((ndb), sizeof(F64));                    \
  }

#define ND_I16_OLD(num /* I16* */, buf /* CBuf */, ofs /* U32 */) \
  {                                                               \
    I16 tmp;                                                      \
    memcpy(&tmp, buf + ofs, sizeof(I16));                         \
    tmp = nd_i16(tmp);                                            \
    *(num) = tmp;                                                 \
    ofs += sizeof(I16);                                           \
  }

#define ND_I32_OLD(num /* I32* */, buf /* CBuf */, ofs /* U32 */) \
  {                                                               \
    I32 tmp;                                                      \
    memcpy(&tmp, buf + ofs, sizeof(I32));                         \
    tmp = nd_i32(tmp);                                            \
    *(num) = tmp;                                                 \
    ofs += sizeof(I32);                                           \
  }

#define ND_I64_OLD(num /* I64* */, buf /* CBuf */, ofs /* U32 */) \
  {                                                               \
    I64 tmp;                                                      \
    memcpy(&tmp, buf + ofs, sizeof(I64));                         \
    tmp = nd_i64(tmp);                                            \
    *(num) = tmp;                                                 \
    ofs += sizeof(I64);                                           \
  }

#define ND_F32_OLD(num /* F32* */, buf /* CBuf */, ofs /* U32 */) \
  {                                                               \
    F32 tmp;                                                      \
    memcpy(&tmp, buf + ofs, sizeof(F32));                         \
    tmp = nd_f32(tmp);                                            \
    *(num) = tmp;                                                 \
    ofs += sizeof(F32);                                           \
  }

#define ND_F64_OLD(num /* F64* */, buf /* CBuf */, ofs /* U32 */) \
  {                                                               \
    F64 tmp;                                                      \
    memcpy(&tmp, buf + ofs, sizeof(F64));                         \
    tmp = nd_f64(tmp);                                            \
    *(num) = tmp;                                                 \
    ofs += sizeof(F64);                                           \
  }

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_ENDIAN_IMPLEMENTATION

internal inline Bool
isle() {
  U16 num = 0x1;
  return (*(U8*)&num == 1);
}

I16
nd_i16(I16 num) {
  return isle() ? num : (I16)((num >> 8) | (num << 8));
}

I32
nd_i32(I32 num) {
  return isle() ? num
                : (I32)((num >> 24) | ((num >> 8) & 0x0000FF00) |
                        ((num << 8) & 0x00FF0000) | (num << 24));
}

I64
nd_i64(I64 num) {
  return isle() ? num
                : (I64)((num >> 56) | ((num >> 40) & 0x000000000000FF00LL) |
                        ((num >> 24) & 0x0000000000FF0000LL) |
                        ((num >> 8) & 0x00000000FF000000LL) |
                        ((num << 8) & 0x000000FF00000000LL) |
                        ((num << 24) & 0x0000FF0000000000LL) |
                        ((num << 40) & 0x00FF000000000000LL) | (num << 56));
}

F32
nd_f32(F32 num) {
  if (isle()) {
    return num;
  }

  F32 result;
  Str src = (Str)&num;
  Str dst = (Str)&result;
  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
  return result;
}

F64
nd_f64(F64 num) {
  if (isle()) {
    return num;
  }
  F64 result;
  Str src = (Str)&num;
  Str dst = (Str)&result;
  dst[0] = src[7];
  dst[1] = src[6];
  dst[2] = src[5];
  dst[3] = src[4];
  dst[4] = src[3];
  dst[5] = src[2];
  dst[6] = src[1];
  dst[7] = src[0];
  return result;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_ENDIAN_IMPLEMENTATION
#endif  // SEPI_ENDIAN_H
