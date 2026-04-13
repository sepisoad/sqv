#ifndef SEPI_MEMPOOL_H
#define SEPI_MEMPOOL_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/platform.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct MempoolParams MempoolParams;
struct MempoolParams {
  // TODO:
};

typedef struct Mempool Mempool;
struct Mempool {
  // TODO:
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// TODO:

Mempool* mempool_create_(MempoolParams* mempool_params);
Nothing mempool_destroy(Mempool* mempool);

/* ===================================================== */
/*                   INLINE FUNCTIONS                    */
/* ===================================================== */

// TODO:

embed Mempool*
mempool_create() {
  return mempool_create_(&(MempoolParams){});
}

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_MEMPOOL_IMPLEMENTATION

mount_slave_profiling_context();

/* ----------------------------------------------------- */

Mempool*
mempool_create_(MempoolParams* mempool_params) {
  start_profiling();

  // TODO:

  ignore(mempool_params);
  not_implemented();

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
mempool_destroy(Mempool* mempool) {
  start_profiling();

  // TODO:

  ignore(mempool);
  not_implemented();

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_MEMPOOL_IMPLEMENTATION */
#endif /* SEPI_MEMPOOL_H */
