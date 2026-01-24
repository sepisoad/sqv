#ifndef SEPI_COMMON_PLATFORM_H
#define SEPI_COMMON_PLATFORM_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../base.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

// --

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

U32 platform_get_cpu_cores();
Sz platform_get_page_size();
Sz platform_get_large_page_size();
RawPtr platform_reserve_large_pages(Sz size);
U32 platform_commit_large_pages(RawPtr ptr, Sz size);
Nothing platform_release(RawPtr ptr, Sz size);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_COMMON_PLATFORM_IMPLEMENTATION

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_COMMON_PLATFORM_IMPLEMENTATION */
#endif /* SEPI_COMMON_PLATFORM_H */
