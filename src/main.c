#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "app.h"

app_t* setup_ui(uint32_t width, uint32_t height);
int32_t render_ui(app_t* app);

int main(int argc, char* argv[])
{
  app_t* app = setup_ui(800, 600);
  render_ui(app);
}

