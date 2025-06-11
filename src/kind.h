#ifndef KIND_HEADER_
#define KIND_HEADER_

#include <stdio.h>

#include "../deps/sepi_types.h"
#include "../deps/sepi_macros.h"

typedef enum {
  KIND_UNKNOWN = -1,

  KIND_PAK = 0,
  KIND_PK3,
  KIND_TPAK,

  KIND_MD1,
  KIND_MD2,
  KIND_MD3,
  KIND_MS2,
  KIND_BSP,
  KIND_ENT,

  KIND_MAP,
  KIND_RAW,
  KIND_DEM,
  KIND_LIT,
  KIND_LIGHTS,
  KIND_RTLIGHTS,

  KIND_WAD,
  KIND_LMP,
  KIND_PCX,
  KIND_JPG,
  KIND_PNG,
  KIND_TGA,
  KIND_SPR,
  KIND_SPR32,
  KIND_SKIN,

  KIND_WAV,
  KIND_OGG,

  KIND_RC,
  KIND_CFG,
  KIND_TXT,

} kind;

/* ****************** API ****************** */
kind kind_guess_file(cstr);
kind kind_guess_buffer(cstr);
/* ****************** API ****************** */

#ifdef KIND_IMPLEMENTATION

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

static constexpr u32 MAXBUFSIZE = 8;

static kind guess_file_type(cstr buf) {
  if (strncmp(buf, "PACK", 4) == 0) {
    return KIND_PAK;
  }
  if (strncmp(buf, "IDPO", 4) == 0) {
    return KIND_MD1;
  }
  return KIND_UNKNOWN;
}

kind kind_guess_file(cstr path) {
  char buf[MAXBUFSIZE + 1] = {0};

  FILE* f = fopen(path, "rb");
  notnull(f);
  notzero(!fseek(f, 0, SEEK_END));
  isvalid(notzero(ftell(f)) >= MAXBUFSIZE);
  rewind(f);
  isvalid(fread(buf, sizeof(char), MAXBUFSIZE, f) == MAXBUFSIZE);

  fclose(f);

  return guess_file_type(buf);
}

kind kind_guess_buffer(cstr data) {
  char buf[9] = {0};

  notzero(!memcpy(buf, data, 8));

  return guess_file_type(buf);
}

#endif  // KIND_IMPLEMENTATION
#endif  // KIND_HEADER_
