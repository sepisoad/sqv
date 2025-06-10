#define QK_MD1_IMPLEMENTATION
#define QK_FILES_IMPLEMENTATION

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
#include "../deps/sepi_types.h"
#include "../deps/sepi_alloc.h"
#include "../deps/sepi_endian.h"

#include "glsl_default.h"
#include "qk_md1.h"
#include "app.h"
#include "qk_files.h"

#ifdef DEBUG
#include "../deps/sokol_memtrack.h"
#endif

context3d ctx3d = {0};
static state s;
static u64 init_tm = 0;
static u64 last_frame_tick = 0;

void update_offscreen_target(state* s, int width, int height);
void create_offscreen_target(state* s, cstr path);
void draw_3d(state* s);
void draw_ui(state* s);

void set_skin(u32 idx) {
  makesure(idx <= s.mdl.header.skins_length, "invalid skin index");
  s.bind.images[IMG_tex] = s.mdl.skins[idx].image;
  s.bind.samplers[SMP_smp] = s.mdl.skins[idx].sampler;
}

void reset_state() {
  s.mdl_skn = 0;
  s.mdl_pos = 0;
  s.mdl_frm = 0;
  s.zoom = 1;
  s.frame_rate = 60;
}

// PRIVATE FUNCTIONS

static void next_pose() {
  s.mdl_pos++;
  if (s.mdl_pos >= s.mdl.header.poses_length) {
    s.mdl_pos = 0;
  }
  s.mdl_frm = 0;
}

static void prev_pose() {
  if (s.mdl_pos == 0) {
    s.mdl_pos = s.mdl.header.poses_length - 1;
  } else {
    s.mdl_pos--;
  }
  s.mdl_frm = 0;
}

static void next_frame() {
  s.mdl_frm++;
  if (s.mdl_frm >= s.mdl.poses[s.mdl_pos].frames_length) {
    s.mdl_frm = 0;
  }
}

static void prev_frame() {
  if (((i32)s.mdl_frm - 1) < 0) {
    s.mdl_frm = s.mdl.poses[s.mdl_pos].frames_length - 1;
  } else {
    s.mdl_frm--;
  }
}

static void set_zoom(f32 val) {
  if (val < 0) {
    s.zoom -= 0.1;
  } else if (val > 0) {
    s.zoom += 0.1;
  }

  if (s.zoom < MIN_ZOOM) {
    s.zoom = MIN_ZOOM;
  } else if (s.zoom > MAX_ZOOM) {
    s.zoom = MAX_ZOOM;
  }
}

static void set_frame_rate(f32 val) {
  if (val < 0) {
    s.frame_rate -= 10;
  } else if (val > 0) {
    s.frame_rate += 10;
  }

  if (s.frame_rate < MIN_FRAME_RATE) {
    s.frame_rate = MIN_FRAME_RATE;
  } else if (s.frame_rate > MAX_FRAME_RATE) {
    s.frame_rate = MAX_FRAME_RATE;
  }
}

static cstr major_mode_str(major_mode m) {
  switch (m) {
    case MAJOR_MODE_INIT:
      return "INIT";
    case MAJOR_MODE_NORMAL:
      return "NORMAL";
    case MAJOR_MODE_HELP:
      return "HELP";
    case MAJOR_MODE_INFO:
      return "INFO";
    case MAJOR_MODE_SKINS:
      return "SKINS";
    case MAJOR_MODE_POSES:
      return "POSES";
    default:
      return "UNKNOWN";
  }
}

static void set_major_mode(major_mode m) {
  if (s.mjm != m) {
    DBG("changing mode from '%s' to '%s'", major_mode_str(s.mjm),
        major_mode_str(m));
    s.mjm = m;
  }
}

static void set_minor_mode(minor_mode m) {
  if (s.mnm != m) {
    /* DBG("changing mode from '%s' to '%s'", major_mode_str(s.mjm), */
    /*     major_mode_str(m)); */
    s.mnm = m;
  }
}

static void update(void) {
  if (s.rotating) {
    s.mdl_roty += ROT_FACTOR;
  }

  if (s.animating && (stm_ms(stm_since(last_frame_tick)) > s.frame_rate)) {
    last_frame_tick = stm_now();
    s.mdl_frm++;
    if (s.mdl_frm >= s.mdl.poses[s.mdl_pos].frames_length) {
      s.mdl_frm = 0;
    }
  }
}

