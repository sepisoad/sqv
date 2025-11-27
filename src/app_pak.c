/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#define PAK_IMPLEMENTATION
#define KIND_IMPLEMENTATION

#include <stdio.h>

#include "deps/hmm/hmm.h"
#include "deps/log/log.h"
#include "deps/nuklear/nuklear.h"
#include "deps/sokol/sokol_app.h"
#include "deps/sokol/sokol_args.h"
#include "deps/sokol/sokol_gfx.h"
#include "deps/sokol/sokol_glue.h"
#include "deps/sokol/sokol_log.h"
#include "deps/sokol/sokol_nuklear.h"
#include "deps/sokol/sokol_time.h"
#include "deps/sepi/base.h"
#include "deps/sepi/io.h"

#include "shaders/default.glsl.h"

#include "pak.h"

internal struct {
  Arena* arena;
  Pak    pak;
} S;

internal int draw_demo_ui(struct nk_context* ctx);

internal void
init(void) {
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  snk_setup(&(snk_desc_t){
      .enable_set_mouse_cursor = true,
      .dpi_scale = sapp_dpi_scale(),
      .logger.func = slog_func,
  });

  S.arena = arena_create();

  NDBuffer ndb = {0};
  CStr path = "/home/sepi/Games/pc/quake1/id1/pak0.pak";
  IOError  ioerr = io_load_file(S.arena, path, &ndb);
  if (ioerr != IO_ERR_SUCCESS) {
    // TODO: handle the error
  }

  PakError pakerr = pak_load(&S.pak, &ndb);
  if (pakerr != PAK_ERR_SUCCESS) {
    // TODO: handle the error
  }
}

internal void
frame(void) {
  struct nk_context* ctx = snk_new_frame();

  // see big function at end of file
  draw_demo_ui(ctx);

  // the sokol_gfx draw pass
  sg_begin_pass(&(sg_pass){
      .action = {.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                               .clear_value = {0.25f, 0.5f, 0.7f, 1.0f}}},
      .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());

  sg_end_pass();
  sg_commit();
}

internal void
cleanup(void) {
  snk_shutdown();
  sg_shutdown();
}

internal void
input(const sapp_event* event) {
  snk_handle_event(event);
}

sapp_desc
sokol_main(int argc, char* argv[]) {
  Ignore(argc);
  Ignore(argv);

  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .event_cb = input,
      .enable_clipboard = true,
      .width = 1024,
      .height = 768,
      .window_title = "nuklear (sokol-app)",
      .ios_keyboard_resizes_canvas = true,
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };
}

static int
draw_demo_ui(struct nk_context* ctx) {
  internal const char* window_title = "SQV::Pak Explorer";
  internal nk_flags    window_flags =
      NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_TITLE;

  nk_style_hide_cursor(ctx);

  if (nk_begin(ctx, window_title, nk_rect(0, 0, sapp_width(), sapp_height()),
               window_flags)) {
    nk_button_label(ctx, "#FFAA");
  }
  nk_end(ctx);
  return !nk_window_is_closed(ctx, window_title);
}
