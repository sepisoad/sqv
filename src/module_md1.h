/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#ifndef MODULE_MD1_HEADER
#define MODULE_MD1_HEADER

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <ctype.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <deps/tracy/tracy.h>
#include <deps/hmm/hmm.h>
#include <deps/nuklear/nuklear.h>
#include <deps/sokol/sokol_app.h>
#include <deps/sokol/sokol_gfx.h>
#include <deps/sokol/sokol_nuklear.h>

#include <deps/sepi/arena.h>
#include <deps/sepi/endian.h>
#include <deps/sepi/io.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define MD1_MAGIC_CODE (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MD1_VERSION 6
#define MD1_MAX_SKIN_HEIGHT 480
#define MD1_MAX_VERTICES 2000
#define MD1_MAX_TRIANGLES 4096
#define MD1_MAX_SKINS 32
#define MD1_MAX_FRAME_NAME_LEN 16
#define MD1_BBOX_VERTEX_COUNT 24

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef F32 MD1RawVector3F[3];
typedef I32 MD1RawTriangle[3];
typedef U8 MD1RawVertex[3];

typedef enum {
  MD1_ERR_SUCCESS = 1,
  MD1_ERR_FILE_OPEN,
  MD1_ERR_MEM_ALLOC,
  MD1_ERR_READ_SIZE,
  MD1_ERR_RAME_IDX,
  MD1_ERR_INVALID,
  MD1_ERR__COUNT,
} Md1Error;

typedef I32 MD1SyncType;
enum {
  MD1_SYNC_SYNC,
  MD1_SYNC_RAND,
  MD1_SYNC_FRAMETIME,
};

typedef I32 MD1SkinType;
enum {
  MD1_SKIN_SINGLE,
  MD1_SKIN_GROUP,
};

typedef I32 MD1FrameType;
enum {
  MD1_FT_SINGLE,
  MD1_FT_GROUP,
};

typedef struct {
  I32 magic_code;
  I32 version;
  MD1RawVector3F scale;
  MD1RawVector3F translate;
  F32 bounding_radius;
  MD1RawVector3F eye_position;
  I32 skins_count;
  I32 skin_width;
  I32 skin_height;
  I32 vertices_count;
  I32 triangles_count;
  I32 frames_count;
  MD1SyncType sync_type;
  I32 flags;
  F32 size;
} Md1RawHeader;

typedef struct {
  I32 is_on_seam;
  I32 u;
  I32 v;
} Md1UV;

typedef struct {
  I32 is_front_face;
  MD1RawTriangle vertices_idx;
} Md1FacedTriangle;

typedef struct {
  MD1RawVertex vertex;
  U8 normal_idx;
} Md1NormalVertex;

typedef struct {
  Md1NormalVertex bbox_min;
  Md1NormalVertex bbox_max;
  char name[16];
} MD1FrameSingle;

typedef struct {
  I32 frames_count;
  Md1NormalVertex bbox_min;
  Md1NormalVertex bbox_max;
} MD1FramesGroup;

/* processed */

typedef struct {
  F32 uv[2];
  U32 vertex_index;
} Md1MeshVertices;

typedef struct {
  hmm_v3 min;
  hmm_v3 max;
  hmm_v3 center;
  F32 radius;
} Md1BBox;

typedef struct {
  F32 radius;
  U32 skin_width;
  U32 skin_height;
  U32 skins_count;
  U32 vertices_count;
  U32 indices_count;
  U32 triangles_count;
  U32 frames_count;
  U32 poses_count;
  hmm_v3 scale;
  hmm_v3 translate;
  hmm_v3 eye;
} Md1Details;

typedef struct {
  hmm_v3 vertex;
  /* TODO: we do not apply lighing in SQV,
     so why waste memory for normal data! */
  hmm_v3 normal;
} Md1Vertex;

typedef struct {
  char name[MD1_MAX_FRAME_NAME_LEN];
  U32 first_frame;
  U32 frames_count;
} Md1Pose;

