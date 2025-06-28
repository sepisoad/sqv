#ifndef SEPI_IO_HEADER_
#define SEPI_IO_HEADER_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sepi_macros.h" // IWYU pragma: keep
#include "sepi_types.h"

/* ****************** utils::io API ****************** */
size_t sepi_io_load_file(cstr, u8 **);
/* ****************** utils::io API ****************** */

#ifdef SEPI_IO_IMPLEMENTATION

// .--------------------------------------------------------------------------.
// | _                 _                           _        _   _             |
// |(_)               | |                         | |      | | (_)            |
// | _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __  |
// || | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \ |
// || | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | ||
// ||_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_||
// |            | |                                                           |
// |            |_|                                                           |
// '--------------------------------------------------------------------------'

size_t sepi_io_load_file(cstr path, u8 **buf) {
  FILE *f = fopen(path, "rb");
  notnull(f);

  fseek(f, 0, SEEK_END);
  sz fsize = ftell(f);
  rewind(f);

  *buf = (u8 *)malloc(sizeof(u8) * fsize);
  notnull(*buf);

  sz rsize = fread(*buf, 1, fsize, f);
  makesure(rsize == fsize, "read size '%zu' did not match the file size '%zu'",
           rsize, fsize);

  if (f) {
    fclose(f);
  }

  return fsize;
}

#endif // SEPI_IO_IMPLEMENTATION
#endif // SEPI_IO_HEADER_
