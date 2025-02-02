#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "sqv_err.h"
#include "qk_mdl.h"

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
  // we suippose the image depth is always 1

  size_t size = width * height;
  skin->width = width;
  skin->height = height;
  skin->pixels = (uint8_t *)malloc(size * 4); // SEPI:MEM: 4 => r, g, b, a

  for (size_t i = 0, j = 0; i < size; i++, j += 4) {
    uint8_t index = indices[i];
    const uint8_t *rgb = _qk_palette[index];
    skin->pixels[j + 0] = rgb[0]; // red
    skin->pixels[j + 1] = rgb[1]; // green
    skin->pixels[j + 2] = rgb[2]; // blue
    skin->pixels[j + 3] = 255;    // alpha, always opaque
  }

  // Each row has width * 4 bytes (RGBA format)
  int stride = width * 4;

  // Save as PNG
  if (stbi_write_png("___skin.png", width, height, 4, skin->pixels, stride)) {
    printf("Image saved successfully as %s\n", "___skin.png");
  } else {
    fprintf(stderr, "Error saving PNG file.\n");
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

  // SEPI:MEM:
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
      _qk_load_image(skin_data, hdr.skin_width, hdr.skin_height,
                     &(mdl->skins[i]));

      mem = mem + skin_size;

      free(skin_data);
      skin_data = NULL;
    } else {
      // SEPI: this is not implement yet, and i don't know if i will ever add
      // this feature!
      assert(1 == 0);
    }
  }

cleanup:
  if (skin_data)
    free(skin_data);

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
  mem = _qk_load_skins(mdl, mem);

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
    if (mdl->skins[i].pixels)
      free(mdl->skins[i].pixels);
  }

  if (mdl->skins)
    free(mdl->skins);

  return SQV_SUCCESS;
}