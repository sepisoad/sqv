#ifndef SEPI_LINUX_IO_H
#define SEPI_LINUX_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "../../tracy/tracy.h"

#include "../base.h"
#include "../string.h"
#include "../endian.h"
#include "../arena.h"
#include "../hashmap.h"
#include "../common/io.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

// --

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

// --

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// --

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_LINUX_IO_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;
const I8 IO_PATH_SEPARATOR = '/';

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
io_directory_children(Arena* arena, Str8 path, HashMap* children) {
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

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_LINUX_IO_IMPLEMENTATION
#endif  // SEPI_LINUX_IO_H
