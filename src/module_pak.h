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

#define SEPI_MAP_PAKITEM_IMPLEMENTATION
#define SEPI_MAP_PAKCOUNTER_IMPLEMENTATION
#include "module_kind.h"
#include "generated/map_pakitem.h"
#include "generated/map_pakcounter.h"

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

typedef enum {
  PAK_ERR_SUCCESS,
  PAK_ERR_MALFORMED,
  PAK_ERR_INVALID_ENTRY_PATH,
  PAK_ERR_OUT_DIR_NOT_FOUND,
  PAK_ERR_EXTRACT,
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

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError pak_load_from_file(Pak* pak, IOItem* node);
Nothing pak_unload(Pak* pak);
PakError pak_extract(Pak* pak, IOItem* node, Str8 out_dir);
PakError pak_extract_item(Pak* pak,
                          PakItem* pak_item_for_path,
                          IOItem* io_node,
                          Str8 out_dir);
PakError pak_generate(Pak* pak, IOItem* node);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_PAK_IMPLEMENTATION

mount_slave_profiling_context();

// TODO:
// use Str8
static PakError
pak_get_path_depth(CStr path, U32 length, U32* depth) {
  start_profiling(1);

  assert(path != 0);
  assert(length > 0);
  assert(depth != 0);

  for (U32 index = 0; index < length; index++) {
    if (path[index] == 0) {
      break;
    }

    if (path[index] == '/') {
      *depth = *depth + 1;
    }
  }

  end_profiling();
  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

// TODO:
// use Str8
static PakError
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
  return PAK_ERR_SUCCESS;
}

// TODO:
// use Str8
static PakError
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
  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

static PakError
pak_read_entries_from_io_item(Pak* pak, IOItem* io_item) {
  start_profiling(1);

  assert(pak != 0);
  assert(io_item != 0);
  assert(io_item->file != 0);

  PakError err = PAK_ERR_SUCCESS;
  Arena* arena = pak->arena;

  MapPakCounter map_counts = map_pakcounter_make(arena, 64);
  U64 io_item_file_offset = IO_POS(io_item);
  PakCounter* root_counter =
      arena_push(pak->arena, sizeof(PakCounter), alignof(PakCounter), TRUE);

  root_counter->path = S(".");
  map_pakcounter_push(&map_counts, root_counter->path, root_counter);

  for (U32 index = 0; index < pak->items_count; index++) {
    char entry_str[PAK_ENTRY_NAME_LEN] = {0};

    IO_BUF(io_item, PAK_ENTRY_NAME_LEN, entry_str);
    IO_MOVE(io_item, sizeof(I32));
    IO_MOVE(io_item, sizeof(I32));

    U32 path_depth = 0;
    err = pak_get_path_depth(entry_str, PAK_ENTRY_NAME_LEN, &path_depth);
    if (err != PAK_ERR_SUCCESS) {
      goto cleanup;
    }

    PakCounter* last_counter = root_counter;
    for (U32 depth_index = 0; depth_index < path_depth + 1; depth_index++) {
      char path[PAK_ENTRY_NAME_LEN] = {0};
      char name[PAK_ENTRY_NAME_LEN] = {0};

      err = pak_get_path_at_depth(entry_str, PAK_ENTRY_NAME_LEN,
                                  depth_index + 1, &path[0]);
      if (err != PAK_ERR_SUCCESS) {
        goto cleanup;
      }

      err = pak_get_name_at_depth(path, strlen(path), depth_index, &name[0]);
      if (err != PAK_ERR_SUCCESS) {
        goto cleanup;
      }

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

  IO_SET(io_item, io_item_file_offset);

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

  PakItem* last_pak_item = root_pak_item;
  for (U32 index = 0; index < pak->items_count; index++) {
    char entry_str[PAK_ENTRY_NAME_LEN] = {0};
    U32 entry_size = 0;
    U32 entry_offset = 0;

    IO_BUF(io_item, PAK_ENTRY_NAME_LEN, entry_str);
    IO_I32(io_item, &entry_offset);
    IO_I32(io_item, &entry_size);

    U32 path_depth = 0;
    err = pak_get_path_depth(entry_str, PAK_ENTRY_NAME_LEN, &path_depth);
    if (err != PAK_ERR_SUCCESS) {
      goto cleanup;
    }

    last_pak_item = root_pak_item;
    for (U32 depth_index = 0; depth_index < path_depth + 1; depth_index++) {
      char path[PAK_ENTRY_NAME_LEN] = {0};
      char name[PAK_ENTRY_NAME_LEN] = {0};

      err = pak_get_path_at_depth(entry_str, PAK_ENTRY_NAME_LEN,
                                  depth_index + 1, &path[0]);
      if (err != PAK_ERR_SUCCESS) {
        goto cleanup;
      }

      err = pak_get_name_at_depth(path, strlen(path), depth_index, &name[0]);
      if (err != PAK_ERR_SUCCESS) {
        goto cleanup;
      }

      PakItem* existing_pak_item = map_pakitem_get(&map_items, S(path));
      if (existing_pak_item) {
        last_pak_item = existing_pak_item;
        continue;
      }

      PakCounter* counter = map_pakcounter_get(&map_counts, S(path));
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

  // Str8* keys = 0;
  // U64 keys_count = 0;
  // map_pakcounter_keys(&map_counts, &keys, &keys_count);
  // for (U64 index = 0; index < keys_count; index++) {
  //   PakCounter* counter = map_pakcounter_get(&map_counts, keys[index]);
  //   PakItem* item = map_pakitem_get(&map_items, keys[index]);
  //   if (!item) {
  //     dbg("WTF: %s", CS(keys[index]));
  //     continue;
  //   }
  //   assert(str8_equal(counter->path, item->path, 0));
  //   // dbg("%s => %s", CS(item->name), CS(item->path));
  //   dbg("%s => %s", CS(counter->path), CS(item->name));
  //   // dbg("%s", CS(item->path));
  // }

  // PakItem* item_root = map_pakitem_get(&map_items, S("."));
  // PakItem* item_progs = map_pakitem_get(&map_items, S("progs/"));

  // dbg("yaaaaay");

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */

PakError
pak_load_from_file(Pak* pak, IOItem* io_item) {
  start_profiling(1);

  assert(pak != 0);
  assert(io_item != 0);

  PakError err = PAK_ERR_SUCCESS;

  U8 magic_code[PAK_MAGIC_CODE_LEN] = {0};
  I32 offset = 0;
  I32 size = 0;

  pak->arena = arena_create();

  IO_BUF(io_item, PAK_MAGIC_CODE_LEN, magic_code);
  IO_I32(io_item, &offset);
  IO_I32(io_item, &size);
  IO_SET(io_item, offset);

  if ((offset <= 0) || (size <= 0) || (magic_code[0] != 'P') ||
      (magic_code[1] != 'A') || (magic_code[2] != 'C') ||
      (magic_code[3] != 'K')) {
    err = PAK_ERR_MALFORMED;
    goto cleanup;
  }

  pak->items_count = size / sizeof(PakRawEntry);
  err = pak_read_entries_from_io_item(pak, io_item);
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
pak_extract(Pak* pak, IOItem* ionode, Str8 out_dir) {
  start_profiling(1);

  PakError err = PAK_ERR_SUCCESS;
  // ArenaScratch scratch = arena_scratch_begin(pak->arena);
  // PakItem* pak_item_for_path = &pak->tree.root;
  // StackTreeNodePakItem* stack = stack_treenodepakitem_create(scratch.arena);

  // stack_treenodepakitem_push(stack, pak_item_for_path);

  // while (stack->length > 0) {
  //   pak_item_for_path = stack_treenodepakitem_pop(stack);
  //   if (0 == pak_item_for_path) {
  //     break;
  //   }

  //   U32 children_count = tree_pakitem_node_length(pak_item_for_path);
  //   for (U32 index = 0; index < children_count; index++) {
  //     PakItem* current_child =
  //         tree_pakitem_node_get_child(pak_item_for_path, index);
  //     PakItem* pak_item = current_child->data;
  //     if (0 == pak_item) {
  //       err = PAK_ERR_EXTRACT;
  //       goto cleanup;
  //     }

  //     Str8 full_path_str =
  //         str8_join(scratch.arena, out_dir, pak_item->path,
  //         IO_PATH_SEPARATOR);

  //     if (TRUE == pak_item->is_directory) {
  //       stack_treenodepakitem_push(stack, current_child);

  //       IOError ioerr = io_make_directory(full_path_str);
  //       if (IO_ERR_SUCCESS != ioerr) {
  //         err = PAK_ERR_EXTRACT;
  //         goto cleanup;
  //       }
  //     } else {
  //       CBuf src_buffer =
  //           arena_push(scratch.arena, pak_item->data_size, alignof(U8),
  //           TRUE);

  //       IO_SET(ionode, pak_item->data_offset);
  //       IO_BUF(ionode, pak_item->data_size, src_buffer);
  //       io_dump_buffer_to_path(full_path_str,
  //                              buf8(src_buffer, pak_item->data_size));
  //     }
  //   }
  // }

cleanup:
  // stack_treenodepakitem_clean(stack);
  // arena_scratch_end(scratch);
  end_profiling();
  return err;
}

/* ===================================================== */

PakError
pak_extract_item(Pak* pak,
                 PakItem* pak_item_for_path,
                 IOItem* io_node,
                 Str8 out_dir) {
  start_profiling(1);

  PakError err = PAK_ERR_SUCCESS;
  // ArenaScratch scratch = arena_scratch_begin(pak->arena);
  // StackTreeNodePakItem* stack = stack_treenodepakitem_create(scratch.arena);

  // stack_treenodepakitem_push(stack, pak_item_for_path);

  // if (0 == tree_pakitem_node_length(pak_item_for_path)) {
  //   // it's a file!
  //   PakItem* pak_item = pak_item_for_path->data;
  //   if (pak_item->is_directory) {
  //     // fuck, it's an empty directory, we won't do anything here
  //     goto cleanup;
  //   }
  //   Str8 full_path_str =
  //       str8_join(scratch.arena, out_dir, pak_item->name, IO_PATH_SEPARATOR);
  //   CBuf src_buffer =
  //       arena_push(scratch.arena, pak_item->data_size, alignof(U8), TRUE);

  //   IO_SET(io_node, pak_item->data_offset);
  //   IO_BUF(io_node, pak_item->data_size, src_buffer);
  //   io_dump_buffer_to_path(full_path_str,
  //                          buf8(src_buffer, pak_item->data_size));
  //   goto cleanup;
  // }

  // while (stack->length > 0) {
  //   pak_item_for_path = stack_treenodepakitem_pop(stack);
  //   if (0 == pak_item_for_path) {
  //     break;
  //   }

  //   U32 children_count = tree_pakitem_node_length(pak_item_for_path);
  //   for (U32 index = 0; index < children_count; index++) {
  //     PakItem* current_child =
  //         tree_pakitem_node_get_child(pak_item_for_path, index);
  //     PakItem* pak_item = current_child->data;
  //     if (0 == pak_item) {
  //       err = PAK_ERR_EXTRACT;
  //       goto cleanup;
  //     }

  //     Str8 full_path_str =
  //         str8_join(scratch.arena, out_dir, pak_item->path,
  //         IO_PATH_SEPARATOR);

  //     if (TRUE == pak_item->is_directory) {
  //       stack_treenodepakitem_push(stack, current_child);

  //       IOError ioerr = io_make_nested_directory(full_path_str);
  //       if (IO_ERR_SUCCESS != ioerr) {
  //         err = PAK_ERR_EXTRACT;
  //         goto cleanup;
  //       }
  //     } else {
  //       CBuf src_buffer =
  //           arena_push(scratch.arena, pak_item->data_size, alignof(U8),
  //           TRUE);

  //       IO_SET(io_node, pak_item->data_offset);
  //       IO_BUF(io_node, pak_item->data_size, src_buffer);
  //       io_dump_buffer_to_path(full_path_str,
  //                              buf8(src_buffer, pak_item->data_size));
  //     }
  //   }
  // }

cleanup:
  // stack_treenodepakitem_clean(stack);
  // arena_scratch_end(scratch);
  end_profiling();
  return err;
}

/* ===================================================== */

PakError
pak_generate(Pak* pak, IOItem* node) {
  start_profiling(1);

  assert(pak != 0);
  assert(node != 0);

  PakError err = PAK_ERR_SUCCESS;

cleanup:
  end_profiling();
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_PAK_IMPLEMENTATION
#endif  // MODULE_PAK_HEADER
