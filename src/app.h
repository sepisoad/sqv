#ifndef SQV_APP_HEADER_
#define SQV_APP_HEADER_

#include "../deps/nuklear.h"
#include "../deps/sepi_types.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_nuklear.h"

#include "./md1.h"
#include "./pak.h"

typedef struct nk_context contextui;

#define BIN_FLAG(x) (1 << (x))

typedef enum {
  SQV_ERR_UNKNOWN = -1,
  SQV_ERR_SUCCESS = 0,
} sqv_err;

typedef enum {
  MAJOR_MODE_UNKNOWN = -1,
  MAJOR_MODE_INIT = 0,
  MAJOR_MODE_PAK,
  MAJOR_MODE_MD1,
  MAJOR_MODE_WAD,
  MAJOR_MODE_LMP,
} major_mode;

typedef enum {
  MINOR_MODE_UNKNOWN = BIN_FLAG(0),
  MINOR_MODE_INIT = BIN_FLAG(1),
  MINOR_MODE_HELP = BIN_FLAG(2),
  MINOR_MODE_INFO = BIN_FLAG(3),
  MINOR_MODE_TREE = BIN_FLAG(4),
  MINOR_MODE_SKINS = BIN_FLAG(5),
  MINOR_MODE_POSES = BIN_FLAG(6),
  MINOR_MODE_FRAMES = BIN_FLAG(7),
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
  pak pak;
  md1 mdl;
  u32 mdl_pos;
  u32 mdl_frm;
  u32 mdl_skn;
  i32 frame_rate;
  f32 mdl_roty;
  f32 zoom;
  bool rotating;
  bool animating;
  bool show_cmd;
  kind knd;
  major_mode mjm;
  minor_mode mnm;

  char cmd[2048];
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
