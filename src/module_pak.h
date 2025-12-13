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

#include "deps/tracy/tracy.h"
#include "deps/sepi/arena.h"
#include "deps/sepi/endian.h"
#include "deps/sepi/list.h"
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

typedef struct {
  char name[PAK_ENTRY_NAME_LEN];
  I32 offset;
  I32 size;
} PakRawEntry;

typedef struct {
  U32 entries_count;
} PakDetails;

typedef struct {
  char name[PAK_ENTRY_NAME_LEN];
  Kind kind;
} PakEntry;

typedef struct PakTreeNode PakTreeNode;

struct PakTreeNode {
  char name[PAK_ENTRY_NAME_LEN];
  char item_name[PAK_ENTRY_NAME_LEN];
  Sz size;
  U32 offset;
  Bool is_dir;
  HashMap* children;
  PakTreeNode* parent;
};

typedef struct {
  PakTreeNode root;
} PakTree;

typedef struct {
  PakDetails details;
  PakEntry* entries;
  PakTree tree;
  char error_text[PAK_MAX_ERROR_LENGTH];
  Arena* arena;
} Pak;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError pak_load_from_memory(Pak* pak, NDBuffer* ndb);
PakError pak_load_from_file(Pak* pak, FILE* file);
Nothing pak_unload(Pak* pak);
PakError pak_extract(Pak* pak, FILE* file, Str8 out_dir);
PakError pak_extract_item(Pak* pak,
                          PakTreeNode* node,
                          FILE* file,
                          Str8 out_dir);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_PAK_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

internal PakError
pak_get_path_depth(CStr path, U32 length, U32* depth) {
  TracyCZoneN(trcyctx, "pak_get_path_depth", 1);

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

  TracyCZoneEnd(trcyctx);
  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

internal PakError
pak_get_path_at_depth(CStr path,
                      U32 length,
                      U32 depth,
                      char out[PAK_ENTRY_NAME_LEN]) {
  TracyCZoneN(trcyctx, "pak_get_path_at_depth", 1);

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

  TracyCZoneEnd(trcyctx);
  return PAK_ERR_SUCCESS;
}

internal PakError
pak_get_item_name_at_depth(CStr path,
                           U32 length,
                           U32 depth,
                           char out[PAK_ENTRY_NAME_LEN]) {
  TracyCZoneN(trcyctx, "pak_get_item_name_at_depth", 1);

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

  TracyCZoneEnd(trcyctx);
  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

internal PakError
pak_read_entries_from_memory(Pak* pak, NDBuffer* ndb) {
  TracyCZoneN(trcyctx, "pak_read_entries_from_memory", 1);

  Assert(pak != 0);
  Assert(ndb != 0);

  PakError err = PAK_ERR_SUCCESS;

  Arena* arena = pak->arena;
  Sz sz = sizeof(PakEntry) * pak->details.entries_count;
  PakEntry* entries = (PakEntry*)arena_push(arena, sz, AlignOf(PakEntry), TRUE);

  pak->entries = entries;

  for (U32 index = 0; index < pak->details.entries_count; index++) {
    PakEntry* entry = entries + index;
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

    strncpy(entry->name, ND_ADDR(ndb), PAK_ENTRY_NAME_LEN);
    ND_MOVE(ndb, PAK_ENTRY_NAME_LEN);
    ND_I32(ndb, &offset);
    ND_I32(ndb, &size);

    KindError kerr =
        kind_guess_entry(entry->name, PAK_ENTRY_NAME_LEN, &entry->kind);
    if (kerr != KIND_ERR_SUCCESS) {
      err = PAK_ERR_MALFORMED;
      snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
               "failed to guess item '%s' kind", entry->name);
      goto cleanup;
    }

    U32 depth = 0;
    err = pak_get_path_depth(entry->name, PAK_ENTRY_NAME_LEN, &depth);
    if (err != PAK_ERR_SUCCESS) {
      snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
               "failed to get path '%s' depth", entry->name);
      goto cleanup;
    }

    PakTreeNode* node = &pak->tree.root;
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

      err = pak_get_item_name_at_depth(name, strlen(name), depth_index,
                                       &item_name[0]);
      if (err != PAK_ERR_SUCCESS) {
        snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
                 "failed to get item '%s' name at depth '%d'", name,
                 depth_index);
        goto cleanup;
      }

      HashMapKV* kv = hashmap_find(node->children, str8(name));
      if (kv) {
        node = (PakTreeNode*)kv->v_rawptr;
        continue;
      }

      PakTreeNode* child =
          arena_push(arena, sizeof(PakTreeNode), AlignOf(PakTreeNode), TRUE);
      if (child != 0)
        ;

      memcpy(child->name, name, PAK_ENTRY_NAME_LEN);
      memcpy(child->item_name, item_name, PAK_ENTRY_NAME_LEN);

      hashmap_push_rawptr(arena, node->children, str8(child->name),
                          (RawPtr)child);

      if (depth_index >= depth) {
        child->is_dir = FALSE;
        child->size = size;
        child->offset = offset;
        continue;
      }

      child->is_dir = TRUE;
      child->children = hashmap_init(arena, 64);
      child->parent = node;
      node = child;
    }
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

