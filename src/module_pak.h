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

// ========== TO BE DELETED ===============
typedef struct DELETE_Pak_Tree_Node DELETE_Pak_Tree_Node;
struct DELETE_Pak_Tree_Node {
  char name[PAK_ENTRY_NAME_LEN];
  char item_name[PAK_ENTRY_NAME_LEN];
  Sz size;
  U32 offset;
  Bool is_directory;
  DELETE_Pak_Tree_Node* parent;
  HashMap* children;
};

typedef struct DELETE_Pak_Tree DELETE_Pak_Tree;
struct DELETE_Pak_Tree {
  DELETE_Pak_Tree_Node root;
};
// ========================================

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

  // TODO:
  // do i need this 'flat_entries' field?
  PakRawEntry* flat_entries;
  TreeOf(PackNode) tree;

  // TODO:
  // use Str8
  char error_text[PAK_MAX_ERROR_LENGTH];
  Arena* arena;

  // TODO:
  // delete this
  DELETE_Pak_Tree DELETE_tree;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError DELETE_pak_load_from_file(Pak* pak, IONode* node);
PakError pak_load_from_file(Pak* pak, IONode* node);
Nothing pak_unload(Pak* pak);
PakError pak_extract(Pak* pak, IONode* node, Str8 out_dir);
PakError pak_extract_item(Pak* pak,
                          PakNode* node,
                          IONode* ionode,
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
pak_read_entries_from_memory(Pak* pak, NDBuffer* ndb) {
  START_PROFILING(1);

  Assert(pak != 0);
  Assert(ndb != 0);

  PakError err = PAK_ERR_SUCCESS;

  Arena* arena = pak->arena;
  Sz sz = sizeof(PakRawEntry) * pak->meta.entries_count;
  PakRawEntry* flat_entries =
      (PakRawEntry*)arena_push(arena, sz, AlignOf(PakRawEntry), TRUE);

  pak->flat_entries = flat_entries;

  for (U32 index = 0; index < pak->meta.entries_count; index++) {
    PakRawEntry* entry = flat_entries + index;
    U32 size = 0;
    U32 offset = 0;

    // NOTE:
    // i was under impression the the name buffer is filled with zeros
    // after the last character, but i was proven wrong when i tested
    // https://www.slipseer.com/index.php?resources/dwell.21/
    // unfortunately using memcpy here is not that safe
    // so i had to compromise and use strncpy instead which is slower!
    // ---
    // memcpy(entry->name, ND_ADDR(ndb), PAK_ENTRY_NAME_LEN);

    strncpy(entry->name, (CStr)ND_ADDR(ndb), PAK_ENTRY_NAME_LEN);
    ND_MOVE(ndb, PAK_ENTRY_NAME_LEN);
    ND_I32(ndb, &offset);
    ND_I32(ndb, &size);

    // KindError kerr = kind_guess_entry(S(entry->name), &entry->kind);
    // if (kerr != KIND_ERR_SUCCESS) {
    //   err = PAK_ERR_MALFORMED;
    //   snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
    //            "failed to guess item '%s' kind", entry->name);
    //   goto cleanup;
    // }

    U32 depth = 0;
    err = pak_get_path_depth(entry->name, PAK_ENTRY_NAME_LEN, &depth);
    if (err != PAK_ERR_SUCCESS) {
      snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
               "failed to get path '%s' depth", entry->name);
      goto cleanup;
    }

    DELETE_Pak_Tree_Node* node = &pak->DELETE_tree.root;
    PakNode* node_v2 = tree_root_data(pak->tree);
    for (U32 depth_index = 0; depth_index < depth + 1; depth_index++) {
      char name[PAK_ENTRY_NAME_LEN] = {0};
      char item_name[PAK_ENTRY_NAME_LEN] = {0};

      Str8 name_v2 = {0};
      Str8 path_v2 = {0};

      err = pak_get_path_at_depth(entry->name, PAK_ENTRY_NAME_LEN,
                                  depth_index + 1, &name[0]);
      if (err != PAK_ERR_SUCCESS) {
        snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
                 "failed to get path '%s' at depth '%d'", entry->name,
                 depth_index);
        goto cleanup;
      }

      err =
          pak_get_name_at_depth(name, strlen(name), depth_index, &item_name[0]);
      if (err != PAK_ERR_SUCCESS) {
        snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
                 "failed to get item '%s' name at depth '%d'", name,
                 depth_index);
        goto cleanup;
      }

      HashMapKV* kv = hashmap_find(node->children, S(name));
      if (kv) {
        node = (DELETE_Pak_Tree_Node*)kv->v_rawptr;
        continue;
      }

      DELETE_Pak_Tree_Node* child =
          arena_push(arena, sizeof(DELETE_Pak_Tree_Node),
                     AlignOf(DELETE_Pak_Tree_Node), TRUE);
      if (child != 0)
        ;

      memcpy(child->name, name, PAK_ENTRY_NAME_LEN);
      memcpy(child->item_name, item_name, PAK_ENTRY_NAME_LEN);

      hashmap_push_rawptr(arena, node->children, S(child->name), (RawPtr)child);

      if (depth_index >= depth) {
        child->is_directory = FALSE;
        child->size = size;
        child->offset = offset;
        continue;
      }

      child->is_directory = TRUE;
      child->children = hashmap_init(arena, 64);
      child->parent = node;
      node = child;
    }
  }

