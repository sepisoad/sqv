/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#ifndef MODULE_KIND_HEADER
#define MODULE_KIND_HEADER

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "deps/sepi/base.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define MAXBUFSIZE 8

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  KIND_ERR_UNKNOWN,
  KIND_ERR_SUCCESS,
  KIND_ERR_INVALID,
  KIND_ERR__COUNT,
} KindError;

typedef enum {
  KIND_UNKNOWN = -1,

  KIND_PAK = 0,
  KIND_PK3,
  KIND_BIN,
  KIND_DAT,
  KIND_TPAK,

  KIND_MD1,
  KIND_MD2,
  KIND_MD3,
  KIND_MS2,
  KIND_SMD,
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
  KIND_MP3,

  KIND_RC,
  KIND_CFG,
  KIND_TXT,
  KIND_JSON,

} Kind;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

KindError kind_guess_file(CStr, Kind*);
KindError kind_guess_buffer(CStr, Kind*);
KindError kind_guess_entry(CStr, U32, Kind*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_KIND_IMPLEMENTATION

internal KindError
guess_file_type(CStr buf, Kind* kind) {
  Dbg("guess_file_type() ...");

  Assert(buf != 0);
  Assert(kind != 0);

  if (strncmp(buf, "PACK", 4) == 0) {
    *kind = KIND_PAK;
    ;
    return KIND_ERR_SUCCESS;
  }

  if (strncmp(buf, "IDPO", 4) == 0) {
    *kind = KIND_MD1;
    ;
    return KIND_ERR_SUCCESS;
  }

  return KIND_ERR_INVALID;
}

KindError
kind_guess_entry(CStr path, U32 len, Kind* kind) {
  Dbg("kind_guess_entry() ...");

  Assert(path != 0);
  Assert(len > 0);
  Assert(kind != 0);

  char ext[32] = {0};
  I32 ridx = len;
  I32 end = 0;
  I32 start = 0;
  I32 extlen = 0;

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

  for (I32 i = 0; i < extlen; i++)
    ext[i] = toupper(ext[i]);

  if (!strncmp(ext, "PAK", 3)) {
    *kind = KIND_PAK;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "PA3", 3)) {
    *kind = KIND_PK3;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "BIN", 3)) {
    *kind = KIND_BIN;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "DAT", 3)) {
    *kind = KIND_DAT;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MDL", 3)) {
    *kind = KIND_MD1;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MD2", 3)) {
    *kind = KIND_MD2;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MD3", 3)) {
    *kind = KIND_MD3;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MS2", 3)) {
    *kind = KIND_MS2;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "SMD", 3)) {
    *kind = KIND_SMD;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "BSP", 3)) {
    *kind = KIND_BSP;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "ENT", 3)) {
    *kind = KIND_ENT;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MD5MESH", 7)) {
    *kind = KIND_MD5MESH;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MD5ANIM", 7)) {
    *kind = KIND_MD5ANIM;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MDANIM", 6)) {
    *kind = KIND_MDANIM;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "BNVIB", 5)) {
    *kind = KIND_BNVIB;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "NAV", 3)) {
    *kind = KIND_NAV;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MAP", 3)) {
    *kind = KIND_MAP;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "RAW", 3)) {
    *kind = KIND_RAW;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "DEM", 3)) {
    *kind = KIND_DEM;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "LIT", 3)) {
    *kind = KIND_LIT;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "LIGHTS", 6)) {
    *kind = KIND_LIGHTS;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "RTLIGHTS", 8)) {
    *kind = KIND_RTLIGHTS;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "WAD", 3)) {
    *kind = KIND_WAD;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "LMP", 3)) {
    *kind = KIND_LMP;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "PCX", 3)) {
    *kind = KIND_PCX;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "JPG", 3)) {
    *kind = KIND_JPG;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "PNG", 3)) {
    *kind = KIND_PNG;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "TGA", 3)) {
    *kind = KIND_TGA;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "SPR", 3)) {
    *kind = KIND_SPR;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "SPR32", 5)) {
    *kind = KIND_SPR32;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "SKIN", 4)) {
    *kind = KIND_SKIN;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "WAV", 3)) {
    *kind = KIND_WAV;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "OGG", 3)) {
    *kind = KIND_OGG;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MP3", 3)) {
    *kind = KIND_MP3;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "RC", 3)) {
    *kind = KIND_RC;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "CFG", 3)) {
    *kind = KIND_CFG;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "TXT", 3)) {
    *kind = KIND_TXT;
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "JSON", 3)) {
    *kind = KIND_JSON;
    return KIND_ERR_SUCCESS;
  }

  *kind = KIND_UNKNOWN;
  return KIND_ERR_SUCCESS;
}

KindError
kind_guess_file(CStr path, Kind* kind) {
  Dbg("kind_guess_file() ...");

  Assert(path != 0);
  Assert(kind != 0);

  char buf[MAXBUFSIZE + 1] = {0};

  FILE* f = fopen(path, "rb");

  Assert(f != 0);
  Assert(fseek(f, 0, SEEK_END) == 0);

  rewind(f);

  Sz sz = fread(buf, sizeof(buf[0]), MAXBUFSIZE, f);
  Assert(sz == MAXBUFSIZE);

  fclose(f);

  return guess_file_type(buf, kind);
}

KindError
kind_guess_buffer(CStr data, Kind* kind) {
  Dbg("Kind_Guess_Buffer() ...");

  Assert(data != 0);
  Assert(kind != 0);

  char buf[9] = {0};
  Assert(memcpy(buf, data, 8) != 0);

  return guess_file_type(buf, kind);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_KIND_IMPLEMENTATION
#endif  // MODULE_KIND_HEADER