typedef struct {
  sg_image image;
  sg_image depth;
  sg_view view;
  sg_sampler sampler;
} Md1Skin;

typedef struct {
  F32* frames_vertex_buffer;
  F32* frames_vertex_buffer_v2;
  Sz frame_vertex_buffer_size;
  U32* index_buffer;
  Sz index_buffer_size;
  F32 bbox_vertex_buffer[MD1_BBOX_VERTEX_COUNT * 3];
  Sz bbox_vertex_buffer_size;
} Md1GPUBuffers;

typedef struct {
  Md1Details details;
  Md1Skin* skins;
  Md1Vertex* vertices;
  Md1Pose* poses;
  Md1BBox bbox;
  Md1GPUBuffers gpu;
  Arena* arena;
} Md1;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

fn Md1Error md1_load(Md1* md1, IOFile* io_file);
fn Md1Error md1_get_vertices(const Md1* md1,
                          U32 pose_idx,
                          U32 frame_idx,
                          F32** frame_vbuf,
                          Sz* frame_vertex_buffer_size);
fn Md1Error md1_get_vertices_v2(Md1* md1,
                             U32 pose_idx,
                             U32 frame_idx,
                             F32** vbuf,
                             Sz* vbuf_size,
                             U32** ibuf,
                             Sz* ibuf_size);
fn Md1Error md1_unload(Md1* md1);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_MD1_IMPLEMENTATION

#include "data_quake.h"

mount_slave_profiling_context();

local fn Md1Error
md1_load_skins(Md1* md1, IOFile* io_file) {
  start_profiling(1);

  assert(md1 != 0);
  assert(io_file != 0);
  assert(io_file->buffer.base != 0);
  assert(io_file->buffer.size > 0);

  const Md1Details* details = &md1->details;
  const U32 channels = 4;
  Arena* a = md1->arena;
  U32 sw = details->skin_width;
  U32 sh = details->skin_height;
  Sz skin_sz = sw * sh;
  Md1Skin* skins = arena_push_array(a, Md1Skin, details->skins_count);
  Md1Error err = MD1_ERR_SUCCESS;

  runtime_assert(skins != 0);

  for (U32 skin_idx = 0; skin_idx < details->skins_count; skin_idx++) {
    const MD1SkinType* st = (MD1SkinType*)buf_get_position(io_file->buffer);
    buf_move_offset(io_file->buffer, sizeof(MD1SkinType));

    if (MD1_SKIN_SINGLE == *st) {
      Sz data_sz = sizeof(U8) * skin_sz * channels;
      U8* data = arena_push(a, data_sz, alignof(U8), TRUE);
      assert(data != 0);

      // constructing pixel data
      const U8* const raw_data = (U8*)buf_get_position(io_file->buffer);
      for (U32 raw_idx = 0, rgba = 0; raw_idx < skin_sz; raw_idx++, rgba += 4) {
        U32 palette_idx = raw_data[raw_idx];
        data[rgba + 0] = quake1_palette[palette_idx][0]; /* RED */
        data[rgba + 1] = quake1_palette[palette_idx][1]; /* GREEN */
        data[rgba + 2] = quake1_palette[palette_idx][2]; /* BLUE */
        data[rgba + 3] = 255; /* ALPHA - always opaque */
      }
      buf_move_offset(io_file->buffer, skin_sz);

      // allocating texture data
      skins[skin_idx].image = sg_make_image(&(sg_image_desc){
          .usage = {.immutable = TRUE},
          .width = sw,
          .height = sh,
          .pixel_format = SG_PIXELFORMAT_RGBA8,
          .sample_count = 1,
          .data.mip_levels[0] = {.ptr = data, .size = data_sz},
      });
      skins[skin_idx].depth = sg_make_image(&(sg_image_desc){
          .usage = {.depth_stencil_attachment = TRUE},
          .width = sw,
          .height = sh,
          .pixel_format = SG_PIXELFORMAT_DEPTH,
          .sample_count = 1,
      });
      skins[skin_idx].sampler = sg_make_sampler(&(sg_sampler_desc){
          .min_filter = SG_FILTER_LINEAR,
          .mag_filter = SG_FILTER_LINEAR,
          .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
          .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
      });
      skins[skin_idx].view = sg_make_view(&(sg_view_desc){
          .texture = {.image = skins[skin_idx].image},
      });
    } else {
      not_implemented();
    }
  }

  md1->skins = skins;

  end_profiling();
  return err;
}

