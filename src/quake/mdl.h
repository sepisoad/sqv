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

typedef f32 qk_raw_vectorf;

typedef qk_raw_vectorf qk_raw_vector3f[3];

typedef i32 qk_raw_vertices_idx[3];

typedef u8 qk_raw_vertex3[3];

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
  i32 magic_codes;
  i32 version;
  qk_raw_vector3f scale;
  qk_raw_vector3f translate;
  f32 bounding_radius;
  qk_raw_vector3f eye_position;
  i32 skins_count;
  i32 skin_width;
  i32 skin_height;
  i32 vertices_count;
  i32 triangles_count;
  i32 frames_count;
  qk_synctype sync_type;  // SEPI: what is this?
  i32 flags;              // SEPI: what is this?
  f32 size;               // SEPI: what is this?
} qk_raw_header;

typedef struct {
  i32 onseam;
  i32 s;
  i32 t;
} qk_raw_texcoord;

typedef struct {
  i32 frontface;
  qk_raw_vertices_idx vertices_idx;
} qk_raw_triangles_idx;

typedef struct {
  qk_raw_vertex3 vertex;
  u8 normal_idx;
} qk_raw_triangle_vertex;

typedef struct {
  qk_raw_triangle_vertex bbox_min;
  qk_raw_triangle_vertex bbox_max;
  char name[16];
} qk_raw_frame_single;

typedef struct {
  i32 frames_count;
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
  u32 vertex_idx;
} qk_mesh;

typedef struct {
  char name[16];
  hmm_v3 bbox_min;
  hmm_v3 bbox_max;
  qk_vertex* vertices;
  u32 vertices_count;
} qk_frame;

/* animation is needed to categorize frames into poses*/
// typedef struct {
//   u32 frames_count;
//   qk_frame** frames;
// } qk_animation;

typedef struct {
  f32 radius;  // bounding radius
  u32 skin_width;
  u32 skin_height;
  u32 skins_count;
  u32 vertices_count;
  u32 triangles_count;
  u32 mesh_count;
  u32 indices_count;
  u32 frames_count;
  u32 poses_count;
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
  u32* indices;
  qk_frame* frames;
  arena mem;
} qk_mdl;

/* ****************** quake::mdl API ****************** */
qk_err qk_load_mdl(const u8*, sz, qk_mdl*);
void qk_get_frame_vertices(qk_mdl*, u32, f32**, u32*, hmm_vec3*, hmm_vec3*);
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

extern const u8 _qk_palette[256][3];
extern const f32 _qk_normals[162][3];