cleanup:
  END_PROFILING();
  return err;
}

/* ===================================================== */

static PakError
DELETE_pak_read_entries_from_file(Pak* pak, IONode* node) {
  START_PROFILING(1);

  Assert(pak != 0);
  Assert(node != 0);
  Assert(node->file != 0);

  PakError err = PAK_ERR_SUCCESS;

  Arena* arena = pak->arena;
  Sz sz = sizeof(PakRawEntry) * pak->meta.entries_count;
  PakRawEntry* flat_entries =
      (PakRawEntry*)arena_push(arena, sz, AlignOf(PakRawEntry), TRUE);

  pak->flat_entries = flat_entries;

  for (U32 index = 0; index < pak->meta.entries_count; index++) {
    PakRawEntry* entry = flat_entries + index;
    U32 size = 0;
    U32 offset = 0;

    IO_BUF(node, PAK_ENTRY_NAME_LEN, entry->name);
    IO_I32(node, &offset);
    IO_I32(node, &size);

    // KindError kerr = kind_guess_entry(S(entry->name), &entry->kind);
    // if (kerr != KIND_ERR_SUCCESS) {
    //   err = PAK_ERR_MALFORMED;
    //   snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
    //            "failed to guess item '%s' kind", entry->name);
    //   goto cleanup;
    // }

    U32 depth = 0;
    err = pak_get_path_depth(entry->name, PAK_ENTRY_NAME_LEN, &depth);
    if (err != PAK_ERR_SUCCESS) {
      snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
               "failed to get path '%s' depth", entry->name);
      goto cleanup;
    }

    DELETE_Pak_Tree_Node* node = &pak->DELETE_tree.root;
    for (U32 depth_index = 0; depth_index < depth + 1; depth_index++) {
      char name[PAK_ENTRY_NAME_LEN] = {0};
      char item_name[PAK_ENTRY_NAME_LEN] = {0};

      err = pak_get_path_at_depth(entry->name, PAK_ENTRY_NAME_LEN,
                                  depth_index + 1, &name[0]);
      if (err != PAK_ERR_SUCCESS) {
        snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
                 "failed to get path '%s' at depth '%d'", entry->name,
                 depth_index);
        goto cleanup;
      }

      err =
          pak_get_name_at_depth(name, strlen(name), depth_index, &item_name[0]);
      if (err != PAK_ERR_SUCCESS) {
        snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
                 "failed to get item '%s' name at depth '%d'", name,
                 depth_index);
        goto cleanup;
      }

      HashMapKV* kv = hashmap_find(node->children, S(name));
      if (kv) {
        node = (DELETE_Pak_Tree_Node*)kv->v_rawptr;
        continue;
      }

      DELETE_Pak_Tree_Node* child =
          arena_push(arena, sizeof(DELETE_Pak_Tree_Node),
                     AlignOf(DELETE_Pak_Tree_Node), TRUE);
      if (child != 0)
        ;

      memcpy(child->name, name, PAK_ENTRY_NAME_LEN);
      memcpy(child->item_name, item_name, PAK_ENTRY_NAME_LEN);

      hashmap_push_rawptr(arena, node->children, S(child->name), (RawPtr)child);

      if (depth_index >= depth) {
        child->is_directory = FALSE;
        child->size = size;
        child->offset = offset;
        continue;
      }

      child->is_directory = TRUE;
      child->children = hashmap_init(arena, 64);
      child->parent = node;
      node = child;
    }
  }

