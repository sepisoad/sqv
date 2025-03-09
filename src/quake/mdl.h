#ifndef QK_MDL_HEADER_
#define QK_MDL_HEADER_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include "../deps/stb_image_write.h"
#endif

#include "../utils/all.h"
#include "../deps/hmm.h"
#include "../deps/vector.h"
#include "../deps/sokol_gfx.h"

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
  qk_raw_vector3f origin;
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

typedef struct {
  int32_t first_pose;
  float interval;
  qk_raw_triangle_vertex bbox_min;
  qk_raw_triangle_vertex bbox_max;
  int32_t frame;
  char name[16];
  Vector* raw_vertices_ptr;
} qk_raw_frame;

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
  Vector* vertices;
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
  hmm_vec3 scale;   // model scale
  hmm_vec3 origin;  // model origin
  hmm_vec3 eye;     // eye position
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
  Vector* frames;
} qk_mdl;

/* ****************** quake::mdl API ****************** */
qk_err qk_load_mdl(const char* path, qk_mdl* _, arena*);
qk_err qk_init(void);
qk_err qk_deinit(qk_mdl* mdl);
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

static void load_mdl_buffer(const char* path, uint8_t** buf) {
  FILE* f = fopen(path, "rb");
  makesure(f != NULL, "invalid mdl file");

  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  rewind(f);

  *buf = (uint8_t*)malloc(sizeof(uint8_t) * fsize);
  makesure(*buf != NULL, "malloc failed");

  size_t rsize = fread(*buf, 1, fsize, f);
  makesure(rsize == fsize, "read size '%zu' did not match the file size '%zu'",
           rsize, fsize);

  if (f) {
    fclose(f);
  }
}

static qk_err load_image(uintptr_t mem,
                         uint8_t** pixels,
                         size_t size,
                         uint32_t width,
                         uint32_t height) {
  uint8_t* indices = (uint8_t*)mem;
  *pixels = (uint8_t*)malloc(size * 4);
  makesure(*pixels != NULL, "malloc failed");

  for (size_t i = 0, j = 0; i < size; i++, j += 4) {
    uint32_t index = indices[i];
    (*pixels)[j + 0] = _qk_palette[index][0];  // red
    (*pixels)[j + 1] = _qk_palette[index][1];  // green
    (*pixels)[j + 2] = _qk_palette[index][2];  // blue
    (*pixels)[j + 3] = 255;                    // alpha, always opaque
  }

  return QK_ERR_SUCCESS;
}

static uintptr_t load_skins(const qk_header* hdr,
                            qk_skin** skins,
                            uintptr_t ptr) {
  qk_skintype* skin_type = NULL;
  uint32_t width = hdr->skin_width;
  uint32_t height = hdr->skin_height;
  uint32_t skin_size = width * height;

  *skins = (qk_skin*)malloc(sizeof(qk_skin) * hdr->skins_count);
  makesure(*skins != NULL, "malloc failed");

  for (size_t i = 0; i < hdr->skins_count; i++) {
    skin_type = (qk_skintype*)ptr;
    if (*skin_type == QK_SKIN_SINGLE) {
      uint8_t* pixels = NULL;
      ptr += sizeof(qk_skintype);
      qk_err err = load_image(ptr, &pixels, skin_size, width, height);
      makesure(err == QK_ERR_SUCCESS, "load_image() failed");

#ifdef DEBUG
      char skin_name[1024] = {0};
      sprintf(skin_name, ".ignore/skin_%zu.png", i);
      stbi_write_png(skin_name, width, height, 4, pixels, width * 4);
#endif

      (*skins)[i].image = sg_alloc_image();
      (*skins)[i].sampler = sg_make_sampler(&(sg_sampler_desc){
          .min_filter = SG_FILTER_LINEAR,
          .mag_filter = SG_FILTER_LINEAR,
      });
      sg_init_image((*skins)[i].image,
                    &(sg_image_desc){.width = width,
                                     .height = height,
                                     .pixel_format = SG_PIXELFORMAT_RGBA8,
                                     .data.subimage[0][0] = {
                                         .ptr = pixels,
                                         .size = (size_t)(width * height * 4),
                                     }});
      free(pixels);
      ptr += skin_size;
    } else {
      /*
       * this is not implemented yet, and may never be implemented!
       */
      makesure(false, "load_skin() does not support multi skin YET!");
    }
  }

cleanup:
  return ptr;
}

