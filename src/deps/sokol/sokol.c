#define SOKOL_IMPL
#define NK_IMPLEMENTATION
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS

#include "sokol_app.h"
#include "sokol_args.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#include "sokol_log.h"
#include "../nuklear/nuklear.h"
#include "sokol_nuklear.h"

#ifdef DEBUG
#define SOKOL_MEMTRACK_API_DECL
#include "sokol_memtrack.h"
#endif
