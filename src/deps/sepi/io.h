#ifndef SEPI_IO_H
#define SEPI_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../tracy/tracy.h"

#include "base.h"
#include "endian.h"
#include "arena.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  IO_ERR_SUCCESS = 1,
  IO_ERR_FAILED,
  IO_ERR__COUNT,
} IOError;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

IOError io_load_file(Arena*, CBuf, NDBuffer*);

#define IO_POS(f) ftell((f))
#define IO_SET(f, ofs) fseek((f), (ofs), SEEK_SET)
#define IO_MOVE(f, sz) fseek((f), (sz), SEEK_CUR)

#define IO_I16(f /* FILE* */, num /* I16* */) \
  {                                           \
    I16 tmp;                                  \
    fread(&tmp, 1, sizeof(I16), (f));         \
    tmp = nd_i16(tmp);                        \
    *(num) = tmp;                             \
  }

#define IO_I32(f /* FILE* */, num /* I32* */) \
  {                                           \
    I32 tmp;                                  \
    fread(&tmp, 1, sizeof(I32), (f));         \
    tmp = nd_i32(tmp);                        \
    *(num) = tmp;                             \
  }

#define IO_I64(f /* FILE* */, num /* I64* */) \
  {                                           \
    I64 tmp;                                  \
    fread(&tmp, 1, sizeof(I64), (f));         \
    tmp = nd_i64(tmp);                        \
    *(num) = tmp;                             \
  }

#define IO_F32(f /* FILE* */, num /* F32* */) \
  {                                           \
    F32 tmp;                                  \
    fread(&tmp, 1, sizeof(F32), (f));         \
    tmp = nd_f32(tmp);                        \
    *(num) = tmp;                             \
  }

#define IO_F64(f /* FILE* */, num /* F64* */) \
  {                                           \
    F64 tmp;                                  \
    fread(&tmp, 1, sizeof(F64), (f));         \
    tmp = nd_f64(tmp);                        \
    *(num) = tmp;                             \
  }

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_IO_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

IOError
io_load_file(Arena* arena, CBuf path, NDBuffer* ndb) {
  TracyCZoneN(trcyctx, "io_load_file", 1);

  Assert(arena != 0);
  Assert(path != 0);
  Assert(ndb != 0);

  FILE* f;
  {
    TracyCZoneN(trcyctx, "io_load_file::fopen", 2);
    f = fopen(path, "rb");
    TracyCZoneEnd(trcyctx);
  }

  AssertAlways(f != 0);

  {
    TracyCZoneN(trcyctx, "io_load_file::fseek", 2);
    TracyCZoneEnd(trcyctx);
  }

  {
    TracyCZoneN(trcyctx, "io_load_file::ftell", 2);
    ndb->size = ftell(f) * sizeof(U8);
    TracyCZoneEnd(trcyctx);
  }

  {
    TracyCZoneN(trcyctx, "io_load_file::rewind", 2);
    rewind(f);
    TracyCZoneEnd(trcyctx);
  }

  ndb->base = (CBuf)arena_push(arena, ndb->size, AlignOf(U8), FALSE);
  AssertAlways(ndb->base != 0);

  Sz rsize;
  {
    TracyCZoneN(trcyctx, "io_load_file::fread", 2);
    rsize = fread((void*)ndb->base, 1, ndb->size, f);
    TracyCZoneEnd(trcyctx);
  }

  AssertAlways(rsize == ndb->size);

  if (f) {
    TracyCZoneN(trcyctx, "io_load_file::fclose", 2);
    fclose(f);
    TracyCZoneEnd(trcyctx);
  }

  TracyCZoneEnd(trcyctx);
  return IO_ERR_SUCCESS;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_IO_IMPLEMENTATION
#endif  // SEPI_IO_H
