#ifndef PAK_HEADER_
#define PAK_HEADER_

#include <string.h>
#include <stdlib.h>

#include "deps/sepi/endian.h"
#include "deps/sepi/arena.h"

#include "?kind.h"

#define PAK_HEADER_LEN 4
#define PAK_ENTRY_NAME_LEN 56
#define PAK_HEADER_ID "PACK"

typedef enum {
  PAK_ERR_UNKNOWN = -1,
  PAK_ERR_SUCCESS = 0,
  PAK_ERR_MALFORMED,
} pak_err;

typedef struct {
  char id[PAK_HEADER_LEN];
  i32 offset;
  i32 size;
} pak_raw_header;

typedef struct {
  char name[PAK_ENTRY_NAME_LEN];
  i32 offset;
  i32 size;
} pak_raw_entry;

typedef struct {
  char name[PAK_ENTRY_NAME_LEN];
  kind kind;
  sz size;
  void* data;
} pak_entry;

typedef struct {
  u32 entries_count;
  pak_entry* entries;
  char* _ex_pak_tree_list[PAK_ENTRY_NAME_LEN];
  arena mem;
} pak;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

pak_err pak_load(cbuf, sz, pak*);
void pak_unload(pak*);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef PAK_IMPLEMENTATION

static pak_err pak_estimate_memory(cbuf buf, pak* pak, i32 offset) {
  DBG("trying to estimate required memory");

  arena_estimate_begin(&pak->mem);
  pak_raw_entry* rent = (pak_raw_entry*)(buf + offset);

  sz entsz = sizeof(pak_entry);
  for (i32 i = 0; i < pak->entries_count; i++) {
    i32 size = endian_i32(rent->size);
    arena_estimate_add(&pak->mem, size, alignof(u8));
    arena_estimate_add(&pak->mem, entsz, alignof(pak_entry));
    rent++;
  }

  arena_estimate_end(&pak->mem);
  DBG("memory size needed: %d bytes", pak->mem.estimation);

  return PAK_ERR_SUCCESS;
}

static pak_err pak_read_header(cbuf buf, sz bufsz, pak* pak, i32* tbloff) {
  DBG("reading pak header");

  pak_raw_header* rhdr = (pak_raw_header*)buf;

  i32 offset = endian_i32(rhdr->offset);
  i32 size = endian_i32(rhdr->size);
  i32 idlen = (sizeof(PAK_HEADER_ID) / sizeof(char)) - 1;

  ERROROUT(!strncmp(rhdr->id, PAK_HEADER_ID, idlen), PAK_ERR_MALFORMED);
  ERROROUT(offset > 0, PAK_ERR_MALFORMED);
  ERROROUT(size > 0, PAK_ERR_MALFORMED);

  pak->entries_count = size / sizeof(pak_raw_entry);
  ERROROUT(pak->entries_count > 0, PAK_ERR_MALFORMED);

  *tbloff = offset;
  return PAK_ERR_SUCCESS;
}

static pak_err pak_read_entries(cbuf buf, sz bufsz, pak* pak, i32 tbloff) {
  DBG("reading pak entries");

  arena* m = &pak->mem;
  pak_raw_entry* ptr = (pak_raw_entry*)(buf + tbloff);
  sz sz = sizeof(pak_entry) * pak->entries_count;
  pak_entry* entries = (pak_entry*)arena_alloc(m, sz, alignof(pak_entry));

  pak->entries = entries;

  for (i32 i = 0; i < pak->entries_count; i++) {
    char* name = ptr->name;
    i32 offset = endian_i32(ptr->offset);
    i32 size = endian_i32(ptr->size);
    cbuf pos = buf + offset;
    entries->size = size;
    entries->data = arena_alloc(m, size, alignof(char));
    memcpy(entries->data, pos, size);
    strncpy(entries->name, name, PAK_ENTRY_NAME_LEN);
    entries->kind = kind_guess_entry(entries->name, PAK_ENTRY_NAME_LEN);
    if (entries->kind == KIND_UNKNOWN)
      log_warn("the entry kind for '%s' is unknonw", entries->name);

    ptr++;
    entries++;
  }

  return PAK_ERR_SUCCESS;
}

pak_err pak_load(cbuf buf, sz bufsz, pak* pak) {
  i32 tbloff;
  pak_err err = pak_read_header(buf, bufsz, pak, &tbloff);
  if (err != PAK_ERR_SUCCESS) {
    return err;
  }

  pak_estimate_memory(buf, pak, tbloff);
  pak_read_entries(buf, bufsz, pak, tbloff);

  return PAK_ERR_SUCCESS;
}

void pak_unload(pak* pak) {
  arena_destroy(&pak->mem);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif  // PAK_IMPLEMENTATION
#endif  // PAK_HEADER_
