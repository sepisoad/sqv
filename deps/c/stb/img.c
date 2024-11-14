#include <stdint.h>
#include <stdbool.h>
#include <lua5.4/lua.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "img_decoders.h"
#include "img_encoders.h"
#include "img.h"

typedef struct RGB_t { uint8_t R; uint8_t G; uint8_t B; } RGB_t;

typedef enum ERRCODE {
  ERR_SUCCESS = 0,

  ERR_ENCPLT_PAR1NOSTR,
  ERR_ENCPLT_PAR2NOTBL,
  ERR_ENCPLT_PAR3NOTBL,
  ERR_ENCPLT_PAR4NONUM,
  ERR_ENCPLT_PAR5NONUM,
  ERR_ENCPLT_PALNORGB,
  ERR_ENCPLT_RGBNONUM,
  ERR_ENCPLT_PIXNONUM,
  ERR_ENCPLT_WRPNG,

  ERR_DECPLT_PAR1NOTBL,
  ERR_DECPLT_PAR2NOSTR,
  ERR_DECPLT_LDPNG,
  ERR_DECPLT_INVPNG,
  ERR_DECPLT_INVPLTIDX,
} ERRCODE;

static char* errstr(ERRCODE err) {
  switch (err)
  {
  case ERR_SUCCESS:
    return "";

  case ERR_ENCPLT_PAR1NOSTR:
    return "l_encode_paletted_png: Expected a string (path)";
  case ERR_ENCPLT_PAR2NOTBL:
    return "l_encode_paletted_png: Expected a table (palette)";
  case ERR_ENCPLT_PAR3NOTBL:
    return "l_encode_paletted_png: Expected a table (pixels)";
  case ERR_ENCPLT_PAR4NONUM:
    return "l_encode_paletted_png: Expected a number (width)";
  case ERR_ENCPLT_PAR5NONUM:
    return "l_encode_paletted_png: Expected a number (height)";
  case ERR_ENCPLT_PALNORGB:
    return "l_encode_paletted_png: Palette element is not a RGB value";
  case ERR_ENCPLT_RGBNONUM:
    return "l_encode_paletted_png: Palette RGB value is not a number";
  case ERR_ENCPLT_PIXNONUM:
    return "l_encode_paletted_png: Pixel element is not a number";
  case ERR_ENCPLT_WRPNG:
    return "c_encode_paletted_png: Failed to write png data into file";

  case ERR_DECPLT_PAR1NOTBL:
    return "l_decode_paletted_png: Expected a table (palette)";
  case ERR_DECPLT_PAR2NOSTR:
    return "l_decode_paletted_png: Expected a string (path)";
  case ERR_DECPLT_LDPNG:
    return "c_decode_paletted_png: Failed to load png data from file";
  case ERR_DECPLT_INVPNG:
    return "c_decode_paletted_png: png file data is not valid";
  case ERR_DECPLT_INVPLTIDX:
    return "c_decode_paletted_png: color cannot be indexed";
  default:
    return "FATAL, BUG!";
  }
}

static const uint8_t CHANNELS = 3;

// ========================================================
// C level functions
// ========================================================

static ERRCODE c_encode_paletted_png(
  const char* path,
  const uint8_t* pixels,
  uint32_t width,
  uint32_t height
) {
  if (!stbi_write_png(path, width, height, CHANNELS, pixels, width * 3)) {
    return ERR_ENCPLT_WRPNG;
  }

  return ERR_SUCCESS;
}

// --------------------------------------------------------

static ERRCODE c_decode_paletted_png(
  const char* path,
  const RGB_t* palette,
  int palette_len,
  uint8_t** indices,
  int* indices_len,
  int* width,
  int* height
) {
  ERRCODE err = ERR_SUCCESS;
  uint32_t comp;

  uint8_t* data = stbi_load(path, width, height, &comp, 0);
  if (data == NULL) {
    err = ERR_DECPLT_LDPNG;
    goto cleanup;
  }

  if ((*width) <= 0 || (*height) <= 0 || comp != CHANNELS) {
    err = ERR_DECPLT_INVPNG;
    goto cleanup;
  }

  *indices_len = (*width) * (*height);
  *indices = malloc(sizeof(uint8_t) * (*indices_len));

  int index = 0, indices_idx = 0;
  for (index = 0, indices_idx = 0; index < (*width) * (*height) * CHANNELS; index += 3, indices_idx++) {
    uint8_t r = data[index];
    uint8_t g = data[index + 1];
    uint8_t b = data[index + 2];
    int match_idx = -1;
    for (int idx = 0; idx < palette_len; idx++) {
      if (palette[idx].R == r && palette[idx].G == g && palette[idx].B == b) {
        match_idx = idx;
        break;
      }
    }
    if (match_idx == -1) {
      err = ERR_DECPLT_INVPLTIDX;
      goto cleanup;
    }

    (*indices)[indices_idx] = match_idx;
  }

cleanup:
  if (data) free(data);

  return err;
}

// ========================================================
// Lua level functions
// ========================================================

