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
#include "deps/sepi/hashmap.h"
#include "deps/sepi/array.h"
#include "deps/sepi/io.h"

#include "shaders/default.glsl.h"

#include "data_icons.h"
#include "data_style.h"
#include "module_pak.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define APP_PAK_MAX_EXPORT_PATH_LENGTH 1024
#define APP_PAK_MAX_ERROR_LENGTH 512

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  APP_PAK_MODE_EMPTY,
  APP_PAK_MODE_PAK_LOADED,
  APP_PAK_MODE_DIR_LOADED,
  APP_PAK_MODE_FAILED,
} AppPakMode;

typedef enum {
  APP_PAK_ERR_SUCCESS = 1,
  APP_PAK_ERR_DROP,
  APP_PAK_ERR_FILE_OPEN,
  APP_PAK_ERR_DIR_OPEN,
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

static struct {
  AppPakImage home;
  AppPakImage back;
  AppPakImage package;
  AppPakImage extract;
  AppPakImage folder;
  AppPakImage file;
} g_icons;

static struct {
  Bool is_app_styled;
  Bool is_extracting_requested;
  Bool is_packaging_requested;
  AppPakMode mode;
  Pak pak;
  TreeNode* tree_current_node;
  TreeNode* requested_extracting_item;
  IONode input_node;
  IONode* current_dir_node;
  Str8 input_path;

  // TODO:
  // maybe use a dynamic array now that we have proper arena allocator
  char export_path_buffer[APP_PAK_MAX_EXPORT_PATH_LENGTH];
  char error_text[APP_PAK_MAX_ERROR_LENGTH];
  Arena* arena;
} g_state;

MASTER_PROFILING_CONTEXT;

/* ===================================================== */
/*                      DECLERATIONS                     */
/* ===================================================== */

static Nothing app_pak_init();
static AppPakError app_pak_init_style(struct nk_style* s);
static AppPakError app_pak_init_icons();
static AppPakError app_pak_init_icon(AppPakImage* app_icon,
                                     CBuf buffer,
                                     Sz size);
static Nothing app_pak_cleanup();
static Nothing app_pak_cleanup_reload();
static AppPakError app_pak_cleanup_icons();
static AppPakError app_pak_cleanup_icon(AppPakImage* app_icon);

static Nothing app_pak_handle_user_input_events(const sapp_event* event);
static AppPakError app_pak_handle_drop_event(Str8 path);
static AppPakError app_pak_handle_pak(Str8 path);
static AppPakError app_pak_handle_dir(Str8 path);

static Nothing app_pak_frame();
static U32 app_pak_draw(struct nk_context* ctx);
static Nothing app_pak_draw_mode_empty(struct nk_context* ctx,
                                       nk_flags window_flags,
                                       U32 window_width,
                                       U32 window_height);
static Nothing app_pak_draw_mode_failed(struct nk_context* ctx,
                                        nk_flags window_flags,
                                        U32 window_width,
                                        U32 window_height);
static Nothing app_pak_draw_mode_pak_loaded(struct nk_context* ctx,
                                            nk_flags window_flags,
                                            U32 window_width,
                                            U32 window_height);
static Nothing app_pak_draw_mode_dir_loaded(struct nk_context* ctx,
                                            nk_flags window_flags,
                                            U32 window_width,
                                            U32 window_height);
static Nothing app_pak_draw_widget_pak_explorer_area(struct nk_context* ctx,
                                                     U32 window_width,
                                                     U32 window_height);
static Nothing app_pak_draw_widget_dir_explorer_area(struct nk_context* ctx,
                                                     U32 window_width,
                                                     U32 window_height);
static Nothing app_pak_draw_widget_explorer_pak_item(struct nk_context* ctx,
                                                     TreeNode* node);
static Nothing app_pak_draw_widget_explorer_dir_item(struct nk_context* ctx,
                                                     IONode* node);
static Nothing app_pak_draw_widget_explorer_pak_icon(struct nk_context* ctx,
                                                     TreeNode* node,
                                                     Bool is_directory,
                                                     struct nk_image* image,
                                                     Str8 text);
static Nothing app_pak_draw_widget_explorer_dir_icon(struct nk_context* ctx,
                                                     IONode* node,
                                                     Bool is_directory,
                                                     struct nk_image* image,
                                                     Str8 text);
/* ===================================================== */
/*                       FUNCTIONS                       */
/* ===================================================== */

sapp_desc
sokol_main(int argc, char* argv[]) {
  START_PROFILING(1);

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  MemZero(&g_state, sizeof(g_state));

  if (sargs_exists("-i")) {
    g_state.input_path = S(sargs_value("-i"));
  } else if (sargs_exists("--input")) {
    g_state.input_path = S(sargs_value("--input"));
  }

  return (sapp_desc){
      .init_cb = app_pak_init,
      .frame_cb = app_pak_frame,
      .cleanup_cb = app_pak_cleanup,
      .event_cb = app_pak_handle_user_input_events,
      .enable_clipboard = true,
      .width = 640,   // TODO: hard coded, who cares!?
      .height = 480,  // TODO: hard coded
      .enable_dragndrop = true,
      .max_dropped_files = 1,
      .window_title = "SQV::PAK Manager",
      .ios_keyboard_resizes_canvas = true,
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };

  END_PROFILING();
}

/* ===================================================== */

void
app_pak_init(void) {
  START_PROFILING(1);

  g_state.arena = arena_create();

  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  snk_setup(&(snk_desc_t){
      .enable_set_mouse_cursor = true,
      .dpi_scale = sapp_dpi_scale(),
      .logger.func = slog_func,
  });

  g_state.mode = APP_PAK_MODE_EMPTY;

  if (CS(g_state.input_path) != NULL) {
    log_info("loading '%s' model", g_state.input_path);
    app_pak_handle_drop_event(g_state.input_path);
  }

  app_pak_init_icons();

  END_PROFILING();
}

/* ===================================================== */

static AppPakError
app_pak_init_style(struct nk_style* s) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  struct nk_style_window* window = &s->window;

  window->padding.x = STYLE.window.padding.x;
  window->padding.y = STYLE.window.padding.y;
  window->scrollbar_size.x = STYLE.scrollbar.size.x;
  window->scrollbar_size.y = STYLE.scrollbar.size.y;
  window->group_padding.x = STYLE.explorer.padding.x;
  window->group_padding.y = STYLE.explorer.padding.y;

  window->border = STYLE.global.border.size;
  window->combo_border = STYLE.global.border.size;
  window->contextual_border = STYLE.global.border.size;
  window->menu_border = STYLE.global.border.size;
  window->group_border = STYLE.global.border.size;
  window->tooltip_border = STYLE.global.border.size;
  window->popup_border = STYLE.global.border.size;
  window->min_row_height_padding = STYLE.global.border.size;
  window->group_border = STYLE.global.border.size;

  window->border_color.r = STYLE.global.border.color.r;
  window->border_color.g = STYLE.global.border.color.g;
  window->border_color.b = STYLE.global.border.color.b;
  window->border_color.a = 255;

  window->popup_border_color.r = STYLE.global.border.color.r;
  window->popup_border_color.g = STYLE.global.border.color.g;
  window->popup_border_color.b = STYLE.global.border.color.b;
  window->popup_border_color.a = 255;

  window->combo_border_color.r = STYLE.global.border.color.r;
  window->combo_border_color.g = STYLE.global.border.color.g;
  window->combo_border_color.b = STYLE.global.border.color.b;
  window->combo_border_color.a = 255;

  window->contextual_border_color.r = STYLE.global.border.color.r;
  window->contextual_border_color.g = STYLE.global.border.color.g;
  window->contextual_border_color.b = STYLE.global.border.color.b;
  window->contextual_border_color.a = 255;

  window->menu_border_color.r = STYLE.global.border.color.r;
  window->menu_border_color.g = STYLE.global.border.color.g;
  window->menu_border_color.b = STYLE.global.border.color.b;
  window->menu_border_color.a = 255;

  window->group_border_color.r = STYLE.global.border.color.r;
  window->group_border_color.g = STYLE.global.border.color.g;
  window->group_border_color.b = STYLE.global.border.color.b;
  window->group_border_color.a = 255;

  window->tooltip_border_color.r = STYLE.global.border.color.r;
  window->tooltip_border_color.g = STYLE.global.border.color.g;
  window->tooltip_border_color.b = STYLE.global.border.color.b;
  window->tooltip_border_color.a = 255;

  window->background.r = STYLE.dialog.background.color.r;
  window->background.g = STYLE.dialog.background.color.g;
  window->background.b = STYLE.dialog.background.color.b;
  window->background.a = 255;

  window->fixed_background.type = NK_STYLE_ITEM_COLOR;
  window->fixed_background.data.color.r = STYLE.window.color.background.r;
  window->fixed_background.data.color.g = STYLE.window.color.background.g;
  window->fixed_background.data.color.b = STYLE.window.color.background.b;
  window->fixed_background.data.color.a = 255;

  struct nk_style_button* button = &s->button;

  button->normal.data.color.r = STYLE.button.color.normal.r;
  button->normal.data.color.g = STYLE.button.color.normal.g;
  button->normal.data.color.b = STYLE.button.color.normal.b;

  button->hover.data.color.r = STYLE.button.color.hover.r;
  button->hover.data.color.g = STYLE.button.color.hover.g;
  button->hover.data.color.b = STYLE.button.color.hover.b;

  button->active.data.color.r = STYLE.button.color.active.r;
  button->active.data.color.g = STYLE.button.color.active.g;
  button->active.data.color.b = STYLE.button.color.active.b;

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static AppPakError
app_pak_init_icons() {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  err = app_pak_init_icon(&g_icons.home, icon_home_png, sizeof(icon_home_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&g_icons.back, icon_back_png, sizeof(icon_back_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&g_icons.package, icon_package_png,
                          sizeof(icon_package_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&g_icons.extract, icon_extract_png,
                          sizeof(icon_extract_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&g_icons.folder, icon_folder_png,
                          sizeof(icon_folder_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&g_icons.file, icon_file_png, sizeof(icon_file_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static AppPakError
app_pak_init_icon(AppPakImage* app_icon, CBuf buffer, Sz size) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  I32 w, h, c = 0;
  CBuf data = stbi_load_from_memory(buffer, size, &w, &h, &c, 4);
  if (0 == data) {
    err = APP_PAK_ERR_ICON_INIT;
    snprintf(g_state.error_text, APP_PAK_MAX_ERROR_LENGTH,
             "failed to load icon image from memory");
    goto cleanup;
  }
  START_MEMORY_PROFILING(data, size);

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
    free((RawPtr)data);
    END_MEMORY_PROFILING(data);
  }

  END_PROFILING();
  return err;
}

/* ===================================================== */

static Nothing
app_pak_cleanup() {
  START_PROFILING(1);

  io_close_file(&g_state.input_node);
  app_pak_cleanup_icons();
  snk_shutdown();
  sg_shutdown();
  if (APP_PAK_MODE_PAK_LOADED == g_state.mode) {
    pak_unload(&g_state.pak);
  }
  arena_destroy(g_state.arena);

  END_PROFILING();
}

static Nothing
app_pak_cleanup_reload() {
  START_PROFILING(1);

  AppPakMode old_mode = g_state.mode;
  g_state.mode = APP_PAK_MODE_EMPTY;
  g_state.is_extracting_requested = FALSE;
  g_state.is_packaging_requested = FALSE;
  g_state.requested_extracting_item = 0;

  MemZeroArray(g_state.error_text);
  io_close_file(&g_state.input_node);
  if (APP_PAK_MODE_PAK_LOADED == old_mode) {
    pak_unload(&g_state.pak);
  }
  arena_clear(g_state.arena);

  END_PROFILING();
}

static AppPakError
app_pak_cleanup_icons() {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  err = app_pak_cleanup_icon(&g_icons.home);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&g_icons.back);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&g_icons.package);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&g_icons.extract);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&g_icons.folder);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&g_icons.file);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  END_PROFILING();
  return err;
}

static AppPakError
app_pak_cleanup_icon(AppPakImage* app_icon) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  sg_destroy_view(app_icon->view);
  sg_destroy_sampler(app_icon->sampler);
  sg_destroy_image(app_icon->image);
  snk_destroy_image(app_icon->ui_image);

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static Nothing
app_pak_handle_user_input_events(const sapp_event* event) {
  START_PROFILING(1);

  if ((event->type == SAPP_EVENTTYPE_KEY_DOWN) && !event->key_repeat) {
    if (event->key_code == SAPP_KEYCODE_ESCAPE) {
      sapp_request_quit();
    }
  }

  snk_handle_event(event);
  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    app_pak_handle_drop_event(S(sapp_get_dropped_file_path(0)));
  }

  END_PROFILING();
}

/* ===================================================== */

static AppPakError
app_pak_handle_drop_event(Str8 path) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  Bool is_directory = FALSE;
  IOError ioerr = io_is_directory(path, &is_directory);
  if (IO_ERR_SUCCESS != ioerr) {
    g_state.mode = APP_PAK_MODE_FAILED;
    err = APP_PAK_ERR_DROP;
    goto cleanup;
  }

  if (is_directory) {
    app_pak_handle_dir(path);
  } else {
    app_pak_handle_pak(path);
  }

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static AppPakError
app_pak_handle_pak(Str8 path) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  if (g_state.mode == APP_PAK_MODE_PAK_LOADED ||
      g_state.mode == APP_PAK_MODE_DIR_LOADED) {
    // NOTE:
    // we don't check the error cuz it does not return anything!
    app_pak_cleanup_reload();
  }

  IOError ioerr = io_open_file(g_state.arena, path, &g_state.input_node);
  if (IO_ERR_SUCCESS != ioerr) {
    snprintf(g_state.error_text, APP_PAK_MAX_ERROR_LENGTH,
             "failed to open '%s'", CS(path));
    err = APP_PAK_ERR_FILE_OPEN;
    g_state.mode = APP_PAK_MODE_FAILED;
    goto cleanup;
  }

  PakError perr = pak_load_from_file(&g_state.pak, &g_state.input_node);
  if (perr != PAK_ERR_SUCCESS) {
    snprintf(g_state.error_text, APP_PAK_MAX_ERROR_LENGTH,
             "failed to load '%s' items", CS(path));
    err = APP_PAK_ERR_MODULE_PAK;
    g_state.mode = APP_PAK_MODE_FAILED;
    goto cleanup;
  }

  g_state.mode = APP_PAK_MODE_PAK_LOADED;
  g_state.input_path = path;
  g_state.tree_current_node = tree_root(g_state.pak.tree);

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static AppPakError
app_pak_handle_dir(Str8 path) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  if (g_state.mode == APP_PAK_MODE_PAK_LOADED ||
      g_state.mode == APP_PAK_MODE_DIR_LOADED) {
    // NOTE:
    // we don't check the error cuz it does not return anything!
    app_pak_cleanup_reload();
  }

  IOError ioerr =
      io_directory_nested_children(g_state.arena, path, &g_state.input_node);
  if (ioerr != IO_ERR_SUCCESS) {
    err = APP_PAK_ERR_DIR_OPEN;
    goto cleanup;
  }

  g_state.mode = APP_PAK_MODE_DIR_LOADED;
  g_state.input_path = path;
  g_state.current_dir_node = &g_state.input_node;

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static Nothing
app_pak_frame() {
  START_PROFILING(1);
  TracyCFrameMarkStart(0);

  struct nk_context* ctx = snk_new_frame();

  if (g_state.is_app_styled == FALSE) {
    app_pak_init_style(&ctx->style);
    g_state.is_app_styled = TRUE;
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
  END_PROFILING();
}

/* ===================================================== */

static U32
app_pak_draw(struct nk_context* ctx) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  static char window_title[] = "SQV::Pak Explorer";
  static nk_flags window_flags = NK_WINDOW_BORDER;

  U32 window_width = sapp_width();
  U32 window_height = sapp_height();

  nk_style_hide_cursor(ctx);
  if (g_state.mode == APP_PAK_MODE_EMPTY) {
    app_pak_draw_mode_empty(ctx, window_flags, window_width, window_height);
  } else if (g_state.mode == APP_PAK_MODE_PAK_LOADED) {
    app_pak_draw_mode_pak_loaded(ctx, window_flags, window_width,
                                 window_height);
  } else if (g_state.mode == APP_PAK_MODE_DIR_LOADED) {
    app_pak_draw_mode_dir_loaded(ctx, window_flags, window_width,
                                 window_height);
  } else if (g_state.mode == APP_PAK_MODE_FAILED) {
    app_pak_draw_mode_failed(ctx, window_flags, window_width, window_height);
  }

  END_PROFILING();
  return !nk_window_is_closed(ctx, window_title);
}

/* ===================================================== */

static Nothing
app_pak_draw_mode_empty(struct nk_context* ctx,
                        nk_flags window_flags,
                        U32 window_width,
                        U32 window_height) {
  START_PROFILING(1);

  static char label_line_1[] = "drop a .pak file for extraction";
  static char label_line_2[] = "or drop a folder for packaging as .pak file";

  if (nk_begin(ctx, "", nk_rect(0, 0, window_width, window_height),
               window_flags)) {
    struct nk_rect content_region = nk_window_get_content_region(ctx);
    const struct nk_user_font* font = ctx->style.font;

    F32 line_width = font->width(font->userdata, font->height, label_line_2,
                                 (int)strlen(label_line_2));

    F32 text_height = font->height;
    F32 text_pad_x = ctx->style.text.padding.x;
    F32 text_pad_y = ctx->style.text.padding.y;

    struct nk_rect r1 = {
        .x = content_region.x + (content_region.w - line_width) * 0.5f,
        .y = content_region.y + (content_region.h - (text_height * 4)) * 0.5f,
        .w = line_width,
        .h = text_height,
    };

    struct nk_rect r2 = {
        .x = content_region.x + (content_region.w - line_width) * 0.5f,
        .y = r1.y + text_height,
        .w = line_width,
        .h = text_height,
    };

    nk_layout_space_begin(ctx, NK_STATIC, content_region.h, 2);
    nk_layout_space_push(ctx, r1);
    nk_label(ctx, label_line_1, NK_TEXT_CENTERED);
    nk_layout_space_push(ctx, r2);
    nk_label(ctx, label_line_2, NK_TEXT_CENTERED);
    nk_layout_space_end(ctx);
  }
  nk_end(ctx);

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_mode_failed(struct nk_context* ctx,
                         nk_flags window_flags,
                         U32 window_width,
                         U32 window_height) {
  START_PROFILING(1);

  if (nk_begin(ctx, "", nk_rect(0, 0, window_width, window_height),
               window_flags)) {
    struct nk_rect content_region = nk_window_get_content_region(ctx);
    const struct nk_user_font* font = ctx->style.font;

    F32 text_width =
        font->width(font->userdata, font->height, g_state.error_text,
                    (int)strlen(g_state.error_text));
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
    nk_text(ctx, g_state.error_text, strlen(g_state.error_text),
            NK_TEXT_CENTERED);
    nk_layout_space_end(ctx);
  }
  nk_end(ctx);

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_mode_pak_loaded(struct nk_context* ctx,
                             nk_flags window_flags,
                             U32 window_width,
                             U32 window_height) {
  START_PROFILING(1);

  /* === TOP REGION === */
  if (nk_begin(ctx, "loaded_mode_top_region",
               nk_rect(0, 0, window_width, STYLE.toolbar.height),
               NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
    nk_layout_row_template_begin(ctx, STYLE.toolbar.icon.height);
    nk_layout_row_template_push_static(ctx, STYLE.toolbar.icon.height);
    if (g_state.tree_current_node != tree_root(g_state.pak.tree)) {
      nk_layout_row_template_push_static(ctx, STYLE.toolbar.icon.height);
      nk_layout_row_template_push_static(ctx, STYLE.toolbar.icon.height);
    }
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);

    if (nk_button_image(ctx, g_icons.extract.icon_image)) {
      g_state.is_extracting_requested = TRUE;
    }

    if (g_state.tree_current_node != tree_root(g_state.pak.tree)) {
      if (nk_button_image(ctx, g_icons.home.icon_image)) {
        g_state.tree_current_node = tree_root(g_state.pak.tree);
      }

      if (nk_button_image(ctx, g_icons.back.icon_image)) {
        if (g_state.tree_current_node->parent) {
          g_state.tree_current_node = g_state.tree_current_node->parent;
        }
      }
    }

    PakNode* pnode = (PakNode*)g_state.tree_current_node->data;
    nk_label(ctx, pnode->name.cstr, NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
  }
  nk_end(ctx);

  /* === MIDDLE REGION === */
  U32 middle_region_height =
      window_height - STYLE.toolbar.height - STYLE.statusbar.height;
  struct nk_style_item bkg = {
      .type = NK_STYLE_ITEM_COLOR,
      .data = {.color = {
                   .r = STYLE.explorer.color.background.r,
                   .g = STYLE.explorer.color.background.g,
                   .b = STYLE.explorer.color.background.b,
                   .a = STYLE.explorer.color.background.a,
               }}};

  nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, bkg);

  if (nk_begin(
          ctx, "loaded_mode_middle_region",
          nk_rect(0, STYLE.toolbar.height, window_width, middle_region_height),
          NK_WINDOW_SCROLL_AUTO_HIDE | NK_WINDOW_BORDER)) {
    const struct nk_user_font* font = ctx->style.font;
    F32 text_height = font->height;
    app_pak_draw_widget_pak_explorer_area(ctx, window_width,
                                          middle_region_height);
  }

  if (g_state.is_extracting_requested) {
    struct nk_rect s = {
        .x = STYLE.dialog.rectangle.x,
        .y = STYLE.dialog.rectangle.y,
        .w = window_width - STYLE.dialog.rectangle.w,
        .h = STYLE.dialog.rectangle.h,
    };
    if (nk_popup_begin(ctx, NK_POPUP_DYNAMIC, "Extract",
                       NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER, s)) {
      nk_layout_row_dynamic(ctx, STYLE.dialog.font.height, 1);
      nk_label(ctx, "output path:", NK_TEXT_LEFT);
      nk_edit_string_zero_terminated(
          ctx, NK_EDIT_FIELD, g_state.export_path_buffer,
          APP_PAK_MAX_EXPORT_PATH_LENGTH, nk_filter_default);
      nk_layout_row_dynamic(ctx, 0, 3);
      if (nk_button_label(ctx, "ok")) {
        if (0 == g_state.requested_extracting_item) {
          PakError perr = pak_extract(&g_state.pak, &g_state.input_node,
                                      S(g_state.export_path_buffer));
          if (PAK_ERR_SUCCESS != perr) {
            snprintf(g_state.error_text, APP_PAK_MAX_ERROR_LENGTH,
                     "failed to extract pak file into '%s'",
                     g_state.export_path_buffer);
            g_state.mode = APP_PAK_MODE_FAILED;
          }
        } else {
          PakNode* pnode = (PakNode*)g_state.requested_extracting_item->data;
          PakError perr =
              pak_extract_item(&g_state.pak, pnode, &g_state.input_node,
                               S(g_state.export_path_buffer));
          if (PAK_ERR_SUCCESS != perr) {
            snprintf(g_state.error_text, APP_PAK_MAX_ERROR_LENGTH,
                     "failed to extract item '%s' into '%s'", pnode->name.cstr,
                     g_state.export_path_buffer);
            g_state.mode = APP_PAK_MODE_FAILED;
          }
        }

        g_state.is_extracting_requested = FALSE;
        g_state.requested_extracting_item = 0;
      }
      nk_label(ctx, "", 0);
      if (nk_button_label(ctx, "cancel")) {
        g_state.is_extracting_requested = FALSE;
        g_state.requested_extracting_item = 0;
      }

      nk_popup_end(ctx);
    } else {
      g_state.is_extracting_requested = FALSE;
      g_state.requested_extracting_item = 0;
    }
  }

  nk_end(ctx);
  nk_style_pop_style_item(ctx);

  /* === BOTTOM REGION === */
  if (nk_begin(ctx, "loaded_mode_bottom_region",
               nk_rect(0, window_height - STYLE.statusbar.height, window_width,
                       STYLE.statusbar.height),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx, 0, 1);
    nk_label(ctx, CS(g_state.input_node.path),
             NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
  }
  nk_end(ctx);

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_mode_dir_loaded(struct nk_context* ctx,
                             nk_flags window_flags,
                             U32 window_width,
                             U32 window_height) {
  START_PROFILING(1);

  /* === TOP REGION === */
  if (nk_begin(ctx, "loaded_mode_top_region",
               nk_rect(0, 0, window_width, STYLE.toolbar.height),
               NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
    nk_layout_row_template_begin(ctx, STYLE.toolbar.icon.height);
    if (g_state.current_dir_node == &g_state.input_node) {
      nk_layout_row_template_push_static(ctx, STYLE.toolbar.icon.height);
    } else {
      nk_layout_row_template_push_static(ctx, STYLE.toolbar.icon.height);
      nk_layout_row_template_push_static(ctx, STYLE.toolbar.icon.height);
    }
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);

    if (g_state.current_dir_node == &g_state.input_node) {
      if (nk_button_image(ctx, g_icons.package.icon_image)) {
        g_state.is_packaging_requested = TRUE;
      }
    } else {
      if (nk_button_image(ctx, g_icons.home.icon_image)) {
        g_state.current_dir_node = &g_state.input_node;
      }

      if (nk_button_image(ctx, g_icons.back.icon_image)) {
        if (g_state.current_dir_node->parent) {
          g_state.current_dir_node = g_state.current_dir_node->parent;
        }
      }
    }

    nk_label(ctx, CS(g_state.current_dir_node->path),
             NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
  }
  nk_end(ctx);

  /* === MIDDLE REGION === */
  U32 middle_region_height =
      window_height - STYLE.toolbar.height - STYLE.statusbar.height;
  struct nk_style_item bkg = {
      .type = NK_STYLE_ITEM_COLOR,
      .data = {.color = {
                   .r = STYLE.explorer.color.background.r,
                   .g = STYLE.explorer.color.background.g,
                   .b = STYLE.explorer.color.background.b,
                   .a = STYLE.explorer.color.background.a,
               }}};

  nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, bkg);

  if (nk_begin(
          ctx, "loaded_mode_middle_region",
          nk_rect(0, STYLE.toolbar.height, window_width, middle_region_height),
          NK_WINDOW_SCROLL_AUTO_HIDE | NK_WINDOW_BORDER)) {
    const struct nk_user_font* font = ctx->style.font;
    F32 text_height = font->height;

    app_pak_draw_widget_dir_explorer_area(ctx, window_width,
                                          middle_region_height);
  }

  if (g_state.is_packaging_requested) {
    struct nk_rect s = {
        .x = STYLE.dialog.rectangle.x,
        .y = STYLE.dialog.rectangle.y,
        .w = window_width - STYLE.dialog.rectangle.w,
        .h = STYLE.dialog.rectangle.h,
    };
    if (nk_popup_begin(
            ctx, NK_POPUP_DYNAMIC, "Genetate PAK",
            NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER,
            s)) {
      nk_layout_row_dynamic(ctx, 0, 1);
      nk_label(ctx, "output path:", NK_TEXT_LEFT);
      nk_edit_string_zero_terminated(
          ctx, NK_EDIT_FIELD, g_state.export_path_buffer,
          APP_PAK_MAX_EXPORT_PATH_LENGTH, nk_filter_default);
      nk_layout_row_dynamic(ctx, 0, 3);
      if (nk_button_label(ctx, "ok")) {
        PakError pakerr = pak_generate(&g_state.pak, &g_state.input_node);
        if (PAK_ERR_SUCCESS != pakerr) {
          snprintf(g_state.error_text, APP_PAK_MAX_ERROR_LENGTH,
                   "failed to generate pak file from '%s'",
                   CS(g_state.input_path));
          g_state.mode = APP_PAK_MODE_FAILED;
        }
        g_state.is_packaging_requested = FALSE;
      }
    }
    nk_label(ctx, "", 0);
    if (nk_button_label(ctx, "cancel")) {
      g_state.is_packaging_requested = FALSE;
    }

    nk_popup_end(ctx);
  } else {
    g_state.is_packaging_requested = FALSE;
  }

  nk_end(ctx);
  nk_style_pop_style_item(ctx);

  /* === BOTTOM REGION === */
  if (nk_begin(ctx, "loaded_mode_bottom_region",
               nk_rect(0, window_height - STYLE.statusbar.height, window_width,
                       STYLE.statusbar.height),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx, 0, 1);
    nk_label(ctx, CS(g_state.input_node.path),
             NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
  }
  nk_end(ctx);

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_widget_pak_explorer_area(struct nk_context* ctx,
                                      U32 window_width,
                                      U32 window_height) {
  START_PROFILING(1);

  ArrayOf(PakNode) children = g_state.tree_current_node->children;
  U32 items_count = array_length(children);
  U32 columns = (window_width + STYLE.explorer.icon.gap) /
                (STYLE.explorer.icon.text.width + STYLE.explorer.icon.gap);
  columns = columns ? columns : 1;
  U32 rows = (items_count / columns);
  U32 remainder = items_count % columns;
  U32 row_height = STYLE.explorer.icon.image.height +
                   STYLE.explorer.icon.text.height + STYLE.explorer.icon.gap +
                   STYLE.explorer.padding.x;
  U32 actual_height = rows * row_height;
  if (remainder > 0) {
    actual_height += row_height;
  }

  if (actual_height < window_height) {
    actual_height = window_height -
                    ((STYLE.explorer.icon.gap + STYLE.explorer.padding.x) * 2);
  }

  nk_layout_row_dynamic(ctx, actual_height, 1);
  if (nk_group_begin(ctx, "", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
    nk_layout_row_dynamic(
        ctx,
        (STYLE.explorer.icon.image.height + STYLE.explorer.icon.text.height),
        columns);
    U32 index = 0;
    for (U32 row = 0; row < rows; row++) {
      for (U32 column = 0; column < columns; column++) {
        if (index >= items_count)
          break;
        TreeNode* child = array_get(children, index);
        index++;

        app_pak_draw_widget_explorer_pak_item(ctx, child);
      }
    }
    for (U32 column = 0; column < remainder; column++) {
      TreeNode* child = array_get(children, index);
      index++;

      app_pak_draw_widget_explorer_pak_item(ctx, child);
    }
    nk_group_end(ctx);
  }

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_widget_dir_explorer_area(struct nk_context* ctx,
                                      U32 window_width,
                                      U32 window_height) {
  START_PROFILING(1);

  Array* children = g_state.current_dir_node->children;
  U32 items_count = children->offset;
  U32 columns = (window_width + STYLE.explorer.icon.gap) /
                (STYLE.explorer.icon.text.width + STYLE.explorer.icon.gap);
  columns = columns ? columns : 1;
  U32 rows = (items_count / columns);
  U32 remainder = items_count % columns;
  U32 row_height = STYLE.explorer.icon.image.height +
                   STYLE.explorer.icon.text.height + STYLE.explorer.icon.gap +
                   STYLE.explorer.padding.x;
  U32 actual_height = rows * row_height;
  if (remainder > 0) {
    actual_height += row_height;
  }

  if (actual_height < window_height) {
    actual_height = window_height -
                    ((STYLE.explorer.icon.gap + STYLE.explorer.padding.x) * 2);
  }

  nk_layout_row_dynamic(ctx, actual_height, 1);
  if (nk_group_begin(ctx, "", NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(
        ctx,
        (STYLE.explorer.icon.image.height + STYLE.explorer.icon.text.height),
        columns);
    U32 index = 0;
    for (U32 row = 0; row < rows; row++) {
      for (U32 column = 0; column < columns; column++) {
        if (index >= items_count)
          break;
        IONode* node = array_get(children, index);
        index++;

        app_pak_draw_widget_explorer_dir_item(ctx, node);
      }
    }
    for (U32 column = 0; column < remainder; column++) {
      IONode* node = array_get(children, index);
      index++;

      app_pak_draw_widget_explorer_dir_item(ctx, node);
    }
    nk_group_end(ctx);
  }

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_widget_explorer_pak_item(struct nk_context* ctx, TreeNode* node) {
  START_PROFILING(1);

  PakNode* pnode = (PakNode*)node->data;

  if (nk_group_begin(
          ctx, "",
          NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT | NK_WINDOW_BORDER)) {
    if (pnode->is_directory) {
      app_pak_draw_widget_explorer_pak_icon(
          ctx, node, TRUE, &g_icons.folder.icon_image, pnode->name);
    } else {
      app_pak_draw_widget_explorer_pak_icon(
          ctx, node, FALSE, &g_icons.file.icon_image, pnode->name);
    }
    nk_group_end(ctx);
  }

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_widget_explorer_dir_item(struct nk_context* ctx, IONode* node) {
  START_PROFILING(1);

  if (nk_group_begin(
          ctx, "",
          NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT | NK_WINDOW_BORDER)) {
    if (node->is_directory) {
      app_pak_draw_widget_explorer_dir_icon(
          ctx, node, TRUE, &g_icons.folder.icon_image, node->name);
    } else {
      app_pak_draw_widget_explorer_dir_icon(
          ctx, node, FALSE, &g_icons.file.icon_image, node->name);
    }
    nk_group_end(ctx);
  }

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_widget_explorer_pak_icon(struct nk_context* ctx,
                                      TreeNode* node,
                                      Bool is_directory,
                                      struct nk_image* image,
                                      Str8 text) {
  START_PROFILING(1);

  // icon image
  nk_layout_row_static(ctx, STYLE.explorer.icon.image.height,
                       STYLE.explorer.icon.image.width, 1);

  // icon image context menue
  struct nk_rect bounds;
  bounds = nk_widget_bounds(ctx);

  if (APP_PAK_MODE_PAK_LOADED == g_state.mode) {
    if (nk_contextual_begin(
            ctx, 0,
            nk_vec2(STYLE.explorer.contextual.w, STYLE.explorer.contextual.h),
            bounds)) {
      nk_layout_row_dynamic(ctx, 0, 1);
      if (nk_contextual_item_label(ctx, "extract this item",
                                   NK_TEXT_CENTERED)) {
        g_state.is_extracting_requested = TRUE;
        g_state.requested_extracting_item = node;
      }
      nk_contextual_end(ctx);
    }
  }

  if (nk_button_image(ctx, *image)) {
    if (is_directory) {
      g_state.tree_current_node = node;
    }
  }

  // icon text
  nk_layout_row_static(ctx, STYLE.explorer.icon.text.height,
                       STYLE.explorer.icon.text.width, 1);
  nk_text_wrap(ctx, CS(text), SL(text));

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_widget_explorer_dir_icon(struct nk_context* ctx,
                                      IONode* node,
                                      Bool is_directory,
                                      struct nk_image* image,
                                      Str8 text) {
  START_PROFILING(1);

  // icon image
  nk_layout_row_static(ctx, STYLE.explorer.icon.image.height,
                       STYLE.explorer.icon.image.width, 1);

  // icon image context menue
  struct nk_rect bounds;
  bounds = nk_widget_bounds(ctx);

  if (nk_button_image(ctx, *image)) {
    if (is_directory) {
      g_state.current_dir_node = node;
    }
  }

  // icon text
  nk_layout_row_static(ctx, STYLE.explorer.icon.text.height,
                       STYLE.explorer.icon.text.width, 1);
  nk_text_wrap(ctx, CS(text), SL(text));

  END_PROFILING();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */
