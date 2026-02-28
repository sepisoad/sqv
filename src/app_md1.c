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
#include "module_md1.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define APP_MD1_WINDOW_WIDTH 640
#define APP_MD1_WINDOW_HEIGHT 480

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

/* ===================================================== */
/*                        GLOBALS                        */
/* ===================================================== */

mount_master_profiling_context();

static struct {
  Arena* arena;
  Md1 md1;
  U32 zoom;
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

static Nothing init(void);
static Nothing cleanup(void);
static Nothing input(const sapp_event* e);
static Nothing frame(void);

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

  // TODO:
  // WTF!
  CStr inpath = ".keep/knight.mdl";

  return (sapp_desc){
      .init_cb = init,
      .cleanup_cb = cleanup,
      .event_cb = input,
      .frame_cb = frame,
      .user_data = (RawPtr)inpath,
      .width = APP_MD1_WINDOW_WIDTH,
      .height = APP_MD1_WINDOW_HEIGHT,
      .sample_count = 1,
      .window_title = "SQV :: MD1 Viewer",
      .icon.sokol_default = TRUE,
      .enable_dragndrop = TRUE,
      .max_dropped_files = 1,
      .logger.func = slog_func,
  };

  end_profiling();
}

static Nothing
init(void) {
  start_profiling(1);

  // init arena allocator
  g_state.arena = arena_create();

  // load default Md1 file
  Str8 path = S(sapp_userdata());

  // NDBuffer ndb = {0};
  IOFile io_file = {0};
  IOError ioerr = io_load_file(g_state.arena, path, &io_file);
  if (ioerr != IO_ERR_SUCCESS) {
    // NOTE: this is a playground!
  }

  md1_load(&g_state.md1, &io_file);
  md1_get_vertices(&g_state.md1, 0, 0, &g_state.model.vbuf,
                   &g_state.model.vbuf_size);

  // MODEL

  // render pass action
  g_state.pass_action = (sg_pass_action){
      .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.125f, 0.25f, 0.35f, 1.0f}},
  };

  // bindings
  g_state.model.bindings.views[VIEW_default_tex] = g_state.md1.skins[0].view;
  g_state.model.bindings.samplers[SMP_default_smp] =
      g_state.md1.skins[0].sampler;

  g_state.model.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .data =
          {
              .ptr = g_state.model.vbuf,
              .size = g_state.model.vbuf_size,
          },
      .label = "vertex-buffer-model",
  });

  // build shader
  g_state.model.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
      .shader =
          sg_make_shader(default_md1_model_shader_desc(sg_query_backend())),
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .cull_mode = SG_CULLMODE_NONE,
      .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true},
      .layout =
          {
              .attrs =
                  {
                      [ATTR_default_md1_model_position] =
                          {.format = SG_VERTEXFORMAT_FLOAT3},
                      [ATTR_default_md1_model_texcoord0] =
                          {.format = SG_VERTEXFORMAT_FLOAT2},
                  },
          },
  });

  // BBOX

  g_state.bbox.vbuf = (F32*)g_state.md1.gpu.bbox_vertex_buffer;
  g_state.bbox.vbuf_size = sizeof(g_state.md1.gpu.bbox_vertex_buffer);

  g_state.bbox.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .data =
          {
              .ptr = g_state.bbox.vbuf,
              .size = g_state.bbox.vbuf_size,
          },
      .label = "vertex-buffer-bbox",
  });

  // build shader
  g_state.bbox.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = sg_make_shader(bbox_md1_bbox_shader_desc(sg_query_backend())),
      .primitive_type = SG_PRIMITIVETYPE_LINES,
      .cull_mode = SG_CULLMODE_NONE,
      .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false},
      .layout =
          {
              .attrs =
                  {
                      [ATTR_bbox_md1_bbox_position] =
                          {.format = SG_VERTEXFORMAT_FLOAT3},
                  },
          },
  });

  end_profiling();
}

static Nothing
cleanup(void) {
  start_profiling(1);

  md1_unload(&g_state.md1);
  arena_destroy(g_state.arena);
  sg_shutdown();

  end_profiling();
}

static Nothing
input(const sapp_event* e) {
  start_profiling(1);

  switch (e->type) {
    case SAPP_EVENTTYPE_KEY_UP:
      if (e->key_code == SAPP_KEYCODE_ESCAPE) {
        sapp_quit();
      }
      break;

    default:
      break;
  }

  if (e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
    if (e->scroll_y > 0) {
      g_state.zoom++;
    } else {
      g_state.zoom--;
    }
  }

  end_profiling();
}

static Nothing
frame(void) {
  start_profiling(1);

  Md1* m = &g_state.md1;

  F32 field_of_view = 60.0;
  F32 view_aspect_ratio = sapp_widthf() / sapp_heightf();
  F32 camera_distance = m->bbox.radius * 3;
  hmm_vec3 view_center = HMM_Vec3(0.0f, 0.0f, 0.0f);
  hmm_vec3 camera_position = HMM_Vec3(m->bbox.center.X, m->bbox.center.Y,
                                      m->bbox.center.Z + camera_distance);
  hmm_mat4 proj = HMM_Perspective(field_of_view, view_aspect_ratio,
                                  m->bbox.radius / 100, m->bbox.radius * 100);
  hmm_mat4 view = HMM_LookAt(camera_position, view_center, HMM_Vec3(0, 1, 0));
  hmm_mat4 center = HMM_Translate(HMM_MultiplyVec3f(m->bbox.center, -1.0f));
  hmm_mat4 rot_x = HMM_Rotate(-90.0f, HMM_Vec3(1.0f, 0.0f, 0.0f));
  hmm_mat4 rot_z = HMM_Rotate(-90.0f, HMM_Vec3(0.0f, 0.0f, 1.0f));
  hmm_mat4 rot = HMM_MultiplyMat4(rot_x, rot_z);
  hmm_mat4 model = HMM_MultiplyMat4(rot, center);
  hmm_mat4 mvp = HMM_MultiplyMat4(proj, HMM_MultiplyMat4(view, model));

  default_vs_params_t vs_params = {.mvp = mvp};

  sg_begin_pass(&(sg_pass){.action = g_state.pass_action,
                           .swapchain = sglue_swapchain()});

  sg_apply_pipeline(g_state.model.pipeline);
  sg_apply_bindings(&g_state.model.bindings);
  sg_apply_uniforms(UB_default_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, g_state.model.vbuf_size / 5, 1);

  sg_apply_pipeline(g_state.bbox.pipeline);
  sg_apply_bindings(&g_state.bbox.bindings);
  sg_apply_uniforms(UB_default_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, MD1_BBOX_VERTEX_COUNT, 1);

  sg_end_pass();
  sg_commit();

  end_profiling();
}
