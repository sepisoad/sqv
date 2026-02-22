#ifndef SEPI_IO_H
#define SEPI_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tracy/tracy.h>

#if defined(OS_LINUX)
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#elif defined(OS_MACOS)
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#elif defined(OS_WINDOWS)
#include <windows.h>
#endif

#include <sepi/base.h>
#include <sepi/arena.h>
#include <sepi/string.h>
#include <sepi/endian.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#if defined(OS_LINUX) || defined(OS_MACOS)
#define IO_PATH_SEPARATOR '/'

#elif defined(OS_WINDOWS)
#define IO_PATH_SEPARATOR '\\'

#endif

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  IO_ERR_SUCCESS = 1,
  IO_ERR_MKFILE,
  IO_ERR_MKDIR,
  IO_ERR_MKDIR_RECUR,
  IO_ERR_FILE_OPEN,
  IO_ERR_NOT_FILE,
  IO_ERR_NOT_DIR,
  IO_ERR_STAT,
  IO_ERR_OPENDIR,
  IO_ERR__COUNT,
} IOError;

typedef U32 IOFlags;
enum {
  IO_READ_DIR_RECURSIVE = (1 << 0),
  IO_READ_IGNORE_HIDDEN = (1 << 1),
  IO_MAKE_DIR_RECURSIVE = (1 << 2),
};

typedef enum {
  IO_KIND_FILE = 1,
  IO_KIND_DIRECTORY,
  IO_KIND_OTHER,
  IO_KIND__COUNT,
} IOKind;

typedef struct IOItem IOItem;
struct IOItem {
  IOItem* parent;
  IOItem* children;
  Str8 name;
  Str8 path;
  FILE* file;
  NDBuffer buffer;
  U16 children_count;
  Bool is_directory;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

IOError io_open_file(Arena* arena, Str8 path, IOItem* io_item);
IOError io_close_file(IOItem* io_item);
IOError io_load_file(Arena* a, Str8 path, IOItem* io_item);
IOError io_dump_buffer_to_path(Str8 path, Buf8 data);
IOError io_is_path_a_file(Str8 path, Bool* is_file);
IOError io_is_path_a_directory(Str8 path, Bool* is_dir);
IOError io_make_directory(Str8 path, IOFlags flags);
IOError io_read_directory(Arena* arena,
                          Str8 path,
                          IOItem* io_item,
                          IOFlags flags);
IOError io_get_directory_children_count(Str8 path, U32* count, IOFlags flags);
IOError io_get_path_base_name(Arena* arena, Str8 path, Str8* out);
IOError io_get_path_directory_name(Arena* arena, Str8 path, Str8* out);

#define IO_POS(n /* IOItem* */) ftell((n->file))
#define IO_SET(n /* IOItem* */, ofs /* U32 */) fseek((n->file), (ofs), SEEK_SET)
#define IO_MOVE(n /* IOItem* */, sz /* Sz */) fseek((n->file), (sz), SEEK_CUR)

#define IO_BUF(n /* IOItem* */, len /* U32 */, buf /* CBuf */) \
  fread((RawPtr)(buf), 1, (len), ((n)->file))

#define IO_I16(n /* IOItem* */, num /* I16* */)         \
  {                                                     \
    I16 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(I16), ((n)->file)); \
    tmp = nd_i16(tmp);                                  \
    *(num) = tmp;                                       \
  }

#define IO_I32(n /* IOItem* */, num /* I32* */)         \
  {                                                     \
    I32 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(I32), ((n)->file)); \
    tmp = nd_i32(tmp);                                  \
    *(num) = tmp;                                       \
  }

#define IO_I64(n /* IOItem* */, num /* I64* */)         \
  {                                                     \
    I64 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(I64), ((n)->file)); \
    tmp = nd_i64(tmp);                                  \
    *(num) = tmp;                                       \
  }

#define IO_F32(n /* IOItem* */, num /* F32* */)         \
  {                                                     \
    F32 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(F32), ((n)->file)); \
    tmp = nd_f32(tmp);                                  \
    *(num) = tmp;                                       \
  }

