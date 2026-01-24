#ifndef SEPI_WINDOWS_IO_H
#define SEPI_WINDOWS_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

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

#ifdef SEPI_WINDOWS_IO_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

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

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // SEPI_WINDOWS_IO_IMPLEMENTATION
#endif  // SEPI_WINDOWS_IO_H
