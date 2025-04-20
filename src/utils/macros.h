#ifndef UTIL_COMMON_HEADER_
#define UTIL_COMMON_HEADER_

#include "../../deps/log.h"

static char error_buffer[1024];  // removed const so snprintf can write into it

#define makesure(expr, msg, ...)     \
  ({                                 \
    typeof(expr) _val = (expr);      \
    if (!_val) {                     \
      log_fatal(msg, ##__VA_ARGS__); \
      abort();                       \
    }                                \
    _val;                            \
  })

#define notnull(val) makesure((val), "NULL")
#define notzero(val) makesure((val), "zero")
#define mustdie(msg, ...) makesure(false, msg, ##__VA_ARGS__)

#ifdef DEBUG
#define DBG(msg, ...) log_debug(msg, ##__VA_ARGS__)
#else
#define DBG(msg, ...)
#endif

#endif  // UTIL_COMMON_HEADER_