local fn Md1Error
md1_load_uvs(const Md1* md1, IOFile* io_file, Md1UV** uvs) {
  start_profiling(1);

  assert(md1 != 0);
  assert(io_file != 0);
  assert(io_file->buffer.base != 0);
  assert(uvs != 0);

  const Md1Details* details = &md1->details;
  Arena* a = md1->arena;
  Sz uvs_sz = sizeof(Md1UV) * details->vertices_count;
  Md1Error err = MD1_ERR_SUCCESS;

  *uvs = arena_push(a, uvs_sz, alignof(Md1UV), TRUE);
  runtime_assert(*uvs != 0);

  for (U32 i = 0; i < details->vertices_count; i++) {
    buf_read_i32(io_file->buffer, &(*uvs)[i].is_on_seam);
    buf_read_i32(io_file->buffer, &(*uvs)[i].u);
    buf_read_i32(io_file->buffer, &(*uvs)[i].v);
  }

  end_profiling();
  return err;
}

local fn Md1Error
md1_load_triangles(Md1* md1, IOFile* io_file, Md1FacedTriangle** fts) {
  start_profiling(1);

  assert(md1 != 0);
  assert(io_file != 0);
  assert(io_file->buffer.base != 0);
  assert(fts != 0);

  const Md1Details* const details = &md1->details;
  Arena* arena = md1->arena;
  Sz fts_sz = sizeof(Md1FacedTriangle) * details->triangles_count;
  // Sz idices_sz = sizeof(U32) * details->triangles_count * 3;
  Md1Error err = MD1_ERR_SUCCESS;

  *fts = arena_push(arena, fts_sz, alignof(Md1FacedTriangle), TRUE);
  runtime_assert(*fts != 0);

  for (U32 i = 0, j = 0; i < details->triangles_count; i++, j += 3) {
    I32 a, b, c, f = 0;

    buf_read_i32(io_file->buffer, &f);
    buf_read_i32(io_file->buffer, &a);
    buf_read_i32(io_file->buffer, &b);
    buf_read_i32(io_file->buffer, &c);

    (*fts)[i].is_front_face = f;
    (*fts)[i].vertices_idx[0] = a;
    (*fts)[i].vertices_idx[1] = b;
    (*fts)[i].vertices_idx[2] = c;
  }

  end_profiling();
  return err;
}

local fn Bool
md1_has_pose_name_changed(ZStr new, CZStr old) {
  start_profiling(1);

  for (U32 i = 0; i < MD1_MAX_FRAME_NAME_LEN - 1; i++) {
    if (isdigit(new[i])) {
      new[i] = 0;
    }
  }

  if (strlen(old) == 0) {
    end_profiling();
    return TRUE;
  }

  if (strcmp(new, old) != 0) {
    end_profiling();
    return TRUE;
  }

  end_profiling();
  return FALSE;
}

