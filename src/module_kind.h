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

fn KindError kind_guess_entry(Str, Kind*);
fn KindError kind_guess_file(CZStr, Kind*);
fn KindError kind_guess_buffer(CZStr, Kind*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_KIND_IMPLEMENTATION

mount_slave_profiling_context();

local fn KindError
guess_file_type(CZStr buf, Kind* kind) {
  start_profiling(1);

  assert(buf != 0);
  assert(kind != 0);

  KindError err = KIND_ERR_SUCCESS;

  if (strncmp(buf, "PACK", 4) == 0) {
    *kind = KIND_PAK;
    goto cleanup;
  }

  if (strncmp(buf, "IDPO", 4) == 0) {
    *kind = KIND_MD1;
    goto cleanup;
  }

cleanup:
  end_profiling();
  return err;
}

fn KindError
// cppcheck-suppress unusedFunction
kind_guess_entry(Str path, Kind* kind) {
  start_profiling(1);

  assert(ZS(path) != 0);
  assert(path.length > 0);
  assert(kind != 0);

  KindError err = KIND_ERR_SUCCESS;
  StrCmpFlags compare = STR_CMP_CASE_INSENSITIVE;
  *kind = KIND_UNKNOWN;

  I64 dot_index = str_find_last(path, '.') - 1;
  if (dot_index < 0) {
    err = KIND_ERR_INVALID;
    goto cleanup;
  }

  Str ext = S(ZS(path) + (dot_index + 1));

  if (!str_equal(ext, S("pak"), compare)) {
    *kind = KIND_PAK;
    goto cleanup;
  }

  if (!str_equal(ext, S("pa3"), compare)) {
    *kind = KIND_PK3;
    goto cleanup;
  }

  if (!str_equal(ext, S("bin"), compare)) {
    *kind = KIND_BIN;
    goto cleanup;
  }

  if (!str_equal(ext, S("dat"), compare)) {
    *kind = KIND_DAT;
    goto cleanup;
  }

  if (!str_equal(ext, S("mdl"), compare)) {
    *kind = KIND_MD1;
    goto cleanup;
  }

  if (!str_equal(ext, S("md2"), compare)) {
    *kind = KIND_MD2;
    goto cleanup;
  }

  if (!str_equal(ext, S("md3"), compare)) {
    *kind = KIND_MD3;
    goto cleanup;
  }

  if (!str_equal(ext, S("ms2"), compare)) {
    *kind = KIND_MS2;
    goto cleanup;
  }

  if (!str_equal(ext, S("smd"), compare)) {
    *kind = KIND_SMD;
    goto cleanup;
  }

  if (!str_equal(ext, S("bsp"), compare)) {
    *kind = KIND_BSP;
    goto cleanup;
  }

  if (!str_equal(ext, S("ent"), compare)) {
    *kind = KIND_ENT;
    goto cleanup;
  }

  if (!str_equal(ext, S("md5mesh"), compare)) {
    *kind = KIND_MD5MESH;
    goto cleanup;
  }

  if (!str_equal(ext, S("md5anim"), compare)) {
    *kind = KIND_MD5ANIM;
    goto cleanup;
  }

  if (!str_equal(ext, S("mdanim"), compare)) {
    *kind = KIND_MDANIM;
    goto cleanup;
  }

  if (!str_equal(ext, S("bnvib"), compare)) {
    *kind = KIND_BNVIB;
    goto cleanup;
  }

  if (!str_equal(ext, S("nav"), compare)) {
    *kind = KIND_NAV;
    goto cleanup;
  }

  if (!str_equal(ext, S("map"), compare)) {
    *kind = KIND_MAP;
    goto cleanup;
  }

  if (!str_equal(ext, S("raw"), compare)) {
    *kind = KIND_RAW;
    goto cleanup;
  }

  if (!str_equal(ext, S("dem"), compare)) {
    *kind = KIND_DEM;
    goto cleanup;
  }

  if (!str_equal(ext, S("lit"), compare)) {
    *kind = KIND_LIT;
    goto cleanup;
  }

  if (!str_equal(ext, S("lighs"), compare)) {
    *kind = KIND_LIGHTS;
    goto cleanup;
  }

  if (!str_equal(ext, S("rtlights"), compare)) {
    *kind = KIND_RTLIGHTS;
    goto cleanup;
  }

  if (!str_equal(ext, S("wad"), compare)) {
    *kind = KIND_WAD;
    goto cleanup;
  }

  if (!str_equal(ext, S("lmp"), compare)) {
    *kind = KIND_LMP;
    goto cleanup;
  }

  if (!str_equal(ext, S("pcx"), compare)) {
    *kind = KIND_PCX;
    goto cleanup;
  }

  if (!str_equal(ext, S("jpg"), compare)) {
    *kind = KIND_JPG;
    goto cleanup;
  }

  if (!str_equal(ext, S("png"), compare)) {
    *kind = KIND_PNG;
    goto cleanup;
  }

  if (!str_equal(ext, S("tga"), compare)) {
    *kind = KIND_TGA;
    goto cleanup;
  }

  if (!str_equal(ext, S("spr"), compare)) {
    *kind = KIND_SPR;
    goto cleanup;
  }

  if (!str_equal(ext, S("spr32"), compare)) {
    *kind = KIND_SPR32;
    goto cleanup;
  }

  if (!str_equal(ext, S("skin"), compare)) {
    *kind = KIND_SKIN;
    goto cleanup;
  }

  if (!str_equal(ext, S("wav"), compare)) {
    *kind = KIND_WAV;
    goto cleanup;
  }

  if (!str_equal(ext, S("ogg"), compare)) {
    *kind = KIND_OGG;
    goto cleanup;
  }

  if (!str_equal(ext, S("mp3"), compare)) {
    *kind = KIND_MP3;
    goto cleanup;
  }

  if (!str_equal(ext, S("rc"), compare)) {
    *kind = KIND_RC;
    goto cleanup;
  }

  if (!str_equal(ext, S("cfg"), compare)) {
    *kind = KIND_CFG;
    goto cleanup;
  }

  if (!str_equal(ext, S("txt"), compare)) {
    *kind = KIND_TXT;
    goto cleanup;
  }

  if (!str_equal(ext, S("json"), compare)) {
    *kind = KIND_JSON;
    goto cleanup;
  }

cleanup:
  end_profiling();
  return err;
}

fn KindError
// cppcheck-suppress unusedFunction
kind_guess_file(CZStr path, Kind* kind) {
  start_profiling(1);

  assert(path != 0);
  assert(kind != 0);

  KindError err = KIND_ERR_SUCCESS;

  char buf[MAXBUFSIZE + 1] = {0};

  FILE* f = fopen(path, "rb");

  runtime_assert(f != 0);
  assert(fseek(f, 0, SEEK_END) == 0);

  rewind(f);

  Sz sz = fread(buf, sizeof(buf[0]), MAXBUFSIZE, f);
  assert(sz == MAXBUFSIZE);

  fclose(f);

  err = guess_file_type(buf, kind);

  end_profiling();
  return err;
}

fn KindError
// cppcheck-suppress unusedFunction
kind_guess_buffer(CZStr data, Kind* kind) {
  start_profiling(1);

  assert(data != 0);
  assert(kind != 0);

  KindError err = KIND_ERR_SUCCESS;

  char buf[9] = {0};
  assert(memcpy(buf, data, 8) != 0);

  err = guess_file_type(buf, kind);

  end_profiling();
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_KIND_IMPLEMENTATION
#endif  // MODULE_KIND_HEADER
