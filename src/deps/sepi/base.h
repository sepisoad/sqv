#ifndef SEPI_BASE_H
#define SEPI_BASE_H

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef int8_t I8;
typedef uint8_t U8;
typedef int16_t I16;
typedef uint16_t U16;
typedef int32_t I32;
typedef uint32_t U32;
typedef int64_t I64;
typedef uint64_t U64;
typedef size_t Sz;

typedef float F32;
typedef double F64;

typedef I8 Bool;
#define TRUE 1
#define FALSE 0

typedef void Nothing;
typedef void* RawPtr;
typedef intptr_t IPtr;
typedef uintptr_t Ptr;
typedef const intptr_t CIPtr;
typedef const uintptr_t CPtr;
typedef ptrdiff_t PtrDiff;

typedef U8* Buf;
typedef const U8* CBuf;
typedef char* Str;
typedef const char* CStr;

typedef struct Empty Empty;
struct Empty {};

/* ===================================================== */
/*                         DEBUG                         */
/* ===================================================== */

#if defined(DEBUG) || defined(_DEBUG)
#define DEBUG_MODE
#else
#undef DEBUG_MODE
#endif /* DEBUG_MODE */

/* ===================================================== */
/*                      COMPILERS                        */
/* ===================================================== */

#if defined(__clang__)
#define CC_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define CC_GCC 1
#elif defined(_MSCvalueER)
#define CC_MSVC 1
#else
#error "compiler not supported!"
#endif

/* ===================================================== */
/*                   OPERATING SYSTEM                    */
/* ===================================================== */

#if defined(_WIN32)
#define OS_WINDOWS 1
#elif defined(__gnu_linux__) || defined(__linux__)
#define OS_LINUX 1
#define _GNU_SOURCE
#elif defined(__APPLE__) && defined(__MACH__)
#define OS_MACOS 1
#else
#error "os not supported!"
#endif

/* ===================================================== */
/*                       CPU ARCHS                       */
/* ===================================================== */

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || \
    defined(__x86_64)
#define CPU_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__)
#define CPU_X86 1
#elif defined(__aarch64__)
#define CPU_ARM64 1
#elif defined(__arm__)
#define CPU_ARM32 1
#else
#error "cpu not supported!"
#endif

/* ===================================================== */
/*                         MATHS                         */
/* ===================================================== */

#define is_pow2(X) ((X) != 0 && ((X) & ((X) - 1)) == 0)
#define is_pow2_or_zero(X) ((((X) - 1) & (X)) == 0)
#define max(A, B) ((A) > (B) ? (A) : (B))
#define min(A, B) ((A) < (B) ? (A) : (B))

/* ===================================================== */
/*                         UNITS                         */
/* ===================================================== */

#define kilo_bytes(number) (((U64)(number)) << 10)
#define mega_bytes(number) (((U64)(number)) << 20)
#define giga_bytes(number) (((U64)(number)) << 30)
#define terra_bytes(number) (((U64)(number)) << 40)
#define thousand(number) ((number) * 1000)
#define million(number) ((number) * 1000000)
#define billion(number) ((number) * 1000000000)

/* ===================================================== */
/*                         UTILS                         */
/* ===================================================== */

#define ignore(value) ((void)(value))

#define to_string_(expression) #expression
#define to_string(expression) to_string_(expression)

#define glue_(expression_a, expression_b) expression_a##expression_b
#define glue(expression_a, expression_b) glue_(expression_a, expression_b)

/* ===================================================== */
/*                       ALIGNMENT                       */
/* ===================================================== */

#if defined(CC_MSVC)
#define alignof(type) __alignof(type)
#elif defined(CC_GCC) || defined(CC_CLANG)
#define alignof(type) __alignof(type)
#else
#error "alignof() not supported!"
#endif

#define align_up(value, boundry) \
  (((value) + (boundry) - 1) & (~((boundry) - 1)))
#define align_down(value, boundry) ((value) & (~((boundry) - 1)))
#define get_alignment_padding(value, boundry) ((0 - (value)) & ((boundry) - 1))

#define get_native_alignment() \
  max(alignof(int),            \
      max(alignof(long),       \
          max(alignof(long long), max(alignof(double), alignof(void*)))))

/* ===================================================== */
/*                        BITOPS                         */
/* ===================================================== */

#if defined(CC_MSVC)
#define get_leading_0_bits(T) _BitScanReverse64(0, T)  // TODO: not tested!
#elif defined(CC_GCC) || defined(CC_CLANG)
#define get_leading_0_bits(T) __builtin_clzll(T)
#else
#error "get_leading_0_bits() not supported!"
#endif