static void frame(void) {
  if (s.qft == QK_FILE_QUAKE1_MDL) {
    draw_3d(&s);
  }
  draw_ui(&s);
  update();

  if (s.mjm == MAJOR_MODE_INIT &&
      stm_sec(stm_since(init_tm)) > MAX_INIT_DELAY) {
    set_major_mode(MAJOR_MODE_NORMAL);
  }
}

static void mode_normal_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_SLASH:
        if (e->modifiers & SAPP_MODIFIER_SHIFT) {
          set_major_mode(MAJOR_MODE_HELP);
        }
        break;
      case SAPP_KEYCODE_I:
        set_major_mode(MAJOR_MODE_INFO);
        break;
      case SAPP_KEYCODE_R:
        s.rotating = !s.rotating;
        break;
      case SAPP_KEYCODE_S:
        set_major_mode(MAJOR_MODE_SKINS);
        break;
      case SAPP_KEYCODE_P:
        set_major_mode(MAJOR_MODE_POSES);
        break;
      case SAPP_KEYCODE_A:
        s.animating = !s.animating;
        break;
      case SAPP_KEYCODE_PERIOD:
        if (e->modifiers & SAPP_MODIFIER_SHIFT) {
          next_pose();
        } else {
          next_frame();
        }
        break;
      case SAPP_KEYCODE_COMMA:
        if (e->modifiers & SAPP_MODIFIER_SHIFT) {
          prev_pose();
        } else {
          prev_frame();
        }
        break;
      default:
        break;
    }
  }

  if ((e->type == SAPP_EVENTTYPE_MOUSE_SCROLL)) {
    set_zoom(e->scroll_y);
    set_frame_rate(e->scroll_x);
  }
}

static void mode_init_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    set_major_mode(MAJOR_MODE_NORMAL);
    mode_normal_input(e);
  }
}

static void mode_info_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_ESCAPE:
        set_major_mode(MAJOR_MODE_NORMAL);
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
        set_major_mode(MAJOR_MODE_NORMAL);
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
        set_major_mode(MAJOR_MODE_NORMAL);
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
        set_major_mode(MAJOR_MODE_NORMAL);
        break;
      default:
        break;
    }
  }
}

static void handle_file(cstr path) {
  s.qft = qk_file_guess_type_from_path(path);
  if (s.qft == QK_FILE_QUAKE1_MDL) {
    create_offscreen_target(&s, path);
  }
}

static void input(const sapp_event* e) {
  snk_handle_event(e);

  if (e->type == SAPP_EVENTTYPE_RESIZED) {
    update_offscreen_target(&s, e->window_width, e->window_height);
  }

  if (e->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    handle_file(sapp_get_dropped_file_path(0));
  }

  switch (s.mjm) {
    case MAJOR_MODE_INIT:
      mode_init_input(e);
      break;
    case MAJOR_MODE_NORMAL:
      mode_normal_input(e);
      break;
    case MAJOR_MODE_INFO:
      mode_info_input(e);
      break;
    case MAJOR_MODE_HELP:
      mode_help_input(e);
      break;
    case MAJOR_MODE_SKINS:
      mode_skins_input(e);
      break;
    case MAJOR_MODE_POSES:
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

#ifdef DEBUG
  sepi_alloc_report();
#endif
}

static void init(void) {
  log_info("initializing gpu ...");

  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
#ifdef DEBUG
      .allocator = {.alloc_fn = smemtrack_alloc, .free_fn = smemtrack_free}
#endif
  });

  stm_setup();

  snk_setup(&(snk_desc_t){
      .enable_set_mouse_cursor = true,
      .dpi_scale = sapp_dpi_scale(),
      .logger.func = slog_func,
  });

  init_tm = stm_now();
  s.ctx3d = &ctx3d;
  s.mjm = MAJOR_MODE_INIT;
  s.mnm = MINOR_MODE_UNKNOWN;
  s.mdl_pos = 0;
  s.mdl_skn = 0;
  s.frame_rate = 60;
  s.zoom = 1;
  s.rotating = true;
  s.animating = false;
  last_frame_tick = stm_now();

  cstr path = (cstr)sapp_userdata();
  if (path != NULL) {
    log_info("loading '%s' model", path);
    handle_file(path);
  }
}

sapp_desc sokol_main(i32 argc, char* argv[]) {
  log_info("starting");

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  cstr mdlpath = NULL;
  if (sargs_exists("-i"))
    mdlpath = sargs_value("-i");
  else if (sargs_exists("--input"))
    mdlpath = sargs_value("--input");

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
