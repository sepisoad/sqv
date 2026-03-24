#ifndef SEPI_ENDIAN_H
#define SEPI_ENDIAN_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdlib.h>

#include <sepi/base.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

static inline Bool isle();
static inline I16 nd_i16(I16 num);
static inline I32 nd_i32(I32 num);
static inline I64 nd_i64(I64 num);
static inline F32 nd_f32(F32 num);
static inline F64 nd_f64(F64 num);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

// NOTE:
// since this module implements all the functions as `static inline` we do not
// need an implementation guard here anyway

/* ----------------------------------------------------- */

static inline Bool
isle() {
  U16 num = 0x1;
  return (*(U8*)&num == 1);
}

/* ----------------------------------------------------- */

static inline I16
nd_i16(I16 num) {
  return isle() ? num : (I16)((num >> 8) | (num << 8));
}

/* ----------------------------------------------------- */

static inline I32
nd_i32(I32 num) {
  return isle() ? num
                : (I32)((num >> 24) | ((num >> 8) & 0x0000FF00) |
                        ((num << 8) & 0x00FF0000) | (num << 24));
}

/* ----------------------------------------------------- */

static inline I64
nd_i64(I64 num) {
  return isle() ? num
                : (I64)((num >> 56) | ((num >> 40) & 0x000000000000FF00LL) |
                        ((num >> 24) & 0x0000000000FF0000LL) |
                        ((num >> 8) & 0x00000000FF000000LL) |
                        ((num << 8) & 0x000000FF00000000LL) |
                        ((num << 24) & 0x0000FF0000000000LL) |
                        ((num << 40) & 0x00FF000000000000LL) | (num << 56));
}

/* ----------------------------------------------------- */

static inline F32
nd_f32(F32 num) {
  if (isle()) {
    return num;
  }

  F32 result;
  ZStr src = (ZStr)&num;
  ZStr dst = (ZStr)&result;
  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
  return result;
}

/* ----------------------------------------------------- */

static inline F64
nd_f64(F64 num) {
  if (isle()) {
    return num;
  }
  F64 result;
  ZStr src = (ZStr)&num;
  ZStr dst = (ZStr)&result;
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

#endif  // SEPI_ENDIAN_H
