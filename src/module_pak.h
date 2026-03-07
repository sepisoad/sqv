/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#ifndef MODULE_PAK_HEADER
#define MODULE_PAK_HEADER

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <string.h>
#include <stdlib.h>

#include <sepi/arena.h>
#include <sepi/endian.h>
#include <sepi/string.h>
#include <sepi/io.h>
#include <sepi/generated/stack_ioitem.h>

#define SEPI_MAP_PAKITEM_IMPLEMENTATION
#define SEPI_MAP_PAKCOUNTER_IMPLEMENTATION
#define SEPI_STACK_PAKITEM_IMPLEMENTATION
#include "module_kind.h"
#include "generated/map_pakitem.h"
#include "generated/map_pakcounter.h"
#include "generated/stack_pakitem.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define PAK_MAGIC_CODE_LEN 4
#define PAK_ENTRY_NAME_LEN 56
#define PAK_ENTRY_MAX_EXTRACT_PATH_LEN 2048
#define PAK_MAX_ERROR_LENGTH 512

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

DefineFStr8(PAK_ENTRY_NAME_LEN);

typedef enum {
  PAK_ERR_SUCCESS,
  PAK_ERR_MALFORMED,
  PAK_ERR_INVALID_ENTRY_PATH,
  PAK_ERR_OUT_DIR_NOT_FOUND,
  PAK_ERR_EXTRACT,
  PAK_ERR_PATH_LENGTH_TOO_LONG,
  PAK_ERR_PAK_CREATION_ERROR,
  PAK_ERR_ITEM_ZERO_SIZE,
  PAK_ERR__COUNT,
} PakError;

typedef struct PakCounter PakCounter;
struct PakCounter {
  Str8 path;
  U16 number;
};

typedef struct PakRawEntry PakRawEntry;
struct PakRawEntry {
  char name[PAK_ENTRY_NAME_LEN];
  I32 offset;
  I32 size;
};

typedef struct PakItem PakItem;
struct PakItem {
  PakItem* parent;
  PakItem** children;
  Str8 name;
  Str8 path;
  I32 data_size;
  I32 data_offset;
  U16 children_count;
  Bool is_directory;
};

typedef struct Pak Pak;
struct Pak {
  PakItem* items;
  Arena* arena;
  U16 items_count;
};

typedef struct FL512_Str8 FL512_Str8;
static inline Nothing fl512_str8_set(FL512_Str8* f, const char* str);

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError pak_load_from_io_file(Pak* pak, IOFile* io_file);
Nothing pak_unload(Pak* pak);
PakError pak_extract(Pak* pak, IOFile* io_file, Str8 out_dir);
PakError pak_extract_item(Pak* pak,
                          PakItem* pak_item_for_path,
                          IOFile* io_file,
                          Str8 out_dir);
PakError pak_make_from_ioitem(Arena* arena,
                              IOItem* io_item,
                              Str8 base_path,
                              Str8 out_path,
                              FL512_Str8* out_err);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_PAK_IMPLEMENTATION

mount_slave_profiling_context();

// TODO:
// use Str8
static U32
pak_get_path_depth(CStr path, U32 length) {
  start_profiling(1);

  assert(path != 0);
  assert(length > 0);

  U32 depth = 0;

  for (U32 index = 0; index < length; index++) {
    if (path[index] == 0) {
      break;
    }

    if (path[index] == '/') {
      depth = depth + 1;
    }
  }

  end_profiling();
  return depth;
}

/* ===================================================== */

// TODO:
// use Str8
static Nothing
pak_get_path_at_depth(CStr path,
                      U32 length,
                      U32 depth,
                      char out[PAK_ENTRY_NAME_LEN]) {
  start_profiling(1);

  assert(path != 0);
  assert(out != 0);

  U32 traversed = 0;

  for (U32 index = 0; index < length; index++) {
    if (path[index] == 0 || traversed >= depth) {
      break;
    }

    if (path[index] == '/') {
      traversed++;
    }

    out[index] = path[index];
  }

  end_profiling();
}

