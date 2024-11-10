#include "app.h"
#include "shader.h"

static const int WINDOW_WIDTH = 400;
static const int WINDOW_HEIGHT = 400;
static const char* WINDOW_TITLE = "sqv - sepi's quake model viewer!";
static const unsigned int WINDOW_INIT_FLAGS = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
static const int GL_MAJOR = 3;
static const int GL_MINOR = 3;
static const char* DEFAULT_VERTEX_SHADER = "res/shaders/wireframe.vertex.glsl";
static const char* DEFAULT_FRAGMENT_SHADER = "res/shaders/wireframe.fragment.glsl";

sqv_error_t sqv_app_init(app_t* app) {
  assert(app != NULL);
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);

  app->window = SDL_CreateWindow(
    WINDOW_TITLE, 
    SDL_WINDOWPOS_CENTERED, 
    SDL_WINDOWPOS_CENTERED,
    WINDOW_WIDTH, 
    WINDOW_HEIGHT, 
    WINDOW_INIT_FLAGS);

  assert(app->window != NULL);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_MAJOR);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_MINOR);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  
  app->gl_context = SDL_GL_CreateContext(app->window);
  assert(app->gl_context != NULL);

  glewExperimental = GL_TRUE;
  assert(glewInit() == GLEW_OK);

  app->shader_program = createShaderProgramFromFiles(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
  assert(app->shader_program > 0);
  glUseProgram(app->shader_program);

  return SQV_SUCCESS;
}

sqv_error_t sqv_app_clean(app_t* app) {
  assert(app != NULL);
  assert(app->window != NULL);
  assert(app->gl_context != NULL);

  glDeleteProgram(app->shader_program);
  SDL_GL_DeleteContext(app->gl_context);
  SDL_DestroyWindow(app->window);
  SDL_Quit();

  return SQV_SUCCESS;
}

