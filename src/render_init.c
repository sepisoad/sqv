#include "../deps/nuklear.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_nuklear.h"

#include "app.h"

void render_init(state* s) {
  s->ctxui = snk_new_frame();
  nk_style_hide_cursor(s->ctxui);

  struct nk_style_window default_window_style = s->ctxui->style.window;
  struct nk_vec2 default_spacing = s->ctxui->style.window.spacing;
  s->ctxui->style.window.padding = nk_vec2(0, 0);
  s->ctxui->style.window.spacing = nk_vec2(0, 0);

  // TODO:
  // switch (s->mjm) {
  // case MAJOR_MODE_PAK:
  //   draw_init_mode(s);
  //   break;
  // case MAJOR_MODE_MD1:
  //   draw_normal_mode(s);
  //   break;
  // case MAJOR_MODE_WAD:
  //   draw_info_mode(s);
  //   break;
  // case MAJOR_MODE_LMP:
  //   draw_help_mode(s);
  //   break;
  // case MAJOR_MODE_SKINS:
  //   draw_skins_mode(s);
  //   break;
  // case MAJOR_MODE_POSES:
  //   draw_poses_mode(s);
  //   break;
  // default:
  //   log_warn("this mode should have not happened!");
  //   break;
  // }

  s->ctxui->style.window.padding = default_window_style.padding;
  s->ctxui->style.window.spacing = default_spacing;

  sg_begin_pass(&(sg_pass){.action = s->ctx3d->pass_action,
                           .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());
  sg_end_pass();
  sg_commit();
}
