#ifndef SQV_APP_H
#define SQV_APP_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#include "../dep/nuklear/nuklear.h"
#include "../dep/nuklear/nuklear_sdl_gl3.h"

typedef struct app_t {
  SDL_Window *window;
  SDL_GLContext gl_ctx;
  struct nk_context* nk_ctx;
  struct nk_colorf bg;
  uint32_t width;
  uint32_t height;
} app_t;

#endif //SQV_APP_H
