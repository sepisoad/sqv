#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "err.h"
#include "qk_mdl.h"

/*
 * external variables
 */
extern const uint8_t _qk_palette[256][3];
// SEPI: do i really need '_qk_colormap' and '_qk_palette_full_bright' or are
// they just useful for rendering textures inside the game?
extern const uint8_t _qk_colormap[256 * 64];
extern uint8_t _qk_palette_full_bright[256 / 32];

/*
 * _qk_is_little_endian
 *
 * decides whether we are on a little endian system
 */
static int _qk_is_little_endian() {
  uint16_t num = 0x1;
  return (*(uint8_t *)&num == 1);
}

/*
 * _qk_to_le16
 *
 * handles the 16 bits integer endianness
 */
static int16_t _qk_to_le16(int16_t num) {
  if (_qk_is_little_endian()) {
    return num;
  } else {
    return (int16_t)((num >> 8) | (num << 8));
  }
}

/*
 * _qk_to_le32
 *
 * handles the 32 bits integer endianness
 */
static int32_t _qk_to_le32(int32_t num) {
  if (_qk_is_little_endian()) {
    return num;
  } else {
    return (int32_t)((num >> 24) | ((num >> 8) & 0x0000FF00) |
                     ((num << 8) & 0x00FF0000) | (num << 24));
  }
}

/*
 * _qk_to_le64
 *
 * handles the 64 bits integer endianness
 */
static int64_t _qk_to_le64(int64_t num) {
  if (_qk_is_little_endian()) {
    return num;
  } else {
    return (int64_t)((num >> 56) | ((num >> 40) & 0x000000000000FF00LL) |
                     ((num >> 24) & 0x0000000000FF0000LL) |
                     ((num >> 8) & 0x00000000FF000000LL) |
                     ((num << 8) & 0x000000FF00000000LL) |
                     ((num << 24) & 0x0000FF0000000000LL) |
                     ((num << 40) & 0x00FF000000000000LL) | (num << 56));
  }
}

/*
 * _qk_to_lefloat
 *
 * handles the float endianness
 */
static float _qk_to_lefloat(float num) {
  if (_qk_is_little_endian()) {
    return num;
  } else {
    float result;
    char *src = (char *)&num;
    char *dst = (char *)&result;
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
    return result;
  }
}

/*
 * _qk_is_full_bright
 *
 * SEPI: add comment!
 */

static bool _qk_is_full_bright(uint8_t *px, size_t sz) {
  for (size_t i = 0; i < sz; i++) {
    if ((_qk_palette_full_bright[(*px) / 32u] & (1u << ((*px) % 32u))) != 0u) {
      px++;
      return true;
    }
  }
  px++;
  return false;
}

static sqv_err _qk_load_image(uint8_t *px, uint32_t w, uint32_t h, uint8_t f) {
  // SEPI: this function (so far) only supports loading of 'indexed' images
}

/*
 * _qk_load_skins
 *
 * loads the skin(s) data from mdl buffer
 */
static uintptr_t _qk_load_skins(qk_mdl *mdl, uintptr_t mem) {
  const uint32_t make_index_255_transparent = (1u << 14);
  qk_header hdr = mdl->header;
  qk_skin *skn = mdl->skin_p = (qk_skin *)mem;
  uint32_t skin_size = hdr.skin_width * hdr.skin_height;
  qk_texture_flags tf = QK_TEXFLG_PAD;

  if (hdr.flags & make_index_255_transparent) {
    // SEPI: figure out how to use it!
    tf |= QK_TEXFLG_ALPHA;
  }

  for (size_t i = 0; i < hdr.skins_count; i++) {
    if (skn->type == QK_SKIN_SINGLE) {
      // SEPI: in ironwail's 'Mod_LoadAllSkins' function they call
      // 'Mod_FloodFillSkin' which does something to the sking data buffer for
      // now i'm just ignoring that logic, but remember to come back here and
      // figure out what it does!

      skn->data = (uint8_t *)malloc(skin_size);
      assert(skn->data != NULL);

      mem = mem + sizeof(skn->type);
      memcpy(skn->data, (uint8_t *)mem, skin_size);
      qk_texture_flags ltf;

      if (_qk_is_full_bright(skn->data, skin_size)) {
        if (!(tf & QK_TEXFLG_ALPHA))
          ltf = tf | QK_TEXFLG_ALPHABRIGHT;
        else
          ltf = tf | QK_TEXFLG_NOBRIGHT;
      } else {
        ltf = tf;
      }

      _qk_load_image(skn->data, hdr.skin_width, hdr.skin_height, ltf);
      mem = mem + skin_size;
    } else {
      // SEPI: this is not implement yet, and i don't know if i will ever add
      // this feature!
      assert(1 == 0);
    }
  }

cleanup:
  return mem;
}

