#ifndef SEPI_PLATFORM_H
#define SEPI_PLATFORM_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <tracy/tracy.h>

#if defined(OS_LINUX)
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/mman.h>
#elif defined(OS_MACOS)
#include <mach/mach_vm.h>
#include <unistd.h>   /* getpagesize */
#include <sys/mman.h> /* mmap */
#elif defined(OS_WINDOWS)
#include <sysinfoapi.h>
#include <memoryapi.h>
#endif

#include <sepi/base.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef U8 PlatformOS;
enum {
  PLATFORM_OS_LINUX = 1,
  PLATFORM_OS_MACOS,
  PLATFORM_OS_WINDOWS,
  PLATFORM_OS__COUNT,
};

typedef U8 PlatformCPU;
enum {
  PLATFORM_CPU_INTEL_32 = 1,
  PLATFORM_CPU_INTEL_64,
  PLATFORM_CPU_ARM_32,
  PLATFORM_CPU_ARM_64,
  PLATFORM_CPU__COUNT,
};

typedef struct PlatformInfo PlatformInfo;
struct PlatformInfo {
  PlatformOS os;
  PlatformCPU cpu;
  U8 cpu_cores;
  Sz memory_page_size;
  Sz memory_large_page_size;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

static inline PlatformCPU platform_get_cpu(Nothing);
static inline PlatformOS platform_get_os(Nothing);
static inline PlatformInfo platform_get_info();
static inline U8 platform_get_cpu_cores(Nothing);
static inline Sz platform_get_page_size(Nothing);
static inline Sz platform_get_large_page_size(Nothing);
static inline RawPtr platform_reserve_large_pages(Sz size);
static inline U32 platform_commit_large_pages(RawPtr ptr, Sz size);
static inline Nothing platform_release(RawPtr ptr, Sz size);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

mount_slave_profiling_context();

static inline PlatformCPU
platform_get_cpu(Nothing) {
#if defined(CPU_INTEL_64)
  return PLATFORM_CPU_INTEL_64;
#elif defined(CPU_INTEL_32)
  return PLATFORM_CPU_INTEL_32;
#elif defined(CPU_ARM_64)
  return PLATFORM_CPU_ARM_64;
#elif defined(CPU_ARM_32)
  return PLATFORM_CPU_ARM_32;
#endif
}

static inline PlatformOS
platform_get_os(Nothing) {
#if defined(OS_LINUX)
  return PLATFORM_OS_LINUX;
#elif defined(OS_MACOS)
  return PLATFORM_OS_MACOS;
#elif defined(OS_WINDOWS)
  return PLATFORM_OS_WINDOWS;
#endif
}

static inline PlatformInfo
platform_get_info() {
  return (PlatformInfo){
      .os = platform_get_os(),
      .cpu = platform_get_cpu(),
      .cpu_cores = platform_get_cpu_cores(),
      .memory_page_size = platform_get_page_size(),
      .memory_large_page_size = platform_get_large_page_size(),
  };
}

static inline U8
platform_get_cpu_cores(Nothing) {
#if defined(OS_LINUX)
  return (U32)get_nprocs();
#elif defined(OS_MACOS)
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  return (n > 0) ? (U8)n : 1;
#elif defined(OS_WINDOWS)
  SYSTEM_INFO si = {0};
  GetSystemInfo(&si);
  return (U32)si.dwNumberOfProcessors;
#endif
}

static inline Sz
platform_get_page_size(Nothing) {
#if defined(OS_LINUX)
  return (Sz)sysconf(_SC_PAGESIZE);
#elif defined(OS_MACOS)
  return (Sz)sysconf(_SC_PAGESIZE);
#elif defined(OS_WINDOWS)
  SYSTEM_INFO si = {0};
  GetSystemInfo(&si);
  return (Sz)si.dwPageSize;
#endif
}

static inline Sz
platform_get_large_page_size(Nothing) {
#if defined(OS_LINUX)
  return mega_bytes(2);
#elif defined(OS_MACOS)
  return mega_bytes(2);
#elif defined(OS_WINDOWS)
  return GetLargePageMinimum();
#endif
}

static inline RawPtr
platform_reserve_large_pages(Sz size) {
  start_profiling(1);

#if defined(OS_LINUX)
  U32 flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;
  RawPtr result = mmap(0, size, PROT_NONE, flags, -1, 0);

  if (result == MAP_FAILED) {
    flags = MAP_PRIVATE | MAP_ANONYMOUS;
    result = mmap(0, size, PROT_NONE, flags, -1, 0);
    if (result == MAP_FAILED) {
      result = 0;
    }
  }

#elif defined(OS_MACOS)
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

#elif defined(OS_WINDOWS)
  // TODO:
  // sepi, this part of the code for windows is completely untested i partially
  // took this code from rad-debugger, but there they use 'MEM_RESERVE |
  // MEM_COMMIT' af the same time in this function and inside
  // 'platform_commit_large_pages' they just return 0 (or maybe 1) as return
  // value, so it seems on the surface that both reservation and commit happens
  // at the same time which means the whole memory will be allocated at once
  // which is unlike what we do (and they do as well) for linux (and macos)
  DWORD flags = | MEM_LARGE_PAGES;
  RawPtr result = VirtualAlloc(0, size, flags, PAGE_READWRITE);
  if (!result) {
    flags = MEM_RESERVE;
    result = VirtualAlloc(0, size, flags, PAGE_READWRITE);
    if (!result) {
      return 0;
    }
  }

#endif

  start_memory_profiling(result, size);
  end_profiling();
  return result;
}

static inline U32
platform_commit_large_pages(RawPtr ptr, Sz size) {
  start_profiling(1);

#if defined(OS_LINUX)
  mprotect(ptr, size, PROT_READ | PROT_WRITE);
#elif defined(OS_MACOS)
  mprotect(ptr, size, PROT_READ | PROT_WRITE);
#elif defined(OS_WINDOWS)
  VirtualAlloc(ptr, size, , PAGE_READWRITE);
#endif

  end_profiling();
  return 1;
}

static inline Nothing
platform_release(RawPtr ptr, Sz size) {
  start_profiling(1);

#if defined(OS_LINUX)
  munmap(ptr, size);
#elif defined(OS_MACOS)
  munmap(ptr, size);
#elif defined(OS_WINDOWS)
  ignore(size);
  VirtualFree(ptr, 0, MEM_RELEASE);
#endif

  end_memory_profiling(ptr);
  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_PLATFORM_H
