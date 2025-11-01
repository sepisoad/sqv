#ifndef SQV_APP_HEADER_
#define SQV_APP_HEADER_

#include "deps/nuklear/nuklear.h"
#include "deps/sepi/base.h"
#include "deps/sokol/sokol_app.h"
#include "deps/sokol/sokol_gfx.h"
#include "deps/sokol/sokol_nuklear.h"

#include "md1.h"
#include "pak.h"

typedef struct nk_context ContextUI;

#define BIN_FLAG(x) (1 << (x))

typedef enum {
  SQV_ERR_UNKNOWN = -1,
  SQV_ERR_SUCCESS = 0,
} SQVError;

typedef enum {
  MAJOR_MODE_UNKNOWN = -1,
  MAJOR_MODE_INIT = 0,
  MAJOR_MODE_PAK,
  MAJOR_MODE_MD1,
  MAJOR_MODE_WAD,
  MAJOR_MODE_LMP,
} MajorMode;

typedef enum {
  MINOR_MODE_UNKNOWN = BIN_FLAG(0),
  MINOR_MODE_INIT = BIN_FLAG(1),
  MINOR_MODE_HELP = BIN_FLAG(2),
  MINOR_MODE_INFO = BIN_FLAG(3),
  MINOR_MODE_TREE = BIN_FLAG(4),
  MINOR_MODE_SKINS = BIN_FLAG(5),
  MINOR_MODE_POSES = BIN_FLAG(6),
  MINOR_MODE_FRAMES = BIN_FLAG(7),
} MinorMode;

typedef struct {
  sg_image color_img;
  sg_image depth_img;
  sg_attachments atts;
  sg_pass_action pass_action;
  snk_image_t nk_img;
  sg_sampler sampler;
  int width;
  int height;
} Context3D;

typedef struct {
  // rendering states
  sg_shader shd;
  sg_pipeline pip;
  sg_bindings bind;
  sg_pass_action pass_action;
  ContextUI* ctxui;
  Context3D* ctx3d;

  // mdl model states
  Pak pak;
  MD1 mdl;
  U32 mdl_pos;
  U32 mdl_frm;
  U32 mdl_skn;
  I32 frame_rate;
  F32 mdl_roty;
  F32 zoom;
  Bool rotating;
  Bool animating;
  Bool show_cmd;
  Kind knd;
  MajorMode mjm;
  MinorMode mnm;

  char cmd[2048];
} State;

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
