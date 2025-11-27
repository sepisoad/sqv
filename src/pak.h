/*
 * Copyright 2025 Sepehr Aryani (me@sepi.me)
 * Licensed under LGPL v3
 */

#ifndef PAK_HEADER_
#define PAK_HEADER_

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <string.h>
#include <stdlib.h>

#include "deps/sepi/arena.h"
#include "deps/sepi/endian.h"

#include "kind.h"

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
  PAK_ERR__COUNT,
} PakError;

typedef struct {
  char name[PAK_ENTRY_NAME_LEN];
  I32  offset;
  I32  size;
} PakRawEntry;

typedef struct {
  U32 entries_count;
} PakDetails;

typedef struct {
  char   name[PAK_ENTRY_NAME_LEN];
  Kind   kind;
  Sz     size;
  RawPtr data;
} PakEntry;

typedef struct {
  PakDetails details;
  PakEntry*  entries;
  char*      _ex_pak_tree_list[PAK_ENTRY_NAME_LEN];
  Arena*     arena;
} Pak;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError pak_load(Pak*, NDBuffer*);
Nothing  pak_unload(Pak*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef PAK_IMPLEMENTATION

internal PakError
pak_read_entries(Pak* pak, NDBuffer* ndb) {
  Dbg("pak_read_entries() ...");

  Assert(pak != 0);
  Assert(ndb != 0);

  Arena*       arena = pak->arena;
  Sz           sz = sizeof(PakEntry) * pak->details.entries_count;
  PakEntry* entries = (PakEntry*)arena_push(arena, sz, AlignOf(PakEntry), TRUE);

  pak->entries = entries;

  for (U32 i = 0; i < pak->details.entries_count; i++) {
    PakEntry* entry = entries + i;
    U32       offset = 0;

    memcpy(entry->name, ND_ADDR(ndb), PAK_ENTRY_NAME_LEN);
    ND_MOVE(ndb, PAK_ENTRY_NAME_LEN);
    ND_I32(ndb, &offset);
    ND_I32(ndb, &entry->size);

    KindError err =
        kind_guess_entry(entry->name, PAK_ENTRY_NAME_LEN, &entry->kind);
    if (err != KIND_ERR_SUCCESS) {
      return PAK_ERR_MALFORMED;
    }

    entry->data = arena_push(arena, entry->size, AlignOf(U8), TRUE);
    memcpy(entry->data, ndb->base + offset, entry->size);
  }

  return PAK_ERR_SUCCESS;
}

// TODO: do i need the `buffer_size`
PakError
pak_load(Pak* pak, NDBuffer* ndb) {
  Dbg("pak_load() ...");

  Assert(pak != 0);
  Assert(ndb != 0);
  Assert(pak->arena == 0);

  PakError     err;
  U8           magic_code[PAK_MAGIC_CODE_LEN] = {0};
  I32          offset = 0;
  I32          size = 0;

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

  err = pak_read_entries(pak, ndb);
  if (err != PAK_ERR_SUCCESS) {
    return err;
  }

  return PAK_ERR_SUCCESS;
}

Nothing
pak_unload(Pak* pak) {
  if (pak->arena) {
    arena_destroy(pak->arena);
  }
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // PAK_IMPLEMENTATION
#endif  // PAK_HEADER_
