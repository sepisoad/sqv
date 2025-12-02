/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#define PAK_IMPLEMENTATION
#define KIND_IMPLEMENTATION

#include <stdio.h>

#include "deps/hmm/hmm.h"
#include "deps/log/log.h"
#include "deps/stb/stb_image.h"
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

#include "icons.h"
#include "pak.h"

typedef enum {
  APP_PAK_MODE_EMPTY,
  APP_PAK_MODE_LOADED,
  APP_PAK_MODE_LOAD_FAILED,
} AppPakMode;

internal struct {
  struct nk_style_button toolbar_button;
} STYLES;

internal struct {
  struct nk_image home;
  struct nk_image back;
  struct nk_image settings;
  struct nk_image folder;
  struct nk_image text;
} ICONS;

internal struct {
  Arena* arena;
  Pak pak;
  AppPakMode mode;
  PakTreeNode* current_pak_tree_node;
  CBuf input_pak_file_path;
} S;

internal U32 app_pak_draw_ui(struct nk_context*);
internal U32 app_pak_draw_mode_empty(struct nk_context*, nk_flags, CBuf);
internal U32
app_pak_draw_mode_loaded(struct nk_context*, nk_flags, CBuf, U32, U32);
internal Nothing app_pak_handle_file_drop(CBuf);

internal Nothing
app_pak_init_icon(struct nk_image* icon_image, CBuf buffer, Sz size) {
  U32 w, h, c = 0;
  CBuf data = stbi_load_from_memory(buffer, size, &w, &h, &c, 4);

  sg_image image = sg_make_image(&(sg_image_desc){
      .width = w,
      .height = h,
      .pixel_format = SG_PIXELFORMAT_RGBA8,
      .sample_count = 1,
      .num_mipmaps = 1,
      .data.mip_levels[0] = {.ptr = data, .size = (Sz)(w * h * 4)}});

  sg_view view = sg_make_view(&(sg_view_desc){
      .texture = {.image = image},
  });

  sg_sampler sampler = sg_make_sampler(&(sg_sampler_desc){
      .min_filter = SG_FILTER_LINEAR,
      .mag_filter = SG_FILTER_LINEAR,
  });

  snk_image_t ui_image = snk_make_image(&(snk_image_desc_t){
      .texture_view = view,
      .sampler = sampler,
  });

  nk_handle handle = snk_nkhandle(ui_image);
  *icon_image = nk_image_handle(handle);
}

internal Nothing
app_pak_init_icons() {
  app_pak_init_icon(&ICONS.home, icon_home_png, sizeof(icon_home_png));
  app_pak_init_icon(&ICONS.back, icon_back_png, sizeof(icon_back_png));
  app_pak_init_icon(&ICONS.settings, icon_settings_png,
                    sizeof(icon_settings_png));
  app_pak_init_icon(&ICONS.folder, icon_folder_png, sizeof(icon_folder_png));
  app_pak_init_icon(&ICONS.text, icon_text_png, sizeof(icon_text_png));
}

internal Nothing
app_pak_init_styles() {
  // TODO:
}

internal Nothing
app_pak_init() {
  Dbg("app_pak_init() ...");

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

  if (S.input_pak_file_path != NULL) {
    log_info("loading '%s' model", S.input_pak_file_path);
    app_pak_handle_file_drop(S.input_pak_file_path);
  }

  app_pak_init_icons();
  app_pak_init_styles();
}

internal Nothing
app_pak_frame() {
  // Dbg("app_pak_frame() ...");

  struct nk_context* ctx = snk_new_frame();

  app_pak_draw_ui(ctx);
  sg_begin_pass(&(sg_pass){
      .action = {.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                               .clear_value = {0.25f, 0.5f, 0.7f, 1.0f}}},
      .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());

  sg_end_pass();
  sg_commit();
}

internal Nothing
app_pak_cleanup() {
  snk_shutdown();
  sg_shutdown();
}

internal Nothing
app_pak_input(const sapp_event* event) {
  // Dbg("app_pak_input() ...");

  snk_handle_event(event);
  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    app_pak_handle_file_drop(sapp_get_dropped_file_path(0));
  }
}

internal Nothing
app_pak_handle_file_drop(CBuf path) {
  Dbg("app_pak_handle_file_drop() ...");

  if (S.mode == APP_PAK_MODE_LOADED) {
    return;
  }

  /* TODO:
   * in the future we want to expand the support to other asset file types as
   * well suck as quake 2 and quake 3, etc, so we need to first figure out the
   * file type here and then call into the corresponding handler, for now we
   * just focus on quake 1 '.PAK' files
   */

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
  S.input_pak_file_path = path;
  S.current_pak_tree_node = &S.pak.tree.root;
}

