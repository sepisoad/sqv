#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>

void plugin_fn() {
  BeginDrawing();
  ClearBackground(RAYWHITE);
  DrawText("Congrats! You created your first window!", -10, 10, 20, BLACK);
  EndDrawing();
}
