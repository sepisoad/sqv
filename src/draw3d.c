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

#include "glsl/default.h"
#include "quake/md1.h"
#include "utils/types.h"
#include "app.h"

void load_3d_model(cstr path, qk_model* m) {
  notnull(path);
  notnull(m);

  u8* bf = NULL;
  sz bfsz = notzero(ut_load_file(path, &bf));

  qk_error err = qk_load_mdl(bf, bfsz, m);
  makesure(err == MD1_ERR_SUCCESS, "qk_load_mdl failed");
  // SEPI: if tomorrow you decide not to crash on error and return error then
  // you need to make sure to free the buffer on error, right now the buffer
  // only is freed if all goes well and if shit hits the fan the program
  // intetially terminates
  free(bf);
}

void unload_3d_model(qk_model* m) {
  notnull(m);
  qk_unload_mdl(m);
}

// SEPI: this function is called in the main loop so we should avoid unnecessary
// operations
void draw_3d(state* s) {
  qk_model* m = &s->mdl;
  const f32* vb = NULL;
  u32 vb_len = 0;

  qk_get_frame_vertices(m, s->mdl_pos, s->mdl_frm, &vb, &vb_len);
  hmm_v3* bbmin = &m->header.bbox_min;
  hmm_v3* bbmax = &m->header.bbox_max;
  hmm_vec3 center = HMM_MultiplyVec3f(HMM_AddVec3(*bbmin, *bbmax), 0.5f);
  f32 dx = bbmax->X - bbmin->X;
  f32 dy = bbmax->Y - bbmin->Y;
  f32 dz = bbmax->Z - bbmin->Z;
  f32 rad = 0.5f * sqrtf(dx * (dx * s->zoom) + dy * dy + dz * dz);

  f32 aspect = (f32)s->ctx3d->width / (f32)s->ctx3d->height;
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

  sg_begin_pass(&(sg_pass){.action = s->ctx3d->pass_action,
                           .attachments = s->ctx3d->atts});
  sg_apply_pipeline(s->pip);
  sg_apply_bindings(&s->bind);
  sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
  sg_draw(0, vb_len / 5, 1);
  sg_end_pass();
}