static uintptr_t load_raw_texture_coordinates(const qk_header* hdr,
                                              qk_raw_texcoord** coords,
                                              uintptr_t ptr) {
  *coords =
      (qk_raw_texcoord*)malloc(sizeof(qk_raw_texcoord) * hdr->vertices_count);
  makesure(*coords != NULL, "malloc failedes");

  for (size_t i = 0; i < hdr->vertices_count; i++) {
    qk_raw_texcoord* ptr = (qk_raw_texcoord*)ptr;
    (*coords)[i].onseam = endian_i32(ptr->onseam);
    (*coords)[i].s = endian_i32(ptr->s);
    (*coords)[i].t = endian_i32(ptr->t);
    ptr += sizeof(qk_raw_texcoord);
  }

  return ptr;
}

static uintptr_t load_raw_triangles_indices(const qk_header* hdr,
                                            qk_raw_triangles_idx** trisidx,
                                            uintptr_t ptr) {
  size_t alloc_size = sizeof(qk_raw_triangles_idx) * hdr->triangles_count;
  *trisidx = (qk_raw_triangles_idx*)malloc(alloc_size);
  makesure(*trisidx != NULL, "malloc failed");

  for (size_t i = 0; i < hdr->triangles_count; i++) {
    qk_raw_triangles_idx* ptr = (qk_raw_triangles_idx*)ptr;
    (*trisidx)[i].frontface = endian_i32(ptr->frontface);
    for (size_t j = 0; j < 3; j++) {
      (*trisidx)[i].vertices_idx[j] = endian_i32(ptr->vertices_idx[j]);
    }
    ptr += sizeof(qk_raw_triangles_idx);
  }

  return ptr;
}

static uintptr_t load_frame_single(const qk_header* hdr,
                                   Vector* frms,
                                   uint32_t* posidx,
                                   uintptr_t ptr) {
  qk_raw_frame_single* snl = (qk_raw_frame_single*)ptr;
  // qk_raw_frame frm;
  qk_frame frm;

  strncpy(frm.name, snl->name, sizeof(snl->name));
  // frm.first_pose = (*posidx);
  (*posidx)++;

  frm.bbox_min.X = snl->bbox_min.vertex[0];
  frm.bbox_min.Y = snl->bbox_min.vertex[1];
  frm.bbox_min.Z = snl->bbox_min.vertex[2];

  frm.bbox_max.X = snl->bbox_max.vertex[0];
  frm.bbox_max.Y = snl->bbox_max.vertex[1];
  frm.bbox_max.Z = snl->bbox_max.vertex[2];

  frm.vertices = vector_create(sizeof(qk_vertex));
  qk_raw_triangle_vertex* pvert = (qk_raw_triangle_vertex*)(snl + 1);

  qk_vertex vert = {.vertex = {.X = pvert->vertex[0],
                               .Y = pvert->vertex[1],
                               .Z = pvert->vertex[2]},
                    .normal = {.X = _qk_normals[pvert->normal_idx][0],
                               .Y = _qk_normals[pvert->normal_idx][1],
                               .Z = _qk_normals[pvert->normal_idx][2]}};

  vector_push_back(frm.vertices, &vert);
  vector_push_back(frms, &frm);

  ptr = (uintptr_t)(pvert + hdr->vertices_count);
  return ptr;
}

