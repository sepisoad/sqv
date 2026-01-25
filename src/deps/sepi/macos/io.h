#ifndef SEPI_MACOS_IO_H
#define SEPI_MACOS_IO_H

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
#include "../array.h"
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

#ifdef SEPI_MACOS_IO_IMPLEMENTATION

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
io_directory_children(Arena* arena, Str8 path, IONode* node) {
  TracyCZoneN(trcyctx, "io_directory_children", 1);

  Assert(path.cstr != 0);
  Assert(path.size > 0);
  Assert(node != 0);
  Assert(node->children == 0);

  IOError err = IO_ERR_SUCCESS;
  DIR* dir = 0;
  Bool is_dir = FALSE;

  io_is_directory(path, &is_dir);
  if (is_dir == FALSE) {
    err = IO_ERR_NOT_DIR;
    goto cleanup;
  }

  dir = opendir(path.cstr);
  if (0 == dir) {
    err = IO_ERR_OPENDIR;
    goto cleanup;
  }

  node->children = array_create(arena, sizeof(IONode), AlignOf(IONode));

  Str8 dot_node_name = str8(".");
  Str8 dot_dot_node_name = str8("..");

  do {
    struct dirent* ent = readdir(dir);
    if (0 == ent) {
      break;
    }

    if (DT_DIR != ent->d_type && DT_REG != ent->d_type) {
      // NOTE: if the item is not a file or directory,
      // we don't care and don't process it. RIGHT?
      continue;
    }

    if (str8_is_equal(dot_node_name, str8(ent->d_name)) ||
        str8_is_equal(dot_dot_node_name, str8(ent->d_name))) {
      continue;
    }

    IONode* child = arena_push(arena, sizeof(IONode), AlignOf(IONode), TRUE);
    child->name = str8_arena(arena, ent->d_name);
    child->is_directory = (DT_DIR == ent->d_type);
    child->parent = node;

    array_push(node->children, child);
  } while (TRUE);

  node->name = str8_arena(arena, path.cstr);
  node->is_directory = TRUE;

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

  IOError err = IO_ERR_SUCCESS;

  // NOTE: is this an error!?
  if (path.size <= 0) {
    goto cleanup;
  }

  Bool is_dir = FALSE;
  // NOTE: we probably don't want to check the error, why?
  // because if the path that we want to create does not
  // exist then we get an error which is to be expected!
  io_is_directory(path, &is_dir);
  if (is_dir) {
    goto cleanup;
  }

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

#endif  // SEPI_MACOS_IO_IMPLEMENTATION
#endif  // SEPI_MACOS_IO_H
