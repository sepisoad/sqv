#ifndef QK_MDL_HEADER_
#define QK_MDL_HEADER_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>

#ifdef DEBUG
#include "../deps/stb_image_write.h"
#endif

#include "../utils/all.h"
#include "../../deps/hmm.h"
#include "../../deps/sokol_gfx.h"

/* RAW QUAKE TYPES AS THEY ARE STORED IN MDL FILE */

typedef float qk_raw_vectorf;

typedef qk_raw_vectorf qk_raw_vector3f[3];

typedef int32_t qk_raw_vertices_idx[3];

typedef uint8_t qk_raw_vertex3[3];

typedef enum {
  QK_ERR_UNKNOWN = -1,
  QK_ERR_SUCCESS = 0,
  QK_ERR_FILE_OPEN,
  QK_ERR_MEM_ALLOC,
  QK_ERR_READ_SIZE,
  QK_ERR_RAME_IDX,
  QK_ERR_INVALID,
} qk_err;

typedef enum {
  QK_ST_UNKNOWN = -1,
  QK_ST_SYNC = 0,
  QK_ST_RAND,
  QK_ST_FRAMETIME,
} qk_synctype;

typedef enum {
  QK_SKIN_UNKNOWN = -1,
  QK_SKIN_SINGLE = 0,
  QK_SKIN_GROUP,
} qk_skintype;

typedef enum {
  QK_FT_UNKNOWN = -1,
  QK_FT_SINGLE = 0,
  QK_FT_GROUP,
} qk_frametype;

typedef struct {
  int32_t magic_codes;
  int32_t version;
  qk_raw_vector3f scale;
  qk_raw_vector3f translate;
  float bounding_radius;
  qk_raw_vector3f eye_position;
  int32_t skins_count;
  int32_t skin_width;
  int32_t skin_height;
  int32_t vertices_count;
  int32_t triangles_count;
  int32_t frames_count;
  qk_synctype sync_type;  // SEPI: what is this?
  int32_t flags;          // SEPI: what is this?
  float size;             // SEPI: what is this?
} qk_raw_header;

typedef struct {
  int32_t onseam;
  int32_t s;
  int32_t t;
} qk_raw_texcoord;

typedef struct {
  int32_t frontface;
  qk_raw_vertices_idx vertices_idx;
} qk_raw_triangles_idx;

typedef struct {
  qk_raw_vertex3 vertex;
  uint8_t normal_idx;
} qk_raw_triangle_vertex;

typedef struct {
  qk_raw_triangle_vertex bbox_min;
  qk_raw_triangle_vertex bbox_max;
  char name[16];
} qk_raw_frame_single;

typedef struct {
  int32_t frames_count;
  qk_raw_triangle_vertex bbox_min;
  qk_raw_triangle_vertex bbox_max;
} qk_raw_frames_group;

/* PROCESSED QUAKE TYPES, USEFUL AT RUNTIME */

typedef struct {
  hmm_v3 vertex;
  hmm_v3 normal;
} qk_vertex;

typedef struct {
  hmm_v2 uv;
  uint32_t vertex_idx;
} qk_mesh;

typedef struct {
  char name[16];
  hmm_v3 bbox_min;
  hmm_v3 bbox_max;
  qk_vertex* vertices;
  uint32_t vertices_count;
} qk_frame;

/* animation is needed to categorize frames into poses*/
// typedef struct {
//   uint32_t frames_count;
//   qk_frame** frames;
// } qk_animation;

typedef struct {
  float radius;  // bounding radius
  uint32_t skin_width;
  uint32_t skin_height;
  uint32_t skins_count;
  uint32_t vertices_count;
  uint32_t triangles_count;
  uint32_t mesh_count;
  uint32_t indices_count;
  uint32_t frames_count;
  uint32_t poses_count;
  hmm_vec3 scale;      // model scale
  hmm_vec3 translate;  // model translate
  hmm_vec3 eye;        // eye position
} qk_header;

