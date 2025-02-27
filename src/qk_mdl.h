#ifndef QK_MDL_HEADER_
#define QK_MDL_HEADER_

#include <stdint.h>

#include "../deps/hmm.h"
#include "../deps/list.h"
#include "../deps/sokol_gfx.h"

/* RAW QUAKE TYPES AS THEY ARE STORED IN MDL FILE */

typedef float qk_raw_vectorf;

typedef qk_raw_vectorf qk_raw_vector3f[3];

typedef int32_t qk_raw_vertices_idx[3];

typedef uint8_t qk_raw_vertex3[3];

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
  qk_synctype sync_type;
  int32_t flags;
  float size;
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
  List* raw_vertices_ptr;
} qk_raw_frame;

/* PROCESSED QUAKE TYPES, USEFUL AT RUNTIME */

typedef struct {
  hmm_v3 vertex;
  hmm_v3 normal;
} qk_triangle;

typedef struct {
  hmm_v3 bbox_min;
  hmm_v3 bbox_max;
  qk_triangle* vertices;
} qk_frame;

typedef struct {
  uint32_t frames_count;
  qk_frame** frames;
  char name[16];
} qk_pose;

typedef struct {
  uint32_t poses_count;
  qk_pose** poses;
} qk_animation;

typedef struct {
  float radius;  // bounding radius
  uint32_t skin_width;
  uint32_t skin_height;
  uint32_t skins_count;
  uint32_t vertices_count;
  uint32_t triangles_count;
  uint32_t triangles_order_count;
  uint32_t uv_count;
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
  int* uvs;
} qk_texture_uvs;

typedef struct {
  qk_header header;
  qk_skin* skins;
  qk_texture_uvs* uvs;
  // qk_raw_texcoord*      texcoords;
  // qk_raw_triangles_idx* triangles_idx;
  // qk_frames*            frames;
} qk_mdl;

#endif  // QK_MDL_HEADER_
