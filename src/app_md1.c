/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#define MODULE_MD1_IMPLEMENTATION

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdio.h>

#include <deps/hmm/hmm.h>
#include <deps/log/log.h>
#include <stb/stb_image.h>
#include <deps/nuklear/nuklear.h>
#include <deps/sokol/sokol_app.h>
#include <deps/sokol/sokol_args.h>
#include <deps/sokol/sokol_gfx.h>
#include <deps/sokol/sokol_glue.h>
#include <deps/sokol/sokol_log.h>
#include <deps/sokol/sokol_nuklear.h>
#include <deps/sokol/sokol_time.h>
#include <deps/sepi/base.h>
#include <sepi/app.h>
#include <deps/sepi/io.h>

#if defined(OS_LINUX)
#include "shaders/default.ogl.h"
#include "shaders/bbox.ogl.h"
#elif defined(OS_MACOS)
#include "shaders/default.mtl.h"
#include "shaders/bbox.mtl.h"
#elif defined(OS_WINDOWS)
#include "shaders/default.d3d.h"
#include "shaders/bbox.d3d.h"
#endif

#include "data_icons.h"
#include "data_style.h"
#include "module_md1.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define APP_MD1_WINDOW_WIDTH 640
#define APP_MD1_WINDOW_HEIGHT 480
#define APP_MD1_MAX_ERROR_LENGTH 512

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  APP_MD1_MODE_EMPTY,
  APP_MD1_MODE_LOADED,
  APP_MD1_MODE_FAILED,
} AppMd1Mode;

typedef enum {
  APP_MD1_ERR_SUCCESS = 1,
  APP_MD1_ERR_DROP,
  APP_MD1_ERR_FILE_OPEN,
  APP_MD1_ERR_ICON_INIT,
  APP_MD1_ERR_MODULE_MD1,
  APP_MD1_ERR__COUNT,
} AppMd1Error;

// TODO:
// do i need this?
typedef struct {
  sg_image image;
  sg_view view;
  sg_sampler sampler;
  snk_image_t ui_image;
  nk_handle handle;
  struct nk_image icon_image;
} AppMd1Image;

/* ===================================================== */
/*                        GLOBALS                        */
/* ===================================================== */

mount_master_profiling_context();

DefineFStr(APP_MD1_MAX_ERROR_LENGTH);

local struct {
  App app;
  AppMd1Mode mode;
  Bool is_app_styled;
  Md1 md1;
  U32 pose;
  U32 frame;
  U32 skin;
  U32 zoom;
  F32 rotation_y;
  Str input_path;
  IOFile input_io_file;
  Str512 error_text;
  struct {
    sg_pass_action pass_action;
  } display;
  struct {
    struct {
      sg_pipeline pipeline;
      sg_bindings bindings;
      sg_buffer vertex_data;
      sg_image render_target;
      sg_view render_target_view;
      F32* vbuf;
      Sz vbuf_size;
    } normal;
    struct {
      sg_pipeline pipeline;
      sg_bindings bindings;
      F32* vbuf;
      Sz vbuf_size;
    } bbox;
  } offscreen;
} g_state;

/* ===================================================== */
/*                      DECLERATIONS                     */
/* ===================================================== */

local Nothing app_md1_init(Nothing);
local AppMd1Error app_md1_init_style(struct nk_style* s);
local AppMd1Error app_md1_init_icons(Nothing);
local AppMd1Error app_md1_init_icon(AppMd1Image* app_icon,
                                    const U8* buffer,
                                    Sz size);
local Nothing app_md1_init_state();
local AppMd1Error app_md1_init_display_pipeline();
local AppMd1Error app_md1_init_offscreen_pipeline();

local Nothing app_md1_cleanup(Nothing);
local Nothing app_md1_cleanup_reload(Nothing);
local AppMd1Error app_md1_cleanup_icons(Nothing);
local AppMd1Error app_md1_cleanup_icon(AppMd1Image* app_icon);
local AppMd1Error app_md1_cleanup_3d(Nothing);

