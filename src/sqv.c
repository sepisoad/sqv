#include <stdbool.h>
#include <stdio.h>
#include "../deps/hmm.h"
#include "../deps/log.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_args.h"
#include "../deps/sokol_gfx.h"
#include "../deps/nuklear.h"
#include "../deps/sokol_nuklear.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"

#define UTILS_ARENA_IMPLEMENTATION
#define UTILS_ENDIAN_IMPLEMENTATION
#define MD1_IMPLEMENTATION

#include "glsl/default.h"
#include "quake/md1.h"
#include "utils/types.h"

#define FOV 60.0f
#define DEFAULT_WIDTH 512
#define DEFAULT_HEIGHT 512

static struct {
  sg_pipeline pip;
  sg_bindings bind;
  float roty;
  struct {
    qk_model mdl;
    const f32* vbuf;
    u32 vbuf_len;
  } qk;
  struct {
    sg_image color_img;
    sg_image depth_img;
    sg_attachments atts;
    sg_pass_action pass_action;
    snk_image_t nk_img;
    int width;
    int height;
  } offscreen;
  struct {
    sg_pass_action pass_action;
  } display;
  struct nk_context* ctx;
} S;

static void _load(cstr path) {
  u8* bf = NULL;
  sz bfsz = load_file(path, &bf);
  makesure(bfsz > 0, "loadfile return size is zero");

  qk_error err = qk_load_mdl(bf, bfsz, &S.qk.mdl);
  makesure(err == MD1_ERR_SUCCESS, "qk_load_mdl failed");
  free(bf);
}

static void update_offscreen_target(int width, int height) {
  // Destroy existing resources if they exist
  if (sg_isvalid() && S.offscreen.color_img.id != SG_INVALID_ID) {
    snk_destroy_image(S.offscreen.nk_img);
    sg_destroy_attachments(S.offscreen.atts);
    sg_destroy_image(S.offscreen.depth_img);
    sg_destroy_image(S.offscreen.color_img);
  }

  // Update stored dimensions
  S.offscreen.width = width > 0 ? width : DEFAULT_WIDTH;
  S.offscreen.height = height > 0 ? height : DEFAULT_HEIGHT;

  // Create new color image
  S.offscreen.color_img = sg_make_image(&(sg_image_desc){
      .render_target = true,
      .width = S.offscreen.width,
      .height = S.offscreen.height,
      .pixel_format = SG_PIXELFORMAT_RGBA8,
      .sample_count = 1,
  });

  // Create new depth image
  S.offscreen.depth_img = sg_make_image(&(sg_image_desc){
      .render_target = true,
      .width = S.offscreen.width,
      .height = S.offscreen.height,
      .pixel_format = SG_PIXELFORMAT_DEPTH,
      .sample_count = 1,
  });

  // Create new attachments
  S.offscreen.atts = sg_make_attachments(&(sg_attachments_desc){
      .colors[0].image = S.offscreen.color_img,
      .depth_stencil.image = S.offscreen.depth_img,
  });

  // Create new Nuklear image
  S.offscreen.nk_img = snk_make_image(&(snk_image_desc_t){
      .image = S.offscreen.color_img,
      .sampler = sg_make_sampler(&(sg_sampler_desc){
          .min_filter = SG_FILTER_LINEAR,
          .mag_filter = SG_FILTER_LINEAR,
          .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
          .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
      }),
  });

  // Update pass action (recreate in case dimensions affect it)
  S.offscreen.pass_action = (sg_pass_action){
      .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.25f, 0.5f, 0.75f, 1.0f}},
  };
}

static void init(void) {
  log_info("initializing gpu ...");

  cstr mdl_file_path = (cstr)sapp_userdata();
  log_info("loading '%s' model", mdl_file_path);

  // Setup sokol-gfx
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  // Setup sokol-nuklear
  snk_setup(&(snk_desc_t){
      .enable_set_mouse_cursor = true,
      .dpi_scale = sapp_dpi_scale(),
      .logger.func = slog_func,
  });

  _load(mdl_file_path);

  qk_get_frame_vertices(&S.qk.mdl, 0, 0, &S.qk.vbuf, &S.qk.vbuf_len);

  sg_shader shd = sg_make_shader(cube_shader_desc(sg_query_backend()));

  sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_VERTEXBUFFER,
      .data = {.ptr = S.qk.vbuf, .size = S.qk.vbuf_len * sizeof(f32)},
  });

  S.pip = sg_make_pipeline(&(sg_pipeline_desc){
      .layout = {.attrs =
                     {
                         [ATTR_cube_position].format = SG_VERTEXFORMAT_FLOAT3,
                         [ATTR_cube_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
                     }},
      .shader = shd,
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .cull_mode = SG_CULLMODE_BACK,
      .depth = {
          .write_enabled = true,
          .compare = SG_COMPAREFUNC_LESS_EQUAL,
          .pixel_format = SG_PIXELFORMAT_DEPTH,
      }});

  S.bind = (sg_bindings){.vertex_buffers[0] = vbuf};
  S.bind.images[IMG_tex] = S.qk.mdl.skins[0].image;
  S.bind.samplers[SMP_smp] = S.qk.mdl.skins[0].sampler;

  // Initialize offscreen render target with window size
  update_offscreen_target(sapp_width(), sapp_height());

  // Setup display pass action
  S.display.pass_action = (sg_pass_action){
      .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.1f, 0.1f, 0.1f, 1.0f}},
  };
}

