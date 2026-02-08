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

#include "deps/sepi/arena.h"
#include "deps/sepi/endian.h"
#include "deps/sepi/tree_ex.h"
#include "deps/sepi/stack_ex.h"
#include "deps/sepi/hashmap.h"
#include "deps/sepi/map_ex.h"
#include "deps/sepi/string.h"
#include "deps/sepi/io.h"

#include "module_kind.h"

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

typedef struct PakRawEntry PakRawEntry;
struct PakRawEntry {
  char name[PAK_ENTRY_NAME_LEN];
  I32 offset;
  I32 size;
};

typedef struct PakMeta PakMeta;
struct PakMeta {
  U32 entries_count;
};

typedef struct PakItem PakItem;
struct PakItem {
  Str8 name;
  Str8 path;
  Sz data_size;
  U64 data_offset;
  Bool is_directory;
};

DefineTree(PakItem*, PakItem, pak_item);
DefineMap(TreePakItemNode*, TreePakItemNode, tree_pak_item_node);
DefineStack(TreePakItemNode*, TreePakItemNode, tree_pak_item_node);

typedef struct TreeManager TreeManager;
struct TreeManager {};

typedef struct Pak Pak;
struct Pak {
  PakMeta meta;
  TreePakItem tree;
  Arena* arena;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError pak_load_from_file(Pak* pak, IONode* node);
Nothing pak_unload(Pak* pak);
PakError pak_extract(Pak* pak, IONode* node, Str8 out_dir);
PakError pak_extract_item(Pak* pak,
                          TreePakItemNode* pak_item_node_for_path,
                          IONode* io_node,
                          Str8 out_dir);
PakError pak_generate(Pak* pak, IONode* node);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_PAK_IMPLEMENTATION

SLAVE_PROFILING_CONTEXT;

// TODO:
// use Str8
static PakError
pak_get_path_depth(CStr path, U32 length, U32* depth) {
  START_PROFILING(1);

  Assert(path != 0);
  Assert(length > 0);
  Assert(depth != 0);

  for (U32 index = 0; index < length; index++) {
    if (path[index] == 0) {
      break;
    }

    if (path[index] == '/') {
      *depth = *depth + 1;
    }
  }

  END_PROFILING();
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
  START_PROFILING(1);

  Assert(path != 0);
  Assert(out != 0);

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

  END_PROFILING();
  return PAK_ERR_SUCCESS;
}

// TODO:
// use Str8

static PakError
pak_get_name_at_depth(CStr path,
                      U32 length,
                      U32 depth,
                      char out[PAK_ENTRY_NAME_LEN]) {
  START_PROFILING(1);

  Assert(path != 0);
  Assert(out != 0);

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

  END_PROFILING();
  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

static PakError
pak_read_entries_from_file(Pak* pak, IONode* file_node) {
  START_PROFILING(1);

  Assert(pak != 0);
  Assert(file_node != 0);
  Assert(file_node->file != 0);

  PakError err = PAK_ERR_SUCCESS;
  Arena* arena = pak->arena;

  MapTreePakItemNode map = map_tree_pak_item_node_make(arena, 64);

  PakItem* root_pak_item =
      arena_push(arena, sizeof(PakItem), AlignOf(PakItem), TRUE);

  pak->tree = tree_pak_item_make(arena);

  root_pak_item->name = str8_clone(arena, S("."));
  root_pak_item->path = str8_clone(arena, S("."));
  root_pak_item->data_size = 0;
  root_pak_item->data_offset = 0;
  root_pak_item->is_directory = TRUE;

  pak->tree.root.data = root_pak_item;

  // tree_pak_item_push(&pak->tree, &pak->tree.root, root_pak_item);

  for (U32 index = 0; index < pak->meta.entries_count; index++) {
    char entry_str[PAK_ENTRY_NAME_LEN] = {0};
    U32 entry_size = 0;
    U32 entry_offset = 0;

    IO_BUF(file_node, PAK_ENTRY_NAME_LEN, entry_str);
    IO_I32(file_node, &entry_offset);
    IO_I32(file_node, &entry_size);

    U32 path_depth = 0;
    err = pak_get_path_depth(entry_str, PAK_ENTRY_NAME_LEN, &path_depth);
    if (err != PAK_ERR_SUCCESS) {
      goto cleanup;
    }

    TreePakItemNode* current_pak_item_node = &pak->tree.root;

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

      TreePakItemNode* pak_item_node_for_path =
          map_tree_pak_item_node_find(&map, S(path));
      if (pak_item_node_for_path) {
        current_pak_item_node = pak_item_node_for_path;
        continue;
      }

      PakItem* new_pak_item =
          arena_push(pak->arena, sizeof(PakItem), AlignOf(PakItem), TRUE);

      new_pak_item->name = str8_clone(pak->arena, S(name));
      new_pak_item->path = str8_clone(pak->arena, S(path));

      TreePakItemNode* new_child_pak_item_node =
          tree_pak_item_push(&pak->tree, current_pak_item_node, new_pak_item);
      map_tree_pak_item_node_push(&map, new_pak_item->path,
                                  new_child_pak_item_node);

      if (depth_index >= path_depth) {
        new_pak_item->data_size = entry_size;
        new_pak_item->data_offset = entry_offset;
        continue;
      }

      current_pak_item_node = new_child_pak_item_node;
      new_pak_item->is_directory = TRUE;
    }
  }

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

PakError
pak_load_from_file(Pak* pak, IONode* file_node) {
  START_PROFILING(1);

  Assert(pak != 0);
  Assert(file_node != 0);

  PakError err = PAK_ERR_SUCCESS;

  U8 magic_code[PAK_MAGIC_CODE_LEN] = {0};
  I32 offset = 0;
  I32 size = 0;

  pak->arena = arena_create();

  IO_BUF(file_node, PAK_MAGIC_CODE_LEN, magic_code);
  IO_I32(file_node, &offset);
  IO_I32(file_node, &size);
  IO_SET(file_node, offset);

  if ((offset <= 0) || (size <= 0) || (magic_code[0] != 'P') ||
      (magic_code[1] != 'A') || (magic_code[2] != 'C') ||
      (magic_code[3] != 'K')) {
    err = PAK_ERR_MALFORMED;
    goto cleanup;
  }

  pak->meta.entries_count = size / sizeof(PakRawEntry);
  err = pak_read_entries_from_file(pak, file_node);
  if (err != PAK_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

Nothing
pak_unload(Pak* pak) {
  START_PROFILING(1);

  if (pak->arena) {
    arena_destroy(pak->arena);
  }

  END_PROFILING();
}

/* ===================================================== */

PakError
pak_extract(Pak* pak, IONode* ionode, Str8 out_dir) {
  START_PROFILING(1);

  PakError err = PAK_ERR_SUCCESS;
  ArenaScratch scratch = arena_scratch_begin(pak->arena);
  TreePakItemNode* pak_item_node_for_path = &pak->tree.root;
  StackTreePakItemNode stack = stack_tree_pak_item_node_make(scratch.arena);

  stack_tree_pak_item_node_push(&stack, pak_item_node_for_path);

  while (stack.length > 0) {
    pak_item_node_for_path = stack_tree_pak_item_node_pop(&stack);
    if (0 == pak_item_node_for_path) {
      break;
    }

    U32 children_count = tree_pak_item_node_length(pak_item_node_for_path);
    for (U32 index = 0; index < children_count; index++) {
      TreePakItemNode* current_child =
          tree_pak_item_node_get_child(pak_item_node_for_path, index);
      PakItem* pak_item = current_child->data;
      if (0 == pak_item) {
        err = PAK_ERR_EXTRACT;
        goto cleanup;
      }

      Str8 full_path_str =
          str8_join(scratch.arena, out_dir, pak_item->path, IO_PATH_SEPARATOR);

      if (TRUE == pak_item->is_directory) {
        stack_tree_pak_item_node_push(&stack, current_child);

        IOError ioerr = io_make_directory(full_path_str);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
      } else {
        CBuf src_buffer =
            arena_push(scratch.arena, pak_item->data_size, AlignOf(U8), TRUE);

        IO_SET(ionode, pak_item->data_offset);
        IO_BUF(ionode, pak_item->data_size, src_buffer);
        io_dump(full_path_str, buf8(src_buffer, pak_item->data_size));
      }
    }
  }

cleanup:
  stack_tree_pak_item_node_clean(&stack);
  arena_scratch_end(scratch);
  END_PROFILING();
  return err;
}

/* ===================================================== */

PakError
pak_extract_item(Pak* pak,
                 TreePakItemNode* pak_item_node_for_path,
                 IONode* io_node,
                 Str8 out_dir) {
  START_PROFILING(1);

  PakError err = PAK_ERR_SUCCESS;
  ArenaScratch scratch = arena_scratch_begin(pak->arena);
  StackTreePakItemNode stack = stack_tree_pak_item_node_make(scratch.arena);

  stack_tree_pak_item_node_push(&stack, pak_item_node_for_path);

  if (0 == tree_pak_item_node_length(pak_item_node_for_path)) {
    // it's a file!
    PakItem* pak_item = pak_item_node_for_path->data;
    if (pak_item->is_directory) {
      // fuck, it's an empty directory, we won't do anything here
      goto cleanup;
    }
    Str8 full_path_str =
        str8_join(scratch.arena, out_dir, pak_item->name, IO_PATH_SEPARATOR);
    CBuf src_buffer =
        arena_push(scratch.arena, pak_item->data_size, AlignOf(U8), TRUE);

    IO_SET(io_node, pak_item->data_offset);
    IO_BUF(io_node, pak_item->data_size, src_buffer);
    io_dump(full_path_str, buf8(src_buffer, pak_item->data_size));
    goto cleanup;
  }

  while (stack.length > 0) {
    pak_item_node_for_path = stack_tree_pak_item_node_pop(&stack);
    if (0 == pak_item_node_for_path) {
      break;
    }

    U32 children_count = tree_pak_item_node_length(pak_item_node_for_path);
    for (U32 index = 0; index < children_count; index++) {
      TreePakItemNode* current_child =
          tree_pak_item_node_get_child(pak_item_node_for_path, index);
      PakItem* pak_item = current_child->data;
      if (0 == pak_item) {
        err = PAK_ERR_EXTRACT;
        goto cleanup;
      }

      Str8 full_path_str =
          str8_join(scratch.arena, out_dir, pak_item->path, IO_PATH_SEPARATOR);

      if (TRUE == pak_item->is_directory) {
        stack_tree_pak_item_node_push(&stack, current_child);

        IOError ioerr = io_make_nested_directory(full_path_str);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
      } else {
        CBuf src_buffer =
            arena_push(scratch.arena, pak_item->data_size, AlignOf(U8), TRUE);

        IO_SET(io_node, pak_item->data_offset);
        IO_BUF(io_node, pak_item->data_size, src_buffer);
        io_dump(full_path_str, buf8(src_buffer, pak_item->data_size));
      }
    }
  }

cleanup:
  stack_tree_pak_item_node_clean(&stack);
  arena_scratch_end(scratch);
  END_PROFILING();
  return err;
}

/* ===================================================== */

PakError
pak_generate(Pak* pak, IONode* node) {
  START_PROFILING(1);

  Assert(pak != 0);
  Assert(node != 0);

  PakError err = PAK_ERR_SUCCESS;

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_PAK_IMPLEMENTATION
#endif  // MODULE_PAK_HEADER