local fn Md1Error
md1_load_single_frame(Md1* md1,
                      IOFile* io_file,
                      U32 frame_idx,
                      ZStr frame_name,
                      Bool* is_bbox_loaded) {
  start_profiling(1);

  assert(md1 != 0);
  assert(io_file != 0);
  assert(io_file->buffer.base != 0);
  assert(frame_name != 0);
  assert(is_bbox_loaded != 0);

  Md1Details* details = &md1->details;
  Md1Error err = MD1_ERR_SUCCESS;
  Sz frame_single_size = sizeof(MD1FrameSingle);
  MD1FrameSingle frame_single = {0};

  memcpy(&frame_single, buf_get_position(io_file->buffer), frame_single_size);
  buf_move_offset(io_file->buffer, frame_single_size);

  if (md1_has_pose_name_changed(frame_single.name, frame_name) &&
      frame_idx > 0) {
    memcpy(md1->poses[details->poses_count - 1].name, frame_single.name,
           MD1_MAX_FRAME_NAME_LEN);
    details->poses_count += 1;
    md1->poses[details->poses_count - 1].first_frame = frame_idx;
  }

  md1->poses[details->poses_count - 1].frames_count++;
  memcpy((RawPtr)frame_name, frame_single.name, MD1_MAX_FRAME_NAME_LEN);
  frame_name[MD1_MAX_FRAME_NAME_LEN - 1] = 0;

  if (*is_bbox_loaded != TRUE) {
    *is_bbox_loaded = TRUE;

    // copy bbox min
    md1->bbox.min.X = (frame_single.bbox_min.vertex[0] * md1->details.scale.X) +
                      md1->details.translate.X;
    md1->bbox.min.Y = (frame_single.bbox_min.vertex[1] * md1->details.scale.Y) +
                      md1->details.translate.Y;
    md1->bbox.min.Z = (frame_single.bbox_min.vertex[2] * md1->details.scale.Z) +
                      md1->details.translate.Z;

    // copy bbox max
    md1->bbox.max.X = (frame_single.bbox_max.vertex[0] * md1->details.scale.X) +
                      md1->details.translate.X;
    md1->bbox.max.Y = (frame_single.bbox_max.vertex[1] * md1->details.scale.Y) +
                      md1->details.translate.Y;
    md1->bbox.max.Z = (frame_single.bbox_max.vertex[2] * md1->details.scale.Z) +
                      md1->details.translate.Z;

    // calc mesh center
    md1->bbox.center.X = (md1->bbox.min.X + md1->bbox.max.X) / 2;
    md1->bbox.center.Y = (md1->bbox.min.Y + md1->bbox.max.Y) / 2;
    md1->bbox.center.Z = (md1->bbox.min.Z + md1->bbox.max.Z) / 2;

    // calc mesh dimention
    F32 x = (md1->bbox.max.X - md1->bbox.min.X);
    F32 y = (md1->bbox.max.Y - md1->bbox.min.Y);
    F32 z = (md1->bbox.max.Z - md1->bbox.min.Z);

    // calc bounding sphere radius
    md1->bbox.radius = sqrtf((x * x) + (y * y) + (z * z)) / 2.0f;

    // bounding box vertex array

    F32 x0 = min(md1->bbox.min.X, md1->bbox.max.X);
    F32 x1 = max(md1->bbox.min.X, md1->bbox.max.X);

    F32 y0 = min(md1->bbox.min.Y, md1->bbox.max.Y);
    F32 y1 = max(md1->bbox.min.Y, md1->bbox.max.Y);

    F32 z0 = min(md1->bbox.min.Z, md1->bbox.max.Z);
    F32 z1 = max(md1->bbox.min.Z, md1->bbox.max.Z);

    // corners

    hmm_vec3 v0 = HMM_Vec3(x0, y0, z0);  // back-bottom-left
    hmm_vec3 v1 = HMM_Vec3(x1, y0, z0);  // back-bottom-right
    hmm_vec3 v2 = HMM_Vec3(x1, y1, z0);  // back-top-right
    hmm_vec3 v3 = HMM_Vec3(x0, y1, z0);  // back-top-left

    hmm_vec3 v4 = HMM_Vec3(x0, y0, z1);  // front-bottom-left
    hmm_vec3 v5 = HMM_Vec3(x1, y0, z1);  // front-bottom-right
    hmm_vec3 v6 = HMM_Vec3(x1, y1, z1);  // front-top-right
    hmm_vec3 v7 = HMM_Vec3(x0, y1, z1);  // front-top-left

    // 12 edges, each as (from, to)
    F32 verts[MD1_BBOX_VERTEX_COUNT][3] = {
        // back face
        {v0.X, v0.Y, v0.Z},
        {v1.X, v1.Y, v1.Z},
        {v1.X, v1.Y, v1.Z},
        {v2.X, v2.Y, v2.Z},
        {v2.X, v2.Y, v2.Z},
        {v3.X, v3.Y, v3.Z},
        {v3.X, v3.Y, v3.Z},
        {v0.X, v0.Y, v0.Z},

        // front face
        {v4.X, v4.Y, v4.Z},
        {v5.X, v5.Y, v5.Z},
        {v5.X, v5.Y, v5.Z},
        {v6.X, v6.Y, v6.Z},
        {v6.X, v6.Y, v6.Z},
        {v7.X, v7.Y, v7.Z},
        {v7.X, v7.Y, v7.Z},
        {v4.X, v4.Y, v4.Z},

        // side edges
        {v0.X, v0.Y, v0.Z},
        {v4.X, v4.Y, v4.Z},
        {v1.X, v1.Y, v1.Z},
        {v5.X, v5.Y, v5.Z},
        {v2.X, v2.Y, v2.Z},
        {v6.X, v6.Y, v6.Z},
        {v3.X, v3.Y, v3.Z},
        {v7.X, v7.Y, v7.Z},
    };

    Sz size = sizeof(F32) * MD1_BBOX_VERTEX_COUNT * 3;
    memcpy(md1->gpu.bbox_vertex_buffer, verts, size);
    md1->gpu.bbox_vertex_buffer_size = size;
  }

  // Md1NormalVertex* nv = (Md1NormalVertex*)buf_get_position(io_file->buffer);

  Md1Vertex* frame_verts =
      md1->vertices + (details->vertices_count * frame_idx);

  for (U32 i = 0; i < details->vertices_count; i++) {
    Md1NormalVertex nv = {0};
    Sz nv_size = sizeof(Md1NormalVertex);

    // NOTE: basically 'Md1NormalVertex' is composed of 4 one byte elements
    //       so we do not need to be worried about endianness and we can
    //       safely copy the memory here!
    memcpy(&nv, buf_get_position(io_file->buffer), nv_size);

    frame_verts[i].vertex.X = nv.vertex[0];
    frame_verts[i].vertex.Y = nv.vertex[1];
    frame_verts[i].vertex.Z = nv.vertex[2];

    // TODO: we do not apply lighing in SQV, so why waste memory for normal
    // data!
    frame_verts[i].normal.X = quake1_normals[nv.normal_idx][0];
    frame_verts[i].normal.Y = quake1_normals[nv.normal_idx][1];
    frame_verts[i].normal.Z = quake1_normals[nv.normal_idx][2];

    buf_move_offset(io_file->buffer, sizeof(Md1NormalVertex));
  }

  end_profiling();
  return err;
}

