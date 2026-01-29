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

#include "deps/tracy/tracy.h"
#include "deps/hmm/hmm.h"
#include "deps/nuklear/nuklear.h"
#include "deps/sepi/arena.h"
#include "deps/sepi/endian.h"
#include "deps/sokol/sokol_app.h"
#include "deps/sokol/sokol_gfx.h"
#include "deps/sokol/sokol_nuklear.h"

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

Md1Error md1_load(Md1* md1, IONode* node);
Md1Error md1_get_vertices(const Md1* md1,
                          U32 pose_idx,
                          U32 frame_idx,
                          F32** frame_vbuf,
                          Sz* frame_vertex_buffer_size);
Md1Error md1_get_vertices_v2(Md1* md1,
                             U32 pose_idx,
                             U32 frame_idx,
                             F32** vbuf,
                             Sz* vbuf_size,
                             U32** ibuf,
                             Sz* ibuf_size);
Md1Error md1_unload(Md1* md1);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_MD1_IMPLEMENTATION

#include "data.h"

SLAVE_PROFILING_CONTEXT;

static Nothing
md1_load_image(CBuf ptr, Buf pixels, Sz sz) {
  START_PROFILING(1);

  Assert(pixels != 0);
  Assert(sz > 0);

  Buf indices = (Buf)ptr;
  for (U32 i = 0, j = 0; i < sz; i++, j += 4) {
    U32 index = indices[i];
    pixels[j + 0] = quake1_palette[index][0];  // red
    pixels[j + 1] = quake1_palette[index][1];  // green
    pixels[j + 2] = quake1_palette[index][2];  // blue
    pixels[j + 3] = 255;                       // alpha, always opaque
  }

  END_PROFILING();
}

static Md1Error
md1_load_skins(Md1* md1, IONode* node) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(node != 0);
  Assert(node->buffer.base != 0);
  Assert(node->buffer.size > 0);

  const Md1Details* details = &md1->details;
  const U32 channels = 4;
  Arena* a = md1->arena;
  U32 sw = details->skin_width;
  U32 sh = details->skin_height;
  Sz skin_sz = sw * sh;
  Md1Skin* skins = arena_push_array(a, Md1Skin, details->skins_count);
  Md1Error err = MD1_ERR_SUCCESS;

  AssertAlways(skins != 0);

  for (U32 skin_idx = 0; skin_idx < details->skins_count; skin_idx++) {
    Dbg("skin #%d .P..", skin_idx);

    MD1SkinType* st = (MD1SkinType*)ND_ADDR(&node->buffer);
    ND_MOVE(&node->buffer, sizeof(MD1SkinType));

    if (MD1_SKIN_SINGLE == *st) {
      Sz data_sz = sizeof(U8) * skin_sz * channels;
      Buf data = (Buf)arena_push(a, data_sz, AlignOf(U8), TRUE);
      Assert(data != 0);

      // constructing pixel data
      U8* raw_data = (U8*)ND_ADDR(&node->buffer);
      for (U32 raw_idx = 0, rgba = 0; raw_idx < skin_sz; raw_idx++, rgba += 4) {
        U32 palette_idx = raw_data[raw_idx];
        data[rgba + 0] = quake1_palette[palette_idx][0]; /* RED */
        data[rgba + 1] = quake1_palette[palette_idx][1]; /* GREEN */
        data[rgba + 2] = quake1_palette[palette_idx][2]; /* BLUE */
        data[rgba + 3] = 255; /* ALPHA - always opaque */
      }
      ND_MOVE(&node->buffer, skin_sz);

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
      AssertAlways("NOT IMPLEMENTED!");
    }
  }

  md1->skins = skins;

  END_PROFILING();
  return err;
}

static Md1Error
md1_load_uvs(const Md1* md1, IONode* node, Md1UV** uvs) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(node != 0);
  Assert(node->buffer.base != 0);
  Assert(uvs != 0);

  const Md1Details* details = &md1->details;
  Arena* a = md1->arena;
  Sz uvs_sz = sizeof(Md1UV) * details->vertices_count;
  Md1Error err = MD1_ERR_SUCCESS;

  *uvs = arena_push(a, uvs_sz, AlignOf(Md1UV), TRUE);
  AssertAlways(*uvs != 0);

  for (U32 i = 0; i < details->vertices_count; i++) {
    ND_I32(&node->buffer, &(*uvs)[i].is_on_seam);
    ND_I32(&node->buffer, &(*uvs)[i].u);
    ND_I32(&node->buffer, &(*uvs)[i].v);
  }

  END_PROFILING();
  return err;
}

