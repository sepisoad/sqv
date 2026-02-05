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
#include "deps/sepi/tree.h"
#include "deps/sepi/stack.h"
#include "deps/sepi/hashmap.h"
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

typedef struct PakNode PakNode;
struct PakNode {
  Str8 name;
  Str8 path;
  Sz data_size;
  U64 data_offset;
  Bool is_directory;
};

typedef struct Pak Pak;
struct Pak {
  PakMeta meta;
  TreeOf(PakNode) tree;
  Arena* arena;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError pak_load_from_file(Pak* pak, IONode* node);
Nothing pak_unload(Pak* pak);
PakError pak_extract(Pak* pak, IONode* node, Str8 out_dir);
PakError pak_extract_item(Pak* pak,
                          TreeNode* tree_node,
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
pak_read_entries_from_file(Pak* pak, IONode* node) {
  START_PROFILING(1);

  Assert(pak != 0);
  Assert(node != 0);
  Assert(node->file != 0);

  PakError err = PAK_ERR_SUCCESS;
  Arena* arena = pak->arena;
  HashMap* map = hashmap_init(arena, 64);
  PakRawEntry pak_raw_entry = {0};
  U32 entry_size = 0;
  U32 entry_offset = 0;
  PakNode* pak_node_root =
      arena_push(arena, sizeof(PakNode), AlignOf(PakNode), TRUE);

  pak_node_root->name = str8_clone(arena, S(".")),
  pak_node_root->path = str8_clone(arena, S(".")), pak_node_root->data_size = 0,
  pak_node_root->data_offset = 0, pak_node_root->is_directory = TRUE,

  pak->tree =
      tree_create(arena, pak_node_root, sizeof(PakNode), AlignOf(PakNode));

  for (U32 index = 0; index < pak->meta.entries_count; index++) {
    char pak_entry[PAK_ENTRY_NAME_LEN] = {0};
    IO_BUF(node, PAK_ENTRY_NAME_LEN, pak_entry);
    IO_I32(node, &entry_offset);
    IO_I32(node, &entry_size);

    U32 entry_depth = 0;
    err = pak_get_path_depth(pak_entry, PAK_ENTRY_NAME_LEN, &entry_depth);
    if (err != PAK_ERR_SUCCESS) {
      goto cleanup;
    }

    TreeNode* current_tree_node = tree_root(pak->tree);

    for (U32 depth_index = 0; depth_index < entry_depth + 1; depth_index++) {
      char path[PAK_ENTRY_NAME_LEN] = {0};
      char name[PAK_ENTRY_NAME_LEN] = {0};

      err = pak_get_path_at_depth(pak_entry, PAK_ENTRY_NAME_LEN,
                                  depth_index + 1, &path[0]);
      if (err != PAK_ERR_SUCCESS) {
        goto cleanup;
      }

      err = pak_get_name_at_depth(path, strlen(path), depth_index, &name[0]);
      if (err != PAK_ERR_SUCCESS) {
        goto cleanup;
      }

      HashMapKV* kv = hashmap_find(map, S(path));
      if (kv) {
        current_tree_node = (TreeNode*)kv->v_rawptr;
        continue;
      }

      PakNode* new_pak_node =
          arena_push(pak->arena, sizeof(PakNode), AlignOf(PakNode), TRUE);

      new_pak_node->name = str8_clone(pak->arena, S(name));
      new_pak_node->path = str8_clone(pak->arena, S(path));

      TreeNode* new_tree_node =
          tree_push(pak->tree, current_tree_node, new_pak_node);
      hashmap_push_rawptr(arena, map, new_pak_node->path,
                          (RawPtr)new_tree_node);

      if (depth_index >= entry_depth) {
        new_pak_node->data_size = entry_size;
        new_pak_node->data_offset = entry_offset;
        continue;
      }

      current_tree_node = new_tree_node;
      new_pak_node->is_directory = TRUE;
    }
  }

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

PakError
pak_load_from_file(Pak* pak, IONode* node) {
  START_PROFILING(1);

  Assert(pak != 0);
  Assert(node != 0);

  PakError err = PAK_ERR_SUCCESS;

  U8 magic_code[PAK_MAGIC_CODE_LEN] = {0};
  I32 offset = 0;
  I32 size = 0;

  pak->arena = arena_create();

  IO_BUF(node, PAK_MAGIC_CODE_LEN, magic_code);
  IO_I32(node, &offset);
  IO_I32(node, &size);
  IO_SET(node, offset);

  if ((offset <= 0) || (size <= 0) || (magic_code[0] != 'P') ||
      (magic_code[1] != 'A') || (magic_code[2] != 'C') ||
      (magic_code[3] != 'K')) {
    err = PAK_ERR_MALFORMED;
    goto cleanup;
  }

  pak->meta.entries_count = size / sizeof(PakRawEntry);
  err = pak_read_entries_from_file(pak, node);
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
  TreeNode* tree_node = pak->tree->root;
  Stack* stack = stack_create(scratch.arena);

  stack_push(stack, tree_node);

  while (stack->length > 0) {
    tree_node = stack_pop(stack);
    if (0 == tree_node) {
      break;
    }

    U32 children_count = tree_node_length(tree_node);
    for (U32 index = 0; index < children_count; index++) {
      TreeNode* child_node = tree_node_get(tree_node, index);
      if (0 == child_node) {
        err = PAK_ERR_EXTRACT;
        goto cleanup;
      }

      PakNode* pak_node = (PakNode*)child_node->data;
      Str8 full_path_str =
          str8_join(scratch.arena, out_dir, pak_node->path, IO_PATH_SEPARATOR);

      if (TRUE == pak_node->is_directory) {
        stack_push(stack, child_node);

        IOError ioerr = io_make_directory(full_path_str);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
      } else {
        CBuf src_buffer =
            arena_push(scratch.arena, pak_node->data_size, AlignOf(U8), TRUE);

        IO_SET(ionode, pak_node->data_offset);
        IO_BUF(ionode, pak_node->data_size, src_buffer);
        io_dump(full_path_str, buf8(src_buffer, pak_node->data_size));
      }
    }
  }

cleanup:
  stack_destroy(stack);
  arena_scratch_end(scratch);
  END_PROFILING();
  return err;
}

/* ===================================================== */

PakError
pak_extract_item(Pak* pak, TreeNode* tree_node, IONode* io_node, Str8 out_dir) {
  START_PROFILING(1);

  PakError err = PAK_ERR_SUCCESS;
  ArenaScratch scratch = arena_scratch_begin(pak->arena);
  Stack* stack = stack_create(scratch.arena);

  stack_push(stack, tree_node);

  if (0 == tree_node_length(tree_node)) {
    // it's a file!
    PakNode* pak_node = (PakNode*)tree_node->data;
    if (pak_node->is_directory) {
      // wierd, it's an empty directory, we won't do anything here
      goto cleanup;
    }
    Str8 full_path_str =
          str8_join(scratch.arena, out_dir, pak_node->name, IO_PATH_SEPARATOR);
    CBuf src_buffer =
        arena_push(scratch.arena, pak_node->data_size, AlignOf(U8), TRUE);

    IO_SET(io_node, pak_node->data_offset);
    IO_BUF(io_node, pak_node->data_size, src_buffer);
    io_dump(full_path_str, buf8(src_buffer, pak_node->data_size));
    goto cleanup;
  }

  while (stack->length > 0) {
    tree_node = stack_pop(stack);
    if (0 == tree_node) {
      break;
    }

    U32 children_count = tree_node_length(tree_node);
    for (U32 index = 0; index < children_count; index++) {
      TreeNode* child_node = tree_node_get(tree_node, index);
      if (0 == child_node) {
        err = PAK_ERR_EXTRACT;
        goto cleanup;
      }

      PakNode* pak_node = (PakNode*)child_node->data;
      Str8 full_path_str =
          str8_join(scratch.arena, out_dir, pak_node->path, IO_PATH_SEPARATOR);

      if (TRUE == pak_node->is_directory) {
        stack_push(stack, child_node);

        IOError ioerr = io_make_nested_directory(full_path_str);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
      } else {
        CBuf src_buffer =
            arena_push(scratch.arena, pak_node->data_size, AlignOf(U8), TRUE);

        IO_SET(io_node, pak_node->data_offset);
        IO_BUF(io_node, pak_node->data_size, src_buffer);
        io_dump(full_path_str, buf8(src_buffer, pak_node->data_size));
      }
    }
  }

cleanup:
  stack_destroy(stack);
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
