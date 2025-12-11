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
    fseek(f, 0, SEEK_END);
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
