#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "sqv_err.h"
#include "qk_mdl.h"

// TODO: make sure that when converting numbers from memory, memory alignment is
// properly handled

/*
 * external variables
 */
extern const uint8_t _qk_palette[256][3];

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
 * _qk_load_image
 *
 * loads texture data baked inside mdl receives indices and image width and
 * height and returns the texture image data
 */

static sqv_err _qk_load_image(uint8_t *indices, uint32_t width, uint32_t height,
                              qk_skin *skin) {
  // SEPI: this function (so far) only supports loading of 'indexed' images and
  // we suppose the image depth is always 1

  size_t size = width * height;
  skin->width = width;
  skin->height = height;
  skin->pixels = (uint8_t *)malloc(size * 4); // SEPI: mem 4 => r, g, b, a

  for (size_t i = 0, j = 0; i < size; i++, j += 4) {
    uint8_t index = indices[i];
    const uint8_t *rgb = _qk_palette[index];
    skin->pixels[j + 0] = rgb[0]; // red
    skin->pixels[j + 1] = rgb[1]; // green
    skin->pixels[j + 2] = rgb[2]; // blue
    skin->pixels[j + 3] = 255;    // alpha, always opaque
  }

  return SQV_SUCCESS;
}

/*
 * _qk_load_skins
 *
 * loads the skin(s) data from mdl buffer
 */
static uintptr_t _qk_load_skins(qk_mdl *mdl, uintptr_t mem) {
  const uint32_t make_index_255_transparent = (1u << 14);
  qk_header hdr = mdl->header;
  qk_skintype *skin_type = NULL;
  uint8_t *skin_data = NULL;
  uint32_t skin_size = hdr.skin_width * hdr.skin_height;

  // SEPI: mem
  mdl->skins = (qk_skin *)malloc(sizeof(qk_skin) * hdr.skins_count);
  assert(mdl->skins != NULL);

  for (size_t i = 0; i < hdr.skins_count; i++) {
    skin_type = (qk_skintype *)mem;
    if (*skin_type == QK_SKIN_SINGLE) {
      skin_data = (uint8_t *)malloc(skin_size);
      assert(skin_data != NULL);

      mem = mem + sizeof(qk_skintype);
      memcpy(skin_data, (uint8_t *)mem, skin_size);

      qk_skin skin;
      sqv_err err = _qk_load_image(skin_data, hdr.skin_width, hdr.skin_height,
                                   &(mdl->skins[i]));
      assert(err == SQV_SUCCESS);

      // we need to clean the memory here before it gets leaked
      free(skin_data);
      skin_data = NULL;

      mem = mem + skin_size;
    } else {
      // SEPI: this is not implement yet, and i don't know if i will ever add
      // this feature!
      abort();
    }
  }

cleanup:
  // SEPI: if we do a good job of cleaning memory, there should be no
  // 'skin_data' at this stage, so maybe we should assert this as a fact
  // however there is always the risk that we may set NULL to a pointer value
  // but forget to actually free the memory, but we are using address sanitizer
  // in debug mode, so if there are any memory leaks we should be able to detect
  // it, hmmm (thinking face!)
  assert(skin_data == NULL);

  return mem;
}

/*
 * _qk_load_texcoords
 *
 * loads the texture coordinates from mdl buffer
 */
static uintptr_t _qk_load_texcoords(qk_mdl *mdl, uintptr_t mem) {
  qk_header hdr = mdl->header;

  // SEPI: mem
  mdl->texcoords =
      (qk_texcoords *)malloc(sizeof(qk_texcoords) * hdr.vertices_count);
  assert(mdl->texcoords != NULL);

  for (size_t i = 0; i < hdr.vertices_count; i++) {
    qk_texcoords *ptr = (qk_texcoords *)mem;
    mdl->texcoords[i].onseam = _qk_to_le32(ptr->onseam);
    mdl->texcoords[i].s = _qk_to_le32(ptr->s);
    mdl->texcoords[i].t = _qk_to_le32(ptr->t);
    mem += sizeof(qk_texcoords);
  }

  return mem;
}

/*
 * _qk_load_triangles_idx
 *
 * loads the triangles_idx data from mdl buffer
 */
static uintptr_t _qk_load_triangles_idx(qk_mdl *mdl, uintptr_t mem) {
  qk_header hdr = mdl->header;

  // SEPI: mem
  mdl->triangles_idx = (qk_triangles_idx *)malloc(sizeof(qk_triangles_idx) *
                                                  hdr.triangles_count);
  assert(mdl->triangles_idx != NULL);

  for (size_t i = 0; i < hdr.triangles_count; i++) {
    qk_triangles_idx *ptr = (qk_triangles_idx *)mem;
    mdl->triangles_idx[i].frontface = _qk_to_le32(ptr->frontface);
    for (size_t j = 0; j < 3; j++) {
      mdl->triangles_idx[i].vertices_idx[j] = _qk_to_le32(ptr->vertices_idx[j]);
    }
    mem += sizeof(qk_triangles_idx);
  }

  return mem;
}

/*
 * _qk_calc_bounds
 *
 * loads the triangles_idx data from mdl buffer
 */
static sqv_err _qk_calc_bounds(qk_mdl *mdl) {
  // TODO: implement this
  return SQV_SUCCESS;
}

/*
 * _qk_make_display_lists
 *
 * construct a GPU friendly list of vertices, omFg this is the hardest part of
 * the quake code to fathem!
 */
static sqv_err _qk_make_display_lists(qk_mdl *mdl) {
  // TODO: implement this
  return SQV_SUCCESS;
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

  // SEPI: can i use 'uintptr_t' here as well instead of 'uint8_t *' ?
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

  // TODO: i guess asserts here make no sense because they are removed from
  // release builds and when confronted with a malformed mdl file this will
  // break silently, so maybe introduce some proper error handling
  assert(mdl->header.magic_codes == expected_magic_codes);
  assert(mdl->header.version == expected_version);
  assert(mdl->header.skin_height <= max_allowed_skin_height);
  assert(mdl->header.vertices_count <= max_allowed_vertices_count);
  assert(mdl->header.triangles_count <= max_allowed_triangles_count);
  assert(mdl->header.vertices_count > 0);
  assert(mdl->header.triangles_count > 0);
  assert(mdl->header.frames_count > 0);

  uintptr_t mem = ((uintptr_t)buf) + sizeof(qk_header);
  mem = _qk_load_skins(mdl, mem);
  mem = _qk_load_texcoords(mdl, mem);
  mem = _qk_load_triangles_idx(mdl, mem);

  err = _qk_calc_bounds(mdl);
  assert(err == SQV_SUCCESS);

  err = _qk_make_display_lists(mdl);
  assert(err == SQV_SUCCESS);

cleanup:
  if (buf) {
    free(buf);
  }

  if (f) {
    fclose(f);
  }
  return err;
}

sqv_err qk_init(void) { return SQV_SUCCESS; }

sqv_err qk_deinit(qk_mdl *mdl) {
  for (size_t i = 0; i < mdl->header.skins_count; i++) {
    if (mdl->skins[i].pixels) {
      free(mdl->skins[i].pixels);
      mdl->skins[i].pixels = NULL;
    }
  }

  if (mdl->skins) {
    free(mdl->skins);
    mdl->skins = NULL;
  }

  if (mdl->texcoords) {
    free(mdl->texcoords);
    mdl->texcoords = NULL;
  }

  if (mdl->triangles_idx) {
    free(mdl->triangles_idx);
    mdl->triangles_idx = NULL;
  }

  return SQV_SUCCESS;
}