// TODO:
// use Str8
static Nothing
pak_get_name_at_depth(CStr path,
                      U32 length,
                      U32 depth,
                      char out[PAK_ENTRY_NAME_LEN]) {
  start_profiling(1);

  assert(path != 0);
  assert(out != 0);

  U32 traversed = 0;

  for (U32 idxsrc = 0, idxdst = 0; idxsrc < length; idxsrc++) {
    if (path[idxsrc] == 0 || traversed > depth) {
      break;
    }

    if (path[idxsrc] == '/') {
      traversed++;
      continue;
    }

    if (traversed == depth) {
      out[idxdst++] = path[idxsrc];
    }
  }

  end_profiling();
}

/* ===================================================== */

static I32
pak_item_sort(const void* a, const void* b) {
  const PakItem* pak_item_a = *(const PakItem**)a;
  const PakItem* pak_item_b = *(const PakItem**)b;

  if (pak_item_a->is_directory && !pak_item_b->is_directory) {
    return -1;
  }
  if (!pak_item_a->is_directory && pak_item_b->is_directory) {
    return 1;
  }

  return 0;
}

/* ===================================================== */

static PakError
pak_read_entries_from_io_file(Pak* pak, IOFile* io_file) {
  start_profiling(1);

  assert(pak != 0);
  assert(io_file != 0);
  assert(io_file->file != 0);

  PakError err = PAK_ERR_SUCCESS;
  Arena* arena = pak->arena;

  MapPakCounter map_counts = map_pakcounter_make(arena, 64);
  U64 io_file_offset = io_get_file_position(io_file);
  PakCounter* root_counter =
      arena_push(pak->arena, sizeof(PakCounter), alignof(PakCounter), TRUE);

  root_counter->path = S(".");
  map_pakcounter_push(&map_counts, root_counter->path, root_counter);

  for (U32 index = 0; index < pak->items_count; index++) {
    char entry_str[PAK_ENTRY_NAME_LEN] = {0};

    io_read_into_buffer(io_file, PAK_ENTRY_NAME_LEN, entry_str);
    io_move_file_position(io_file, sizeof(I32));
    io_move_file_position(io_file, sizeof(I32));

    U32 path_depth = pak_get_path_depth(entry_str, PAK_ENTRY_NAME_LEN);
    PakCounter* last_counter = root_counter;
    for (U32 depth_index = 0; depth_index < path_depth + 1; depth_index++) {
      char path[PAK_ENTRY_NAME_LEN] = {0};
      char name[PAK_ENTRY_NAME_LEN] = {0};

      pak_get_path_at_depth(entry_str, PAK_ENTRY_NAME_LEN, depth_index + 1,
                            &path[0]);
      pak_get_name_at_depth(path, strlen(path), depth_index, &name[0]);
      PakCounter* existing_counter = map_pakcounter_get(&map_counts, S(path));
      if (existing_counter) {
        last_counter = existing_counter;
        continue;
      }
      last_counter->number++;

      PakCounter* new_counter =
          arena_push(pak->arena, sizeof(PakCounter), alignof(PakCounter), TRUE);
      new_counter->path = str8_clone(arena, S(path));
      map_pakcounter_push(&map_counts, S(path), new_counter);
      last_counter = new_counter;
    }
  }

  io_set_file_position(io_file, io_file_offset);

  MapPakItem map_items = map_pakitem_make(arena, 64);
  PakItem* root_pak_item =
      arena_push(arena, sizeof(PakItem), alignof(PakItem), TRUE);

  pak->items = root_pak_item;

  root_pak_item->is_directory = TRUE;
  root_pak_item->name = str8_clone(arena, S("."));
  root_pak_item->path = str8_clone(arena, S("."));
  root_pak_item->children = arena_push(
      arena, sizeof(PakItem) * root_counter->number, alignof(PakItem), TRUE);

  map_pakitem_push(&map_items, S("."), root_pak_item);

  PakItem* last_pak_item = 0;
  for (U32 index = 0; index < pak->items_count; index++) {
    char entry_str[PAK_ENTRY_NAME_LEN] = {0};
    I32 entry_size = 0;
    I32 entry_offset = 0;

    io_read_into_buffer(io_file, PAK_ENTRY_NAME_LEN, entry_str);
    io_read_into_i32(io_file, &entry_offset);
    io_read_into_i32(io_file, &entry_size);

    U32 path_depth = pak_get_path_depth(entry_str, PAK_ENTRY_NAME_LEN);
    last_pak_item = root_pak_item;
    for (U32 depth_index = 0; depth_index < path_depth + 1; depth_index++) {
      char path[PAK_ENTRY_NAME_LEN] = {0};
      char name[PAK_ENTRY_NAME_LEN] = {0};

      pak_get_path_at_depth(entry_str, PAK_ENTRY_NAME_LEN, depth_index + 1,
                            &path[0]);
      pak_get_name_at_depth(path, strlen(path), depth_index, &name[0]);
      PakItem* existing_pak_item = map_pakitem_get(&map_items, S(path));
      if (existing_pak_item) {
        last_pak_item = existing_pak_item;
        continue;
      }

      const PakCounter* const counter =
          map_pakcounter_get(&map_counts, S(path));
      assert(0 != counter);

      PakItem* new_pak_item =
          arena_push(arena, sizeof(PakItem), alignof(PakItem), TRUE);

      map_pakitem_push(&map_items, S(path), new_pak_item);

      if (counter->number) {
        new_pak_item->children = arena_push(
            arena, sizeof(PakItem) * counter->number, alignof(PakItem), TRUE);
      }

      new_pak_item->name = str8_clone(pak->arena, S(name));
      new_pak_item->path = str8_clone(pak->arena, S(path));
      new_pak_item->parent = last_pak_item;

      last_pak_item->children[last_pak_item->children_count++] = new_pak_item;
      if (depth_index >= path_depth) {
        new_pak_item->data_size = entry_size;
        new_pak_item->data_offset = entry_offset;
      } else {
        new_pak_item->is_directory = TRUE;
        last_pak_item = new_pak_item;
      }
    }
  }

  ArenaScratch sortmem = arena_scratch_begin(pak->arena);
  StackPakItem* sortstack = stack_pakitem_create(sortmem.arena);

  stack_pakitem_push(sortstack, pak->items);
  while ((last_pak_item = stack_pakitem_pop(sortstack))) {
    qsort(last_pak_item->children, last_pak_item->children_count,
          sizeof(PakItem*), pak_item_sort);
    U16 children_count = last_pak_item->children_count;
    for (U32 index = 0; index < children_count; index++) {
      PakItem* current_child = last_pak_item->children[index];
      if (TRUE == current_child->is_directory) {
        stack_pakitem_push(sortstack, current_child);
      }
    }
  }

  arena_scratch_end(sortmem);
  end_profiling();
  return err;
}