static qk_err estimate_memory(arena* mem,
                              const qk_raw_header* rhdr,
                              sz bufsz,
                              const qk_header* hdr,
                              u32* frames_count,
                              u32* verts_count) {
  arena_begin_estimate(mem);

  // 🔹 Estimate memory for skins pixels
  sz pixelsz =
      hdr->skins_count * hdr->skin_width * hdr->skin_height * sizeof(u8) * 4;
  arena_estimate_add(mem, pixelsz, alignof(u8));

  // 🔹 Estimate memory for skins texture
  sz skinsz = sizeof(qk_skin) * hdr->skins_count;
  arena_estimate_add(mem, skinsz, alignof(qk_skin));

  // 🔹 Estimate memory for texture UVs
  sz rtexcoordsz = sizeof(qk_raw_texcoord) * hdr->vertices_count;
  arena_estimate_add(mem, rtexcoordsz, alignof(qk_raw_texcoord));

  // 🔹 Estimate memory for triangle indices
  sz rtriidxsz = sizeof(qk_raw_triangles_idx) * hdr->triangles_count;
  arena_estimate_add(mem, rtriidxsz, alignof(qk_raw_triangles_idx));

  *frames_count = 0;
  *verts_count = 0;

  const u8* p = (const u8*)rhdr + sizeof(qk_raw_header);
  const u8* data_end = (const u8*)rhdr + bufsz;

  // 🔹 Advance past skin data
  for (u32 i = 0; i < hdr->skins_count; i++) {
    qk_skintype* skin_type = (qk_skintype*)p;
    p += sizeof(qk_skintype);
    if (*skin_type == QK_SKIN_SINGLE) {
      p += hdr->skin_width * hdr->skin_height;
    } else {
      return QK_ERR_INVALID;
    }
  }

  // 🔹 Advance past texture UVs
  p += sizeof(qk_raw_texcoord) * hdr->vertices_count;

  // 🔹 Advance past triangle indices
  p += sizeof(qk_raw_triangles_idx) * hdr->triangles_count;

  // 🔹 Now, `p` points to frames
  for (u32 i = 0; i < hdr->frames_count; i++) {
    if (p + sizeof(qk_frametype) > data_end) {
      return QK_ERR_INVALID;
    }

    qk_frametype ft = *(const qk_frametype*)p;
    p += sizeof(qk_frametype);

    if (ft == QK_FT_SINGLE) {
      *frames_count += 1;
      *verts_count += hdr->vertices_count;

      if (p + sizeof(qk_raw_frame_single) +
              (hdr->vertices_count * sizeof(qk_raw_triangle_vertex)) >
          data_end) {
        return QK_ERR_INVALID;
      }

      p += sizeof(qk_raw_frame_single) +
             (hdr->vertices_count * sizeof(qk_raw_triangle_vertex));
    } else {
      if (p + sizeof(qk_raw_frames_group) > data_end) {
        return QK_ERR_INVALID;
      }

      const qk_raw_frames_group* grp = (const qk_raw_frames_group*)p;
      u32 frame_count = grp->frames_count;
      *frames_count += frame_count;
      *verts_count += frame_count * hdr->vertices_count;

      p += sizeof(qk_raw_frames_group) + sizeof(f32) * frame_count;

      if (p + (hdr->vertices_count * sizeof(qk_raw_triangle_vertex)) *
                    frame_count >
          data_end) {
        return QK_ERR_INVALID;
      }

      p +=
          (hdr->vertices_count * sizeof(qk_raw_triangle_vertex)) * frame_count;
    }
  }

  // 🔹 Add frame memory estimation
  sz framesz = sizeof(qk_frame) * (*frames_count);
  arena_estimate_add(mem, framesz, alignof(qk_frame));

  // 🔹 Add vertices memory estimation
  sz verticesz = sizeof(qk_vertex) * (*verts_count);
  arena_estimate_add(mem, verticesz, alignof(qk_vertex));

  // 🔹 Add mesh memory estimation
  sz meshsz = sizeof(qk_mesh) * hdr->vertices_count;
  arena_estimate_add(mem, meshsz, alignof(qk_mesh));

  // 🔹 Add indices memory estimation
  sz indicesz = sizeof(u32) * hdr->triangles_count * 3;
  arena_estimate_add(mem, indicesz, alignof(u32));

  // 🔹 Finalize estimation
  arena_end_estimate(mem);

  return QK_ERR_SUCCESS;
}

static void load_image(const u8* p, u8* pixels, sz size) {
  u8* indices = (u8*)p;
  for (sz i = 0, j = 0; i < size; i++, j += 4) {
    u32 index = indices[i];
    pixels[j + 0] = _qk_palette[index][0];  // red
    pixels[j + 1] = _qk_palette[index][1];  // green
    pixels[j + 2] = _qk_palette[index][2];  // blue
    pixels[j + 3] = 255;                    // alpha, always opaque
  }
}

static const u8* load_skins(qk_mdl* mdl, const u8* p) {
  const qk_header* hdr = &mdl->header;
  arena* mem = &mdl->mem;
  u32 width = hdr->skin_width;
  u32 height = hdr->skin_height;
  sz skin_size = width * height;

  sz memsz = sizeof(qk_skin) * hdr->skins_count;
  qk_skin* skins = (qk_skin*)arena_alloc(mem, memsz, alignof(qk_skin));
  makesure(skins != NULL, "arena out of memory!");

  for (sz i = 0; i < hdr->skins_count; i++) {
    qk_skintype* skin_type = (qk_skintype*)p;
    if (*skin_type == QK_SKIN_SINGLE) {
      p += sizeof(qk_skintype);

      sz pixelsz = sizeof(u8) * skin_size * 4;
      u8* pixels = (u8*)arena_alloc(mem, pixelsz, alignof(u8));
      makesure(pixels != NULL, "arena out of memory!");

      load_image(p, pixels, skin_size);

#ifdef DEBUG
      char skin_name[1024] = {0};
      sprintf(skin_name, "BUILD/skin_%zu.png", i);
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
                                         .size = (sz)(width * height * 4),
                                     }});
      p += skin_size;
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
  return p;
}

