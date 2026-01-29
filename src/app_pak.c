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

// #if defined(PROFILING)
// #include "deps/tracy/tracy.h"
// TracyCZoneCtx trcyctx;
// #endif

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
// #include "deps/sepi/profiler.h"
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
  AppPakImage text;
} ICONS;

static struct {
  Bool is_app_styled;
  Bool is_extracting_requested;
  Pak pak;
  AppPakMode mode;
  PakTreeNode* current_pak_tree_node;
  PakTreeNode* requested_extracting_item;
  IONode input_dir;
  FILE* input_file;
  CStr input_path;
  // TODO:
  // maybe use a dynamic array now that we have proper arena allocator
  char export_path_buffer[APP_PAK_MAX_EXPORT_PATH_LENGTH];
  char error_text[APP_PAK_MAX_ERROR_LENGTH];
  Arena* arena;
} S;

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

static Nothing app_pak_input(const sapp_event* event);
static AppPakError app_pak_handle_file_drop(CStr path);
static AppPakError app_pak_handle_file_drop_pak(CStr path);
static AppPakError app_pak_handle_file_drop_dir(CStr path);

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
static Nothing app_pak_draw_mode_loaded(struct nk_context* ctx,
                                        nk_flags window_flags,
                                        U32 window_width,
                                        U32 window_height);
static Nothing app_pak_draw_mode_loaded(struct nk_context* ctx,
                                        nk_flags window_flags,
                                        U32 window_width,
                                        U32 window_height);
static Nothing app_pak_draw_widget_explorer_area(struct nk_context* ctx,
                                                 U32 window_width,
                                                 U32 window_height);
static Nothing app_pak_draw_widget_explorer_item(struct nk_context* ctx,
                                                 PakTreeNode* node);
static Nothing app_pak_draw_widget_explorer_icon(struct nk_context* ctx,
                                                 PakTreeNode* node,
                                                 Bool is_directory,
                                                 struct nk_image* image,
                                                 CStr text);

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

  MemZero(&S, sizeof(S));

  if (sargs_exists("-i"))
    S.input_path = sargs_value("-i");
  else if (sargs_exists("--input"))
    S.input_path = sargs_value("--input");

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

  END_PROFILING();
}

/* ===================================================== */

