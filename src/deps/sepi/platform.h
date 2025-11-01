#ifndef SEPI_PLATFORM_H
#define SEPI_PLATFORM_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "base.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#if defined(SEPI_PLATFORM_IMPLEMENTATION)
#define MODULE
#else
#define MODULE static
#endif /* SEPI_PLATFORM_IMPLEMENTATION */

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

MODULE U32 platform_get_cpu_cores();
MODULE Sz platform_get_page_size();
MODULE Sz platform_get_large_page_size();
MODULE RawPtr platform_reserve_large_pages(Sz size);
MODULE U32 platform_commit_large_pages(RawPtr ptr, Sz size);
MODULE Nothing platform_release(RawPtr ptr, Sz size);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_PLATFORM_IMPLEMENTATION

#if defined(OS_LINUX) || defined(OS_MAC)

#include <sys/sysinfo.h> /* get_nprocs */
#include <unistd.h> /* getpagesize */
#include <sys/mman.h> /* mmap */

MODULE U32
platform_get_cpu_cores() {
  return (U32) get_nprocs();
}

MODULE Sz
platform_get_page_size() {
  return (Sz) sysconf(_SC_PAGESIZE);
}

MODULE Sz
platform_get_large_page_size() {
  return MB(2);
}

MODULE RawPtr
platform_reserve_large_pages(Sz size) {
  U32 flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;
  void* result = mmap(0, size, PROT_NONE, flags, -1, 0);
  if(result == MAP_FAILED) {
    flags = MAP_PRIVATE | MAP_ANONYMOUS;
    result = mmap(0, size, PROT_NONE, flags, -1, 0);
    if(result == MAP_FAILED) {
      result = 0;
    }
  }
  return result;
}

MODULE U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  mprotect(ptr, size, PROT_READ | PROT_WRITE);
  return 1;
}

MODULE Nothing
platform_release(RawPtr ptr, Sz size) {
  munmap(ptr, size);
}

#else /* OS_WINDOWS */

#include <sysinfoapi.h>
#include <memoryapi.h>

MODULE U32
platform_get_cpu_cores() {
  SYSTEM_INFO si = {0};
  GetSystemInfo(&si);
  return (U32) si->dwNumberOfProcessors;
}

MODULE Sz
platform_get_page_size() {
  SYSTEM_INFO si = {0};
  GetSystemInfo(&si);
  return (Sz) si->dwPageSize;
}

MODULE Sz
platform_get_large_page_size() {
  return GetLargePageMinimum();
}

MODULE RawPtr
platform_reserve_large_pages(Sz size) {
  RawPtr result = VirtualAlloc(0, size,
                               MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
  return result;
}

MODULE U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  return 1;
}

MODULE Nothing
platform_release(RawPtr ptr, Sz size) {
  Ignore(size);
  VirtualFree(ptr, 0, MEM_RELEASE);
}

#endif

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_PLATFORM_IMPLEMENTATION */
#endif /* SEPI_PLATFORM_H */
