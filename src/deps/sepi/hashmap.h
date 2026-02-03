#ifndef SEPI_HASHMAP_H
#define SEPI_HASHMAP_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../tracy/tracy.h"
#include "../rapidhash/rapidhash.h"

#include "base.h"
#include "string.h"
#include "arena.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

//

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

// TODO: update api to return HashMapError to achieve a more
//       unified api
typedef enum {
  HASHMAP_ERR_SUCCESS = 1,
  HASHMAP_ERR__COUNT,
} HashMapError;

typedef struct HashMapKV HashMapKV;
struct HashMapKV {
  union {
    Str8 k_str;
    RawPtr k_rawptr;
    U32 k_u32;
    U64 k_u64;
  };
  union {
    Str8 v_str;
    RawPtr v_rawptr;
    U32 v_u32;
    U64 v_u64;
  };
};

typedef struct HashMapNode HashMapNode;
struct HashMapNode {
  HashMapNode* next;
  HashMapKV kv;
};

typedef struct HashMapList HashMapList;
struct HashMapList {
  HashMapNode* first;
  HashMapNode* last;
};

typedef struct HashMap HashMap;
struct HashMap {
  U64 count;
  U64 capacity;
  HashMapList* list;
  HashMapList free_list;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

HashMap* hashmap_init(Arena* a, U64 cap);
Nothing hashmap_purge(HashMap* hm);
HashMapNode* hashmap_push(Arena* a, HashMap* hm, U64 hash, HashMapKV kv);
HashMapNode* hashmap_push_str8(Arena* a, HashMap* hm, Str8 key, Str8 value);
HashMapNode* hashmap_push_rawptr(Arena* a, HashMap* hm, Str8 key, RawPtr value);
HashMapNode* hashmap_push_u32(Arena* a, HashMap* hm, Str8 key, U32 value);
HashMapNode* hashmap_push_u64(Arena* a, HashMap* hm, Str8 key, U64 value);
HashMapKV* hashmap_find(HashMap* hm, Str8 key);
HashMapKV hashmap_pop(HashMap* hm, Str8 key);
Str8* hashmap_keys(Arena* a, HashMap* hm);
HashMapKV* hashmap_key_at(HashMap* hm, U32 index);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_HASHMAP_IMPLEMENTATION

MASTER_PROFILING_CONTEXT;

static Nothing
hashmap_list_concat_in_place(HashMapList* to, HashMapList* from) {
  START_PROFILING(1);

  if (from->first) {
    if (to->first) {
      to->last->next = from->first;
      to->last = from->last;
    } else {
      to->first = from->first;
      to->last = from->last;
    }
    MemZeroStruct(from);
  }

  END_PROFILING();
}

static HashMapNode*
hashmap_list_pop(HashMapList* hml) {
  START_PROFILING(1);

  HashMapNode* hmn = hml->first;

  if (hml->first == hml->last) {
    hml->first = 0;
    hml->last = 0;
  } else {
    hml->first = hml->first->next;
  }

  END_PROFILING();
  return hmn;
}

static U64
hashmap_hasher(Str8 str) {
  return rapidhash_withSeed(CS(str), SL(str), 1987);
}

HashMap*
hashmap_init(Arena* a, U64 capacity) {
  START_PROFILING(1);

  HashMap* hm = arena_push_array(a, HashMap, 1);
  hm->capacity = capacity;
  hm->list = arena_push_array(a, HashMapList, capacity);

  END_PROFILING();
  return hm;
}

Nothing
hashmap_purge(HashMap* hm) {
  START_PROFILING(1);

  hm->count = 0;

  for (U64 i = 0; i < hm->capacity; ++i) {
    hashmap_list_concat_in_place(&hm->free_list, &hm->list[i]);
  }

  END_PROFILING();
}

HashMapNode*
hashmap_push(Arena* a, HashMap* hm, U64 hash, HashMapKV kv) {
  START_PROFILING(1);

  HashMapNode* hmn;

  if (hm->free_list.first != 0) {
    hmn = hashmap_list_pop(&hm->free_list);
  } else {
    hmn = arena_push_array(a, HashMapNode, 1);
  }

  hmn->next = 0;
  hmn->kv = kv;

  U64 i = hash % hm->capacity;

  if (hm->list[i].first == 0) {
    hm->list[i].first = hm->list[i].last = hmn;
    hmn->next = 0;
  } else {
    hm->list[i].last->next = hmn;
    hm->list[i].last = hmn;
    hmn->next = 0;
  }

  hm->count += 1;

  END_PROFILING();
  return hmn;
}

HashMapNode*
hashmap_push_str8(Arena* a, HashMap* hm, Str8 key, Str8 value) {
  U64 hash = hashmap_hasher(key);
  return hashmap_push(a, hm, hash, (HashMapKV){.k_str = key, .v_str = value});
}

HashMapNode*
hashmap_push_rawptr(Arena* a, HashMap* hm, Str8 key, RawPtr value) {
  U64 hash = hashmap_hasher(key);
  return hashmap_push(a, hm, hash,
                      (HashMapKV){.k_str = key, .v_rawptr = value});
}

HashMapNode*
hashmap_push_u32(Arena* a, HashMap* hm, Str8 key, U32 value) {
  U64 hash = hashmap_hasher(key);
  return hashmap_push(a, hm, hash, (HashMapKV){.k_str = key, .v_u32 = value});
}

HashMapNode*
hashmap_push_u64(Arena* a, HashMap* hm, Str8 key, U64 value) {
  U64 hash = hashmap_hasher(key);
  return hashmap_push(a, hm, hash, (HashMapKV){.k_str = key, .v_u64 = value});
}

HashMapNode*
hashmap_push_u32_str8(Arena* a, HashMap* hm, U32 key, Str8 value) {
  Str8 strkey = str8_raw(&key, sizeof(key));
  U64 hash = hashmap_hasher(strkey);
  return hashmap_push(a, hm, hash, (HashMapKV){.k_u32 = key, .v_str = value});
}

HashMapKV*
hashmap_find(HashMap* hm, Str8 key) {
  START_PROFILING(1);

  U64 hash = hashmap_hasher(key);
  U64 i = hash % hm->capacity;
  HashMapList* list = hm->list + i;
  for (HashMapNode* hmn = list->first; hmn != 0; hmn = hmn->next) {
    if (str8_cmp(hmn->kv.k_str, key, 0)) {
      END_PROFILING();
      return &hmn->kv;
    }
  }

  END_PROFILING();
  return 0;
}

HashMapKV
hashmap_pop(HashMap* hm, Str8 key) {
  START_PROFILING(1);

  HashMapKV kv = {0};
  U64 hash = hashmap_hasher(key);
  U64 i = hash % hm->capacity;
  HashMapList* list = hm->list + i;
  HashMapNode* itr = list->first;
  HashMapNode* prv = itr;
  Bool single = list->first == list->last ? TRUE : FALSE;
  for (; itr != 0; prv = itr, itr = itr->next) {
    if (str8_cmp(itr->kv.k_str, key, 0)) {
      prv->next = itr->next;
      kv = itr->kv;
      MemZeroStruct(itr);
      if (single) {
        MemZeroStruct(list);
      }
      hm->count--;

      END_PROFILING();
      return kv;
    }
  }

  END_PROFILING();
  return kv;
}

Str8*
hashmap_keys(Arena* a, HashMap* hm) {
  START_PROFILING(1);

  Str8* keys = arena_push_array_no_zero(a, Str8, hm->count);
  for (U64 listidx = 0, cursor = 0; listidx < hm->capacity; ++listidx) {
    for (HashMapNode* node = hm->list[listidx].first; node != 0;
         node = node->next) {
      Assert(cursor < hm->count);
      keys[cursor++] = node->kv.k_str;
    }
  }

  END_PROFILING();
  return keys;
}

HashMapKV*
hashmap_key_at(HashMap* hm, U32 index) {
  START_PROFILING(1);

  for (U64 listidx = 0, cursor = 0; listidx < hm->capacity; ++listidx) {
    for (HashMapNode* node = hm->list[listidx].first; node != 0;
         node = node->next) {
      if (cursor == index) {
        END_PROFILING();
        return &node->kv;
      }
      Assert(cursor < hm->count);
      cursor++;
    }
  }

  END_PROFILING();
  return 0;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_HASHMAP_IMPLEMENTATION */
#endif /* SEPI_HASHMAP_H */
