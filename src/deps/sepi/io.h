#ifndef SEPI_IO_H
#define SEPI_IO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "base.h"

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

size_t sepi_io_load_file(cstr, u8**);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_IO_IMPLEMENTATION

size_t sepi_io_load_file(cstr path, u8** buf) {
  FILE* f = fopen(path, "rb");
  NOTNULL(f);

  fseek(f, 0, SEEK_END);
  sz fsize = ftell(f);
  rewind(f);

  *buf = (u8*)malloc(sizeof(u8) * fsize);
  NOTNULL(*buf);

  sz rsize = fread(*buf, 1, fsize, f);
  MAKESURE(rsize == fsize, "read size '%zu' did not match the file size '%zu'",
           rsize, fsize);

  if (f) {
    fclose(f);
  }

  return fsize;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_IO_IMPLEMENTATION
#endif  // SEPI_IO_H