#define IO_F64(n /* IOItem* */, num /* F64* */)         \
  {                                                     \
    F64 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(F64), ((n)->file)); \
    tmp = nd_f64(tmp);                                  \
    *(num) = tmp;                                       \
  }

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_IO_IMPLEMENTATION

mount_slave_profiling_context();

#define SEPI_STACK_IOITEM_IMPLEMENTATION
#include <sepi/generated/stack_ioitem.h>

static IOError _io_read_directory(Arena* arena,
                                  Str8 path,
                                  IOItem* io_item,
                                  IOFlags flags);
static IOError _io_read_directory_recursively(Arena* arena,
                                              Str8 path,
                                              IOItem* io_item,
                                              IOFlags flags);
static IOError _io_make_directory(Str8 path);
static IOError _io_make_directory_recursively(Str8 path);

IOError
io_open_file(Arena* arena, Str8 path, IOItem* io_item) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(io_item != 0);

  IOError err = IO_ERR_SUCCESS;
  Bool is_file = FALSE;

  err = io_is_path_a_file(path, &is_file);
  if (err != IO_ERR_SUCCESS) {
    // NOTE:
    // we use 'the already set' error value
    goto cleanup;
  }

  io_item->file = fopen(CS(path), "rb");
  if (0 == io_item->file) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  io_item->is_directory = FALSE;
  io_item->name = str8_clone(arena, path);
  io_item->path = str8_clone(arena, path);

cleanup:
  if (io_item->file && err != IO_ERR_SUCCESS) {
    fclose(io_item->file);
  }

  end_profiling();
  return err;
}

IOError
io_close_file(IOItem* io_item) {
  start_profiling(1);

  assert(io_item != 0);

  IOError err = IO_ERR_SUCCESS;

  str8_reset(&io_item->name);
  str8_reset(&io_item->path);
  io_item->is_directory = FALSE;
  if (!io_item->file) {
    fclose(io_item->file);
  }
  nd_reset(&io_item->buffer);
  zero_memory(io_item, sizeof(IOItem));

  // cleanup: // NOTE: silence compiler warning
  end_profiling();
  return err;
}

IOError
io_load_file(Arena* arena, Str8 path, IOItem* io_item) {
  start_profiling(1);

  assert(arena != 0);
  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(io_item != 0);

  IOError err = IO_ERR_SUCCESS;
  Bool is_file = FALSE;

  err = io_is_path_a_file(path, &is_file);
  if (err != IO_ERR_SUCCESS) {
    // NOTE:
    // we return 'the-already-set' error value
    goto cleanup;
  }

  io_item->file = fopen(CS(path), "rb");
  if (0 == io_item->file) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  if (0 != fseek(io_item->file, 0, SEEK_END)) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  I64 file_size = ftell(io_item->file) * sizeof(U8);
  if (-1 == file_size) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  io_item->buffer.size = (Sz)file_size;

  // NOTE:
  // rewind has no return to error on!
  rewind(io_item->file);

  io_item->buffer.base =
      (CBuf)arena_push(arena, io_item->buffer.size, alignof(U8), FALSE);
  runtime_assert(io_item->buffer.base != 0);

  Sz rsize = fread((void*)io_item->buffer.base, 1, io_item->buffer.size,
                   io_item->file);
  if (rsize == io_item->buffer.size) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  io_item->is_directory = FALSE;

cleanup:
  if (io_item->file) {
    fclose(io_item->file);
    io_item->file = 0;
  }

  end_profiling();
  return IO_ERR_SUCCESS;
}

IOError
io_dump_buffer_to_path(Str8 path, Buf8 data) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(data.cbuf != 0);
  assert(data.size > 0);

  IOError err = IO_ERR_SUCCESS;

  FILE* f = fopen(CS(path), "wb");
  if (0 == f) {
    err = IO_ERR_MKFILE;
    goto cleanup;
  }

  Sz write_size = fwrite(data.cbuf, 1, data.size, f);
  if (write_size != data.size) {
    err = IO_ERR_MKFILE;
    goto cleanup;
  }

  fflush(f);