static Md1Error
md1_load_triangles(Md1* md1, IONode* node, Md1FacedTriangle** fts) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(node != 0);
  Assert(node->buffer.base != 0);
  Assert(fts != 0);

  Md1Details* details = &md1->details;
  Arena* a = md1->arena;
  Sz fts_sz = sizeof(Md1FacedTriangle) * details->triangles_count;
  Sz idices_sz = sizeof(U32) * details->triangles_count * 3;
  Md1Error err = MD1_ERR_SUCCESS;

  *fts = arena_push(a, fts_sz, AlignOf(Md1FacedTriangle), TRUE);
  AssertAlways(*fts != 0);

  for (U32 i = 0, j = 0; i < details->triangles_count; i++, j += 3) {
    I32 a, b, c, f = 0;

    ND_I32(&node->buffer, &f);
    ND_I32(&node->buffer, &a);
    ND_I32(&node->buffer, &b);
    ND_I32(&node->buffer, &c);

    (*fts)[i].is_front_face = f;
    (*fts)[i].vertices_idx[0] = a;
    (*fts)[i].vertices_idx[1] = b;
    (*fts)[i].vertices_idx[2] = c;
  }

  END_PROFILING();
  return err;
}

static Bool
md1_has_pose_name_changed(Str new, CStr old) {
  START_PROFILING(1);

  for (U32 i = 0; i < MD1_MAX_FRAME_NAME_LEN - 1; i++) {
    if (isdigit(new[i])) {
      new[i] = 0;
    }
  }

  if (strlen(old) <= 0) {
    END_PROFILING();
    return TRUE;
  }

  if (strcmp(new, old) != 0) {
    END_PROFILING();
    return TRUE;
  }

  END_PROFILING();
  return FALSE;
}

static Md1Error
md1_load_single_frame(Md1* md1,
                      IONode* node,
                      U32 frame_idx,
                      Str frame_name,
                      Bool* is_bbox_loaded) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(node != 0);
  Assert(node->buffer.base != 0);
  Assert(frame_name != 0);
  Assert(is_bbox_loaded != 0);

  Md1Details* details = &md1->details;
  Md1Error err = MD1_ERR_SUCCESS;
  Sz frame_single_size = sizeof(MD1FrameSingle);
  MD1FrameSingle frame_single = {0};

  memcpy(&frame_single, ND_ADDR(&node->buffer), frame_single_size);
  ND_MOVE(&node->buffer, frame_single_size);

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
    md1->bbox.radius = sqrt((x * x) + (y * y) + (z * z)) / 2.0;

    // bounding box vertex array

    F32 x0 = Min(md1->bbox.min.X, md1->bbox.max.X);
    F32 x1 = Max(md1->bbox.min.X, md1->bbox.max.X);

    F32 y0 = Min(md1->bbox.min.Y, md1->bbox.max.Y);
    F32 y1 = Max(md1->bbox.min.Y, md1->bbox.max.Y);

    F32 z0 = Min(md1->bbox.min.Z, md1->bbox.max.Z);
    F32 z1 = Max(md1->bbox.min.Z, md1->bbox.max.Z);

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

  Md1NormalVertex* nv = (Md1NormalVertex*)ND_ADDR(&node->buffer);
  Sz nv_size = sizeof(Md1NormalVertex);
  Md1Vertex* frame_verts =
      md1->vertices + (details->vertices_count * frame_idx);

  for (U32 i = 0; i < details->vertices_count; i++) {
    Md1NormalVertex nv = {0};
    // NOTE: basically 'Md1NormalVertex' is composed of 4 one byte elements
    //       so we do not need to be worried about endianness and we can
    //       safely copy the memory here!
    memcpy(&nv, ND_ADDR(&node->buffer), nv_size);

    frame_verts[i].vertex.X = nv.vertex[0];
    frame_verts[i].vertex.Y = nv.vertex[1];
    frame_verts[i].vertex.Z = nv.vertex[2];

    // TODO: we do not apply lighing in SQV, so why waste memory for normal
    // data!
    frame_verts[i].normal.X = quake1_normals[nv.normal_idx][0];
    frame_verts[i].normal.Y = quake1_normals[nv.normal_idx][1];
    frame_verts[i].normal.Z = quake1_normals[nv.normal_idx][2];

    ND_MOVE(&node->buffer, sizeof(Md1NormalVertex));
  }

  END_PROFILING();
  return err;
}

