#ifndef SEPI_WINDOWS_PLATFORM_H
#define SEPI_WINDOWS_PLATFORM_H

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

#ifdef SEPI_WINDOWS_PLATFORM_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

#include <sysinfoapi.h>
#include <memoryapi.h>

U32
platform_get_cpu_cores() {
  SYSTEM_INFO si = {0};
  GetSystemInfo(&si);
  return (U32)si.dwNumberOfProcessors;
}

Sz
platform_get_page_size() {
  SYSTEM_INFO si = {0};
  GetSystemInfo(&si);
  return (Sz)si.dwPageSize;
}

Sz
platform_get_large_page_size() {
  return GetLargePageMinimum();
}

RawPtr
platform_reserve_large_pages(Sz size) {
  TracyCZoneN(trcyctx, "platform_reserve_large_pages", 1);

  DWORD flags = MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES;
  RawPtr result = VirtualAlloc(0, size, flags, PAGE_READWRITE);
  if (!result) {
    flags = MEM_RESERVE | MEM_COMMIT;
    result = VirtualAlloc(0, size, flags, PAGE_READWRITE);
    if (!result) {
      return 0;
    }
  }

  TracyCAlloc(result, size);
  TracyCZoneEnd(trcyctx);
  return result;
}

U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  return 1;
}

Nothing
platform_release(RawPtr ptr, Sz size) {
  TracyCZoneN(trcyctx, "platform_release", 1);

  Ignore(size);
  VirtualFree(ptr, 0, MEM_RELEASE);

  TracyCFree(ptr);
  TracyCZoneEnd(trcyctx);
}


/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_WINDOWS_PLATFORM_IMPLEMENTATION */
#endif /* SEPI_WINDOWS_PLATFORM_H */