static const u8* load_raw_uv_coordinates(qk_mdl* mdl,
                                         const u8* p,
                                         qk_raw_texcoord** coords) {
  arena* mem = &mdl->mem;
  const qk_header* hdr = &mdl->header;
  sz memsz = sizeof(qk_raw_texcoord) * hdr->vertices_count;
  *coords = (qk_raw_texcoord*)arena_alloc(mem, memsz, alignof(qk_raw_texcoord));
  makesure(coords != NULL, "arena out of memory!");

  const qk_raw_texcoord* src =
      (const qk_raw_texcoord*)p;  // Use a separate pointer

  for (sz i = 0; i < hdr->vertices_count; i++) {
    (*coords)[i].onseam = endian_i32(src->onseam);
    (*coords)[i].s = endian_i32(src->s);
    (*coords)[i].t = endian_i32(src->t);
    src++;  // Move forward correctly
  }

  return (const u8*)src;
}

static const u8* load_raw_triangles_indices(arena* mem,
                                            const u8* p,
                                            const qk_header* hdr,
                                            qk_raw_triangles_idx** trisidx) {
  sz memsz = sizeof(qk_raw_triangles_idx) * hdr->triangles_count;
  *trisidx = (qk_raw_triangles_idx*)arena_alloc(mem, memsz,
                                                alignof(qk_raw_triangles_idx));
  makesure(trisidx != NULL, "arena out of memory!");

  const qk_raw_triangles_idx* src =
      (const qk_raw_triangles_idx*)p;  // Use separate pointer

  for (sz i = 0; i < hdr->triangles_count; i++) {
    (*trisidx)[i].frontface = endian_i32(src->frontface);
    for (sz j = 0; j < 3; j++) {
      (*trisidx)[i].vertices_idx[j] = endian_i32(src->vertices_idx[j]);
    }
    src++;  // Move forward correctly
  }

  return (const u8*)src;
}

static const u8* load_frame_single(const qk_header* hdr,
                                   qk_frame* frame,
                                   u32* posidx,
                                   u32* vertex_offset,
                                   qk_vertex* vertices,
                                   const u8* p) {
  qk_raw_frame_single* snl = (qk_raw_frame_single*)p;

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
  for (u32 i = 0; i < hdr->vertices_count; i++) {
    frame->vertices[i].vertex = (hmm_v3){
        raw_verts[i].vertex[0], raw_verts[i].vertex[1], raw_verts[i].vertex[2]};
    frame->vertices[i].normal =
        (hmm_v3){_qk_normals[raw_verts[i].normal_idx][0],
                 _qk_normals[raw_verts[i].normal_idx][1],
                 _qk_normals[raw_verts[i].normal_idx][2]};
  }

  *vertex_offset += hdr->vertices_count;
  *posidx += 1;
  return (const u8*)(raw_verts + hdr->vertices_count);
}

