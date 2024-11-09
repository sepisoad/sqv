#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "app.h"

static const char* TITLE = "SQV - quake models viewer";
static const uint32_t SDL_INIT_FLAGS = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
static const uint32_t SDL_WIN_FLAGS = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
static const uint32_t OPENGL_MAJOR_VERSION = 3;
static const uint32_t OPENGL_MINOR_VERSION = 3;

app_t* setup_ui(uint32_t width, uint32_t height) {
  app_t* app = NULL;
  bool success = false;
  // ===
  do {
    app = (app_t*)malloc(sizeof(app_t));
    if (!app) goto cleanup;

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    SDL_Init(SDL_INIT_FLAGS);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    app->window = SDL_CreateWindow(TITLE,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WIN_FLAGS);

    app->gl_ctx = SDL_GL_CreateContext(app->window);

    SDL_GetWindowSize(app->window, &width, &height);
    app->width = width;
    app->height = height;

    glViewport(0, 0, width, height);

    glewExperimental = 1;
    if (glewInit() != GLEW_OK) goto cleanup;

    app->nk_ctx = nk_sdl_init(app->window);

    {
      struct nk_font_atlas* atlas;
      nk_sdl_font_stash_begin(&atlas);
      nk_sdl_font_stash_end();
    }

    app->bg.r = 0.10f;
    app->bg.g = 0.18f;
    app->bg.b = 0.24f;
    app->bg.a = 1.0f;

    success = true;
  } while (false);
  // ===

cleanup:
  if (!success) {
    if (app) free(app);
    app = NULL;
  }

  return app;
}

void kill_ui(app_t* app) {
  free(app);
}