#include "deps/nuklear/nuklear.h"
#include "deps/sokol/sokol_app.h"
#include "deps/sokol/sokol_gfx.h"
#include "deps/sokol/sokol_glue.h"
#include "deps/sokol/sokol_nuklear.h"

#include "state.h"

struct nk_rect win_max();
void draw_help(State* s);
void draw_commandline(State* s);

static void draw_default(State* s) {
  ContextUI* ctx = s->ctxui;

  nk_layout_row_dynamic(ctx, 15 /*auto*/, 1);
  nk_label(ctx, "Press '?' to see help instructions", NK_TEXT_LEFT);
  nk_label(ctx, "Press ':' to open command line", NK_TEXT_LEFT);
  nk_label(ctx, "Drag a quake file to this window to see it's content",
           NK_TEXT_LEFT);
}

void render_init(State* s) {
  s->ctxui = snk_new_frame();
  ContextUI* ctx = s->ctxui;

  nk_style_hide_cursor(ctx);
  if (nk_begin(ctx, "", win_max(), NK_WINDOW_NO_SCROLLBAR)) {
    if (s->mnm & MINOR_MODE_HELP) {
      draw_help(s);
    } else {
      draw_default(s);
    }

    if (s->show_cmd) {
      draw_commandline(s);
    }
  }
  nk_end(ctx);

  sg_begin_pass(&(sg_pass){.action = s->ctx3d->pass_action,
                           .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());
  sg_end_pass();
  sg_commit();
}
