/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#define MODULE_PAK_IMPLEMENTATION
#define MODULE_KIND_IMPLEMENTATION

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdio.h>

#if defined(PROFILING)
#include "deps/tracy/tracy.h"
TracyCZoneCtx trcyctx;
#endif

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
#include "module_pak.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define APP_PAK_WINDOW_PADDING_X 2
#define APP_PAK_WINDOW_PADDING_Y 2
#define APP_PAK_TOP_REGION_HEIGHT 35
#define APP_PAK_BOTTOM_REGION_HEIGHT 30
#define APP_PAK_EXPLORER_ICON_IMAGE_HEIGHT 50
#define APP_PAK_EXPLORER_ICON_TEXT_HEIGHT 25
#define APP_PAK_EXPLORER_ICON_HEIGHT \
  APP_PAK_EXPLORER_ICON_IMAGE_HEIGHT + APP_PAK_EXPLORER_ICON_TEXT_HEIGHT
#define APP_PAK_EXPLORER_ICON_IMAGE_WIDTH 50
#define APP_PAK_EXPLORER_ICON_TEXT_WIDTH 200
#define APP_PAK_EXPLORER_ICON_GAP 2
#define APP_PAK_EXPLORER_PADDING_X 2
#define APP_PAK_EXPLORER_PADDING_Y 2
#define APP_PAK_STATUSBAR_HEIGHT 20
#define APP_PAK_MAX_EXPORT_PATH_LENGTH 1024
#define APP_PAK_MAX_ERROR_LENGTH 512

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  APP_PAK_MODE_EMPTY,
  APP_PAK_MODE_LOADED,
  APP_PAK_MODE_FAILED,
} AppPakMode;

typedef enum {
  APP_PAK_ERR_SUCCESS = 1,
  APP_PAK_ERR_FILE_OPEN,
  APP_PAK_ERR_ICON_INIT,
  APP_PAK_ERR_MODULE_PAK,
  APP_PAK_ERR__COUNT,
} AppPakError;

typedef struct {
  sg_image image;
  sg_view view;
  sg_sampler sampler;
  snk_image_t ui_image;
  nk_handle handle;
  struct nk_image icon_image;
} AppPakImage;

/* ===================================================== */
/*                        GLOBALS                        */
/* ===================================================== */

internal struct {
  AppPakImage home;
  AppPakImage back;
  AppPakImage save;
  AppPakImage extract;
  AppPakImage folder;
  AppPakImage text;
} ICONS;

internal struct {
  Bool is_app_styled;
  Bool is_extracting_requested;
  Pak pak;
  AppPakMode mode;
  PakTreeNode* current_pak_tree_node;
  FILE* input_file;
  CBuf input_file_path;
  char export_path_buffer[APP_PAK_MAX_EXPORT_PATH_LENGTH];
  char error_text[APP_PAK_MAX_ERROR_LENGTH];
  Arena* arena;
} S;

/* ===================================================== */
/*                      DECLERATIONS                     */
/* ===================================================== */

internal Nothing app_pak_init();
internal AppPakError app_pak_init_style(struct nk_style* s);
internal AppPakError app_pak_init_icons();
internal AppPakError app_pak_init_icon(AppPakImage* app_icon,
                                       CBuf buffer,
                                       Sz size);
internal Nothing app_pak_cleanup();
internal Nothing app_pak_cleanup_reload();
internal AppPakError app_pak_cleanup_icons();
internal AppPakError app_pak_cleanup_icon(AppPakImage* app_icon);

internal Nothing app_pak_input(const sapp_event* event);
internal AppPakError app_pak_handle_file_drop(CBuf path);
internal Nothing app_pak_frame();
internal U32 app_pak_draw(struct nk_context* ctx);
internal Nothing app_pak_draw_mode_empty(struct nk_context* ctx,
                                         nk_flags window_flags,
                                         U32 window_width,
                                         U32 window_height);