local fn Md1Error
md1_load_frames(Md1* md1, IOFile* io_file) {
  start_profiling(1);

  assert(md1 != 0);
  assert(io_file != 0);
  assert(io_file->buffer.base != 0);

  Md1Details* details = &md1->details;
  Arena* a = md1->arena;
  // U32 pose_frames = 0;
  char frame_name[MD1_MAX_FRAME_NAME_LEN] = {0};
  Bool is_bbox_loaded = FALSE;
  Md1Error err = MD1_ERR_SUCCESS;

  Sz vertices_sz =
      sizeof(Md1Vertex) * details->vertices_count * details->frames_count;
  md1->vertices = arena_push(a, vertices_sz, alignof(Md1Vertex), TRUE);
  runtime_assert(md1->vertices != 0);

  // TODO: the pose count is taken from frames count
  //       which is way more that what it has to be, it is safe though
  //       but a waste of memory!

  Sz poses_sz = sizeof(Md1Pose) * details->frames_count; /* duh! */
  md1->poses = arena_push(a, poses_sz, alignof(Md1Pose), TRUE);
  runtime_assert(md1->poses != 0);

  details->poses_count = 1;
  md1->poses[0].first_frame = 0;

  for (U32 frame_idx = 0; frame_idx < details->frames_count; frame_idx++) {
    MD1FrameType ft = 0;
    buf_read_i32(io_file->buffer, &ft);

    if (MD1_FT_SINGLE == ft) {
      md1_load_single_frame(md1, io_file, frame_idx, frame_name,
                            &is_bbox_loaded);
    } else {
      not_implemented();
    }
  }

  end_profiling();
  return err;
}

