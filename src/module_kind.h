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

#include "deps/tracy/tracy.h"
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
KindError kind_guess_entry(Str, U32, Kind*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_KIND_IMPLEMENTATION

SLAVE_PROFILING_CONTEXT;

static KindError
guess_file_type(CStr buf, Kind* kind) {
  START_PROFILING(1);

  // TODO:
  // use goto here!

  Assert(buf != 0);
  Assert(kind != 0);

  if (strncmp(buf, "PACK", 4) == 0) {
    *kind = KIND_PAK;

    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (strncmp(buf, "IDPO", 4) == 0) {
    *kind = KIND_MD1;

    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  END_PROFILING();
  return KIND_ERR_INVALID;
}

KindError
kind_guess_entry(Str path, U32 len, Kind* kind) {
  START_PROFILING(1);

  Assert(path != 0);
  Assert(len > 0);
  Assert(kind != 0);

  Str ext_base = path;
  Str ext = path;
  for (; *ext_base != 0; ext_base++)
    if ('.' == *ext_base)
      ext = ext_base;

  ext++;
  U32 extlen = ext_base - ext;

  for (I32 i = 0; i < extlen; i++)
    ext[i] = tolower(ext[i]);

  if (!strncmp(ext, "pak", 3)) {
    *kind = KIND_PAK;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "pa3", 3)) {
    *kind = KIND_PK3;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "bin", 3)) {
    *kind = KIND_BIN;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "dat", 3)) {
    *kind = KIND_DAT;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "mdl", 3)) {
    *kind = KIND_MD1;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "md2", 3)) {
    *kind = KIND_MD2;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "md3", 3)) {
    *kind = KIND_MD3;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "ms2", 3)) {
    *kind = KIND_MS2;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "smd", 3)) {
    *kind = KIND_SMD;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "bsp", 3)) {
    *kind = KIND_BSP;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "ent", 3)) {
    *kind = KIND_ENT;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "md5mesh", 7)) {
    *kind = KIND_MD5MESH;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "md5anim", 7)) {
    *kind = KIND_MD5ANIM;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "mdanim", 6)) {
    *kind = KIND_MDANIM;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "bnvib", 5)) {
    *kind = KIND_BNVIB;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "nav", 3)) {
    *kind = KIND_NAV;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "map", 3)) {
    *kind = KIND_MAP;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "raw", 3)) {
    *kind = KIND_RAW;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "dem", 3)) {
    *kind = KIND_DEM;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "lit", 3)) {
    *kind = KIND_LIT;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "lights", 6)) {
    *kind = KIND_LIGHTS;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "rtlights", 8)) {
    *kind = KIND_RTLIGHTS;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "wad", 3)) {
    *kind = KIND_WAD;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "lmp", 3)) {
    *kind = KIND_LMP;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "pcx", 3)) {
    *kind = KIND_PCX;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "jpg", 3)) {
    *kind = KIND_JPG;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "png", 3)) {
    *kind = KIND_PNG;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "tga", 3)) {
    *kind = KIND_TGA;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "spr", 3)) {
    *kind = KIND_SPR;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "spr32", 5)) {
    *kind = KIND_SPR32;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "skin", 4)) {
    *kind = KIND_SKIN;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "wav", 3)) {
    *kind = KIND_WAV;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "ogg", 3)) {
    *kind = KIND_OGG;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "mp3", 3)) {
    *kind = KIND_MP3;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "rc", 3)) {
    *kind = KIND_RC;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "cfg", 3)) {
    *kind = KIND_CFG;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "txt", 3)) {
    *kind = KIND_TXT;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  if (!strncmp(ext, "json", 3)) {
    *kind = KIND_JSON;
    END_PROFILING();
    return KIND_ERR_SUCCESS;
  }

  *kind = KIND_UNKNOWN;
  END_PROFILING();
  return KIND_ERR_SUCCESS;
}

KindError
kind_guess_file(CStr path, Kind* kind) {
  START_PROFILING(1);

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

  END_PROFILING();
  return guess_file_type(buf, kind);
}

KindError
kind_guess_buffer(CStr data, Kind* kind) {
  START_PROFILING(1);

  Assert(data != 0);
  Assert(kind != 0);

  char buf[9] = {0};
  Assert(memcpy(buf, data, 8) != 0);

  END_PROFILING();
  return guess_file_type(buf, kind);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_KIND_IMPLEMENTATION
#endif  // MODULE_KIND_HEADER
