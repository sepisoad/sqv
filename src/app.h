#ifndef SQV_APP_HEADER_
#define SQV_APP_HEADER_

#include "../deps/sokol_gfx.h"

typedef struct nk_context contextui;

typedef enum {
  SQV_ERR_UNKNOWN = -1,
  SQV_ERR_SUCCESS = 0,
} sqv_err;

typedef enum {
  MAJOR_MODE_UNKNOWN = -1,
  MAJOR_MODE_INIT = 0,
  MAJOR_MODE_NORMAL,
  MAJOR_MODE_INFO,
  MAJOR_MODE_HELP,
  MAJOR_MODE_SKINS,
  MAJOR_MODE_POSES,
} major_mode;

typedef enum {
  MINOR_MODE_UNKNOWN = -1,
  MINOR_MODE_STATUS = 0,
  MINOR_MODE_TOOLBAR,
} minor_mode;

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
  i32 frame_rate;
  f32 mdl_roty;
  f32 zoom;
  bool rotating;
  bool animating;
  major_mode mjm;
  minor_mode mnm;
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