static uintptr_t load_frames_group(const qk_header* hdr,
                                   Vector* frms,
                                   uint32_t* posidx,
                                   uintptr_t ptr) {
  qk_raw_frames_group* grp = (qk_raw_frames_group*)ptr;
  qk_frame frm;

  // frm.first_pose = (*posidx);

  frm.bbox_min.X = grp->bbox_min.vertex[0];
  frm.bbox_min.Y = grp->bbox_min.vertex[1];
  frm.bbox_min.Z = grp->bbox_min.vertex[2];

  frm.bbox_max.X = grp->bbox_max.vertex[0];
  frm.bbox_max.Y = grp->bbox_max.vertex[1];
  frm.bbox_max.Z = grp->bbox_max.vertex[2];

  float* interval_ptr = (float*)(grp + 1);
  // frm.interval = endian_f32(*interval_ptr);

  interval_ptr += grp->frames_count;
  ptr = (uintptr_t)interval_ptr;

  frm.vertices = vector_create(sizeof(qk_vertex*));
  for (uint32_t i = 0; i < grp->frames_count; i++) {
    qk_raw_triangle_vertex* pvert =
        (qk_raw_triangle_vertex*)((qk_raw_frame_single*)ptr + 1);

    qk_vertex vert = {.vertex = {.X = pvert->vertex[0],
                                 .Y = pvert->vertex[1],
                                 .Z = pvert->vertex[2]},
                      .normal = {.X = _qk_normals[pvert->normal_idx][0],
                                 .Y = _qk_normals[pvert->normal_idx][1],
                                 .Z = _qk_normals[pvert->normal_idx][2]}};
    vector_push_back(frm.vertices, &vert);

    ptr = (uintptr_t)(pvert + hdr->vertices_count);
    (*posidx)++;
  }

  vector_push_back(frms, &frm);

  return ptr;
}

static uintptr_t load_raw_frames(qk_header* hdr, Vector* frms, uintptr_t ptr) {
  uint32_t poses_count = 0;

  for (uint32_t i = 0; i < hdr->frames_count; i++) {
    qk_frametype ft = endian_i32(*(qk_frametype*)ptr);
    ptr += sizeof(qk_frametype);
    if (ft == QK_FT_SINGLE) {
      ptr = load_frame_single(hdr, frms, &poses_count, ptr);
    } else {
      // example: progs/flame2.mdl
      ptr = load_frames_group(hdr, frms, &poses_count, ptr);
    }
  }

  hdr->poses_count = poses_count;
  return ptr;
}

static qk_err calc_bounds(qk_header* hdr) {
  // TODO: implement this
  return QK_ERR_SUCCESS;
}

static qk_err make_display_lists(qk_mdl* mdl,
                                 const qk_raw_texcoord* coords,
                                 const qk_raw_triangles_idx* trisidx) {
  /*
   * ok sepi, i don't need the bullshit triangle strips or fans
   * they were used by quake engine because old systems had
   * limmitted memory and cpu bandwith so it would be nice to
   * reduce the number of vertices to be sent to gpu
   * but now with modern gpus and cpus we just don't simply care
   * and the amount of performance gain is negligible
   * so instead what we do is simple, we create a buffer of all
   * triangles in correct order, and also texture uvs in order
   * and we may even merge them together and then return back
   * to sokol and ask it to send it to gpu as 'SG_PRIMITIVETYPE_TRIANGLES'
   * which behind the scene uses 'GL_TRIANGLES' because we use opengl 3.3
   * or basically we don't care about the backend.
   *
   * now we need to
   */

  size_t meshessz = sizeof(qk_mesh) * mdl->header.vertices_count * 2;
  qk_mesh* mesh = (qk_mesh*)malloc(meshessz);
  makesure(mesh != NULL, "malloc failed");

  size_t indicessz = sizeof(uint32_t) * mdl->header.triangles_count * 3;
  uint32_t* indices = (uint32_t*)malloc(indicessz);
  makesure(indices != NULL, "malloc failed");

  uint32_t mesh_count = 0;
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

      mesh[mesh_count].vertex_idx = vertidx;
      mesh[mesh_count].uv.U = s;
      mesh[mesh_count].uv.V = t;

      indices[indices_count++] = vertidx;
    }
  }

  mdl->header.mesh_count = mesh_count;
  mdl->mesh = mesh;

  mdl->header.indices_count = indices_count;
  mdl->indices = indices;

  return QK_ERR_SUCCESS;
}

