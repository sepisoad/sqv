#define UTILS_ARENA_IMPLEMENTATION
#define UTILS_ENDIAN_IMPLEMENTATION
#define UTILS_IO_IMPLEMENTATION
#define MD1_IMPLEMENTATION

#include <stdbool.h>
#include <stdio.h>

#include "../deps/hmm.h"
#include "../deps/log.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_args.h"
#include "../deps/sokol_gfx.h"
#include "../deps/nuklear.h"
#include "../deps/sokol_nuklear.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_time.h"
#include "../deps/sokol_log.h"

#include "glsl/default.h"
#include "quake/md1.h"
#include "utils/types.h"
#include "app.h"

#define MAX_INIT_DELAY 10
#define ROT_FACTOR 0.5;

static context3d ctx3d = {0};
static state s;
static u64 init_tm = 0;

void load_3d_model(cstr path, qk_model* m);
void unload_3d_model(qk_model* m);
void draw_3d(state* s);
void draw_ui(state* s);

void set_skin(u32 idx) {
  makesure(idx <= s.mdl.header.skins_length, "invalid skin index");
  s.bind.images[IMG_tex] = s.mdl.skins[idx].image;
  s.bind.samplers[SMP_smp] = s.mdl.skins[idx].sampler;
}

static void reset_state() {
  s.mdl_pos = 0;
  s.mdl_skn = 0;
}

static void update_offscreen_target(int width, int height) {
  if (sg_isvalid() && s.ctx3d->color_img.id != SG_INVALID_ID) {
    snk_destroy_image(s.ctx3d->nk_img);
    sg_destroy_attachments(s.ctx3d->atts);
    sg_destroy_image(s.ctx3d->depth_img);
    sg_destroy_image(s.ctx3d->color_img);
  }

  s.ctx3d->width = width > 0 ? width : DEFAULT_WIDTH;
  s.ctx3d->height = height > 0 ? height : DEFAULT_HEIGHT;

  s.ctx3d->color_img = sg_make_image(&(sg_image_desc){
      .render_target = true,
      .width = s.ctx3d->width,
      .height = s.ctx3d->height,
      .pixel_format = SG_PIXELFORMAT_RGBA8,
      .sample_count = 1,
  });

  s.ctx3d->depth_img = sg_make_image(&(sg_image_desc){
      .render_target = true,
      .width = s.ctx3d->width,
      .height = s.ctx3d->height,
      .pixel_format = SG_PIXELFORMAT_DEPTH,
      .sample_count = 1,
  });

  s.ctx3d->atts = sg_make_attachments(&(sg_attachments_desc){
      .colors[0].image = s.ctx3d->color_img,
      .depth_stencil.image = s.ctx3d->depth_img,
  });

  s.ctx3d->nk_img = snk_make_image(&(snk_image_desc_t){
      .image = s.ctx3d->color_img,
      .sampler = s.ctx3d->sampler,
  });

  s.ctx3d->pass_action = (sg_pass_action){
      .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.25f, 0.5f, 0.75f, 1.0f}},
  };
}

static void create_offscreen_taget(cstr path) {
  if (s.ctx3d != NULL) {
    unload_3d_model(&s.mdl);
    sg_destroy_pipeline(s.pip);
    sg_destroy_shader(s.shd);
    reset_state();
    update_offscreen_target(sapp_width(), sapp_height());
  }

  load_3d_model(path, &s.mdl);
  s.ctx3d = &ctx3d;  // resetting

  // SEPI: we should move this to another location
  const f32* vb = NULL;
  u32 vb_len = 0;
  qk_get_frame_vertices(&s.mdl, s.mdl_pos, s.mdl_frm, &vb, &vb_len);

  s.shd = sg_make_shader(cube_shader_desc(sg_query_backend()));
  sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .data = {.ptr = vb, .size = vb_len * sizeof(f32)},
  });

  s.pip = sg_make_pipeline(&(sg_pipeline_desc){
      .layout = {.attrs =
                     {
                         [ATTR_cube_position].format = SG_VERTEXFORMAT_FLOAT3,
                         [ATTR_cube_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
                     }},
      .shader = s.shd,
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .cull_mode = SG_CULLMODE_BACK,
      .depth = {
          .write_enabled = true,
          .compare = SG_COMPAREFUNC_LESS_EQUAL,
          .pixel_format = SG_PIXELFORMAT_DEPTH,
      }});

  s.bind = (sg_bindings){.vertex_buffers[0] = vbuf};
  s.bind.images[IMG_tex] = s.mdl.skins[s.mdl_skn].image;
  s.bind.samplers[SMP_smp] = s.mdl.skins[s.mdl_skn].sampler;

  // Create a persistent sampler for the Nuklear image
  s.ctx3d->sampler = sg_make_sampler(&(sg_sampler_desc){
      .min_filter = SG_FILTER_LINEAR,
      .mag_filter = SG_FILTER_LINEAR,
      .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
      .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
  });

  // Initialize offscreen render target with window size
  update_offscreen_target(sapp_width(), sapp_height());

  s.pass_action = (sg_pass_action){
      .colors[0] =
          {
              .load_action = SG_LOADACTION_CLEAR,
              .clear_value = {0.1f, 0.1f, 0.1f, 1.0f},
          },
  };
}

