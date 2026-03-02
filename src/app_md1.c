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
#include <deps/sepi/io.h>

#include "shaders/default.glsl.h"
#include "shaders/bbox.glsl.h"
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
  // APP_MD1_ERR_DIR_OPEN,
  APP_MD1_ERR_ICON_INIT,
  // APP_MD1_ERR_MODULE_MD1,
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

DefineFStr8(APP_MD1_MAX_ERROR_LENGTH);

static struct {
  Arena* arena;
  AppMd1Mode mode;
  Bool is_app_styled;
  Md1 md1;
  U32 zoom;
  Str8 input_path;
  IOFile input_io_file;
  FL512_Str8 error_text;
  sg_pass_action pass_action;
  struct {
    sg_pipeline pipeline;
    sg_bindings bindings;
    sg_pass_action pass_action;
    F32* vbuf;
    Sz vbuf_size;
  } model;
  struct {
    sg_pipeline pipeline;
    sg_bindings bindings;
    F32* vbuf;
    Sz vbuf_size;
  } bbox;
} g_state;

/* ===================================================== */
/*                      DECLERATIONS                     */
/* ===================================================== */

static Nothing app_md1_init(void);
static AppMd1Error app_md1_init_style(struct nk_style* s);
static AppMd1Error app_md1_init_icons();
static AppMd1Error app_md1_init_icon(AppMd1Image* app_icon,
                                     CBuf buffer,
                                     Sz size);

static Nothing app_md1_cleanup(void);
static Nothing app_md1_cleanup_reload();
static AppMd1Error app_md1_cleanup_icons();
static AppMd1Error app_md1_cleanup_icon(AppMd1Image* app_icon);

static Nothing app_md1_handle_user_input_events(const sapp_event* e);
static AppMd1Error app_md1_handle_drop_event(Str8 path);

static Nothing app_md1_frame(void);
static U32 app_md1_draw(struct nk_context* ctx);

/* ===================================================== */
/*                       FUNCTIONS                       */
/* ===================================================== */

sapp_desc
sokol_main(I32 argc, char* argv[]) {
  start_profiling(1);

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
      .icon.sokol_default = TRUE,
      .logger.func = slog_func,
      .high_dpi = TRUE,
  };

  end_profiling();
}

/* ===================================================== */

static Nothing
app_md1_init(void) {
  start_profiling(1);

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

  g_state.mode = APP_MD1_MODE_EMPTY;

  // // load default Md1 file
  // Str8 path = S(sapp_userdata());

  // // NDBuffer ndb = {0};
  // IOFile io_file = {0};
  // IOError ioerr = io_load_file(g_state.arena, path, &io_file);
  // if (ioerr != IO_ERR_SUCCESS) {
  //   // NOTE: this is a playground!
  // }

  // md1_load(&g_state.md1, &io_file);
  // md1_get_vertices(&g_state.md1, 0, 0, &g_state.model.vbuf,
  //                  &g_state.model.vbuf_size);

  // // MODEL

  // // render pass action
  // g_state.pass_action = (sg_pass_action){
  //     .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
  //                   .clear_value = {0.125f, 0.25f, 0.35f, 1.0f}},
  // };

  // // bindings
  // g_state.model.bindings.views[VIEW_default_tex] = g_state.md1.skins[0].view;
  // g_state.model.bindings.samplers[SMP_default_smp] =
  //     g_state.md1.skins[0].sampler;

  // g_state.model.bindings.vertex_buffers[0] =
  // sg_make_buffer(&(sg_buffer_desc){
  //     .data =
  //         {
  //             .ptr = g_state.model.vbuf,
  //             .size = g_state.model.vbuf_size,
  //         },
  //     .label = "vertex-buffer-model",
  // });

  // // build shader
  // g_state.model.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
  //     .shader =
  //         sg_make_shader(default_md1_model_shader_desc(sg_query_backend())),
  //     .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
  //     .cull_mode = SG_CULLMODE_NONE,
  //     .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true},
  //     .layout =
  //         {
  //             .attrs =
  //                 {
  //                     [ATTR_default_md1_model_position] =
  //                         {.format = SG_VERTEXFORMAT_FLOAT3},
  //                     [ATTR_default_md1_model_texcoord0] =
  //                         {.format = SG_VERTEXFORMAT_FLOAT2},
  //                 },
  //         },
  // });

  // // BBOX

  // g_state.bbox.vbuf = (F32*)g_state.md1.gpu.bbox_vertex_buffer;
  // g_state.bbox.vbuf_size = sizeof(g_state.md1.gpu.bbox_vertex_buffer);

  // g_state.bbox.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
  //     .data =
  //         {
  //             .ptr = g_state.bbox.vbuf,
  //             .size = g_state.bbox.vbuf_size,
  //         },
  //     .label = "vertex-buffer-bbox",
  // });

  // // build shader
  // g_state.bbox.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
  //     .shader =
  //     sg_make_shader(bbox_md1_bbox_shader_desc(sg_query_backend())),
  //     .primitive_type = SG_PRIMITIVETYPE_LINES,
  //     .cull_mode = SG_CULLMODE_NONE,
  //     .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled =
  //     false}, .layout =
  //         {
  //             .attrs =
  //                 {
  //                     [ATTR_bbox_md1_bbox_position] =
  //                         {.format = SG_VERTEXFORMAT_FLOAT3},
  //                 },
  //         },
  // });

  end_profiling();
}

