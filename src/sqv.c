#include <stdbool.h>

#include "../deps/hmm.h"
#include "../deps/log.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_args.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"

#define UTILS_ENDIAN_IMPLEMENTATION
#define UTILS_ARENA_IMPLEMENTATION
#define UTILS_ENDIAN_IMPLEMENTATION
#define QK_MDL_IMPLEMENTATION

#include "glsl/default.h"
#include "quake/mdl.h"
#include "utils/types.h"

#define FOV 60.0f

static struct {
  sg_pipeline pip;
  sg_bindings bind;
  float roty;
  struct {
    qk_mdl mdl;
    hmm_vec3 bbmin;
    hmm_vec3 bbmax;
  } qk;
} S;

static void _load(cstr path) {
  u8* bf = NULL;
  sz bfsz = load_file(path, &bf);
  makesure(bfsz > 0, "loadfile return size is zero");

  qk_err err = qk_load_mdl(bf, bfsz, &S.qk.mdl);
  makesure(err == QK_ERR_SUCCESS, "qk_load_mdl failed");
  free(bf);
}

static void init(void) {
  log_info("initializing gpu ...");

  cstr mdl_file_path = (cstr)sapp_userdata();
  log_info("loading '%s' model", mdl_file_path);

  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  _load(mdl_file_path);

  f32* verts = NULL;
  u32 verts_cn = 0;
  u32* inds = S.qk.mdl.indices;
  u32 inds_cn = S.qk.mdl.header.indices_count;

  qk_get_frame_vertices(&S.qk.mdl, 0, &verts, &verts_cn, &S.qk.bbmin,
                        &S.qk.bbmax);

  sg_shader shd = sg_make_shader(cube_shader_desc(sg_query_backend()));

  sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .data = {.ptr = verts, .size = verts_cn * sizeof(f32)},
  });

  sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_INDEXBUFFER,
      .data = {.ptr = inds, .size = inds_cn * sizeof(u32)},
  });

  S.pip = sg_make_pipeline(&(sg_pipeline_desc){
      .layout =
          {.attrs = {[ATTR_cube_position] = {.format = SG_VERTEXFORMAT_FLOAT3,
                                             .buffer_index = 0},
                     [ATTR_cube_texcoord0] = {.format = SG_VERTEXFORMAT_FLOAT2,
                                              .buffer_index = 0}}},
      .shader = shd,
      .index_type = SG_INDEXTYPE_UINT32,
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .cull_mode = SG_CULLMODE_BACK,
      .depth = {
          .write_enabled = true,
          .compare = SG_COMPAREFUNC_LESS_EQUAL,
      }});

  S.bind = (sg_bindings){.vertex_buffers[0] = vbuf, .index_buffer = ibuf};
  S.bind.images[IMG_tex] = S.qk.mdl.skins[0].image;
  S.bind.samplers[SMP_smp] = S.qk.mdl.skins[0].sampler;
}

static void frame(void) {
  S.roty += 1.0f;

  hmm_vec3 center =
      HMM_MultiplyVec3f(HMM_AddVec3(S.qk.bbmin, S.qk.bbmax), 0.5f);
  f32 dx = S.qk.bbmax.X - S.qk.bbmin.X;
  f32 dy = S.qk.bbmax.Y - S.qk.bbmin.Y;
  f32 dz = S.qk.bbmax.Z - S.qk.bbmin.Z;
  f32 radius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);

  f32 aspect = sapp_widthf() / sapp_heightf();
  f32 cam_dist = (radius / sinf(HMM_ToRadians(FOV) * 0.5f)) * 1.5f;

  hmm_vec3 eye_pos = HMM_AddVec3(center, HMM_Vec3(0.0f, 0.0f, cam_dist));
  hmm_vec3 up = HMM_Vec3(0.0f, 1.0f, 0.0f);

  hmm_mat4 proj = HMM_Perspective(FOV, aspect, 0.1f, cam_dist * 4.0f);
  hmm_mat4 view = HMM_LookAt(eye_pos, center, up);
  hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);

  hmm_mat4 translate_to_origin =
      HMM_Translate(HMM_MultiplyVec3f(center, -1.0f));

  hmm_mat4 rxm = HMM_Rotate(90, HMM_Vec3(-1.0f, 0.0f, 0.0f));
  hmm_mat4 rym = HMM_Rotate(0, HMM_Vec3(0.0f, 1.0f, 0.0f));
  hmm_mat4 rzm = HMM_Rotate(S.roty, HMM_Vec3(0.0f, 0.0f, -1.0f));
  hmm_mat4 rotation = HMM_MultiplyMat4(HMM_MultiplyMat4(rxm, rym), rzm);

  hmm_mat4 translate_back = HMM_Translate(center);

  hmm_mat4 model = HMM_MultiplyMat4(
      translate_back, HMM_MultiplyMat4(rotation, translate_to_origin));

  vs_params_t vs_params = {
      .mvp = HMM_MultiplyMat4(view_proj, model),
  };

  sg_begin_pass(&(sg_pass){
      .action =
          {
              .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                            .clear_value = {0.25f, 0.5f, 0.75f, 1.0f}},
          },
      .swapchain = sglue_swapchain()});

  sg_apply_pipeline(S.pip);
  sg_apply_bindings(&S.bind);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, S.qk.mdl.header.indices_count, 1);
  sg_end_pass();
  sg_commit();
}

static void cleanup(void) {
  log_info("shuting down");
  qk_unload_mdl(&S.qk.mdl);
  sg_shutdown();
  sargs_shutdown();
}

sapp_desc sokol_main(i32 argc, char* argv[]) {
  log_info("starting");

  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
  });

  cstr _mdl = NULL;
  if (sargs_exists("-m")) {
    _mdl = sargs_value("-m");
  } else if (sargs_exists("--model")) {
    _mdl = sargs_value("--model");
  } else {
    makesure(false,
             "you need to provide either '-m' or '--model' to define the path "
             "to the MDL model");
  }

  return (sapp_desc){
      .user_data = (rptr)_mdl,
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .width = 400,
      .height = 400,
      .sample_count = 1,
      .window_title = "SQV",
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };
}
