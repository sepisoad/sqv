#include <stdbool.h>
#include <stdio.h>
#include "../deps/sokol_app.h"
#include "../deps/sokol_gfx.h"
#include "../deps/nuklear.h"
#include "../deps/sokol_nuklear.h"
#include "../deps/sokol_glue.h"

#include "quake/md1.h"
#include "app.h"

void draw_ui(state* s) {
  s->ctxui = snk_new_frame();
  nk_style_hide_cursor(s->ctxui);

  struct nk_style_window default_window_style = s->ctxui->style.window;
  struct nk_vec2 default_spacing = s->ctxui->style.window.spacing;
  s->ctxui->style.window.padding = nk_vec2(0, 0);
  s->ctxui->style.window.spacing = nk_vec2(0, 0);

  if (nk_begin(s->ctxui, "sqv - sepi's quake md1 viewer",
               nk_rect(0, 0, sapp_width(), sapp_height()),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(s->ctxui, sapp_height(), 1);
    nk_image(s->ctxui, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
  }
  nk_end(s->ctxui);

  s->ctxui->style.window.padding = default_window_style.padding;
  s->ctxui->style.window.spacing = default_spacing;

  sg_begin_pass(&(sg_pass){.action = s->ctx3d->pass_action,
                           .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());
  sg_end_pass();
  sg_commit();
}
