#ifndef SQV_APP_HEADER_
#define SQV_APP_HEADER_

#include "../deps/sokol_gfx.h"

typedef struct nk_context contextui;

typedef enum {
  MODE_UNKNOWN = -1,
  MODE_INIT = 0,
  MODE_NORMAL,
  MODE_INFO,
  MODE_HELP,
  MODE_SKINS,
  MODE_POSES,
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
  sg_shader shd;
  sg_pipeline pip;
  sg_bindings bind;
  sg_pass_action pass_action;
  contextui* ctxui;
  context3d* ctx3d;

  // mdl model states
  qk_model mdl;
  u32 mdl_pos;
  u32 mdl_frm;
  u32 mdl_skn;
  u32 frame_rate;
  f32 mdl_roty;
  f32 zoom;
  bool rotating;
  bool animating;
  mode m;
} state;

#define MAX_INIT_DELAY 10
#define ROT_FACTOR 0.5;
#define FOV 60.0f
#define DEFAULT_WIDTH 512
#define DEFAULT_HEIGHT 512
#define MIN_ZOOM -5.0f
#define MAX_ZOOM 10.0f
#define MIN_FRAME_RATE 1
#define MAX_FRAME_RATE 1000

#endif  // SQV_APP_HEADER_
