#ifndef MD1_HEADER_
#define MD1_HEADER_

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

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef F32 MD1RawVector3F[3];
typedef I32 MD1RawTriangle[3];
typedef U8  MD1RawVertex[3];

typedef enum {
  MD1_ERR_UNKNOWN,
  MD1_ERR_SUCCESS,
  MD1_ERR_FILE_OPEN,
  MD1_ERR_MEM_ALLOC,
  MD1_ERR_READ_SIZE,
  MD1_ERR_RAME_IDX,
  MD1_ERR_INVALID,
  MD1_ERR__COUNT,
} MD1Error;

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
  I32            magic_code;
  I32            version;
  MD1RawVector3F scale;
  MD1RawVector3F translate;
  F32            bounding_radius;
  MD1RawVector3F eye_position;
  I32            skins_count;
  I32            skin_width;
  I32            skin_height;
  I32            vertices_count;
  I32            triangles_count;
  I32            frames_count;
  MD1SyncType    sync_type;
  I32            flags;
  F32            size;
} MD1RawHeader;

typedef struct {
  I32 is_on_seam;
  I32 u;
  I32 v;
} MD1UV;

typedef struct {
  I32            is_front_face;
  MD1RawTriangle vertices_idx;
} MD1FacedTriangle;

typedef struct {
  MD1RawVertex vertex;
  U8           normal_idx;
} MD1NormalVertex;

typedef struct {
  MD1NormalVertex bbox_min;
  MD1NormalVertex bbox_max;
  char            name[16];
} MD1FrameSingle;

typedef struct {
  I32             frames_count;
  MD1NormalVertex bbox_min;
  MD1NormalVertex bbox_max;
} MD1FramesGroup;

/* processed */

typedef struct {
  F32    radius;
  U32    skin_width;
  U32    skin_height;
  U32    skins_count;
  U32    vertices_count;
  U32    triangles_count;
  U32    frames_count;
  U32    poses_count;
  U32    frame_vbuf_size;
  hmm_v3 scale;
  hmm_v3 translate;
  hmm_v3 eye;
  hmm_v3 bbox_min;
  hmm_v3 bbox_max;
} MD1Header;

typedef struct {
  hmm_v3 vertex;
  hmm_v3 normal;
} MD1Vertex;

typedef struct {
  char name[MD1_MAX_FRAME_NAME_LEN];
  U32  first_frame;
  U32  frames_count;
} MD1Pose;

typedef struct {
  sg_image   image;
  sg_image   depth;
  sg_view    view;
  sg_sampler sampler;
} MD1Skin;

typedef struct {
  MD1Header  header;
  MD1Skin*   skins;
  MD1Vertex* vertices;
  MD1Pose*   poses;
  F32*       frames_vbuf;
  Arena*     arena;
} MD1;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

MD1Error md1_load(CBuf buf, Sz buf_sz, MD1* md1);
MD1Error md1_get_vertices(const MD1* md1,
                          U32        pose_idx,
                          U32        frame_idx,
                          F32**      frame_vbuf,
                          Sz*        frame_vbuf_size);
MD1Error md1_unload(MD1* md1);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MD1_IMPLEMENTATION

#include "data.h"

static Nothing
md1_load_image(CBuf ptr, Buf pixels, Sz sz) {
  Dbg("md1_load_image() ...");

  Assert(ptr != 0);
  Assert(pixels != 0);
  Assert(sz > 0);

  U8* indices = (U8*)ptr;
  for (U32 i = 0, j = 0; i < sz; i++, j += 4) {
    U32 index = indices[i];
    pixels[j + 0] = quake1_palette[index][0];  // red
    pixels[j + 1] = quake1_palette[index][1];  // green
    pixels[j + 2] = quake1_palette[index][2];  // blue
    pixels[j + 3] = 255;                       // alpha, always opaque
  }
}

