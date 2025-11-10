#ifndef SEPI_IO_H
#define SEPI_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "base.h"
#include "arena.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

//

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Sz io_load_file(Arena*, CStr, Buf*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_IO_IMPLEMENTATION

Sz
io_load_file(Arena* arena, CStr path, Buf* buf) {
  Assert(arena != 0);
  Assert(path != 0);
  Assert(buf != 0);

  FILE* f = fopen(path, "rb");
  AssertAlways(f != 0);

  fseek(f, 0, SEEK_END);
  Sz fsize = ftell(f);
  rewind(f);

  Sz size = sizeof(U8) * fsize;
  *buf = (Buf)arena_push(arena, size, AlignOf(U8), FALSE);
  AssertAlways(buf != 0);

  Sz rsize = fread(*buf, 1, fsize, f);
  AssertAlways(rsize == fsize);

  if(f) {
    fclose(f);
  }

  return fsize;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_IO_IMPLEMENTATION
#endif  // SEPI_IO_H