/*
 * qk_load_mdl
 *
 * given a path and an header it will load the mdl file point by path
 * into a raw mdl header of type 'qk_header'
 */
sqv_err qk_load_mdl(const char *path, qk_mdl *mdl) {
  sqv_err err = SQV_SUCCESS;
  FILE *f = fopen(path, "rb");
  assert(f != NULL);

  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);

  uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * size);
  assert(f != NULL);

  size_t rsize = fread(buf, 1, size, f);
  assert(rsize == size);

  qk_header *tmp = (qk_header *)buf;
  mdl->header.magic_codes = _qk_to_le32(tmp->magic_codes);
  mdl->header.version = _qk_to_le32(tmp->version);
  mdl->header.skins_count = _qk_to_le32(tmp->skins_count);
  mdl->header.skin_width = _qk_to_le32(tmp->skin_width);
  mdl->header.skin_height = _qk_to_le32(tmp->skin_height);
  mdl->header.vertices_count = _qk_to_le32(tmp->vertices_count);
  mdl->header.triangles_count = _qk_to_le32(tmp->triangles_count);
  mdl->header.frames_count = _qk_to_le32(tmp->frames_count);
  mdl->header.flags = _qk_to_le32(tmp->flags);
  mdl->header.bounding_radius = _qk_to_lefloat(tmp->bounding_radius);
  mdl->header.size = _qk_to_lefloat(tmp->size);

  for (int i = 0; i < 3; i++) {
    mdl->header.scale[i] = _qk_to_lefloat(tmp->scale[i]);
    mdl->header.origin[i] = _qk_to_lefloat(tmp->origin[i]);
    mdl->header.eye_position[i] = _qk_to_lefloat(tmp->eye_position[i]);
  }

  // SEPI: if the following constants are only relevant to this function
  // then by all means keep them here
  const int expected_magic_codes =
      (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I');
  const int expected_version = 6;
  const int max_allowed_skin_height = 480;
  const int max_allowed_vertices_count = 2000;
  const int max_allowed_triangles_count = 4096;

  assert(mdl->header.magic_codes == expected_magic_codes);
  assert(mdl->header.version == expected_version);
  assert(mdl->header.skin_height <= max_allowed_skin_height);
  assert(mdl->header.vertices_count <= max_allowed_vertices_count);
  assert(mdl->header.triangles_count <= max_allowed_triangles_count);
  assert(mdl->header.vertices_count > 0);
  assert(mdl->header.triangles_count > 0);
  assert(mdl->header.frames_count > 0);

  uintptr_t mem = ((uintptr_t)buf) + sizeof(qk_header);
  mem = _qk_load_skins(mdl, ++mem);

cleanup:
  if (buf) {
    free(buf);
  }

  if (f) {
    fclose(f);
  }
  return err;
}

static void _qk_init_full_bright_palette(void) {
  size_t i, j;
  for (i = 0; i < 256; i++) {
    if (!_qk_palette[i][0] && !_qk_palette[i][1] && !_qk_palette[i][2])
      continue; // black can't be fullbright

    for (j = 1; j < 64; j++)
      if (_qk_colormap[i + j * 256] != _qk_colormap[i])
        break;

    if (j == 64) {
      _qk_palette_full_bright[i / 32u] |= 1u << (i % 32u);
    }
  }
}

sqv_err qk_init(void) {
  _qk_init_full_bright_palette();
  return SQV_SUCCESS;
}