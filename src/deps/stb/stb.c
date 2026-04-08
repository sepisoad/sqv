#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// #include <sepi/base.h>
// #include <sepi/arena.h>
// #include <sepi/context.h>

// #define STBIW_MALLOC(sz) arena_push(context_arena(), sz, get_native_alignment(), FALSE)
// #define STBIW_REALLOC(p,newsz) arena_push(context_arena(), newsz, get_native_alignment(), FALSE)
// #define STBIW_FREE(p) /**/

#include <stb/stb_ds.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
