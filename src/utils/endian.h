#ifndef UTILS_ENDIAN_HEADER_
#define UTILS_ENDIAN_HEADER_

#include <stdint.h>
#include <stdlib.h>

/* ****************** utils::endian API ****************** */
int16_t endian_i16(int16_t num);
int32_t endian_i32(int32_t num);
int64_t endian_i64(int64_t num);
float endian_f32(float num);
/* ****************** utils::endian API ****************** */

#ifdef UTILS_ENDIAN_IMPLEMENTATION

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

static inline int isle() {
  uint16_t num = 0x1;
  return (*(uint8_t*)&num == 1);
}

int16_t endian_i16(int16_t num) {
  return isle() ? num : (int16_t)((num >> 8) | (num << 8));
}

int32_t endian_i32(int32_t num) {
  return isle() ? num
                : (int32_t)((num >> 24) | ((num >> 8) & 0x0000FF00) |
                            ((num << 8) & 0x00FF0000) | (num << 24));
}

int64_t endian_i64(int64_t num) {
  return isle() ? num
                : (int64_t)((num >> 56) | ((num >> 40) & 0x000000000000FF00LL) |
                            ((num >> 24) & 0x0000000000FF0000LL) |
                            ((num >> 8) & 0x00000000FF000000LL) |
                            ((num << 8) & 0x000000FF00000000LL) |
                            ((num << 24) & 0x0000FF0000000000LL) |
                            ((num << 40) & 0x00FF000000000000LL) | (num << 56));
}

float endian_f32(float num) {
  if (isle())
    return num;
  float result;
  char* src = (char*)&num;
  char* dst = (char*)&result;
  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
  return result;
}

#endif  // UTILS_ENDIAN_IMPLEMENTATION
#endif  // UTILS_ENDIAN_HEADER_
