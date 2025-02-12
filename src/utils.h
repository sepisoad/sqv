#ifndef UTILS_HEADER_
#define UTILS_HEADER_

#include <stdlib.h>
#include <stdint.h>

static int isle() {
  uint16_t num = 0x1;
  return (*(uint8_t *)&num == 1);
}

int16_t endian_i16(int16_t num) {
  if (isle()) {
    return num;
  } else {
    return (int16_t)((num >> 8) | (num << 8));
  }
}

int32_t endian_i32(int32_t num) {
  if (isle()) {
    return num;
  } else {
    return (int32_t)((num >> 24) | ((num >> 8) & 0x0000FF00) |
                     ((num << 8) & 0x00FF0000) | (num << 24));
  }
}

int64_t endian_i64(int64_t num) {
  if (isle()) {
    return num;
  } else {
    return (int64_t)((num >> 56) | ((num >> 40) & 0x000000000000FF00LL) |
                     ((num >> 24) & 0x0000000000FF0000LL) |
                     ((num >> 8) & 0x00000000FF000000LL) |
                     ((num << 8) & 0x000000FF00000000LL) |
                     ((num << 24) & 0x0000FF0000000000LL) |
                     ((num << 40) & 0x00FF000000000000LL) | (num << 56));
  }
}

float endian_f32(float num) {
  if (isle()) {
    return num;
  } else {
    float result;
    char *src = (char *)&num;
    char *dst = (char *)&result;
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
    return result;
  }
}

#define abortif(expr, msg, ...)                                                \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "Fatal error: " msg "\nFile: %s\nLine: %d\n",            \
              ##__VA_ARGS__, __FILE__, __LINE__);                              \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#endif // UTILS_HEADER_