local fn Md1Error
md1_make_display_list(Md1* md1, Md1UV* uvs, Md1FacedTriangle* faced_triangles) {
  start_profiling(1);

  assert(md1 != 0);
  assert(uvs != 0);
  assert(faced_triangles != 0);

  Md1Error err = MD1_ERR_SUCCESS;
  Md1Details* details = &md1->details;
  Arena* a = md1->arena;
  Sz elems_count = 3 * (3 + 2);  // a->b->c * x,y,z, u,v
  Sz vbuf_sz = sizeof(F32) * details->frames_count * details->triangles_count *
               elems_count;

  md1->gpu.frame_vertex_buffer_size = elems_count * details->triangles_count;
  md1->gpu.frames_vertex_buffer = arena_push(a, vbuf_sz, alignof(F32), TRUE);
  runtime_assert(md1->gpu.frames_vertex_buffer != 0);

  U32 vbuf_vert_idx = 0;
  for (U32 frm_idx = 0; frm_idx < details->frames_count; frm_idx++) {
    for (U32 triangle_index = 0; triangle_index < details->triangles_count;
         triangle_index++) {
      for (U32 tri_xyz = 0; tri_xyz < 3; tri_xyz++) {
        const Md1Vertex* frame_verts =
            md1->vertices + (details->vertices_count * frm_idx);
        I32 xyz = faced_triangles[triangle_index].vertices_idx[tri_xyz];
        F32 x = (frame_verts[xyz].vertex.X * details->scale.X) +
                details->translate.X;
        F32 y = (frame_verts[xyz].vertex.Y * details->scale.Y) +
                details->translate.Y;
        F32 z = (frame_verts[xyz].vertex.Z * details->scale.Z) +
                details->translate.Z;
        F32 u = uvs[xyz].u;
        F32 v = uvs[xyz].v;

        if (!faced_triangles[triangle_index].is_front_face &&
            uvs[xyz].is_on_seam) {
          u += details->skin_width / 2;
        }

        u = (u + 0.5f) / details->skin_width;
        v = (v + 0.5f) / details->skin_height;

        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = x;
        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = y;
        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = z;
        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = u;
        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = v;
      }
    }
  }

  end_profiling();
  return err;
}