static Md1Error
md1_load_frames(Md1* md1, IONode* node) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(node != 0);
  Assert(node->buffer.base != 0);

  Md1Details* details = &md1->details;
  Arena* a = md1->arena;
  U32 pose_frames = 0;
  char frame_name[MD1_MAX_FRAME_NAME_LEN] = {0};
  Bool is_bbox_loaded = FALSE;
  Md1Error err = MD1_ERR_SUCCESS;

  Sz vertices_sz =
      sizeof(Md1Vertex) * details->vertices_count * details->frames_count;
  md1->vertices = arena_push(a, vertices_sz, AlignOf(Md1Vertex), TRUE);
  AssertAlways(md1->vertices != 0);

  // TODO: the pose count is taken from frames count
  //       which is way more that what it has to be, it is safe though
  //       but a waste of memory!

  Sz poses_sz = sizeof(Md1Pose) * details->frames_count; /* duh! */
  md1->poses = arena_push(a, poses_sz, AlignOf(Md1Pose), TRUE);
  AssertAlways(md1->poses != 0);

  details->poses_count = 1;
  md1->poses[0].first_frame = 0;

  for (U32 frame_idx = 0; frame_idx < details->frames_count; frame_idx++) {
    MD1FrameType ft = 0;
    ND_I32(&node->buffer, &ft);

    if (MD1_FT_SINGLE == ft) {
      md1_load_single_frame(md1, node, frame_idx, frame_name, &is_bbox_loaded);
    } else {
      AssertAlways("NOT IMPLEMENTED!");
    }
  }

  END_PROFILING();
  return err;
}

Md1Error
md1_make_display_list(Md1* md1, Md1UV* uvs, Md1FacedTriangle* faced_triangles) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(uvs != 0);
  Assert(faced_triangles != 0);

  Md1Error err = MD1_ERR_SUCCESS;
  Md1Details* details = &md1->details;
  Arena* a = md1->arena;
  Sz elems_count = 3 * (3 + 2);  // a->b->c * x,y,z, u,v
  Sz vbuf_sz = sizeof(F32) * details->frames_count * details->triangles_count *
               elems_count;

  md1->gpu.frame_vertex_buffer_size = elems_count * details->triangles_count;
  md1->gpu.frames_vertex_buffer = arena_push(a, vbuf_sz, AlignOf(F32), TRUE);
  AssertAlways(md1->gpu.frames_vertex_buffer != 0);

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

        u = (u + 0.5) / details->skin_width;
        v = (v + 0.5) / details->skin_height;

        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = x;
        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = y;
        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = z;
        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = u;
        md1->gpu.frames_vertex_buffer[vbuf_vert_idx++] = v;
      }
    }
  }

  END_PROFILING();
  return err;
}

