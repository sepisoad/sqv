#include <lua5.4/lua.h>

#ifdef _WIN32
#define MODULE_EXPORT __declspec (dllexport)
#else
#define MODULE_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

  MODULE_EXPORT int open_module_stb(lua_State * L);

#ifdef __cplusplus
}
#endif
