#ifndef PAK_HEADER_
#define PAK_HEADER_

#include "../deps/log.h"
#include "../deps/sepi_macros.h"
#include "../deps/sepi_types.h"
#include "../deps/sepi_arena.h"

#include "kind.h"

#define PAK_HEADER_LEN 4
#define PAK_ENTRY_NAME_LEN 56
#define PAK_HEADER_ID "PACK"

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
  arena mem;
} pak;

typedef enum {
  PAK_ERR_UNKNOWN = -1,
  PAK_ERR_SUCCESS = 0,
  PAK_ERR_MALFORMED,
} pak_err;

/* ****************** quake::mdl API ****************** */
pak_err pak_load(cbuf, sz, pak*);
void pak_unload(pak*);
/* ****************** quake::mdl API ****************** */

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

static pak_err pak_estimate_memory(cbuf buf, pak* pak, i32 offset) {
  DBG("trying to estimate required memory");

  arena_begin_estimate(&pak->mem);
  pak_raw_entry* rent = (pak_raw_entry*)(buf + offset);

  sz allsz = 0;
  sz entsz = sizeof(pak_entry);
  for (i32 i = 0; i < pak->entries_count; i++) {
    i32 size = endian_i32(rent->size);
    arena_estimate_add(&pak->mem, size, alignof(u8));
    arena_estimate_add(&pak->mem, entsz, alignof(pak_entry));
    rent++;
  }

  arena_end_estimate(&pak->mem);
  DBG("memory size needed: %d bytes", pak->mem.estimate);

  return PAK_ERR_SUCCESS;
}

static pak_err pak_read_header(cbuf buf, sz bufsz, pak* pak, i32* tbloff) {
  DBG("reading pak header");

  pak_raw_header* rhdr = (pak_raw_header*)buf;

  i32 offset = endian_i32(rhdr->offset);
  i32 size = endian_i32(rhdr->size);
  i32 idlen = (sizeof(PAK_HEADER_ID) / sizeof(char)) - 1;

  errorout(!strncmp(rhdr->id, PAK_HEADER_ID, idlen), PAK_ERR_MALFORMED);
  errorout(offset > 0, PAK_ERR_MALFORMED);
  errorout(size > 0, PAK_ERR_MALFORMED);

  pak->entries_count = size / sizeof(pak_raw_entry);
  errorout(pak->entries_count > 0, PAK_ERR_MALFORMED);

  *tbloff = offset;
  return PAK_ERR_SUCCESS;
}

static pak_err pak_read_entries(cbuf buf, sz bufsz, pak* pak, i32 tbloff) {
  DBG("reading pak entries");

  arena* m = &pak->mem;
  pak_raw_entry* rent = (pak_raw_entry*)(buf + tbloff);
  sz entriessz = sizeof(pak_entry) * pak->entries_count;
  pak_entry* entries =
      (pak_entry*)arena_alloc(m, entriessz, alignof(pak_entry));
  pak->entries = entries;
  for (i32 i = 0; i < pak->entries_count; i++) {
    char* name = rent->name;
    i32 offset = endian_i32(rent->offset);
    i32 size = endian_i32(rent->size);
    cbuf pos = buf + offset;
    entries->size = size;
    entries->data = arena_alloc(m, size, alignof(char));
    memcpy(entries->data, pos, size);
    strncpy(entries->name, name, PAK_ENTRY_NAME_LEN);
    entries->kind = kind_guess_entry(entries->name, PAK_ENTRY_NAME_LEN);
    if (entries->kind == KIND_UNKNOWN)
      log_warn("the entry kind for '%s' is unknonw", entries->name);
    rent++;
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

  pak_entry* entries = pak->entries;
  for (i32 i = 0; i < pak->entries_count; i++) {
    printf("%s\n", entries->name);
    entries++;
  }

  return PAK_ERR_SUCCESS;
}

void pak_unload(pak* pak) {}

#ifdef PAK_IMPLEMENTATION

#endif  // PAK_IMPLEMENTATION
#endif  // PAK_HEADER_