static Md1Error
md1_make_display_list_v2(Md1* md1,
                         Md1UV* uvs,
                         Md1FacedTriangle* faced_triangles) {
  START_PROFILING(1);

  Md1Error err = MD1_ERR_SUCCESS;
  Md1Details* details = &md1->details;
  Arena* a = md1->arena;
  U32 mesh_indices_count = 0;
  U32 mesh_vertices_count = 0;

  // TODO: use scratch buffwe for this
  Md1MeshVertices* mesh_vertices =
      arena_push(a, details->triangles_count * 3 * sizeof(Md1MeshVertices),
                 AlignOf(Md1MeshVertices), TRUE);

  Sz index_buffer_size = details->triangles_count * 3 * sizeof(U32);
  U32* index_buffer = arena_push(a, index_buffer_size, AlignOf(U32), TRUE);

  for (U32 triangle_index = 0; triangle_index < details->triangles_count;
       triangle_index++) {
    U32 mesh_vertex_index = 0;
    for (U32 triangle_vertex_index = 0; triangle_vertex_index < 3;
         triangle_vertex_index++, mesh_indices_count++) {
      U32 vertex_index =
          faced_triangles[triangle_index].vertices_idx[triangle_vertex_index];
      F32 u = uvs[vertex_index].u;
      F32 v = uvs[vertex_index].v;

      if (!faced_triangles[triangle_index].is_front_face &&
          uvs[vertex_index].is_on_seam) {
        u += details->skin_width / 2;
      }

      u = (u + 0.5) / details->skin_width;
      v = (v + 0.5) / details->skin_height;

      for (mesh_vertex_index = 0; mesh_vertex_index < mesh_vertices_count;
           mesh_vertex_index++) {
        if (mesh_vertices[mesh_vertex_index].vertex_index == vertex_index &&
            mesh_vertices[mesh_vertex_index].uv[0] == u &&
            mesh_vertices[mesh_vertex_index].uv[1] == v) {
          index_buffer[mesh_indices_count] = mesh_vertex_index;
          break;
        }
      }

      if (mesh_vertex_index == mesh_vertices_count) {
        index_buffer[mesh_indices_count] = mesh_vertices_count;
        mesh_vertices[mesh_vertices_count].vertex_index = vertex_index;
        mesh_vertices[mesh_vertices_count].uv[0] = u;
        mesh_vertices[mesh_vertices_count].uv[1] = v;
        mesh_vertices_count++;
      }
    }
  }

  U32 vbuf_vert_idx = 0;
  Sz elems_count = (3 + 2);
  Sz vbuf_sz =
      details->frames_count * mesh_vertices_count * elems_count * sizeof(F32);

  md1->gpu.frames_vertex_buffer_v2 = arena_push(a, vbuf_sz, AlignOf(F32), TRUE);

  for (U32 frame_index = 0; frame_index < details->frames_count;
       frame_index++) {
    const Md1Vertex* frame_verts =
        md1->vertices + (details->vertices_count * frame_index);
    for (U32 mesh_vertex_index = 0; mesh_vertex_index < mesh_vertices_count;
         mesh_vertex_index++) {
      U32 vertex_index = mesh_vertices[mesh_vertex_index].vertex_index;
      F32 x = (frame_verts[vertex_index].vertex.X * details->scale.X) +
              details->translate.X;
      F32 y = (frame_verts[vertex_index].vertex.Y * details->scale.Y) +
              details->translate.Y;
      F32 z = (frame_verts[vertex_index].vertex.Z * details->scale.Z) +
              details->translate.Z;
      F32 u = mesh_vertices[mesh_vertex_index].uv[0];
      F32 v = mesh_vertices[mesh_vertex_index].uv[1];

      md1->gpu.frames_vertex_buffer_v2[vbuf_vert_idx++] = x;
      md1->gpu.frames_vertex_buffer_v2[vbuf_vert_idx++] = y;
      md1->gpu.frames_vertex_buffer_v2[vbuf_vert_idx++] = z;
      md1->gpu.frames_vertex_buffer_v2[vbuf_vert_idx++] = u;
      md1->gpu.frames_vertex_buffer_v2[vbuf_vert_idx++] = v;
    }
  }

  md1->gpu.frame_vertex_buffer_size =
      elems_count * mesh_vertices_count * sizeof(F32);
  md1->gpu.index_buffer_size = index_buffer_size;
  md1->gpu.index_buffer = index_buffer;

  END_PROFILING();
  return err;
}

