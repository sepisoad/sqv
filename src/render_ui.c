// #include <stdbool.h>
// #include <stdio.h>

// #include "../deps/nuklear.h"
// #include "../deps/sepi_types.h"
// #include "../deps/sokol_app.h"
// #include "../deps/sokol_gfx.h"
// #include "../deps/sokol_glue.h"
// #include "../deps/sokol_nuklear.h"

// #include "app.h"
// #include "md1.h"

// void set_skin(u32 idx);

// static struct nk_color BKG = {0, 0, 0, 180};
// static struct nk_color TXT = {255, 255, 255, 255};
// static struct nk_color BRD = {255, 255, 255, 255};

// struct nk_rect win_max();
// struct nk_rect view_max();

// static void draw_init_mode(state* s) {
//   cstr txt = "press '?' for help";
//   i32 len = strlen(txt);
//   contextui* ctx = s->ctxui;
//   const struct nk_user_font* fnt = ctx->style.font;

//   if (nk_begin(ctx, "", win_max(), NK_WINDOW_NO_SCROLLBAR)) {
//     nk_layout_space_begin(ctx, NK_STATIC, sapp_height(), 2);
//     nk_layout_space_push(ctx, win_max());
//     nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
//     nk_layout_space_end(ctx);

//     struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
//     nk_fill_rect(canvas, nk_rect(5, 5, 135, 22), 1, BKG);
//     nk_draw_text(canvas, nk_rect(10, 10, 200, 20), txt, len, fnt, BRD, TXT);
//   }
//   nk_end(ctx);
// }

// static void draw_normal_mode(state* s) {
//   contextui* ctx = s->ctxui;

//   if (nk_begin(ctx, "sqv - sepi's quake md1 viewer",
//                nk_rect(0, 0, sapp_width(), sapp_height()),
//                NK_WINDOW_NO_SCROLLBAR)) {
//     nk_layout_row_dynamic(ctx, sapp_height(), 1);
//     nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
//   }
//   nk_end(ctx);
// }

// static void draw_info_mode(state* s) {
//   f32 w = (f32)sapp_width();
//   f32 h = (f32)sapp_height();
//   struct nk_context* ctx = s->ctxui;

//   struct nk_style_window old_style = ctx->style.window;
//   ctx->style.window.fixed_background = nk_style_item_color(BKG);
//   ctx->style.window.popup_border_color = BRD;

//   if (nk_begin(ctx, "", nk_rect(0, 0, w, h), NK_WINDOW_NO_SCROLLBAR)) {
//     nk_layout_space_begin(ctx, NK_STATIC, h, 1);
//     nk_layout_space_push(ctx, nk_rect(0, 0, w, h));
//     nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
//     nk_layout_space_end(ctx);

//     md1_header* hdr = &s->mdl.header;
//     char buf[256] = {0};
//     if (nk_popup_begin(ctx, NK_POPUP_STATIC, "", NK_WINDOW_NO_SCROLLBAR,
//                        view_max())) {
//       nk_layout_row_dynamic(ctx, 12, 2);
//       nk_label_colored(ctx, "KEY", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "VALUE", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "------", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "------", NK_TEXT_LEFT, TXT);

//       sprintf(buf, "%u", hdr->vertices_length);
//       nk_label_colored(ctx, "vertices", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, buf, NK_TEXT_LEFT, TXT);
//       memset(buf, 0, 256);

//       sprintf(buf, "%u", hdr->triangles_length);
//       nk_label_colored(ctx, "triangles", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, buf, NK_TEXT_LEFT, TXT);
//       memset(buf, 0, 256);

//       sprintf(buf, "%u", hdr->frames_length);
//       nk_label_colored(ctx, "frames", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, buf, NK_TEXT_LEFT, TXT);
//       memset(buf, 0, 256);

//       sprintf(buf, "%u", hdr->poses_length);
//       nk_label_colored(ctx, "poses", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, buf, NK_TEXT_LEFT, TXT);
//       memset(buf, 0, 256);

//       sprintf(buf, "%u", hdr->skins_length);
//       nk_label_colored(ctx, "skins", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, buf, NK_TEXT_LEFT, TXT);
//       memset(buf, 0, 256);

//       nk_popup_end(ctx);
//     }
//   }
//   nk_end(ctx);
//   ctx->style.window = old_style;
// }

// static void draw_help_mode(state* s) {
//   f32 w = (f32)sapp_width();
//   f32 h = (f32)sapp_height();
//   contextui* ctx = s->ctxui;

//   struct nk_style_window old_style = ctx->style.window;
//   ctx->style.window.fixed_background = nk_style_item_color(BKG);
//   ctx->style.window.popup_border_color = BRD;

//   if (nk_begin(ctx, "", win_max(), NK_WINDOW_NO_SCROLLBAR)) {
//     nk_layout_space_begin(ctx, NK_STATIC, h, 1);
//     nk_layout_space_push(ctx, nk_rect(0, 0, w, h));
//     nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
//     nk_layout_space_end(ctx);

//     if (nk_popup_begin(ctx, NK_POPUP_STATIC, "", NK_WINDOW_NO_SCROLLBAR,
//                        view_max())) {
//       nk_layout_row_dynamic(ctx, 12, 3);
//       nk_label_colored(ctx, "INPUT", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "COMMAND", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "DESCRIPTION", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "------", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "------", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "------", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "[ESC]", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":normal", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "sets ui back to normal mode", NK_TEXT_LEFT,
//       TXT);

