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
  IO_ERR_FILE_TRUNCATE,
  IO_ERR_FILE_DESCRIPTOR,
  IO_ERR_SLURP,
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
  IO_SORT_DIRS_FIRST = (1 << 3),
};

typedef enum {
  IO_KIND_FILE = 1,
  IO_KIND_DIRECTORY,
  IO_KIND_OTHER,
  IO_KIND__COUNT,
} IOKind;

typedef struct IOFile IOFile;
struct IOFile {
  Str8 path;
  NDBuffer buffer;
  FILE* file;
  Sz file_size;
};

typedef struct IOItem IOItem;
struct IOItem {
  IOItem* parent;
  IOItem* children;
  Str8 name;
  Str8 path;
  Sz file_size;
  Sz total_files_size;
  U32 total_files_count;
  U16 children_count;
  Bool is_directory;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

IOError io_create_file(Arena* arena, Str8 path, IOFile* io_file);
IOError io_create_file_with_size(Arena* arena,
                                 Str8 path,
                                 Sz size,
                                 IOFile* io_file);
IOError io_open_file(Arena* arena, Str8 path, IOFile* io_file);
IOError io_close_file(IOFile* io_file);
IOError io_load_file(Arena* a, Str8 path, IOFile* io_file);
IOError io_get_file_size(Str8 path, Sz* out);
IOError io_dump_buffer_to_path(Str8 path, Buf8 data);
IOError io_slurp_path_to_buffer(Arena* arena, Str8 path, Buf8* data);
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

static inline U64
io_get_file_position(IOFile* io_file) {
  return ftell(io_file->file);
}

static inline Nothing
io_set_file_position(IOFile* io_file, U64 offset) {
  fseek(io_file->file, offset, SEEK_SET);
}

static inline Nothing
io_move_file_position(IOFile* io_file, U64 offset) {
  fseek(io_file->file, offset, SEEK_CUR);
}

static inline Sz
io_read_into_buffer(IOFile* io_file, Sz length, RawPtr buffer) {
  return fread(buffer, 1, length, io_file->file);
}

static inline Nothing
io_read_into_i16(IOFile* io_file, I16* out_value) {
  I16 temp_value;
  fread((RawPtr)&temp_value, 1, sizeof(I16), io_file->file);
  temp_value = nd_i16(temp_value);
  *(out_value) = temp_value;
}

static inline Nothing
io_read_into_i32(IOFile* io_file, I32* out_value) {
  I32 temp_value;
  fread((RawPtr)&temp_value, 1, sizeof(I32), io_file->file);
  temp_value = nd_i32(temp_value);
  *(out_value) = temp_value;
}

static inline Nothing
io_read_into_i64(IOFile* io_file, I64* out_values) {
  I64 temp_value;
  fread((RawPtr)&temp_value, 1, sizeof(I64), io_file->file);
  temp_value = nd_i64(temp_value);
  *(out_values) = temp_value;
}

static inline Nothing
io_read_into_f32(IOFile* io_file, F32* out_values) {
  F32 temp_value;
  fread((RawPtr)&temp_value, 1, sizeof(F32), io_file->file);
  temp_value = nd_f32(temp_value);
  *(out_values) = temp_value;
}

static inline Nothing
io_read_into_f64(IOFile* io_file, F64* out_values) {
  F64 temp_value;
  fread((RawPtr)&temp_value, 1, sizeof(F64), io_file->file);
  temp_value = nd_f64(temp_value);
  *(out_values) = temp_value;
}

static inline Sz
io_write_from_buffer(IOFile* io_file, Sz length, RawPtr buffer) {
  return fwrite(buffer, 1, length, io_file->file);
}

static inline Sz
io_write_from_i16(IOFile* io_file, I16* i16) {
  return fwrite((RawPtr)(i16), 1, sizeof(I16), io_file->file);
}

static inline Sz
io_write_from_i32(IOFile* io_file, I32* i32) {
  return fwrite((RawPtr)(i32), 1, sizeof(I32), io_file->file);
}

static inline Sz
io_write_from_i64(IOFile* io_file, I64* i64) {
  return fwrite((RawPtr)(i64), 1, sizeof(I64), io_file->file);
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

static I32 io_item_sort(const void* a, const void* b);

IOError
io_create_file(Arena* arena, Str8 path, IOFile* io_file) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(io_file != 0);

  IOError err = IO_ERR_SUCCESS;

  io_file->file = fopen(CS(path), "wb");
  if (0 == io_file->file) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  io_file->file_size = 0;
  io_file->path = str8_clone(arena, path);

cleanup:
  if (io_file->file && err != IO_ERR_SUCCESS) {
    fclose(io_file->file);
  }

  end_profiling();
  return err;
}

IOError
io_open_file(Arena* arena, Str8 path, IOFile* io_file) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(io_file != 0);

  IOError err = IO_ERR_SUCCESS;
  Bool is_file = FALSE;

  err = io_is_path_a_file(path, &is_file);
  if (err != IO_ERR_SUCCESS) {
    // NOTE:
    // we use 'the already set' error value
    goto cleanup;
  }

  io_file->file = fopen(CS(path), "rb");
  if (0 == io_file->file) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  // TODO: handle error cases
  fseek(io_file->file, 0, SEEK_END);

  io_file->file_size = (Sz)ftell(io_file->file);
  io_file->path = str8_clone(arena, path);

  // TODO: handle error cases
  rewind(io_file->file);

cleanup:
  if (io_file->file && err != IO_ERR_SUCCESS) {
    fclose(io_file->file);
  }