cleanup:
  if (f) {
    fclose(f);
  }
  end_profiling();
  return err;
}

static IOError
_io_make_directory_recursively(Str8 path) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);

  IOError err = IO_ERR_SUCCESS;
  U32 original_length = SL(path);

  for (U32 index = 0; index < SL(path); index++) {
    if (CS(path)[index] == IO_PATH_SEPARATOR) {
      SL(path) = index;
      Str hack = (Str)&CS(path)[index];
      *hack = 0;
      err = _io_make_directory(path);
      SL(path) = original_length;
      *hack = IO_PATH_SEPARATOR;
      if (IO_ERR_SUCCESS != err) {
        err = IO_ERR_MKDIR_RECUR;
        goto cleanup;
      }
    }
  }

  err = _io_make_directory(path);
  if (IO_ERR_SUCCESS != err) {
    err = IO_ERR_MKDIR_RECUR;
    goto cleanup;
  }

cleanup:
  end_profiling();
  return err;
}

IOError
io_make_directory(Str8 path, IOFlags flags) {
  if (flags & IO_MAKE_DIR_RECURSIVE) {
    return _io_make_directory_recursively(path);
  } else {
    return _io_make_directory(path);
  }
}

IOError
io_read_directory(Arena* arena, Str8 path, IOItem* io_item, IOFlags flags) {
  if (flags & IO_READ_DIR_RECURSIVE) {
    return _io_read_directory_recursively(arena, path, io_item, flags);
  } else {
    return _io_read_directory(arena, path, io_item, flags);
  }
}

static IOError
_io_read_directory_recursively(Arena* arena,
                               Str8 path,
                               IOItem* io_item,
                               IOFlags flags) {
  start_profiling(1);

  assert(arena != 0);
  assert(CS(path) != 0);
  assert(SL(path) > 0);

  IOError err = IO_ERR_SUCCESS;

  err = _io_read_directory(arena, path, io_item, flags);
  if (IO_ERR_SUCCESS != err) {
    goto cleanup;
  }

  U16 stack_length = 0;
  StackIOItem* stack = stack_ioitem_create(arena);

  IOItem* last = io_item;
  while (last) {
    for (U16 index = 0; index < last->children_count; index++) {
      IOItem* child = last->children + index;
      if (child->is_directory) {
        Str8 new_path =
            str8_join(arena, last->path, child->name, IO_PATH_SEPARATOR);
        err = _io_read_directory(arena, new_path, child, flags);
        if (IO_ERR_SUCCESS != err) {
          goto cleanup;
        }
        stack_ioitem_push(stack, child);
      }
    }
    last = stack_ioitem_pop(stack);
  }

cleanup:
  // stack_ioitem_clean(stack);
  end_profiling();
  return err;
}

IOError
io_get_path_base_name(Arena* arena, Str8 path, Str8* out) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(out != 0);

  IOError err = IO_ERR_SUCCESS;
  U32 last_index = SL(path) - 1;  // index of the last character
  U32 last_segment_index = 0;

  if (IO_PATH_SEPARATOR == CS(path)[last_index]) {
    last_index--;
  }

  for (U32 index = last_index; index >= 0; index--) {
    if (CS(path)[index] == IO_PATH_SEPARATOR) {
      last_segment_index = index + 1;
      break;
    }
  }

  *out = str8_clone(arena, S(path.cstr + last_segment_index));

  end_profiling();
  return err;
}

IOError
io_get_path_directory_name(Arena* arena, Str8 path, Str8* out) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(out != 0);

  IOError err = IO_ERR_SUCCESS;
  U32 last_index = SL(path) - 1;  // index of the last character
  U32 last_segment_index = last_index;

  for (U32 index = last_index; index >= 0; index--) {
    if (CS(path)[index] == IO_PATH_SEPARATOR) {
      last_segment_index = index;
      break;
    }
  }

  *out = str8_slice(arena, path, 0, last_segment_index);

  end_profiling();
  return err;
}