sapp_desc
sokol_main(int argc, char* argv[]) {
  Dbg("app_pak::sokol_main() ...");

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  S.input_pak_file_path = 0;

  if (sargs_exists("-i"))
    S.input_pak_file_path = sargs_value("-i");
  else if (sargs_exists("--input"))
    S.input_pak_file_path = sargs_value("--input");

  return (sapp_desc){
      .init_cb = app_pak_init,
      .frame_cb = app_pak_frame,
      .cleanup_cb = app_pak_cleanup,
      .event_cb = app_pak_input,
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
app_pak_draw_ui(struct nk_context* ctx) {
  // Dbg("app_pak_draw_ui() ...");

  internal CBuf window_title = "SQV::Pak Explorer";
  internal nk_flags window_flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER;

  U32 window_width = sapp_width();
  U32 window_height = sapp_height();

  nk_style_hide_cursor(ctx);
  if (S.mode == APP_PAK_MODE_EMPTY) {
    app_pak_draw_mode_empty(ctx, window_flags, window_title);
  } else if (S.mode == APP_PAK_MODE_LOADED) {
    app_pak_draw_mode_loaded(ctx, window_flags, window_title, window_width,
                             window_height);
  }

  return !nk_window_is_closed(ctx, window_title);
}

internal U32
app_pak_draw_mode_empty(struct nk_context* ctx,
                        nk_flags window_flags,
                        CBuf window_title) {
  // Dbg("app_pak_draw_mode_empty() ...");

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
    F32 element_width = text_width + 2.0f * text_pad_x;
    F32 element_height = text_height + 2.0f * text_pad_y;

    struct nk_rect r = {
        .x = content_region.x + (content_region.w - element_width) * 0.5f,
        .y = content_region.y + (content_region.h - element_height) * 0.5f,
        .w = element_width,
        .h = element_height,
    };

    nk_layout_space_begin(ctx, NK_STATIC, content_region.h, 1);
    nk_layout_space_push(ctx, r);
    nk_label(ctx, label, NK_TEXT_CENTERED);
    nk_layout_space_end(ctx);
  }
  nk_end(ctx);
}

internal Nothing
app_pak_draw_explorer_icon(struct nk_context* ctx, Bool is_dir, CBuf text) {
  U32 icon_width = 50;
  if (nk_group_begin(ctx, "", NK_WINDOW_NO_SCROLLBAR)) {
    if (is_dir) {
      nk_layout_row_static(ctx, 50, icon_width, 1);
      nk_button_image(ctx, ICONS.folder);
      nk_layout_row_dynamic(ctx, 10, 1);
      nk_label(ctx, text, NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
    } else {
      nk_layout_row_static(ctx, 50, icon_width, 1);
      nk_button_image(ctx, ICONS.text);
      nk_layout_row_dynamic(ctx, 10, 1);
      nk_label(ctx, text, NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
    }
    nk_group_end(ctx);
  }
}

internal Nothing
app_pak_draw_explorer_area(struct nk_context* ctx, U32 window_width, U32 window_height) {
  HashMap* children = S.current_pak_tree_node->children;

  U32 items_count = children->count;
  U32 colmun_gap = 10;
  U32 icon_width = 85;
  U32 columns = (window_width + colmun_gap) / (icon_width + colmun_gap);
  columns = columns ? columns : 1;
  U32 rows = (items_count / columns);
  U32 remainder = items_count % columns;

  nk_layout_row_dynamic(ctx, window_height, 1);
  if (nk_group_begin(ctx, "", 0)) {
    nk_layout_row_static(ctx, 70, icon_width, columns);
    U32 index = 0;
    for (U32 row = 0; row < rows; row++) {
      for (U32 column = 0; column < columns; column++) {
        if (index >= items_count)
          break;

        HashMapKV* kv = hashmap_key_at(children, index);
        Bool is_dir = ((PakTreeNode*)kv->v_rawptr)->is_dir;

        app_pak_draw_explorer_icon(ctx, is_dir, kv->k_str.cstr);

        index++;
      }
    }

    for (U32 column = 0; column < remainder; column++) {
      HashMapKV* kv = hashmap_key_at(children, index);
      Bool is_dir = ((PakTreeNode*)kv->v_rawptr)->is_dir;

      app_pak_draw_explorer_icon(ctx, is_dir, kv->k_str.cstr);

      index++;
    }
    nk_group_end(ctx);
  }
}

internal U32
app_pak_draw_mode_loaded(struct nk_context* ctx,
                         nk_flags window_flags,
                         CBuf window_title,
                         U32 window_width,
                         U32 window_height) {
  // Dbg("app_pak_draw_mode_loaded() ...");

  if (nk_begin(ctx, window_title, nk_rect(0, 0, window_width, window_height),
               window_flags)) {
    const struct nk_user_font* font = ctx->style.font;
    F32 text_height = font->height;

    struct nk_rect content = nk_window_get_content_region(ctx);

    nk_style_push_style_item(ctx, &ctx->style.button.normal,
                             nk_style_item_color(nk_rgb(100, 100, 100)));
    nk_style_push_style_item(ctx, &ctx->style.button.hover,
                             nk_style_item_color(nk_rgb(150, 150, 150)));
    nk_style_push_style_item(ctx, &ctx->style.button.active,
                             nk_style_item_color(nk_rgb(200, 200, 200)));

    nk_layout_row_static(ctx, ICONS.home.h, 32, 3);
    nk_button_image(ctx, ICONS.home);
    nk_button_image(ctx, ICONS.back);
    nk_label(ctx, S.current_pak_tree_node->name,
             NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);

    nk_layout_row_static(ctx, ICONS.home.h, 32, 2);
    nk_button_image(ctx, ICONS.settings);
    nk_button_image(ctx, ICONS.back);

    app_pak_draw_explorer_area(ctx, window_width, window_height);

    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);
    nk_style_pop_style_item(ctx);

    char count_str[16] = {0};
    sprintf(count_str, "%d", 1987);

    nk_layout_space_begin(ctx, NK_STATIC, window_height - 200, 1);
    nk_layout_row_dynamic(ctx, 0, 2);
    nk_label(ctx, S.input_pak_file_path, NK_TEXT_LEFT);
    nk_label(ctx, count_str, NK_TEXT_RIGHT);
    nk_layout_space_end(ctx);
  }
  nk_end(ctx);
}