  end_profiling();
  return err;
}

IOError
io_close_file(IOFile* io_file) {
  start_profiling(1);

  assert(io_file != 0);

  IOError err = IO_ERR_SUCCESS;

  fflush(io_file->file);

  str8_reset(&io_file->path);
  if (!io_file->file) {
    fclose(io_file->file);
  }
  nd_reset(&io_file->buffer);
  zero_memory(io_file, sizeof(IOFile));

  end_profiling();
  return err;
}

IOError
io_load_file(Arena* arena, Str8 path, IOFile* io_file) {
  start_profiling(1);

  assert(arena != 0);
  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(io_file != 0);

  IOError err = IO_ERR_SUCCESS;
  Bool is_file = FALSE;

  err = io_is_path_a_file(path, &is_file);
  if (err != IO_ERR_SUCCESS) {
    // NOTE:
    // we return 'the-already-set' error value
    goto cleanup;
  }

  io_file->file = fopen(CS(path), "rb");
  if (0 == io_file->file) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  if (0 != fseek(io_file->file, 0, SEEK_END)) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  I64 file_size = ftell(io_file->file) * sizeof(U8);
  if (-1 == file_size) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  io_file->buffer.size = (Sz)file_size;

  // NOTE:
  // rewind has no return to error on!
  rewind(io_file->file);

  io_file->buffer.base =
      (CBuf)arena_push(arena, io_file->buffer.size, alignof(U8), FALSE);
  runtime_assert(io_file->buffer.base != 0);

  Sz rsize = fread((void*)io_file->buffer.base, 1, io_file->buffer.size,
                   io_file->file);
  if (rsize == io_file->buffer.size) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

cleanup:
  if (io_file->file) {
    fclose(io_file->file);
    io_file->file = 0;
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

IOError
io_slurp_path_to_buffer(Arena* arena, Str8 path, Buf8* data) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);

  IOError err = IO_ERR_SUCCESS;

  FILE* f = fopen(CS(path), "rb");
  if (0 == f) {
    err = IO_ERR_MKFILE;
    goto cleanup;
  }

  fseek(f, 0, SEEK_END);
  Sz size = ftell(f);
  rewind(f);

  U8* buf = arena_push(arena, sizeof(U8) * size, alignof(U8), TRUE);
  Sz read_size = fread(buf, 1, size, f);
  if (read_size != size) {
    err = IO_ERR_SLURP;
    goto cleanup;
  }

  *data = buf8(buf, size);

cleanup:
  if (f) {
    fclose(f);
  }
  end_profiling();
  return err;
}

static I32
io_item_sort(const void* a, const void* b) {
  const IOItem io_item_a = *(const IOItem*)a;
  const IOItem io_item_b = *(const IOItem*)b;

  if (io_item_a.is_directory && !io_item_b.is_directory) {
    return -1;
  }
  if (!io_item_a.is_directory && io_item_b.is_directory) {
    return 1;
  }

  return 0;
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
  IOItem* last = 0;
  Sz total_files_size = 0;
  U32 total_files_count = 0;

  err = _io_read_directory(arena, path, io_item, flags);
  if (IO_ERR_SUCCESS != err) {
    goto cleanup;
  }

  // TODO:
  // use scratch aren here
  StackIOItem* stack = stack_ioitem_create(arena);
  stack_ioitem_push(stack, io_item);

  while ((last = stack_ioitem_pop(stack))) {
    total_files_size += last->total_files_size;
    total_files_count += last->total_files_count;

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
  }

  io_item->total_files_size = total_files_size;
  io_item->total_files_count = total_files_count;

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
io_create_file_with_size(Arena* arena, Str8 path, Sz size, IOFile* io_file) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(SL(path) > 0);
  assert(size > 0);
  assert(io_file != 0);

  IOError err = IO_ERR_SUCCESS;

  err = io_create_file(arena, path, io_file);
  if (IO_ERR_SUCCESS != err) {
    goto cleanup;
  }

  I32 file_descriptor = fileno(io_file->file);
  if (-1 == file_descriptor) {
    err = IO_ERR_FILE_DESCRIPTOR;
    goto cleanup;
  }

  I32 result = ftruncate(file_descriptor, size);
  if (-1 == result) {
    err = IO_ERR_FILE_TRUNCATE;
    goto cleanup;
  }

  rewind(io_file->file);

cleanup:
  end_profiling();
  return err;
}

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

IOError
io_get_file_size(Str8 path, Sz* out) {
  start_profiling(1);

  assert(CS(path) != 0);
  assert(out != 0);

  IOError err = IO_ERR_SUCCESS;

  struct stat info = {0};
  if (-1 == stat(CS(path), &info)) {
    err = IO_ERR_STAT;
    goto cleanup;
  }

  if ((info.st_mode & S_IFMT) == S_IFDIR) {
    err = IO_ERR_NOT_FILE;
    goto cleanup;
  }

  *out = info.st_size;

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
    if (FALSE == children[children_index].is_directory) {
      Sz file_size;
      err = io_get_file_size(children[children_index].path, &file_size);
      if (IO_ERR_SUCCESS != err) {
        goto cleanup;
      }
      io_item->total_files_size += file_size;
      io_item->total_files_count++;
    }

    children_index++;
  } while (TRUE);

  Str8 temp_name;
  err = io_get_path_base_name(arena, path, &temp_name);
  if (IO_ERR_SUCCESS != err) {
    goto cleanup;
  }

  qsort(children, children_count, sizeof(IOItem), io_item_sort);

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
