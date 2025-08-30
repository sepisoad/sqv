#ifndef SEPI_COMMON_HEADER_
#define SEPI_COMMON_HEADER_

#define makesure(expr, msg, ...) \
  ((expr) ? (expr) : (log_fatal(msg, ##__VA_ARGS__), abort(), (expr)))

#define errorout(expr, err) \
  do {                      \
    if (!(expr)) {          \
      return (err);         \
    }                       \
  } while (0)

#define mustsucceed(expr)  \
  do {                     \
    int _ms_tmp_ = (expr); \
    if (_ms_tmp_ != 0) {   \
      return _ms_tmp_;     \
    }                      \
  } while (0)

#define notnull(val) makesure((val), "NULL")
#define notzero(val) makesure((val), "zero")
#define isvalid(val) makesure((val), "not valid")
#define mustdie(msg, ...) makesure(false, msg, ##__VA_ARGS__)

#ifdef DEBUG
#include "../log/log.h"
#define DBG(msg, ...) log_debug(msg, ##__VA_ARGS__)
#else
#define DBG(msg, ...)
#endif

#endif  // SEPI_COMMON_HEADER_