static cstr mode_str(mode m) {
  switch (m) {
    case MODE_INIT:
      return "INIT";
    case MODE_NORMAL:
      return "NORMAL";
    case MODE_HELP:
      return "HELP";
    case MODE_INFO:
      return "INFO";
    case MODE_SKINS:
      return "SKINS";
    case MODE_POSES:
      return "POSES";
    default:
      return "UNKNOWN";
  }
}

static void set_mode(mode m) {
  if (s.m != m) {
    DBG("changing mode from '%s' to '%s'", mode_str(s.m), mode_str(m));
    s.m = m;
  }
}

static void update(void) {
  if (s.rotating) {
    s.mdl_roty += ROT_FACTOR;
  }
}

static void frame(void) {
  draw_3d(&s);
  draw_ui(&s);
  update();

  if (s.m == MODE_INIT && stm_sec(stm_since(init_tm)) > MAX_INIT_DELAY) {
    set_mode(MODE_NORMAL);
  }
}

static void mode_init_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    set_mode(MODE_NORMAL);
  }
}

static void mode_normal_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_SLASH:
        if (e->modifiers & SAPP_MODIFIER_SHIFT) {
          set_mode(MODE_HELP);
        }
        break;
      case SAPP_KEYCODE_I:
        set_mode(MODE_INFO);
        break;
      case SAPP_KEYCODE_R:
        s.rotating = !s.rotating;
        break;
      case SAPP_KEYCODE_S:
        set_mode(MODE_SKINS);
        break;
      case SAPP_KEYCODE_P:
        set_mode(MODE_POSES);
        break;
      default:
        break;
    }
  }
}

static void mode_info_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_ESCAPE:
        set_mode(MODE_NORMAL);
        break;
      default:
        break;
    }
  }
}

static void mode_help_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_ESCAPE:
        set_mode(MODE_NORMAL);
        break;
      default:
        break;
    }
  }
}

static void mode_skins_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_ESCAPE:
        set_mode(MODE_NORMAL);
        break;
      default:
        break;
    }
  }
}

static void mode_poses_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_ESCAPE:
        set_mode(MODE_NORMAL);
        break;
      default:
        break;
    }
  }
}

static void input(const sapp_event* e) {
  snk_handle_event(e);

  if (e->type == SAPP_EVENTTYPE_RESIZED) {
    update_offscreen_target(e->window_width, e->window_height);
  }

  if (e->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    create_offscreen_taget(sapp_get_dropped_file_path(0));
  }

  switch (s.m) {
    case MODE_INIT:
      mode_init_input(e);
      break;
    case MODE_NORMAL:
      mode_normal_input(e);
      break;
    case MODE_INFO:
      mode_info_input(e);
      break;
    case MODE_HELP:
      mode_help_input(e);
      break;
    case MODE_SKINS:
      mode_skins_input(e);
      break;
    case MODE_POSES:
      mode_poses_input(e);
      break;
    default:
      break;
  }
}

static void cleanup(void) {
  log_info("shutting down");

  qk_unload_mdl(&s.mdl);
  if (sg_isvalid()) {
    snk_destroy_image(s.ctx3d->nk_img);
    sg_destroy_attachments(s.ctx3d->atts);
    sg_destroy_image(s.ctx3d->depth_img);
    sg_destroy_image(s.ctx3d->color_img);
    sg_destroy_sampler(s.ctx3d->sampler);
  }
  snk_shutdown();
  sg_shutdown();
  sargs_shutdown();
}

static void init(void) {
  log_info("initializing gpu ...");

  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  stm_setup();

  snk_setup(&(snk_desc_t){
      .enable_set_mouse_cursor = true,
      .dpi_scale = sapp_dpi_scale(),
      .logger.func = slog_func,
  });

  init_tm = stm_now();
  s.ctx3d = &ctx3d;
  s.m = MODE_INIT;
  s.mdl_pos = 0;
  s.mdl_skn = 0;
  s.rotating = true;

  cstr path = (cstr)sapp_userdata();
  if (path != NULL) {
    log_info("loading '%s' model", path);
    create_offscreen_taget(path);
  }
}

sapp_desc sokol_main(i32 argc, char* argv[]) {
  log_info("starting");

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  cstr mdlpath = NULL;
  if (sargs_exists("-m"))
    mdlpath = sargs_value("-m");
  else if (sargs_exists("--model"))
    mdlpath = sargs_value("--model");

  return (sapp_desc){
      .init_cb = init,
      .cleanup_cb = cleanup,
      .event_cb = input,
      .frame_cb = frame,
      .user_data = (rptr)mdlpath,
      .width = 800,
      .height = 600,
      .sample_count = 1,
      .window_title = "sqv::sepi's quake asset viewer",
      .icon.sokol_default = true,
      .enable_dragndrop = true,
      .max_dropped_files = 1,
      .logger.func = slog_func,
  };
}