Md1Error
md1_load(Md1* md1, IONode* node) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(node != 0);
  Assert(node->buffer.base != 0);
  Assert(md1->arena == 0);

  Md1Error err = MD1_ERR_SUCCESS;
  MemZero(md1, sizeof(Md1));
  Md1RawHeader rh = {0};

  ND_I32(&node->buffer, &rh.magic_code);
  ND_I32(&node->buffer, &rh.version);
  ND_F32(&node->buffer, &rh.scale[0]);
  ND_F32(&node->buffer, &rh.scale[1]);
  ND_F32(&node->buffer, &rh.scale[2]);
  ND_F32(&node->buffer, &rh.translate[0]);
  ND_F32(&node->buffer, &rh.translate[1]);
  ND_F32(&node->buffer, &rh.translate[2]);
  ND_F32(&node->buffer, &rh.bounding_radius);
  ND_F32(&node->buffer, &rh.eye_position[0]);
  ND_F32(&node->buffer, &rh.eye_position[1]);
  ND_F32(&node->buffer, &rh.eye_position[2]);
  ND_I32(&node->buffer, &rh.skins_count);
  ND_I32(&node->buffer, &rh.skin_width);
  ND_I32(&node->buffer, &rh.skin_height);
  ND_I32(&node->buffer, &rh.vertices_count);
  ND_I32(&node->buffer, &rh.triangles_count);
  ND_I32(&node->buffer, &rh.frames_count);
  ND_I32(&node->buffer, &rh.sync_type);
  ND_I32(&node->buffer, &rh.flags);
  ND_F32(&node->buffer, &rh.size);

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

  AssertAlways(details->radius > 0);
  AssertAlways(details->skin_width > 0);
  AssertAlways(details->skin_height > 0);
  AssertAlways(details->skins_count > 0);
  AssertAlways(details->vertices_count > 0);
  AssertAlways(details->triangles_count > 0);
  AssertAlways(details->frames_count > 0);

  // TODO: set some initial params
  md1->arena = arena_create();

  err = md1_load_skins(md1, node);
  if (err != MD1_ERR_SUCCESS) {
    END_PROFILING();
    return err;
  }

  Md1UV* uvs = 0;
  err = md1_load_uvs(md1, node, &uvs);
  if (err != MD1_ERR_SUCCESS) {
    END_PROFILING();
    return err;
  }
  Assert(uvs != 0);

  Md1FacedTriangle* faced_triangles = NULL;
  err = md1_load_triangles(md1, node, &faced_triangles);
  if (err != MD1_ERR_SUCCESS) {
    END_PROFILING();
    return err;
  }
  Assert(faced_triangles != 0);

  err = md1_load_frames(md1, node);
  if (err != MD1_ERR_SUCCESS) {
    END_PROFILING();
    return err;
  }

  err = md1_make_display_list(md1, uvs, faced_triangles);
  if (err != MD1_ERR_SUCCESS) {
    END_PROFILING();
    return err;
  }

  // NOTE: this has some bugs!
  // err = md1_make_display_list_v2(md1, uvs, faced_triangles);
  // if (err != MD1_ERR_SUCCESS)
  //   return err;

  Dbg("details.vertices: %d", details->vertices_count);
  Dbg("details.triangles: %d", details->triangles_count);
  Dbg("details.frames: %d", details->frames_count);
  Dbg("details.skins: %d", details->skins_count);
  Dbg("details.skin size: %d x %d", details->skin_width, details->skin_height);

  END_PROFILING();
  return err;
}

Md1Error
md1_get_vertices(const Md1* md1,
                 U32 pose_idx,
                 U32 pose_frame_idx,
                 F32** frame_vbuf,
                 Sz* frame_vertex_buffer_size) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(frame_vbuf != 0);
  Assert(frame_vertex_buffer_size != 0);
  Assert(pose_idx < md1->details.poses_count);
  Assert(pose_frame_idx < md1->poses[pose_idx].frames_count);

  Md1Error err = MD1_ERR_SUCCESS;
  Md1Pose* pose = &md1->poses[pose_idx];
  *frame_vbuf =
      &md1->gpu.frames_vertex_buffer[(pose->first_frame + pose_frame_idx) *
                                     md1->gpu.frame_vertex_buffer_size];
  *frame_vertex_buffer_size = md1->gpu.frame_vertex_buffer_size * sizeof(F32);

  END_PROFILING();
  return err;
}

Md1Error
md1_get_vertices_v2(Md1* md1,
                    U32 pose_idx,
                    U32 frame_idx,
                    F32** vbuf,
                    Sz* vbuf_size,
                    U32** ibuf,
                    Sz* ibuf_size) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(vbuf != 0);
  Assert(vbuf_size != 0);
  Assert(ibuf != 0);
  Assert(ibuf_size != 0);
  Assert(pose_idx < md1->details.poses_count);
  Assert(frame_idx < md1->poses[pose_idx].frames_count);

  Md1Error err = MD1_ERR_SUCCESS;
  Md1Details* details = &md1->details;
  Md1Pose* pose = &md1->poses[pose_idx];
  U32 vbuf_loc =
      (pose->first_frame + frame_idx) * md1->gpu.frame_vertex_buffer_size;

  *vbuf = &md1->gpu.frames_vertex_buffer_v2[vbuf_loc];
  *vbuf_size = md1->gpu.frame_vertex_buffer_size;
  *ibuf = md1->gpu.index_buffer;
  *ibuf_size = md1->gpu.index_buffer_size;

  END_PROFILING();
  return err;
}

Md1Error
md1_unload(Md1* md1) {
  START_PROFILING(1);

  Assert(md1 != 0);
  Assert(md1->arena != 0);
  arena_destroy(md1->arena);

  END_PROFILING();
  return MD1_ERR_SUCCESS;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_MD1_IMPLEMENTATION
#endif  // MODULE_MD1_HEADER
