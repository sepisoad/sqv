#ifndef UTILS_IO_HEADER_
#define UTILS_IO_HEADER_

#include <stdint.h>
#include <stdio.h>

#include "types.h"

/* ****************** utils::io API ****************** */
size_t load_file(const char* path, u8** buf);
/* ****************** utils::io API ****************** */

#ifdef UTILS_ENDIAN_IMPLEMENTATION

#include "macros.h"

size_t load_file(const char* path, u8** buf) {
  FILE* f = fopen(path, "rb");
  makesure(f != NULL, "failed to open '%s'", path);

  fseek(f, 0, SEEK_END);
  sz fsize = ftell(f);
  rewind(f);

  *buf = (u8*)malloc(sizeof(u8) * fsize);
  makesure(*buf != NULL, "malloc failed");

  sz rsize = fread(*buf, 1, fsize, f);
  makesure(rsize == fsize, "read size '%zu' did not match the file size '%zu'",
           rsize, fsize);

  if (f) {
    fclose(f);
  }

  return fsize;
}

#endif  // UTILS_ENDIAN_IMPLEMENTATION
#endif  // UTILS_IO_HEADER_
