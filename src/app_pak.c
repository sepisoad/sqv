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
#define APP_PAK_EXPLORER_ICON_TEXT_WIDTH 350
#define APP_PAK_EXPLORER_ICON_GAP 2
#define APP_PAK_EXPLORER_PADDING_X 2
#define APP_PAK_EXPLORER_PADDING_Y 2
#define APP_PAK_STATUSBAR_HEIGHT 20

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  APP_PAK_MODE_EMPTY,
  APP_PAK_MODE_LOADED,
  APP_PAK_MODE_LOAD_FAILED,
} AppPakMode;

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
  Pak pak;
  AppPakMode mode;
  PakTreeNode* current_pak_tree_node;
  CBuf input_pak_file_path;
} S;

/* ===================================================== */
/*                      DECLERATIONS                     */
/* ===================================================== */

internal Nothing app_pak_init();
internal Nothing app_pak_init_style(struct nk_style* s);
internal Nothing app_pak_init_icons();
internal Nothing app_pak_init_icon(AppPakImage* app_icon, CBuf buffer, Sz size);
internal Nothing app_pak_cleanup();
internal Nothing app_pak_cleanup_icons();
internal Nothing app_pak_cleanup_icon(AppPakImage* app_icon);
internal Nothing app_pak_input(const sapp_event* event);
internal Nothing app_pak_frame();
internal Nothing app_pak_handle_file_drop(CBuf path);
internal U32 app_pak_draw(struct nk_context* ctx);
internal U32 app_pak_draw_mode_empty(struct nk_context* ctx,
                                     nk_flags window_flags,
                                     U32 window_width,
                                     U32 window_height);
internal U32 app_pak_draw_mode_loaded(struct nk_context* ctx,
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
      .width = 640,   // TODO: hard coded
      .height = 480,  // TODO: hard coded
      .enable_dragndrop = true,
      .max_dropped_files = 1,
      .window_title = "nuklear (sokol-app)",
      .ios_keyboard_resizes_canvas = true,
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };
}

/* ===================================================== */

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

  if (S.input_pak_file_path != NULL) {
    log_info("loading '%s' model", S.input_pak_file_path);
    app_pak_handle_file_drop(S.input_pak_file_path);
  }

  app_pak_init_icons();
}

/* ===================================================== */

internal Nothing
app_pak_init_style(struct nk_style* s) {
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
}

/* ===================================================== */

internal Nothing
app_pak_init_icons() {
  app_pak_init_icon(&ICONS.home, icon_home_png, sizeof(icon_home_png));
  app_pak_init_icon(&ICONS.back, icon_back_png, sizeof(icon_back_png));
  app_pak_init_icon(&ICONS.save, icon_save_png, sizeof(icon_save_png));
  app_pak_init_icon(&ICONS.extract, icon_extract_png, sizeof(icon_extract_png));
  app_pak_init_icon(&ICONS.folder, icon_folder_png, sizeof(icon_folder_png));
  app_pak_init_icon(&ICONS.text, icon_text_png, sizeof(icon_text_png));
}

/* ===================================================== */

internal Nothing
app_pak_init_icon(AppPakImage* app_icon, CBuf buffer, Sz size) {
  U32 w, h, c = 0;
  CStr data = stbi_load_from_memory(buffer, size, &w, &h, &c, 4);

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

  // TODO: if i could use arena allocator with stb,
  //       then i could avoid making direct call to free!
  free(data);
}

/* ===================================================== */

internal Nothing
app_pak_cleanup() {
  app_pak_cleanup_icons();
  snk_shutdown();
  sg_shutdown();
  pak_unload(&S.pak);
}

internal Nothing
app_pak_cleanup_icons() {
  app_pak_cleanup_icon(&ICONS.home);
  app_pak_cleanup_icon(&ICONS.back);
  app_pak_cleanup_icon(&ICONS.save);
  app_pak_cleanup_icon(&ICONS.extract);
  app_pak_cleanup_icon(&ICONS.folder);
  app_pak_cleanup_icon(&ICONS.text);
}

internal Nothing
app_pak_cleanup_icon(AppPakImage* app_icon) {
  sg_destroy_view(app_icon->view);
  sg_destroy_sampler(app_icon->sampler);
  sg_destroy_image(app_icon->image);
  snk_destroy_image(app_icon->ui_image);
}

/* ===================================================== */

internal Nothing
app_pak_input(const sapp_event* event) {
  // Dbg("app_pak_input() ...");

  snk_handle_event(event);
  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    app_pak_handle_file_drop(sapp_get_dropped_file_path(0));
  }
}

/* ===================================================== */

internal Nothing
app_pak_frame() {
  // Dbg("app_pak_frame() ...");

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
}

/* ===================================================== */

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

  Arena* arena = arena_create();

  NDBuffer ndb = {0};
  IOError ioerr = io_load_file(arena, path, &ndb);
  if (ioerr != IO_ERR_SUCCESS) {
    S.mode = APP_PAK_MODE_LOAD_FAILED;
    return;
  }

  PakError pakerr = pak_load(&S.pak, &ndb);
  if (pakerr != PAK_ERR_SUCCESS) {
    S.mode = APP_PAK_MODE_LOAD_FAILED;
    return;
  }
  arena_destroy(arena);

  S.mode = APP_PAK_MODE_LOADED;
  S.input_pak_file_path = path;
  S.current_pak_tree_node = &S.pak.tree.root;
}

