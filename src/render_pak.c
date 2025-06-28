#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../deps/nuklear.h"
#include "../deps/sepi_types.h"  // IWYU pragma: keep
#include "../deps/sokol_app.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_nuklear.h"

#include "app.h"

struct nk_rect win_max();
void draw_help(state* s);

static void draw_default(state* s) {
  contextui* ctx = s->ctxui;
  if (nk_begin(ctx, "", win_max(), NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx, 15 /*auto*/, 1);
    nk_label(ctx, "PAK MODE", NK_TEXT_LEFT);
  }
  nk_end(ctx);
}

void render_pak(state* s) {
  s->ctxui = snk_new_frame();
  nk_style_hide_cursor(s->ctxui);

  if (s->mnm & MINOR_MODE_HELP) {
    draw_help(s);
  } else if (s->mnm & MINOR_MODE_COMMANDLINE) {
    // draw_help(s);
  } else {
    draw_default(s);
  }

  sg_begin_pass(&(sg_pass){.action = s->ctx3d->pass_action,
                           .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());
  sg_end_pass();
  sg_commit();
}
