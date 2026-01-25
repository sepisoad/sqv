#ifndef SEPI_MACOS_PLATFORM_H
#define SEPI_MACOS_PLATFORM_H

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

#ifdef SEPI_MACOS_PLATFORM_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

#include <mach/mach_vm.h>
#include <unistd.h>      /* getpagesize */
#include <sys/mman.h>    /* mmap */

U32
platform_get_cpu_cores() {
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  return (n > 0) ? (U32)n : 1;
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
  TracyCZoneN(trcyctx, "platform_reserve_large_pages", 1);

  U32 flags = MAP_PRIVATE | MAP_ANON;
  U32 prot = PROT_READ | PROT_WRITE;
  RawPtr result = mmap(0, size, prot, flags, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0);
  if (result == MAP_FAILED) {
    Sz page_size = (Sz)sysconf(_SC_PAGESIZE);
    size = (size + (page_size - 1)) & ~(page_size - 1);
    result = mmap(0, size, prot, MAP_PRIVATE | MAP_ANON, -1, 0);

    if (result == MAP_FAILED) {
      result = 0;
    }
  }

  TracyCAlloc(result, size);
  TracyCZoneEnd(trcyctx);
  return result;
}

U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  TracyCZoneN(trcyctx, "platform_commit_large_pages", 1);

  mprotect(ptr, size, PROT_READ | PROT_WRITE);

  TracyCZoneEnd(trcyctx);
  return 1;
}

Nothing
platform_release(RawPtr ptr, Sz size) {
  TracyCZoneN(trcyctx, "platform_release", 1);

  munmap(ptr, size);

  TracyCFree(ptr);
  TracyCZoneEnd(trcyctx);
}


/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_MACOS_PLATFORM_IMPLEMENTATION */
#endif /* SEPI_MACOS_PLATFORM_H */