/* ===================================================== */

static AppMd1Error
app_md1_init_style(struct nk_style* s) {
  start_profiling(1);

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

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
  end_profiling();
  return err;
}

/* ===================================================== */

static AppMd1Error
app_md1_init_icons() {
  start_profiling(1);

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  // err = app_md1_init_icon(&g_icons.home, icon_home_png,
  // sizeof(icon_home_png)); if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }

  // err = app_md1_init_icon(&g_icons.back, icon_back_png,
  // sizeof(icon_back_png)); if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }

  // err = app_md1_init_icon(&g_icons.package, icon_package_png,
  //                         sizeof(icon_package_png));
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }

  // err = app_md1_init_icon(&g_icons.extract, icon_extract_png,
  //                         sizeof(icon_extract_png));
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }

  // err = app_md1_init_icon(&g_icons.folder, icon_folder_png,
  //                         sizeof(icon_folder_png));
  // if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }

  // err = app_md1_init_icon(&g_icons.file, icon_file_png,
  // sizeof(icon_file_png)); if (err != APP_MD1_ERR_SUCCESS) {
  //   goto cleanup;
  // }

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */

static AppMd1Error
app_md1_init_icon(AppMd1Image* app_icon, CBuf buffer, Sz size) {
  start_profiling(1);

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  I32 w, h, c = 0;
  CBuf data = stbi_load_from_memory(buffer, size, &w, &h, &c, 4);
  if (0 == data) {
    err = APP_MD1_ERR_ICON_INIT;
    fl512_str8_set(&g_state.error_text,
                   "failed to load icon image from memory");
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

static Nothing
app_md1_cleanup(void) {
  start_profiling(1);

  md1_unload(&g_state.md1);
  snk_shutdown();
  sg_shutdown();
  if (APP_MD1_MODE_LOADED == g_state.mode) {
    // TODO:
    // handle this shit!
    // pak_unload(&g_state.pak);
  }
  arena_destroy(g_state.arena);

  end_profiling();
}

/* ===================================================== */

static Nothing
app_md1_cleanup_reload() {
  start_profiling(1);

  AppMd1Mode old_mode = g_state.mode;
  g_state.mode = APP_MD1_MODE_EMPTY;
  // g_state.is_extracting_requested = FALSE;
  // g_state.is_packaging_requested = FALSE;
  // g_state.requested_extracting_md1_item = 0;

  fl512_str8_reset(&g_state.error_text);

  io_close_file(&g_state.input_io_file);
  if (APP_MD1_MODE_LOADED == old_mode) {
    md1_unload(&g_state.md1);
  }
  arena_clear(g_state.arena);

  end_profiling();
}

/* ===================================================== */

static AppMd1Error
app_md1_cleanup_icons() {
  start_profiling(1);

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

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */

static AppMd1Error
app_md1_cleanup_icon(AppMd1Image* app_icon) {
  start_profiling(1);

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  sg_destroy_view(app_icon->view);
  sg_destroy_sampler(app_icon->sampler);
  sg_destroy_image(app_icon->image);
  snk_destroy_image(app_icon->ui_image);

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */

static Nothing
app_md1_handle_user_input_events(const sapp_event* event) {
  start_profiling(1);

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

static AppMd1Error
app_md1_handle_drop_event(Str8 path) {
  start_profiling(1);

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  Bool is_directory = FALSE;
  IOError ioerr = io_is_path_a_directory(path, &is_directory);
  if (IO_ERR_SUCCESS != ioerr) {
    g_state.mode = APP_MD1_MODE_FAILED;
    err = APP_MD1_ERR_DROP;
    goto cleanup;
  }

  if (g_state.mode == APP_MD1_MODE_LOADED) {
    app_md1_cleanup_reload();
  }

  ioerr = io_open_file(g_state.arena, path, &g_state.input_io_file);
  if (IO_ERR_SUCCESS != ioerr) {
    char err_text[APP_MD1_MAX_ERROR_LENGTH] = {0};
    snprintf(err_text, APP_MD1_MAX_ERROR_LENGTH, "failed to open '%s'",
             CS(path));
    fl512_str8_set(&g_state.error_text, err_text);
    err = APP_MD1_ERR_FILE_OPEN;
    g_state.mode = APP_MD1_MODE_FAILED;
    goto cleanup;
  }

  // TODO:
  // fix this (copied from pak)
  // PakError perr = pak_load_from_io_file(&g_state.pak,
  // &g_state.input_io_file); if (perr != PAK_ERR_SUCCESS) {
  //   char err_text[APP_MD1_MAX_ERROR_LENGTH] = {0};
  //   snprintf(err_text, APP_MD1_MAX_ERROR_LENGTH, "failed to load '%s' items",
  //            CS(path));
  //   fl512_str8_set(&g_state.error_text, err_text);
  //   err = APP_MD1_ERR_MODULE_PAK;
  //   g_state.mode = APP_MD1_MODE_FAILED;
  //   goto cleanup;
  // }


  g_state.mode = APP_MD1_MODE_LOADED;
  g_state.input_path = path;

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */

static Nothing
app_md1_frame(void) {
  start_profiling(1);
  start_frame_profiling();

  // Md1* m = &g_state.md1;

  // F32 field_of_view = 60.0;
  // F32 view_aspect_ratio = sapp_widthf() / sapp_heightf();
  // F32 camera_distance = m->bbox.radius * 3;
  // hmm_vec3 view_center = HMM_Vec3(0.0f, 0.0f, 0.0f);
  // hmm_vec3 camera_position = HMM_Vec3(m->bbox.center.X, m->bbox.center.Y,
  //                                     m->bbox.center.Z + camera_distance);
  // hmm_mat4 proj = HMM_Perspective(field_of_view, view_aspect_ratio,
  //                                 m->bbox.radius / 100, m->bbox.radius *
  //                                 100);
  // hmm_mat4 view = HMM_LookAt(camera_position, view_center, HMM_Vec3(0, 1,
  // 0)); hmm_mat4 center = HMM_Translate(HMM_MultiplyVec3f(m->bbox.center,
  // -1.0f)); hmm_mat4 rot_x = HMM_Rotate(-90.0f, HMM_Vec3(1.0f, 0.0f, 0.0f));
  // hmm_mat4 rot_z = HMM_Rotate(-90.0f, HMM_Vec3(0.0f, 0.0f, 1.0f));
  // hmm_mat4 rot = HMM_MultiplyMat4(rot_x, rot_z);
  // hmm_mat4 model = HMM_MultiplyMat4(rot, center);
  // hmm_mat4 mvp = HMM_MultiplyMat4(proj, HMM_MultiplyMat4(view, model));

  // default_vs_params_t vs_params = {.mvp = mvp};

  // sg_begin_pass(&(sg_pass){.action = g_state.pass_action,
  //                          .swapchain = sglue_swapchain()});

  // sg_apply_pipeline(g_state.model.pipeline);
  // sg_apply_bindings(&g_state.model.bindings);
  // sg_apply_uniforms(UB_default_vs_params, &SG_RANGE(vs_params));
  // sg_draw(0, g_state.model.vbuf_size / 5, 1);

  // sg_apply_pipeline(g_state.bbox.pipeline);
  // sg_apply_bindings(&g_state.bbox.bindings);
  // sg_apply_uniforms(UB_default_vs_params, &SG_RANGE(vs_params));
  // sg_draw(0, MD1_BBOX_VERTEX_COUNT, 1);

  // sg_end_pass();
  // sg_commit();

  //----
  struct nk_context* ctx = snk_new_frame();

  if (g_state.is_app_styled == FALSE) {
    app_md1_init_style(&ctx->style);
    g_state.is_app_styled = TRUE;
  }

  app_md1_draw(ctx);
  sg_begin_pass(
      &(sg_pass){.action =
                     {
                         .colors[0] = {.load_action = SG_LOADACTION_CLEAR},
                     },
                 .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());

  sg_end_pass();
  sg_commit();
  //----

  end_frame_profiling();
  end_profiling();
}

/* ===================================================== */

static U32
app_md1_draw(struct nk_context* ctx) {
  start_profiling(1);

  AppMd1Error err = APP_MD1_ERR_SUCCESS;

  static char window_title[] = "SQV::Md1 Viewer";
  static nk_flags window_flags = NK_WINDOW_BORDER;

  U32 window_width = sapp_width();
  U32 window_height = sapp_height();

  nk_style_hide_cursor(ctx);

  // if (g_state.mode == APP_PAK_MODE_EMPTY) {
  //   app_md1_draw_mode_empty(ctx, window_flags, window_width, window_height);
  // } else if (g_state.mode == APP_PAK_MODE_PAK_LOADED) {
  //   app_md1_draw_mode_md1_loaded(ctx, window_flags, window_width,
  //                                window_height);
  // } else if (g_state.mode == APP_PAK_MODE_DIR_LOADED) {
  //   app_md1_draw_mode_dir_loaded(ctx, window_flags, window_width,
  //                                window_height);
  // } else if (g_state.mode == APP_PAK_MODE_FAILED) {
  //   app_md1_draw_mode_failed(ctx, window_flags, window_width, window_height);
  // }

  Bool is_window_closed = !nk_window_is_closed(ctx, window_title);

  end_profiling();
  return is_window_closed;
}


/* ===================================================== */
