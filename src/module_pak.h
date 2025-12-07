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
#include "deps/sepi/hashmap.h"
#include "deps/sepi/string.h"

#include "module_kind.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define PAK_MAGIC_CODE_LEN 4
#define PAK_ENTRY_NAME_LEN 56

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  PAK_ERR_UNKNOWN,
  PAK_ERR_SUCCESS,
  PAK_ERR_MALFORMED,
  PAK_ERR_INVALID_ENTRY_PATH,
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
  Sz size;
} PakEntry;

typedef struct PakTreeNode PakTreeNode;

struct PakTreeNode {
  char name[PAK_ENTRY_NAME_LEN];
  Bool is_dir;
  Bool is_deleted;
  U32 actual_count;
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
  Arena* arena;
} Pak;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError pak_load(Pak*, NDBuffer*);
Nothing pak_unload(Pak*);
PakError pak_extract(Pak*);
PakError pak_extract_item(Pak*, PakTreeNode* node);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef MODULE_PAK_IMPLEMENTATION

internal PakError
pak_get_path_depth(CStr path, U32 length, U32* depth) {
  Dbg("pak_get_path_depth() ...");

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

  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

internal PakError
pak_get_path_segment_at_depth(CStr path,
                              U32 length,
                              U32 segments,
                              char out[PAK_ENTRY_NAME_LEN]) {
  Dbg("pak_get_path_depth() ...");

  Assert(path != 0);
  Assert(out != 0);

  U32 traversed = 0;

  for (U32 index = 0; index < length; index++) {
    if (path[index] == 0 || traversed >= segments) {
      break;
    }

    if (path[index] == '/') {
      traversed++;
    }

    out[index] = path[index];
  }

  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

internal PakError
pak_read_entries(Pak* pak, NDBuffer* ndb) {
  Dbg("pak_read_entries() ...");

  Assert(pak != 0);
  Assert(ndb != 0);

  Arena* arena = pak->arena;
  Sz sz = sizeof(PakEntry) * pak->details.entries_count;
  PakEntry* entries = (PakEntry*)arena_push(arena, sz, AlignOf(PakEntry), TRUE);

  pak->entries = entries;

  for (U32 index = 0; index < pak->details.entries_count; index++) {
    PakEntry* entry = entries + index;
    U32 offset = 0;

    // NOTE: i was under impression the the name buffer is filled with zeros
    //       after the last character, but i was proven wrong when i tested
    //       https://www.slipseer.com/index.php?resources/dwell.21/
    //       unfortunately using memcpy here is not that safe
    //       so i had to compromise and use strncpy instead!
    //       ---
    //       memcpy(entry->name, ND_ADDR(ndb), PAK_ENTRY_NAME_LEN);

    strncpy(entry->name, ND_ADDR(ndb), PAK_ENTRY_NAME_LEN);
    ND_MOVE(ndb, PAK_ENTRY_NAME_LEN);
    ND_I32(ndb, &offset);
    ND_I32(ndb, &entry->size);

    KindError kerr =
        kind_guess_entry(entry->name, PAK_ENTRY_NAME_LEN, &entry->kind);
    if (kerr != KIND_ERR_SUCCESS) {
      return PAK_ERR_MALFORMED;
    }

    U32 depth = 0;
    PakError perr = pak_get_path_depth(entry->name, PAK_ENTRY_NAME_LEN, &depth);
    if (perr != PAK_ERR_SUCCESS) {
      return perr;
    }

    PakTreeNode* node = &pak->tree.root;
    for (U32 depth_index = 0; depth_index < depth + 1; depth_index++) {
      char name[PAK_ENTRY_NAME_LEN] = {0};

      PakError perr = pak_get_path_segment_at_depth(
          entry->name, PAK_ENTRY_NAME_LEN, depth_index + 1, &name[0]);
      if (perr != PAK_ERR_SUCCESS) {
        return perr;
      }

      HashMapKV* kv = hashmap_find(node->children, str8(name));
      if (kv) {
        node = (PakTreeNode*)kv->v_rawptr;
        continue;
      }

      PakTreeNode* child =
          arena_push(arena, sizeof(PakTreeNode), AlignOf(PakTreeNode), TRUE);
      AssertAlways(child != 0);
      node->actual_count++;

      memcpy(child->name, name, PAK_ENTRY_NAME_LEN);
      hashmap_push_rawptr(arena, node->children, str8(child->name),
                          (RawPtr)child);

      if (depth_index >= depth) {
        child->is_dir = FALSE;
        continue;
      }

      child->is_deleted = FALSE;
      child->is_dir = TRUE;
      child->children = hashmap_init(arena, 64);
      child->parent = node;
      node = child;
      node->actual_count = 0;
    }
  }

  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

PakError
pak_load(Pak* pak, NDBuffer* ndb) {
  Dbg("pak_load() ...");

  Assert(pak != 0);
  Assert(ndb != 0);
  Assert(pak->arena == 0);

  PakError err;
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
  AssertAlways(offset > 0);
  AssertAlways(size > 0);
  AssertAlways(magic_code[0] == 'P');
  AssertAlways(magic_code[1] == 'A');
  AssertAlways(magic_code[2] == 'C');
  AssertAlways(magic_code[3] == 'K');

  pak->details.entries_count = size / sizeof(PakRawEntry);

  // TODO: find a proper default 'cap'
  pak->tree.root.parent = 0;
  pak->tree.root.children = hashmap_init(pak->arena, 64);
  pak->tree.root.is_deleted = FALSE;
  pak->tree.root.is_dir = TRUE;
  MemZero(pak->tree.root.name, PAK_ENTRY_NAME_LEN);
  pak->tree.root.name[0] = ' ';

  err = pak_read_entries(pak, ndb);
  if (err != PAK_ERR_SUCCESS) {
    return err;
  }
  pak->tree.root.actual_count = pak->tree.root.children->count;

  return PAK_ERR_SUCCESS;
}

/* ===================================================== */

Nothing
pak_unload(Pak* pak) {
  if (pak->arena) {
    arena_destroy(pak->arena);
  }
}

/* ===================================================== */

PakError pak_extract(Pak*) {
}

/* ===================================================== */

PakError pak_extract_item(Pak*, PakTreeNode* node) {
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // MODULE_PAK_IMPLEMENTATION
#endif  // MODULE_PAK_HEADER
