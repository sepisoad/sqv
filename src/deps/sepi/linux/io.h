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

SLAVE_PROFILING_CONTEXT;

const I8 IO_PATH_SEPARATOR = '/';

IOError
io_is_file(Str8 path, Bool* is_file) {
  START_PROFILING(1);

  Assert(CS(path) != 0);
  Assert(path.size > 0);
  Assert(is_file != 0);

  IOError err = IO_ERR_SUCCESS;

  struct stat info = {0};
  if (-1 == stat(CS(path), &info)) {
    err = IO_ERR_STAT;
    goto cleanup;
  }

  if ((info.st_mode & S_IFMT) == S_IFREG) {
    *is_file = TRUE;
  } else {
    *is_file = FALSE;
  }

cleanup:
  END_PROFILING();
  return err;
}

IOError
io_is_directory(Str8 path, Bool* is_dir) {
  START_PROFILING(1);

  Assert(CS(path) != 0);
  Assert(path.size > 0);
  Assert(is_dir != 0);

  IOError err = IO_ERR_SUCCESS;

  struct stat info = {0};
  if (-1 == stat(CS(path), &info)) {
    err = IO_ERR_STAT;
    goto cleanup;
  }

  if ((info.st_mode & S_IFMT) == S_IFDIR) {
    *is_dir = TRUE;
  } else {
    *is_dir = FALSE;
  }

cleanup:
  END_PROFILING();
  return err;
}

IOError
io_directory_children(Arena* arena, Str8 path, HashMap* children) {
  START_PROFILING(1);

  Assert(CS(path) != 0);
  Assert(path.size > 0);
  Assert(children != 0);

  IOError err = IO_ERR_SUCCESS;

  DIR *dir = opendir(CS(path));
  if (!dir) {
    err = IO_ERR_OPENDIR;
    goto cleanup;
  }

cleanup:
  if (dir) {
    closedir(dir);
  }
  END_PROFILING();
  return err;
}

IOError
io_make_directory(Str8 path) {
  START_PROFILING(1);

  Assert(CS(path) != 0);
  Assert(path.size > 0);

  IOError err = IO_ERR_SUCCESS;

  if (mkdir((char*)CS(path), 0755) == -1) {
    if (EEXIST != errno) {
      err = IO_ERR_MKDIR;
      goto cleanup;
    }
  }

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_LINUX_IO_IMPLEMENTATION
#endif  // SEPI_LINUX_IO_H
