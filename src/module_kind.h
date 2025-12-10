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

#if defined(PROFILING)
#include "deps/tracy/tracy.h"
extern TracyCZoneCtx trcyctx;
#endif

#include "deps/sepi/base.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define MAXBUFSIZE 8

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  KIND_ERR_SUCCESS = 1,
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
  TracyCZoneN(trcyctx, "guess_file_type", 1);

  Assert(buf != 0);
  Assert(kind != 0);

  if (strncmp(buf, "PACK", 4) == 0) {
    *kind = KIND_PAK;

    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (strncmp(buf, "IDPO", 4) == 0) {
    *kind = KIND_MD1;

    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  TracyCZoneEnd(trcyctx);
  return KIND_ERR_INVALID;
}

KindError
kind_guess_entry(CStr path, U32 len, Kind* kind) {
  TracyCZoneN(trcyctx, "kind_guess_entry", 1);

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
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "PA3", 3)) {
    *kind = KIND_PK3;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "BIN", 3)) {
    *kind = KIND_BIN;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "DAT", 3)) {
    *kind = KIND_DAT;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MDL", 3)) {
    *kind = KIND_MD1;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MD2", 3)) {
    *kind = KIND_MD2;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MD3", 3)) {
    *kind = KIND_MD3;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MS2", 3)) {
    *kind = KIND_MS2;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "SMD", 3)) {
    *kind = KIND_SMD;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "BSP", 3)) {
    *kind = KIND_BSP;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "ENT", 3)) {
    *kind = KIND_ENT;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MD5MESH", 7)) {
    *kind = KIND_MD5MESH;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MD5ANIM", 7)) {
    *kind = KIND_MD5ANIM;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MDANIM", 6)) {
    *kind = KIND_MDANIM;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "BNVIB", 5)) {
    *kind = KIND_BNVIB;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "NAV", 3)) {
    *kind = KIND_NAV;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MAP", 3)) {
    *kind = KIND_MAP;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "RAW", 3)) {
    *kind = KIND_RAW;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "DEM", 3)) {
    *kind = KIND_DEM;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "LIT", 3)) {
    *kind = KIND_LIT;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "LIGHTS", 6)) {
    *kind = KIND_LIGHTS;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "RTLIGHTS", 8)) {
    *kind = KIND_RTLIGHTS;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "WAD", 3)) {
    *kind = KIND_WAD;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "LMP", 3)) {
    *kind = KIND_LMP;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "PCX", 3)) {
    *kind = KIND_PCX;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "JPG", 3)) {
    *kind = KIND_JPG;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "PNG", 3)) {
    *kind = KIND_PNG;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "TGA", 3)) {
    *kind = KIND_TGA;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "SPR", 3)) {
    *kind = KIND_SPR;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "SPR32", 5)) {
    *kind = KIND_SPR32;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "SKIN", 4)) {
    *kind = KIND_SKIN;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "WAV", 3)) {
    *kind = KIND_WAV;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "OGG", 3)) {
    *kind = KIND_OGG;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "MP3", 3)) {
    *kind = KIND_MP3;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "RC", 3)) {
    *kind = KIND_RC;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "CFG", 3)) {
    *kind = KIND_CFG;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "TXT", 3)) {
    *kind = KIND_TXT;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "JSON", 3)) {
    *kind = KIND_JSON;
    TracyCZoneEnd(trcyctx);
    return KIND_ERR_SUCCESS;
  }

  *kind = KIND_UNKNOWN;
  TracyCZoneEnd(trcyctx);
  return KIND_ERR_SUCCESS;
}

KindError
kind_guess_file(CStr path, Kind* kind) {
  TracyCZoneN(trcyctx, "kind_guess_file", 1);

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

  TracyCZoneEnd(trcyctx);
  return guess_file_type(buf, kind);
}

KindError
kind_guess_buffer(CStr data, Kind* kind) {
  TracyCZoneN(trcyctx, "kind_guess_buffer", 1);

  Assert(data != 0);
  Assert(kind != 0);

  char buf[9] = {0};
  Assert(memcpy(buf, data, 8) != 0);

  TracyCZoneEnd(trcyctx);
  return guess_file_type(buf, kind);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_KIND_IMPLEMENTATION
#endif  // MODULE_KIND_HEADER