local Nothing app_md1_handle_user_input_events(const sapp_event* e);
local AppMd1Error app_md1_handle_drop_event(Str path);

local Nothing app_md1_frame(Nothing);
local Nothing app_md1_draw_mode_empty(struct nk_context* ctx,
                                      nk_flags window_flags,
                                      U32 window_width,
                                      U32 window_height);
local Nothing app_md1_draw_mode_failed(struct nk_context* ctx,
                                       nk_flags window_flags,
                                       U32 window_width,
                                       U32 window_height);
local Nothing app_md1_draw_mode_md1_loaded(U32 window_width, U32 window_height);
/* ===================================================== */
/*                       FUNCTIONS                       */
/* ===================================================== */

sapp_desc
// cppcheck-suppress unusedFunction
sokol_main(I32 argc, char* argv[]) {
  start_profiling();

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  zero_memory(&g_state, sizeof(g_state));

  if (sargs_exists("-i")) {
    g_state.input_path = S(sargs_value("-i"));
  } else if (sargs_exists("--input")) {
    g_state.input_path = S(sargs_value("--input"));
  }

  return (sapp_desc){
      .init_cb = app_md1_init,
      .frame_cb = app_md1_frame,
      .cleanup_cb = app_md1_cleanup,
      .event_cb = app_md1_handle_user_input_events,
      .enable_clipboard = TRUE,
      .width = APP_MD1_WINDOW_WIDTH,
      .height = APP_MD1_WINDOW_HEIGHT,
      .enable_dragndrop = TRUE,
      .max_dropped_files = 1,
      .window_title = "SQV :: MD1 Viewer",
      .icon.sokol_default = TRUE,
      .logger.func = slog_func,
      .high_dpi = TRUE,
  };

  end_profiling();
}

/* ===================================================== */

local Nothing
app_md1_init(Nothing) {
  start_profiling();

  app_md1_init_state();

  g_state.app = app_create(0, 0);

  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  snk_setup(&(snk_desc_t){
      .enable_set_mouse_cursor = true,
      .dpi_scale = sapp_dpi_scale(),
      .logger.func = slog_func,
  });

  app_md1_init_display_pipeline();

  if (ZS(g_state.input_path) != NULL) {
    log_info("loading '%s'", ZS(g_state.input_path));
    app_md1_handle_drop_event(g_state.input_path);
  }

  app_md1_init_icons();

  end_profiling();
}

/* ===================================================== */

local AppMd1Error
app_md1_init_style(struct nk_style* s) {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  struct nk_style_window* window = &s->window;

  window->padding.x = STYLE.window.padding.x;
  window->padding.y = STYLE.window.padding.y;
  window->scrollbar_size.x = STYLE.scrollbar.size.x;
  window->scrollbar_size.y = STYLE.scrollbar.size.y;
  window->group_padding.x = STYLE.explorer.padding.x;
  window->group_padding.y = STYLE.explorer.padding.y;

  window->border = STYLE.general.border.size;
  window->combo_border = STYLE.general.border.size;
  window->contextual_border = STYLE.general.border.size;
  window->menu_border = STYLE.general.border.size;
  window->group_border = STYLE.general.border.size;
  window->tooltip_border = STYLE.general.border.size;
  window->popup_border = STYLE.general.border.size;
  window->min_row_height_padding = STYLE.general.border.size;

  window->border_color.r = STYLE.general.border.color.r;
  window->border_color.g = STYLE.general.border.color.g;
  window->border_color.b = STYLE.general.border.color.b;
  window->border_color.a = 255;

  window->popup_border_color.r = STYLE.general.border.color.r;
  window->popup_border_color.g = STYLE.general.border.color.g;
  window->popup_border_color.b = STYLE.general.border.color.b;
  window->popup_border_color.a = 255;

  window->combo_border_color.r = STYLE.general.border.color.r;
  window->combo_border_color.g = STYLE.general.border.color.g;
  window->combo_border_color.b = STYLE.general.border.color.b;
  window->combo_border_color.a = 255;

  window->contextual_border_color.r = STYLE.general.border.color.r;
  window->contextual_border_color.g = STYLE.general.border.color.g;
  window->contextual_border_color.b = STYLE.general.border.color.b;
  window->contextual_border_color.a = 255;

  window->menu_border_color.r = STYLE.general.border.color.r;
  window->menu_border_color.g = STYLE.general.border.color.g;
  window->menu_border_color.b = STYLE.general.border.color.b;
  window->menu_border_color.a = 255;

  window->group_border_color.r = STYLE.general.border.color.r;
  window->group_border_color.g = STYLE.general.border.color.g;
  window->group_border_color.b = STYLE.general.border.color.b;
  window->group_border_color.a = 255;

  window->tooltip_border_color.r = STYLE.general.border.color.r;
  window->tooltip_border_color.g = STYLE.general.border.color.g;
  window->tooltip_border_color.b = STYLE.general.border.color.b;
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

  end_profiling();
  return err;
}

