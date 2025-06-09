#ifndef SEPI_ENDIAN_HEADER_
#define SEPI_ENDIAN_HEADER_

#include <stdlib.h>
#include <stdbool.h>

#include "sepi_types.h"

/* ****************** API ****************** */
i16 endian_i16(i16 num);
i32 endian_i32(i32 num);
i64 endian_i64(i64 num);
f32 endian_f32(f32 num);
/* ****************** API ****************** */

#ifdef SEPI_ENDIAN_IMPLEMENTATION

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

static inline bool isle() {
  u16 num = 0x1;
  return (*(u8*)&num == 1);
}

i16 endian_i16(i16 num) {
  return isle() ? num : (i16)((num >> 8) | (num << 8));
}

i32 endian_i32(i32 num) {
  return isle() ? num
                : (i32)((num >> 24) | ((num >> 8) & 0x0000FF00) |
                        ((num << 8) & 0x00FF0000) | (num << 24));
}

i64 endian_i64(i64 num) {
  return isle() ? num
                : (i64)((num >> 56) | ((num >> 40) & 0x000000000000FF00LL) |
                        ((num >> 24) & 0x0000000000FF0000LL) |
                        ((num >> 8) & 0x00000000FF000000LL) |
                        ((num << 8) & 0x000000FF00000000LL) |
                        ((num << 24) & 0x0000FF0000000000LL) |
                        ((num << 40) & 0x00FF000000000000LL) | (num << 56));
}

f32 endian_f32(f32 num) {
  if (isle())
    return num;
  f32 result;
  char* src = (char*)&num;
  char* dst = (char*)&result;
  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
  return result;
}

#endif  // SEPI_ENDIAN_IMPLEMENTATION
#endif  // SEPI_ENDIAN_HEADER_