/* ===================================================== */
/*                         LINUX                         */
/* ===================================================== */

#if defined(OS_LINUX)

IOError
io_is_path_a_file(Str8 path, Bool* is_file) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);
  assert(is_file != 0);

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
  end_profiling();
  return err;
}

IOError
io_is_path_a_directory(Str8 path, Bool* is_dir) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);
  assert(is_dir != 0);

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
  end_profiling();
  return err;
}

static IOError
_io_read_directory(Arena* arena, Str8 path, IOItem* parent, IOFlags flags) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);
  assert(parent != 0);

  not_implemented();

  IOError err = IO_ERR_SUCCESS;

cleanup:
  end_profiling();
  return err;
}

IOError
io_get_directory_children_count(Str8 path, U32* count, IOFlags flags) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);
  assert(count != 0);

  not_implemented();

  IOError err = IO_ERR_SUCCESS;

cleanup:
  end_profiling();
  return err;
}

static IOError
_io_make_directory(Str8 path) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);

  IOError err = IO_ERR_SUCCESS;

  if (mkdir((char*)CS(path), 0755) == -1) {
    if (EEXIST != errno) {
      err = IO_ERR_MKDIR;
      goto cleanup;
    }
  }

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */
/*                         MACOS                         */
/* ===================================================== */

#elif defined(OS_MACOS)

IOError
io_is_path_a_file(Str8 path, Bool* is_file) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(is_file != 0);

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
  end_profiling();
  return err;
}

IOError
io_is_path_a_directory(Str8 path, Bool* is_dir) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(is_dir != 0);

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
  end_profiling();
  return err;
}

static IOError
_io_read_directory(Arena* arena, Str8 path, IOItem* io_item, IOFlags flags) {
  start_profiling(1);

  assert(arena != 0);
  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(io_item != 0);

  IOError err = IO_ERR_SUCCESS;
  DIR* dir = 0;
  Bool is_dir = FALSE;

  io_is_path_a_directory(path, &is_dir);
  if (is_dir == FALSE) {
    err = IO_ERR_NOT_DIR;
    goto cleanup;
  }

  U32 children_count = 0;
  err = io_get_directory_children_count(path, &children_count, flags);
  if (IO_ERR_SUCCESS != err) {
    goto cleanup;
  }

  IOItem* children =
      arena_push(arena, sizeof(IOItem) * children_count, alignof(IOItem), TRUE);

  dir = opendir(CS(path));
  if (0 == dir) {
    err = IO_ERR_OPENDIR;
    goto cleanup;
  }

  U32 children_index = 0;
  do {
    struct dirent* ent = readdir(dir);
    if (0 == ent) {
      break;
    }

    if (DT_DIR != ent->d_type && DT_REG != ent->d_type) {
      // NOTE: if the item is not a file or directory,
      // we don't care and don't process it. RIGHT?
      // for example a device node or a sym-link! RIGHT?
      continue;
    }

    if (flags & IO_READ_IGNORE_HIDDEN) {
      if (0 == str8_find_first(S(ent->d_name), '.')) {
        continue;
      }
    }

    children[children_index].name = str8_clone(arena, S(ent->d_name));
    children[children_index].path =
        str8_join(arena, path, S(ent->d_name), IO_PATH_SEPARATOR);
    children[children_index].is_directory = (DT_DIR == ent->d_type);
    children[children_index].parent = io_item;
    children_index++;
  } while (TRUE);

  Str8 temp_name;
  err = io_get_path_base_name(arena, path, &temp_name);
  if (IO_ERR_SUCCESS != err) {
    goto cleanup;
  }

  io_item->children = children;
  io_item->children_count = children_count;
  io_item->name = str8_clone(arena, temp_name);
  io_item->path = str8_clone(arena, path);
  io_item->is_directory = TRUE;

cleanup:
  if (dir) {
    closedir(dir);
  }

  end_profiling();
  return err;
}

