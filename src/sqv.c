#include <assert.h>
#include <stdbool.h>

#include "../deps/log.h"
#include "../deps/hmm.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"

#include "glsl_default.h"
#include "qk_mdl.h"
#include "sqv_err.h"

sqv_err qk_init(void);
sqv_err qk_deinit(qk_mdl* mdl);
sqv_err qk_load_mdl(const char* path, qk_mdl* mdl);

static struct {
  float       rx, ry;
  sg_pipeline pip;
  sg_bindings bind;
  qk_mdl      mdl;
} state;

void init(void) {
  log_info("initializing gpu ...");

  sg_setup(&(sg_desc) {
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  sqv_err err = qk_init();
  assert(err == SQV_SUCCESS);

  // err = qk_load_mdl(".keep/boss.mdl", &state.mdl);
  // assert(err == SQV_SUCCESS);

  /* cube vertex buffer */
  float vertices[] = {
    -1.0, -1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0,  -1.0, -1.0, 1.0, 0.0, 0.0, 1.0,
    1.0,  1.0,  -1.0, 1.0, 0.0, 0.0, 1.0, -1.0, 1.0,  -1.0, 1.0, 0.0, 0.0, 1.0,

    -1.0, -1.0, 1.0,  0.0, 1.0, 0.0, 1.0, 1.0,  -1.0, 1.0,  0.0, 1.0, 0.0, 1.0,
    1.0,  1.0,  1.0,  0.0, 1.0, 0.0, 1.0, -1.0, 1.0,  1.0,  0.0, 1.0, 0.0, 1.0,

    -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0, -1.0, 1.0,  -1.0, 0.0, 0.0, 1.0, 1.0,
    -1.0, 1.0,  1.0,  0.0, 0.0, 1.0, 1.0, -1.0, -1.0, 1.0,  0.0, 0.0, 1.0, 1.0,

    1.0,  -1.0, -1.0, 1.0, 0.5, 0.0, 1.0, 1.0,  1.0,  -1.0, 1.0, 0.5, 0.0, 1.0,
    1.0,  1.0,  1.0,  1.0, 0.5, 0.0, 1.0, 1.0,  -1.0, 1.0,  1.0, 0.5, 0.0, 1.0,

    -1.0, -1.0, -1.0, 0.0, 0.5, 1.0, 1.0, -1.0, -1.0, 1.0,  0.0, 0.5, 1.0, 1.0,
    1.0,  -1.0, 1.0,  0.0, 0.5, 1.0, 1.0, 1.0,  -1.0, -1.0, 0.0, 0.5, 1.0, 1.0,

    -1.0, 1.0,  -1.0, 1.0, 0.0, 0.5, 1.0, -1.0, 1.0,  1.0,  1.0, 0.0, 0.5, 1.0,
    1.0,  1.0,  1.0,  1.0, 0.0, 0.5, 1.0, 1.0,  1.0,  -1.0, 1.0, 0.0, 0.5, 1.0
  };
  sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc
  ) { .data = SG_RANGE(vertices), .label = "cube-vertices" });

  /* create an index buffer for the cube */
  uint16_t  indices[] = { 0,  1,  2,  0,  2,  3,  6,  5,  4,  7,  6,  4,
                          8,  9,  10, 8,  10, 11, 14, 13, 12, 15, 14, 12,
                          16, 17, 18, 16, 18, 19, 22, 21, 20, 23, 22, 20 };
  sg_buffer ibuf
      = sg_make_buffer(&(sg_buffer_desc) { .type  = SG_BUFFERTYPE_INDEXBUFFER,
                                           .data  = SG_RANGE(indices),
                                           .label = "cube-indices" });

  /* create shader */
  sg_shader shd = sg_make_shader(cube_shader_desc(sg_query_backend()));

  /* create pipeline object */
  state.pip = sg_make_pipeline(&(sg_pipeline_desc) {
      .layout     = { /* test to provide buffer stride, but no attr offsets */
                      .buffers[0].stride = 28,
                      .attrs             = { [ATTR_cube_position].format = SG_VERTEXFORMAT_FLOAT3,
                                             [ATTR_cube_color0].format   = SG_VERTEXFORMAT_FLOAT4 } },
      .shader     = shd,
      .index_type = SG_INDEXTYPE_UINT16,
      .cull_mode  = SG_CULLMODE_BACK,
      .depth      = {
               .write_enabled = true,
               .compare       = SG_COMPAREFUNC_LESS_EQUAL,
      },
      .label = "cube-pipeline" });

  /* setup resource bindings */
  state.bind
      = (sg_bindings) { .vertex_buffers[0] = vbuf, .index_buffer = ibuf };
}

void frame(void) {
  vs_params_t vs_params;
  const float w    = sapp_widthf();
  const float h    = sapp_heightf();
  const float t    = (float)(sapp_frame_duration() * 60.0);
  hmm_mat4    proj = HMM_Perspective(60.0f, w / h, 0.01f, 10.0f);
  hmm_mat4    view = HMM_LookAt(
      HMM_Vec3(0.0f, 1.5f, 6.0f), HMM_Vec3(0.0f, 0.0f, 0.0f),
      HMM_Vec3(0.0f, 1.0f, 0.0f)
  );
  hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);
  state.rx += 1.0f * t;
  state.ry += 2.0f * t;
  hmm_mat4 rxm   = HMM_Rotate(state.rx, HMM_Vec3(1.0f, 0.0f, 0.0f));
  hmm_mat4 rym   = HMM_Rotate(state.ry, HMM_Vec3(0.0f, 1.0f, 0.0f));
  hmm_mat4 model = HMM_MultiplyMat4(rxm, rym);
  vs_params.mvp  = HMM_MultiplyMat4(view_proj, model);

  sg_begin_pass(&(sg_pass) {
      .action = {
          .colors[0] = { .load_action = SG_LOADACTION_CLEAR,
                         .clear_value = { 0.25f, 0.5f, 0.75f, 1.0f } },
      },
      .swapchain = sglue_swapchain() });
  sg_apply_pipeline(state.pip);
  sg_apply_bindings(&state.bind);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, 36, 1);
  sg_end_pass();
  sg_commit();
}

void cleanup(void) {
  log_info("shuting down");

  sg_shutdown();
  sqv_err err = qk_deinit(&state.mdl);
  assert(err == SQV_SUCCESS);
}

sapp_desc sokol_main(int argc, char* argv[]) {
  log_info("starting");

  (void)argc;
  (void)argv;

  return (sapp_desc) {
    .init_cb    = init,
    .frame_cb   = frame,
    .cleanup_cb = cleanup,
    // .event_cb = __dbgui_event,
    .width              = 50,
    .height             = 50,
    .sample_count       = 4,
    .window_title       = "Cube (sokol-app)",
    .icon.sokol_default = true,
    .logger.func        = slog_func,
  };
}