internal MD1Error
md1_load_skins(MD1* md1, CBuf ptr, U32* ofs) {
  Dbg("md1_load_skins() ...");

  Assert(md1 != 0);
  Assert(ptr != 0);
  Assert(ofs != 0);
  Assert(*ofs > 0);

  const MD1Header* h = &md1->header;
  const U32        channels = 4;
  Arena*           a = md1->arena;
  U32              sw = h->skin_width;
  U32              sh = h->skin_height;
  Sz               skin_sz = sw * sh;
  MD1Skin*         skins = arena_push_array(a, MD1Skin, h->skins_count);
  MD1Error         err = MD1_ERR_SUCCESS;

  AssertAlways(skins != 0);

  for (U32 skin_idx = 0; skin_idx < h->skins_count; skin_idx++) {
    Dbg("skin #%d .P..", skin_idx);

    MD1SkinType* st = (MD1SkinType*)(ptr + (*ofs));
    *ofs += sizeof(MD1SkinType);

    if (MD1_SKIN_SINGLE == *st) {
      Sz  data_sz = sizeof(U8) * skin_sz * channels;
      Buf data = (Buf)arena_push(a, data_sz, AlignOf(U8), TRUE);
      Assert(data != 0);

      // constructing pixel data
      U8* raw_data = (U8*)(ptr + (*ofs));
      for (U32 raw_idx = 0, rgba = 0; raw_idx < skin_sz; raw_idx++, rgba += 4) {
        U32 palette_idx = raw_data[raw_idx];
        data[rgba + 0] = quake1_palette[palette_idx][0]; /* RED */
        data[rgba + 1] = quake1_palette[palette_idx][1]; /* GREEN */
        data[rgba + 2] = quake1_palette[palette_idx][2]; /* BLUE */
        data[rgba + 3] = 255; /* ALPHA - always opaque */
      }
      *ofs += skin_sz;

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
  return err;
}

internal MD1Error
md1_load_uvs(const MD1* md1, CBuf ptr, U32* ofs, MD1UV** uvs) {
  Dbg("md1_load_uvs() ...");

  Assert(md1 != 0);
  Assert(ptr != 0);
  Assert(ofs != 0);
  Assert(*ofs > 0);
  Assert(uvs != 0);

  const MD1Header* h = &md1->header;
  Arena*           a = md1->arena;
  Sz               uvs_sz = sizeof(MD1UV) * h->vertices_count;
  MD1Error         err = MD1_ERR_SUCCESS;

  *uvs = arena_push(a, uvs_sz, AlignOf(MD1UV), TRUE);
  AssertAlways(*uvs != 0);

  for (U32 i = 0; i < h->vertices_count; i++) {
    ND_I32(&(*uvs)[i].is_on_seam, ptr, *ofs);
    ND_I32(&(*uvs)[i].u, ptr, *ofs);
    ND_I32(&(*uvs)[i].v, ptr, *ofs);
  }

  return err;
}

internal MD1Error
md1_load_triangles(const MD1* md1, CBuf ptr, U32* ofs, MD1FacedTriangle** fts) {
  Dbg("md1_load_triangles() ...");

  Assert(md1 != 0);
  Assert(ptr != 0);
  Assert(ofs != 0);
  Assert(*ofs > 0);
  Assert(fts != 0);

  const MD1Header* h = &md1->header;
  Arena*           a = md1->arena;
  Sz               fts_sz = sizeof(MD1FacedTriangle) * h->triangles_count;
  MD1Error         err = MD1_ERR_SUCCESS;

  *fts = arena_push(a, fts_sz, AlignOf(MD1FacedTriangle), TRUE);
  AssertAlways(*fts != 0);

  for (U32 i = 0; i < h->triangles_count; i++) {
    ND_I32(&(*fts)[i].is_front_face, ptr, *ofs);
    ND_I32(&(*fts)[i].vertices_idx[0], ptr, *ofs);
    ND_I32(&(*fts)[i].vertices_idx[1], ptr, *ofs);
    ND_I32(&(*fts)[i].vertices_idx[2], ptr, *ofs);
  }

  return err;
}

internal Bool
md1_has_pose_name_changed(Str new, Str old) {
  for (U32 i = 0; i < MD1_MAX_FRAME_NAME_LEN - 1; i++) {
    if (isdigit(new[i])) {
      new[i] = 0;
    }
  }

  if (strlen(old) <= 0) {
    return TRUE;
  }

  if (strcmp(new, old) != 0) {
    return TRUE;
  }

  return FALSE;
}

internal MD1Error
md1_load_single_frame(MD1* md1,
                      CBuf ptr,
                      U32* ofs,
                      U32  frame_idx,
                      Str  frame_name) {
  Dbg("md1_load_single_frame() ...");

  Assert(md1 != 0);
  Assert(ptr != 0);
  Assert(ofs != 0);
  Assert(*ofs > 0);
  Assert(frame_name != 0);

  MD1Header*     h = &md1->header;
  MD1Error       err = MD1_ERR_SUCCESS;
  Sz             sf_size = sizeof(MD1FrameSingle);
  MD1FrameSingle sf = {0};

  memcpy(&sf, ptr + (*ofs), sf_size);
  *ofs += sf_size;

  if (md1_has_pose_name_changed(sf.name, frame_name) && frame_idx > 0) {
    memcpy(md1->poses[h->poses_count-1].name, sf.name, MD1_MAX_FRAME_NAME_LEN);
    h->poses_count += 1;
    md1->poses[h->poses_count-1].first_frame = frame_idx;
  }

  md1->poses[h->poses_count-1].frames_count++;
  memcpy(frame_name, sf.name, MD1_MAX_FRAME_NAME_LEN);
  frame_name[MD1_MAX_FRAME_NAME_LEN - 1] = 0;

  MD1NormalVertex* nv = (MD1NormalVertex*)(ptr + (*ofs));

  Sz               nv_size = sizeof(MD1NormalVertex);
  for (U32 i = 0; i < h->vertices_count; i++) {
    MD1NormalVertex nv = {0};
    memcpy(&nv, ptr + (*ofs), nv_size);

    md1->vertices[i].vertex.X = nv.vertex[0];
    md1->vertices[i].vertex.Y = nv.vertex[1];
    md1->vertices[i].vertex.Z = nv.vertex[2];

    md1->vertices[i].normal.X = quake1_normals[nv.normal_idx][0];
    md1->vertices[i].normal.Y = quake1_normals[nv.normal_idx][1];
    md1->vertices[i].normal.Z = quake1_normals[nv.normal_idx][2];

    *ofs += sizeof(MD1NormalVertex);
  }

  return err;
}

internal MD1Error
md1_load_frames(MD1* md1, CBuf ptr, U32* ofs) {
  Dbg("md1_load_frames() ...");

  Assert(md1 != 0);
  Assert(ptr != 0);
  Assert(ofs != 0);
  Assert(*ofs > 0);

  MD1Header* h = &md1->header;
  Arena*     a = md1->arena;
  U32        pose_frames = 0;
  char       frame_name[MD1_MAX_FRAME_NAME_LEN] = {0};
  MD1Error   err = MD1_ERR_SUCCESS;

  Sz vertices_sz = sizeof(MD1Vertex) * h->vertices_count * h->frames_count;
  md1->vertices = arena_push(a, vertices_sz, AlignOf(MD1Vertex), TRUE);
  AssertAlways(md1->vertices != 0);

  /* TODO: the pose count is taken from frames count
     which is way more that what it has to be, it is safe though
     but a waste of memory!
  */
  Sz poses_sz = sizeof(MD1Pose) * h->frames_count; /* TODO: duh! */
  md1->poses = arena_push(a, poses_sz, AlignOf(MD1Pose), TRUE);
  AssertAlways(md1->poses != 0);

  h->poses_count = 1;
  md1->poses[0].first_frame = 0;

  for (U32 frame_idx = 0; frame_idx < h->frames_count; frame_idx++) {
    MD1FrameType ft = 0;
    ND_I32(&ft, ptr, *ofs);

    if (MD1_FT_SINGLE == ft) {
      md1_load_single_frame(md1, ptr, ofs, frame_idx, frame_name);
    } else {
      AssertAlways("NOT IMPLEMENTED!");
    }
  }

  return err;
}

MD1Error
md1_make_display_list(MD1* md1, MD1UV* uv_list, MD1FacedTriangle* ft_list) {
  Dbg("md1_make_display_list() ...");

  Assert(md1 != 0);
  Assert(uv_list != 0);
  Assert(ft_list != 0);

  MD1Error   err = MD1_ERR_SUCCESS;
  MD1Header* h = &md1->header;
  Arena*     a = md1->arena;
  Sz         elems_count = 3 * (3 + 2);  // a->b->c * x,y,z, u,v
  Sz vbuf_sz = sizeof(F32) * h->frames_count * h->triangles_count * elems_count;

  h->frame_vbuf_size = elems_count * h->triangles_count;
  md1->frames_vbuf = arena_push(a, vbuf_sz, AlignOf(F32), TRUE);
  AssertAlways(md1->frames_vbuf != 0);

  U32 vbuf_vert_idx = 0;
  for (U32 frm_idx = 0; frm_idx < h->frames_count; frm_idx++) {
    for (U32 tri_idx = 0; tri_idx < h->triangles_count; tri_idx++) {
      for (U32 tri_xyz = 0; tri_xyz < 3; tri_xyz++) {
        const MD1Vertex* frame_verts =
            md1->vertices + (h->vertices_count * frm_idx);
        I32 xyz = ft_list[tri_idx].vertices_idx[tri_xyz];
        F32 x = (frame_verts[xyz].vertex.X * h->scale.X) + h->translate.X;
        F32 y = (frame_verts[xyz].vertex.Y * h->scale.Y) + h->translate.Y;
        F32 z = (frame_verts[xyz].vertex.Z * h->scale.Z) + h->translate.Z;
        F32 u = uv_list[xyz].u;
        F32 v = uv_list[xyz].v;

        if (!ft_list[tri_idx].is_front_face && uv_list[xyz].is_on_seam) {
          u += h->skin_width / 2;
        }

        u = (u + 0.5) / h->skin_width;
        v = (v + 0.5) / h->skin_height;

        md1->frames_vbuf[vbuf_vert_idx++] = x;
        md1->frames_vbuf[vbuf_vert_idx++] = y;
        md1->frames_vbuf[vbuf_vert_idx++] = z;
        md1->frames_vbuf[vbuf_vert_idx++] = u;
        md1->frames_vbuf[vbuf_vert_idx++] = v;
      }
    }
  }

  return err;
}

internal MD1Error
md1_scale_and_translate_bbox(MD1* mdl) {
  Dbg("md1_scale_and_translate_bbox() ...");

  MD1Error   err = MD1_ERR_SUCCESS;
  MD1Header* h = &mdl->header;

  h->bbox_min.X = (h->bbox_min.X * h->scale.X) + h->translate.X;
  h->bbox_min.Y = (h->bbox_min.Y * h->scale.Y) + h->translate.Y;
  h->bbox_min.Z = (h->bbox_min.Z * h->scale.Z) + h->translate.Z;
  h->bbox_max.X = (h->bbox_max.X * h->scale.X) + h->translate.X;
  h->bbox_max.Y = (h->bbox_max.Y * h->scale.Y) + h->translate.Y;
  h->bbox_max.Z = (h->bbox_max.Z * h->scale.Z) + h->translate.Z;

  return err;
}

MD1Error
md1_load(CBuf buf, Sz buf_sz, MD1* md1) {
  Dbg("mdl_load() ...");

  Assert(buf != 0);
  Assert(buf_sz > 0);
  Assert(md1 != 0);
  Assert(md1->arena == 0);

  MD1Error err = MD1_ERR_SUCCESS;
  MemZero(md1, sizeof(MD1));
  U32          ofs = 0;
  CBuf         ptr = (CBuf)buf;
  MD1RawHeader rh = {0};

  ND_I32(&rh.magic_code, ptr, ofs);
  ND_I32(&rh.version, ptr, ofs);
  ND_F32(&rh.scale[0], ptr, ofs);
  ND_F32(&rh.scale[1], ptr, ofs);
  ND_F32(&rh.scale[2], ptr, ofs);
  ND_F32(&rh.translate[0], ptr, ofs);
  ND_F32(&rh.translate[1], ptr, ofs);
  ND_F32(&rh.translate[2], ptr, ofs);
  ND_F32(&rh.bounding_radius, ptr, ofs);
  ND_F32(&rh.eye_position[0], ptr, ofs);
  ND_F32(&rh.eye_position[1], ptr, ofs);
  ND_F32(&rh.eye_position[2], ptr, ofs);
  ND_I32(&rh.skins_count, ptr, ofs);
  ND_I32(&rh.skin_width, ptr, ofs);
  ND_I32(&rh.skin_height, ptr, ofs);
  ND_I32(&rh.vertices_count, ptr, ofs);
  ND_I32(&rh.triangles_count, ptr, ofs);
  ND_I32(&rh.frames_count, ptr, ofs);
  ND_I32(&rh.sync_type, ptr, ofs);
  ND_I32(&rh.flags, ptr, ofs);
  ND_F32(&rh.size, ptr, ofs);

  MD1Header* h = &md1->header;

  h->radius = rh.bounding_radius;
  h->skin_width = rh.skin_width;
  h->skin_height = rh.skin_height;
  h->skins_count = rh.skins_count;
  h->vertices_count = rh.vertices_count;
  h->triangles_count = rh.triangles_count;
  h->frames_count = rh.frames_count;
  h->scale.X = rh.scale[0];
  h->scale.Y = rh.scale[1];
  h->scale.Z = rh.scale[2];
  h->translate.X = rh.translate[0];
  h->translate.Y = rh.translate[1];
  h->translate.Z = rh.translate[2];
  h->eye.X = rh.eye_position[0];
  h->eye.Y = rh.eye_position[1];
  h->eye.Z = rh.eye_position[2];

  AssertAlways(h->radius > 0);
  AssertAlways(h->skin_width > 0);
  AssertAlways(h->skin_height > 0);
  AssertAlways(h->skins_count > 0);
  AssertAlways(h->vertices_count > 0);
  AssertAlways(h->triangles_count > 0);
  AssertAlways(h->frames_count > 0);

  md1->arena = arena_create(); /* TODO: set some initial params */

  err = md1_load_skins(md1, ptr, &ofs);
  if (err != MD1_ERR_SUCCESS)
    return err;

  MD1UV* uv_list = 0;
  err = md1_load_uvs(md1, ptr, &ofs, &uv_list);
  if (err != MD1_ERR_SUCCESS)
    return err;
  Assert(uv_list != 0);

  MD1FacedTriangle* ft_list = NULL;
  err = md1_load_triangles(md1, ptr, &ofs, &ft_list);
  if (err != MD1_ERR_SUCCESS)
    return err;
  Assert(ft_list != 0);

  err = md1_load_frames(md1, ptr, &ofs);
  if (err != MD1_ERR_SUCCESS)
    return err;

  err = md1_scale_and_translate_bbox(md1);
  if (err != MD1_ERR_SUCCESS)
    return err;

  err = md1_make_display_list(md1, uv_list, ft_list);
  if (err != MD1_ERR_SUCCESS)
    return err;

  Dbg("header.vertices: %d", h->vertices_count);
  Dbg("header.triangles: %d", h->triangles_count);
  Dbg("header.frames: %d", h->frames_count);
  Dbg("header.skins: %d", h->skins_count);
  Dbg("header.skin size: %d x %d", h->skin_width, h->skin_height);

  return err;
}

MD1Error
md1_get_vertices(const MD1* md1,
                 U32        pose_idx,
                 U32        pose_frame_idx,
                 F32**      frame_vbuf,
                 Sz*        frame_vbuf_size) {
  Assert(md1 != 0);
  Assert(frame_vbuf != 0);
  Assert(frame_vbuf_size != 0);
  Assert(pose_idx < md1->header.poses_count);
  Assert(pose_frame_idx < md1->poses[pose_idx].frames_count);

  MD1Error err = MD1_ERR_SUCCESS;
  MD1Pose* pose = &md1->poses[pose_idx];
  *frame_vbuf = &md1->frames_vbuf[(pose->first_frame + pose_frame_idx) *
                                  md1->header.frame_vbuf_size];
  *frame_vbuf_size = md1->header.frame_vbuf_size;
  return err;
}

MD1Error
md1_unload(MD1* md1) {
  Assert(md1 != 0);
  Assert(md1->arena != 0);
  arena_destroy(md1->arena);
  return MD1_ERR_SUCCESS;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MD1_IMPLEMENTATION
#endif  // MD1_HEADER_