static int l_encode_paletted_png(lua_State* L) {
  ERRCODE err = ERR_SUCCESS;
  RGB_t* palette = NULL;
  uint8_t* pixels = NULL;

  // validate parameters type
  if (!lua_isstring(L, 1)) { err = ERR_ENCPLT_PAR1NOSTR; goto cleanup; }
  if (!lua_istable(L, 2)) { err = ERR_ENCPLT_PAR2NOTBL; goto cleanup; }
  if (!lua_istable(L, 3)) { err = ERR_ENCPLT_PAR3NOTBL; goto cleanup; }
  if (!lua_isinteger(L, 4)) { err = ERR_ENCPLT_PAR4NONUM; goto cleanup; }
  if (!lua_isinteger(L, 5)) { err = ERR_ENCPLT_PAR5NONUM; goto cleanup; }

  const char* path = luaL_checkstring(L, 1);
  int palette_len = luaL_len(L, 2);
  int pixels_len = luaL_len(L, 3);
  int width = luaL_checkinteger(L, 4);
  int height = luaL_checkinteger(L, 5);

  // read palette values into memory
  palette = malloc(sizeof(RGB_t) * palette_len);
  for (int palette_index = 0; palette_index < palette_len; palette_index++) {
    lua_rawgeti(L, 2, palette_index + 1);
    if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      err = ERR_ENCPLT_PALNORGB;
      goto cleanup;
    }

    lua_getfield(L, -1, "Red");
    lua_getfield(L, -2, "Green");
    lua_getfield(L, -3, "Blue");

    if (!lua_isnumber(L, -3) || !lua_isnumber(L, -2) || !lua_isnumber(L, -1)) {
      lua_pop(L, 4);
      err = ERR_ENCPLT_RGBNONUM;
      goto cleanup;
    }

    palette[palette_index].R = luaL_checkinteger(L, -3);
    palette[palette_index].G = luaL_checkinteger(L, -2);
    palette[palette_index].B = luaL_checkinteger(L, -1);
    lua_pop(L, 4);
  }

  // read pixel values into memory
  pixels = malloc(sizeof(uint8_t) * CHANNELS * pixels_len);
  for (int pixel_index = 0; pixel_index < pixels_len; pixel_index++) {
    lua_rawgeti(L, 3, pixel_index + 1);

    if (!lua_isnumber(L, -1)) {
      lua_pop(L, 1);
      err = ERR_ENCPLT_PIXNONUM;
      goto cleanup;
    }

    uint8_t palette_index = luaL_checkinteger(L, -1); // from lua 1 index to c 0 index
    int img_index = pixel_index * 3;
    pixels[img_index] = palette[palette_index].R;
    pixels[img_index + 1] = palette[palette_index].G;
    pixels[img_index + 2] = palette[palette_index].B;

    lua_pop(L, 1);
  }

  // do the actual processing:
  err = c_encode_paletted_png(path, pixels, width, height);
  if (err > 0) { goto cleanup; }

cleanup:
  if (palette) free(palette);
  if (pixels) free(pixels);
  if (err > 0) return luaL_error(L, errstr(err));

  return 0;
}

// --------------------------------------------------------

static int l_decode_paletted_png(lua_State* L) {
  ERRCODE err = ERR_SUCCESS;

  if (!lua_istable(L, 1)) { err = ERR_DECPLT_PAR1NOTBL; goto cleanup; }
  if (!lua_isstring(L, 2)) { err = ERR_DECPLT_PAR2NOSTR; goto cleanup; }

  // read palette values into memory
  int palette_len = luaL_len(L, 1);
  RGB_t* palette = malloc(sizeof(RGB_t) * palette_len);
  for (int palette_index = 0; palette_index < palette_len; palette_index++) {
    lua_rawgeti(L, 1, palette_index + 1);
    if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      err = ERR_ENCPLT_PALNORGB;
      goto cleanup;
    }

    lua_getfield(L, -1, "Red");
    lua_getfield(L, -2, "Green");
    lua_getfield(L, -3, "Blue");

    if (!lua_isnumber(L, -3) || !lua_isnumber(L, -2) || !lua_isnumber(L, -1)) {
      lua_pop(L, 4);
      err = ERR_ENCPLT_RGBNONUM;
      goto cleanup;
    }

    palette[palette_index].R = luaL_checkinteger(L, -3);
    palette[palette_index].G = luaL_checkinteger(L, -2);
    palette[palette_index].B = luaL_checkinteger(L, -1);
    lua_pop(L, 4);
  }

  const char* path = luaL_checkstring(L, 2);

  uint8_t* data = NULL;
  int data_len, width, height = 0;
  err = c_decode_paletted_png(path, palette, palette_len, &data, &data_len, &width, &height);
  if (err > 0) { goto cleanup; }

  lua_pushlstring(L, data, data_len);
  lua_pushinteger(L, width);
  lua_pushinteger(L, height);

cleanup:
  if (palette) free(palette);
  if (data) free(data);
  if (err > 0) return luaL_error(L, errstr(err));

  return 3;
}

// ========================================================
// Module
// ========================================================

static const struct luaL_Reg module[] = {
  { "encode_paletted_png", l_encode_paletted_png },
  { "decode_paletted_png", l_decode_paletted_png },
  { NULL, NULL },
};

static int define_module(lua_State* L) {
  luaL_newlib(L, module);
  return 1;
}

static int register_foo(lua_State* L) {
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  lua_pushcfunction(L, define_module);
  lua_setfield(L, -2, "stb");
  lua_pop(L, 2);
}

MODULE_EXPORT int open_module_stb(lua_State* L) {
  int res = define_module(L);
  if (res == 1) register_foo(L);
  return res;
}