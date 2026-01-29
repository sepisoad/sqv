#ifndef SEPI_COMMON_IO_H
#define SEPI_COMMON_IO_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../base.h"
#include "../string.h"
#include "../endian.h"
#include "../array.h"
#include "../stack.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

extern const I8 IO_PATH_SEPARATOR;

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  IO_ERR_SUCCESS = 1,
  IO_ERR_MKFILE,
  IO_ERR_MKDIR,
  IO_ERR_MKDIR_RECUR,
  IO_ERR_NOT_FILE,
  IO_ERR_NOT_DIR,
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
  Str8 path;
  Bool is_directory;
  IONode* parent;
  ArrayOf(IONode*) children;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

IOError io_load_file(Arena*, CStr, NDBuffer*);
IOError io_dump(Str8 path, Buf8 data);
IOError io_is_file(Str8 path, Bool* is_file);
IOError io_is_directory(Str8 path, Bool* is_dir);
IOError io_make_directory(Str8 path);
IOError io_make_nested_directory(Str8 path);
IOError io_directory_children(Arena* arena, Str8 path, IONode* node);
IOError io_directory_nested_children(Arena* arena, Str8 path, IONode* node);

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

#ifdef SEPI_COMMON_IO_IMPLEMENTATION

IOError
io_load_file(Arena* arena, CStr path, NDBuffer* ndb) {
  START_PROFILING(1);

  Assert(arena != 0);
  Assert(path != 0);
  Assert(ndb != 0);

  FILE* f;
  f = fopen(path, "rb");

  AssertAlways(f != 0);
  fseek(f, 0, SEEK_END);

  ndb->size = ftell(f) * sizeof(U8);
  rewind(f);

  ndb->base = (CBuf)arena_push(arena, ndb->size, AlignOf(U8), FALSE);
  AssertAlways(ndb->base != 0);

  Sz rsize;
  rsize = fread((void*)ndb->base, 1, ndb->size, f);

  AssertAlways(rsize == ndb->size);

  if (f) {
    fclose(f);
  }

  END_PROFILING();
  return IO_ERR_SUCCESS;
}

IOError
io_dump(Str8 path, Buf8 data) {
  START_PROFILING(1);

  Assert(path.cstr != 0);
  Assert(path.length > 0);
  Assert(data.cbuf != 0);
  Assert(data.size > 0);

  IOError err = IO_ERR_SUCCESS;

  FILE* f = fopen(path.cstr, "wb");
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
  END_PROFILING();
  return err;
}

IOError
io_make_nested_directory(Str8 path) {
  START_PROFILING(1);

  Assert(path.cstr != 0);
  Assert(path.length > 0);

  IOError err = IO_ERR_SUCCESS;
  U32 original_length = path.length;

  for (U32 index = 0; index < path.length; index++) {
    if (path.cstr[index] == IO_PATH_SEPARATOR) {
      path.length = index;
      Str hack = (Str)&path.cstr[index];
      *hack = 0;
      err = io_make_directory(path);
      path.length = original_length;
      *hack = IO_PATH_SEPARATOR;
      if (IO_ERR_SUCCESS != err) {
        err = IO_ERR_MKDIR_RECUR;
        goto cleanup;
      }
    }
  }

  err = io_make_directory(path);
  if (IO_ERR_SUCCESS != err) {
    err = IO_ERR_MKDIR_RECUR;
    goto cleanup;
  }

cleanup:
  END_PROFILING();
  return err;
}

IOError
io_directory_nested_children(Arena* arena, Str8 path, IONode* node) {
  START_PROFILING(1);

  Assert(arena != 0);
  Assert(path.cstr != 0);
  Assert(path.length > 0);

  IOError err = IO_ERR_SUCCESS;
  StackOf(IONode*) nodes = 0;

  err = io_directory_children(arena, path, node);
  if (IO_ERR_SUCCESS != err) {
    goto cleanup;
  }

  nodes = stack_create(arena);
  // StackOf(Str8*) paths = stack_create(arena);
  //

  IONode* last = node;

  do {
    for (U32 index = 0; index < last->children->offset; index++) {
      IONode* child = array_get(last->children, index);
      if (child->is_directory) {
        Str8 new_path =
            str8_join(arena, last->path, child->name, IO_PATH_SEPARATOR);
        err = io_directory_children(arena, new_path, child);
        if (IO_ERR_SUCCESS != err) {
          goto cleanup;
        }
        stack_push(nodes, child);
      }
    }
    last = stack_pop(nodes);
  } while (last != 0);


cleanup:
  if (nodes) {
    stack_destroy(nodes);
  }

  END_PROFILING();
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_COMMON_IO_IMPLEMENTATION */
#endif /* SEPI_COMMON_IO_H */