/* ===================================================== */

local AppMd1Error
app_md1_init_icons(Nothing) {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  // app_md1_init_icon(&g_icons.home, icon_home_png, sizeof(icon_home_png));
  // app_md1_init_icon(&g_icons.back, icon_back_png, sizeof(icon_back_png));
  // app_md1_init_icon(&g_icons.package, icon_package_png,
  //                   sizeof(icon_package_png));
  // app_md1_init_icon(&g_icons.extract, icon_extract_png,
  //                   sizeof(icon_extract_png));
  // app_md1_init_icon(&g_icons.folder, icon_folder_png,
  // sizeof(icon_folder_png)); app_md1_init_icon(&g_icons.file, icon_file_png,
  // sizeof(icon_file_png)); end_profiling();
  return err;
}

/* ===================================================== */

local AppMd1Error
// cppcheck-suppress unusedFunction
app_md1_init_icon(AppMd1Image* app_icon, const U8* buffer, Sz size) {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  I32 w, h, c = 0;
  const U8* data = stbi_load_from_memory(buffer, (I32)size, &w, &h, &c, 4);
  if (0 == data) {
    err = APP_MD1_ERR_ICON_INIT;
    str512_set(&g_state.error_text,
               (U8*)"failed to load icon image from memory");
    goto cleanup;
  }
  start_memory_profiling(data, size);

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
    end_memory_profiling(data);
  }

  end_profiling();
  return err;
}

/* ===================================================== */

local Nothing
app_md1_init_state() {
  start_profiling();

  g_state.mode = APP_MD1_MODE_EMPTY;
  g_state.zoom = 0;
  str512_reset(&g_state.error_text);

  // g_state.display.pass_action;

  // g_state.offscreen.normal.pipeline;
  // g_state.offscreen.normal.bindings;
  // g_state.offscreen.normal.data;
  // g_state.offscreen.normal.target;
  // g_state.offscreen.normal.view;
  // g_state.offscreen.normal.vbuf;
  // g_state.offscreen.normal.size;

  // g_state.offscreen.bbox.pipeline;
  // g_state.offscreen.bbox.bindings;
  // g_state.offscreen.bbox.vbuf;
  // g_state.offscreen.bbox.size;

  end_profiling();
}

/* ===================================================== */

local AppMd1Error
app_md1_init_display_pipeline() {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  // TODO:
  // do i need to set any value for this pass_action?
  g_state.display.pass_action = (sg_pass_action){};

  end_profiling();
  return err;
}

/* ===================================================== */

