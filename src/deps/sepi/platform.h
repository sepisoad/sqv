#ifndef SEPI_PLATFORM_H
#define SEPI_PLATFORM_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <tracy/tracy.h>

#if defined(OS_LINUX)
# include <sys/sysinfo.h>
# include <unistd.h>
# include <sys/mman.h>

#elif defined(OS_MACOS)
# include <mach/mach_vm.h>
# include <unistd.h>      /* getpagesize */
# include <sys/mman.h>    /* mmap */

#elif defined(OS_WINDOWS)
# include <sysinfoapi.h>
# include <memoryapi.h>

#endif

#include <sepi/base.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

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

#ifdef SEPI_PLATFORM_IMPLEMENTATION

mount_slave_profiling_context();

#if defined(OS_LINUX)

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
  start_profiling(1);

  U32 flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;
  RawPtr result = mmap(0, size, PROT_NONE, flags, -1, 0);

  if (result == MAP_FAILED) {
    flags = MAP_PRIVATE | MAP_ANONYMOUS;
    result = mmap(0, size, PROT_NONE, flags, -1, 0);
    if (result == MAP_FAILED) {
      result = 0;
    }
  }

  start_memory_profiling(result, size);
  end_profiling();
  return result;
}

U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  start_profiling(1);

  mprotect(ptr, size, PROT_READ | PROT_WRITE);

  end_profiling();
  return 1;
}

Nothing
platform_release(RawPtr ptr, Sz size) {
  start_profiling(1);

  munmap(ptr, size);

  end_memory_profiling(ptr);
  end_profiling();
}

#elif defined(OS_MACOS)

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
  return mega_bytes(2);
}

RawPtr
platform_reserve_large_pages(Sz size) {
  start_profiling(1);

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

  start_memory_profiling(result, size);
  end_profiling();
  return result;
}

U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  start_profiling(1);

  mprotect(ptr, size, PROT_READ | PROT_WRITE);

  end_profiling();
  return 1;
}

Nothing
platform_release(RawPtr ptr, Sz size) {
  start_profiling(1);

  munmap(ptr, size);

  end_memory_profiling(ptr);
  end_profiling();
}

#elif defined(OS_WINDOWS)

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
  start_profiling(1);

  DWORD flags = MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES;
  RawPtr result = VirtualAlloc(0, size, flags, PAGE_READWRITE);
  if (!result) {
    flags = MEM_RESERVE | MEM_COMMIT;
    result = VirtualAlloc(0, size, flags, PAGE_READWRITE);
    if (!result) {
      return 0;
    }
  }

  start_memory_profiling(result, size);
  end_profiling();
  return result;
}

U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  return 1;
}

Nothing
platform_release(RawPtr ptr, Sz size) {
  start_profiling(1);

  ignore(size);
  VirtualFree(ptr, 0, MEM_RELEASE);

  end_memory_profiling(ptr);
  end_profiling();
}

#endif

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_PLATFORM_IMPLEMENTATION */
#endif  // SEPI_PLATFORM_H
