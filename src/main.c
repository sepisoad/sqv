#define SOKOL_IMPL
#define SOKOL_GLCORE
#define SOKOL_DEBUG
#define STB_IMAGE_IMPLEMENTATION

#include <sokol_app.h>
#include <sokol_fetch.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <stb_image.h>

#include "shader.h"
#include "sokol_helper.h"
#include "mdl.h"

int mdl_read_model(sg_bindings b, const char *path, mdl_model *m);
void mdl_render_frame(int n, const mdl_model *mdl);
void mdl_free(mdl_model *m);

static struct {
  sg_pipeline pip;
  sg_bindings bind;
  sg_pass_action pass_action;
  uint8_t file_buffer[512 * 1024];
} state;

static void fetch_callback(const sfetch_response_t *);

static void init(void) {
  sg_setup(&(sg_desc){.environment = sglue_environment()});

  sfetch_setup(
      &(sfetch_desc_t){.max_requests = 1, .num_channels = 1, .num_lanes = 1});

  sg_alloc_image_smp(state.bind, 0, 1);

  float vertices[] = {
      // positions        // texture coords
      0.5f,  0.5f,  0.0f, 1.0f, 1.0f, // top right
      0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, // bottom right
      -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // bottom left
      -0.5f, 0.5f,  0.0f, 0.0f, 1.0f  // top left
  };
  state.bind.vertex_buffers[0] =
      sg_make_buffer(&(sg_buffer_desc){.size = sizeof(vertices),
                                       .data = SG_RANGE(vertices),
                                       .label = "quad-vertices"});

  uint16_t indices[] = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };
  state.bind.index_buffer =
      sg_make_buffer(&(sg_buffer_desc){.type = SG_BUFFERTYPE_INDEXBUFFER,
                                       .size = sizeof(indices),
                                       .data = SG_RANGE(indices),
                                       .label = "quad-indices"});

  sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

  state.pip = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = shd,
      .index_type = SG_INDEXTYPE_UINT16,
      .layout = {.attrs = {[0].format = SG_VERTEXFORMAT_FLOAT3,
                           [1].format = SG_VERTEXFORMAT_FLOAT2}},
      .label = "triangle-pipeline"});

  state.pass_action =
      (sg_pass_action){.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                                     .clear_value = {0.2f, 0.3f, 0.3f, 1.0f}}};

  const char *mdlpath = ".keep/dog.mdl";
  mdl_model mdl;
  mdl_read_model(state.bind, mdlpath, &mdl);

  // sfetch_send(&(sfetch_request_t){
  //     .path = "res/textures/container2.png",
  //     .callback = fetch_callback,
  //     .buffer = SFETCH_RANGE(state.file_buffer),
  // });
}

// static void fetch_callback(const sfetch_response_t *response) {
//   if (response->fetched) {
//     int img_width, img_height, num_channels;
//     const int desired_channels = 4;
//     stbi_uc *pixels = stbi_load_from_memory(
//         response->data.ptr, (int)response->data.size, &img_width,
//         &img_height, &num_channels, desired_channels);
//     if (pixels) {
//       sg_init_image(state.bind.images[0],
//                     &(sg_image_desc){.width = img_width,
//                                      .height = img_height,
//                                      .pixel_format = SG_PIXELFORMAT_RGBA8,
//                                      .data.subimage[0][0] = {
//                                          .ptr = pixels,
//                                          .size = img_width * img_height * 4,
//                                      }});
//       stbi_image_free(pixels);
//     }
//   } else if (response->failed) {
//     state.pass_action = (sg_pass_action){
//         .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
//                       .clear_value = {1.0f, 0.0f, 0.0f, 1.0f}}};
//   }
// }

void frame(void) {
  sfetch_dowork();
  sg_begin_pass(
      &(sg_pass){.action = state.pass_action, .swapchain = sglue_swapchain()});
  sg_apply_pipeline(state.pip);
  sg_apply_bindings(&state.bind);
  sg_draw(0, 6, 1);
  sg_end_pass();
  sg_commit();
}

void cleanup(void) {
  sg_shutdown();
  sfetch_shutdown();
}

void event(const sapp_event *e) {
  if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
    if (e->key_code == SAPP_KEYCODE_ESCAPE) {
      sapp_request_quit();
    }
  }
}

sapp_desc sokol_main(int argc, char *argv[]) {
  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .event_cb = event,
      .width = 400,
      .height = 400,
      .high_dpi = true,
      .window_title = "Texture - LearnOpenGL",
  };
}