static const u8* load_frames_group(const qk_header* hdr,
                                   qk_frame* frames,
                                   u32* posidx,
                                   u32* vertex_offset,
                                   qk_vertex* vertices,
                                   const u8* p) {
  qk_raw_frames_group* grp = (qk_raw_frames_group*)p;
  u32 frames_count = endian_i32(grp->frames_count);

  f32* interval_p = (f32*)(grp + 1);
  p = (const u8*)(interval_p + frames_count);

  for (u32 i = 0; i < frames_count; i++) {
    qk_frame* frame = &frames[*posidx];

    frame->bbox_min.X = grp->bbox_min.vertex[0];
    frame->bbox_min.Y = grp->bbox_min.vertex[1];
    frame->bbox_min.Z = grp->bbox_min.vertex[2];

    frame->bbox_max.X = grp->bbox_max.vertex[0];
    frame->bbox_max.Y = grp->bbox_max.vertex[1];
    frame->bbox_max.Z = grp->bbox_max.vertex[2];

    frame->vertices = &vertices[*vertex_offset];
    frame->vertices_count = hdr->vertices_count;

    qk_raw_triangle_vertex* raw_verts = (qk_raw_triangle_vertex*)p;
    for (u32 j = 0; j < hdr->vertices_count; j++) {
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
    p = (const u8*)(raw_verts + hdr->vertices_count);
  }

  return p;
}

static const u8* load_raw_frames(qk_mdl* mdl,
                                 const u8* p,
                                 u32 frames_count,
                                 u32 verts_count) {
  arena* mem = &mdl->mem;
  const qk_header* hdr = &mdl->header;

  sz framesz = sizeof(qk_frame) * frames_count;
  mdl->frames = (qk_frame*)arena_alloc(mem, framesz, alignof(qk_frame));
  makesure(mdl->frames != NULL, "arena out of memory!");

  sz verticesz = sizeof(qk_vertex) * verts_count;
  mdl->vertices = (qk_vertex*)arena_alloc(mem, verticesz, alignof(qk_vertex));
  makesure(mdl->vertices != NULL, "arena out of memory!");

  u32 frame_index = 0, vertex_offset = 0;
  for (u32 i = 0; i < mdl->header.frames_count; i++) {
    qk_frametype ft = endian_i32(*(qk_frametype*)p);
    p += sizeof(qk_frametype);

    if (ft == QK_FT_SINGLE) {
      p = load_frame_single(&mdl->header, &mdl->frames[frame_index],
                              &frame_index, &vertex_offset, mdl->vertices, p);
    } else {
      p = load_frames_group(&mdl->header, mdl->frames, &frame_index,
                              &vertex_offset, mdl->vertices, p);
    }
  }

  return p;
}

static void make_display_lists(arena* mem,
                               qk_mdl* mdl,
                               const qk_raw_texcoord* coords,
                               const qk_raw_triangles_idx* trisidx) {
  sz memsz = sizeof(qk_mesh) * mdl->header.vertices_count;
  qk_mesh* mesh = (qk_mesh*)arena_alloc(mem, memsz, alignof(qk_mesh));
  makesure(mesh != NULL, "arena out of memory!");

  memsz = sizeof(u32) * mdl->header.triangles_count * 3;
  u32* indices = (u32*)arena_alloc(mem, memsz, alignof(u32));
  makesure(indices != NULL, "arena out of memory!");

  u32 mesh_count = mdl->header.vertices_count;
  u32 indices_count = 0;

  for (u32 triidx = 0; triidx < mdl->header.triangles_count; triidx++) {
    for (u8 vertxyz = 0; vertxyz < 3; vertxyz++) {
      i32 vertidx = trisidx[triidx].vertices_idx[vertxyz];

      f32 s = coords[vertidx].s;
      f32 t = coords[vertidx].t;

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

void qk_get_frame_vertices(qk_mdl* mdl,
                           u32 frmidx,
                           f32** verts,
                           u32* verts_cn,
                           hmm_vec3* bbox_min,
                           hmm_vec3* bbox_max) {
  makesure(frmidx < mdl->header.frames_count, "frame index out of bound");

  qk_frame* frm = &mdl->frames[frmidx];
  makesure(frm->vertices != NULL, "frame vertices are NULL");

  *verts = (f32*)malloc(sizeof(f32) * frm->vertices_count *
                        (3 /*vertices*/ + 2 /*UVs*/));
  makesure(*verts != NULL, "malloc failed!");

  *verts_cn = frm->vertices_count * 5;
  for (u32 bi = 0, vi = 0; bi < *verts_cn; bi += 5, vi++) {
    makesure(vi < frm->vertices_count, "vertex index out of bounds!");

    (*verts)[bi + 0] = (frm->vertices[vi].vertex.X * mdl->header.scale.X) +
                       mdl->header.translate.X;
    (*verts)[bi + 1] = (frm->vertices[vi].vertex.Y * mdl->header.scale.Y) +
                       mdl->header.translate.Y;
    (*verts)[bi + 2] = (frm->vertices[vi].vertex.Z * mdl->header.scale.Z) +
                       mdl->header.translate.Z;

    (*verts)[bi + 3] = (mdl->mesh[vi].uv.U);
    (*verts)[bi + 4] = (mdl->mesh[vi].uv.V);
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
}

qk_err qk_load_mdl(const u8* buf, sz bufsz, qk_mdl* mdl) {
  const u8* p = buf;
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
  u32 frames_count, verts_count;
  qk_raw_texcoord* raw_tex_coords = NULL;
  qk_raw_triangles_idx* raw_tris_idx = NULL;

  estimate_memory(mem, rhdr, bufsz, hdr, &frames_count, &verts_count);

  p += sizeof(qk_raw_header);
  p = load_skins(mdl, p);
  p = load_raw_uv_coordinates(mdl, p, &raw_tex_coords);
  p = load_raw_triangles_indices(mem, p, hdr, &raw_tris_idx);
  p = load_raw_frames(mdl, p, frames_count, verts_count);

  make_display_lists(mem, mdl, raw_tex_coords, raw_tris_idx);
  return QK_ERR_SUCCESS;
}

void qk_unload_mdl(qk_mdl* mdl) {
  arena_destroy(&mdl->mem);
}

#endif  // QK_MDL_IMPLEMENTATION
#endif  // QK_MDL_HEADER_
