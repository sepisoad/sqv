#ifndef SEPI_CONTEXT_H
#define SEPI_CONTEXT_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/arena.h>
#include <sepi/string.h>

/* ===================================================== */
/*                  FORWARD DECLERATION                  */
/* ===================================================== */

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef U32 ContextID;

typedef Str ContextIDToNameMapperFn(ContextID);
typedef ContextIDToNameMapperFn* ContextIDToNameMapperFnPtr;

typedef struct Context Context;
struct Context {
  Arena* arena;
  ContextID id;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Nothing context_init(ContextID context_id);
Nothing context_deinit(Nothing);
Context* context(Nothing);
Arena* context_arena(Nothing);
ArenaScratch context_scratch_begin(Nothing);
Nothing context_scratch_end(ArenaScratch scratch);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_CONTEXT_IMPLEMENTATION

mount_slave_profiling_context();

thread_local Context context_thread_local;

/* ----------------------------------------------------- */

Nothing
context_init(ContextID context_id) {
  start_profiling();

  assert(context_thread_local.arena == 0);

  Arena* arena = arena_create();
  // context_thread_local =
  //     arena_push(arena, sizeof(Context), alignof(Context), TRUE);
  context_thread_local.arena = arena;
  context_thread_local.id = context_id;

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
context_deinit(Nothing) {
  start_profiling();

  assert(context_thread_local.arena != 0);

  arena_destroy(context_thread_local.arena);
  zero_memory(&context_thread_local, sizeof(Context));

  end_profiling();
}

/* ----------------------------------------------------- */

Context*
context(Nothing) {
  start_profiling();

  assert(context_thread_local.arena != 0);

  end_profiling();
  return &context_thread_local;
}

/* ----------------------------------------------------- */

Arena*
context_arena(Nothing) {
  start_profiling();

  assert(context_thread_local.arena != 0);

  end_profiling();
  return context_thread_local.arena;
}

/* ----------------------------------------------------- */

ArenaScratch
context_scratch_begin(Nothing) {
  start_profiling();

  assert(context_thread_local.arena != 0);

  end_profiling();
  return arena_scratch_begin(context_thread_local.arena);
}

/* ----------------------------------------------------- */

Nothing
context_scratch_end(ArenaScratch scratch) {
  start_profiling();

  arena_scratch_end(scratch);

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_CONTEXT_IMPLEMENTATION */
#endif /* SEPI_CONTEXT_H */
