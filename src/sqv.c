#include <stdbool.h>

#include "../deps/hmm.h"
#include "../deps/log.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"

#define UTILS_ENDIAN_IMPLEMENTATION
#define UTILS_ARENA_IMPLEMENTATION
#define UTILS_ENDIAN_IMPLEMENTATION
#define QK_MDL_IMPLEMENTATION

#include "glsl/default.h"
#include "quake/mdl.h"
#include "errors.h"
#include "utils/all.h"

static struct {
  float rx, ry;
  sg_pipeline pip;
  sg_bindings bind;
  qk_mdl qkmdl;
  float* vertices;
  uint32_t* indices;
  size_t indices_count;
  hmm_vec3 bbox_min;
  hmm_vec3 bbox_max;
  hmm_vec3 scale;
} state;

void load_mdl_file(const char* path) {
  uint8_t* mdlbuf = NULL;
  size_t mdlbufsz = load_file(path, &mdlbuf);
  makesure(mdlbufsz > 0, "loadfile return size is zero");

  qk_err err = qk_load_mdl(mdlbuf, mdlbufsz, &state.qkmdl);
  makesure(err == QK_ERR_SUCCESS, "qk_load_mdl failed");
  free(mdlbuf);
}

void init(void) {
  log_info("initializing gpu ...");

  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  sg_shader shd = sg_make_shader(cube_shader_desc(sg_query_backend()));

  state.vertices = NULL;
  state.indices = NULL;
  size_t verticesz = 0;

  load_mdl_file(".keep/dog.mdl");
  arena_print(&state.qkmdl.mem);

  memcpy(&state.scale, &state.qkmdl.header.scale, sizeof(state.scale));
  state.indices = state.qkmdl.indices;
  state.indices_count = state.qkmdl.header.indices_count;
  qk_get_frame_vertices(&state.qkmdl, 0, &state.vertices, &verticesz,
                        &state.bbox_min, &state.bbox_max);

  sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
      .size = state.qkmdl.header.vertices_count * 3 *
              sizeof(float),  // Corrected size
      .data = {.ptr = state.vertices,
               .size = state.qkmdl.header.vertices_count * 3 * sizeof(float)},
  });

  sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_INDEXBUFFER,
      .size = state.qkmdl.header.indices_count * sizeof(uint32_t),
      .data =
          {
              .ptr = state.qkmdl.indices,
              .size = state.qkmdl.header.indices_count * sizeof(uint32_t),
          },
      .label = "cube-indices"});

  state.pip = sg_make_pipeline(&(sg_pipeline_desc){
      .layout = {.attrs = {[ATTR_cube_position].format = SG_VERTEXFORMAT_FLOAT3,
                           [ATTR_cube_position].buffer_index = 0}},
      .shader = shd,
      .index_type = SG_INDEXTYPE_UINT32,
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .cull_mode = SG_CULLMODE_BACK,
      .depth =
          {
              .write_enabled = true,
              .compare = SG_COMPAREFUNC_LESS_EQUAL,
          },
      .label = "cube-pipeline"});

  // ===============================================

  state.bind = (sg_bindings){.vertex_buffers[0] = vbuf, .index_buffer = ibuf};
}

void frame(void) {
  vs_params_t vs_params;

  float w = sapp_widthf();
  float h = sapp_heightf();

  // Transform the bounding box using scale and translate
  // hmm_vec3 bbox_min = HMM_AddVec3(HMM_MultiplyVec3(state.bbox_min,
  // state.scale),
  //                                 state.qkmdl.header.translate);

  // hmm_vec3 bbox_max = HMM_AddVec3(HMM_MultiplyVec3(state.bbox_max,
  // state.scale),
  //                                 state.qkmdl.header.translate);

  hmm_vec3 bbox_min = state.bbox_min;
  hmm_vec3 bbox_max = state.bbox_max;
  hmm_vec3 center = HMM_MultiplyVec3f(HMM_AddVec3(bbox_min, bbox_max), 0.5f);
  float dx = bbox_max.X - bbox_min.X;
  float dy = bbox_max.Y - bbox_min.Y;
  float dz = bbox_max.Z - bbox_min.Z;
  float radius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);

  // Compute proper camera distance
  float fov = 60.0f;
  float aspect = w / h;
  float cam_dist = (radius / sinf(HMM_ToRadians(fov) * 0.5f)) * 1.5f;

  hmm_vec3 eye_pos = HMM_AddVec3(center, HMM_Vec3(0.0f, 0.0f, cam_dist));
  hmm_vec3 up = HMM_Vec3(0.0f, 1.0f, 0.0f);

  hmm_mat4 proj = HMM_Perspective(fov, aspect, 0.1f, cam_dist * 4.0f);
  hmm_mat4 view = HMM_LookAt(eye_pos, center, up);
  hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);
  vs_params.mvp = HMM_MultiplyMat4(view_proj, HMM_Mat4d(1.0f));

  sg_begin_pass(&(sg_pass){
      .action =
          {
              .colors[0] =
                  {
                      .load_action = SG_LOADACTION_CLEAR,
                      .clear_value = {0.25f, 0.5f, 0.75f, 1.0f},
                  },
          },
      .swapchain = sglue_swapchain(),
  });

  sg_apply_pipeline(state.pip);
  sg_apply_bindings(&state.bind);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, state.indices_count, 1);
  sg_end_pass();
  sg_commit();
}

void cleanup(void) {
  log_info("shuting down");
  sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
  log_info("starting");

  (void)argc;
  (void)argv;

  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .width = 400,
      .height = 400,
      .sample_count = 1,
      .window_title = "Cube (sokol-app)",
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };
}