void
app_pak_init(void) {
  START_PROFILING(1);

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

  if (S.input_path != NULL) {
    log_info("loading '%s' model", S.input_path);
    app_pak_handle_file_drop(S.input_path);
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
  END_PROFILING();
  return err;
}

/* ===================================================== */

static AppPakError
app_pak_init_icons() {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  err = app_pak_init_icon(&ICONS.home, icon_home_png, sizeof(icon_home_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&ICONS.back, icon_back_png, sizeof(icon_back_png));
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }

  err = app_pak_init_icon(&ICONS.package, icon_package_png,
                          sizeof(icon_package_png));
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
    snprintf(S.error_text, APP_PAK_MAX_ERROR_LENGTH,
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

  if (S.input_file) {
    fclose(S.input_file);
    S.input_file = 0;
  }

  // TODO:
  // should we check the loaded directory if there is?

  app_pak_cleanup_icons();
  snk_shutdown();
  sg_shutdown();
  pak_unload(&S.pak);
  arena_destroy(S.arena);

  END_PROFILING();
}

static Nothing
app_pak_cleanup_reload() {
  START_PROFILING(1);

  if (S.input_file) {
    fclose(S.input_file);
    S.input_file = 0;
    S.input_path = 0;
  }

  // TODO:
  // should we clean up opened directory if there is

  S.is_extracting_requested = FALSE;
  S.requested_extracting_item = 0;
  S.mode = APP_PAK_MODE_EMPTY;
  MemZeroArray(S.error_text);

  pak_unload(&S.pak);
  arena_destroy(S.arena);
  S.arena = 0;
  S.arena = arena_create();

  END_PROFILING();
}

static AppPakError
app_pak_cleanup_icons() {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  err = app_pak_cleanup_icon(&ICONS.home);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&ICONS.back);
  if (err != APP_PAK_ERR_SUCCESS) {
    goto cleanup;
  }
  err = app_pak_cleanup_icon(&ICONS.package);
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
app_pak_input(const sapp_event* event) {
  START_PROFILING(1);

  if ((event->type == SAPP_EVENTTYPE_KEY_DOWN) && !event->key_repeat) {
    if (event->key_code == SAPP_KEYCODE_ESCAPE) {
      sapp_request_quit();
    }
  }

  snk_handle_event(event);
  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    app_pak_handle_file_drop(sapp_get_dropped_file_path(0));
  }

  END_PROFILING();
}

/* ===================================================== */

static AppPakError
app_pak_handle_file_drop(CStr path) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  Bool is_directory = FALSE;
  IOError ioerr = io_is_directory(str8((Str)path), &is_directory);
  if (IO_ERR_SUCCESS != ioerr) {
    S.mode = APP_PAK_MODE_FAILED;
    err = APP_PAK_ERR_DROP;
    goto cleanup;
  }

  if (is_directory) {
    app_pak_handle_file_drop_dir(path);
  } else {
    app_pak_handle_file_drop_pak(path);
  }

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static AppPakError
app_pak_handle_file_drop_pak(CStr path) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  // NOTE:
  // we may want to allow this to happen, so the user does not have to
  // re-run the app for a new .PAK file! but this has to reset all the
  // states and wipe the arenas!
  if (S.mode == APP_PAK_MODE_PAK_LOADED || S.mode == APP_PAK_MODE_DIR_LOADED) {
    // goto cleanup;
    app_pak_cleanup_reload();
    // TODO:
    // do we need to call any cleanup function for loaded directory?
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

  S.mode = APP_PAK_MODE_PAK_LOADED;
  S.input_path = path;
  S.current_pak_tree_node = &S.pak.tree.root;

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static AppPakError
app_pak_handle_file_drop_dir(CStr path) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  IOError ioerr =
      io_directory_nested_children(S.arena, str8(path), &S.input_dir);
  if (ioerr != IO_ERR_SUCCESS) {
    Dbg("FAKKK YOOOO");
    err = APP_PAK_ERR_DIR_OPEN;
    goto cleanup;
  }

  Dbg("Noice!");

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
  END_PROFILING();
}

/* ===================================================== */

static U32
app_pak_draw(struct nk_context* ctx) {
  START_PROFILING(1);

  AppPakError err = APP_PAK_ERR_SUCCESS;

  static char window_title[] = "SQV::Pak Explorer";
  static nk_flags window_flags = 0;

  U32 window_width = sapp_width();
  U32 window_height = sapp_height();

  nk_style_hide_cursor(ctx);
  if (S.mode == APP_PAK_MODE_EMPTY) {
    app_pak_draw_mode_empty(ctx, window_flags, window_width, window_height);
  } else if (S.mode == APP_PAK_MODE_PAK_LOADED) {
    app_pak_draw_mode_loaded(ctx, window_flags, window_width, window_height);
  } else if (S.mode == APP_PAK_MODE_FAILED) {
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
    F32 element_width = line_width + 2.0f * text_pad_x;
    F32 element_height = text_height + 2.0f * text_pad_y;

    struct nk_rect r1 = {
        .x = content_region.x + (content_region.w - element_width) * 0.5f,
        .y =
            content_region.y + (content_region.h - (element_height * 4)) * 0.5f,
        .w = element_width,
        .h = element_height,
    };

    struct nk_rect r2 = {
        .x = content_region.x + (content_region.w - element_width) * 0.5f,
        .y = r1.y + element_height,
        .w = element_width,
        .h = element_height,
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

    F32 text_width = font->width(font->userdata, font->height, S.error_text,
                                 (int)strlen(S.error_text));
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
    nk_text(ctx, S.error_text, strlen(S.error_text), NK_TEXT_CENTERED);
    nk_layout_space_end(ctx);
  }
  nk_end(ctx);

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_mode_loaded(struct nk_context* ctx,
                         nk_flags window_flags,
                         U32 window_width,
                         U32 window_height) {
  START_PROFILING(1);

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
    if (nk_popup_begin(ctx, NK_POPUP_DYNAMIC, "Extract",
                       NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR, s)) {
      nk_layout_row_dynamic(ctx, 20, 1);
      nk_label(ctx, "output path:", NK_TEXT_LEFT);
      nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, S.export_path_buffer,
                                     APP_PAK_MAX_EXPORT_PATH_LENGTH,
                                     nk_filter_default);
      nk_layout_row_dynamic(ctx, 0, 3);
      if (nk_button_label(ctx, "ok")) {
        if (0 == S.requested_extracting_item) {
          PakError perr =
              pak_extract(&S.pak, S.input_file, str8(S.export_path_buffer));
          if (PAK_ERR_SUCCESS != perr) {
            snprintf(S.error_text, APP_PAK_MAX_ERROR_LENGTH,
                     "failed to extract pak file into '%s'",
                     S.export_path_buffer);
            S.mode = APP_PAK_MODE_FAILED;
          }
        } else {
          PakError perr =
              pak_extract_item(&S.pak, S.requested_extracting_item,
                               S.input_file, str8(S.export_path_buffer));
          if (PAK_ERR_SUCCESS != perr) {
            snprintf(S.error_text, APP_PAK_MAX_ERROR_LENGTH,
                     "failed to extract item '%s' into '%s'",
                     S.requested_extracting_item->name, S.export_path_buffer);
            S.mode = APP_PAK_MODE_FAILED;
          }
        }

        S.is_extracting_requested = FALSE;
        S.requested_extracting_item = 0;
      }
      nk_label(ctx, "", 0);
      if (nk_button_label(ctx, "cancel")) {
        S.is_extracting_requested = FALSE;
        S.requested_extracting_item = 0;
      }

      nk_popup_end(ctx);
    } else {
      S.is_extracting_requested = FALSE;
      S.requested_extracting_item = 0;
    }
  }

  nk_end(ctx);
  nk_style_pop_style_item(ctx);

  // === BOTTOM REGION ===
  if (nk_begin(ctx, "loaded_mode_bottom_region",
               nk_rect(0, window_height - APP_PAK_BOTTOM_REGION_HEIGHT,
                       window_width, APP_PAK_BOTTOM_REGION_HEIGHT),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx, 0, 1);
    nk_label(ctx, S.input_path, NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
  }
  nk_end(ctx);

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_widget_explorer_area(struct nk_context* ctx,
                                  U32 window_width,
                                  U32 window_height) {
  START_PROFILING(1);

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

  END_PROFILING();
}

/* ===================================================== */

static Nothing
app_pak_draw_widget_explorer_item(struct nk_context* ctx, PakTreeNode* node) {
  START_PROFILING(1);

  if (nk_group_begin(ctx, "", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT)) {
    if (node->is_directory) {
      app_pak_draw_widget_explorer_icon(
          ctx, node, TRUE, &ICONS.folder.icon_image, node->item_name);
    } else {
      app_pak_draw_widget_explorer_icon(
          ctx, node, FALSE, &ICONS.text.icon_image, node->item_name);
    }
    nk_group_end(ctx);
  }

  END_PROFILING();
}

static Nothing
app_pak_draw_widget_explorer_icon(struct nk_context* ctx,
                                  PakTreeNode* node,
                                  Bool is_directory,
                                  struct nk_image* image,
                                  CStr text) {
  START_PROFILING(1);

  // icon image
  nk_layout_row_static(ctx, APP_PAK_EXPLORER_ICON_IMAGE_HEIGHT,
                       APP_PAK_EXPLORER_ICON_IMAGE_WIDTH, 1);

  // icon image context menue
  struct nk_rect bounds;
  bounds = nk_widget_bounds(ctx);
  if (nk_contextual_begin(ctx, 0, nk_vec2(150, 230), bounds)) {
    nk_layout_row_dynamic(ctx, 20, 1);
    if (nk_contextual_item_label(ctx, "extract this item", NK_TEXT_CENTERED)) {
      S.is_extracting_requested = TRUE;
      S.requested_extracting_item = node;
    }
    nk_contextual_end(ctx);
  }

  if (nk_button_image(ctx, *image)) {
    if (is_directory) {
      S.current_pak_tree_node = node;
    }
  }

  // icon text
  nk_layout_row_static(ctx, APP_PAK_EXPLORER_ICON_TEXT_HEIGHT,
                       APP_PAK_EXPLORER_ICON_TEXT_WIDTH, 1);
  // nk_text(ctx, text, strlen(text), NK_TEXT_ALIGN_LEFT |
  // NK_TEXT_ALIGN_MIDDLE); nk_label_wrap(ctx, text);
  nk_text_wrap(ctx, text, strlen(text));

  END_PROFILING();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */
