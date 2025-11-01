#define PAK_IMPLEMENTATION
#define MD1_IMPLEMENTATION
#define KIND_IMPLEMENTATION

#include <stdbool.h>
#include <stdio.h>

#include "deps/hmm/hmm.h"
#include "deps/log/log.h"
#include "deps/nuklear/nuklear.h"
#include "deps/sepi/io.h"
#include "deps/sokol/sokol_app.h"
#include "deps/sokol/sokol_args.h"
#include "deps/sokol/sokol_gfx.h"
#include "deps/sokol/sokol_glue.h"
#include "deps/sokol/sokol_log.h"
#include "deps/sokol/sokol_nuklear.h"
#include "deps/sokol/sokol_time.h"

#include "shaders/default.glsl.h"
#include "state.h"
#include "md1.h"
#include "pak.h"

Context3D ctx3d = {0};
static State s;
static U64 init_tm = 0;
static U64 last_frame_tick = 0;

Nothing render_init(State*);
Nothing render_pak(State*);
Nothing render_md1(State*);
Nothing render_wad(State*);
Nothing render_lmp(State*);

Nothing set_skin(U32 idx) {
  MakeSure(idx <= s.mdl.header.skins_length, "invalid skin index");
  s.bind.images[IMG_tex] = s.mdl.skins[idx].image;
  s.bind.samplers[SMP_smp] = s.mdl.skins[idx].sampler;
}

static Nothing clean_commandline() {
  memset(s.cmd, 0, sizeof(s.cmd) - 1);
  s.show_cmd = 0;
}

Nothing reset_state() {
  s.mdl_skn = 0;
  s.mdl_pos = 0;
  s.mdl_frm = 0;
  s.zoom = 1;
  s.frame_rate = 60;
  clean_commandline();
}

static Nothing next_pose() {
  s.mdl_pos++;
  if (s.mdl_pos >= s.mdl.header.poses_length) {
    s.mdl_pos = 0;
  }
  s.mdl_frm = 0;
}

static Nothing prev_pose() {
  if (s.mdl_pos == 0) {
    s.mdl_pos = s.mdl.header.poses_length - 1;
  } else {
    s.mdl_pos--;
  }
  s.mdl_frm = 0;
}

static Nothing next_frame() {
  s.mdl_frm++;
  if (s.mdl_frm >= s.mdl.poses[s.mdl_pos].frames_length) {
    s.mdl_frm = 0;
  }
}

static Nothing prev_frame() {
  if (((I32)s.mdl_frm - 1) < 0) {
    s.mdl_frm = s.mdl.poses[s.mdl_pos].frames_length - 1;
  } else {
    s.mdl_frm--;
  }
}

static Nothing set_zoom(F32 val) {
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

static Nothing set_frame_rate(F32 val) {
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

static CStr MajorModeStr(MajorMode m) {
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

static CStr MinorModeStr(MinorMode m) {
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

static Nothing set_MajorMode(MajorMode m) {
  if (s.mjm != m) {
    Dbg("changing major mode from '%s' to '%s'", MajorModeStr(s.mjm),
        MajorModeStr(m));
    s.mjm = m;
  }
}

static Nothing reset_MinorMode(MinorMode m) {
  Dbg("reseting minor mode to '%s'", MinorModeStr(m));
  s.mnm = m;
}

static Nothing enable_MinorMode(MinorMode m) {
  Dbg("enabling minor mode '%s'", MinorModeStr(m));
  s.mnm |= m;
}

static Nothing clear_MinorMode(MinorMode m) {
  Dbg("clearing minor mode '%s'", MinorModeStr(m));
  s.mnm &= ~m;
}

static Nothing toggle_MinorMode(MinorMode m) {
  Dbg("toggleing minor mode '%s' '%s'", MinorModeStr(m),
      m & s.mnm ? "off" : "on");

  s.mnm ^= m;
}

static Nothing mode_init_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_SLASH:
        if (e->modifiers & SAPP_MODIFIER_SHIFT)
          enable_MinorMode(MINOR_MODE_HELP);
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

static Nothing mode_pak_input(const sapp_event* e) {}

static Nothing mode_md1_input(const sapp_event* e) {
  if ((e->type == SAPP_EVENTTYPE_KEY_DOWN) && !e->key_repeat) {
    switch (e->key_code) {
      case SAPP_KEYCODE_SLASH:
        if (e->modifiers & SAPP_MODIFIER_SHIFT)
          toggle_MinorMode(MINOR_MODE_HELP);
        break;
      case SAPP_KEYCODE_I:
        toggle_MinorMode(MINOR_MODE_INFO);
        break;
      case SAPP_KEYCODE_R:
        s.rotating = !s.rotating;
        break;
      case SAPP_KEYCODE_S:
        toggle_MinorMode(MINOR_MODE_SKINS);
        break;
      case SAPP_KEYCODE_P:
        toggle_MinorMode(MINOR_MODE_POSES);
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

static Nothing mode_wad_input(const sapp_event* e) {}

static Nothing mode_lmp_input(const sapp_event* e) {}

static Nothing handle_file(CStr path) {
  s.knd = kind_guess_file(path);

  if (s.knd == KIND_PAK) {
    U8* buf = NULL;  // IWYU pragma: always_keep
    Sz bufsz = io_load_file(path, &buf);
    // TODO: handle errors
    pak_load(buf, bufsz, &s.pak);
    // TODO: handle errors
    set_MajorMode(MAJOR_MODE_PAK);
  } else if (s.knd == KIND_MDL) {
    // create_offscreen_target(&s, path);
  }
}

static Nothing input(const sapp_event* e) {
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
        reset_MinorMode(MINOR_MODE_INIT);
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

static Nothing frame(void) {
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

static Nothing cleanup(void) {
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
}

static Nothing init(void) {
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

  CStr path = (CStr)sapp_userdata();
  if (path != NULL) {
    log_info("loading '%s' model", path);
    handle_file(path);
  }
}

sapp_desc sokol_main(I32 argc, char* argv[]) {
  log_info("starting");

#ifdef DEBUG_MODE
  Dbg("DEBUG MODE IS ON!");
#endif /* DEBUG_MODE */

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  CStr mdlpath = NULL;
  if (sargs_exists("-i"))
    mdlpath = sargs_value("-i");
  else if (sargs_exists("--input"))
    mdlpath = sargs_value("--input");

  return (sapp_desc){
      .init_cb = init,
      .cleanup_cb = cleanup,
      .event_cb = input,
      .frame_cb = frame,
      .user_data = (RawPtr)mdlpath,
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