internal Nothing app_pak_draw_mode_failed(struct nk_context* ctx,
                                          nk_flags window_flags,
                                          U32 window_width,
                                          U32 window_height);
internal Nothing app_pak_draw_mode_loaded(struct nk_context* ctx,
                                          nk_flags window_flags,
                                          U32 window_width,
                                          U32 window_height);
internal Nothing app_pak_draw_mode_loaded(struct nk_context* ctx,
                                          nk_flags window_flags,
                                          U32 window_width,
                                          U32 window_height);
internal Nothing app_pak_draw_widget_explorer_area(struct nk_context* ctx,
                                                   U32 window_width,
                                                   U32 window_height);
internal Nothing app_pak_draw_widget_explorer_item(struct nk_context* ctx,
                                                   PakTreeNode* node);
internal Nothing app_pak_draw_widget_explorer_icon(struct nk_context* ctx,
                                                   PakTreeNode* node,
                                                   Bool is_dir,
                                                   struct nk_image* image,
                                                   CStr text);

/* ===================================================== */
/*                       FUNCTIONS                       */
/* ===================================================== */

sapp_desc
sokol_main(int argc, char* argv[]) {
  TracyCZoneN(trcyctx, "sokol_main", 1);

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  MemZero(&S, sizeof(S));

  if (sargs_exists("-i"))
    S.input_file_path = sargs_value("-i");
  else if (sargs_exists("--input"))
    S.input_file_path = sargs_value("--input");

  return (sapp_desc){
      .init_cb = app_pak_init,
      .frame_cb = app_pak_frame,
      .cleanup_cb = app_pak_cleanup,
      .event_cb = app_pak_input,
      .enable_clipboard = true,
      .width = 640,   // TODO: hard coded
      .height = 480,  // TODO: hard coded
      .enable_dragndrop = true,
      .max_dropped_files = 1,
      .window_title = "nuklear (sokol-app)",
      .ios_keyboard_resizes_canvas = true,
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal Nothing
app_pak_init() {
  TracyCZoneN(trcyctx, "app_pak_init", 1);

  S.arena = arena_create();

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

  if (S.input_file_path != NULL) {
    log_info("loading '%s' model", S.input_file_path);
    app_pak_handle_file_drop(S.input_file_path);
  }

  app_pak_init_icons();

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal AppPakError
app_pak_init_style(struct nk_style* s) {
  TracyCZoneN(trcyctx, "app_pak_init_style", 1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  struct nk_style_window* window = &s->window;

  window->padding.x = APP_PAK_WINDOW_PADDING_X;
  window->padding.y = APP_PAK_WINDOW_PADDING_Y;
  window->scrollbar_size.x = 2;
  window->scrollbar_size.y = 2;
  window->group_padding.x = APP_PAK_EXPLORER_PADDING_X;
  window->group_padding.y = APP_PAK_EXPLORER_PADDING_Y;

  window->border = 0;
  window->group_border = 0;

  window->background.r = 100;
  window->background.g = 100;
  window->background.b = 100;
  window->background.a = 255;

  window->fixed_background.type = NK_STYLE_ITEM_COLOR;
  window->fixed_background.data.color.r = 54;
  window->fixed_background.data.color.g = 51;
  window->fixed_background.data.color.b = 50;

  struct nk_style_button* button = &s->button;

  button->normal.data.color.r = 100;
  button->normal.data.color.g = 100;
  button->normal.data.color.b = 100;

  button->hover.data.color.r = 150;
  button->hover.data.color.g = 150;
  button->hover.data.color.b = 150;

  button->active.data.color.r = 200;
  button->active.data.color.g = 200;
  button->active.data.color.b = 200;

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

internal AppPakError
app_pak_init_icons() {
  TracyCZoneN(trcyctx, "app_pak_init_icons", 1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  err = app_pak_init_icon(&ICONS.home, icon_home_png, sizeof(icon_home_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&ICONS.back, icon_back_png, sizeof(icon_back_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&ICONS.save, icon_save_png, sizeof(icon_save_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&ICONS.extract, icon_extract_png,
                          sizeof(icon_extract_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&ICONS.folder, icon_folder_png,
                          sizeof(icon_folder_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&ICONS.text, icon_text_png, sizeof(icon_text_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

internal AppPakError
app_pak_init_icon(AppPakImage* app_icon, CBuf buffer, Sz size) {
  TracyCZoneN(trcyctx, "app_pak_init_icon", 1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  U32 w, h, c = 0;
  CStr data = stbi_load_from_memory(buffer, size, &w, &h, &c, 4);
  if (0 == data) {
    err = APP_PAK_ERR_ICON_INIT;
    snprintf(S.error_text, APP_PAK_MAX_ERROR_LENGTH,
             "failed to load icon image from memory");
    goto cleanup;
  }
  TracyCAlloc(data, size);

  app_icon->image = sg_make_image(&(sg_image_desc){
      .width = w,
      .height = h,
      .pixel_format = SG_PIXELFORMAT_RGBA8,
      .sample_count = 1,
      .num_mipmaps = 1,
      .data.mip_levels[0] = {.ptr = data, .size = (Sz)(w * h * 4)}});

  app_icon->view = sg_make_view(&(sg_view_desc){
      .texture = {.image = app_icon->image},
  });

  app_icon->sampler = sg_make_sampler(&(sg_sampler_desc){
      .min_filter = SG_FILTER_LINEAR,
      .mag_filter = SG_FILTER_LINEAR,
  });

  app_icon->ui_image = snk_make_image(&(snk_image_desc_t){
      .texture_view = app_icon->view,
      .sampler = app_icon->sampler,
  });

  app_icon->handle = snk_nkhandle(app_icon->ui_image);
  app_icon->icon_image = nk_image_handle(app_icon->handle);

cleanup:
  if (data) {
    free(data);
    TracyCFree(data);
  }

  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

internal Nothing
app_pak_cleanup() {
  TracyCZoneN(trcyctx, "app_pak_cleanup", 1);

  if (S.input_file) {
    fclose(S.input_file);
    S.input_file = 0;
  }

  app_pak_cleanup_icons();
  snk_shutdown();
  sg_shutdown();
  pak_unload(&S.pak);
  arena_destroy(S.arena);

  TracyCZoneEnd(trcyctx);
}

internal Nothing
app_pak_cleanup_reload() {
  TracyCZoneN(trcyctx, "app_pak_cleanup_reload", 1);

  if (S.input_file) {
    fclose(S.input_file);
    S.input_file = 0;
    S.input_file_path = 0;
  }

  S.is_extracting_requested = FALSE;
  S.mode = APP_PAK_MODE_EMPTY;
  MemZeroArray(S.error_text);

  pak_unload(&S.pak);
  arena_destroy(S.arena);
  S.arena = 0;
  S.arena = arena_create();

  TracyCZoneEnd(trcyctx);
}

internal AppPakError
app_pak_cleanup_icons() {
  TracyCZoneN(trcyctx, "app_pak_cleanup_icons", 1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  err = app_pak_cleanup_icon(&ICONS.home);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&ICONS.back);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&ICONS.save);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&ICONS.extract);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&ICONS.folder);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&ICONS.text);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

internal AppPakError
app_pak_cleanup_icon(AppPakImage* app_icon) {
  TracyCZoneN(trcyctx, "app_pak_cleanup_icon", 1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  sg_destroy_view(app_icon->view);
  sg_destroy_sampler(app_icon->sampler);
  sg_destroy_image(app_icon->image);
  snk_destroy_image(app_icon->ui_image);

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

internal Nothing
app_pak_input(const sapp_event* event) {
  TracyCZoneN(trcyctx, "app_pak_input", 1);

  if ((event->type == SAPP_EVENTTYPE_KEY_DOWN) && !event->key_repeat) {
    if (event->key_code == SAPP_KEYCODE_ESCAPE) {
      sapp_request_quit();
    }
  }

  snk_handle_event(event);
  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    app_pak_handle_file_drop(sapp_get_dropped_file_path(0));
  }

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal AppPakError
app_pak_handle_file_drop(CBuf path) {
  TracyCZoneN(trcyctx, "app_pak_handle_file_drop", 1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  // NOTE:
  // we may want to allow this to happen, so the user does not have to
  // re-run the app for a new .PAK file! but this has to reset all the
  // states and wipe the arenas!
  if (S.mode == APP_PAK_MODE_LOADED){
    // goto cleanup;
    app_pak_cleanup_reload();
  }

  S.input_file = fopen(path, "rb");
  if (0 == S.input_file) {
    snprintf(S.error_text, APP_PAK_MAX_ERROR_LENGTH, "failed to open '%s'",
             path);
    err = APP_PAK_ERR_FILE_OPEN;
    S.mode = APP_PAK_MODE_FAILED;
    goto cleanup;
  }

  PakError perr = pak_load_from_file(&S.pak, S.input_file);
  if (perr != PAK_ERR_SUCCESS) {
    snprintf(S.error_text, APP_PAK_MAX_ERROR_LENGTH,
             "failed to load '%s' items", path);
    err = APP_PAK_ERR_MODULE_PAK;
    S.mode = APP_PAK_MODE_FAILED;
    goto cleanup;
  }

  S.mode = APP_PAK_MODE_LOADED;
  S.input_file_path = path;
  S.current_pak_tree_node = &S.pak.tree.root;

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

internal Nothing
app_pak_frame() {
  TracyCZoneN(trcyctx, "app_pak_frame", 1);
  TracyCFrameMarkStart(0);

  struct nk_context* ctx = snk_new_frame();

  if (S.is_app_styled == FALSE) {
    app_pak_init_style(&ctx->style);
    S.is_app_styled = TRUE;
  }

  app_pak_draw(ctx);
  sg_begin_pass(
      &(sg_pass){.action =
                     {
                         .colors[0] = {.load_action = SG_LOADACTION_CLEAR},
                     },
                 .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());

  sg_end_pass();
  sg_commit();

  TracyCFrameMarkEnd(0);
  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal U32
app_pak_draw(struct nk_context* ctx) {
  TracyCZoneN(trcyctx, "app_pak_draw", 1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  internal CBuf window_title = "SQV::Pak Explorer";
  internal nk_flags window_flags = 0;

  U32 window_width = sapp_width();
  U32 window_height = sapp_height();

  nk_style_hide_cursor(ctx);
  if (S.mode == APP_PAK_MODE_EMPTY) {
    app_pak_draw_mode_empty(ctx, window_flags, window_width, window_height);
  } else if (S.mode == APP_PAK_MODE_LOADED) {
    app_pak_draw_mode_loaded(ctx, window_flags, window_width, window_height);
  } else if (S.mode == APP_PAK_MODE_FAILED) {
    app_pak_draw_mode_failed(ctx, window_flags, window_width, window_height);
  }

  TracyCZoneEnd(trcyctx);
  return !nk_window_is_closed(ctx, window_title);
}

/* ===================================================== */

internal Nothing
app_pak_draw_mode_empty(struct nk_context* ctx,
                        nk_flags window_flags,
                        U32 window_width,
                        U32 window_height) {
  TracyCZoneN(trcyctx, "app_pak_draw_mode_empty", 1);

  internal CBuf label = "Please drop a .PAK file here, I'm hungry!";

  if (nk_begin(ctx, "", nk_rect(0, 0, window_width, window_height),
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

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal Nothing
app_pak_draw_mode_failed(struct nk_context* ctx,
                         nk_flags window_flags,
                         U32 window_width,
                         U32 window_height) {
  TracyCZoneN(trcyctx, "app_pak_draw_mode_failed", 1);

  if (nk_begin(ctx, "", nk_rect(0, 0, window_width, window_height),
               window_flags)) {
    struct nk_rect content_region = nk_window_get_content_region(ctx);
    const struct nk_user_font* font = ctx->style.font;

    F32 text_width =
        font->width(font->userdata, font->height, S.error_text, (int)strlen(S.error_text));
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
    nk_label(ctx, S.error_text, NK_TEXT_CENTERED);
    nk_layout_space_end(ctx);
  }
  nk_end(ctx);

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal Nothing
app_pak_draw_mode_loaded(struct nk_context* ctx,
                         nk_flags window_flags,
                         U32 window_width,
                         U32 window_height) {
  TracyCZoneN(trcyctx, "app_pak_draw_mode_loaded", 1);

  Bool is_root = S.current_pak_tree_node->item_name[0] == ' ';

  // === TOP REGION ===
  if (nk_begin(ctx, "loaded_mode_top_region",
               nk_rect(0, 0, window_width, APP_PAK_TOP_REGION_HEIGHT),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_template_begin(ctx, ICONS.home.icon_image.h);
    nk_layout_row_template_push_static(ctx, 30);
    nk_layout_row_template_push_static(ctx, 30);
    nk_layout_row_template_push_static(ctx, 30);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);

    if (nk_button_image(ctx, ICONS.extract.icon_image)) {
      // S.current_pak_tree_node = &S.pak.tree.root;
      S.is_extracting_requested = TRUE;
    }

    if (nk_button_image(ctx, ICONS.home.icon_image)) {
      S.current_pak_tree_node = &S.pak.tree.root;
    }

    if (nk_button_image(ctx, ICONS.back.icon_image)) {
      if (S.current_pak_tree_node->parent) {
        S.current_pak_tree_node = S.current_pak_tree_node->parent;
      }
    }

    nk_label(ctx, S.current_pak_tree_node->name,
             NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
  }
  nk_end(ctx);

  // === MIDDLE REGION ===
  U32 middle_region_height =
      window_height - APP_PAK_TOP_REGION_HEIGHT - APP_PAK_BOTTOM_REGION_HEIGHT;
  struct nk_style_item bkg = {
      .type = NK_STYLE_ITEM_COLOR,
      .data = {.color = {.r = 34, .g = 37, .b = 35, .a = 255}}};

  nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, bkg);

  if (nk_begin(ctx, "loaded_mode_middle_region",
               nk_rect(0, APP_PAK_TOP_REGION_HEIGHT, window_width,
                       middle_region_height),
               NK_WINDOW_SCROLL_AUTO_HIDE)) {
    const struct nk_user_font* font = ctx->style.font;
    F32 text_height = font->height;

    app_pak_draw_widget_explorer_area(ctx, window_width, middle_region_height);
  }

  if (S.is_extracting_requested) {
    struct nk_rect s = {.x = 50, .y = 50, .w = window_width - 100, .h = 190};
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Export",
                       NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR, s)) {
      nk_layout_row_dynamic(ctx, 20, 1);
      nk_label(ctx, "output path:", NK_TEXT_LEFT);
      nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, S.export_path_buffer,
                                     APP_PAK_MAX_EXPORT_PATH_LENGTH,
                                     nk_filter_default);
      nk_popup_end(ctx);
    } else
      S.is_extracting_requested = FALSE;
  }

  nk_end(ctx);
  nk_style_pop_style_item(ctx);

  // === BOTTOM REGION ===
  if (nk_begin(ctx, "loaded_mode_bottom_region",
               nk_rect(0, window_height - APP_PAK_BOTTOM_REGION_HEIGHT,
                       window_width, APP_PAK_BOTTOM_REGION_HEIGHT),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx, 0, 1);
    nk_label(ctx, S.input_file_path, NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
  }
  nk_end(ctx);

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal Nothing
app_pak_draw_widget_explorer_area(struct nk_context* ctx,
                                  U32 window_width,
                                  U32 window_height) {
  TracyCZoneN(trcyctx, "app_pak_draw_widget_explorer_area", 1);

  HashMap* children = S.current_pak_tree_node->children;
  U32 items_count = children->count;
  U32 columns = (window_width + APP_PAK_EXPLORER_ICON_GAP) /
                (APP_PAK_EXPLORER_ICON_TEXT_WIDTH + APP_PAK_EXPLORER_ICON_GAP);
  columns = columns ? columns : 1;
  U32 rows = (items_count / columns);
  U32 remainder = items_count % columns;
  U32 row_height = (APP_PAK_EXPLORER_ICON_HEIGHT + APP_PAK_EXPLORER_ICON_GAP +
                    APP_PAK_EXPLORER_PADDING_Y);
  U32 actual_height = rows * row_height;
  if (remainder > 0) {
    actual_height += row_height;
  }

  if (actual_height < window_height) {
    actual_height = window_height;
  }

  nk_layout_row_dynamic(ctx, actual_height, 1);
  if (nk_group_begin(ctx, "", NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx, APP_PAK_EXPLORER_ICON_HEIGHT, columns);
    U32 index = 0;
    for (U32 row = 0; row < rows; row++) {
      for (U32 column = 0; column < columns; column++) {
        if (index >= items_count)
          break;
        HashMapKV* kv = hashmap_key_at(children, index);
        index++;

        app_pak_draw_widget_explorer_item(ctx, (PakTreeNode*)kv->v_rawptr);
      }
    }
    for (U32 column = 0; column < remainder; column++) {
      HashMapKV* kv = hashmap_key_at(children, index);
      index++;

      app_pak_draw_widget_explorer_item(ctx, (PakTreeNode*)kv->v_rawptr);
    }
    nk_group_end(ctx);
  }

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal Nothing
app_pak_draw_widget_explorer_item(struct nk_context* ctx, PakTreeNode* node) {
  TracyCZoneN(trcyctx, "app_pak_draw_widget_explorer_item", 1);

  if (nk_group_begin(ctx, "", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT)) {
    if (node->is_dir) {
      app_pak_draw_widget_explorer_icon(
          ctx, node, TRUE, &ICONS.folder.icon_image, node->item_name);
    } else {
      app_pak_draw_widget_explorer_icon(
          ctx, node, FALSE, &ICONS.text.icon_image, node->item_name);
    }
    nk_group_end(ctx);
  }

  TracyCZoneEnd(trcyctx);
}

internal Nothing
app_pak_draw_widget_explorer_icon(struct nk_context* ctx,
                                  PakTreeNode* node,
                                  Bool is_dir,
                                  struct nk_image* image,
                                  CStr text) {
  TracyCZoneN(trcyctx, "app_pak_draw_widget_explorer_icon", 1);

  // icon image
  nk_layout_row_static(ctx, APP_PAK_EXPLORER_ICON_IMAGE_HEIGHT,
                       APP_PAK_EXPLORER_ICON_IMAGE_WIDTH, 1);

  // icon image context menue
  struct nk_rect bounds;
  bounds = nk_widget_bounds(ctx);
  if (nk_contextual_begin(ctx, 0, nk_vec2(150, 230), bounds)) {
    nk_layout_row_dynamic(ctx, 20, 1);
    if (nk_contextual_item_label(ctx, "extract this item", NK_TEXT_CENTERED)) {
    }
    nk_contextual_end(ctx);
  }

  if (nk_button_image(ctx, *image)) {
    if (is_dir) {
      S.current_pak_tree_node = node;
    }
  }

  // icon text
  nk_layout_row_static(ctx, APP_PAK_EXPLORER_ICON_TEXT_HEIGHT,
                       APP_PAK_EXPLORER_ICON_TEXT_WIDTH, 1);
  // nk_text(ctx, text, strlen(text), NK_TEXT_ALIGN_LEFT |
  // NK_TEXT_ALIGN_MIDDLE); nk_label_wrap(ctx, text);
  nk_text_wrap(ctx, text, strlen(text));

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */
