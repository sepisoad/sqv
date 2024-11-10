#include <assert.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "error.h"

typedef struct app_t {
  SDL_Window* window;
  SDL_GLContext gl_context;
  GLuint shader_program;
} app_t;

sqv_error_t sqv_app_init(app_t* app);
sqv_error_t sqv_app_clean(app_t* app);