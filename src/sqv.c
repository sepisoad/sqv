#include <stdbool.h>
#include <stdio.h>
#include "../deps/hmm.h"
#include "../deps/log.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_args.h"
#include "../deps/sokol_gfx.h"
/* #include "../deps/nuklear.h" */
/* #include "../deps/sokol_nuklear.h" */
#include "../deps/cimgui.h"
#include "../deps/sokol_imgui.h"

#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"

/* #define UTILS_ENDIAN_IMPLEMENTATION */
/* #define UTILS_ARENA_IMPLEMENTATION */
/* #define UTILS_ENDIAN_IMPLEMENTATION */
/* #define MD1_IMPLEMENTATION */

/* #include "glsl/default.h" */
/* #include "quake/md1.h" */
/* #include "utils/types.h" */

/* #define FOV 60.0f */
/* #define DEFAULT_WINDOW_WIDTH 400 */
/* #define DEFAULT_WINDOW_HEIGHT 400 */

/* static struct { */
/*   struct { */
/*     qk_model mdl; */
/*     const f32* vbuf; */
/*     u32 vbuf_len; */
/*   } qk; */

/*   struct { */
/*     float rty; */
/*   } app; */

/*   struct { */
/*     sg_image scn; */
/*     sg_image dpt; */
/*     sg_pipeline pip; */
/*     sg_bindings bnd; */
/*     sg_attachments atc; */
/*     sg_pass_action pas; */
/*   } ui3d; */

/*   struct { */
/*     snk_image_t scn; */
/*     sg_pass_action pas; */
/*   } ui2d; */
/* } S; */

/* static void load_md1_file(cstr path) { */
/*   log_debug("loading '%s'", path); */
/*   u8* bf = NULL; */
/*   sz bfsz = load_file(path, &bf); */
/*   makesure(bfsz > 0, "loadfile return size is zero"); */

/*   qk_error err = qk_load_mdl(bf, bfsz, &S.qk.mdl); */
/*   makesure(err == MD1_ERR_SUCCESS, "qk_load_mdl failed"); */
/*   free(bf); */
/* } */

/* static void init(void) { */
/*   log_debug("initializing gpu ..."); */

/*   sg_setup(&(sg_desc){ */
/*       .environment = sglue_environment(), */
/*       .logger.func = slog_func, */
/*   }); */

/*   snk_setup(&(snk_desc_t){ */
/*       .enable_set_mouse_cursor = true, */
/*       .dpi_scale = sapp_dpi_scale(), */
/*       .logger.func = slog_func, */
/*   }); */

/*   cstr mdl_file_path = (cstr)sapp_userdata(); */
/*   load_md1_file(mdl_file_path); */
/*   qk_get_frame_vertices(&S.qk.mdl, 0, 0, &S.qk.vbuf, &S.qk.vbuf_len); */

/*   sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){ */
/*       .type = SG_BUFFERTYPE_VERTEXBUFFER, */
/*       .data = {.ptr = S.qk.vbuf, .size = S.qk.vbuf_len * sizeof(f32)}, */
/*   }); */

/*   S.ui3d.pip = sg_make_pipeline(&(sg_pipeline_desc){ */
/*       .cull_mode = SG_CULLMODE_BACK, */
/*       .primitive_type = SG_PRIMITIVETYPE_TRIANGLES, */
/*       .shader = sg_make_shader(cube_shader_desc(sg_query_backend())), */
/*       .depth = */
/*           { */
/*               .write_enabled = true, .compare = SG_COMPAREFUNC_LESS_EQUAL, */
/*               /\** .pixel_format = SG_PIXELFORMAT_DEPTH, **\/ */
/*           }, */
/*       .layout = */
/*           { */
/*               .attrs = */
/*                   { */
/*                       [ATTR_cube_position].format = SG_VERTEXFORMAT_FLOAT3,
 */
/*                       [ATTR_cube_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
 */
/*                   }, */
/*           }, */
/*   }); */

/*   S.ui3d.scn = sg_make_image(&(sg_image_desc){ */
/*       .render_target = true, */
/*       .width = DEFAULT_WINDOW_WIDTH, */
/*       .height = DEFAULT_WINDOW_HEIGHT, */
/*       .pixel_format = SG_PIXELFORMAT_RGBA8, */
/*       .sample_count = 1, */
/*   }); */

/*   S.ui3d.dpt = sg_make_image(&(sg_image_desc){ */
/*       .render_target = true, */
/*       .width = DEFAULT_WINDOW_WIDTH, */
/*       .height = DEFAULT_WINDOW_HEIGHT, */
/*       .pixel_format = SG_PIXELFORMAT_DEPTH, */
/*       .sample_count = 1, */
/*   }); */

/*   S.ui3d.atc = sg_make_attachments(&(sg_attachments_desc){ */
/*       .colors[0].image = S.ui3d.scn, */
/*       .depth_stencil.image = S.ui3d.dpt, */
/*   }); */

