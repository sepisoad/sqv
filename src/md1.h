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
  U32 uv[2];
  U32 vertex_index;
} MD1MeshVertices;

typedef struct {
  F32    radius;
  U32    skin_width;
  U32    skin_height;
  U32    skins_count;
  U32    vertices_count;
  U32    indices_count;
  U32    triangles_count;
  U32    frames_count;
  U32    poses_count;
  Sz     frame_vertex_buffer_size;
  Sz     index_buffer_size;
  hmm_v3 scale;
  hmm_v3 translate;
  hmm_v3 eye;
  hmm_v3 bbox_min;
  hmm_v3 bbox_max;
} MD1Header;

typedef struct {
  hmm_v3 vertex;
  // TODO: we do not apply lighing in SQV, so why waste memory for normal data!
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
  F32*       frames_vbuf_v2;
  U32*       index_buffer;
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
                          Sz*        frame_vertex_buffer_size);
MD1Error md1_get_vertices_v2(MD1*  md1,
                             U32   pose_idx,
                             U32   frame_idx,
                             F32** vbuf,
                             Sz*   vbuf_size,
                             U32** ibuf,
                             Sz*   ibuf_size);
MD1Error md1_unload(MD1* md1);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MD1_IMPLEMENTATION

#include "data.h"

static Nothing
md1_load_image(CBuf ptr, Buf pixels, Sz sz) {
  Dbg("md1_load_image() ...");

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
md1_load_skins(MD1* md1, NDBuffer* ndb) {
  Dbg("md1_load_skins() ...");

  Assert(md1 != 0);
  Assert(ndb != 0);

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

    MD1SkinType* st = (MD1SkinType*)ND_ADDR(ndb);
    ND_MOVE(ndb, sizeof(MD1SkinType));

    if (MD1_SKIN_SINGLE == *st) {
      Sz  data_sz = sizeof(U8) * skin_sz * channels;
      Buf data = (Buf)arena_push(a, data_sz, AlignOf(U8), TRUE);
      Assert(data != 0);

      // constructing pixel data
      U8* raw_data = (U8*)ND_ADDR(ndb);
      for (U32 raw_idx = 0, rgba = 0; raw_idx < skin_sz; raw_idx++, rgba += 4) {
        U32 palette_idx = raw_data[raw_idx];
        data[rgba + 0] = quake1_palette[palette_idx][0]; /* RED */
        data[rgba + 1] = quake1_palette[palette_idx][1]; /* GREEN */
        data[rgba + 2] = quake1_palette[palette_idx][2]; /* BLUE */
        data[rgba + 3] = 255; /* ALPHA - always opaque */
      }
      ND_MOVE(ndb, skin_sz);

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
md1_load_uvs(const MD1* md1, NDBuffer* ndb, MD1UV** uvs) {
  Dbg("md1_load_uvs() ...");

  Assert(md1 != 0);
  Assert(ndb != 0);
  Assert(uvs != 0);

  const MD1Header* h = &md1->header;
  Arena*           a = md1->arena;
  Sz               uvs_sz = sizeof(MD1UV) * h->vertices_count;
  MD1Error         err = MD1_ERR_SUCCESS;

  *uvs = arena_push(a, uvs_sz, AlignOf(MD1UV), TRUE);
  AssertAlways(*uvs != 0);

  for (U32 i = 0; i < h->vertices_count; i++) {
    ND_I32(&(*uvs)[i].is_on_seam, ndb);
    ND_I32(&(*uvs)[i].u, ndb);
    ND_I32(&(*uvs)[i].v, ndb);
  }

  return err;
}

internal MD1Error
md1_load_triangles(MD1* md1, NDBuffer* ndb, MD1FacedTriangle** fts) {
  Dbg("md1_load_triangles() ...");

  Assert(md1 != 0);
  Assert(ndb != 0);
  Assert(fts != 0);

  MD1Header* h = &md1->header;
  Arena*     a = md1->arena;
  Sz         fts_sz = sizeof(MD1FacedTriangle) * h->triangles_count;
  Sz         idices_sz = sizeof(U32) * h->triangles_count * 3;
  MD1Error   err = MD1_ERR_SUCCESS;

  *fts = arena_push(a, fts_sz, AlignOf(MD1FacedTriangle), TRUE);
  AssertAlways(*fts != 0);

  // TODO: we no longer need this commented block, delete it
  // md1->indices = arena_push(a, idices_sz, AlignOf(U32), TRUE);
  // AssertAlways(md1->indices);

  for (U32 i = 0, j = 0; i < h->triangles_count; i++, j += 3) {
    I32 a, b, c, f = 0;

    ND_I32(&f, ndb);
    ND_I32(&a, ndb);
    ND_I32(&b, ndb);
    ND_I32(&c, ndb);

    (*fts)[i].is_front_face = f;
    (*fts)[i].vertices_idx[0] = a;
    (*fts)[i].vertices_idx[1] = b;
    (*fts)[i].vertices_idx[2] = c;

    // TODO: we no longer need this commented block, delete it
    // md1->indices[j + 0] = (U32)a;
    // md1->indices[j + 1] = (U32)b;
    // md1->indices[j + 2] = (U32)c;
  }

  // TODO: we no longer need this commented block, delete it
  // h->indices_count = h->triangles_count * 3;

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
md1_load_single_frame(MD1* md1, NDBuffer* ndb, U32 frame_idx, Str frame_name) {
  Dbg("md1_load_single_frame() ...");

  Assert(md1 != 0);
  Assert(ndb != 0);
  Assert(frame_name != 0);

  MD1Header*     h = &md1->header;
  MD1Error       err = MD1_ERR_SUCCESS;
  Sz             sf_size = sizeof(MD1FrameSingle);
  MD1FrameSingle sf = {0};

  memcpy(&sf, ND_ADDR(ndb), sf_size);
  ND_MOVE(ndb, sf_size);

  if (md1_has_pose_name_changed(sf.name, frame_name) && frame_idx > 0) {
    memcpy(md1->poses[h->poses_count - 1].name, sf.name,
           MD1_MAX_FRAME_NAME_LEN);
    h->poses_count += 1;
    md1->poses[h->poses_count - 1].first_frame = frame_idx;
  }

  md1->poses[h->poses_count - 1].frames_count++;
  memcpy(frame_name, sf.name, MD1_MAX_FRAME_NAME_LEN);
  frame_name[MD1_MAX_FRAME_NAME_LEN - 1] = 0;

  MD1NormalVertex* nv = (MD1NormalVertex*)ND_ADDR(ndb);
  Sz               nv_size = sizeof(MD1NormalVertex);
  MD1Vertex* frame_verts = md1->vertices + (h->vertices_count * frame_idx);

  for (U32 i = 0; i < h->vertices_count; i++) {
    MD1NormalVertex nv = {0};
    memcpy(&nv, ND_ADDR(ndb), nv_size);

    frame_verts[i].vertex.X = nv.vertex[0];
    frame_verts[i].vertex.Y = nv.vertex[1];
    frame_verts[i].vertex.Z = nv.vertex[2];

    // TODO: we do not apply lighing in SQV, so why waste memory for normal
    // data!
    frame_verts[i].normal.X = quake1_normals[nv.normal_idx][0];
    frame_verts[i].normal.Y = quake1_normals[nv.normal_idx][1];
    frame_verts[i].normal.Z = quake1_normals[nv.normal_idx][2];

    ND_MOVE(ndb, sizeof(MD1NormalVertex));
  }

  return err;
}

internal MD1Error
md1_load_frames(MD1* md1, NDBuffer* ndb) {
  Dbg("md1_load_frames() ...");

  Assert(md1 != 0);
  Assert(ndb != 0);

  MD1Header* h = &md1->header;
  Arena*     a = md1->arena;
  U32        pose_frames = 0;
  char       frame_name[MD1_MAX_FRAME_NAME_LEN] = {0};
  MD1Error   err = MD1_ERR_SUCCESS;

  Sz vertices_sz = sizeof(MD1Vertex) * h->vertices_count * h->frames_count;
  md1->vertices = arena_push(a, vertices_sz, AlignOf(MD1Vertex), TRUE);
  AssertAlways(md1->vertices != 0);

  // TODO: the pose count is taken from frames count
  //       which is way more that what it has to be, it is safe though
  //       but a waste of memory!

  Sz poses_sz = sizeof(MD1Pose) * h->frames_count; /* duh! */
  md1->poses = arena_push(a, poses_sz, AlignOf(MD1Pose), TRUE);
  AssertAlways(md1->poses != 0);

  h->poses_count = 1;
  md1->poses[0].first_frame = 0;

  for (U32 frame_idx = 0; frame_idx < h->frames_count; frame_idx++) {
    MD1FrameType ft = 0;
    ND_I32(&ft, ndb);

    if (MD1_FT_SINGLE == ft) {
      md1_load_single_frame(md1, ndb, frame_idx, frame_name);
    } else {
      AssertAlways("NOT IMPLEMENTED!");
    }
  }

  return err;
}

MD1Error
md1_make_display_list(MD1* md1, MD1UV* uvs, MD1FacedTriangle* faced_triangles) {
  Dbg("md1_make_display_list() ...");

  Assert(md1 != 0);
  Assert(uvs != 0);
  Assert(faced_triangles != 0);

  MD1Error   err = MD1_ERR_SUCCESS;
  MD1Header* h = &md1->header;
  Arena*     a = md1->arena;
  Sz         elems_count = 3 * (3 + 2);  // a->b->c * x,y,z, u,v
  Sz vbuf_sz = sizeof(F32) * h->frames_count * h->triangles_count * elems_count;

  h->frame_vertex_buffer_size = elems_count * h->triangles_count;
  md1->frames_vbuf = arena_push(a, vbuf_sz, AlignOf(F32), TRUE);
  AssertAlways(md1->frames_vbuf != 0);

  U32 vbuf_vert_idx = 0;
  for (U32 frm_idx = 0; frm_idx < h->frames_count; frm_idx++) {
    for (U32 triangle_index = 0; triangle_index < h->triangles_count;
         triangle_index++) {
      for (U32 tri_xyz = 0; tri_xyz < 3; tri_xyz++) {
        const MD1Vertex* frame_verts =
            md1->vertices + (h->vertices_count * frm_idx);
        I32 xyz = faced_triangles[triangle_index].vertices_idx[tri_xyz];
        F32 x = (frame_verts[xyz].vertex.X * h->scale.X) + h->translate.X;
        F32 y = (frame_verts[xyz].vertex.Y * h->scale.Y) + h->translate.Y;
        F32 z = (frame_verts[xyz].vertex.Z * h->scale.Z) + h->translate.Z;
        F32 u = uvs[xyz].u;
        F32 v = uvs[xyz].v;

        if (!faced_triangles[triangle_index].is_front_face &&
            uvs[xyz].is_on_seam) {
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
md1_make_display_list_v2(MD1*              md1,
                         MD1UV*            uvs,
                         MD1FacedTriangle* faced_triangles) {
  MD1Error         err = MD1_ERR_SUCCESS;
  MD1Header*       h = &md1->header;
  Arena*           a = md1->arena;
  U32              mesh_indices_count = 0;
  U32              mesh_triangles_count = 0;

  // TODO: use scratch buffwe for this
  MD1MeshVertices* mesh_vertices =
      arena_push(a, h->triangles_count * 3 * sizeof(MD1MeshVertices),
                 AlignOf(MD1MeshVertices), TRUE);

  Sz   index_buffer_size = h->triangles_count * 3 * sizeof(U32);
  U32* index_buffer = arena_push(a, index_buffer_size, AlignOf(U32), TRUE);

  for (U32 triangle_index = 0; triangle_index < h->triangles_count;
       triangle_index++) {
    for (U32 triangle_vertex_index = 0; triangle_vertex_index < 3;
         triangle_vertex_index++, mesh_indices_count++) {
      U32 vertex_index =
          faced_triangles[triangle_index].vertices_idx[triangle_vertex_index];
      U32 u = uvs[vertex_index].u;
      U32 v = uvs[vertex_index].v;

      if (!faced_triangles[triangle_index].is_front_face &&
          uvs[vertex_index].is_on_seam) {
        u += h->skin_width / 2;
      }

      U32 mesh_triangle_index = 0;
      for (; mesh_triangle_index < mesh_triangles_count;
           mesh_triangle_index++) {
        if (mesh_vertices[mesh_triangle_index].vertex_index == vertex_index &&
            mesh_vertices[mesh_triangle_index].uv[0] == u &&
            mesh_vertices[mesh_triangle_index].uv[1] == v) {
          index_buffer[mesh_indices_count] = mesh_triangle_index;
          break;
        }
      }

      if (mesh_triangle_index == mesh_triangles_count) {
        index_buffer[mesh_triangle_index] = mesh_triangles_count;
        mesh_vertices[mesh_triangle_index].vertex_index = vertex_index;
        mesh_vertices[mesh_triangle_index].uv[0] = u;
        mesh_vertices[mesh_triangle_index].uv[1] = v;
        mesh_triangles_count++;
      }
    }
  }

  U32 vbuf_vert_idx = 0;
  U32 mesh_vertices_count = mesh_triangles_count * 3;
  Sz  elems_count = 3 * (3 + 2);
  Sz  vbuf_sz =
      sizeof(F32) * h->frames_count * mesh_triangles_count * elems_count;

  h->frame_vertex_buffer_size = elems_count * h->triangles_count;
  md1->frames_vbuf_v2 = arena_push(a, vbuf_sz, AlignOf(F32), TRUE);

  for (U32 frame_index = 0; frame_index < h->frames_count; frame_index++) {
    for (U32 mesh_vertex_index = 0; mesh_vertex_index < mesh_vertices_count;
         mesh_vertex_index++) {
      const MD1Vertex* frame_verts =
          md1->vertices + (h->vertices_count * frame_index);

      U32 vertex_index = mesh_vertices[mesh_vertex_index].vertex_index;
      F32 x =
          (frame_verts[vertex_index].vertex.X * h->scale.X) + h->translate.X;
      F32 y =
          (frame_verts[vertex_index].vertex.Y * h->scale.Y) + h->translate.Y;
      F32 z =
          (frame_verts[vertex_index].vertex.Z * h->scale.Z) + h->translate.Z;
      F32 u = mesh_vertices[mesh_vertex_index].uv[0];
      F32 v = mesh_vertices[mesh_vertex_index].uv[1];

      md1->frames_vbuf_v2[vbuf_vert_idx++] = x;
      md1->frames_vbuf_v2[vbuf_vert_idx++] = y;
      md1->frames_vbuf_v2[vbuf_vert_idx++] = z;
      md1->frames_vbuf_v2[vbuf_vert_idx++] = u;
      md1->frames_vbuf_v2[vbuf_vert_idx++] = v;
    }
  }

  h->index_buffer_size = index_buffer_size;
  md1->index_buffer = index_buffer;

  // ***************************************
  // U32              mesh_triangles_count;
  // U32              mesh_indices_count;
  // MD1MeshVertices* mesh_vertices;
  // U32*             index_buffer;
  // ***************************************
  // h->mesh_indices_count = mesh_indices_count;
  // h->mesh_triangles_count = mesh_triangles_count;
  // h->mesh_vertices = mesh_vertices;
  // h->index_buffer = index_buffer;

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
  MD1RawHeader rh = {0};
  NDBuffer     nd_buffer = {.base = (CBuf)buf, .offset = 0};
  NDBuffer*    ndb = &nd_buffer;

  ND_I32(&rh.magic_code, ndb);
  ND_I32(&rh.version, ndb);
  ND_F32(&rh.scale[0], ndb);
  ND_F32(&rh.scale[1], ndb);
  ND_F32(&rh.scale[2], ndb);
  ND_F32(&rh.translate[0], ndb);
  ND_F32(&rh.translate[1], ndb);
  ND_F32(&rh.translate[2], ndb);
  ND_F32(&rh.bounding_radius, ndb);
  ND_F32(&rh.eye_position[0], ndb);
  ND_F32(&rh.eye_position[1], ndb);
  ND_F32(&rh.eye_position[2], ndb);
  ND_I32(&rh.skins_count, ndb);
  ND_I32(&rh.skin_width, ndb);
  ND_I32(&rh.skin_height, ndb);
  ND_I32(&rh.vertices_count, ndb);
  ND_I32(&rh.triangles_count, ndb);
  ND_I32(&rh.frames_count, ndb);
  ND_I32(&rh.sync_type, ndb);
  ND_I32(&rh.flags, ndb);
  ND_F32(&rh.size, ndb);

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

  // TODO: set some initial params
  md1->arena = arena_create();

  err = md1_load_skins(md1, ndb);
  if (err != MD1_ERR_SUCCESS)
    return err;

  MD1UV* uvs = 0;
  err = md1_load_uvs(md1, ndb, &uvs);
  if (err != MD1_ERR_SUCCESS)
    return err;
  Assert(uvs != 0);

  MD1FacedTriangle* faced_triangles = NULL;
  err = md1_load_triangles(md1, ndb, &faced_triangles);
  if (err != MD1_ERR_SUCCESS)
    return err;
  Assert(faced_triangles != 0);

  err = md1_load_frames(md1, ndb);
  if (err != MD1_ERR_SUCCESS)
    return err;

  err = md1_scale_and_translate_bbox(md1);
  if (err != MD1_ERR_SUCCESS)
    return err;

  err = md1_make_display_list(md1, uvs, faced_triangles);
  if (err != MD1_ERR_SUCCESS)
    return err;

  // err = md1_make_display_list_v2(md1, uvs, faced_triangles);
  // if (err != MD1_ERR_SUCCESS)
  //   return err;

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
                 Sz*        frame_vertex_buffer_size) {
  Assert(md1 != 0);
  Assert(frame_vbuf != 0);
  Assert(frame_vertex_buffer_size != 0);
  Assert(pose_idx < md1->header.poses_count);
  Assert(pose_frame_idx < md1->poses[pose_idx].frames_count);

  MD1Error err = MD1_ERR_SUCCESS;
  MD1Pose* pose = &md1->poses[pose_idx];
  *frame_vbuf = &md1->frames_vbuf[(pose->first_frame + pose_frame_idx) *
                                  md1->header.frame_vertex_buffer_size];
  *frame_vertex_buffer_size = md1->header.frame_vertex_buffer_size;
  return err;
}

MD1Error
md1_get_vertices_v2(MD1*  md1,
                    U32   pose_idx,
                    U32   frame_idx,
                    F32** vbuf,
                    Sz*   vbuf_size,
                    U32** ibuf,
                    Sz*   ibuf_size) {
  Assert(md1 != 0);
  Assert(vbuf != 0);
  Assert(vbuf_size != 0);
  Assert(ibuf != 0);
  Assert(ibuf_size != 0);
  Assert(pose_idx < md1->header.poses_count);
  Assert(frame_idx < md1->poses[pose_idx].frames_count);

  MD1Error   err = MD1_ERR_SUCCESS;
  MD1Header* h = &md1->header;
  MD1Pose*   pose = &md1->poses[pose_idx];
  U32 vbuf_loc = (pose->first_frame + frame_idx) * h->frame_vertex_buffer_size;

  *vbuf = &md1->frames_vbuf_v2[vbuf_loc];
  *vbuf_size = h->frame_vertex_buffer_size;
  *ibuf = md1->index_buffer;
  *ibuf_size = h->index_buffer_size;

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
