#pragma once

#include <stdint.h>

typedef float mdl_vec3[3];

typedef struct {
  int ident;
  int version;

  mdl_vec3 scale;
  mdl_vec3 translate;
  float boundingradius;
  mdl_vec3 eyeposition;

  int num_skins;
  int skinwidth;
  int skinheight;

  int num_verts;
  int num_tris;
  int num_frames;

  int synctype;
  int flags;
  float size;
} mdl_header;

typedef struct {
  int group;
  uint8_t *data;
} mdl_skin;

typedef struct {
  int onseam;
  int s;
  int t;
} mdl_texcoord;

typedef struct {
  int facesfront;
  int vertex[3];
} mdl_triangle;

typedef struct {
  unsigned char v[3];
  unsigned char normalIndex;
} mdl_vertex;

/* Simple frame */
typedef struct {
  mdl_vertex bboxmin;
  mdl_vertex bboxmax;
  char name[16];
  mdl_vertex *verts;
} mdl_simpleframe;

typedef struct {
  int type;
  mdl_simpleframe frame;
} mdl_frame;

typedef struct {
  mdl_header header;

  mdl_skin *skins;
  mdl_texcoord *texcoords;
  mdl_triangle *triangles;
  mdl_frame *frames;

  uint32_t *tex_id;
} mdl_model;