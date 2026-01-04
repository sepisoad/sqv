#ifndef SEPI_IO_H
#define SEPI_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(OS_LINUX) || defined(OS_MAC)
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#else  // OS_WINDOWS
#include <windows.h>
#endif

#include "../tracy/tracy.h"

#include "base.h"
#include "string.h"
#include "endian.h"
#include "arena.h"
#include "hashmap.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#if defined(OS_LINUX) || defined(OS_MAC)
#define IO_PATH_SEPARATOR '/'
#else
#define IO_PATH_SEPARATOR '\\'
#endif

#define IO_PATH_MAX_LENGTH 2048

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  IO_ERR_SUCCESS = 1,
  IO_ERR_MKFILE,
  IO_ERR_MKDIR,
  IO_ERR_STAT,
  IO_ERR_OPENDIR,
  IO_ERR__COUNT,
} IOError;

typedef enum {
  IO_KIND_FILE = 1,
  IO_KIND_DIRECTORY,
  IO_KIND_OTHER,
  IO_KIND__COUNT,
} IOKind;

typedef struct IONode IONode;

struct IONode {
  Str8 name;
  Sz size;
  Bool is_directory;
  IONode* parent;
  HashMap* children;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

IOError io_load_file(Arena*, CStr, NDBuffer*);
IOError io_dump(Str8 path, Str8 data);
IOError io_is_file(Str8 path, Bool* is_dir);
IOError io_is_directory(Str8 path, Bool* is_dir);
IOError io_make_directory(Str8 path);
IOError io_make_nested_directory(Str8 path);
IOError io_directory_children(Str8 path, HashMap* children);

#define IO_POS(f) ftell((f))
#define IO_SET(f, ofs) fseek((f), (ofs), SEEK_SET)
#define IO_MOVE(f, sz) fseek((f), (sz), SEEK_CUR)

#define IO_BUF(f /* FILE* */, len /* U32 */, buf /* CBuf */) \
  fread((RawPtr)(buf), 1, (len), (f))

#define IO_I16(f /* FILE* */, num /* I16* */)   \
  {                                             \
    I16 tmp;                                    \
    fread((RawPtr) & tmp, 1, sizeof(I16), (f)); \
    tmp = nd_i16(tmp);                          \
    *(num) = tmp;                               \
  }

#define IO_I32(f /* FILE* */, num /* I32* */)   \
  {                                             \
    I32 tmp;                                    \
    fread((RawPtr) & tmp, 1, sizeof(I32), (f)); \
    tmp = nd_i32(tmp);                          \
    *(num) = tmp;                               \
  }

#define IO_I64(f /* FILE* */, num /* I64* */)   \
  {                                             \
    I64 tmp;                                    \
    fread((RawPtr) & tmp, 1, sizeof(I64), (f)); \
    tmp = nd_i64(tmp);                          \
    *(num) = tmp;                               \
  }

#define IO_F32(f /* FILE* */, num /* F32* */)   \
  {                                             \
    F32 tmp;                                    \
    fread((RawPtr) & tmp, 1, sizeof(F32), (f)); \
    tmp = nd_f32(tmp);                          \
    *(num) = tmp;                               \
  }

#define IO_F64(f /* FILE* */, num /* F64* */)   \
  {                                             \
    F64 tmp;                                    \
    fread((RawPtr) & tmp, 1, sizeof(F64), (f)); \
    tmp = nd_f64(tmp);                          \
    *(num) = tmp;                               \
  }

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_IO_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

// TODO:
// refactor this function, in fact i don't use this function
// maybe eve delete this function
IOError
io_load_file(Arena* arena, CStr path, NDBuffer* ndb) {
  TracyCZoneN(trcyctx, "io_load_file", 1);

  Assert(arena != 0);
  Assert(path != 0);
  Assert(ndb != 0);

  FILE* f;
  {
    TracyCZoneN(trcyctx, "io_load_file::fopen", 2);
    f = fopen(path, "rb");
    TracyCZoneEnd(trcyctx);
  }

  AssertAlways(f != 0);

  {
    TracyCZoneN(trcyctx, "io_load_file::fseek", 2);
    TracyCZoneEnd(trcyctx);
  }

  {
    TracyCZoneN(trcyctx, "io_load_file::ftell", 2);
    ndb->size = ftell(f) * sizeof(U8);
    TracyCZoneEnd(trcyctx);
  }

  {
    TracyCZoneN(trcyctx, "io_load_file::rewind", 2);
    rewind(f);
    TracyCZoneEnd(trcyctx);
  }

  ndb->base = (CBuf)arena_push(arena, ndb->size, AlignOf(U8), FALSE);
  AssertAlways(ndb->base != 0);

  Sz rsize;
  {
    TracyCZoneN(trcyctx, "io_load_file::fread", 2);
    rsize = fread((void*)ndb->base, 1, ndb->size, f);
    TracyCZoneEnd(trcyctx);
  }

  AssertAlways(rsize == ndb->size);

  if (f) {
    TracyCZoneN(trcyctx, "io_load_file::fclose", 2);
    fclose(f);
    TracyCZoneEnd(trcyctx);
  }

  TracyCZoneEnd(trcyctx);
  return IO_ERR_SUCCESS;
}

IOError
io_dump(Str8 path, Str8 data) {
  TracyCZoneN(trcyctx, "io_dump", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  Assert(data.cstr != 0);
  Assert(data.size > 0);

  IOError err = IO_ERR_SUCCESS;

  FILE* f = fopen(path.cstr, "wb");
  if (0 == f) {
    err = IO_ERR_MKFILE;
    goto cleanup;
  }

  Sz write_size = fwrite(data.cstr, 1, data.size, f);
  if (write_size != data.size) {
    err = IO_ERR_MKFILE;
    goto cleanup;
  }

  fflush(f);

cleanup:
  if (f) {
    fclose(f);
  }
  TracyCZoneEnd(trcyctx);
  return err;
}

IOError
io_directory_children(Str8 path, HashMap* children) {
  TracyCZoneN(trcyctx, "io_directory_children", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  Assert(children != 0);

  IOError err = IO_ERR_SUCCESS;

  DIR *dir = opendir(path.cstr);
  if (!dir) {
    err = IO_ERR_OPENDIR;
    goto cleanup;
  }

cleanup:
  if (dir) {
    closedir(dir);
  }
  TracyCZoneEnd(trcyctx);
  return err;
}

#if defined(OS_LINUX) || defined(OS_MAC)

IOError
io_is_file(Str8 path, Bool* is_file) {
  TracyCZoneN(trcyctx, "io_is_file", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  Assert(is_file != 0);

  IOError err = IO_ERR_SUCCESS;

  struct stat info = {0};
  if (-1 == stat(path.cstr, &info)) {
    err = IO_ERR_STAT;
    goto cleanup;
  }

  if ((info.st_mode & S_IFMT) == S_IFREG) {
    *is_file = TRUE;
  } else {
    *is_file = FALSE;
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

IOError
io_is_directory(Str8 path, Bool* is_dir) {
  TracyCZoneN(trcyctx, "io_is_directory", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  Assert(is_dir != 0);

  IOError err = IO_ERR_SUCCESS;

  struct stat info = {0};
  if (-1 == stat(path.cstr, &info)) {
    err = IO_ERR_STAT;
    goto cleanup;
  }

  if ((info.st_mode & S_IFMT) == S_IFDIR) {
    *is_dir = TRUE;
  } else {
    *is_dir = FALSE;
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

IOError
io_make_directory(Str8 path) {
  TracyCZoneN(trcyctx, "io_make_directory", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);

  IOError err = IO_ERR_SUCCESS;

  if (mkdir((char*)path.cstr, 0755) == -1) {
    if (EEXIST != errno) {
      err = IO_ERR_MKDIR;
      goto cleanup;
    }
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

#else /* OS_WINDOWS */

IOError
io_is_file(Str8 path, Bool* is_file) {
  TracyCZoneN(trcyctx, "io_is_file", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  Assert(is_file != 0);

  IOError err = IO_ERR_SUCCESS;

  DWORD attr = GetFileAttributesW((WCHAR*)path.cstr);
  if (attr == INVALID_FILE_ATTRIBUTES) {
    return IO_ERR_STAT;
  }

  *is_file = (attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE)) != 0;

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

IOError
io_is_directory(Str8 path, Bool* is_dir) {
  TracyCZoneN(trcyctx, "io_is_directory", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  Assert(is_dir != 0);

  IOError err = IO_ERR_SUCCESS;

  DWORD attr = GetFileAttributesW((WCHAR*)path.cstr);
  if (attr == INVALID_FILE_ATTRIBUTES) {
    return IO_ERR_STAT;
  }

  *is_dir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

IOError
io_make_directory(Str8 path) {
  TracyCZoneN(trcyctx, "io_make_directory", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);

  IOError err = IO_ERR_SUCCESS;

  WIN32_FILE_ATTRIBUTE_DATA attributes = {0};
  GetFileAttributesExW((WCHAR*)path.cstr, GetFileExInfoStandard, &attributes);
  if (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    err = IO_ERR_MKDIR;
    goto cleanup;
  } else if (CreateDirectoryW((WCHAR*)path.cstr, 0)) {
    err = IO_ERR_MKDIR;
    goto cleanup;
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

IOError
io_directory_children(Str8 path, HashMap* children) {
  TracyCZoneN(trcyctx, "io_directory_children", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  Assert(children != 0);
  NotImplemented();

  IOError err = IO_ERR_SUCCESS;

// cleanup:
//   TracyCZoneEnd(trcyctx);
//   return err;
}


#endif

IOError
io_make_nested_directory(Str8 path) {
  TracyCZoneN(trcyctx, "io_make_nested_directory", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  AssertAlways(path.size < IO_PATH_MAX_LENGTH);

  IOError err = IO_ERR_SUCCESS;
  char clone[IO_PATH_MAX_LENGTH] = {0};

  for (U32 index = 0; index < path.size; index++) {
    clone[index] = path.cstr[index];
    if (path.cstr[index] == IO_PATH_SEPARATOR) {
      err = io_make_directory(str8(clone));
      if (err != IO_ERR_SUCCESS) {
        goto cleanup;
      }
    }
  }

  io_make_directory(path);
  if (err != IO_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_IO_IMPLEMENTATION
#endif  // SEPI_IO_H
