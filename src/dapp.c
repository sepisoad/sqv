/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#define MD1_IMPLEMENTATION

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
#include "shaders/bbox.glsl.h"
#include "md1.h"

internal struct {
  Arena*         arena;
  MD1            md1;
  U32            zoom;
  sg_pass_action pass_action;
  struct {
    sg_pipeline    pipeline;
    sg_bindings    bindings;
    sg_pass_action pass_action;
    F32*           vbuf;
    Sz             vbuf_size;
  } model;
  struct {
    sg_pipeline pipeline;
    sg_bindings bindings;
    F32*        vbuf;
    Sz          vbuf_size;
  } bbox;
} S;

internal Arena* arena = {0};
internal MD1    md1 = {0};

internal Nothing
init(void) {
  log_info("initializing gpu ...");

  // init sokol
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  // init arena allocator
  S.arena = arena_create();

  // load default MD1 file
  CStr path = (CStr)sapp_userdata();
  Buf  buf = 0;
  Sz   bufsz = io_load_file(S.arena, path, &buf);
  md1_load((CBuf)buf, bufsz, &S.md1);

  md1_get_vertices(&S.md1, 0, 0, &S.model.vbuf, &S.model.vbuf_size);

  // MODEL

  // render pass action
  S.pass_action = (sg_pass_action){
      .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.125f, 0.25f, 0.35f, 1.0f}},
  };

  // bindings
  S.model.bindings.views[VIEW_default_tex] = S.md1.skins[0].view;
  S.model.bindings.samplers[SMP_default_smp] = S.md1.skins[0].sampler;

  S.model.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .data =
          {
              .ptr = S.model.vbuf,
              .size = S.model.vbuf_size,
          },
      .label = "vertex-buffer-model",
  });

  // build shader
  S.model.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
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

  S.bbox.vbuf = (F32*)S.md1.gpu.bbox_vertex_buffer;
  S.bbox.vbuf_size = sizeof(S.md1.gpu.bbox_vertex_buffer);

  S.bbox.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .data =
          {
              .ptr = S.bbox.vbuf,
              .size = S.bbox.vbuf_size,
          },
      .label = "vertex-buffer-bbox",
  });

  // build shader
  S.bbox.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
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
}

internal Nothing
cleanup(void) {
  log_info("shutting down");

  md1_unload(&md1);
  arena_destroy(S.arena);
  sg_shutdown();

  printf("FUCKING QUITING...\n");
}

static Nothing
input(const sapp_event* e) {
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
      S.zoom++;
    } else {
      S.zoom--;
    }
    Dbg("%d\n", S.zoom);
  }
}

static Nothing
frame(void) {
  MD1*     m = &S.md1;

  F32      field_of_view = 60.0;
  F32      view_aspect_ratio = sapp_widthf() / sapp_heightf();
  F32      camera_distance = m->bbox.radius * 3;
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
  hmm_mat4 model =
      HMM_MultiplyMat4(rot, center);
  hmm_mat4 mvp = HMM_MultiplyMat4(proj, HMM_MultiplyMat4(view, model));

  default_vs_params_t vs_params = {.mvp = mvp};

  sg_begin_pass(
      &(sg_pass){.action = S.pass_action, .swapchain = sglue_swapchain()});

  sg_apply_pipeline(S.model.pipeline);
  sg_apply_bindings(&S.model.bindings);
  sg_apply_uniforms(UB_default_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, S.model.vbuf_size / 5, 1);

  sg_apply_pipeline(S.bbox.pipeline);
  sg_apply_bindings(&S.bbox.bindings);
  sg_apply_uniforms(UB_default_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, MD1_BBOX_VERTEX_COUNT, 1);

  sg_end_pass();
  sg_commit();
}

sapp_desc
sokol_main(I32 argc, char* argv[]) {
  log_info("starting");

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  // CStr inpath = "/home/sepi/Projects/sepi/sqv/.keep/pak0/progs/spike.mdl";
  // CStr inpath = "/home/sepi/Projects/sepi/sqv/.keep/pak0/progs/shambler.mdl";
  // CStr inpath = "/home/sepi/Projects/sepi/sqv/.keep/pak0/progs/boss.mdl";
  CStr inpath = "/home/sepi/Games/pc/quake1/dwellv1p2/progs/boss_egypt.mdl";
  // CStr inpath = "/home/sepi/Games/pc/quake1/MALICE/progs/rat.mdl";

  return (sapp_desc){
      .init_cb = init,
      .cleanup_cb = cleanup,
      .event_cb = input,
      .frame_cb = frame,
      .user_data = (RawPtr)inpath,
      .width = 800,
      .height = 600,
      .sample_count = 1,
      .window_title = "playground",
      .icon.sokol_default = TRUE,
      .enable_dragndrop = TRUE,
      .max_dropped_files = 1,
      .logger.func = slog_func,
  };
}
