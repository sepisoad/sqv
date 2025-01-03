#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <raylib.h>
#include <threads.h>
#include <sys/stat.h>
#include <unistd.h>

volatile bool plugin_updated = false;
const char* plugin_source = "src/live.c";
const char* plugin_path = ".ignore/build/live.so";
void* plugin_handler = NULL;

time_t get_file_date(const char* filename) {
  struct stat file_stat;
  if (stat(filename, &file_stat) == -1) {
    perror("stat");
    return 0;
  }
  return file_stat.st_mtime;
}

int monitor_plugin(void* arg) {
  time_t last_modified = get_file_date(plugin_source);

  while (1) {
    sleep(1); // Check the file every second
    time_t current_modified = get_file_date(plugin_source);

    if (current_modified > last_modified) {
      plugin_updated = true;
      last_modified = current_modified;
    }
  }
  return 0;
}

int init_monitor_plugin() {
  thrd_t monitor_thread;
  if (thrd_create(&monitor_thread, monitor_plugin, NULL) != thrd_success) {
    fprintf(stderr, "Failed to create thread\n");
    return EXIT_FAILURE;
  }
}

void build_plugin() {
  int ret = system("make clean_plugin");
  if (ret == -1) {
    perror("system");
  }

  ret = system("make plugin");
  if (ret == -1) {
    perror("system");
  }
  else {
    printf("Command executed with return code: %d\n", ret);
  }
}

void reaload_plugin() {
  if (plugin_handler) {
    dlclose(plugin_handler);
    plugin_handler = NULL;
  }

  plugin_handler = dlopen(plugin_path, RTLD_LAZY);
  if (!plugin_handler) {
    fprintf(stderr, "Error: %s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  char* error = dlerror();
  if (error) {
    fprintf(stderr, "Error: %s\n", error);
    dlclose(plugin_handler);
    exit(EXIT_FAILURE);
  }

  int (*func)();
  *(int**)(&func) = dlsym(plugin_handler, "func");

  error = dlerror();
  if (error) {
    fprintf(stderr, "Error: %s\n", error);
    dlclose(plugin_handler);
    exit(EXIT_FAILURE);
  }

  // Call the function
  printf("HELLO: %d\n:", func());
}

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

  SetTargetFPS(60);
  //--------------------------------------------------------------------------------------

  bool pause = true;
  init_monitor_plugin();
  reaload_plugin();
  while (true) {
    if (IsKeyReleased(KEY_ESCAPE) || plugin_updated) {
      build_plugin();
      reaload_plugin();
      plugin_updated = false;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}