//       nk_label_colored(ctx, "?", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":help", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "shows this help", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "i", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":info", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "shows model information", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "s", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":skins", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "shows skins", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "p", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":poses", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "shows poses", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, ">", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":next-pose", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "goes to next pose", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "<", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":prev-pose", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "goes to previous pose", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "a", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":!animate", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "toggles animation", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "r", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":!auto-rotate", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "toggles auto rotation", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "mouse wheel up", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":zoom-in [N]", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "zooms in", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "mouse wheel down", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":zoom-out [N]", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "zooms out", NK_TEXT_LEFT, TXT);

//       nk_label_colored(ctx, "N/A", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, ":quit", NK_TEXT_LEFT, TXT);
//       nk_label_colored(ctx, "quits application", NK_TEXT_LEFT, TXT);

//       nk_popup_end(ctx);
//     }
//   }
//   nk_end(ctx);
//   ctx->style.window = old_style;
// }

// static void draw_skins_mode(state* s) {
//   f32 w = (f32)sapp_width();
//   f32 h = (f32)sapp_height();
//   f32 iw = s->mdl.header.skin_width;
//   f32 ih = s->mdl.header.skin_height;
//   struct nk_context* ctx = s->ctxui;

//   struct nk_style_window old_window_style = ctx->style.window;
//   struct nk_style_button old_button_style = ctx->style.button;
//   ctx->style.window.fixed_background = nk_style_item_color(BKG);
//   ctx->style.window.popup_border_color = BRD;
//   ctx->style.button.padding = nk_vec2(0, 0);
//   ctx->style.button.image_padding = nk_vec2(0, 0);
//   ctx->style.button.touch_padding = nk_vec2(0, 0);
//   ctx->style.button.border = 2.5;
//   ctx->style.button.rounding = 0;

//   if (nk_begin(ctx, "model", nk_rect(0, 0, w, h),
//                NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
//     nk_layout_space_begin(ctx, NK_STATIC, h, 1);
//     nk_layout_space_push(ctx, nk_rect(0, 0, w, h));
//     nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
//     nk_layout_space_end(ctx);
//   }
//   nk_end(ctx);

//   if (nk_begin(ctx, "skins", nk_rect(5, 5, iw + 10, h - 10),
//                NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
//     nk_layout_row_dynamic(ctx, 15, 1);
//     nk_label(ctx, "SKINS", NK_TEXT_CENTERED);

//     nk_layout_row_dynamic(ctx, ih, 1);

//     for (u32 i = 0; i < s->mdl.header.skins_length; i++) {
//       if (nk_button_image(
//               ctx, nk_image_handle(snk_nkhandle(s->mdl.skins[i].ui_image))))
//               {
//         set_skin(i);
//       }
//     }
//   }
//   nk_end(ctx);

//   ctx->style.window = old_window_style;
//   ctx->style.button = old_button_style;
// }

// static void draw_poses_mode(state* s) {
//   f32 w = (f32)sapp_width();
//   f32 h = (f32)sapp_height();
//   struct nk_context* ctx = s->ctxui;

//   struct nk_style_window old_window_style = ctx->style.window;
//   struct nk_style_button old_button_style = ctx->style.button;
//   ctx->style.window.fixed_background = nk_style_item_color(BKG);
//   ctx->style.window.popup_border_color = BRD;
//   ctx->style.button.padding = nk_vec2(0, 0);
//   ctx->style.button.border = 2.5;
//   ctx->style.button.rounding = 0;

//   struct nk_style_button cur_sty = ctx->style.button;
//   cur_sty.normal.data.color = (struct nk_color){100, 0, 0, 255};

//   if (nk_begin(ctx, "model", nk_rect(0, 0, w, h),
//                NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
//     nk_layout_space_begin(ctx, NK_STATIC, h, 1);
//     nk_layout_space_push(ctx, nk_rect(0, 0, w, h));
//     nk_image(ctx, nk_image_handle(snk_nkhandle(s->ctx3d->nk_img)));
//     nk_layout_space_end(ctx);
//   }
//   nk_end(ctx);

//   if (nk_begin(
//           ctx, "poses", nk_rect(5, 5, 100, h - 10),
//           NK_WINDOW_MOVABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_SCALABLE)) {
//     nk_layout_row_dynamic(ctx, 15, 1);
//     nk_label(ctx, "POSES", NK_TEXT_CENTERED);

//     char number[16] = {0};
//     nk_layout_row_dynamic(ctx, 15, 2);
//     for (u32 i = 0; i < s->mdl.header.poses_length; i++) {
//       sprintf(number, "%u", s->mdl.poses[i].frames_length);
//       nk_label(ctx, number, NK_TEXT_CENTERED);

//       if (i == s->mdl_pos) {
//         nk_button_label_styled(ctx, &cur_sty, s->mdl.poses[i].name);
//       } else {
//         if (nk_button_label(ctx, s->mdl.poses[i].name)) {
//           s->mdl_pos = i;
//           s->mdl_frm = 0;
//         }
//       }
//     }
//   }
//   nk_end(ctx);

//   ctx->style.window = old_window_style;
//   ctx->style.button = old_button_style;
// }

// void render_ui(state* s) {
//   s->ctxui = snk_new_frame();
//   nk_style_hide_cursor(s->ctxui);

//   sg_begin_pass(&(sg_pass){.action = s->ctx3d->pass_action,
//                            .swapchain = sglue_swapchain()});
//   snk_render(sapp_width(), sapp_height());
//   sg_end_pass();
//   sg_commit();
// }
