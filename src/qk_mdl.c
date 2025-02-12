#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qk_mdl.h"
#include "sqv_err.h"
#include "sqv_mdl.h"
#include "utils.h"

// TODO: make sure that when converting numbers from memory, memory alignment is
// properly handled

extern const uint8_t _qk_palette[256][3];

static sqv_err load_image(uint8_t *indices, uint32_t width, uint32_t height,
                          qk_skin *skin) {
  // SEPI: this function (so far) only supports loading of 'indexed' images and
  // we suppose the image depth is always 1
  size_t size = width * height;
  skin->width = width;
  skin->height = height;
  skin->pixels = (uint8_t *)malloc(size * 4); // SEPI: mem 4 => r, g, b, a
  abortif(skin->pixels == NULL, "failed to allocate memory for skin image");

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

static uintptr_t load_skins(qk_mdl *mdl, uintptr_t mem) {
  const uint32_t make_index_255_transparent = (1u << 14);
  qk_header hdr = mdl->header;
  qk_skintype *skin_type = NULL;
  uint8_t *skin_data = NULL;
  uint32_t skin_size = hdr.skin_width * hdr.skin_height;

  // SEPI: mem
  mdl->skins = (qk_skin *)malloc(sizeof(qk_skin) * hdr.skins_count);
  abortif(mdl->skins != NULL, "failed to allocate memory for skins");

  for (size_t i = 0; i < hdr.skins_count; i++) {
    skin_type = (qk_skintype *)mem;
    if (*skin_type == QK_SKIN_SINGLE) {
      skin_data = (uint8_t *)malloc(skin_size);
      abortif(skin_data != NULL, "failed to allocate memory for a single skin data");

      mem = mem + sizeof(qk_skintype);
      memcpy(skin_data, (uint8_t *)mem, skin_size);

      qk_skin skin;
      sqv_err err = load_image(skin_data, hdr.skin_width, hdr.skin_height,
                               &(mdl->skins[i]));
      abortif(err == SQV_SUCCESS, "load_image() failed");

      // we need to clean the memory here before it gets leaked
      free(skin_data);
      skin_data = NULL;

      mem = mem + skin_size;
    } else {
      // SEPI: this is not implement yet, and i don't know if i will ever add
      // this feature!
      abortif(false, "load_skin() does not support multi skin YET!");
    }
  }

cleanup:
  abortif(skin_data == NULL, "skin data is not cleaned up properly");

  return mem;
}

static uintptr_t load_texcoords(qk_mdl *mdl, uintptr_t mem) {
  qk_header hdr = mdl->header;

  // SEPI: mem
  mdl->texcoords =
      (qk_texcoords *)malloc(sizeof(qk_texcoords) * hdr.vertices_count);
  abortif(mdl->texcoords != NULL, "failed to allocate memory for texture coordinates");

  for (size_t i = 0; i < hdr.vertices_count; i++) {
    qk_texcoords *ptr = (qk_texcoords *)mem;
    mdl->texcoords[i].onseam = endian_i32(ptr->onseam);
    mdl->texcoords[i].s = endian_i32(ptr->s);
    mdl->texcoords[i].t = endian_i32(ptr->t);
    mem += sizeof(qk_texcoords);
  }

  return mem;
}

static uintptr_t load_triangles_idx(qk_mdl *mdl, uintptr_t mem) {
  qk_header hdr = mdl->header;

  // SEPI: mem
  mdl->triangles_idx = (qk_triangles_idx *)malloc(sizeof(qk_triangles_idx) *
                                                  hdr.triangles_count);
  abortif(mdl->triangles_idx != NULL, "failed to allocate memory for triangle indices");

  for (size_t i = 0; i < hdr.triangles_count; i++) {
    qk_triangles_idx *ptr = (qk_triangles_idx *)mem;
    mdl->triangles_idx[i].frontface = endian_i32(ptr->frontface);
    for (size_t j = 0; j < 3; j++) {
      mdl->triangles_idx[i].vertices_idx[j] = endian_i32(ptr->vertices_idx[j]);
    }
    mem += sizeof(qk_triangles_idx);
  }

  return mem;
}

static uintptr_t load_frame_single(qk_mdl *mdl, uintptr_t mem) {
  qk_frame_single frs;
  qk_header hdr = mdl->header;
  size_t nln = sizeof(frs.name);

  qk_frame_single *tmp = (qk_frame_single *)mem;
  for (size_t i = 0; i < 3; i++) {
    frs.bbox_min.vertex[i] = tmp->bbox_min.vertex[i];
    frs.bbox_max.vertex[i] = tmp->bbox_max.vertex[i];
  }
  strncpy(frs.name, tmp->name, nln);
  mem += sizeof(qk_frame_single);
  return mem;
}

static uintptr_t load_frames_group(qk_mdl *mdl, uintptr_t mem) {
  qk_header hdr = mdl->header;
  qk_frames_group *tmp = (qk_frames_group *)mem;
  uint32_t cnt = endian_i32(tmp->frames_count);

  qk_frames_group frg;
  for (size_t i = 0; i < 3; i++) {
    frg.bbox_min.vertex[i] = tmp->bbox_min.vertex[i];
    frg.bbox_max.vertex[i] = tmp->bbox_max.vertex[i];
  }
  frg.frames_count = cnt;

  qk_frame_interval *pfi = (qk_frame_interval *)(tmp + 1);
  qk_frame_interval fi = {.interval = endian_f32(pfi->interval)};

  pfi += cnt;
  mem = (uintptr_t)pfi;

  for (size_t i = 0; i < cnt; i++) {
    qk_triangle_vertex *zzz =
        (qk_triangle_vertex *)((qk_frame_single *)pfi + 1);
    // posenum++;

    mem = (uintptr_t)((qk_triangle_vertex *)((qk_frame_single *)pfi + 1) +
                      hdr.frames_count);
  }

  return mem;
}

static uintptr_t load_frames(qk_mdl *mdl, uintptr_t mem) {
  qk_header hdr = mdl->header;

  // SEPI: i think there are two things associated with frames, first we have
  // animations maybe also known as 'poses' and then we have frames in each
  // pose, but in order to know which one is which, we keep track of each
  // animation by a combination of 'animation frames count', and 'animation
  // first frame', and again i think you need to extract the 'animation first
  // frame' indirectly by counting how many poses exist in a mdl file and then
  // how many frames are in that pose and then you need to keep it somewhere!

  for (size_t i = 0; i < hdr.frames_count; i++) {
    qk_frametype ft = endian_i32(*(qk_frametype *)mem);
    mem += sizeof(qk_frametype);
    if (ft == QK_FT_SINGLE) {
      mem = load_frame_single(mdl, mem);
    } else {
      mem = load_frames_group(mdl, mem);
    }
  }

  return mem;
}

static sqv_err calc_bounds(qk_mdl *mdl) {
  // TODO: implement this
  return SQV_SUCCESS;
}

static sqv_err make_display_lists(qk_mdl *mdl) {
  qk_header hdr = mdl->header;
  qk_triangles_idx *tip = mdl->triangles_idx;
  // qk_triangle_vertex* tvp = mdl->triangles_vertices;
  uint32_t vc = hdr.vertices_count;
  uint32_t fc = hdr.frames_count;

  // SEPI: mem
  // tvp = (qk_triangle_vertex*)malloc(sizeof(qk_triangle_vertex) * vc * fc);
  // assert(tvp != NULL);

  // for (size_t i = 0; i < fc; i++) {
  //   for (size_t j = 0; j < vc; j++) {
  //     // tvp[i * hdr.vertices_count + j] = tip[i][j];
  //     // TODO:
  //   }
  // }

  return SQV_SUCCESS;
}

// SEPI: i have changed my mind, i think we need to types of structs,
// one that represent data as is in memory with 'qk_' prefix and another
// one that represent the processed data with 'sqv_' prefix
// for now ignore the `qk_mdl *mdl` and act like that you are simply
// processing the data in this function and you will throw it away until
// the types are completly know to you!
sqv_err qk_load_mdl(const char *path, qk_mdl *_mdl_) {
  sqv_err err = SQV_SUCCESS;
  FILE *f = fopen(path, "rb");
  abortif(f != NULL, "invalid mdl file");

  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  rewind(f);

  // SEPI: can i use 'uintptr_t' here as well instead of 'uint8_t *' ?
  uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * fsize);
  abortif(buf != NULL, "failed to allocate memory to mdl file");

  // uintptr_t mem = ((uintptr_t)buf) + sizeof(qk_header);
  size_t rsize = fread(buf, 1, fsize, f);
  abortif(rsize == fsize, "read size '%zu' did not match the file size '%zu'",
          rsize, fsize);

  uintptr_t mem = (uintptr_t)buf;
  qk_header *qkhdr = (qk_header *)buf;

  // TODO: verify these values for correctness
  // also i don't know what 'flags' and 'size' are!
  int32_t _magic_code = endian_i32(qkhdr->magic_codes);
  int32_t _version = endian_i32(qkhdr->version);
  int32_t _flags = endian_i32(qkhdr->flags);
  int32_t _size = endian_f32(qkhdr->size);

  sqv_mdl_header hdr = {
      .radius = endian_f32(qkhdr->bounding_radius),
      .skin_width = endian_i32(qkhdr->skin_width),
      .skin_height = endian_i32(qkhdr->skin_height),
      .skins_count = endian_i32(qkhdr->skins_count),
      .vertices_count = endian_i32(qkhdr->vertices_count),
      .triangles_count = endian_i32(qkhdr->triangles_count),
      .frames_count = endian_i32(qkhdr->frames_count),
      .scale = {.X = endian_f32(qkhdr->scale[0]),
                .Y = endian_f32(qkhdr->scale[1]),
                .Z = endian_f32(qkhdr->scale[2])},
      .origin = {.X = endian_f32(qkhdr->origin[0]),
                 .Y = endian_f32(qkhdr->origin[1]),
                 .Z = endian_f32(qkhdr->origin[2])},
      .eye = {.X = endian_f32(qkhdr->eye_position[0]),
              .Y = endian_f32(qkhdr->eye_position[1]),
              .Z = endian_f32(qkhdr->eye_position[2])},
  };

  // SEPI: if the following constants are only relevant to this function
  // then by all means keep them here
  const int expected_magic_codes =
      (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I');
  const int expected_version = 6;
  const int max_allowed_skin_height = 480;
  const int max_allowed_vertices_count = 2000;
  const int max_allowed_triangles_count = 4096;

  abortif(_magic_code == expected_magic_codes, "wrong magic code");
  abortif(_version == expected_version, "wrong version");
  abortif(hdr.skin_height <= max_allowed_skin_height, "invalid skin height");
  abortif(hdr.vertices_count <= max_allowed_vertices_count,
          "invalid vertices count ");
  abortif(hdr.triangles_count <= max_allowed_triangles_count,
          "invalid triangles count");
  abortif(hdr.vertices_count > 0, "invalid vertices count");
  abortif(hdr.triangles_count > 0, "invalid triangles count");
  abortif(hdr.frames_count > 0, "invalid frames count");

  mem = load_skins(_mdl_, mem);
  mem = load_texcoords(_mdl_, mem);
  mem = load_triangles_idx(_mdl_, mem);
  mem = load_frames(_mdl_, mem);

  err = calc_bounds(_mdl_);
  abortif(err == SQV_SUCCESS, "calc_bound() failed");

  err = make_display_lists(_mdl_);
  abortif(err == SQV_SUCCESS, "make_display_lists() failed");

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
