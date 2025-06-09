#ifndef QK1_MD1_HEADER_
#define QK1_MD1_HEADER_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <ctype.h>

#include "../deps/hmm.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sepi_macros.h"
#include "../deps/sepi_types.h"
#include "../deps/sepi_endian.h"
#include "../deps/sepi_arena.h"

#define MAX_STATIC_MEM 8192
#define MAX_FRAME_NAME_LEN 16

typedef f32 qk1_md1_vectorf;

typedef qk1_md1_vectorf qk1_md1_vector3f[3];

typedef i32 qk1_md1_triangle[3];

typedef u8 qk1_md1_raw_vertex[3];

typedef enum {
  MD1_ST_UNKNOWN = -1,
  MD1_ST_SYNC = 0,
  MD1_ST_RAND,
  MD1_ST_FRAMETIME,
} qk1_md1_sync_type;

typedef enum {
  MD1_SKIN_UNKNOWN = -1,
  MD1_SKIN_SINGLE = 0,
  MD1_SKIN_GROUP,
} qk1_md1_skin_type;

typedef enum {
  MD1_FT_UNKNOWN = -1,
  MD1_FT_SINGLE = 0,
  MD1_FT_GROUP,
} qk1_md1_frame_type;

typedef struct {
  i32 magic_codes;
  i32 version;
  qk1_md1_vector3f scale;
  qk1_md1_vector3f translate;
  f32 bounding_radius;
  qk1_md1_vector3f eye_position;
  i32 skins_length;
  i32 skin_width;
  i32 skin_height;
  i32 vertices_length;
  i32 triangles_length;
  i32 frames_length;
  qk1_md1_sync_type sync_type;
  i32 flags;
  f32 size;
} qk1_md1_raw_header;

typedef struct {
  i32 onseam;
  i32 s;
  i32 t;
} qk1_md1_st;

typedef struct {
  i32 frontface;
  qk1_md1_triangle vertices_idx;
} qk1_md1_faced_triangle;

typedef struct {
  qk1_md1_raw_vertex vertex;
  u8 normal_idx;
} qk1_md1_normal_vertex;

typedef struct {
  qk1_md1_normal_vertex bbox_min;
  qk1_md1_normal_vertex bbox_max;
  char name[16];
} md1_frame_single;

typedef struct {
  i32 frames_length;
  qk1_md1_normal_vertex bbox_min;
  qk1_md1_normal_vertex bbox_max;
} qk1_md1_frames_group;

typedef enum {
  MD1_ERR_UNKNOWN = -1,
  MD1_ERR_SUCCESS = 0,
  MD1_ERR_FILE_OPEN,
  MD1_ERR_MEM_ALLOC,
  MD1_ERR_READ_SIZE,
  MD1_ERR_RAME_IDX,
  MD1_ERR_INVALID,
} qk1_md1_error;

typedef struct {
  f32 radius;
  u32 skin_width;
  u32 skin_height;
  u32 skins_length;
  u32 vertices_length;
  u32 triangles_length;
  u32 frames_length;
  u32 poses_length;
  u32 vbuf_length;
  hmm_v3 scale;
  hmm_v3 translate;
  hmm_v3 eye;
  hmm_v3 bbox_min;
  hmm_v3 bbox_max;
} qk1_md1_header;

typedef struct {
  hmm_v3 vertex;
  hmm_v3 normal;
} qk1_md1_vertex;

typedef struct {
  char name[MAX_FRAME_NAME_LEN];
  u32 start;
  u32 frames_length;
} qk1_md1_pose;

typedef struct {
  sg_image image;
  sg_sampler sampler;
  snk_image_t ui_image;
} qk1_md1_skin;

typedef struct {
  qk1_md1_header header;
  qk1_md1_skin* skins;
  qk1_md1_vertex* vertices;
  qk1_md1_pose* poses;
  f32* vbuf;
  arena mem;
} qk1_md1_model;

