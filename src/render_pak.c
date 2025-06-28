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

static constexpr const char txt[] = "press '?' for help";
static constexpr i32 txt_len = 18;

static struct nk_color BKG = {0, 0, 0, 180};
static struct nk_color TXT = {255, 255, 255, 255};
static struct nk_color BRD = {255, 255, 255, 255};

static struct nk_rect win_max() {
  f32 w = (f32)sapp_width();
  f32 h = (f32)sapp_height();

  return nk_rect(0, 0, w, h);
}

void render_pak(state* s) {
  s->ctxui = snk_new_frame();
  nk_style_hide_cursor(s->ctxui);

  contextui* ctx = s->ctxui;
  const struct nk_user_font* fnt = ctx->style.font;

  if (nk_begin(ctx, "", win_max(), NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_space_begin(ctx, NK_STATIC, sapp_height(), 2);
    nk_layout_space_push(ctx, win_max());
    nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
    nk_layout_space_end(ctx);

    struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
    nk_fill_rect(canvas, nk_rect(5, 5, 135, 22), 1, BKG);
    nk_draw_text(canvas, nk_rect(10, 10, 200, 20), txt, txt_len, fnt, BRD, TXT);
  }
  nk_end(ctx);

  sg_begin_pass(&(sg_pass){
      .action = s->ctx3d->pass_action,
      .swapchain = sglue_swapchain(),
  });
  snk_render(sapp_width(), sapp_height());
  sg_end_pass();
  sg_commit();
}