/* ===================================================== */

PakError
pak_load_from_io_file(Pak* pak, IOFile* io_file) {
  start_profiling(1);

  assert(pak != 0);
  assert(io_file != 0);

  PakError err = PAK_ERR_SUCCESS;

  U8 magic_code[PAK_MAGIC_CODE_LEN] = {0};
  I32 offset = 0;
  I32 size = 0;

  pak->arena = arena_create();

  io_read_into_buffer(io_file, PAK_MAGIC_CODE_LEN, magic_code);
  io_read_into_i32(io_file, &offset);
  io_read_into_i32(io_file, &size);
  io_set_file_position(io_file, offset);

  if ((offset <= 0) || (size <= 0) || (magic_code[0] != 'P') ||
      (magic_code[1] != 'A') || (magic_code[2] != 'C') ||
      (magic_code[3] != 'K')) {
    err = PAK_ERR_MALFORMED;
    goto cleanup;
  }

  pak->items_count = size / sizeof(PakRawEntry);
  err = pak_read_entries_from_io_file(pak, io_file);
  if (err != PAK_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */

Nothing
pak_unload(Pak* pak) {
  start_profiling(1);

  if (pak->arena) {
    arena_destroy(pak->arena);
  }

  end_profiling();
}

/* ===================================================== */

PakError
pak_extract(Pak* pak, IOFile* io_file, Str8 out_dir) {
  start_profiling(1);

  PakError err = PAK_ERR_SUCCESS;
  ArenaScratch scratch = arena_scratch_begin(pak->arena);
  PakItem* pak_item_for_path = 0;
  StackPakItem* stack = stack_pakitem_create(scratch.arena);

  stack_pakitem_push(stack, pak->items);

  while ((pak_item_for_path = stack_pakitem_pop(stack))) {
    U16 children_count = pak_item_for_path->children_count;
    for (U32 index = 0; index < children_count; index++) {
      PakItem* current_child = pak_item_for_path->children[index];
      Str8 full_path_str = str8_join(scratch.arena, out_dir,
                                     current_child->path, IO_PATH_SEPARATOR);

      if (TRUE == current_child->is_directory) {
        stack_pakitem_push(stack, current_child);

        IOError ioerr = io_make_directory(full_path_str, IO_MAKE_DIR_RECURSIVE);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
      } else {
        U8* src_buffer = arena_push(scratch.arena, current_child->data_size,
                                    alignof(U8), TRUE);

        io_set_file_position(io_file, current_child->data_offset);
        io_read_into_buffer(io_file, current_child->data_size, src_buffer);
        io_dump_buffer_to_path(full_path_str,
                               buf8(src_buffer, current_child->data_size));
      }
    }
  }

cleanup:
  stack_pakitem_clean(stack);
  arena_scratch_end(scratch);
  end_profiling();
  return err;
}

/* ===================================================== */

PakError
pak_extract_item(Pak* pak,
                 PakItem* pak_item_for_path,
                 IOFile* io_file,
                 Str8 out_dir) {
  start_profiling(1);

  PakError err = PAK_ERR_SUCCESS;
  ArenaScratch scratch = arena_scratch_begin(pak->arena);
  StackPakItem* stack = stack_pakitem_create(scratch.arena);

  stack_pakitem_push(stack, pak_item_for_path);

  if (0 == pak_item_for_path->children) {
    if (pak_item_for_path->is_directory) {
      // fuck, it's an empty directory, we won't do anything here
      goto cleanup;
    }
    Str8 full_path_str = str8_join(scratch.arena, out_dir,
                                   pak_item_for_path->name, IO_PATH_SEPARATOR);
    U8* src_buffer = arena_push(scratch.arena, pak_item_for_path->data_size,
                                alignof(U8), TRUE);

    io_set_file_position(io_file, pak_item_for_path->data_offset);
    io_read_into_buffer(io_file, pak_item_for_path->data_size, src_buffer);
    io_dump_buffer_to_path(full_path_str,
                           buf8(src_buffer, pak_item_for_path->data_size));
    goto cleanup;
  }

  while (stack->length > 0) {
    pak_item_for_path = stack_pakitem_pop(stack);
    if (0 == pak_item_for_path) {
      break;
    }

    U32 children_count = pak_item_for_path->children_count;
    for (U32 index = 0; index < children_count; index++) {
      PakItem* current_child = pak_item_for_path->children[index];
      if (0 == current_child) {
        err = PAK_ERR_EXTRACT;
        goto cleanup;
      }

      Str8 full_path_str = str8_join(scratch.arena, out_dir,
                                     current_child->path, IO_PATH_SEPARATOR);

      if (TRUE == current_child->is_directory) {
        stack_pakitem_push(stack, current_child);

        IOError ioerr = io_make_directory(full_path_str, IO_MAKE_DIR_RECURSIVE);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
      } else {
        Str8 base_dir;
        IOError ioerr =
            io_get_path_directory_name(scratch.arena, full_path_str, &base_dir);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
        ioerr = io_make_directory(base_dir, IO_MAKE_DIR_RECURSIVE);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }

        U8* src_buffer = arena_push(scratch.arena, current_child->data_size,
                                    alignof(U8), TRUE);

        io_set_file_position(io_file, current_child->data_offset);
        io_read_into_buffer(io_file, current_child->data_size, src_buffer);
        io_dump_buffer_to_path(full_path_str,
                               buf8(src_buffer, current_child->data_size));
      }
    }
  }

cleanup:
  stack_pakitem_clean(stack);
  arena_scratch_end(scratch);
  end_profiling();
  return err;
}

/* ===================================================== */

PakError
pak_make_from_ioitem(Arena* arena,
                     IOItem* io_item,
                     Str8 base_path,
                     Str8 out_path,
                     FL512_Str8* out_err) {
  start_profiling(1);

  assert(arena != 0);
  assert(io_item != 0);
  assert(out_err != 0);

  // TODO:
  // do i need out_err?
  ignore(out_err);

  PakError err = PAK_ERR_SUCCESS;
  ArenaScratch fnmem = arena_scratch_begin(arena);

  const IOItem* current_directory = 0;
  StackIOItem* stack_directories = stack_ioitem_create(fnmem.arena);

  stack_ioitem_push(stack_directories, io_item);

  I32 files_table_offset = (4 + sizeof(I32) + sizeof(I32));
  I32 files_table_size = io_item->total_files_count * sizeof(PakRawEntry);
  I32 current_data_position = files_table_offset + files_table_size;
  Sz total_size = current_data_position + io_item->total_files_size;

  IOFile pak_io_file = {0};
  IOError ioerr =
      io_create_file_with_size(fnmem.arena, out_path, total_size, &pak_io_file);
  if (IO_ERR_SUCCESS != ioerr) {
    err = PAK_ERR_PAK_CREATION_ERROR;
    goto cleanup;
  }

  Str8 pak_file_id = str8("PACK");
  io_write_from_buffer(&pak_io_file, pak_file_id.length,
                       (RawPtr)pak_file_id.cstr);
  io_write_from_i32(&pak_io_file, &files_table_offset);
  io_write_from_i32(&pak_io_file, &files_table_size);
  while (0 != (current_directory = stack_ioitem_pop(stack_directories))) {
    IOItem* children = current_directory->children;
    for (U16 index = 0; index < current_directory->children_count; index++) {
      if (children[index].is_directory) {
        stack_ioitem_push(stack_directories, (children + index));
      } else {
        ArenaScratch loopmem = arena_scratch_begin(fnmem.arena);

        FL56_Str8 clean_name_ =
            fl56_str8(children[index].path.cstr + (base_path.length + 1));

        Sz pak_item_size = 0;
        ioerr = io_get_file_size(children[index].path, &pak_item_size);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_PAK_CREATION_ERROR;
          goto cleanup;
        }

        if (pak_item_size == 0) {
          err = PAK_ERR_ITEM_ZERO_SIZE;
          goto cleanup;
        }

        io_write_from_buffer(&pak_io_file, PAK_ENTRY_NAME_LEN,
                             (RawPtr)clean_name_.cstr);
        io_write_from_i32(&pak_io_file, &current_data_position);
        io_write_from_i32(&pak_io_file, (I32*)&pak_item_size);

        U64 saved_position = io_get_file_position(&pak_io_file);
        io_set_file_position(&pak_io_file, current_data_position);

        Buf8 buf = {0};
        io_slurp_path_to_buffer(loopmem.arena, children[index].path, &buf);
        io_write_from_buffer(&pak_io_file, pak_item_size, (RawPtr)buf.cbuf);
        io_set_file_position(&pak_io_file, saved_position);

        current_data_position += pak_item_size;
        arena_scratch_end(loopmem);
      }
    }
  }

cleanup:
  io_close_file(&pak_io_file);
  arena_scratch_end(fnmem);
  end_profiling();
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_PAK_IMPLEMENTATION
#endif  // MODULE_PAK_HEADER
