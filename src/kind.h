#ifndef KIND_HEADER_
#define KIND_HEADER_

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "../deps/sepi_types.h"

typedef enum {
  KIND_UNKNOWN = -1,

  KIND_PAK = 0,
  KIND_PK3,
  KIND_BIN,
  KIND_DAT,
  KIND_TPAK,

  KIND_MDL,
  KIND_MD2,
  KIND_MD3,
  KIND_MS2,
  KIND_BSP,
  KIND_ENT,
  KIND_MD5MESH,
  KIND_MD5ANIM,
  KIND_MDANIM,
  KIND_BNVIB,

  KIND_NAV,
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
  KIND_JSON,

} kind;

/* ****************** API ****************** */
kind kind_guess_file(cstr);
kind kind_guess_buffer(cstr);
kind kind_guess_entry(cstr, u32);
/* ****************** API ****************** */

#ifdef KIND_IMPLEMENTATION

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

#define MAXBUFSIZE 8

static kind guess_file_type(cstr buf) {
  if (strncmp(buf, "PACK", 4) == 0) {
    return KIND_PAK;
  }
  if (strncmp(buf, "IDPO", 4) == 0) {
    return KIND_MDL;
  }
  return KIND_UNKNOWN;
}

kind kind_guess_entry(cstr path, u32 len) {
  char ext[32] = {0};
  i32 ridx = len;
  i32 idx = 0;
  i32 end = 0;
  i32 start = 0;
  i32 extlen = 0;

  for (; ridx >= 0; ridx--)
    if (path[ridx] != 0)
      break;
  end = ridx + 1;

  for (; ridx >= 0; ridx--)
    if (path[ridx] == '.')
      break;
  start = ridx + 1;

  extlen = end - start;
  strncpy(ext, path + start, extlen);

  for (i32 i = 0; i < extlen; i++)
    ext[i] = toupper(ext[i]);

  if (!strncmp(ext, "PAK", 3)) {
    return KIND_PAK;
  }

  if (!strncmp(ext, "PA3", 3)) {
    return KIND_PK3;
  }

  if (!strncmp(ext, "BIN", 3)) {
    return KIND_BIN;
  }

  if (!strncmp(ext, "DAT", 3)) {
    return KIND_DAT;
  }

  if (!strncmp(ext, "MDL", 3)) {
    return KIND_MDL;
  }

  if (!strncmp(ext, "MD2", 3)) {
    return KIND_MD2;
  }

  if (!strncmp(ext, "MD3", 3)) {
    return KIND_MD3;
  }

  if (!strncmp(ext, "MS2", 3)) {
    return KIND_MS2;
  }

  if (!strncmp(ext, "BSP", 3)) {
    return KIND_BSP;
  }

  if (!strncmp(ext, "ENT", 3)) {
    return KIND_ENT;
  }

  if (!strncmp(ext, "MD5MESH", 7)) {
    return KIND_MD5MESH;
  }

  if (!strncmp(ext, "MD5ANIM", 7)) {
    return KIND_MD5ANIM;
  }

  if (!strncmp(ext, "MDANIM", 6)) {
    return KIND_MDANIM;
  }

  if (!strncmp(ext, "BNVIB", 5)) {
    return KIND_BNVIB;
  }

  if (!strncmp(ext, "NAV", 3)) {
    return KIND_NAV;
  }

  if (!strncmp(ext, "MAP", 3)) {
    return KIND_MAP;
  }

  if (!strncmp(ext, "RAW", 3)) {
    return KIND_RAW;
  }

  if (!strncmp(ext, "DEM", 3)) {
    return KIND_DEM;
  }

  if (!strncmp(ext, "LIT", 3)) {
    return KIND_LIT;
  }

  if (!strncmp(ext, "LIGHTS", 6)) {
    return KIND_LIGHTS;
  }

  if (!strncmp(ext, "RTLIGHTS", 8)) {
    return KIND_RTLIGHTS;
  }

  if (!strncmp(ext, "WAD", 3)) {
    return KIND_WAD;
  }

  if (!strncmp(ext, "LMP", 3)) {
    return KIND_LMP;
  }

  if (!strncmp(ext, "PCX", 3)) {
    return KIND_PCX;
  }

  if (!strncmp(ext, "JPG", 3)) {
    return KIND_JPG;
  }

  if (!strncmp(ext, "PNG", 3)) {
    return KIND_PNG;
  }

  if (!strncmp(ext, "TGA", 3)) {
    return KIND_TGA;
  }

  if (!strncmp(ext, "SPR", 3)) {
    return KIND_SPR;
  }

  if (!strncmp(ext, "SPR32", 5)) {
    return KIND_SPR32;
  }

  if (!strncmp(ext, "SKIN", 4)) {
    return KIND_SKIN;
  }

  if (!strncmp(ext, "WAV", 3)) {
    return KIND_WAV;
  }

  if (!strncmp(ext, "OGG", 3)) {
    return KIND_OGG;
  }

  if (!strncmp(ext, "RC", 3)) {
    return KIND_RC;
  }

  if (!strncmp(ext, "CFG", 3)) {
    return KIND_CFG;
  }

  if (!strncmp(ext, "TXT", 3)) {
    return KIND_TXT;
  }

  if (!strncmp(ext, "JSON", 3)) {
    return KIND_JSON;
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
