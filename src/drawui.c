#include <stdbool.h>
#include <stdio.h>
#include "../deps/sokol_app.h"
#include "../deps/sokol_gfx.h"
#include "../deps/nuklear.h"
#include "../deps/sokol_nuklear.h"
#include "../deps/sokol_glue.h"

#include "quake/md1.h"
#include "app.h"

static void draw_init_mode(state* s) {
  i32 w = sapp_width();
  i32 h = sapp_height();
  cstr txt = "press '?' for help";

  if (nk_begin(s->ctxui, "Overlay", nk_rect(0, 0, w, h),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_space_begin(s->ctxui, NK_STATIC, sapp_height(), 2);
    nk_layout_space_push(s->ctxui, nk_rect(0, 0, w, h));
    nk_image(s->ctxui, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
    nk_layout_space_end(s->ctxui);

    struct nk_command_buffer* canvas = nk_window_get_canvas(s->ctxui);
    nk_fill_rect(canvas, nk_rect(5, 5, 135, 22), 0, nk_rgba(0, 0, 0, 180));
    nk_draw_text(canvas, nk_rect(10, 10, 200, 20), txt, strlen(txt),
                 s->ctxui->style.font, nk_rgba(0, 0, 0, 0),
                 nk_rgba(255, 255, 255, 255));
  }
  nk_end(s->ctxui);
}

static void draw_normal_mode(state* s) {
  if (nk_begin(s->ctxui, "sqv - sepi's quake md1 viewer",
               nk_rect(0, 0, sapp_width(), sapp_height()),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(s->ctxui, sapp_height(), 1);
    nk_image(s->ctxui, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
  }
  nk_end(s->ctxui);
}

static void draw_help_mode(state* s) {
  f32 w = (f32)sapp_width();
  f32 h = (f32)sapp_height();

  struct nk_context* ctx = s->ctxui;
  struct nk_style_window old_style = ctx->style.window;
  ctx->style.window.fixed_background =
      nk_style_item_color(nk_rgba(0, 0, 0, 180));
  ctx->style.window.popup_border_color = nk_rgba(0, 0, 0, 0);
  ctx->style.window.border_color = nk_rgba(255, 255, 255, 255);

  if (nk_begin(ctx, "", nk_rect(0, 0, w, h), NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_space_begin(ctx, NK_STATIC, h, 1);
    nk_layout_space_push(ctx, nk_rect(0, 0, w, h));
    nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
    nk_layout_space_end(ctx);

    struct nk_rect rect = {5, 5, w - 10, h - 10};
    struct nk_color txtcol = {255, 255, 255, 255};
    char buf[256] = {0};
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "", NK_WINDOW_NO_SCROLLBAR,
                       rect)) {
      nk_layout_row_dynamic(ctx, 12, 3);
      nk_label_colored(ctx, "INPUT", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "COMMAND", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "DESCRIPTION", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "------", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "------", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "------", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "[ESC]", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":normal", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "sets ui back to normal mode", NK_TEXT_LEFT,
                       txtcol);

      nk_label_colored(ctx, "?", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":help", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "shows this help", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "i", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":info", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "shows model information", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "s", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":skins", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "shows skins", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "p", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":poses", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "shows poses", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, ">", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":next-pose", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "goes to next pose", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "<", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":prev-pose", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "goes to previous pose", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "a", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":!animate", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "toggles animation", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "r", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":!auto-rotate", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "toggles auto rotation", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "mouse wheel up", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":zoom-in [N]", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "zooms in", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "mouse wheel down", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":zoom-out [N]", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "zooms out", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "N/A", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, ":quit", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "quits application", NK_TEXT_LEFT, txtcol);

      nk_popup_end(ctx);
    }
  }
  nk_end(ctx);
  ctx->style.window = old_style;
}

static void draw_info_mode(state* s) {
  f32 w = (f32)sapp_width();
  f32 h = (f32)sapp_height();

  struct nk_context* ctx = s->ctxui;
  struct nk_style_window old_style = ctx->style.window;
  ctx->style.window.fixed_background =
      nk_style_item_color(nk_rgba(0, 0, 0, 180));
  ctx->style.window.popup_border_color = nk_rgba(0, 0, 0, 0);
  ctx->style.window.border_color = nk_rgba(255, 255, 255, 255);

  if (nk_begin(ctx, "", nk_rect(0, 0, w, h), NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_space_begin(ctx, NK_STATIC, h, 1);
    nk_layout_space_push(ctx, nk_rect(0, 0, w, h));
    nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
    nk_layout_space_end(ctx);

    qk_header* hdr = &s->mdl.header;
    struct nk_rect rect = {5, 5, w - 10, h - 10};
    struct nk_color txtcol = {255, 255, 255, 255};
    char buf[256] = {0};
    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "", NK_WINDOW_NO_SCROLLBAR,
                       rect)) {
      nk_layout_row_dynamic(ctx, 12, 2);
      nk_label_colored(ctx, "KEY", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "VALUE", NK_TEXT_LEFT, txtcol);

      nk_label_colored(ctx, "------", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, "------", NK_TEXT_LEFT, txtcol);

      sprintf(buf, "%u", hdr->vertices_length);
      nk_label_colored(ctx, "vertices", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, buf, NK_TEXT_LEFT, txtcol);
      memset(buf, 0, 256);

      sprintf(buf, "%u", hdr->triangles_length);
      nk_label_colored(ctx, "triangles", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, buf, NK_TEXT_LEFT, txtcol);
      memset(buf, 0, 256);

      sprintf(buf, "%u", hdr->frames_length);
      nk_label_colored(ctx, "frames", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, buf, NK_TEXT_LEFT, txtcol);
      memset(buf, 0, 256);

      sprintf(buf, "%u", hdr->poses_length);
      nk_label_colored(ctx, "poses", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, buf, NK_TEXT_LEFT, txtcol);
      memset(buf, 0, 256);

      sprintf(buf, "%u", hdr->skins_length);
      nk_label_colored(ctx, "skins", NK_TEXT_LEFT, txtcol);
      nk_label_colored(ctx, buf, NK_TEXT_LEFT, txtcol);
      memset(buf, 0, 256);

      nk_popup_end(ctx);
    }
  }
  nk_end(ctx);
  ctx->style.window = old_style;
}

void draw_ui(state* s) {
  s->ctxui = snk_new_frame();
  nk_style_hide_cursor(s->ctxui);

  struct nk_style_window default_window_style = s->ctxui->style.window;
  struct nk_vec2 default_spacing = s->ctxui->style.window.spacing;
  s->ctxui->style.window.padding = nk_vec2(0, 0);
  s->ctxui->style.window.spacing = nk_vec2(0, 0);

  switch (s->m) {
    case MODE_INIT:
      draw_init_mode(s);
      break;
    case MODE_NORMAL:
      draw_normal_mode(s);
      break;
    case MODE_HELP:
      draw_help_mode(s);
      break;
    case MODE_INFO:
      draw_info_mode(s);
      break;
    default:
      log_warn("this mode should have not happened!");
      break;
  }

  s->ctxui->style.window.padding = default_window_style.padding;
  s->ctxui->style.window.spacing = default_spacing;

  sg_begin_pass(&(sg_pass){.action = s->ctx3d->pass_action,
                           .swapchain = sglue_swapchain()});
  snk_render(sapp_width(), sapp_height());
  sg_end_pass();
  sg_commit();
}
