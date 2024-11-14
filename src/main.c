#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>

#include <lfs/lfs.h>
#include <stb/img.h>

#define ENTRY_SCRIPT "./src/init.lua"

void set_lua_package_path(lua_State *L) {
  // Get the current package.path
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");  // Get the current path
  const char *current_path = lua_tostring(L, -1);

  // Construct the new paths to only include the root deps/lua directory
  const char *new_paths = "./deps/lua/?.lua;./deps/lua/?/init.lua;";
  char updated_path[2048];

  // Combine new paths with the current path
  snprintf(updated_path, sizeof(updated_path), "%s%s", new_paths, current_path);

  // Update package.path
  lua_pop(L, 1);  // Remove the old path
  lua_pushstring(L, updated_path);
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);  // Remove the package table
}

int main(int argc, char** argv) {
  // init lua vm
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  // load custom modules
  open_module_lfs(L);
  open_module_stb(L);
  set_lua_package_path(L);

  // redirect input args to the main lua script
  for (int i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "arg");

  // run the entry script
  if (luaL_dofile(L, ENTRY_SCRIPT) != LUA_OK) {
    fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  }

  // run the debug script
  // luaL_dofile(L, "src/debug.lua");

  // clean up
  lua_close(L);
  return 0;
}
