#define SOKOL_IMPL
#define SOKOL_GLCORE
#define SOKOL_DEBUG
#define STB_IMAGE_IMPLEMENTATION
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE

#include "hmm.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "stb/stb_image.h"
#include "shader.h"
#include "mdl.h"

int mdl_read_model(sg_bindings, const char *, mdl_model *);
void mdl_assemble_buffer(int, const mdl_model *, float *, uint16_t *);
void mdl_free(mdl_model *);

static struct {
  float rx, ry;
  sg_pass_action pass_action;
  sg_pipeline pip;
  sg_bindings bind;
  mdl_model mdl;
  int indices_count;
} state;

float *mdl_vertices = NULL;
uint16_t *mdl_indices = NULL;

static void init(void) {
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  state.pass_action = (sg_pass_action){
      .colors[0] =
          {
              .load_action = SG_LOADACTION_CLEAR,
              .clear_value = {0.125f, 0.25f, 0.35f, 1.0f},
          },
  };

  state.bind.images[IMG_tex] = sg_alloc_image();
  state.bind.samplers[SMP_smp] = sg_make_sampler(&(sg_sampler_desc){
      .min_filter = SG_FILTER_LINEAR,
      .mag_filter = SG_FILTER_LINEAR,
  });

  const char *mdl_path = ".keep/spike.mdl";
  mdl_read_model(state.bind, mdl_path, &state.mdl);

  const size_t mdl_vertices_count = state.mdl.header.num_tris * 3 * 3;
  const size_t mdl_uv_count = state.mdl.header.num_tris * 3 * 2;
  const size_t mdl_vertices_size =
      sizeof(float) * (mdl_vertices_count + mdl_uv_count);

  const size_t mdl_indices_count = state.mdl.header.num_tris * 3;
  const size_t mdl_indices_size = sizeof(int) * mdl_indices_count;
  state.indices_count = mdl_indices_count;

  mdl_vertices = (float *)malloc(mdl_vertices_size);
  mdl_indices = (uint16_t *)malloc(mdl_indices_size);

  mdl_assemble_buffer(0, &state.mdl, mdl_vertices, mdl_indices);

  // for (int i = 0; i < mdl_indices_count; i++) {
  //   printf("ind%02d=[%d]\n", i, mdl_indices[i]);
  // }

  // printf("============\n");

  // int dbg_idx = 0;
  // for (int i = 0; i < mdl_vertices_count + mdl_uv_count; i += 5) {
  //   printf("xyz%02d=[%f, %f, %f]\n", dbg_idx, mdl_vertices[i + 0],
  //          mdl_vertices[i + 1], mdl_vertices[i + 2]);
  //   dbg_idx++;
  // }

  // printf("============\n");

  // dbg_idx = 0;
  // for (int i = 0; i < mdl_vertices_count + mdl_uv_count; i += 5) {
  //   printf("uv%02d=[%f, %f]\n", dbg_idx, mdl_vertices[i + 3],
  //          mdl_vertices[i + 4]);
  //   dbg_idx++;
  // }

  // state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
  //     .type = SG_BUFFERTYPE_VERTEXBUFFER,
  //     .data =
  //         (sg_range){
  //             .ptr = mdl_vertices,
  //             .size = mdl_vertices_size,
  //         },
  // });
  // state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
  //     .type = SG_BUFFERTYPE_INDEXBUFFER,
  //     .data =
  //         (sg_range){
  //             .ptr = mdl_indices,
  //             .size = mdl_indices_size,
  //         },
  // });

  float dbg_vertices[] = {
      /* vert(3)                       uv(2)                 */
      -0.016965, -1.055456, -0.941016, 0.145833, 0.925000, /**/
      10.157645, -0.005767, 0.051735,  0.256944, 0.525000, /**/
      -0.016965, 1.019510,  -0.918281, 0.368056, 0.875000, /**/
      -0.016965, -1.055456, -0.941016, 0.145833, 0.925000, /**/
      -0.016965, -0.005767, 0.991438,  0.256944, 0.125000, /**/
      10.157645, -0.005767, 0.051735,  0.256944, 0.525000, /**/
      -0.016965, 1.019510,  -0.918281, 0.368056, 0.875000, /**/
      10.157645, -0.005767, 0.051735,  0.256944, 0.525000, /**/
      -0.016965, -0.005767, 0.991438,  0.256944, 0.125000, /**/
      -0.016965, 1.019510,  -0.918281, 0.868056, 0.875000, /**/
      -0.016965, -0.005767, 0.991438,  0.756944, 0.125000, /**/
      -0.016965, -1.055456, -0.941016, 0.645833, 0.925000, /**/
  };

  uint16_t dbg_indices[] = {0, 1, 2, 0, 3, 1, 2, 1, 3, 2, 3, 0};

  state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .data = SG_RANGE(dbg_vertices),
  });
  state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_INDEXBUFFER,
      .data = SG_RANGE(dbg_indices),
  });

  state.pip = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = sg_make_shader(sepi_shader_desc(sg_query_backend())),
      .index_type = SG_INDEXTYPE_UINT16,
      .cull_mode = SG_CULLMODE_BACK,
      .face_winding = SG_FACEWINDING_CCW,
      .primitive_type = SG_PRIMITIVETYPE_LINES,
      .depth =
          {
              .compare = SG_COMPAREFUNC_LESS_EQUAL,
              .write_enabled = true,
          },
      .layout = {
          .attrs =
              {
                  [ATTR_sepi_pos].format = SG_VERTEXFORMAT_FLOAT3,
                  [ATTR_sepi_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
              },
      }});
}

static void frame(void) {
  // compute model-view-projection matrix for vertex shader
  const float t = (float)(sapp_frame_duration() * 60.0);
  hmm_mat4 proj =
      HMM_Perspective(60.0f, sapp_widthf() / sapp_heightf(), 0.01f, 1000.0f);
  hmm_mat4 view =
      HMM_LookAt(HMM_Vec3(0.0f, 1.5f, 60.0f), HMM_Vec3(0.0f, 0.0f, 0.0f),
                 HMM_Vec3(0.0f, 1.0f, 0.0f));
  hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);
  vs_params_t vs_params;
  state.rx += 1.0f * t;
  state.ry += 2.0f * t;
  hmm_mat4 rxm = HMM_Rotate(state.rx, HMM_Vec3(1.0f, 0.0f, 0.0f));
  hmm_mat4 rym = HMM_Rotate(state.ry, HMM_Vec3(0.0f, 1.0f, 0.0f));
  hmm_mat4 model = HMM_MultiplyMat4(rxm, rym);
  vs_params.mvp = HMM_MultiplyMat4(view_proj, model);

  sg_begin_pass(
      &(sg_pass){.action = state.pass_action, .swapchain = sglue_swapchain()});
  sg_apply_pipeline(state.pip);
  sg_apply_bindings(&state.bind);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, state.indices_count, 1);
  sg_end_pass();
  sg_commit();
}

static void cleanup(void) {
  sg_shutdown();

  if (mdl_vertices) {
    free(mdl_vertices);
    mdl_vertices = NULL;
  }

  if (mdl_indices) {
    free(mdl_indices);
    mdl_indices = NULL;
  }
}

void event(const sapp_event *e) {
  if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
    if (e->key_code == SAPP_KEYCODE_ESCAPE) {
      sapp_request_quit();
    }
  }
}

sapp_desc sokol_main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .event_cb = event,
      .width = 400,
      .height = 400,
      .sample_count = 4,
      .window_title = "learn",
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };
}