PakError
pak_load_from_memory(Pak* pak, NDBuffer* ndb) {
  TracyCZoneN(trcyctx, "pak_load_from_memory", 1);

  Assert(pak != 0);
  Assert(ndb != 0);
  Assert(pak->arena == 0);

  PakError err = PAK_ERR_SUCCESS;

  U8 magic_code[PAK_MAGIC_CODE_LEN] = {0};
  I32 offset = 0;
  I32 size = 0;

  pak->arena = arena_create();

  memcpy(magic_code, ndb->base, PAK_MAGIC_CODE_LEN);
  ND_MOVE(ndb, PAK_MAGIC_CODE_LEN);
  ND_I32(ndb, &offset);
  ND_I32(ndb, &size);
  ND_ADDR_SET(ndb, offset);

  // TODO: replace these with error codes!
  if (offset > 0)
    ;
  if (size > 0)
    ;
  if (magic_code[0] == 'P')
    ;
  if (magic_code[1] == 'A')
    ;
  if (magic_code[2] == 'C')
    ;
  if (magic_code[3] == 'K')
    ;

  pak->details.entries_count = size / sizeof(PakRawEntry);

  // TODO: find a proper default 'cap'
  pak->tree.root.parent = 0;
  pak->tree.root.children = hashmap_init(pak->arena, 64);
  pak->tree.root.is_dir = TRUE;
  MemZero(pak->tree.root.name, PAK_ENTRY_NAME_LEN);
  pak->tree.root.name[0] = ' ';

  err = pak_read_entries_from_memory(pak, ndb);
  if (err != PAK_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

internal PakError
pak_read_entries_from_file(Pak* pak, FILE* file) {
  TracyCZoneN(trcyctx, "pak_read_entries_from_file", 1);

  Assert(pak != 0);
  Assert(file != 0);

  PakError err = PAK_ERR_SUCCESS;

  Arena* arena = pak->arena;
  Sz sz = sizeof(PakEntry) * pak->details.entries_count;
  PakEntry* entries = (PakEntry*)arena_push(arena, sz, AlignOf(PakEntry), TRUE);

  pak->entries = entries;

  for (U32 index = 0; index < pak->details.entries_count; index++) {
    PakEntry* entry = entries + index;
    U32 size = 0;
    U32 offset = 0;

    IO_BUF(file, PAK_ENTRY_NAME_LEN, entry->name);
    IO_I32(file, &offset);
    IO_I32(file, &size);

    KindError kerr =
        kind_guess_entry(entry->name, PAK_ENTRY_NAME_LEN, &entry->kind);
    if (kerr != KIND_ERR_SUCCESS) {
      err = PAK_ERR_MALFORMED;
      snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
               "failed to guess item '%s' kind", entry->name);
      goto cleanup;
    }

    U32 depth = 0;
    err = pak_get_path_depth(entry->name, PAK_ENTRY_NAME_LEN, &depth);
    if (err != PAK_ERR_SUCCESS) {
      snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
               "failed to get path '%s' depth", entry->name);
      goto cleanup;
    }

    PakTreeNode* node = &pak->tree.root;
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

      err = pak_get_item_name_at_depth(name, strlen(name), depth_index,
                                       &item_name[0]);
      if (err != PAK_ERR_SUCCESS) {
        snprintf(pak->error_text, PAK_MAX_ERROR_LENGTH,
                 "failed to get item '%s' name at depth '%d'", name,
                 depth_index);
        goto cleanup;
      }

      HashMapKV* kv = hashmap_find(node->children, str8(name));
      if (kv) {
        node = (PakTreeNode*)kv->v_rawptr;
        continue;
      }

      PakTreeNode* child =
          arena_push(arena, sizeof(PakTreeNode), AlignOf(PakTreeNode), TRUE);
      if (child != 0)
        ;

      memcpy(child->name, name, PAK_ENTRY_NAME_LEN);
      memcpy(child->item_name, item_name, PAK_ENTRY_NAME_LEN);

      hashmap_push_rawptr(arena, node->children, str8(child->name),
                          (RawPtr)child);

      if (depth_index >= depth) {
        child->is_dir = FALSE;
        child->size = size;
        child->offset = offset;
        continue;
      }

      child->is_dir = TRUE;
      child->children = hashmap_init(arena, 64);
      child->parent = node;
      node = child;
    }
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

PakError
pak_load_from_file(Pak* pak, FILE* file) {
  TracyCZoneN(trcyctx, "pak_load_from_file", 1);

  Assert(pak != 0);
  Assert(file != 0);

  PakError err = PAK_ERR_SUCCESS;

  U8 magic_code[PAK_MAGIC_CODE_LEN] = {0};
  I32 offset = 0;
  I32 size = 0;

  pak->arena = arena_create();

  IO_BUF(file, PAK_MAGIC_CODE_LEN, magic_code);
  IO_I32(file, &offset);
  IO_I32(file, &size);
  IO_SET(file, offset);

  if ((offset <= 0) || (size <= 0) || (magic_code[0] != 'P') ||
      (magic_code[1] != 'A') || (magic_code[2] != 'C') ||
      (magic_code[3] != 'K')) {
    err = PAK_ERR_MALFORMED;
    goto cleanup;
  }

  pak->details.entries_count = size / sizeof(PakRawEntry);

  pak->tree.root.parent = 0;
  pak->tree.root.children = hashmap_init(pak->arena, 64);
  pak->tree.root.is_dir = TRUE;
  MemZero(pak->tree.root.name, PAK_ENTRY_NAME_LEN);
  pak->tree.root.name[0] = ' ';

  err = pak_read_entries_from_file(pak, file);
  if (err != PAK_ERR_SUCCESS) {
    goto cleanup;
  }

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

Nothing
pak_unload(Pak* pak) {
  TracyCZoneN(trcyctx, "pak_unload", 1);

  if (pak->arena) {
    arena_destroy(pak->arena);
  }

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */

internal PakError
pak_join_path(Str8 a, Str8 b, Str8 c) {
  TracyCZoneN(trcyctx, "pak_join_path", 1);
  PakError err = PAK_ERR_SUCCESS;

  str8_join(a, b, c, IO_PATH_SEPARATOR);

cleanup:
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

PakError
pak_extract(Pak* pak, FILE* file, Str8 out_dir) {
  TracyCZoneN(trcyctx, "pak_extract", 1);

  PakError err = PAK_ERR_SUCCESS;
  PakTreeNode* node = &pak->tree.root;
  ListNode* list_node;
  List nodes = {0};
  ArenaScratch scratch = arena_scratch_begin(pak->arena);

  list_push(scratch.arena, &nodes, &pak->tree.root);

  while (nodes.count > 0) {
    ListError lerr = list_pop(&nodes, &list_node);
    if (LIST_ERR_SUCCESS != lerr) {
      if (LIST_EMPTY == lerr || 0 == list_node) {
        goto cleanup;
      }
      err = PAK_ERR_EXTRACT;
      goto cleanup;
    }
    node = (PakTreeNode*)list_node->data;

    for (U32 index = 0; index < node->children->count; index++) {
      HashMapKV* kv = hashmap_key_at(node->children, index);
      if (0 == kv) {
        err = PAK_ERR_EXTRACT;
        goto cleanup;
      }

      PakTreeNode* new_node = (PakTreeNode*)kv->v_rawptr;

      char full_path_buf[PAK_ENTRY_MAX_EXTRACT_PATH_LEN] = {0};
      Str8 full_path_str =
          str8_raw(full_path_buf, PAK_ENTRY_MAX_EXTRACT_PATH_LEN);
      Str8 new_node_str = str8(new_node->name);

      pak_join_path(out_dir, new_node_str, full_path_str);

      if (TRUE == new_node->is_dir) {
        ListError lerr = list_push(scratch.arena, &nodes, new_node);
        if (LIST_ERR_SUCCESS != lerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }

        IOError ioerr = io_make_directory(full_path_str);
        if (IO_ERR_SUCCESS != ioerr) {
          err = PAK_ERR_EXTRACT;
          goto cleanup;
        }
      } else {
        CStr src_buffer =
            arena_push(scratch.arena, new_node->size, AlignOf(U8), TRUE);

        IO_SET(file, new_node->offset);
        IO_BUF(file, new_node->size, src_buffer);
        io_dump(full_path_str, str8_raw(src_buffer, new_node->size));
      }
    }
  }

cleanup:
  arena_scratch_end(scratch);
  TracyCZoneEnd(trcyctx);
  return err;
}

/* ===================================================== */

PakError
pak_extract_item(Pak* pak, PakTreeNode* node, FILE* file, Str8 out_dir) {
  TracyCZoneN(trcyctx, "pak_extract_item", 1);
  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_PAK_IMPLEMENTATION
#endif  // MODULE_PAK_HEADER
