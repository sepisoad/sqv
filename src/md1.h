#ifndef MD1_HEADER_
#define MD1_HEADER_

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

#define MAX_STATIC_MEM 8192
#define MAX_FRAME_NAME_LEN 16

typedef F32 MD1VectorF;

typedef MD1VectorF MD1Vector3F[3];

typedef I32 MD1Triangle[3];

typedef U8 MD1RawVertex[3];

typedef enum {
  md1_SYNC_UNKNOWN = -1,
  md1_SYNC_SYNC = 0,
  md1_SYNC_RAND,
  md1_SYNC_FRAMETIME,
} MD1SyncType;

typedef enum {
  md1_SKIN_UNKNOWN = -1,
  md1_SKIN_SINGLE = 0,
  md1_SKIN_GROUP,
} MD1SkinType;

typedef enum {
  md1_FT_UNKNOWN = -1,
  md1_FT_SINGLE = 0,
  md1_FT_GROUP,
} MD1FrameType;

typedef struct {
  I32 magic_codes;
  I32 version;
  MD1Vector3F scale;
  MD1Vector3F translate;
  F32 bounding_radius;
  MD1Vector3F eye_position;
  I32 skins_length;
  I32 skin_width;
  I32 skin_height;
  I32 vertices_length;
  I32 triangles_length;
  I32 frames_length;
  MD1SyncType sync_type;
  I32 flags;
  F32 size;
} MD1RawHeader;

typedef struct {
  I32 onseam;
  I32 s;
  I32 t;
} MD1ST;

typedef struct {
  I32 frontface;
  MD1Triangle vertices_idx;
} MD1FacedTriangle;

typedef struct {
  MD1RawVertex vertex;
  U8 normal_idx;
} MD1NormalVertex;

typedef struct {
  MD1NormalVertex bbox_min;
  MD1NormalVertex bbox_max;
  char name[16];
} MD1FrameSingle;

typedef struct {
  I32 frames_length;
  MD1NormalVertex bbox_min;
  MD1NormalVertex bbox_max;
} MD1FramesGroup;

typedef struct {
  F32 radius;
  U32 skin_width;
  U32 skin_height;
  U32 skins_length;
  U32 vertices_length;
  U32 triangles_length;
  U32 frames_length;
  U32 poses_length;
  U32 vbuf_length;
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
  char name[MAX_FRAME_NAME_LEN];
  U32 start;
  U32 frames_length;
} MD1Pose;

typedef struct {
  sg_view image;
  sg_sampler sampler;
  // snk_image_t ui_image;
} MD1Skin;

typedef struct {
  MD1Header header;
  MD1Skin* skins;
  MD1Vertex* vertices;
  MD1Pose* poses;
  F32* vbuf;
  Arena* arena;
} MD1;

typedef enum {
  MD1_ERR_UNKNOWN = -1,
  MD1_ERR_SUCCESS = 0,
  MD1_ERR_FILE_OPEN,
  MD1_ERR_MEM_ALLOC,
  MD1_ERR_READ_SIZE,
  MD1_ERR_RAME_IDX,
  MD1_ERR_INVALID,
} MD1Error;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