/* ===================================================== */

internal U32
app_pak_draw(struct nk_context* ctx) {
  // Dbg("app_pak_draw() ...");

  internal CBuf window_title = "SQV::Pak Explorer";
  internal nk_flags window_flags = 0;

  U32 window_width = sapp_width();
  U32 window_height = sapp_height();

  nk_style_hide_cursor(ctx);
  if (S.mode == APP_PAK_MODE_EMPTY) {
    app_pak_draw_mode_empty(ctx, window_flags, window_width, window_height);
  } else if (S.mode == APP_PAK_MODE_LOADED) {
    app_pak_draw_mode_loaded(ctx, window_flags, window_width, window_height);
  }

  return !nk_window_is_closed(ctx, window_title);
}

/* ===================================================== */

internal U32
app_pak_draw_mode_empty(struct nk_context* ctx,
                        nk_flags window_flags,
                        U32 window_width,
                        U32 window_height) {
  // Dbg("app_pak_draw_mode_empty() ...");

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
}

/* ===================================================== */

internal U32
app_pak_draw_mode_loaded(struct nk_context* ctx,
                         nk_flags window_flags,
                         U32 window_width,
                         U32 window_height) {
  // Dbg("app_pak_draw_mode_loaded() ...");

  Bool is_root = S.current_pak_tree_node->name[0] == ' ';

  // === TOP REGION ===
  if (nk_begin(ctx, "loaded_mode_top_region",
               nk_rect(0, 0, window_width, APP_PAK_TOP_REGION_HEIGHT),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_template_begin(ctx, ICONS.home.icon_image.h);
    if (S.pak.is_modified) {
      nk_layout_row_template_push_static(ctx, 30);
    }
    if (is_root == TRUE) {
      nk_layout_row_template_push_static(ctx, 30);
    }
    nk_layout_row_template_push_static(ctx, 30);
    nk_layout_row_template_push_static(ctx, 30);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);

    if (S.pak.is_modified == TRUE) {
      if (nk_button_image(ctx, ICONS.save.icon_image)) {
        // S.current_pak_tree_node = &S.pak.tree.root;
      }
    }

    if (is_root == TRUE) {
      if (nk_button_image(ctx, ICONS.extract.icon_image)) {
        // S.current_pak_tree_node = &S.pak.tree.root;
      }
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
  nk_end(ctx);
  nk_style_pop_style_item(ctx);

  // === BOTTOM REGION ===
  if (nk_begin(ctx, "loaded_mode_bottom_region",
               nk_rect(0, window_height - APP_PAK_BOTTOM_REGION_HEIGHT,
                       window_width, APP_PAK_BOTTOM_REGION_HEIGHT),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx, 0, 1);
    nk_label(ctx, S.input_pak_file_path,
             NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
  }
  nk_end(ctx);
}

/* ===================================================== */

internal Nothing
app_pak_draw_widget_explorer_area(struct nk_context* ctx,
                                  U32 window_width,
                                  U32 window_height) {
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
        if (((PakTreeNode*)kv->v_rawptr)->is_deleted == FALSE) {
          app_pak_draw_widget_explorer_item(ctx, (PakTreeNode*)kv->v_rawptr);
        }
      }
    }
    for (U32 column = 0; column < remainder; column++) {
      HashMapKV* kv = hashmap_key_at(children, index);
      index++;
      if (((PakTreeNode*)kv->v_rawptr)->is_deleted == FALSE) {
        app_pak_draw_widget_explorer_item(ctx, (PakTreeNode*)kv->v_rawptr);
      }
    }
    nk_group_end(ctx);
  }
}

/* ===================================================== */

internal Nothing
app_pak_draw_widget_explorer_item(struct nk_context* ctx, PakTreeNode* node) {
  if (nk_group_begin(ctx, "", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT)) {
    if (node->is_dir) {
      app_pak_draw_widget_explorer_icon(ctx, node, TRUE,
                                        &ICONS.folder.icon_image, node->name);
    } else {
      app_pak_draw_widget_explorer_icon(ctx, node, FALSE,
                                        &ICONS.text.icon_image, node->name);
    }
    nk_group_end(ctx);
  }
}

internal Nothing
app_pak_draw_widget_explorer_icon(struct nk_context* ctx,
                                  PakTreeNode* node,
                                  Bool is_dir,
                                  struct nk_image* image,
                                  CStr text) {
  // icon image
  nk_layout_row_static(ctx, APP_PAK_EXPLORER_ICON_IMAGE_HEIGHT,
                       APP_PAK_EXPLORER_ICON_IMAGE_WIDTH, 1);

  // icon image context menue
  struct nk_rect bounds;
  bounds = nk_widget_bounds(ctx);
  if (nk_contextual_begin(ctx, 0, nk_vec2(100, 300), bounds)) {
    nk_layout_row_dynamic(ctx, 15, 1);
    if (nk_contextual_item_label(ctx, "view", NK_TEXT_CENTERED)) {
      if (is_dir) {
        S.current_pak_tree_node = node;
      }
    }
    if (nk_contextual_item_label(ctx, "delete", NK_TEXT_CENTERED)) {
      S.pak.is_modified = TRUE;
      node->is_deleted = TRUE;
      node->actual_count--;
    }
    if (nk_contextual_item_label(ctx, "extract", NK_TEXT_CENTERED)) {
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
  nk_label(ctx, text, NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */
