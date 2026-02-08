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
  IO_ERR_FILE_OPEN,
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
  FILE* file;
  NDBuffer buffer;
  IONode* parent;
  ArrayOf(IONode*) children;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

IOError io_open_file(Arena* arena, Str8 path, IONode* node);
IOError io_close_file(IONode* node);
IOError io_load_file(Arena* a, Str8 path, IONode* node);
IOError io_dump(Str8 path, Buf8 data);
IOError io_is_file(Str8 path, Bool* is_file);
IOError io_is_directory(Str8 path, Bool* is_dir);
IOError io_make_directory(Str8 path);
IOError io_make_nested_directory(Str8 path);
IOError io_directory_children(Arena* arena, Str8 path, IONode* node);
IOError io_directory_nested_children(Arena* arena, Str8 path, IONode* node);
IOError io_get_path_base_name(Str8 path, Str8* name);

#define IO_POS(n /* IONode* */) ftell((n->file))
#define IO_SET(n /* IONode* */, ofs /* U32 */) fseek((n->file), (ofs), SEEK_SET)
#define IO_MOVE(n /* IONode* */, sz /* Sz */) fseek((n->file), (sz), SEEK_CUR)

#define IO_BUF(n /* IONode* */, len /* U32 */, buf /* CBuf */) \
  fread((RawPtr)(buf), 1, (len), ((n)->file))

#define IO_I16(n /* IONode* */, num /* I16* */)         \
  {                                                     \
    I16 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(I16), ((n)->file)); \
    tmp = nd_i16(tmp);                                  \
    *(num) = tmp;                                       \
  }

#define IO_I32(n /* IONode* */, num /* I32* */)         \
  {                                                     \
    I32 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(I32), ((n)->file)); \
    tmp = nd_i32(tmp);                                  \
    *(num) = tmp;                                       \
  }

#define IO_I64(n /* IONode* */, num /* I64* */)         \
  {                                                     \
    I64 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(I64), ((n)->file)); \
    tmp = nd_i64(tmp);                                  \
    *(num) = tmp;                                       \
  }

#define IO_F32(n /* IONode* */, num /* F32* */)         \
  {                                                     \
    F32 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(F32), ((n)->file)); \
    tmp = nd_f32(tmp);                                  \
    *(num) = tmp;                                       \
  }

#define IO_F64(n /* IONode* */, num /* F64* */)         \
  {                                                     \
    F64 tmp;                                            \
    fread((RawPtr)(&tmp), 1, sizeof(F64), ((n)->file)); \
    tmp = nd_f64(tmp);                                  \
    *(num) = tmp;                                       \
  }

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_COMMON_IO_IMPLEMENTATION

DefineStack(IONode*, IONode, io_node);

IOError
io_open_file(Arena* arena, Str8 path, IONode* node) {
  START_PROFILING(1);

  Assert(CS(path) != 0);
  Assert(SL(path) > 0);
  Assert(node != 0);

  IOError err = IO_ERR_SUCCESS;
  Bool is_file = FALSE;

  err = io_is_file(path, &is_file);
  if (err != IO_ERR_SUCCESS) {
    // NOTE:
    // we use 'the already set' error value
    goto cleanup;
  }

  node->file = fopen(CS(path), "rb");
  if (0 == node->file) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  node->is_directory = FALSE;
  node->name = str8_clone(arena, path);
  node->path = str8_clone(arena, path);

cleanup:
  if (node->file && err != IO_ERR_SUCCESS) {
    fclose(node->file);
  }

  END_PROFILING();
  return err;
}

IOError
io_close_file(IONode* node) {
  START_PROFILING(1);

  Assert(node != 0);

  IOError err = IO_ERR_SUCCESS;

  str8_reset(&node->name);
  str8_reset(&node->path);
  node->is_directory = FALSE;
  if (!node->file) {
    fclose(node->file);
    node->file = 0;
  }
  nd_reset(&node->buffer);
  node->parent = 0;
  node->children = 0;

cleanup:
  END_PROFILING();
  return err;
}