/* ****************** quake::mdl API ****************** */
qk1_md1_error qk_load_mdl(const u8*, sz, qk1_md1_model*);
void qk_get_frame_vertices(const qk1_md1_model*, u32, u32, const f32**, u32*);
void qk_unload_mdl(qk1_md1_model*);
/* ****************** quake::mdl API ****************** */

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

#ifdef QK1_MD1_IMPLEMENTATION

#include "qk1_md1_data.h"

const int MAGICCODE = (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I');
const int MD0VERSION = 6;
const int MAXSKINHEIGHT = 480;
const int MAXVERTICES = 2000;
const int MAXTRIANGLES = 4096;
const int MAXSKINS = 32;

extern const u8 _qk_palette[256][3];
extern const f32 _qk_normals[162][3];

static bool md1_pose_changed(char* new, char* old) {
  for (i32 i = 0; i < MAX_FRAME_NAME_LEN - 1; i++) {
    if (isdigit(new[i])) {
      new[i] = 0;
    }
  }

  if (strlen(old) <= 0) {
    return false;
  }

  if (strcmp(new, old) == 0) {
    return false;
  }

  return true;
}

static void md1_estimate_memory(arena* mem,
                                const qk1_md1_raw_header* rhdr,
                                sz bufsz,
                                qk1_md1_header* hdr) {
  DBG("trying to estimate required memory");
  arena_begin_estimate(mem);

  const u32 frm_len = hdr->frames_length;
  const u32 tri_len = hdr->triangles_length;
  const u32 vrt_len = hdr->vertices_length;
  const u32 skn_len = hdr->skins_length;
  const u32 skn_wdt = hdr->skin_width;
  const u32 skn_hgt = hdr->skin_height;

  // Estimate memory for skins texture
  sz skin_sz = sizeof(qk1_md1_skin) * skn_len;
  arena_estimate_add(mem, skin_sz, alignof(qk1_md1_skin));

  // Estimate memory for skins pixels
  sz pixel_sz = skn_len * skn_wdt * skn_hgt * sizeof(u8) * 4;
  arena_estimate_add(mem, pixel_sz, alignof(u8));

  // Estimate memory for texture UVs
  sz texcoord_sz = sizeof(qk1_md1_st) * vrt_len;
  arena_estimate_add(mem, texcoord_sz, alignof(qk1_md1_st));

  // Estimate memory for triangle indices
  sz trisix_sz = sizeof(qk1_md1_faced_triangle) * tri_len;
  arena_estimate_add(mem, trisix_sz, alignof(qk1_md1_faced_triangle));

  const u8* p = (const u8*)rhdr + sizeof(qk1_md1_raw_header);
  const u8* pend = (const u8*)rhdr + bufsz;

  // Advance past skin data
  for (u32 i = 0; i < hdr->skins_length; i++) {
    qk1_md1_skin_type* skin_type = (qk1_md1_skin_type*)p;
    p += sizeof(qk1_md1_skin_type);
    if (*skin_type == MD1_SKIN_SINGLE) {
      p += skn_wdt * skn_hgt;
    } else {
      mustdie("sqv does not support multi skin YET!");
    }
  }

  // Advance past texture UVs
  p += sizeof(qk1_md1_st) * vrt_len;
  // Advance past triangle indices
  p += sizeof(qk1_md1_faced_triangle) * tri_len;

  char fname[MAX_FRAME_NAME_LEN] = {0};
  char fnameold[MAX_FRAME_NAME_LEN] = {0};
  u32 pos_len = 0;

  for (u32 i = 0; i < frm_len; i++) {
    if (p + sizeof(qk1_md1_frame_type) > pend) {
      mustdie("failed to parse frames data");
    }

    qk1_md1_frame_type ft = *(const qk1_md1_frame_type*)p;
    p += sizeof(qk1_md1_frame_type);

    if (ft == MD1_FT_SINGLE) {
      if (p + sizeof(md1_frame_single) +
              (vrt_len * sizeof(qk1_md1_normal_vertex)) >
          pend) {
        mustdie("failed to parse frames data");
      }

      const char* pname = ((md1_frame_single*)p)->name;

      memcpy(fname, pname, MAX_FRAME_NAME_LEN);
      fname[MAX_FRAME_NAME_LEN - 1] = '\0';

      if (md1_pose_changed(fname, fnameold))
        pos_len++;

      memcpy(fnameold, fname, MAX_FRAME_NAME_LEN);
      fnameold[MAX_FRAME_NAME_LEN - 1] = '\0';

      if (i == frm_len - 1)
        pos_len++;

      p += sizeof(md1_frame_single) + (vrt_len * sizeof(qk1_md1_normal_vertex));
    } else {
      mustdie("sqv does not support group frames YET!");
    }
  }

  hdr->poses_length = pos_len;

  // Add raw vertices memory
  sz raw_verts_sz = sizeof(qk1_md1_vertex) * vrt_len * frm_len;
  arena_estimate_add(mem, raw_verts_sz, alignof(qk1_md1_vertex));

  // Add pose memory estimation
  sz poses_sz = sizeof(qk1_md1_pose) * pos_len;
  arena_estimate_add(mem, poses_sz, alignof(qk1_md1_pose));

  // Add processed vertices memory estimation
  u32 elm_len = 3 * (3 + 2);  // a->b->c * x,y,z, u,v
  sz verts_sz = sizeof(f32) * frm_len * tri_len * elm_len;
  arena_estimate_add(mem, verts_sz, alignof(f32));

  // Add indices memory estimation
  sz inds_sz = sizeof(u32) * tri_len * 3;
  arena_estimate_add(mem, inds_sz, alignof(u32));

  // Finalize estimation
  arena_end_estimate(mem);
  DBG("memory size needed: %d bytes", mem->estimate);
}

static void md1_load_image(const u8* p, u8* pixels, sz size) {
  DBG("loading skin image data");
  u8* indices = (u8*)p;
  for (sz i = 0, j = 0; i < size; i++, j += 4) {
    u32 index = indices[i];
    pixels[j + 0] = _qk_palette[index][0];  // red
    pixels[j + 1] = _qk_palette[index][1];  // green
    pixels[j + 2] = _qk_palette[index][2];  // blue
    pixels[j + 3] = 255;                    // alpha, always opaque
  }
}

static const u8* md1_load_skins(qk1_md1_model* mdl, const u8* p) {
  const qk1_md1_header* hdr = &mdl->header;
  arena* mem = &mdl->mem;
  u32 width = hdr->skin_width;
  u32 height = hdr->skin_height;
  sz skin_size = width * height;

  sz memsz = sizeof(qk1_md1_skin) * hdr->skins_length;
  qk1_md1_skin* skins =
      (qk1_md1_skin*)arena_alloc(mem, memsz, alignof(qk1_md1_skin));
  notnull(skins);

  for (sz i = 0; i < hdr->skins_length; i++) {
    DBG("loading skins #%d", i);
    qk1_md1_skin_type* skin_type = (qk1_md1_skin_type*)p;
    if (*skin_type == MD1_SKIN_SINGLE) {
      p += sizeof(qk1_md1_skin_type);

      sz pixel_sz = sizeof(u8) * skin_size * 4;
      u8* pixels = (u8*)arena_alloc(mem, pixel_sz, alignof(u8));
      notnull(pixels);

      md1_load_image(p, pixels, skin_size);

      skins[i].image = sg_alloc_image();
      skins[i].sampler = sg_make_sampler(&(sg_sampler_desc){
          .min_filter = SG_FILTER_LINEAR,
          .mag_filter = SG_FILTER_LINEAR,
      });
      skins[i].ui_image = snk_make_image(&(snk_image_desc_t){
          .image = skins[i].image,
          .sampler = skins[i].sampler,
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
      mustdie("load_skin() does not support multi skin YET!");
    }
  }

  mdl->skins = skins;
  return p;
}

static const u8* md1_load_st(qk1_md1_model* mdl,
                             const u8* p,
                             qk1_md1_st** coords) {
  DBG("loading texture S/T coordinates");
  arena* mem = &mdl->mem;
  const qk1_md1_header* hdr = &mdl->header;
  sz mem_sz = sizeof(qk1_md1_st) * hdr->vertices_length;
  *coords = (qk1_md1_st*)arena_alloc(mem, mem_sz, alignof(qk1_md1_st));
  notnull(coords);

  const qk1_md1_st* src = (const qk1_md1_st*)p;

  for (sz i = 0; i < hdr->vertices_length; i++) {
    (*coords)[i].onseam = endian_i32(src->onseam);
    (*coords)[i].s = endian_i32(src->s);
    (*coords)[i].t = endian_i32(src->t);
    src++;  // Move forward correctly
  }

  return (const u8*)src;
}

static const u8* md1_load_triangles(qk1_md1_model* mdl,
                                    const u8* p,
                                    const qk1_md1_header* hdr,
                                    qk1_md1_faced_triangle** ftris) {
  DBG("loading triangles");
  arena* mem = &mdl->mem;
  sz mem_sz = sizeof(qk1_md1_faced_triangle) * hdr->triangles_length;
  *ftris = (qk1_md1_faced_triangle*)arena_alloc(
      mem, mem_sz, alignof(qk1_md1_faced_triangle));
  notnull(ftris);

  const qk1_md1_faced_triangle* src =
      (const qk1_md1_faced_triangle*)p;  // Use separate pointer

  for (sz i = 0; i < hdr->triangles_length; i++) {
    (*ftris)[i].frontface = endian_i32(src->frontface);
    for (sz j = 0; j < 3; j++) {
      (*ftris)[i].vertices_idx[j] = endian_i32(src->vertices_idx[j]);
    }
    src++;  // Move forward correctly
  }

  return (const u8*)src;
}

static const u8* md1_load_single_frame(qk1_md1_model* mdl,
                                       u32 frm_idx,
                                       u32* pos_len,
                                       u32* pos_idx,
                                       char* oldname,
                                       const u8* p) {
  const qk1_md1_header* hdr = &mdl->header;
  md1_frame_single* snl = (md1_frame_single*)p;

  char name[16] = {0};
  memcpy(name, snl->name, sizeof(name) - 1);
  DBG("loading single frame #%d from pose: %s", frm_idx, name);

  if (md1_pose_changed(name, oldname)) {
    memcpy(mdl->poses[*pos_idx].name, oldname, MAX_FRAME_NAME_LEN);
    mdl->poses[*pos_idx].frames_length = *pos_len;
    mdl->poses[(*pos_idx) + 1].start = frm_idx;
    *pos_idx = *pos_idx + 1;
    *pos_len = 0;
  }

  *pos_len = *pos_len + 1;
  memcpy(oldname, name, MAX_FRAME_NAME_LEN);
  oldname[MAX_FRAME_NAME_LEN - 1] = '\0';

  if (frm_idx + 1 >= hdr->frames_length) {
    memcpy(mdl->poses[*pos_idx].name, oldname, MAX_FRAME_NAME_LEN);
    mdl->poses[*pos_idx].frames_length = *pos_len;
    mdl->poses[(*pos_idx) + 1].start = frm_idx;
  }

  for (u8 i = 0; i < 3; i++) {
    mdl->header.bbox_min.Elements[i] =
        HMM_MIN(snl->bbox_min.vertex[i], mdl->header.bbox_min.Elements[i]);

    mdl->header.bbox_max.Elements[i] =
        HMM_MAX(snl->bbox_max.vertex[i], mdl->header.bbox_max.Elements[i]);
  }

  qk1_md1_normal_vertex* raw_verts = (qk1_md1_normal_vertex*)(snl + 1);
  qk1_md1_vertex* frmverts = mdl->vertices + (hdr->vertices_length * frm_idx);

  for (u32 i = 0; i < hdr->vertices_length; i++) {
    frmverts[i].vertex = (hmm_v3){
        raw_verts[i].vertex[0], raw_verts[i].vertex[1], raw_verts[i].vertex[2]};
    frmverts[i].normal = (hmm_v3){_qk_normals[raw_verts[i].normal_idx][0],
                                  _qk_normals[raw_verts[i].normal_idx][1],
                                  _qk_normals[raw_verts[i].normal_idx][2]};
  }
  return (const u8*)(raw_verts + hdr->vertices_length);
}

static const u8* md1_load_frames(qk1_md1_model* mdl, const u8* p) {
  DBG("loading frames");
  arena* mem = &mdl->mem;
  const qk1_md1_header* hdr = &mdl->header;
  u32 frames_length = hdr->frames_length;
  u32 verts_length = hdr->vertices_length;

  sz verts_sz = sizeof(qk1_md1_vertex) * verts_length * frames_length;
  mdl->vertices =
      (qk1_md1_vertex*)arena_alloc(mem, verts_sz, alignof(qk1_md1_vertex));
  notnull(mdl->vertices);

  sz poses_sz = sizeof(qk1_md1_pose) * mdl->header.poses_length;
  mdl->poses = (qk1_md1_pose*)arena_alloc(mem, poses_sz, alignof(qk1_md1_pose));
  notnull(mdl->poses);

  u32 pos_idx = 0;
  u32 pos_len = 0;
  char oldname[MAX_FRAME_NAME_LEN] = {0};

  mdl->poses[0].start = 0;

  for (u32 i = 0; i < mdl->header.frames_length; i++) {
    qk1_md1_frame_type ft = endian_i32(*(qk1_md1_frame_type*)p);
    p += sizeof(qk1_md1_frame_type);

    if (ft == MD1_FT_SINGLE) {
      p = md1_load_single_frame(mdl, i, &pos_len, &pos_idx, oldname, p);
    } else {
      mustdie("md1_load_frames() does not support multi frames YET!");
    }
  }

  return p;
}

static void md1_make_display_list(qk1_md1_model* mdl,
                                  const qk1_md1_st* coords,
                                  const qk1_md1_faced_triangle* ftris) {
  DBG("generating vertex buffer data");
  arena* mem = &mdl->mem;
  qk1_md1_header* hdr = &mdl->header;
  hmm_v3* scl = &hdr->scale;
  hmm_v3* trn = &hdr->translate;
  hmm_v3* bbx_min = &hdr->bbox_min;
  hmm_v3* bbx_max = &hdr->bbox_max;
  const u32 skn_wdt = mdl->header.skin_width;
  const u32 skn_hgt = mdl->header.skin_height;
  const u32 vrt_len = hdr->vertices_length;
  const u32 frm_len = hdr->frames_length;
  const u32 tri_len = hdr->triangles_length;

  u32 elm_len = 3 * (3 + 2);  // a->b->c * x,y,z, u,v
  sz vbuf_sz = sizeof(f32) * frm_len * tri_len * elm_len;
  f32* vbuf = (f32*)arena_alloc(mem, vbuf_sz, alignof(f32));
  notnull(vbuf);

  u32 idx = 0;
  for (u32 frm_idx = 0; frm_idx < frm_len; frm_idx++) {
    for (u32 tri_idx = 0; tri_idx < tri_len; tri_idx++) {
      for (u8 vrt_idx = 0; vrt_idx < 3; vrt_idx++) {
        i32 tri_abc = ftris[tri_idx].vertices_idx[vrt_idx];

        const qk1_md1_vertex* vrts = mdl->vertices + (vrt_len * frm_idx);

        f32 x = (vrts[tri_abc].vertex.X * scl->X) + trn->X;
        f32 y = (vrts[tri_abc].vertex.Y * scl->Y) + trn->Y;
        f32 z = (vrts[tri_abc].vertex.Z * scl->Z) + trn->Z;

        f32 s = coords[tri_abc].s;
        f32 t = coords[tri_abc].t;

        if (!ftris[tri_idx].frontface && coords[tri_abc].onseam)
          s += skn_wdt / 2;

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

  mdl->vbuf = vbuf;
  hdr->vbuf_length = elm_len * tri_len;
  DBG("generated vertex buffer with %d items", frm_len * tri_len * elm_len);
}

void md1_scale_translate_bbox(qk1_md1_model* mdl) {
  DBG("scaling and translating bbox data");

  qk1_md1_header* hdr = &mdl->header;
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

void qk_get_frame_vertices(const qk1_md1_model* mdl,
                           u32 pos_idx,
                           u32 frm_idx,
                           const f32** vbuf,
                           u32* vbuf_len) {
  makesure(pos_idx < mdl->header.poses_length, "invalid pose index");
  makesure(frm_idx < mdl->poses[pos_idx].frames_length,
           "invalid frame index in pose");

  qk1_md1_pose* pos = &mdl->poses[pos_idx];
  *vbuf = &mdl->vbuf[(pos->start + frm_idx) * mdl->header.vbuf_length];
  *vbuf_len = mdl->header.vbuf_length;
}

qk1_md1_error qk_load_mdl(const u8* buf, sz bufsz, qk1_md1_model* mdl) {
  const u8* p = buf;
  const qk1_md1_raw_header* rhdr = (qk1_md1_raw_header*)buf;

  memset(mdl, 0, sizeof(qk1_md1_model));

  DBG("loading MD1 haeder ...");
  mdl->header = (qk1_md1_header){
      .radius = endian_f32(rhdr->bounding_radius),
      .skin_width = endian_i32(rhdr->skin_width),
      .skin_height = endian_i32(rhdr->skin_height),
      .skins_length = endian_i32(rhdr->skins_length),
      .vertices_length = endian_i32(rhdr->vertices_length),
      .triangles_length = endian_i32(rhdr->triangles_length),
      .frames_length = endian_i32(rhdr->frames_length),
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

  qk1_md1_header* hdr = &mdl->header;
  arena* mem = &mdl->mem;
  qk1_md1_st* stcoords = NULL;
  qk1_md1_faced_triangle* ftris = NULL;

  DBG("header/vertices: %d", hdr->vertices_length);
  DBG("header/triangles: %d", hdr->triangles_length);
  DBG("header/frames: %d", hdr->frames_length);
  DBG("header/skins: %d", hdr->skins_length);
  DBG("header/skin size: %d x %d", hdr->skin_width, hdr->skin_height);

  md1_estimate_memory(mem, rhdr, bufsz, hdr);

  p += sizeof(qk1_md1_raw_header);
  p = md1_load_skins(mdl, p);
  p = md1_load_st(mdl, p, &stcoords);
  p = md1_load_triangles(mdl, p, hdr, &ftris);
  p = md1_load_frames(mdl, p);

  md1_scale_translate_bbox(mdl);
  md1_make_display_list(mdl, stcoords, ftris);
  return MD1_ERR_SUCCESS;
}

void qk_unload_mdl(qk1_md1_model* mdl) {
  for (u32 i = 0; i < mdl->header.skins_length; i++) {
    sg_destroy_image(mdl->skins[i].image);
    sg_destroy_sampler(mdl->skins[i].sampler);
    snk_destroy_image(mdl->skins[i].ui_image);
  }
  arena_destroy(&mdl->mem);
}

#endif  // QK1_MD1_IMPLEMENTATION
#endif  // QK1_MD1_HEADER_
