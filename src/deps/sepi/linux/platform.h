#ifndef SEPI_LINUX_PLATFORM_H
#define SEPI_LINUX_PLATFORM_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../../tracy/tracy.h"

#include "../base.h"
#include "../common/platform.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

// --

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// --

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_LINUX_PLATFORM_IMPLEMENTATION

SLAVE_PROFILING_CONTEXT;

#include <sys/sysinfo.h> /* get_nprocs */
#include <unistd.h>      /* getpagesize */
#include <sys/mman.h>    /* mmap */

U32
platform_get_cpu_cores() {
  return (U32)get_nprocs();
}

Sz
platform_get_page_size() {
  return (Sz)sysconf(_SC_PAGESIZE);
}

Sz
platform_get_large_page_size() {
  return MB(2);
}

RawPtr
platform_reserve_large_pages(Sz size) {
  START_PROFILING(1);

  U32 flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;
  RawPtr result = mmap(0, size, PROT_NONE, flags, -1, 0);

  if (result == MAP_FAILED) {
    flags = MAP_PRIVATE | MAP_ANONYMOUS;
    result = mmap(0, size, PROT_NONE, flags, -1, 0);
    if (result == MAP_FAILED) {
      result = 0;
    }
  }

  START_MEMORY_PROFILING(result, size);
  END_PROFILING();
  return result;
}

U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  START_PROFILING(1);

  mprotect(ptr, size, PROT_READ | PROT_WRITE);

  END_PROFILING();
  return 1;
}

Nothing
platform_release(RawPtr ptr, Sz size) {
  START_PROFILING(1);

  munmap(ptr, size);

  END_MEMORY_PROFILING(ptr);
  END_PROFILING();
}


/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_LINUX_PLATFORM_IMPLEMENTATION */
#endif /* SEPI_LINUX_PLATFORM_H */
