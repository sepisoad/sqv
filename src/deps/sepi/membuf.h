#ifndef SEPI_MEMBUF_H
#define SEPI_MEMBUF_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/platform.h>
#include <sepi/buffer.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct MembufParams MembufParams;
struct MembufParams {
  // TODO:
};

typedef struct Membuf Membuf;
struct Membuf {
  // TODO:
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// TODO:

Membuf* membuf_create_(Buf buf);
Nothing membuf_destroy(Membuf* membuf);

/* ===================================================== */
/*                   INLINE FUNCTIONS                    */
/* ===================================================== */

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_MEMBUF_IMPLEMENTATION

mount_slave_profiling_context();

/* ----------------------------------------------------- */

Membuf*
membuf_create(Buf buf) {
  start_profiling();

  // TODO:

  ignore(buf);
  not_implemented();

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
membuf_destroy(Membuf* membuf) {
  start_profiling();

  // TODO:

  ignore(membuf);
  not_implemented();

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_MEMBUF_IMPLEMENTATION */
#endif /* SEPI_MEMBUF_H */
