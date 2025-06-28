#define PAK_IMPLEMENTATION
#define MD1_IMPLEMENTATION
#define KIND_IMPLEMENTATION

#include <stdbool.h>
#include <stdio.h>

#include "../deps/hmm.h"
#include "../deps/log.h"
#include "../deps/nuklear.h"
#include "../deps/sepi_io.h"
#include "../deps/sepi_types.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_args.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"
#include "../deps/sokol_nuklear.h"
#include "../deps/sokol_time.h"

#include "../res/shaders/default.glsl.h"
#include "./app.h"
#include "./kind.h"
#include "./md1.h"
#include "./pak.h"

#ifdef DEBUG
#include "../deps/sepi_alloc.h"
#include "../deps/sokol_memtrack.h"
#endif

context3d ctx3d = {0};
static state s;
static u64 init_tm = 0;
static u64 last_frame_tick = 0;

void render_init(state*);
void render_pak(state*);
void render_md1(state*);
void render_wad(state*);
void render_lmp(state*);

void set_skin(u32 idx) {
  makesure(idx <= s.mdl.header.skins_length, "invalid skin index");
  s.bind.images[IMG_tex] = s.mdl.skins[idx].image;
  s.bind.samplers[SMP_smp] = s.mdl.skins[idx].sampler;
}

static void clean_commandline() {
  memset(s.cmd, 0, sizeof(s.cmd) - 1);
  s.show_cmd = 0;
}

void reset_state() {
  s.mdl_skn = 0;
  s.mdl_pos = 0;
  s.mdl_frm = 0;
  s.zoom = 1;
  s.frame_rate = 60;
  clean_commandline();
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
    case MAJOR_MODE_PAK:
      return "PAK";
    case MAJOR_MODE_MD1:
      return "MD1";
    case MAJOR_MODE_WAD:
      return "WAD";
    case MAJOR_MODE_LMP:
      return "LMP";
    default:
      return "UNKNOWN";
  }
}

static cstr minor_mode_str(minor_mode m) {
  switch (m) {
    case MINOR_MODE_INIT:
      return "INIT";
    case MINOR_MODE_INFO:
      return "INFO";
    case MINOR_MODE_HELP:
      return "HELP";
    case MINOR_MODE_TREE:
      return "TREE";
    case MINOR_MODE_SKINS:
      return "SKINS";
    case MINOR_MODE_POSES:
      return "POSES";
    case MINOR_MODE_FRAMES:
      return "FRAMES";
    default:
      return "UNKNOWN";
  }
}

static void set_major_mode(major_mode m) {
  if (s.mjm != m) {
    DBG("changing major mode from '%s' to '%s'", major_mode_str(s.mjm),
        major_mode_str(m));
    s.mjm = m;
  }
}

static void reset_minor_mode(minor_mode m) {
  DBG("reseting minor mode to '%s'", minor_mode_str(m));
  s.mnm = m;
}

static void enable_minor_mode(minor_mode m) {
  DBG("enabling minor mode '%s'", minor_mode_str(m));
  s.mnm |= m;
}

static void clear_minor_mode(minor_mode m) {
  DBG("clearing minor mode '%s'", minor_mode_str(m));
  s.mnm &= ~m;
}

static void toggle_minor_mode(minor_mode m) {
  DBG("toggleing minor mode '%s' '%s'", minor_mode_str(m),
      m & s.mnm ? "off" : "on");

  s.mnm ^= m;
}

static void mode_init_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_SLASH:
        if (e->modifiers & SAPP_MODIFIER_SHIFT)
          enable_minor_mode(MINOR_MODE_HELP);
        break;
      case SAPP_KEYCODE_SEMICOLON:
        if (e->modifiers & SAPP_MODIFIER_SHIFT)
          if (!s.show_cmd)
            s.show_cmd = true;
        break;
      default:
        break;
    }
  }
}

static void mode_pak_input(const sapp_event* e) {}

static void mode_md1_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_SLASH:
        if (e->modifiers & SAPP_MODIFIER_SHIFT)
          toggle_minor_mode(MINOR_MODE_HELP);
        break;
      case SAPP_KEYCODE_I:
        toggle_minor_mode(MINOR_MODE_INFO);
        break;
      case SAPP_KEYCODE_R:
        s.rotating = !s.rotating;
        break;
      case SAPP_KEYCODE_S:
        toggle_minor_mode(MINOR_MODE_SKINS);
        break;
      case SAPP_KEYCODE_P:
        toggle_minor_mode(MINOR_MODE_POSES);
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

static void mode_wad_input(const sapp_event* e) {}

static void mode_lmp_input(const sapp_event* e) {}

static void handle_file(cstr path) {
  s.knd = kind_guess_file(path);

  if (s.knd == KIND_PAK) {
    u8* buf = NULL;  // IWYU pragma: always_keep
    sz bufsz = sepi_io_load_file(path, &buf);
    // TODO: handle errors
    pak_load(buf, bufsz, &s.pak);
    // TODO: handle errors
    set_major_mode(MAJOR_MODE_PAK);
  } else if (s.knd == KIND_MDL) {
    // create_offscreen_target(&s, path);
  }
}

static void input(const sapp_event* e) {
  snk_handle_event(e);

  if (e->type == SAPP_EVENTTYPE_RESIZED) {
    // update_offscreen_target(&s, e->window_width, e->window_height);
  }

  if (e->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    handle_file(sapp_get_dropped_file_path(0));
  }

  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_ESCAPE:
        reset_minor_mode(MINOR_MODE_INIT);
        clean_commandline();
        break;
      default:
        break;
    }
  }

  switch (s.mjm) {
    case MAJOR_MODE_INIT:
      mode_init_input(e);
      break;
    case MAJOR_MODE_PAK:
      mode_pak_input(e);
      break;
    case MAJOR_MODE_MD1:
      mode_md1_input(e);
      break;
    case MAJOR_MODE_WAD:
      mode_wad_input(e);
      break;
    case MAJOR_MODE_LMP:
      mode_lmp_input(e);
      break;
    default:
      mode_init_input(e);
      break;
  }
}

// static void update(void) {
//   if (s.rotating) {
//     s.mdl_roty += ROT_FACTOR;
//   }

//   if (s.animating && (stm_ms(stm_since(last_frame_tick)) > s.frame_rate)) {
//     last_frame_tick = stm_now();
//     s.mdl_frm++;
//     if (s.mdl_frm >= s.mdl.poses[s.mdl_pos].frames_length) {
//       s.mdl_frm = 0;
//     }
//   }
// }

static void frame(void) {
  switch (s.mjm) {
    case MAJOR_MODE_INIT:
      render_init(&s);
      break;
    case MAJOR_MODE_PAK:
      render_pak(&s);
      break;
    case MAJOR_MODE_MD1:
      render_md1(&s);
      break;
    case MAJOR_MODE_WAD:
      render_wad(&s);
      break;
    case MAJOR_MODE_LMP:
      render_lmp(&s);
      break;
    default:
      render_init(&s);
  }
}

static void cleanup(void) {
  log_info("shutting down");

  pak_unload(&s.pak);
  md1_unload(&s.mdl);
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
  s.mnm = MINOR_MODE_INIT;
  s.mdl_pos = 0;
  s.mdl_skn = 0;
  s.frame_rate = 60;
  s.zoom = 1;
  s.rotating = true;
  s.animating = false;
  s.show_cmd = false;
  clean_commandline();
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