cleanup:
  END_PROFILING();
  return err;
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
  PakNode* pak_node_root = arena_push(arena, sizeof(PakNode), AlignOf(PakNode), TRUE);

  pak_node_root->name = str8_clone(arena, S(".")),
  pak_node_root->path = str8_clone(arena, S(".")),
  pak_node_root->data_size = 0,
  pak_node_root->data_offset = 0,
  pak_node_root->is_directory = TRUE,


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
      hashmap_push_rawptr(arena, map, new_pak_node->path, (RawPtr)new_tree_node);

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
DELETE_pak_load_from_file(Pak* pak, IONode* node) {
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

  pak->DELETE_tree.root.parent = 0;
  pak->DELETE_tree.root.children = hashmap_init(pak->arena, 64);
  pak->DELETE_tree.root.is_directory = TRUE;
  MemZero(pak->DELETE_tree.root.name, PAK_ENTRY_NAME_LEN);
  pak->DELETE_tree.root.name[0] = ' ';

  err = DELETE_pak_read_entries_from_file(pak, node);
  if (err != PAK_ERR_SUCCESS) {
    goto cleanup;
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
  DELETE_Pak_Tree_Node* node = &pak->DELETE_tree.root;

  // TODO: either use scratch arena for all allocation or use the main arena
  ArenaScratch scratch = arena_scratch_begin(pak->arena);
  Stack* nodes = stack_create(scratch.arena);

  stack_push(nodes, &pak->DELETE_tree.root);

  while (nodes->length > 0) {
    RawPtr raw_data = stack_pop(nodes);
    if (0 == raw_data) {
      break;
    }
    node = (DELETE_Pak_Tree_Node*)raw_data;

    for (U32 index = 0; index < node->children->count; index++) {
      HashMapKV* kv = hashmap_key_at(node->children, index);
      if (0 == kv) {
        err = PAK_ERR_EXTRACT;
        goto cleanup;
      }

      DELETE_Pak_Tree_Node* new_node = (DELETE_Pak_Tree_Node*)kv->v_rawptr;
      Str8 new_node_str = S(new_node->name);
      Str8 full_path_str =
          str8_join(scratch.arena, out_dir, new_node_str, IO_PATH_SEPARATOR);

      if (TRUE == new_node->is_directory) {
        stack_push(nodes, new_node);

        IOError ioerr = io_make_directory(full_path_str);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
      } else {
        CBuf src_buffer =
            arena_push(scratch.arena, new_node->size, AlignOf(U8), TRUE);

        IO_SET(ionode, new_node->offset);
        IO_BUF(ionode, new_node->size, src_buffer);
        io_dump(full_path_str, buf8(src_buffer, new_node->size));
      }
    }
  }

cleanup:
  arena_scratch_end(scratch);
  END_PROFILING();
  return err;
}

/* ===================================================== */

PakError
pak_extract_item(Pak* pak,
                 PakNode* node,
                 IONode* ionode,
                 Str8 out_dir) {
  START_PROFILING(1);
  PakError err = PAK_ERR_SUCCESS;
  // ArenaScratch scratch = arena_scratch_begin(pak->arena);
  // Stack* nodes = stack_create(scratch.arena);
  // Str8 node_str = node->name;
  // Str8 full_path_str =
  //     str8_join(scratch.arena, out_dir, node_str, IO_PATH_SEPARATOR);

  // if (FALSE == node->is_directory) {
  //   CBuf src_buffer = arena_push(scratch.arena, node->data_size, AlignOf(U8), TRUE);

  //   IO_SET(ionode, node->data_offset);
  //   IO_BUF(ionode, node->data_size, src_buffer);
  //   io_dump(full_path_str, buf8(src_buffer, node->data_size));
  //   goto cleanup;
  // }

  // IOError ioerr = io_make_directory(full_path_str);
  // if (IO_ERR_SUCCESS != ioerr) {
  //   err = PAK_ERR_EXTRACT;
  //   goto cleanup;
  // }

  // U32 start_index = 0;
  // if (node->parent != &pak->DELETE_tree.root) {
  //   start_index = strlen(node->parent->name);
  // }

  // stack_push(nodes, node);

  // while (nodes->length > 0) {
  //   RawPtr raw_data = stack_pop(nodes);
  //   if (0 == raw_data) {
  //     break;
  //   }

  //   node = (DELETE_Pak_Tree_Node*)raw_data;

  //   for (U32 index = 0; index < node->children->count; index++) {
  //     HashMapKV* kv = hashmap_key_at(node->children, index);
  //     if (0 == kv) {
  //       err = PAK_ERR_EXTRACT;
  //       goto cleanup;
  //     }

  //     DELETE_Pak_Tree_Node* new_node = (DELETE_Pak_Tree_Node*)kv->v_rawptr;
  //     Str8 new_node_str = S(new_node->name + start_index);
  //     Str8 full_path_str =
  //         str8_join(scratch.arena, out_dir, new_node_str, IO_PATH_SEPARATOR);

  //     if (TRUE == new_node->is_directory) {
  //       stack_push(nodes, new_node);

  //       IOError ioerr = io_make_directory(full_path_str);
  //       if (IO_ERR_SUCCESS != ioerr) {
  //         err = PAK_ERR_EXTRACT;
  //         goto cleanup;
  //       }
  //     } else {
  //       CBuf src_buffer =
  //           arena_push(scratch.arena, new_node->size, AlignOf(U8), TRUE);

  //       IO_SET(ionode, new_node->offset);
  //       IO_BUF(ionode, new_node->size, src_buffer);
  //       io_dump(full_path_str, buf8(src_buffer, new_node->size));
  //     }
  //   }
  // }

cleanup:
  // arena_scratch_end(scratch);
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
