#include "../deps/nuklear.h"
#include "../deps/sokol_app.h"
#include "../deps/sokol_gfx.h"
#include "../deps/sokol_glue.h"
#include "../deps/sokol_nuklear.h"
#include "../deps/sepi_types.h"

#include "app.h"

// static struct nk_color BKG = {0, 0, 0, 180};
static struct nk_color TXT = {255, 255, 255, 255};
// static struct nk_color BRD = {255, 255, 255, 255};

struct nk_rect win_max() {
  f32 w = (f32)sapp_width();
  f32 h = (f32)sapp_height();
  return nk_rect(0, 0, w, h);
}

struct nk_rect view_max() {
  f32 w = (f32)sapp_width();
  f32 h = (f32)sapp_height();
  return nk_rect(5, 5, w - 10, h - 10);
}

void draw_help(state* s) {
  contextui* ctx = s->ctxui;

  if (nk_begin(ctx, "", win_max(), NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx, 15, 3);
    nk_label_colored(ctx, "INPUT", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "COMMAND", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "DESCRIPTION", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "------", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "------", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "------", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "[ESC]", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":normal", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "sets ui back to normal mode", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "?", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":help", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "shows this help", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "i", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":info", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "shows model information", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "s", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":skins", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "shows skins", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "p", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":poses", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "shows poses", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, ">", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":next-pose", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "goes to next pose", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "<", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":prev-pose", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "goes to previous pose", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "a", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":!animate", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "toggles animation", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "r", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":!auto-rotate", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "toggles auto rotation", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "mouse wheel up", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":zoom-in [N]", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "zooms in", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "mouse wheel down", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":zoom-out [N]", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "zooms out", NK_TEXT_LEFT, TXT);

    nk_label_colored(ctx, "N/A", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, ":quit", NK_TEXT_LEFT, TXT);
    nk_label_colored(ctx, "quits application", NK_TEXT_LEFT, TXT);
  }
  nk_end(ctx);
}

void draw_commandline(state* s) {
  contextui* ctx = s->ctxui;
  f32 w = (f32)sapp_width();
  f32 h = (f32)sapp_height();

  struct nk_rect rect = {0, h - 40, w, h};
  float width_two[] = {27, w - 43};

  if (nk_begin(ctx, "", rect, NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row(ctx, NK_STATIC, 30, 2, width_two);
    nk_label(ctx, "CMD:", NK_TEXT_LEFT);
    nk_edit_focus(ctx, NK_EDIT_ACTIVE);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, s->cmd,
                                   sizeof(s->cmd) - 1, nk_filter_default);
    nk_edit_unfocus(ctx);
  }
  nk_end(ctx);
}
