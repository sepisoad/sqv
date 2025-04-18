#ifndef SQV_APP_HEADER_
#define SQV_APP_HEADER_

#include "../deps/sokol_gfx.h"

typedef struct nk_context contextui;

typedef enum {
  MODE_UNKNOWN = -1,
  MODE_NORMAL = 0,
  MODE_INFO,
  MODE_HELP,
} mode;

typedef struct {
  sg_image color_img;
  sg_image depth_img;
  sg_attachments atts;
  sg_pass_action pass_action;
  snk_image_t nk_img;
  sg_sampler sampler;
  int width;
  int height;
} context3d;

typedef struct {
  // rendering states
  sg_pipeline pip;
  sg_bindings bind;
  sg_pass_action pass_action;
  contextui* ctxui;
  context3d* ctx3d;

  // mdl model states
  qk_model mdl;
  u32 mdl_pos;
  u32 mdl_frm;
  float mdl_roty;
  mode m;
} state;

#define FOV 60.0f
#define DEFAULT_WIDTH 512
#define DEFAULT_HEIGHT 512

#endif  // SQV_APP_HEADER_
