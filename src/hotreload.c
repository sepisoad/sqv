#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <threads.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

volatile bool plugin_updated = false;
const char* plugin_source = "src/plugin.c";
const char* plugin_path = ".ignore/build/plugin.so";
void* plugin_handler = NULL;
void (*plugin_fn)();

time_t get_file_date(const char* filename) {
  struct stat file_stat;
  if (stat(filename, &file_stat) == -1) {
    fprintf(stderr, "Error: Failed to get file status for '%s'\n", filename);
    perror("stat");
    exit(EXIT_FAILURE);
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

void init_monitor_plugin() {
  thrd_t monitor_thread;
  if (thrd_create(&monitor_thread, monitor_plugin, NULL) != thrd_success) {
    fprintf(stderr, "Error: Failed to create monitoring thread\n");
    // exit(EXIT_FAILURE);
    return;
  }
}

bool build_plugin() {
  int ret = system("make clean_plugin");
  if (ret == -1) {
    fprintf(stderr, "Error: Failed to execute 'make clean_plugin'\n");
    perror("system");
    // exit(EXIT_FAILURE);
    return false;
  }

  ret = system("make plugin");
  if (ret == -1) {
    fprintf(stderr, "Error: Failed to execute 'make plugin'\n");
    perror("system");
    // exit(EXIT_FAILURE);
    return false;
  }

  if (ret != 0) {
    fprintf(stderr, "Error: 'make plugin' exited with code %d\n", ret);
    // exit(EXIT_FAILURE);
    return false;
  }

  return true;
}

void reload_plugin() {
  if (plugin_handler) {
    dlclose(plugin_handler);
    plugin_handler = NULL;
  }

  plugin_handler = dlopen(plugin_path, RTLD_LAZY);
  if (!plugin_handler) {
    fprintf(stderr, "Error: Failed to load plugin '%s': %s\n", plugin_path, dlerror());
    // exit(EXIT_FAILURE);
    return;
  }

  dlerror(); // Clear existing errors

  *(void**)(&plugin_fn) = dlsym(plugin_handler, "plugin_fn");

  char* error = dlerror();
  if (error) {
    fprintf(stderr, "Error: Failed to load symbol 'plugin_fn': %s\n", error);
    dlclose(plugin_handler);
    // exit(EXIT_FAILURE);
    return;
  }
}

void live_reload_plugin() {
  if (build_plugin()) reload_plugin();
}