local AppMd1Error
app_md1_init_offscreen_pipeline() {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  // main 3d model pipeline
  sg_buffer_desc vertex_buffer_desc = {
      .size = g_state.offscreen.normal.vbuf_size,
      .usage.dynamic_update = true,
  };
  sg_buffer vertex_buffer = sg_make_buffer(&vertex_buffer_desc);
  sg_shader shader =
      sg_make_shader(default_md1_model_shader_desc(sg_query_backend()));

  g_state.offscreen.normal.bindings.views[VIEW_default_tex] =
      g_state.md1.skins[0].view;
  g_state.offscreen.normal.bindings.samplers[SMP_default_smp] =
      g_state.md1.skins[0].sampler;
  g_state.offscreen.normal.bindings.vertex_buffers[0] = vertex_buffer;

  g_state.offscreen.normal.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = shader,
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .cull_mode = SG_CULLMODE_NONE,
      .face_winding = SG_FACEWINDING_CCW,
      .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true},
      .layout = {.attrs = {[ATTR_default_md1_model_position] =
                               {.format = SG_VERTEXFORMAT_FLOAT3},
                           [ATTR_default_md1_model_texcoord0] = {
                               .format = SG_VERTEXFORMAT_FLOAT2}}}});

  // bbox pipeline
  sg_buffer_desc bbox_vertex_buffer_desc = {
      .size = sizeof(g_state.md1.gpu.bbox_vertex_buffer),
      .usage.dynamic_update = true,
  };
  sg_buffer bbox_vertex_buffer = sg_make_buffer(&bbox_vertex_buffer_desc);
  sg_shader bbox_shader =
      sg_make_shader(bbox_md1_bbox_shader_desc(sg_query_backend()));
  g_state.offscreen.bbox.bindings.vertex_buffers[0] = bbox_vertex_buffer;

  g_state.offscreen.bbox.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = bbox_shader,
      .primitive_type = SG_PRIMITIVETYPE_LINES,
      .cull_mode = SG_CULLMODE_NONE,
      .face_winding = SG_FACEWINDING_CCW,
      .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false},
      .layout = {.attrs = {[ATTR_bbox_md1_bbox_position] = {
                               .format = SG_VERTEXFORMAT_FLOAT3}}}});

  end_profiling();
  return err;
}

/* ===================================================== */

local Nothing
app_md1_cleanup(Nothing) {
  start_profiling();

  md1_unload(&g_state.md1);
  snk_shutdown();
  sg_shutdown();
  app_destroy(g_state.app);

  end_profiling();
}

/* ===================================================== */

local Nothing
app_md1_cleanup_reload(Nothing) {
  start_profiling();

  app_md1_cleanup_3d();
  io_close_file(&g_state.input_io_file);
  if (APP_MD1_MODE_LOADED == g_state.mode) {
    md1_unload(&g_state.md1);
  }

  arena_clear(context_arena());
  app_md1_init_state();

  end_profiling();
}

/* ===================================================== */

local AppMd1Error
// cppcheck-suppress unusedFunction
app_md1_cleanup_icons(Nothing) {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  // err = app_md1_cleanup_icon(&g_icons.home);
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }
  // err = app_md1_cleanup_icon(&g_icons.back);
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }
  // err = app_md1_cleanup_icon(&g_icons.package);
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }
  // err = app_md1_cleanup_icon(&g_icons.extract);
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }
  // err = app_md1_cleanup_icon(&g_icons.folder);
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }
  // err = app_md1_cleanup_icon(&g_icons.file);
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }

  end_profiling();
  return err;
}

/* ===================================================== */

local AppMd1Error
// cppcheck-suppress unusedFunction
app_md1_cleanup_icon(AppMd1Image* app_icon) {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  sg_destroy_view(app_icon->view);
  sg_destroy_sampler(app_icon->sampler);
  sg_destroy_image(app_icon->image);
  snk_destroy_image(app_icon->ui_image);

  end_profiling();
  return err;
}

/* ===================================================== */

// TODO:
// complete this
local AppMd1Error
app_md1_cleanup_3d(Nothing) {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  sg_destroy_buffer(g_state.offscreen.normal.vertex_data);
  sg_destroy_image(g_state.offscreen.normal.render_target);
  sg_destroy_view(g_state.offscreen.normal.render_target_view);
  // sg_destroy_sampler();
  // sg_destroy_shader();
  sg_destroy_pipeline(g_state.offscreen.normal.pipeline);

  end_profiling();
  return err;
}

/* ===================================================== */