static void draw_3d() {
  S.roty += 1.0f;

  hmm_v3* bbmin = &S.qk.mdl.header.bbox_min;
  hmm_v3* bbmax = &S.qk.mdl.header.bbox_max;
  hmm_vec3 center = HMM_MultiplyVec3f(HMM_AddVec3(*bbmin, *bbmax), 0.5f);
  f32 dx = bbmax->X - bbmin->X;
  f32 dy = bbmax->Y - bbmin->Y;
  f32 dz = bbmax->Z - bbmin->Z;
  f32 radius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);

  // Use current offscreen target dimensions for aspect ratio
  f32 aspect = (f32)S.offscreen.width / (f32)S.offscreen.height;
  f32 cam_dist = (radius / sinf(HMM_ToRadians(FOV) * 0.5f)) * 1.5f;

  hmm_vec3 eye_pos = HMM_AddVec3(center, HMM_Vec3(0.0f, 0.0f, cam_dist));
  hmm_vec3 up = HMM_Vec3(0.0f, 1.0f, 0.0f);

  hmm_mat4 proj = HMM_Perspective(FOV, aspect, 0.1f, cam_dist * 4.0f);
  hmm_mat4 view = HMM_LookAt(eye_pos, center, up);
  hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);

  hmm_mat4 translate_to_origin =
      HMM_Translate(HMM_MultiplyVec3f(center, -1.0f));

  hmm_mat4 rxm =
      HMM_Rotate(89, HMM_Vec3(1.0f, 0.0f, 0.0f));  // Your fix for orientation
  hmm_mat4 rym = HMM_Rotate(0, HMM_Vec3(0.0f, 1.0f, 0.0f));
  hmm_mat4 rzm = HMM_Rotate(S.roty, HMM_Vec3(0.0f, 0.0f, -1.0f));
  hmm_mat4 rotation = HMM_MultiplyMat4(HMM_MultiplyMat4(rxm, rym), rzm);

  hmm_mat4 translate_back = HMM_Translate(center);

  hmm_mat4 model = HMM_MultiplyMat4(
      translate_back, HMM_MultiplyMat4(rotation, translate_to_origin));

  vs_params_t vs_params = {
      .mvp = HMM_MultiplyMat4(view_proj, model),
  };

  // Render 3D scene to offscreen texture
  sg_begin_pass(&(sg_pass){
      .action = S.offscreen.pass_action,
      .attachments = S.offscreen.atts,
  });
  sg_apply_pipeline(S.pip);
  sg_apply_bindings(&S.bind);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, S.qk.vbuf_len, 1);
  sg_end_pass();
}

static void draw_ui() {
  S.ctx = snk_new_frame();
  nk_style_hide_cursor(S.ctx);

  struct nk_style_window default_window_style = S.ctx->style.window;
  struct nk_vec2 default_spacing = S.ctx->style.window.spacing;
  S.ctx->style.window.padding = nk_vec2(0, 0);
  S.ctx->style.window.spacing = nk_vec2(0, 0);

  // Begin the window with no padding
  if (nk_begin(S.ctx, "SQV", nk_rect(0, 0, sapp_width(), sapp_height()),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(S.ctx, sapp_height(), 1);
    nk_image(S.ctx, nk_image_handle(snk_nkhandle(S.offscreen.nk_img)));
  }
  nk_end(S.ctx);

  S.ctx->style.window.padding = default_window_style.padding;
  S.ctx->style.window.spacing = default_spacing;

  sg_begin_pass(&(sg_pass){
      .action = S.display.pass_action,
      .swapchain = sglue_swapchain(),
  });

  snk_render(sapp_width(), sapp_height());
  sg_end_pass();
  sg_commit();
}

static void frame(void) {
  draw_3d();
  draw_ui();
}

static void input(const sapp_event* event) {
  snk_handle_event(event);
  if (event->type == SAPP_EVENTTYPE_RESIZED) {
    // Update offscreen render target when window is resized
    update_offscreen_target(event->window_width, event->window_height);
  }
}

static void cleanup(void) {
  log_info("shutting down");
  qk_unload_mdl(&S.qk.mdl);
  // Clean up offscreen resources
  if (sg_isvalid()) {
    snk_destroy_image(S.offscreen.nk_img);
    sg_destroy_attachments(S.offscreen.atts);
    sg_destroy_image(S.offscreen.depth_img);
    sg_destroy_image(S.offscreen.color_img);
  }
  snk_shutdown();
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
      .event_cb = input,
      .width = 800,
      .height = 600,
      .sample_count = 1,
      .window_title = "SQV with Nuklear",
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };
}
