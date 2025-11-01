#ifndef PAK_HEADER_
#define PAK_HEADER_

#include <string.h>
#include <stdlib.h>

#include "deps/sepi/endian.h"
#include "deps/sepi/arena.h"

#include "kind.h"

#define PAK_HEADER_LEN 4
#define PAK_ENTRY_NAME_LEN 56
#define PAK_HEADER_ID "PACK"

typedef enum {
  PAK_ERR_UNKNOWN = -1,
  PAK_ERR_SUCCESS = 0,
  PAK_ERR_MALFORMED,
} PakError;

typedef struct {
  char id[PAK_HEADER_LEN];
  I32 offset;
  I32 size;
} PakRawHeader;

typedef struct {
  char name[PAK_ENTRY_NAME_LEN];
  I32 offset;
  I32 size;
} PakRawEntry;

typedef struct {
  char name[PAK_ENTRY_NAME_LEN];
  Kind kind;
  Sz size;
  RawPtr data;
} PakEntry;

typedef struct {
  U32 entries_count;
  PakEntry* entries;
  char* _ex_pak_tree_list[PAK_ENTRY_NAME_LEN];
  Arena* arena;
} Pak;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

PakError pak_load(CBuf, Sz, Pak*);
Nothing pak_unload(Pak*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef PAK_IMPLEMENTATION

static PakError
pak_read_header(CBuf buf, Sz bufsz, Pak* pak, I32* tbloff) {
  Dbg("reading pak header");

  PakRawHeader* rhdr = (PakRawHeader*)buf;

  I32 offset = nd_i32(rhdr->offset);
  I32 size = nd_i32(rhdr->size);
  I32 idlen = (sizeof(PAK_HEADER_ID) / sizeof(char)) - 1;

  ErrorOut(!strncmp(rhdr->id, PAK_HEADER_ID, idlen), PAK_ERR_MALFORMED);
  ErrorOut(offset > 0, PAK_ERR_MALFORMED);
  ErrorOut(size > 0, PAK_ERR_MALFORMED);

  pak->entries_count = size / sizeof(PakRawEntry);
  ErrorOut(pak->entries_count > 0, PAK_ERR_MALFORMED);

  *tbloff = offset;
  return PAK_ERR_SUCCESS;
}

static PakError
pak_read_entries(CBuf buf, Sz bufsz, Pak* pak, I32 tbloff) {
  Dbg("reading pak entries");

  Arena* arena = pak->arena;
  PakRawEntry* ptr = (PakRawEntry*)(buf + tbloff);
  Sz sz = sizeof(PakEntry) * pak->entries_count;
  PakEntry* entries = (PakEntry*)arena_push(arena, sz, alignof(PakEntry), TRUE);

  pak->entries = entries;

  for (I32 i = 0; i < pak->entries_count; i++) {
    char* name = ptr->name;
    I32 offset = nd_i32(ptr->offset);
    I32 size = nd_i32(ptr->size);
    CBuf pos = buf + offset;
    entries->size = size;
    entries->data = arena_push(arena, size, alignof(char), TRUE);
    memcpy(entries->data, pos, size);
    strncpy(entries->name, name, PAK_ENTRY_NAME_LEN);
    entries->kind = kind_guess_entry(entries->name, PAK_ENTRY_NAME_LEN);
    if (entries->kind == KIND_UNKNOWN) {
      log_warn("the entry kind for '%s' is unknonw", entries->name);
    }

    ptr++;
    entries++;
  }

  return PAK_ERR_SUCCESS;
}

PakError
pak_load(CBuf buf, Sz bufsz, Pak* pak) {
  Arena* arena = arena_alloc(.requested_reserve_size = 1024,
                             .requested_commit_size = 1024);
  pak->arena = arena;

  I32 tbloff;
  PakError err = pak_read_header(buf, bufsz, pak, &tbloff);
  if (err != PAK_ERR_SUCCESS) {
    return err;
  }

  pak_read_entries(buf, bufsz, pak, tbloff);

  return PAK_ERR_SUCCESS;
}

void
pak_unload(Pak* pak) {
  if(pak->arena) {
    arena_release(pak->arena);
  }
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // PAK_IMPLEMENTATION
#endif  // PAK_HEADER_
