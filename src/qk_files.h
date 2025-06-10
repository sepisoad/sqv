#ifndef QK_FILES_HEADER_
#define QK_FILES_HEADER_

#include <stdio.h>

#include "../deps/sepi_types.h"
#include "../deps/sepi_macros.h"

typedef enum {
  QK_FILE_UNKNOWN = -1,
  QK_FILE_QUAKE1_PAK = 0,
  QK_FILE_QUAKE1_WAD,
  QK_FILE_QUAKE1_MDL,
} qk_file_type;

/* ****************** API ****************** */
qk_file_type qk_file_guess_type_from_path(cstr);
qk_file_type qk_file_guess_type_from_buffer(cstr);
/* ****************** API ****************** */

#ifdef QK_FILES_IMPLEMENTATION

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

static constexpr u32 MAXBUFSIZE = 8;

static qk_file_type guess_file_type(cstr buf) {
  if (strncmp(buf, "PACK", 4) == 0) {
    return QK_FILE_QUAKE1_PAK;
  }
  if (strncmp(buf, "IDPO", 4) == 0) {
    return QK_FILE_QUAKE1_MDL;
  }
  return QK_FILE_UNKNOWN;
}

qk_file_type qk_file_guess_type_from_path(cstr path) {
  char buf[MAXBUFSIZE + 1] = {0};

  FILE* f = fopen(path, "rb");
  notnull(f);
  notzero(!fseek(f, 0, SEEK_END));
  isvalid(notzero(ftell(f)) >= MAXBUFSIZE);
  rewind(f);
  isvalid(fread(buf, sizeof(char), MAXBUFSIZE, f) == MAXBUFSIZE);

  return guess_file_type(buf);
}

qk_file_type qk_file_guess_type_from_buffer(cstr data) {
  char buf[9] = {0};

  notzero(!memcpy(buf, data, 8));

  return guess_file_type(buf);
}

#endif  // QK_FILES_IMPLEMENTATION
#endif  // QK_FILES_HEADER_