/*   S.ui3d.pas = (sg_pass_action){.colors[0] = { */
/*                                     .load_action = SG_LOADACTION_CLEAR, */
/*                                     .clear_value = {0.0f, 0.0f, 0.0f, 1.0f},
 */
/*                                 }}; */

/*   S.ui2d.pas = (sg_pass_action){.colors[0] = { */
/*                                     .load_action = SG_LOADACTION_CLEAR, */
/*                                     .clear_value = {0.5f, 0.5f, 1.0f, 1.0f},
 */
/*                                 }}; */

/*   S.ui2d.scn = snk_make_image(&(snk_image_desc_t){ */
/*       .image = S.ui3d.scn, */
/*       .sampler = sg_make_sampler(&(sg_sampler_desc){ */
/*           .min_filter = SG_FILTER_NEAREST, */
/*           .mag_filter = SG_FILTER_NEAREST, */
/*           .wrap_u = SG_WRAP_CLAMP_TO_EDGE, */
/*           .wrap_v = SG_WRAP_CLAMP_TO_EDGE, */
/*       }), */
/*   }); */

/*   S.ui3d.bnd.vertex_buffers[0] = vbuf; */
/*   S.ui3d.bnd.images[IMG_tex] = S.qk.mdl.skins[0].image; */
/*   S.ui3d.bnd.samplers[SMP_smp] = S.qk.mdl.skins[0].sampler; */
/* } */

/* static void draw_model(void) { */
/*   S.app.rty += 1.0f; */

/*   hmm_v3* bbmin = &S.qk.mdl.header.bbox_min; */
/*   hmm_v3* bbmax = &S.qk.mdl.header.bbox_max; */
/*   hmm_vec3 center = HMM_MultiplyVec3f(HMM_AddVec3(*bbmin, *bbmax), 0.5f); */
/*   f32 dx = bbmax->X - bbmin->X; */
/*   f32 dy = bbmax->Y - bbmin->Y; */
/*   f32 dz = bbmax->Z - bbmin->Z; */
/*   f32 radius = 0.3f * sqrtf(dx * dx + dy * dy + dz * dz); */

/*   f32 aspect = sapp_widthf() / sapp_heightf(); */
/*   f32 cam_dist = (radius / sinf(HMM_ToRadians(FOV) * 0.5f)) * 1.5f; */

/*   hmm_vec3 eye_pos = HMM_AddVec3(center, HMM_Vec3(0.0f, 0.0f, cam_dist)); */
/*   hmm_vec3 up = HMM_Vec3(0.0f, 1.0f, 0.0f); */

/*   hmm_mat4 proj = HMM_Perspective(FOV, aspect, 0.1f, cam_dist * 4.0f); */
/*   hmm_mat4 view = HMM_LookAt(eye_pos, center, up); */
/*   hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view); */

/*   hmm_mat4 translate_to_origin = */
/*       HMM_Translate(HMM_MultiplyVec3f(center, -1.0f)); */

/*   hmm_mat4 rxm = HMM_Rotate(90, HMM_Vec3(-1.0f, 0.0f, 0.0f)); */
/*   hmm_mat4 rym = HMM_Rotate(0, HMM_Vec3(0.0f, 1.0f, 0.0f)); */
/*   hmm_mat4 rzm = HMM_Rotate(S.app.rty, HMM_Vec3(0.0f, 0.0f, -1.0f)); */
/*   hmm_mat4 rotation = HMM_MultiplyMat4(HMM_MultiplyMat4(rxm, rym), rzm); */

/*   hmm_mat4 translate_back = HMM_Translate(center); */

/*   hmm_mat4 model = HMM_MultiplyMat4( */
/*       translate_back, HMM_MultiplyMat4(rotation, translate_to_origin)); */

/*   vs_params_t vs_params = { */
/*       .mvp = HMM_MultiplyMat4(view_proj, model), */
/*   }; */

/*   sg_begin_pass( */
/*       &(sg_pass){.action = S.ui3d.pas, .swapchain = sglue_swapchain()}); */

/*   sg_apply_pipeline(S.ui3d.pip); */
/*   sg_apply_bindings(&S.ui3d.bnd); */
/*   sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params)); */
/*   sg_draw(0, S.qk.vbuf_len, 1); */
/*   sg_end_pass(); */
/*   sg_commit(); */
/* } */

/* static void draw_ui() { */
/*   struct nk_context* ctx = snk_new_frame(); */
/*   nk_style_hide_cursor(ctx); */
/*   if (nk_begin(ctx, "Hello", nk_rect(0, 0, 100, 100), 0)) { */
/*     nk_image(ctx, nk_image_handle(snk_nkhandle(S.ui2d.scn))); */
/*   } */
/*   nk_end(ctx); */

/*   sg_begin_pass( */
/*       &(sg_pass){.action = S.ui2d.pas, .swapchain = sglue_swapchain()}); */
/*   snk_render(sapp_width(), sapp_height()); */
/*   sg_end_pass(); */
/*   sg_commit(); */
/* } */

