#ifndef SEPI_APP_H
#define SEPI_APP_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/arena.h>
#include <sepi/string.h>
#include <sepi/context.h>
#include <sepi/sync.h>

/* ===================================================== */
/*                  FORWARD DECLERATION                  */
/* ===================================================== */

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  APP_ERR_SUCCESS = 1,
  APP_ERR__COUNT,
} AppError;

typedef enum {
  // app hit an unsupported cpu instruction or something
  APP_CRASH_CAUSE_NANI = 1,
  // app hit a debugger breakpoint
  APP_CRASH_CAUSE_TRAP,
  // app hit an assert or similar situation
  APP_CRASH_CAUSE_ASSERT,
  // app messed up with memory (at software layer)
  APP_CRASH_CAUSE_SOFT_MEMORY,
  // app messed up with memory (at hardware layer)
  APP_CRASH_CAUSE_HARD_MEMORY,
  // app was forcefully terminated by user
  APP_CRASH_CAUSE_USER_KILL,
  //
  APP_CRASH_CAUSE__COUNT,
} AppCrashCause;

typedef Nothing AppCrashHandlerFn(AppCrashCause cause, Str details);
typedef AppCrashHandlerFn* AppCrashHandlerFnPtr;

typedef Str AppContextIDToNameMapper(ContextID);
typedef AppContextIDToNameMapper* AppContextIDToNameMapperPtr;

typedef U8 AppRoutineId;

typedef struct AppRoutine AppRoutine;
struct AppRoutine {
  AppRoutineId id;
  SyncThread* thread;
  Context* context;
};

typedef struct App App;
struct App {
  Context* context;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

App app_create(ContextID context_id, AppCrashHandlerFnPtr crash_handler);
Nothing app_destroy(App app);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_APP_IMPLEMENTATION

mount_slave_profiling_context();

local AppCrashHandlerFnPtr app_crash_handler_ = 0;

/* ----------------------------------------------------- */

local Nothing
app_default_crash_handler(AppCrashCause cause, Str details) {
  start_profiling();

  ignore(cause);
  ignore(details);

  not_implemented();

  end_profiling();
}

/* ----------------------------------------------------- */

App
app_create(ContextID context_id, AppCrashHandlerFnPtr crash_handler) {
  start_profiling();

  context_init(context_id);
  App app = {.context = context()};

  if (0 == crash_handler) {
    app_crash_handler_ = app_default_crash_handler;
  } else {
    app_crash_handler_ = crash_handler;
  }

  end_profiling();
  return app;
}

/* ----------------------------------------------------- */

Nothing
app_destroy(App app) {
  start_profiling();

  assert(app.context != 0);

  app.context = 0;
  context_deinit();

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_APP_IMPLEMENTATION */
#endif /* SEPI_APP_H */
