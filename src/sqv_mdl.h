#ifndef SQV_MDL_HEADER_
#define SQV_MDL_HEADER_

#include "../deps/hmm.h"

typedef struct {
  float radius;    // bounding radius
  int32_t skin_width;
  int32_t skin_height;
  int32_t skins_count;
  int32_t vertices_count;
  int32_t triangles_count;
  int32_t frames_count;
  hmm_vec3 scale;  // model scale
  hmm_vec3 origin; // model origin
  hmm_vec3 eye;    // eye position
} sqv_mdl_header;

typedef struct {

} sqv_mdl;

#endif // SQV_MDL_HEADER_
