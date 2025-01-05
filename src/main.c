#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#include "quake/mdl_decoder.h"

void live_reload_plugin();
void init_monitor_plugin();
extern bool plugin_updated;
extern void (*plugin_init_fn)();
extern void (*plugin_main_fn)(int, int);
extern void (*plugin_kill_fn)();
int quake_read_model(const char* filename, quake_model* mdl);

const int screen_width = 400;
const int screen_height = 400;
const char mdl_file_path[] = ".keep/dog.mdl";

int main(void) {
  InitWindow(screen_width, screen_height, "raylib [core] example - 3d picking");
  SetTargetFPS(60);

  init_monitor_plugin();
  live_reload_plugin();

  quake_model model;
  quake_read_model(".keep/dog.mdl", &model);

  plugin_init_fn();
  while (true) {
    if (plugin_updated) {
      live_reload_plugin();
      plugin_updated = false;
      plugin_init_fn();
    }
    plugin_main_fn(screen_width, screen_height);
  }
  plugin_kill_fn();

  CloseWindow();
  return 0;
}