IOError
io_get_directory_children_count(Str8 path, U32* count, IOFlags flags) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.length > 0);
  assert(count != 0);

  IOError err = IO_ERR_SUCCESS;
  DIR* dir = 0;
  Bool is_dir = FALSE;

  io_is_path_a_directory(path, &is_dir);
  if (is_dir == FALSE) {
    err = IO_ERR_NOT_DIR;
    goto cleanup;
  }

  dir = opendir(CS(path));
  if (0 == dir) {
    err = IO_ERR_OPENDIR;
    goto cleanup;
  }

  struct dirent* iter = 0;
  *count = 0;
  while ((iter = readdir(dir))) {
    if (flags & IO_READ_IGNORE_HIDDEN) {
      if (0 == str8_find_first(S(iter->d_name), '.')) {
        continue;
      }
    }

    (*count)++;
  }

cleanup:
  if (dir) {
    closedir(dir);
  }
  end_profiling();
  return err;
}

static IOError
_io_make_directory(Str8 path) {
  start_profiling(1);

  assert(CS(path) != 0);

  IOError err = IO_ERR_SUCCESS;

  // NOTE: is this an error!?
  if (SL(path) <= 0) {
    goto cleanup;
  }

  Bool is_dir = FALSE;
  // NOTE: we probably don't want to check the error, why?
  // because if the path that we want to create does not
  // exist then we get an error which is to be expected!
  io_is_path_a_directory(path, &is_dir);
  if (is_dir) {
    goto cleanup;
  }

  if (mkdir((char*)CS(path), 0755) == -1) {
    if (EEXIST != errno) {
      err = IO_ERR_MKDIR;
      goto cleanup;
    }
  }

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */
/*                        WINDOWS                        */
/* ===================================================== */

#elif defined(OS_WINDOWS)

IOError
io_is_path_a_file(Str8 path, Bool* is_file) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);
  assert(is_file != 0);

  IOError err = IO_ERR_SUCCESS;

  DWORD attr = GetFileAttributesW((WCHAR*)CS(path));
  if (attr == INVALID_FILE_ATTRIBUTES) {
    return IO_ERR_STAT;
  }

  *is_file = (attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE)) != 0;

cleanup:
  end_profiling();
  return err;
}

IOError
io_is_path_a_directory(Str8 path, Bool* is_dir) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);
  assert(is_dir != 0);

  IOError err = IO_ERR_SUCCESS;

  DWORD attr = GetFileAttributesW((WCHAR*)CS(path));
  if (attr == INVALID_FILE_ATTRIBUTES) {
    return IO_ERR_STAT;
  }

  *is_dir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;

cleanup:
  end_profiling();
  return err;
}

static IOError
_io_make_directory(Str8 path) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);

  IOError err = IO_ERR_SUCCESS;

  WIN32_FILE_ATTRIBUTE_DATA attributes = {0};
  GetFileAttributesExW((WCHAR*)CS(path), GetFileExInfoStandard, &attributes);
  if (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    err = IO_ERR_MKDIR;
    goto cleanup;
  } else if (CreateDirectoryW((WCHAR*)CS(path), 0)) {
    err = IO_ERR_MKDIR;
    goto cleanup;
  }

cleanup:
  end_profiling();
  return err;
}

static IOError
_io_read_directory(Arena* arena, Str8 path, IOItem* parent, IOFlags flags) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);
  assert(parent != 0);
  not_implemented();

  IOError err = IO_ERR_SUCCESS;

cleanup:
  end_profiling();
  return err;
}

IOError
io_get_directory_children_count(Str8 path, U32* count, IOFlags flags) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(path.size > 0);
  assert(count != 0);

  not_implemented();

  IOError err = IO_ERR_SUCCESS;

cleanup:
  end_profiling();
  return err;
}

#endif

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_IO_IMPLEMENTATION */
#endif /* SEPI_IO_H */
