#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "deps/sepi/base.h"
#include "deps/nuklear/nuklear.h"
#include "deps/sokol/sokol_app.h"
#include "deps/sokol/sokol_gfx.h"
#include "deps/sokol/sokol_glue.h"
#include "deps/sokol/sokol_nuklear.h"

#include "app.h"

struct nk_rect win_max();
void draw_help(state* s);
void draw_commandline(state* s);

static void draw_default(state* s) {
  contextui* ctx = s->ctxui;

  static float ratio_two[] = {0.2f, 0.6f, 0.2f};
  nk_layout_row(ctx, NK_STATIC, 1, 3, ratio_two);
  if (nk_tree_push(ctx, NK_TREE_NODE, "Notebook", NK_MINIMIZED)) {
    nk_layout_row_dynamic(ctx, 0, 1);
    nk_label(ctx, "Label aligned left", NK_TEXT_LEFT);
    nk_label(ctx, "Label aligned centered", NK_TEXT_LEFT);
    nk_label(ctx, "Label aligned right", NK_TEXT_LEFT);
    nk_tree_pop(ctx);
  }
  nk_button_label(ctx, "button");
  nk_button_label(ctx, "button");
}

void render_pak(state* s) {
  s->ctxui = snk_new_frame();
  contextui* ctx = s->ctxui;

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
