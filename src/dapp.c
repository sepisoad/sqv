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
#include "md1.h"

internal struct {
  Arena*         arena;
  MD1            md1;
  sg_pipeline    pipeline;
  sg_bindings    bindings;
  sg_pass_action pass_action;
  F32*           vbuf;
  Sz             vbuf_size;
  U32*           ibuf;
  Sz             ibuf_size;
  U32            zoom;
} S;

internal Arena* arena = {0};
internal MD1    md1 = {0};

internal Nothing
init_v1(void) {
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

  md1_get_vertices(&S.md1, 0, 0, &S.vbuf, &S.vbuf_size);

  // render pass action
  S.pass_action = (sg_pass_action){
      .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.125f, 0.25f, 0.35f, 1.0f}},
  };

  // bindings
  S.bindings.views[VIEW_tex] = S.md1.skins[0].view;
  S.bindings.samplers[SMP_smp] = S.md1.skins[0].sampler;

  S.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .data =
          {
              .ptr = S.vbuf,
              .size = (Sz)S.vbuf_size * sizeof(F32),
          },
      .label = "vertex-buffer",
  });

  // build shader
  sg_shader shader = sg_make_shader(cube_shader_desc(sg_query_backend()));
  S.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = shader,
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .cull_mode = SG_CULLMODE_NONE,
      .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true},
      .layout =
          {
              .attrs =
                  {
                      [ATTR_cube_position] = {.format = SG_VERTEXFORMAT_FLOAT3},
                      [ATTR_cube_texcoord0] = {.format =
                                                   SG_VERTEXFORMAT_FLOAT2},
                  },
          },
  });
}

internal Nothing
init_v2(void) {
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

  md1_get_vertices_v2(&S.md1, 0, 0, &S.vbuf, &S.vbuf_size, &S.ibuf,
                      &S.ibuf_size);

  // render pass action
  S.pass_action = (sg_pass_action){
      .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.125f, 0.25f, 0.35f, 1.0f}},
  };

  // bindings
  S.bindings.views[VIEW_tex] = S.md1.skins[0].view;
  S.bindings.samplers[SMP_smp] = S.md1.skins[0].sampler;

  S.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .data =
          {
              .ptr = S.vbuf,
              .size = (Sz)S.vbuf_size * sizeof(F32),
          },
      .label = "vertex-buffer",
  });

  S.bindings.index_buffer = sg_make_buffer(&(sg_buffer_desc){
      .usage.index_buffer = true,
      .data =
          {
              .ptr = S.ibuf,
              .size = (Sz)S.ibuf_size,
          },
      .label = "index-buffer",
  });

  // build shader
  sg_shader shader = sg_make_shader(cube_shader_desc(sg_query_backend()));
  S.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = shader,
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .index_type = SG_INDEXTYPE_UINT32,
      .cull_mode = SG_CULLMODE_NONE,
      .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true},
      .layout =
          {
              .attrs =
                  {
                      [ATTR_cube_position] = {.format = SG_VERTEXFORMAT_FLOAT3},
                      [ATTR_cube_texcoord0] = {.format =
                                                   SG_VERTEXFORMAT_FLOAT2},
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
frame_v1(void) {
  MD1*     m = &S.md1;

  float    aspect = sapp_widthf() / sapp_heightf();
  float    dist = 100.0f;  // distance from camera; tune if model too small
  hmm_vec3 center = HMM_Vec3(0.0f, 0.0f, 0.0f);

  // projection and view matrices
  hmm_mat4 proj = HMM_Perspective(60.0f, aspect, 0.1f, 1000.0f);
  hmm_mat4 view = HMM_LookAt(HMM_Vec3(0, 0, dist), center, HMM_Vec3(0, 1, 0));
  hmm_mat4 rot_x = HMM_Rotate(-90.0f, HMM_Vec3(1.0f, 0.0f, 0.0f));
  hmm_mat4 rot_z = HMM_Rotate(-90.0f, HMM_Vec3(0.0f, 0.0f, 1.0f));
  hmm_mat4 model =
      HMM_MultiplyMat4(HMM_MultiplyMat4(rot_x, rot_z), HMM_Mat4d(1.0f));
  // hmm_mat4 model = HMM_Mat4d(1.0f);

  // combine: mvp = proj * view * model
  hmm_mat4    mvp = HMM_MultiplyMat4(proj, HMM_MultiplyMat4(view, model));

  vs_params_t vs_params = {.mvp = mvp};

  sg_begin_pass(
      &(sg_pass){.action = S.pass_action, .swapchain = sglue_swapchain()});
  sg_apply_pipeline(S.pipeline);
  sg_apply_bindings(&S.bindings);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, S.vbuf_size / 5, 1);
  sg_end_pass();
  sg_commit();
}

static Nothing
frame_v2(void) {
  MD1*     m = &S.md1;

  float    aspect = sapp_widthf() / sapp_heightf();
  float    dist = 100.0f;  // distance from camera; tune if model too small
  hmm_vec3 center = HMM_Vec3(0.0f, 0.0f, 0.0f);

  // projection and view matrices
  hmm_mat4 proj = HMM_Perspective(60.0f, aspect, 0.1f, 1000.0f);
  hmm_mat4 view = HMM_LookAt(HMM_Vec3(0, 0, dist), center, HMM_Vec3(0, 1, 0));
  hmm_mat4 rot_x = HMM_Rotate(-90.0f, HMM_Vec3(1.0f, 0.0f, 0.0f));
  hmm_mat4 rot_z = HMM_Rotate(-90.0f, HMM_Vec3(0.0f, 0.0f, 1.0f));
  hmm_mat4 model =
      HMM_MultiplyMat4(HMM_MultiplyMat4(rot_x, rot_z), HMM_Mat4d(1.0f));
  // hmm_mat4 model = HMM_Mat4d(1.0f);

  // combine: mvp = proj * view * model
  hmm_mat4    mvp = HMM_MultiplyMat4(proj, HMM_MultiplyMat4(view, model));

  vs_params_t vs_params = {.mvp = mvp};

  sg_begin_pass(
      &(sg_pass){.action = S.pass_action, .swapchain = sglue_swapchain()});
  sg_apply_pipeline(S.pipeline);
  sg_apply_bindings(&S.bindings);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, S.vbuf_size / 5, 1);
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

  CStr inpath = "/home/sepi/Projects/sepi/sqv/.keep/pak0/progs/ogre.mdl";

  return (sapp_desc){
      .init_cb = init_v2,
      .cleanup_cb = cleanup,
      .event_cb = input,
      .frame_cb = frame_v2,
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
