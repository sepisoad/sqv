#pragma once

#include <stdint.h>

typedef float vector[3];

typedef struct {
  int ident;
  int version;

  vector model_scale;
  vector model_translate;
  float bounding_radius;
  vector eye_position;

  int skins_count;
  int skin_width;
  int skin_height;

  int vertices_count;
  int triangles_count;
  int frames_count;

  int sync_type;
  int flags;
  float size;
} header;

typedef struct {
  int is_grouped;  // TODO: make bool
  uint8_t* data;
} skin;

typedef struct {
  int is_on_seam;
  int s;
  int t;
} texture_coordinate;

typedef struct {
  int is_front_front;  // TODO: make bool
  int vertex[3];       // TODO: make a typedef of this array
} triangle;

typedef struct {
  unsigned char v[3];  // TODO: make a typedef of this array
  unsigned char normal_index;
} vertex;

typedef struct {
  vertex bouding_box_min;
  vertex bouding_box_max;
  char name[16];
  vertex* vertices; /* vertex list of the frame */
} simple_frame;

typedef struct {
  int type;
  simple_frame frame;
} frame;

typedef struct {
  header header;
  skin* skins;
  texture_coordinate* texture_coordinates;
  triangle* triangles;
  frame* frames;
  uint8_t* texture_id;
  int is_kin;  // TODO: make bool
} quake_model;
