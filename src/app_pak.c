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

typedef enum {
  APP_PAK_MODE_EMPTY,
  APP_PAK_MODE_LOADED,
  APP_PAK_MODE_LOAD_FAILED,
} AppPakMode;

internal struct {
  Arena* arena;
  Pak pak;
  AppPakMode mode;
} S;

internal U32 draw_demo_ui(struct nk_context* ctx);
internal Nothing handle_file_drop(CBuf path);

internal Nothing
init() {
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  snk_setup(&(snk_desc_t){
      .enable_set_mouse_cursor = true,
      .dpi_scale = sapp_dpi_scale(),
      .logger.func = slog_func,
  });

  S.mode = APP_PAK_MODE_EMPTY;
  S.arena = arena_create();
}

internal Nothing
frame() {
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

internal Nothing
cleanup() {
  snk_shutdown();
  sg_shutdown();
}

internal Nothing
input(const sapp_event* event) {
  snk_handle_event(event);

  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    handle_file_drop(sapp_get_dropped_file_path(0));
  }
}

sapp_desc
sokol_main(int argc, char* argv[]) {
  Dbg("app_pak::sokol_main() ...");

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  CBuf input_pak_file_path = 0;
  if (sargs_exists("-i"))
    input_pak_file_path = sargs_value("-i");
  else if (sargs_exists("--input"))
    input_pak_file_path = sargs_value("--input");

  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .event_cb = input,
      .user_data = (RawPtr)input_pak_file_path,
      .enable_clipboard = true,
      .width = 640,
      .height = 480,
      .enable_dragndrop = true,
      .max_dropped_files = 1,
      .window_title = "nuklear (sokol-app)",
      .ios_keyboard_resizes_canvas = true,
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };
}

internal U32
draw_empty_screen(struct nk_context* ctx,
                  nk_flags window_flags,
                  CBuf window_title) {
  internal CBuf label = "Please drop a .PAK file here, I'm hungry!";

  if (nk_begin(ctx, window_title, nk_rect(0, 0, sapp_width(), sapp_height()),
               window_flags)) {
    struct nk_rect content_region = nk_window_get_content_region(ctx);
    const struct nk_user_font* font = ctx->style.font;

    F32 text_width =
        font->width(font->userdata, font->height, label, (int)strlen(label));
    F32 text_height = font->height;
    F32 text_pad_x = ctx->style.text.padding.x;
    F32 text_pad_y = ctx->style.text.padding.y;
    F32 text_normalized_width = text_width + 2.0f * text_pad_x;
    F32 text_normalized_height = text_height + 2.0f * text_pad_y;

    struct nk_rect r = {
        .x = content_region.x +
             (content_region.w - text_normalized_width) * 0.5f,
        .y = content_region.y +
             (content_region.h - text_normalized_height) * 0.5f,
        .w = text_normalized_width,
        .h = text_normalized_height,
    };

    nk_layout_space_begin(ctx, NK_STATIC, content_region.h, 1);
    nk_layout_space_push(ctx, r);
    nk_label(ctx, label, NK_TEXT_CENTERED);
    nk_layout_space_end(ctx);
  }
  nk_end(ctx);
}

internal U32
draw_demo_ui(struct nk_context* ctx) {
  internal CBuf window_title = "SQV::Pak Explorer";
  internal nk_flags window_flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER;

  nk_style_hide_cursor(ctx);
  if (S.mode == APP_PAK_MODE_EMPTY) {
    draw_empty_screen(ctx, window_flags, window_title);
  }

  return !nk_window_is_closed(ctx, window_title);
}

internal Nothing
handle_file_drop(CBuf path) {
  if (S.mode == APP_PAK_MODE_LOADED) {
    return;
  }

  NDBuffer ndb = {0};
  IOError ioerr = io_load_file(S.arena, path, &ndb);
  if (ioerr != IO_ERR_SUCCESS) {
    S.mode = APP_PAK_MODE_LOAD_FAILED;
    return;
  }

  PakError pakerr = pak_load(&S.pak, &ndb);
  if (pakerr != PAK_ERR_SUCCESS) {
    S.mode = APP_PAK_MODE_LOAD_FAILED;
    return;
  }

  S.mode = APP_PAK_MODE_LOADED;
}