IOError
io_load_file(Arena* arena, Str8 path, IONode* node) {
  START_PROFILING(1);

  Assert(arena != 0);
  Assert(CS(path) != 0);
  Assert(SL(path) > 0);
  Assert(node != 0);

  IOError err = IO_ERR_SUCCESS;
  Bool is_file = FALSE;

  err = io_is_file(path, &is_file);
  if (err != IO_ERR_SUCCESS) {
    // NOTE:
    // we use 'the already set' error value
    goto cleanup;
  }

  node->file = fopen(CS(path), "rb");
  if (0 == node->file) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  if (0 != fseek(node->file, 0, SEEK_END)) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  node->buffer.size = ftell(node->file) * sizeof(U8);
  if (-1l == node->buffer.size) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  // NOTE:
  // rewind has no return to error on!
  rewind(node->file);

  node->buffer.base =
      (CBuf)arena_push(arena, node->buffer.size, AlignOf(U8), FALSE);
  AssertAlways(node->buffer.base != 0);

  Sz rsize = fread((void*)node->buffer.base, 1, node->buffer.size, node->file);
  if (rsize == node->buffer.size) {
    err = IO_ERR_FILE_OPEN;
    goto cleanup;
  }

  node->is_directory = FALSE;

cleanup:
  if (node->file) {
    fclose(node->file);
    node->file = 0;
  }

  END_PROFILING();
  return IO_ERR_SUCCESS;
}

IOError
io_dump(Str8 path, Buf8 data) {
  START_PROFILING(1);

  Assert(CS(path) != 0);
  Assert(SL(path) > 0);
  Assert(data.cbuf != 0);
  Assert(data.size > 0);

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
  END_PROFILING();
  return err;
}

IOError
io_make_nested_directory(Str8 path) {
  START_PROFILING(1);

  Assert(CS(path) != 0);
  Assert(SL(path) > 0);

  IOError err = IO_ERR_SUCCESS;
  U32 original_length = SL(path);

  for (U32 index = 0; index < SL(path); index++) {
    if (CS(path)[index] == IO_PATH_SEPARATOR) {
      SL(path) = index;
      Str hack = (Str)&CS(path)[index];
      *hack = 0;
      err = io_make_directory(path);
      SL(path) = original_length;
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
  Assert(CS(path) != 0);
  Assert(SL(path) > 0);

  IOError err = IO_ERR_SUCCESS;
  StackIONode stack = stack_io_node_make(arena);

  err = io_directory_children(arena, path, node);
  if (IO_ERR_SUCCESS != err) {
    goto cleanup;
  }

  IONode* last = node;

  do {
    if (last->children != NULL) {
      for (U32 index = 0; index < last->children->offset; index++) {
        IONode* child = array_get(last->children, index);
        if (child->is_directory) {
          Str8 new_path =
              str8_join(arena, last->path, child->name, IO_PATH_SEPARATOR);
          err = io_directory_children(arena, new_path, child);
          if (IO_ERR_SUCCESS != err) {
            goto cleanup;
          }
          stack_io_node_push(&stack, child);
        }
      }
    }
    last = stack_io_node_pop(&stack);
  } while (last != 0);

cleanup:
  stack_io_node_clean(&stack);
  END_PROFILING();
  return err;
}

IOError
io_get_path_base_name(Str8 path, Str8* name) {
  START_PROFILING(1);

  Assert(CS(path) != 0);
  Assert(SL(path) > 0);
  Assert(name != 0);

  IOError err = IO_ERR_SUCCESS;
  U32 last_index = SL(path) - 1; // index of the last character
  U32 last_segment_index = 0;

  if (IO_PATH_SEPARATOR == CS(path)[last_index]) {
    last_index--;
  }
// /Users/sepi/Games/Quake1/lq/
  for (U32 index = last_index; index >= 0; index--) {
    if (CS(path)[index] == IO_PATH_SEPARATOR) {
      last_segment_index = index + 1;
      break;
    }
  }

  name->cstr = &CS(path)[last_segment_index];
  name->length = strlen(name->cstr);

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_COMMON_IO_IMPLEMENTATION */
#endif /* SEPI_COMMON_IO_H */