MD1Error md1_load(const U8*, Sz, MD1*);
void md1_unload(MD1*);
void md1_get_vertices(const MD1*, U32, U32, const F32**, U32*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MD1_IMPLEMENTATION

#include "data.h"

const int MAGICCODE = (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I');
const int MD0VERSION = 6;
const int MAXSKINHEIGHT = 480;
const int MAXVERTICES = 2000;
const int MAXTRIANGLES = 4096;
const int MAXSKINS = 32;

extern const U8 quake1_palette[256][3];
extern const F32 quake1_normals[162][3];

static Bool
MD1Pose_changed(char* new, char* old) {
  for (I32 i = 0; i < MAX_FRAME_NAME_LEN - 1; i++) {
    if (isdigit(new[i])) {
      new[i] = 0;
    }
  }

  if (strlen(old) <= 0) {
    return FALSE;
  }

  if (strcmp(new, old) == 0) {
    return FALSE;
  }

  return TRUE;
}

static void
md1_load_image(const U8* p, U8* pixels, Sz size) {
  Dbg("loading skin image data");
  U8* indices = (U8*)p;
  for (Sz i = 0, j = 0; i < size; i++, j += 4) {
    U32 index = indices[i];
    pixels[j + 0] = quake1_palette[index][0];  // red
    pixels[j + 1] = quake1_palette[index][1];  // green
    pixels[j + 2] = quake1_palette[index][2];  // blue
    pixels[j + 3] = 255;                       // alpha, always opaque
  }
}

static const U8*
md1_load_skins(MD1* md1, const U8* p) {
  const MD1Header* hdr = &md1->header;
  Arena* arena = md1->arena;
  U32 width = hdr->skin_width;
  U32 height = hdr->skin_height;
  Sz skin_size = width * height;

  Sz sz = sizeof(MD1Skin) * hdr->skins_length;
  MD1Skin* skins = (MD1Skin*)arena_push(arena, sz, alignof(MD1Skin), TRUE);
  NotNull(skins);

  for (Sz i = 0; i < hdr->skins_length; i++) {
    Dbg("loading skins #%d", i);
    MD1SkinType* skin_type = (MD1SkinType*)p;
    if (*skin_type == md1_SKIN_SINGLE) {
      p += sizeof(MD1SkinType);

      Sz pixel_sz = sizeof(U8) * skin_size * 4;
      U8* pixels = (U8*)arena_push(arena, pixel_sz, alignof(U8), TRUE);
      NotNull(pixels);

      md1_load_image(p, pixels, skin_size);

      skins[i].image = sg_alloc_image();
      skins[i].sampler = sg_make_sampler(&(sg_sampler_desc) {
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
      });
      // skins[i].ui_image = snk_make_image(&(snk_image_desc_t) {
      //   .image = skins[i].image,
      //   .sampler = skins[i].sampler,
      // });
      sg_init_image(skins[i].image,
      &(sg_image_desc) {
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = {
          .ptr = pixels,
          .size = (Sz)(width * height * 4),
        }
      });
      p += skin_size;
    } else {
      MustDie("load_skin() does not support multi skin YET!");
    }
  }

  md1->skins = skins;
  return p;
}

static const U8*
md1_load_st(MD1* md1, const U8* p, MD1ST** coords) {
  Dbg("loading texture S/T coordinates");
  Arena* arena = md1->arena;
  const MD1Header* hdr = &md1->header;
  Sz sz = sizeof(MD1ST) * hdr->vertices_length;
  *coords = (MD1ST*)arena_push(arena, sz, alignof(MD1ST), TRUE);
  NotNull(coords);

  const MD1ST* src = (const MD1ST*)p;

  for (Sz i = 0; i < hdr->vertices_length; i++) {
    (*coords)[i].onseam = nd_i32(src->onseam);
    (*coords)[i].s = nd_i32(src->s);
    (*coords)[i].t = nd_i32(src->t);
    src++;  // Move forward correctly
  }

  return (const U8*)src;
}

static const U8*
md1_load_triangles(MD1* md1,
                   const U8* p,
                   const MD1Header* hdr,
                   MD1FacedTriangle** ftris) {
  Dbg("loading triangles");
  Arena* arena = md1->arena;
  Sz sz = sizeof(MD1FacedTriangle) * hdr->triangles_length;
  *ftris = (MD1FacedTriangle*)arena_push(arena, sz, alignof(MD1FacedTriangle),
                                         TRUE);
  NotNull(ftris);

  const MD1FacedTriangle* src =
    (const MD1FacedTriangle*)p;  // Use separate pointer

  for (Sz i = 0; i < hdr->triangles_length; i++) {
    (*ftris)[i].frontface = nd_i32(src->frontface);
    for (Sz j = 0; j < 3; j++) {
      (*ftris)[i].vertices_idx[j] = nd_i32(src->vertices_idx[j]);
    }
    src++;  // Move forward correctly
  }

  return (const U8*)src;
}

static const U8*
md1_load_single_frame(MD1* md1,
                      U32 frm_idx,
                      U32* pos_len,
                      U32* pos_idx,
                      char* oldname,
                      const U8* p) {
  const MD1Header* hdr = &md1->header;
  MD1FrameSingle* snl = (MD1FrameSingle*)p;

  char name[16] = {0};
  strncpy(name, snl->name, sizeof(name) - 1);
  Dbg("loading single frame #%d from pose: %s", frm_idx, name);

  if (MD1Pose_changed(name, oldname)) {
    strncpy(md1->poses[*pos_idx].name, oldname, MAX_FRAME_NAME_LEN);
    md1->poses[*pos_idx].frames_length = *pos_len;
    md1->poses[(*pos_idx) + 1].start = frm_idx;
    *pos_idx = *pos_idx + 1;
    *pos_len = 0;
  }

  *pos_len = *pos_len + 1;
  strncpy(oldname, name, MAX_FRAME_NAME_LEN);
  oldname[MAX_FRAME_NAME_LEN - 1] = '\0';

  if (frm_idx + 1 >= hdr->frames_length) {
    strncpy(md1->poses[*pos_idx].name, oldname, MAX_FRAME_NAME_LEN);
    md1->poses[*pos_idx].frames_length = *pos_len;
    md1->poses[(*pos_idx) + 1].start = frm_idx;
  }

  for (U8 i = 0; i < 3; i++) {
    md1->header.bbox_min.Elements[i] =
      HMM_MIN(snl->bbox_min.vertex[i], md1->header.bbox_min.Elements[i]);

    md1->header.bbox_max.Elements[i] =
      HMM_MAX(snl->bbox_max.vertex[i], md1->header.bbox_max.Elements[i]);
  }

  MD1NormalVertex* raw_verts = (MD1NormalVertex*)(snl + 1);
  MD1Vertex* frmverts = md1->vertices + (hdr->vertices_length * frm_idx);

  for (U32 i = 0; i < hdr->vertices_length; i++) {
    frmverts[i].vertex = (hmm_v3) {
      raw_verts[i].vertex[0], raw_verts[i].vertex[1], raw_verts[i].vertex[2]
    };
    frmverts[i].normal = (hmm_v3) {
      quake1_normals[raw_verts[i].normal_idx][0],
                     quake1_normals[raw_verts[i].normal_idx][1],
                     quake1_normals[raw_verts[i].normal_idx][2]
    };
  }
  return (const U8*)(raw_verts + hdr->vertices_length);
}

static const U8*
md1_load_frames(MD1* md1, const U8* p) {
  Dbg("loading frames");
  Arena* arena = md1->arena;
  const MD1Header* hdr = &md1->header;
  U32 frames_length = hdr->frames_length;
  U32 verts_length = hdr->vertices_length;

  Sz verts_sz = sizeof(MD1Vertex) * verts_length * frames_length;
  md1->vertices = (MD1Vertex*)arena_push(arena, verts_sz, alignof(MD1Vertex),
                                         TRUE);
  NotNull(md1->vertices);

  Sz poses_sz = sizeof(MD1Pose) * md1->header.poses_length;
  md1->poses = (MD1Pose*)arena_push(arena, poses_sz, alignof(MD1Pose), TRUE);
  NotNull(md1->poses);

  U32 pos_idx = 0;
  U32 pos_len = 0;
  char oldname[MAX_FRAME_NAME_LEN] = {0};

  md1->poses[0].start = 0;

  for (U32 i = 0; i < md1->header.frames_length; i++) {
    MD1FrameType ft = nd_i32(*(MD1FrameType*)p);
    p += sizeof(MD1FrameType);

    if (ft == md1_FT_SINGLE) {
      p = md1_load_single_frame(md1, i, &pos_len, &pos_idx, oldname, p);
    } else {
      MustDie("md1_load_frames() does not support multi frames YET!");
    }
  }

  return p;
}

static void
md1_make_display_list(MD1* md1,
                      const MD1ST* coords,
                      const MD1FacedTriangle* ftris) {
  Dbg("generating vertex buffer data");
  Arena* arena = md1->arena;
  MD1Header* hdr = &md1->header;
  hmm_v3* scl = &hdr->scale;
  hmm_v3* trn = &hdr->translate;
  hmm_v3* bbx_min = &hdr->bbox_min;
  hmm_v3* bbx_max = &hdr->bbox_max;
  const U32 skn_wdt = md1->header.skin_width;
  const U32 skn_hgt = md1->header.skin_height;
  const U32 vrt_len = hdr->vertices_length;
  const U32 frm_len = hdr->frames_length;
  const U32 tri_len = hdr->triangles_length;

  U32 elm_len = 3 * (3 + 2);  // a->b->c * x,y,z, u,v
  Sz vbuf_sz = sizeof(F32) * frm_len * tri_len * elm_len;
  F32* vbuf = (F32*)arena_push(arena, vbuf_sz, alignof(F32), TRUE);
  NotNull(vbuf);

  U32 idx = 0;
  for (U32 frm_idx = 0; frm_idx < frm_len; frm_idx++) {
    for (U32 tri_idx = 0; tri_idx < tri_len; tri_idx++) {
      for (U8 vrt_idx = 0; vrt_idx < 3; vrt_idx++) {
        I32 tri_abc = ftris[tri_idx].vertices_idx[vrt_idx];

        const MD1Vertex* vrts = md1->vertices + (vrt_len * frm_idx);

        F32 x = (vrts[tri_abc].vertex.X * scl->X) + trn->X;
        F32 y = (vrts[tri_abc].vertex.Y * scl->Y) + trn->Y;
        F32 z = (vrts[tri_abc].vertex.Z * scl->Z) + trn->Z;

        F32 s = coords[tri_abc].s;
        F32 t = coords[tri_abc].t;

        if (!ftris[tri_idx].frontface && coords[tri_abc].onseam) {
          s += skn_wdt / 2;
        }

        s = (s + 0.5) / skn_wdt;
        t = (t + 0.5) / skn_hgt;

        vbuf[idx + 0] = x;  // x
        vbuf[idx + 1] = y;  // y
        vbuf[idx + 2] = z;  // z
        vbuf[idx + 3] = s;  // u
        vbuf[idx + 4] = t;  // v
        idx += 5;
      }
    }
  }

  md1->vbuf = vbuf;
  hdr->vbuf_length = elm_len * tri_len;
  Dbg("generated vertex buffer with %d items", frm_len * tri_len * elm_len);
}

void
md1_scale_translate_bbox(MD1* md1) {
  Dbg("scaling and translating bbox data");

  MD1Header* hdr = &md1->header;
  hmm_v3* scl = &hdr->scale;
  hmm_v3* trn = &hdr->translate;
  hmm_v3* bbx_min = &hdr->bbox_min;
  hmm_v3* bbx_max = &hdr->bbox_max;

  bbx_min->X = (bbx_min->X * scl->X) + trn->X;
  bbx_min->Y = (bbx_min->Y * scl->Y) + trn->Y;
  bbx_min->Z = (bbx_min->Z * scl->Z) + trn->Z;
  bbx_max->X = (bbx_max->X * scl->X) + trn->X;
  bbx_max->Y = (bbx_max->Y * scl->Y) + trn->Y;
  bbx_max->Z = (bbx_max->Z * scl->Z) + trn->Z;
}

void
md1_get_vertices(const MD1* md1,
                 U32 pos_idx,
                 U32 frm_idx,
                 const F32** vbuf,
                 U32* vbuf_len) {
  MakeSure(pos_idx < md1->header.poses_length, "invalid pose index");
  MakeSure(frm_idx < md1->poses[pos_idx].frames_length,
           "invalid frame index in pose");

  MD1Pose* pos = &md1->poses[pos_idx];
  *vbuf = &md1->vbuf[(pos->start + frm_idx) * md1->header.vbuf_length];
  *vbuf_len = md1->header.vbuf_length;
}

MD1Error
md1_load(const U8* buf, Sz bufsz, MD1* md1) {
  Arena* arena = arena_alloc(.requested_reserve_size = 1024,
                             .requested_commit_size = 1024);
  md1->arena = arena;

  const U8* p = buf;
  const MD1RawHeader* rhdr = (MD1RawHeader*)buf;

  Dbg("loading MD1 haeder ...");
  md1->header = (MD1Header) {
    .radius = nd_f32(rhdr->bounding_radius),
    .skin_width = nd_i32(rhdr->skin_width),
    .skin_height = nd_i32(rhdr->skin_height),
    .skins_length = nd_i32(rhdr->skins_length),
    .vertices_length = nd_i32(rhdr->vertices_length),
    .triangles_length = nd_i32(rhdr->triangles_length),
    .frames_length = nd_i32(rhdr->frames_length),
    .scale = {.X = nd_f32(rhdr->scale[0]),
              .Y = nd_f32(rhdr->scale[1]),
              .Z = nd_f32(rhdr->scale[2])
             },
    .translate = {.X = nd_f32(rhdr->translate[0]),
                  .Y = nd_f32(rhdr->translate[1]),
                  .Z = nd_f32(rhdr->translate[2])
                 },
    .eye = {.X = nd_f32(rhdr->eye_position[0]),
            .Y = nd_f32(rhdr->eye_position[1]),
            .Z = nd_f32(rhdr->eye_position[2])
           },
  };

  MD1Header* hdr = &md1->header;
  MD1ST* st_coords = NULL;
  MD1FacedTriangle* ftris = NULL;

  Dbg("header/vertices: %d", hdr->vertices_length);
  Dbg("header/triangles: %d", hdr->triangles_length);
  Dbg("header/frames: %d", hdr->frames_length);
  Dbg("header/skins: %d", hdr->skins_length);
  Dbg("header/skin size: %d x %d", hdr->skin_width, hdr->skin_height);

  p += sizeof(MD1RawHeader);
  p = md1_load_skins(md1, p);
  p = md1_load_st(md1, p, &st_coords);
  p = md1_load_triangles(md1, p, hdr, &ftris);
  p = md1_load_frames(md1, p);

  md1_scale_translate_bbox(md1);
  md1_make_display_list(md1, st_coords, ftris);
  return MD1_ERR_SUCCESS;
}

void
md1_unload(MD1* md1) {
  for (U32 i = 0; i < md1->header.skins_length; i++) {
    sg_destroy_image(md1->skins[i].image);
    sg_destroy_sampler(md1->skins[i].sampler);
    // snk_destroy_image(md1->skins[i].ui_image);
  }

  if(md1->arena) {
    arena_release(md1->arena);
  }
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MD1_IMPLEMENTATION
#endif  // MD1_HEADER_