/* static void frame(void) { */
/*   draw_model(); */
/*   draw_ui(); */
/* } */

/* static void input(const sapp_event* ev) { */
/*   snk_handle_event(ev); */
/* } */

/* static void cleanup(void) { */
/*   log_debug("shuting down"); */
/*   qk_unload_mdl(&S.qk.mdl); */
/*   snk_shutdown(); */
/*   sg_shutdown(); */
/*   sargs_shutdown(); */
/* } */

/* sapp_desc sokol_main(i32 argc, char* argv[]) { */
/* #ifdef DEBUG */
/*   log_set_level(LOG_TRACE); */
/* #else */
/*   log_set_level(LOG_INFO); */
/* #endif */

/*   log_debug("starting"); */

/*   sargs_setup(&(sargs_desc){ */
/*       .argc = argc, */
/*       .argv = argv, */
/*   }); */

/*   cstr _mdl = NULL; */
/*   if (sargs_exists("-m")) { */
/*     _mdl = sargs_value("-m"); */
/*   } else if (sargs_exists("--model")) { */
/*     _mdl = sargs_value("--model"); */
/*   } else { */
/*     makesure(false, */
/*              "you need to provide either '-m' or '--model' to define the path
 * " */
/*              "to the MDL model"); */
/*   } */

/*   return (sapp_desc){ */
/*       .user_data = (rptr)_mdl, */
/*       .init_cb = init, */
/*       .frame_cb = frame, */
/*       .event_cb = input, */
/*       .cleanup_cb = cleanup, */
/*       .width = DEFAULT_WINDOW_WIDTH, */
/*       .height = DEFAULT_WINDOW_HEIGHT, */
/*       .sample_count = 1, */
/*       .window_title = "SQV", */
/*       .icon.sokol_default = true, */
/*       .logger.func = slog_func, */
/*   }; */
/* } */

//------------------------------------------------------------------------------
//  cimgui-sapp.c
//
//  Demonstrates Dear ImGui UI rendering in C via
//  sokol_gfx.h + sokol_imgui.h + cimgui.h
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "cimgui.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"

typedef struct {
  uint64_t last_time;
  bool show_test_window;
  bool show_another_window;
  sg_pass_action pass_action;
} state_t;
static state_t state;

void init(void) {
  // setup sokol-gfx, sokol-time and sokol-imgui
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });

  // use sokol-imgui with all default-options (we're not doing
  // multi-sampled rendering or using non-default pixel formats)
  simgui_setup(&(simgui_desc_t){
      .logger.func = slog_func,
  });

  /* initialize application state */
  state = (state_t){
      .show_test_window = true,
      .pass_action = {.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                                    .clear_value = {0.7f, 0.5f, 0.0f, 1.0f}}}};
}

void frame(void) {
  const int width = sapp_width();
  const int height = sapp_height();
  simgui_new_frame(&(simgui_frame_desc_t){.width = width,
                                          .height = height,
                                          .delta_time = sapp_frame_duration(),
                                          .dpi_scale = sapp_dpi_scale()});

  // 1. Show a simple window
  // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a
  // window automatically called "Debug"
  static float f = 0.0f;
  igText("Hello, world!");
  igSliderFloatEx("float", &f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
  igColorEdit3("clear color", (float*)&state.pass_action.colors[0].clear_value,
               0);
  if (igButton("Test Window"))
    state.show_test_window ^= 1;
  if (igButton("Another Window"))
    state.show_another_window ^= 1;
  igText("Application average %.3f ms/frame (%.1f FPS)",
         1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);

  // 2. Show another simple window, this time using an explicit Begin/End pair
  if (state.show_another_window) {
    igSetNextWindowSize((ImVec2){200, 100}, ImGuiCond_FirstUseEver);
    igBegin("Another Window", &state.show_another_window, 0);
    igText("Hello");
    igEnd();
  }

  // 3. Show the ImGui test window. Most of the sample code is in
  // ImGui::ShowDemoWindow()
  if (state.show_test_window) {
    igSetNextWindowPos((ImVec2){460, 20}, ImGuiCond_FirstUseEver);
    igShowDemoWindow(0);
  }

  // the sokol_gfx draw pass
  sg_begin_pass(
      &(sg_pass){.action = state.pass_action, .swapchain = sglue_swapchain()});
  simgui_render();
  sg_end_pass();
  sg_commit();
}

void cleanup(void) {
  simgui_shutdown();
  sg_shutdown();
}

void input(const sapp_event* event) {
  simgui_handle_event(event);
}

sapp_desc sokol_main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .event_cb = input,
      .width = 1024,
      .height = 768,
      .window_title = "cimgui (sokol-app)",
      .ios_keyboard_resizes_canvas = false,
      .icon.sokol_default = true,
      .enable_clipboard = true,
      .logger.func = slog_func,
  };
}
