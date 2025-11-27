#ifndef SEPI_IO_H
#define SEPI_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
  IO_ERR_UNKNOWN,
  IO_ERR_SUCCESS,
  IO_ERR_FAILED,
  IO_ERR__COUNT,
} IOError;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

IOError io_load_file(Arena*, CStr, NDBuffer*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_IO_IMPLEMENTATION

IOError
io_load_file(Arena* arena, CStr path, NDBuffer* ndb) {
  Assert(arena != 0);
  Assert(path != 0);
  Assert(ndb != 0);

  FILE* f = fopen(path, "rb");
  AssertAlways(f != 0);

  fseek(f, 0, SEEK_END);
  ndb->size = ftell(f) * sizeof(U8);
  rewind(f);

  ndb->base = (CBuf)arena_push(arena, ndb->size, AlignOf(U8), FALSE);
  AssertAlways(ndb->base != 0);

  Sz rsize = fread((void*)ndb->base, 1, ndb->size, f);
  AssertAlways(rsize == ndb->size);

  if (f) {
    fclose(f);
  }

  return IO_ERR_SUCCESS;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_IO_IMPLEMENTATION
#endif  // SEPI_IO_H