qk_err qk_load_mdl(const char* path, qk_mdl* _, arena* qk_memory) {
  qk_err err = QK_ERR_SUCCESS;
  qk_mdl mdl = {0};
  uint8_t* buf = NULL;

  load_mdl_buffer(path, &buf);

  uintptr_t ptr = (uintptr_t)buf;
  qk_raw_header* rhdr = (qk_raw_header*)buf;
  int32_t mdl_mcode = endian_i32(rhdr->magic_codes);
  int32_t mdl_version = endian_i32(rhdr->version);
  int32_t mdl_flags = endian_i32(rhdr->flags);
  int32_t mdl_size = endian_f32(rhdr->size);

  mdl.header = (qk_header){
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
      .origin = {.X = endian_f32(rhdr->origin[0]),
                 .Y = endian_f32(rhdr->origin[1]),
                 .Z = endian_f32(rhdr->origin[2])},
      .eye = {.X = endian_f32(rhdr->eye_position[0]),
              .Y = endian_f32(rhdr->eye_position[1]),
              .Z = endian_f32(rhdr->eye_position[2])},
  };

  makesure(mdl_mcode == MAGICCODE, "wrong magic code");
  makesure(mdl_version == MD0VERSION, "wrong version");
  makesure(mdl.header.skin_height <= MAXSKINHEIGHT, "invalid skin height");
  makesure(mdl.header.vertices_count <= MAXVERTICES, "invalid vertices count ");
  makesure(mdl.header.triangles_count <= MAXTRIANGLES,
           "invalid triangles count");
  makesure(mdl.header.skins_count <= MAXSKINS, "invalid skins count");
  makesure(mdl.header.vertices_count > 0, "invalid vertices count");
  makesure(mdl.header.triangles_count > 0, "invalid triangles count");
  makesure(mdl.header.frames_count > 0, "invalid frames count");
  makesure(mdl.header.skins_count > 0, "invalid skins count");

  /* TODO: free these memories */
  qk_skin* skins = NULL;
  qk_raw_texcoord* raw_tex_coords = NULL;
  qk_raw_triangles_idx* raw_tris_idx = NULL;
  mdl.frames = vector_create(sizeof(qk_raw_frame));

  ptr += sizeof(qk_raw_header);
  ptr = load_skins(&mdl.header, &skins, ptr);
  ptr = load_raw_texture_coordinates(&mdl.header, &raw_tex_coords, ptr);
  ptr = load_raw_triangles_indices(&mdl.header, &raw_tris_idx, ptr);
  ptr = load_raw_frames(&mdl.header, mdl.frames, ptr);

  err = calc_bounds(&mdl.header);
  makesure(err == QK_ERR_SUCCESS, "calc_bound() failed");

  err = make_display_lists(&mdl, raw_tex_coords, raw_tris_idx);
  makesure(err == QK_ERR_SUCCESS, "make_display_lists() failed");

cleanup:
  if (buf) {
    free(buf);
  }

  if (raw_tex_coords) {
    free(raw_tex_coords);
  }

  if (raw_tris_idx) {
    free(raw_tris_idx);
  }

  return err;
}

qk_err qk_init(void) {
  return QK_ERR_SUCCESS;
}

qk_err qk_deinit(qk_mdl* mdl) {
  if (mdl->skins) {
    free(mdl->skins);
    mdl->skins = NULL;
  }

  for (uint32_t i = 0; i < mdl->header.frames_count; i++) {
    qk_frame* frame = (qk_frame*)vector_at(mdl->frames, i);
    Vector* vector = frame->vertices;
    vector_deallocate(vector);
  }

  if (mdl->frames) {
    vector_deallocate(mdl->frames);
  }

  return QK_ERR_SUCCESS;
}

#endif  // QK_MDL_IMPLEMENTATION
#endif  // QK_MDL_HEADER_
