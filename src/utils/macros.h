#ifndef UTIL_COMMON_HEADER_
#define UTIL_COMMON_HEADER_

#define makesure(expr, msg, ...)                                    \
  do {                                                              \
    if (!(expr)) {                                                  \
      fprintf(stderr, "Fatal error: " msg "\nFile: %s\nLine: %d\n", \
              ##__VA_ARGS__, __FILE__, __LINE__);                   \
      abort();                                                      \
    }                                                               \
  } while (0)

#endif  // UTIL_COMMON_HEADER_
