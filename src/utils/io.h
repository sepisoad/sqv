#ifndef UTILS_IO_HEADER_
#define UTILS_IO_HEADER_

#include <stdint.h>
#include <stdio.h>

#include "types.h"

/* ****************** utils::io API ****************** */
size_t ut_load_file(const char* path, u8** buf);
/* ****************** utils::io API ****************** */

#ifdef UTILS_IO_IMPLEMENTATION

#include "macros.h"

size_t ut_load_file(const char* path, u8** buf) {
  FILE* f = fopen(path, "rb");
  notnull(f);

  fseek(f, 0, SEEK_END);
  sz fsize = ftell(f);
  rewind(f);

  *buf = (u8*)malloc(sizeof(u8) * fsize);
  notnull(*buf);

  sz rsize = fread(*buf, 1, fsize, f);
  makesure(rsize == fsize, "read size '%zu' did not match the file size '%zu'",
           rsize, fsize);

  if (f) {
    fclose(f);
  }

  return fsize;
}

#endif  // UTILS_IO_IMPLEMENTATION
#endif  // UTILS_IO_HEADER_
