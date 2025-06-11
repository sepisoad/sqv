#include <stdbool.h>
#include <stdio.h>

#include "../deps/hmm.h"
#include "../deps/log.h"
#include "../deps/nuklear.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_args.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_nuklear.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_log.h"
#include "../deps/sepi_types.h"
#include "../deps/sepi_io.h"

#include "glsl_default.h"
#include "md1.h"
#include "app.h"

#ifdef DEBUG
#include "../deps/sepi_alloc.h"
#endif

extern context3d ctx3d;

void reset_state();
void load_3d_model(cstr path, md1_model* m);
void unload_3d_model(md1_model* m);

void update_offscreen_target(state* s, int width, int height) {
  snk_destroy_image(s->ctx3d->nk_img);
  sg_destroy_attachments(s->ctx3d->atts);
  sg_destroy_image(s->ctx3d->depth_img);
  sg_destroy_image(s->ctx3d->color_img);

  s->ctx3d->width = width > 0 ? width : DEFAULT_WIDTH;
  s->ctx3d->height = height > 0 ? height : DEFAULT_HEIGHT;

  s->ctx3d->color_img = sg_make_image(&(sg_image_desc){
      .usage.render_attachment = true,
      .width = s->ctx3d->width,
      .height = s->ctx3d->height,
      .sample_count = 1,
  });

  s->ctx3d->depth_img = sg_make_image(&(sg_image_desc){
      .usage.render_attachment = true,
      .width = s->ctx3d->width,
      .height = s->ctx3d->height,
      .pixel_format = SG_PIXELFORMAT_DEPTH,
      .sample_count = 1,
  });

  s->ctx3d->atts = sg_make_attachments(&(sg_attachments_desc){
      .colors[0].image = s->ctx3d->color_img,
      .depth_stencil.image = s->ctx3d->depth_img,
  });

  s->ctx3d->nk_img = snk_make_image(&(snk_image_desc_t){
      .image = s->ctx3d->color_img,
      .sampler = s->ctx3d->sampler,
  });

  s->ctx3d->pass_action = (sg_pass_action){
      .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.25f, 0.5f, 0.75f, 1.0f}},
  };
}

void create_offscreen_target(state* s, cstr path) {
  if (s->ctx3d != NULL) {
    unload_3d_model(&s->mdl);
    sg_destroy_pipeline(s->pip);
    sg_destroy_shader(s->shd);
    reset_state();
    update_offscreen_target(s, sapp_width(), sapp_height());
  }

  load_3d_model(path, &s->mdl);
  s->ctx3d = &ctx3d;  // resetting

  const f32* vb = NULL;
  u32 vb_len = 0;
  md1_get_vertices(&s->mdl, s->mdl_pos, s->mdl_frm, &vb, &vb_len);

  s->shd = sg_make_shader(cube_shader_desc(sg_query_backend()));
  s->pip = sg_make_pipeline(&(sg_pipeline_desc){
      .layout =
          {
              .attrs =
                  {
                      [ATTR_cube_position].format = SG_VERTEXFORMAT_FLOAT3,
                      [ATTR_cube_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
                  },
          },
      .shader = s->shd,
      .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
      .cull_mode = SG_CULLMODE_BACK,
      .depth = {
          .write_enabled = true,
          .compare = SG_COMPAREFUNC_LESS_EQUAL,
          .pixel_format = SG_PIXELFORMAT_DEPTH,
      }});

  s->bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .size = (size_t)vb_len * sizeof(f32),
      .usage = (sg_buffer_usage){.stream_update = true},
  });
  s->bind.images[IMG_tex] = s->mdl.skins[s->mdl_skn].image;
  s->bind.samplers[SMP_smp] = s->mdl.skins[s->mdl_skn].sampler;

  s->ctx3d->sampler = sg_make_sampler(&(sg_sampler_desc){
      .min_filter = SG_FILTER_LINEAR,
      .mag_filter = SG_FILTER_LINEAR,
      .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
      .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
  });

  update_offscreen_target(s, sapp_width(), sapp_height());
  s->pass_action = (sg_pass_action){
      .colors[0] =
          {
              .load_action = SG_LOADACTION_CLEAR,
              .clear_value = {0.1f, 0.1f, 0.1f, 1.0f},
          },
  };
}

void load_3d_model(cstr path, md1_model* m) {
  notnull(path);
  notnull(m);

  u8* bf = NULL;
  sz bfsz = notzero(sepi_io_load_file(path, &bf));

  md1_error err = md1_load(bf, bfsz, m);
  makesure(err == MD1_ERR_SUCCESS, "qk_load_mdl failed");
  free(bf);
}

void unload_3d_model(md1_model* m) {
  notnull(m);
  md1_unload(m);
}

// SEPI: this function is called in the main loop so we should avoid unnecessary
// operations
void draw_3d(state* s) {
  md1_model* m = &s->mdl;
  hmm_v3* bbmin = &m->header.bbox_min;
  hmm_v3* bbmax = &m->header.bbox_max;
  hmm_vec3 center = HMM_MultiplyVec3f(HMM_AddVec3(*bbmin, *bbmax), 0.5f);
  f32 dx = bbmax->X - bbmin->X;
  f32 dy = bbmax->Y - bbmin->Y;
  f32 dz = bbmax->Z - bbmin->Z;
  f32 rad = 0.5f * sqrtf(dx * (dx * s->zoom) + dy * dy + dz * dz);

  f32 aspect = sapp_widthf() / sapp_heightf();
  f32 dist = (rad / sinf(HMM_ToRadians(FOV) * 0.5f)) * 1.5f;

  hmm_vec3 eye = HMM_AddVec3(center, HMM_Vec3(0.0f, 0.0f, dist));
  hmm_vec3 up = HMM_Vec3(0.0f, 1.0f, 0.0f);

  hmm_mat4 proj = HMM_Perspective(FOV, aspect, 0.1f, dist * 4.0f);
  hmm_mat4 view = HMM_LookAt(eye, center, up);
  hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);

  hmm_mat4 rxm = HMM_Rotate(90, HMM_Vec3(1.0f, 0.0f, 0.0f));
  hmm_mat4 rym = HMM_Rotate(0, HMM_Vec3(0.0f, 1.0f, 0.0f));
  hmm_mat4 rzm = HMM_Rotate(s->mdl_roty, HMM_Vec3(0.0f, 0.0f, -1.0f));
  hmm_mat4 rotation = HMM_MultiplyMat4(HMM_MultiplyMat4(rxm, rym), rzm);

  hmm_mat4 model = HMM_MultiplyMat4(
      HMM_Translate(center),
      HMM_MultiplyMat4(rotation,
                       HMM_Translate(HMM_MultiplyVec3f(center, -1.0f))));

  vs_params_t vs_params = {
      .mvp = HMM_MultiplyMat4(view_proj, model),
  };

  const f32* vb = NULL;
  u32 vb_len = 0;

  md1_get_vertices(m, s->mdl_pos, s->mdl_frm, &vb, &vb_len);

  sg_update_buffer(
      s->bind.vertex_buffers[0],
      &(sg_range){.ptr = vb, .size = (size_t)vb_len * sizeof(f32)});

  sg_begin_pass(&(sg_pass){.action = s->ctx3d->pass_action,
                           .attachments = s->ctx3d->atts});
  sg_apply_pipeline(s->pip);
  sg_apply_bindings(&s->bind);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, vb_len / 5, 1);
  sg_end_pass();
}
