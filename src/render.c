#include <stdio.h>
#include <stdbool.h>

#include "app.h"

static const char* TITLE_TMP = "FUCK";
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024


int32_t render_ui(app_t* app) {
  bool running = true;
  while (running) {
    /* Input */
    SDL_Event evt;
    nk_input_begin(app->nk_ctx);
    while (SDL_PollEvent(&evt)) {
      if (evt.type == SDL_QUIT) goto cleanup;
      nk_sdl_handle_event(&evt);
    }
    nk_sdl_handle_grab(); /* optional grabbing behavior */
    nk_input_end(app->nk_ctx);

    /// TODO: clearn up
    if (nk_begin( app->nk_ctx, TITLE_TMP, nk_rect(0, 0, app->width, app->height),
      NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
      NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)){
      enum { EASY, HARD };
      static int op = EASY;
      static int property = 20;

      nk_layout_row_static(app->nk_ctx, 30, 80, 1);
      if (nk_button_label(app->nk_ctx, "button"))
        printf("button pressed!\n");
      nk_layout_row_dynamic(app->nk_ctx, 30, 2);
      if (nk_option_label(app->nk_ctx, "easy", op == EASY)) op = EASY;
      if (nk_option_label(app->nk_ctx, "hard", op == HARD)) op = HARD;
      nk_layout_row_dynamic(app->nk_ctx, 22, 1);
      nk_property_int(app->nk_ctx, "Compression:", 0, &property, 100, 10, 1);

      nk_layout_row_dynamic(app->nk_ctx, 20, 1);
      nk_label(app->nk_ctx, "background:", NK_TEXT_LEFT);
      nk_layout_row_dynamic(app->nk_ctx, 25, 1);
      if (nk_combo_begin_color(app->nk_ctx, nk_rgb_cf(app->bg), nk_vec2(nk_widget_width(app->nk_ctx), 400))) {
        nk_layout_row_dynamic(app->nk_ctx, 120, 1);
        app->bg = nk_color_picker(app->nk_ctx, app->bg, NK_RGBA);
        nk_layout_row_dynamic(app->nk_ctx, 25, 1);
        app->bg.r = nk_propertyf(app->nk_ctx, "#R:", 0, app->bg.r, 1.0f, 0.01f, 0.005f);
        app->bg.g = nk_propertyf(app->nk_ctx, "#G:", 0, app->bg.g, 1.0f, 0.01f, 0.005f);
        app->bg.b = nk_propertyf(app->nk_ctx, "#B:", 0, app->bg.b, 1.0f, 0.01f, 0.005f);
        app->bg.a = nk_propertyf(app->nk_ctx, "#A:", 0, app->bg.a, 1.0f, 0.01f, 0.005f);
        nk_combo_end(app->nk_ctx);
      }
    }
    nk_end(app->nk_ctx);

    /* Draw */
    SDL_GetWindowSize(app->window, &app->width, &app->height);
    glViewport(0, 0, app->width, app->height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(app->bg.r, app->bg.g, app->bg.b, app->bg.a);
    nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
    SDL_GL_SwapWindow(app->window);
  }

cleanup:
  nk_sdl_shutdown();
  SDL_GL_DeleteContext(app->gl_ctx);
  SDL_DestroyWindow(app->window);
  SDL_Quit();
  return 0;
}