Md1Error
md1_load(Md1* md1, IOFile* io_file) {
  start_profiling(1);

  assert(md1 != 0);
  assert(io_file != 0);
  assert(io_file->buffer.base != 0);
  assert(md1->arena == 0);

  Md1Error err = MD1_ERR_SUCCESS;
  zero_memory(md1, sizeof(Md1));
  Md1RawHeader rh = {0};

  buf_read_i32(io_file->buffer, &rh.magic_code);
  buf_read_i32(io_file->buffer, &rh.version);
  buf_read_f32(io_file->buffer, &rh.scale[0]);
  buf_read_f32(io_file->buffer, &rh.scale[1]);
  buf_read_f32(io_file->buffer, &rh.scale[2]);
  buf_read_f32(io_file->buffer, &rh.translate[0]);
  buf_read_f32(io_file->buffer, &rh.translate[1]);
  buf_read_f32(io_file->buffer, &rh.translate[2]);
  buf_read_f32(io_file->buffer, &rh.bounding_radius);
  buf_read_f32(io_file->buffer, &rh.eye_position[0]);
  buf_read_f32(io_file->buffer, &rh.eye_position[1]);
  buf_read_f32(io_file->buffer, &rh.eye_position[2]);
  buf_read_i32(io_file->buffer, &rh.skins_count);
  buf_read_i32(io_file->buffer, &rh.skin_width);
  buf_read_i32(io_file->buffer, &rh.skin_height);
  buf_read_i32(io_file->buffer, &rh.vertices_count);
  buf_read_i32(io_file->buffer, &rh.triangles_count);
  buf_read_i32(io_file->buffer, &rh.frames_count);
  buf_read_i32(io_file->buffer, &rh.sync_type);
  buf_read_i32(io_file->buffer, &rh.flags);
  buf_read_f32(io_file->buffer, &rh.size);

  Md1Details* details = &md1->details;

  details->radius = rh.bounding_radius;
  details->skin_width = rh.skin_width;
  details->skin_height = rh.skin_height;
  details->skins_count = rh.skins_count;
  details->vertices_count = rh.vertices_count;
  details->triangles_count = rh.triangles_count;
  details->frames_count = rh.frames_count;
  details->scale.X = rh.scale[0];
  details->scale.Y = rh.scale[1];
  details->scale.Z = rh.scale[2];
  details->translate.X = rh.translate[0];
  details->translate.Y = rh.translate[1];
  details->translate.Z = rh.translate[2];
  details->eye.X = rh.eye_position[0];
  details->eye.Y = rh.eye_position[1];
  details->eye.Z = rh.eye_position[2];

  runtime_assert(details->radius > 0);
  runtime_assert(details->skin_width > 0);
  runtime_assert(details->skin_height > 0);
  runtime_assert(details->skins_count > 0);
  runtime_assert(details->vertices_count > 0);
  runtime_assert(details->triangles_count > 0);
  runtime_assert(details->frames_count > 0);

  // TODO: set some initial params
  md1->arena = arena_create();

  err = md1_load_skins(md1, io_file);
  if (err != MD1_ERR_SUCCESS) {
    end_profiling();
    return err;
  }

  Md1UV* uvs = 0;
  err = md1_load_uvs(md1, io_file, &uvs);
  if (err != MD1_ERR_SUCCESS) {
    end_profiling();
    return err;
  }
  assert(uvs != 0);

  Md1FacedTriangle* faced_triangles = NULL;
  err = md1_load_triangles(md1, io_file, &faced_triangles);
  if (err != MD1_ERR_SUCCESS) {
    end_profiling();
    return err;
  }
  assert(faced_triangles != 0);

  err = md1_load_frames(md1, io_file);
  if (err != MD1_ERR_SUCCESS) {
    end_profiling();
    return err;
  }

  err = md1_make_display_list(md1, uvs, faced_triangles);
  if (err != MD1_ERR_SUCCESS) {
    end_profiling();
    return err;
  }

  end_profiling();
  return err;
}

Md1Error
md1_get_vertices(const Md1* md1,
                 U32 pose_idx,
                 U32 pose_frame_idx,
                 F32** frame_vbuf,
                 Sz* frame_vertex_buffer_size) {
  start_profiling(1);

  assert(md1 != 0);
  assert(frame_vbuf != 0);
  assert(frame_vertex_buffer_size != 0);
  assert(pose_idx < md1->details.poses_count);
  assert(pose_frame_idx < md1->poses[pose_idx].frames_count);

  Md1Error err = MD1_ERR_SUCCESS;
  const Md1Pose* const pose = &md1->poses[pose_idx];
  *frame_vbuf =
      &md1->gpu.frames_vertex_buffer[(pose->first_frame + pose_frame_idx) *
                                     md1->gpu.frame_vertex_buffer_size];
  *frame_vertex_buffer_size = md1->gpu.frame_vertex_buffer_size * sizeof(F32);

  end_profiling();
  return err;
}

Md1Error
md1_unload(Md1* md1) {
  start_profiling(1);

  assert(md1 != 0);
  if (md1->arena) {
    arena_destroy(md1->arena);
  }

  end_profiling();
  return MD1_ERR_SUCCESS;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_MD1_IMPLEMENTATION
#endif  // MODULE_MD1_HEADER
