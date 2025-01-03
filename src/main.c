#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>

void live_reload_plugin();
void init_monitor_plugin();
extern bool plugin_updated;
extern void (*plugin_fn)();

int main(void) {
  const int screenWidth = 400;
  const int screenHeight = 400;

  InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

  SetTargetFPS(60);
  //--------------------------------------------------------------------------------------

  bool pause = true;
  init_monitor_plugin();
  live_reload_plugin();
  while (true) {
    if (IsKeyReleased(KEY_ESCAPE) || plugin_updated) {
      live_reload_plugin();
      plugin_updated = false;
    }

    plugin_fn();
  }

  CloseWindow();
  return 0;
}