#ifndef SEPI_IO_H
#define SEPI_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "base.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

//

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Sz io_load_file(CStr, Buf*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_IO_IMPLEMENTATION

Sz
io_load_file(CStr path, Buf* buf) {
  FILE* f = fopen(path, "rb");
  NotNull(f);

  fseek(f, 0, SEEK_END);
  Sz fsize = ftell(f);
  rewind(f);

  *buf = (Buf)malloc(sizeof(U8) * fsize);
  NotNull(*buf);

  Sz rsize = fread(*buf, 1, fsize, f);
  MakeSure(rsize == fsize, "read size '%zu' did not match the file size '%zu'",
           rsize, fsize);

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