/* ===================================================== */
/*                      MEMORY OPS                       */
/* ===================================================== */

#define zero_memory(ptr, size) memset((ptr), 0, (size))
#define zero_struct(ptr) zero_memory((ptr), sizeof(*(ptr)))
#define zero_array(ptr) zero_memory((ptr), sizeof(ptr))
#define zero_memory_typed(ptr, count) \
  zero_memory((ptr), sizeof(*(ptr)) * (count))
#define copy_memory(DST, SRC, SZ) memcpy((DST), (SRC), (SZ))
#define compare_memory(a, b, size) memcmp((a), (b), (size))
#define is_memory_equal(a, b, z) (compare_memory((a), (b), (z)) == 0)
#define is_struct_equal(a, b) is_memory_equal((a), (b), sizeof(*(a)))
#define is_array_equal(a, b) is_memory_equal((a), (b), sizeof(a))

/* ===================================================== */
/*                         ASAN                          */
/* ===================================================== */

#if defined(CC_MSVC)
#if defined(__SANITIZE_ADDRESS__)
#define ASAN_ENABLED 1
#define NO_ASAN __declspec(no_sanitize_address)
#else
#define NO_ASAN
#endif

#elif defined(CC_CLANG)
#if defined(__has_feature)
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#define ASAN_ENABLED 1
#endif
#endif
#define NO_ASAN __attribute__((no_sanitize("address")))

#elif CC_GCC
#if defined(__SANITIZE_ADDRESS__)
#define ASAN_ENABLED 1
#endif
#define NO_ASAN __attribute__((no_sanitize_address))

#else
#define NO_ASAN
#endif

#ifdef ASAN_ENABLED
void __asan_poison_memory_region(void const volatile* addr, size_t size);
void __asan_unpoison_memory_region(void const volatile* addr, size_t size);
#define asan_poison_memory_region(addr, size) \
  __asan_poison_memory_region((addr), (size))
#define asan_unpoison_memory_region(addr, size) \
  __asan_unpoison_memory_region((addr), (size))
#else
#define asan_poison_memory_region(addr, size) ((void)(addr), (void)(size))
#define asan_unpoison_memory_region(addr, size) ((void)(addr), (void)(size))
#endif /* DEBUG_MODE */

/* ===================================================== */
/*                      ASSERTIONS                       */
/* ===================================================== */

#if CC_MSVC
#define trap() __debugbreak()
#elif CC_CLANG || CC_GCC
#define trap() __builtin_trap()
#else
#error unsupported compiler
#endif

#define static_assert(condition, id) \
  typedef char glue(id, __LINE__)[(condition) ? 1 : -1]

#ifdef DEBUG_MODE
#define runtime_assert(condition)                         \
  do {                                                    \
    if (!(condition)) {                                   \
      fprintf(stderr, "assert: (%s)\n", #condition);      \
      fprintf(stderr, "at: %s:%d\n", __FILE__, __LINE__); \
      trap();                                             \
    }                                                     \
  } while (0)
#define assert(condition) runtime_assert(condition)
#else /* DEBUG_MODE */
#define runtime_assert(condition) \
  do {                            \
    if (!(condition)) {           \
      trap();                     \
    }                             \
  } while (0)
#define assert(condition) (void)(condition)
#endif

#define abort(message) runtime_assert(!#message)
#define not_implemented() abort("NOT IMPLEMENTED!")

/* ===================================================== */
/*                     DEBUG LOGGER                      */
/* ===================================================== */

#ifdef DEBUG_MODE
#include <stdio.h>
#define dbg(format, ...)           \
  do {                             \
    printf(format, ##__VA_ARGS__); \
    printf("\n");                  \
  } while (0);
#else
#define dbg(format, ...)
#endif /* DEBUG_MODE */

/* ===================================================== */
/*                      PROFILING                        */
/* ===================================================== */

#ifdef PROFILING
#include <tracy/tracy.h>
#define mount_master_profiling_context() TracyCZoneCtx tracyctx;
#define mount_slave_profiling_context() extern TracyCZoneCtx tracyctx;
#define start_profiling(NUM) TracyCZoneN(tracyctx, __func__, (NUM));
#define end_profiling() TracyCZoneEnd(tracyctx);
#define start_memory_profiling(PTR, SIZE) TracyCAlloc((PTR), (SIZE));
#define end_memory_profiling(PTR) TracyCFree((PTR));
#else /* NOT PROFILING */
#define mount_master_profiling_context()
#define mount_slave_profiling_context()
#define start_profiling(NUM)
#define end_profiling()
#define start_memory_profiling(PTR, SIZE)
#define end_memory_profiling(PTR)
#endif /* */

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_BASE_H
