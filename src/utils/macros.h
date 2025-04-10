#ifndef UTIL_COMMON_HEADER_
#define UTIL_COMMON_HEADER_

#include "../../deps/log.h"

static char error_buffer[1024];  // removed const so snprintf can write into it

#define makesureOld(expr, msg, ...)                                         \
  do {                                                                      \
    if (!(expr)) {                                                          \
      fprintf(stderr, "Fatal error: " msg "\nFile: %s:%d\n", ##__VA_ARGS__, \
              __FILE__, __LINE__);                                          \
      abort();                                                              \
    }                                                                       \
  } while (0)

#define makesure(expr, msg, ...)     \
  do {                               \
    if (!(expr)) {                   \
      log_fatal(msg, ##__VA_ARGS__); \
      abort();                       \
    }                                \
  } while (0)

#endif  // UTIL_COMMON_HEADER_
