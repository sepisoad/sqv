#ifndef SEPI_BASE_H
#define SEPI_BASE_H

/* ===================================================== */
/*                         DEBUG                         */
/* ===================================================== */

#if defined(DEBUG) || defined(_DEBUG)
#define DEBUG_MODE
#else
#undef DEBUG_MODE
#endif

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

#include <stdint.h>
#include <stddef.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef size_t sz;

typedef float f32;
typedef double f64;

typedef void* rptr;
typedef intptr_t iptr;
typedef uintptr_t uptr;
typedef ptrdiff_t xptr;

typedef char* str;
typedef const char* cstr;
typedef unsigned char* buf;
typedef const unsigned char* cbuf;

/* ===================================================== */
/*                         UTILS                         */
/* ===================================================== */

/* GENERAL */
#define NOOP ((void)0)
#define IGNORE(_V) ((void)(_V))

/* MATH MACROS */
#define IS_POW2(_X) ((_X) && (((_X) & ((_X) - 1)) == 0))
#define MAX(_A, _B) ((_A) > (_B) ? (_A) : (_B))
#define MIN(_A, _B) ((_A) < (_B) ? (_A) : (_B))

/* ALIGNMENT MACROS */
#ifdef _MSC_VER
#define ALIGNOF(T) __alignof(T) /* MSVC */
#else
#define ALIGNOF(T) __alignof__(T) /* GCC/Clang/TCC */
#endif

#define NATIVE_ALIGNMENT \
  MAX(ALIGNOF(int),      \
      MAX(ALIGNOF(long), \
          MAX(ALIGNOF(long long), MAX(ALIGNOF(double), ALIGNOF(void*)))))

#define ALIGN_UP(V, A) ((sz)((((sz)(V) + (sz)(A) - 1) / (sz)(A)) * (sz)(A)))
#define ALIGN_DN(V, A) ((sz)(V) - ((sz)(V) % (sz)(A)))

/* ===================================================== */
/*                        STRINGS                        */
/* ===================================================== */

#define STR_(x) #x
#define STR(x) STR_(x)

/* ===================================================== */
/*                      ASSERTIONS                       */
/* ===================================================== */

#ifdef DEBUG_MODE

#define MAKESURE(expr, msg, ...) \
  ((expr) ? (expr) : (log_fatal(msg, ##__VA_ARGS__), abort(), (expr)))

#define NOTNULL(val) MAKESURE((val), "<<NULL>>")
#define NOTZERO(val) MAKESURE((val), "<<ZERO>>")
#define ISVALID(val) MAKESURE((val), "<<INVALID>>")
#define MUSTDIE(msg, ...) MAKESURE(false, msg, ##__VA_ARGS__)

#else /* not debug mode */

#define MAKESURE(expr, msg, ...) NOOP
#define NOTNULL(val) NOOP
#define NOTZERO(val) NOOP
#define ISVALID(val) NOOP
#define MUSTDIE(msg, ...) NOOP

#endif /* DEBUG_MODE */

#define ERROROUT(expr, err) \
  do {                      \
    if (!(expr)) {          \
      return (err);         \
    }                       \
  } while (0)

#define SUCCEED(expr)      \
  do {                     \
    i64 _ms_tmp_ = (expr); \
    if (_ms_tmp_ != 0) {   \
      return _ms_tmp_;     \
    }                      \
  } while (0)

/* ===================================================== */
/*                     DEBUG LOGGER                      */
/* ===================================================== */

#ifdef DEBUG_MODE
#include "../log/log.h"
#define DBG(msg, ...) log_debug(msg, ##__VA_ARGS__)
#else
#define DBG(msg, ...)
#endif /* DEBUG_MODE */

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_BASE_H