local Nothing
app_md1_handle_user_input_events(const sapp_event* event) {
  start_profiling();

  // switch (e->type) {
  //   case SAPP_EVENTTYPE_KEY_UP:
  //     if (e->key_code == SAPP_KEYCODE_ESCAPE) {
  //       sapp_quit();
  //     }
  //     break;

  //   default:
  //     break;
  // }

  // if (e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
  //   if (e->scroll_y > 0) {
  //     g_state.zoom++;
  //   } else {
  //     g_state.zoom--;
  //   }
  // }

  if ((event->type == SAPP_EVENTTYPE_KEY_DOWN) && !event->key_repeat) {
    if (event->key_code == SAPP_KEYCODE_ESCAPE) {
      // sapp_request_quit();
    }
  }

  snk_handle_event(event);
  if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
    app_md1_handle_drop_event(S(sapp_get_dropped_file_path(0)));
  }

  end_profiling();
}

/* ===================================================== */

local AppMd1Error
app_md1_handle_drop_event(Str path) {
  start_profiling();

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  if (g_state.mode == APP_MD1_MODE_LOADED) {
    app_md1_cleanup_reload();
  }

  // NDBuffer ndb = {0};
  IOFile io_file = {0};
  IOError ioerr = io_load_file(context_arena(), path, &io_file);
  if (ioerr != IO_ERR_SUCCESS) {
    err = APP_MD1_ERR_FILE_OPEN;
    goto cleanup;
  }

  Md1Error mderr = md1_load(context_arena(), &g_state.md1, &io_file);
  if (MD1_ERR_SUCCESS != mderr) {
    err = APP_MD1_ERR_MODULE_MD1;
    goto cleanup;
  }

  md1_get_vertices(&g_state.md1, 0, 0, &g_state.offscreen.normal.vbuf,
                   &g_state.offscreen.normal.vbuf_size);

  app_md1_init_offscreen_pipeline();

  // render pass action

  g_state.mode = APP_MD1_MODE_LOADED;
  g_state.input_path = path;

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */

local Nothing
app_md1_frame(Nothing) {
  start_profiling();
  start_frame_profiling();

  // if (g_state.is_app_styled == FALSE) {
  //   app_md1_init_style(&ctx->style);
  //   g_state.is_app_styled = TRUE;
  // }

  // app_md1_draw_ui(ctx);

  // sg_begin_pass(&(sg_pass){
  //     .action = g_state.display.pass_action,
  //     .swapchain = sglue_swapchain(),
  // });
  // snk_render(sapp_width(), sapp_height());

  // sg_end_pass();

  // sg_commit();

  ///

  U32 window_width = sapp_width();
  U32 window_height = sapp_height();
  local nk_flags window_flags = NK_WINDOW_BORDER;

  if (g_state.mode == APP_MD1_MODE_EMPTY) {
    struct nk_context* ctx = snk_new_frame();
    app_md1_draw_mode_empty(ctx, window_flags, window_width, window_height);
  } else if (g_state.mode == APP_MD1_MODE_LOADED) {
    app_md1_draw_mode_md1_loaded(window_width, window_height);
  } else if (g_state.mode == APP_MD1_MODE_FAILED) {
    struct nk_context* ctx = snk_new_frame();
    app_md1_draw_mode_failed(ctx, window_flags, window_width, window_height);
  }

  ///

  end_frame_profiling();
  end_profiling();
}

/* ===================================================== */

local Nothing
app_md1_draw_mode_empty(struct nk_context* ctx,
                        nk_flags window_flags,
                        U32 window_width,
                        U32 window_height) {
  start_profiling();

  if (nk_begin(ctx, "", nk_rect(0, 0, window_width, window_height),
               window_flags)) {
    local char label_line_1[] = "drop a .MDL or .GLTF file for rendering";
    local char label_line_2[] =
        "note that you can convert .MDL to .GLTF and vice versa";

    struct nk_rect content_region = nk_window_get_content_region(ctx);
    const struct nk_user_font* font = ctx->style.font;

    F32 line_width = font->width(font->userdata, font->height, label_line_2,
                                 (int)strlen(label_line_2));

    F32 text_height = font->height;
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

  end_profiling();
}

/* ===================================================== */

local Nothing
app_md1_draw_mode_failed(struct nk_context* ctx,
                         nk_flags window_flags,
                         U32 window_width,
                         U32 window_height) {
  start_profiling();

  if (nk_begin(ctx, "", nk_rect(0, 0, window_width, window_height),
               window_flags)) {
    struct nk_rect content_region = nk_window_get_content_region(ctx);
    nk_layout_row_dynamic(ctx, content_region.h, 1);
    nk_label_wrap(ctx, ZS(g_state.error_text));
  }
  nk_end(ctx);

  end_profiling();
}

/* ===================================================== */

local Nothing
app_md1_draw_mode_md1_loaded(U32 window_width, U32 window_height) {
  start_profiling();

  const F32 FOV = 60.0f;

  Md1* md1 = &g_state.md1;
  hmm_v3* bbmin = &md1->bbox.min;
  hmm_v3* bbmax = &md1->bbox.max;
  hmm_vec3 center = HMM_MultiplyVec3f(HMM_AddVec3(*bbmin, *bbmax), 0.5f);
  F32 dx = bbmax->X - bbmin->X;
  F32 dy = bbmax->Y - bbmin->Y;
  F32 dz = bbmax->Z - bbmin->Z;
  F32 rad = 1.0f * sqrtf(dx * (dx * g_state.zoom) + dy * dy + dz * dz);

  F32 aspect = window_width / window_height;
  F32 dist = (rad / sinf(HMM_ToRadians(FOV) * 0.5f)) * 1.5f;

  hmm_vec3 eye = HMM_AddVec3(center, HMM_Vec3(0.0f, 0.0f, dist));
  hmm_vec3 up = HMM_Vec3(0.0f, 1.0f, 0.0f);

  hmm_mat4 proj = HMM_Perspective(FOV, aspect, 0.1f, dist * 4.0f);
  hmm_mat4 view = HMM_LookAt(eye, center, up);
  hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);

  hmm_mat4 rxm = HMM_Rotate(90, HMM_Vec3(1.0f, 0.0f, 0.0f));
  hmm_mat4 rym = HMM_Rotate(180, HMM_Vec3(0.0f, 1.0f, 0.0f));
  hmm_mat4 rzm = HMM_Rotate(g_state.rotation_y, HMM_Vec3(0.0f, 0.0f, -1.0f));
  hmm_mat4 rotation = HMM_MultiplyMat4(HMM_MultiplyMat4(rxm, rym), rzm);

  hmm_mat4 model = HMM_MultiplyMat4(
      HMM_Translate(center),
      HMM_MultiplyMat4(rotation,
                       HMM_Translate(HMM_MultiplyVec3f(center, -1.0f))));

  default_vs_params_t vs_params = {
      .mvp = HMM_MultiplyMat4(view_proj, model),
  };

  F32* vertex_buffer = NULL;
  Sz vertex_buffer_size = 0;

  md1_get_vertices(md1, g_state.pose, g_state.frame, &vertex_buffer,
                   &vertex_buffer_size);
  I32 elements_count = vertex_buffer_size / sizeof(F32) / 5;

  sg_update_buffer(
      g_state.offscreen.normal.bindings.vertex_buffers[0],
      &(sg_range){.ptr = vertex_buffer, .size = vertex_buffer_size});

  sg_begin_pass(&(sg_pass){.action = g_state.display.pass_action,
                           .swapchain = sglue_swapchain()});
  sg_apply_pipeline(g_state.offscreen.normal.pipeline);
  sg_apply_uniforms(UB_default_vs_params, &SG_RANGE(vs_params));
  sg_apply_bindings(&g_state.offscreen.normal.bindings);
  sg_draw(0, elements_count, 1);
  sg_end_pass();
  sg_commit();

  end_profiling();
}