typedef struct {
  sg_image image;
  sg_sampler sampler;
} qk_skin;

typedef struct {
  qk_header header;
  qk_skin* skins;
  qk_vertex* vertices;
  qk_mesh* mesh;
  uint32_t* indices;
  qk_frame* frames;
  arena mem;
} qk_mdl;

/* ****************** quake::mdl API ****************** */
qk_err qk_load_mdl(const uint8_t*, size_t, qk_mdl*);
void qk_get_frame_vertices(qk_mdl*,
                           uint32_t,
                           float**,
                           size_t*,
                           hmm_vec3*,
                           hmm_vec3*);
void qk_unload_mdl(qk_mdl*);
/* ****************** quake::mdl API ****************** */

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

#ifdef QK_MDL_IMPLEMENTATION

#include "data.h"

#define MAX_STATIC_MEM 8192

const int MAGICCODE = (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I');
const int MD0VERSION = 6;
const int MAXSKINHEIGHT = 480;
const int MAXVERTICES = 2000;
const int MAXTRIANGLES = 4096;
const int MAXSKINS = 32;

extern const uint8_t _qk_palette[256][3];
extern const float _qk_normals[162][3];

static qk_err estimate_memory(arena* mem,
                              const qk_raw_header* rhdr,
                              size_t bufsz,
                              const qk_header* hdr,
                              uint32_t* frames_count,
                              uint32_t* verts_count) {
  arena_begin_estimate(mem);

  // 🔹 Estimate memory for skins pixels
  size_t pixelsz = hdr->skins_count * hdr->skin_width * hdr->skin_height *
                   sizeof(uint8_t) * 4;
  arena_estimate_add(mem, pixelsz, alignof(uint8_t));

  // 🔹 Estimate memory for skins texture
  size_t skinsz = sizeof(qk_skin) * hdr->skins_count;
  arena_estimate_add(mem, skinsz, alignof(qk_skin));

  // 🔹 Estimate memory for texture UVs
  size_t rtexcoordsz = sizeof(qk_raw_texcoord) * hdr->vertices_count;
  arena_estimate_add(mem, rtexcoordsz, alignof(qk_raw_texcoord));

  // 🔹 Estimate memory for triangle indices
  size_t rtriidxsz = sizeof(qk_raw_triangles_idx) * hdr->triangles_count;
  arena_estimate_add(mem, rtriidxsz, alignof(qk_raw_triangles_idx));

  *frames_count = 0;
  *verts_count = 0;

  const uint8_t* ptr = (const uint8_t*)rhdr + sizeof(qk_raw_header);
  const uint8_t* data_end = (const uint8_t*)rhdr + bufsz;

  // 🔹 Advance past skin data
  for (uint32_t i = 0; i < hdr->skins_count; i++) {
    qk_skintype* skin_type = (qk_skintype*)ptr;
    ptr += sizeof(qk_skintype);
    if (*skin_type == QK_SKIN_SINGLE) {
      ptr += hdr->skin_width * hdr->skin_height;
    } else {
      return QK_ERR_INVALID;
    }
  }

  // 🔹 Advance past texture UVs
  ptr += sizeof(qk_raw_texcoord) * hdr->vertices_count;

  // 🔹 Advance past triangle indices
  ptr += sizeof(qk_raw_triangles_idx) * hdr->triangles_count;

  // 🔹 Now, `ptr` points to frames
  for (uint32_t i = 0; i < hdr->frames_count; i++) {
    if (ptr + sizeof(qk_frametype) > data_end) {
      return QK_ERR_INVALID;
    }

    qk_frametype ft = *(const qk_frametype*)ptr;
    ptr += sizeof(qk_frametype);

    if (ft == QK_FT_SINGLE) {
      *frames_count += 1;
      *verts_count += hdr->vertices_count;

      if (ptr + sizeof(qk_raw_frame_single) +
              (hdr->vertices_count * sizeof(qk_raw_triangle_vertex)) >
          data_end) {
        return QK_ERR_INVALID;
      }

      ptr += sizeof(qk_raw_frame_single) +
             (hdr->vertices_count * sizeof(qk_raw_triangle_vertex));
    } else {
      if (ptr + sizeof(qk_raw_frames_group) > data_end) {
        return QK_ERR_INVALID;
      }

      const qk_raw_frames_group* grp = (const qk_raw_frames_group*)ptr;
      uint32_t frame_count = grp->frames_count;
      *frames_count += frame_count;
      *verts_count += frame_count * hdr->vertices_count;

      ptr += sizeof(qk_raw_frames_group) + sizeof(float) * frame_count;

      if (ptr + (hdr->vertices_count * sizeof(qk_raw_triangle_vertex)) *
                    frame_count >
          data_end) {
        return QK_ERR_INVALID;
      }

      ptr +=
          (hdr->vertices_count * sizeof(qk_raw_triangle_vertex)) * frame_count;
    }
  }

  // 🔹 Add frame memory estimation
  size_t framesz = sizeof(qk_frame) * (*frames_count);
  arena_estimate_add(mem, framesz, alignof(qk_frame));

  // 🔹 Add vertices memory estimation
  size_t verticesz = sizeof(qk_vertex) * (*verts_count);
  arena_estimate_add(mem, verticesz, alignof(qk_vertex));

  // 🔹 Add mesh memory estimation
  size_t meshsz = sizeof(qk_mesh) * hdr->vertices_count;
  arena_estimate_add(mem, meshsz, alignof(qk_mesh));

  // 🔹 Add indices memory estimation
  size_t indicesz = sizeof(uint32_t) * hdr->triangles_count * 3;
  arena_estimate_add(mem, indicesz, alignof(uint32_t));

  // 🔹 Finalize estimation
  arena_end_estimate(mem);

  return QK_ERR_SUCCESS;
}

static void load_image(const uint8_t* ptr, uint8_t* pixels, size_t size) {
  uint8_t* indices = (uint8_t*)ptr;
  for (size_t i = 0, j = 0; i < size; i++, j += 4) {
    uint32_t index = indices[i];
    pixels[j + 0] = _qk_palette[index][0];  // red
    pixels[j + 1] = _qk_palette[index][1];  // green
    pixels[j + 2] = _qk_palette[index][2];  // blue
    pixels[j + 3] = 255;                    // alpha, always opaque
  }
}

static const uint8_t* load_skins(qk_mdl* mdl, const uint8_t* ptr) {
  const qk_header* hdr = &mdl->header;
  arena* mem = &mdl->mem;
  uint32_t width = hdr->skin_width;
  uint32_t height = hdr->skin_height;
  size_t skin_size = width * height;

  size_t memsz = sizeof(qk_skin) * hdr->skins_count;
  qk_skin* skins = (qk_skin*)arena_alloc(mem, memsz, alignof(qk_skin));
  makesure(skins != NULL, "arena out of memory!");

  for (size_t i = 0; i < hdr->skins_count; i++) {
    qk_skintype* skin_type = (qk_skintype*)ptr;
    if (*skin_type == QK_SKIN_SINGLE) {
      ptr += sizeof(qk_skintype);

      size_t pixelsz = sizeof(uint8_t) * skin_size * 4;
      uint8_t* pixels = (uint8_t*)arena_alloc(mem, pixelsz, alignof(uint8_t));
      makesure(pixels != NULL, "arena out of memory!");

      load_image(ptr, pixels, skin_size);

#ifdef DEBUG
      char skin_name[1024] = {0};
      sprintf(skin_name, ".ignore/skin_%zu.png", i);
      stbi_write_png(skin_name, width, height, 4, pixels, width * 4);
#endif

      skins[i].image = sg_alloc_image();
      skins[i].sampler = sg_make_sampler(&(sg_sampler_desc){
          .min_filter = SG_FILTER_LINEAR,
          .mag_filter = SG_FILTER_LINEAR,
      });
      sg_init_image(skins[i].image,
                    &(sg_image_desc){.width = width,
                                     .height = height,
                                     .pixel_format = SG_PIXELFORMAT_RGBA8,
                                     .data.subimage[0][0] = {
                                         .ptr = pixels,
                                         .size = (size_t)(width * height * 4),
                                     }});
      ptr += skin_size;
    } else {
      /*
       * SEPI:
       * this is not implemented yet, and may never be implemented!
       * make sure to re-evaluate arena size calculations when you
       * implement this feature!
       */
      makesure(false, "load_skin() does not support multi skin YET!");
    }
  }

  mdl->skins = skins;
  return ptr;
}

static const uint8_t* load_raw_uv_coordinates(qk_mdl* mdl,
                                              const uint8_t* ptr,
                                              qk_raw_texcoord** coords) {
  arena* mem = &mdl->mem;
  const qk_header* hdr = &mdl->header;
  size_t memsz = sizeof(qk_raw_texcoord) * hdr->vertices_count;
  *coords = (qk_raw_texcoord*)arena_alloc(mem, memsz, alignof(qk_raw_texcoord));
  makesure(coords != NULL, "arena out of memory!");

  const qk_raw_texcoord* src =
      (const qk_raw_texcoord*)ptr;  // Use a separate pointer

  for (size_t i = 0; i < hdr->vertices_count; i++) {
    (*coords)[i].onseam = endian_i32(src->onseam);
    (*coords)[i].s = endian_i32(src->s);
    (*coords)[i].t = endian_i32(src->t);
    src++;  // Move forward correctly
  }

  return (const uint8_t*)src;
}

static const uint8_t* load_raw_triangles_indices(
    arena* mem,
    const uint8_t* ptr,
    const qk_header* hdr,
    qk_raw_triangles_idx** trisidx) {
  size_t memsz = sizeof(qk_raw_triangles_idx) * hdr->triangles_count;
  *trisidx = (qk_raw_triangles_idx*)arena_alloc(mem, memsz,
                                                alignof(qk_raw_triangles_idx));
  makesure(trisidx != NULL, "arena out of memory!");

  const qk_raw_triangles_idx* src =
      (const qk_raw_triangles_idx*)ptr;  // Use separate pointer

  for (size_t i = 0; i < hdr->triangles_count; i++) {
    (*trisidx)[i].frontface = endian_i32(src->frontface);
    for (size_t j = 0; j < 3; j++) {
      (*trisidx)[i].vertices_idx[j] = endian_i32(src->vertices_idx[j]);
    }
    src++;  // Move forward correctly
  }

  return (const uint8_t*)src;
}

static const uint8_t* load_frame_single(const qk_header* hdr,
                                        qk_frame* frame,
                                        uint32_t* posidx,
                                        uint32_t* vertex_offset,
                                        qk_vertex* vertices,
                                        const uint8_t* ptr) {
  qk_raw_frame_single* snl = (qk_raw_frame_single*)ptr;

  memcpy(frame->name, snl->name, sizeof(frame->name) - 1);
  frame->name[sizeof(frame->name) - 1] = '\0';

  frame->bbox_min.X = snl->bbox_min.vertex[0];
  frame->bbox_min.Y = snl->bbox_min.vertex[1];
  frame->bbox_min.Z = snl->bbox_min.vertex[2];

  frame->bbox_max.X = snl->bbox_max.vertex[0];
  frame->bbox_max.Y = snl->bbox_max.vertex[1];
  frame->bbox_max.Z = snl->bbox_max.vertex[2];

  frame->vertices = &vertices[*vertex_offset];
  frame->vertices_count = hdr->vertices_count;

  qk_raw_triangle_vertex* raw_verts = (qk_raw_triangle_vertex*)(snl + 1);
  for (uint32_t i = 0; i < hdr->vertices_count; i++) {
    frame->vertices[i].vertex = (hmm_v3){
        raw_verts[i].vertex[0], raw_verts[i].vertex[1], raw_verts[i].vertex[2]};
    frame->vertices[i].normal =
        (hmm_v3){_qk_normals[raw_verts[i].normal_idx][0],
                 _qk_normals[raw_verts[i].normal_idx][1],
                 _qk_normals[raw_verts[i].normal_idx][2]};
  }

  *vertex_offset += hdr->vertices_count;
  *posidx += 1;
  return (const uint8_t*)(raw_verts + hdr->vertices_count);
}

static const uint8_t* load_frames_group(const qk_header* hdr,
                                        qk_frame* frames,
                                        uint32_t* posidx,
                                        uint32_t* vertex_offset,
                                        qk_vertex* vertices,
                                        const uint8_t* ptr) {
  qk_raw_frames_group* grp = (qk_raw_frames_group*)ptr;
  uint32_t frames_count = endian_i32(grp->frames_count);

  float* interval_ptr = (float*)(grp + 1);
  ptr = (const uint8_t*)(interval_ptr + frames_count);

  for (uint32_t i = 0; i < frames_count; i++) {
    qk_frame* frame = &frames[*posidx];

    frame->bbox_min.X = grp->bbox_min.vertex[0];
    frame->bbox_min.Y = grp->bbox_min.vertex[1];
    frame->bbox_min.Z = grp->bbox_min.vertex[2];

    frame->bbox_max.X = grp->bbox_max.vertex[0];
    frame->bbox_max.Y = grp->bbox_max.vertex[1];
    frame->bbox_max.Z = grp->bbox_max.vertex[2];

    frame->vertices = &vertices[*vertex_offset];
    frame->vertices_count = hdr->vertices_count;

    qk_raw_triangle_vertex* raw_verts = (qk_raw_triangle_vertex*)ptr;
    for (uint32_t j = 0; j < hdr->vertices_count; j++) {
      frame->vertices[j].vertex =
          (hmm_v3){raw_verts[j].vertex[0], raw_verts[j].vertex[1],
                   raw_verts[j].vertex[2]};
      frame->vertices[j].normal =
          (hmm_v3){_qk_normals[raw_verts[j].normal_idx][0],
                   _qk_normals[raw_verts[j].normal_idx][1],
                   _qk_normals[raw_verts[j].normal_idx][2]};
    }

    *vertex_offset += hdr->vertices_count;
    *posidx += 1;
    ptr = (const uint8_t*)(raw_verts + hdr->vertices_count);
  }

  return ptr;
}

static const uint8_t* load_raw_frames(qk_mdl* mdl,
                                      const uint8_t* ptr,
                                      uint32_t frames_count,
                                      uint32_t verts_count) {
  arena* mem = &mdl->mem;
  const qk_header* hdr = &mdl->header;

  size_t framesz = sizeof(qk_frame) * frames_count;
  mdl->frames = (qk_frame*)arena_alloc(mem, framesz, alignof(qk_frame));
  makesure(mdl->frames != NULL, "arena out of memory!");

  size_t verticesz = sizeof(qk_vertex) * verts_count;
  mdl->vertices = (qk_vertex*)arena_alloc(mem, verticesz, alignof(qk_vertex));
  makesure(mdl->vertices != NULL, "arena out of memory!");

  uint32_t frame_index = 0, vertex_offset = 0;
  for (uint32_t i = 0; i < mdl->header.frames_count; i++) {
    qk_frametype ft = endian_i32(*(qk_frametype*)ptr);
    ptr += sizeof(qk_frametype);

    if (ft == QK_FT_SINGLE) {
      ptr = load_frame_single(&mdl->header, &mdl->frames[frame_index],
                              &frame_index, &vertex_offset, mdl->vertices, ptr);
    } else {
      ptr = load_frames_group(&mdl->header, mdl->frames, &frame_index,
                              &vertex_offset, mdl->vertices, ptr);
    }
  }

  return ptr;
}

static qk_err calc_bounds(qk_header* hdr) {
  return QK_ERR_SUCCESS;
}

static void make_display_lists(arena* mem,
                               qk_mdl* mdl,
                               const qk_raw_texcoord* coords,
                               const qk_raw_triangles_idx* trisidx) {
  size_t memsz = sizeof(qk_mesh) * mdl->header.vertices_count;
  qk_mesh* mesh = (qk_mesh*)arena_alloc(mem, memsz, alignof(qk_mesh));
  makesure(mesh != NULL, "arena out of memory!");

  memsz = sizeof(uint32_t) * mdl->header.triangles_count * 3;
  uint32_t* indices = (uint32_t*)arena_alloc(mem, memsz, alignof(uint32_t));
  makesure(indices != NULL, "arena out of memory!");

  uint32_t mesh_count = mdl->header.vertices_count;
  uint32_t indices_count = 0;

  for (uint32_t triidx = 0; triidx < mdl->header.triangles_count; triidx++) {
    for (uint8_t vertxyz = 0; vertxyz < 3; vertxyz++) {
      int32_t vertidx = trisidx[triidx].vertices_idx[vertxyz];

      float s = coords[vertidx].s;
      float t = coords[vertidx].t;

      if (!trisidx[triidx].frontface && coords[vertidx].onseam)
        s += mdl->header.skin_width / 2;

      s = (s + 0.5) / mdl->header.skin_width;
      t = (t + 0.5) / mdl->header.skin_height;

      mesh[vertidx].vertex_idx = vertidx;
      mesh[vertidx].uv.U = s;
      mesh[vertidx].uv.V = t;

      indices[indices_count++] = vertidx;
    }
  }

  mdl->header.mesh_count = mesh_count;
  mdl->mesh = mesh;

  mdl->header.indices_count = indices_count;
  mdl->indices = indices;
}

qk_err qk_load_mdl(const uint8_t* buf, size_t bufsz, qk_mdl* mdl) {
  const uint8_t* ptr = buf;
  const qk_raw_header* rhdr = (qk_raw_header*)buf;

  memset(mdl, 0, sizeof(qk_mdl));

  mdl->header = (qk_header){
      .radius = endian_f32(rhdr->bounding_radius),
      .skin_width = endian_i32(rhdr->skin_width),
      .skin_height = endian_i32(rhdr->skin_height),
      .skins_count = endian_i32(rhdr->skins_count),
      .vertices_count = endian_i32(rhdr->vertices_count),
      .triangles_count = endian_i32(rhdr->triangles_count),
      .frames_count = endian_i32(rhdr->frames_count),
      .scale = {.X = endian_f32(rhdr->scale[0]),
                .Y = endian_f32(rhdr->scale[1]),
                .Z = endian_f32(rhdr->scale[2])},
      .translate = {.X = endian_f32(rhdr->translate[0]),
                    .Y = endian_f32(rhdr->translate[1]),
                    .Z = endian_f32(rhdr->translate[2])},
      .eye = {.X = endian_f32(rhdr->eye_position[0]),
              .Y = endian_f32(rhdr->eye_position[1]),
              .Z = endian_f32(rhdr->eye_position[2])},
  };

  const qk_header* hdr = &mdl->header;
  arena* mem = &mdl->mem;
  uint32_t frames_count, verts_count;
  qk_raw_texcoord* raw_tex_coords = NULL;
  qk_raw_triangles_idx* raw_tris_idx = NULL;

  estimate_memory(mem, rhdr, bufsz, hdr, &frames_count, &verts_count);

  ptr += sizeof(qk_raw_header);
  ptr = load_skins(mdl, ptr);
  ptr = load_raw_uv_coordinates(mdl, ptr, &raw_tex_coords);
  ptr = load_raw_triangles_indices(mem, ptr, hdr, &raw_tris_idx);
  ptr = load_raw_frames(mdl, ptr, frames_count, verts_count);

  make_display_lists(mem, mdl, raw_tex_coords, raw_tris_idx);
  return QK_ERR_SUCCESS;
}

void qk_get_frame_vertices(qk_mdl* mdl,
                           uint32_t frmidx,
                           float** verts,
                           size_t* vertsz,
                           hmm_vec3* bbox_min,
                           hmm_vec3* bbox_max) {
  makesure(frmidx < mdl->header.frames_count, "frame index out of bound");

  qk_frame* frm = &mdl->frames[frmidx];
  makesure(frm->vertices != NULL, "frame vertices are NULL");

  *verts = (float*)malloc(sizeof(float) * frm->vertices_count *
                          (3 /*vertices*/ + 2 /*UVs*/));
  makesure(*verts != NULL, "malloc failed!");

  *vertsz = frm->vertices_count * 5;
  printf("----------------------\nvertices {");
  for (uint32_t bi = 0, vi = 0; bi < *vertsz; bi += 5, vi++) {
    makesure(vi < frm->vertices_count, "vertex index out of bounds!");

    (*verts)[bi + 0] = (frm->vertices[vi].vertex.X * mdl->header.scale.X) +
                       mdl->header.translate.X;
    (*verts)[bi + 1] = (frm->vertices[vi].vertex.Y * mdl->header.scale.Y) +
                       mdl->header.translate.Y;
    (*verts)[bi + 2] = (frm->vertices[vi].vertex.Z * mdl->header.scale.Z) +
                       mdl->header.translate.Z;

    (*verts)[bi + 3] = (mdl->mesh[vi].uv.U);
    (*verts)[bi + 4] = (mdl->mesh[vi].uv.U);

    printf("vert{%f, %f, %f} uv{%f, %f}\n", (*verts)[bi + 0], (*verts)[bi + 1],
           (*verts)[bi + 2], (*verts)[bi + 3], (*verts)[bi + 4]);
  }

  bbox_min->X =
      (frm->bbox_min.X * mdl->header.scale.X) + mdl->header.translate.X;
  bbox_min->Y =
      (frm->bbox_min.Y * mdl->header.scale.Y) + mdl->header.translate.Y;
  bbox_min->Z =
      (frm->bbox_min.Z * mdl->header.scale.Z) + mdl->header.translate.Z;

  bbox_max->X =
      (frm->bbox_max.X * mdl->header.scale.X) + mdl->header.translate.X;
  bbox_max->Y =
      (frm->bbox_max.Y * mdl->header.scale.Y) + mdl->header.translate.Y;
  bbox_max->Z =
      (frm->bbox_max.Z * mdl->header.scale.Z) + mdl->header.translate.Z;

  printf("}\n----------------------\nindices {");
  for (uint32_t i = 0; i < mdl->header.indices_count; i++) {
    printf("%d, ", mdl->indices[i]);
  }
  printf("}\n----------------------\n ");
  printf("eye: (%f, %f, %f)\n", mdl->header.eye.X, mdl->header.eye.Y,
         mdl->header.eye.Z);
  printf("scale: (%f, %f, %f)\n", mdl->header.scale.X, mdl->header.scale.Y,
         mdl->header.scale.Z);
  printf("translate: (%f, %f, %f)\n", mdl->header.translate.X,
         mdl->header.translate.Y, mdl->header.translate.Z);
  printf("bbox min: (%f, %f, %f)\n", frm->bbox_min.X, frm->bbox_min.Y,
         frm->bbox_min.Z);
  printf("bbox max: (%f, %f, %f)\n", frm->bbox_max.X, frm->bbox_max.Y,
         frm->bbox_max.Z);
  printf("----------------------\n");
}

void qk_unload_mdl(qk_mdl* mdl) {
  arena_destroy(&mdl->mem);
}

#endif  // QK_MDL_IMPLEMENTATION
#endif  // QK_MDL_HEADER_
