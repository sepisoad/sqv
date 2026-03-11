#ifndef SEPI_MAP__CLASS__H
#define SEPI_MAP__CLASS__H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#define _EXTRA_DEFINES_

#include <tracy/tracy.h>
#include <rapidhash/rapidhash.h>

#include <sepi/base.h>
#include <sepi/arena.h>
#include <sepi/platform.h>
#include <sepi/string.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct _Class_ _Class_;

typedef struct Map_Class_Node Map_Class_Node;
struct Map_Class_Node {
  Map_Class_Node* next;
  Str key;
  _Type_* value;
};

typedef struct Map_Class_ Map_Class_;
struct Map_Class_ {
  U64 keys_count;
  U64 max_keys_list_length;
  Map_Class_Node** keys_list;
  Arena* arena;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Map_Class_ map__class__make(Arena* arena, U64 max_keys_list_length);
Nothing map__class__push(Map_Class_* map, Str key, _Class_* value);
_Class_* map__class__get(Map_Class_* map, Str key);
_Class_* map__class__delete(Map_Class_* map, Str key);
Nothing map__class__clean(Map_Class_* map);
Nothing map__class__keys(Map_Class_* map, Str** keys, U64* length);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_MAP__CLASS__IMPLEMENTATION

mount_slave_profiling_context();

#define map__class__hasher(str) rapidhash_withSeed(ZS((str)), SL((str)), 1987)

Map_Class_
map__class__make(Arena* arena, U64 max_keys_list_length) {
  start_profiling(1);

  assert(arena != 0);
  assert(max_keys_list_length > 0);

  Map_Class_ map = {
      .arena = arena,
      .keys_count = 0,
      .max_keys_list_length = max_keys_list_length,
      .keys_list =
          arena_push(arena, sizeof(Map_Class_Node*) * max_keys_list_length,
                     alignof(Map_Class_Node*), TRUE),
  };

  end_profiling();
  return map;
}

Nothing
map__class__push(Map_Class_* map, Str key, _Class_* value) {
  start_profiling(1);

  assert(map != 0);
  assert(ZS(key) != 0);
  assert(key.length != 0);
  assert(value != 0);

  U64 key_hash = map__class__hasher(key);
  U64 key_index = key_hash % map->max_keys_list_length;
  Map_Class_Node* first_node = map->keys_list[key_index];
  Map_Class_Node* iter = first_node;

  while (0 != iter) {
    if (str_equal(iter->key, key, 0)) {
      iter->value = value;
      goto cleanup;
    }
    iter = iter->next;
  }

  Map_Class_Node* new_node = arena_push(map->arena, sizeof(Map_Class_Node),
                                        alignof(Map_Class_Node), TRUE);
  new_node->key = str_clone(map->arena, key);
  new_node->value = value;

  if (first_node) {
    new_node->next = first_node;
  }
  map->keys_list[key_index] = new_node;
  map->keys_count++;

cleanup:
  end_profiling();
}

_Class_*
map__class__get(Map_Class_* map, Str key) {
  start_profiling(1);

  assert(map != 0);
  assert(ZS(key) != 0);
  assert(key.length != 0);

  _Class_* found = {0};
  U64 key_hash = map__class__hasher(key);
  U64 key_index = key_hash % map->max_keys_list_length;
  Map_Class_Node* iter = map->keys_list[key_index];

  while (0 != iter) {
    if (str_equal(iter->key, key, 0)) {
      found = iter->value;
      goto cleanup;
    }
    iter = iter->next;
  }

cleanup:
  end_profiling();
  return found;
}

_Class_*
// cppcheck-suppress unusedFunction
map__class__delete(Map_Class_* map, Str key) {
  start_profiling(1);

  assert(map != 0);
  assert(ZS(key) != 0);
  assert(key.length != 0);

  _Class_* found = {0};
  U64 key_hash = map__class__hasher(key);
  U64 key_index = key_hash % map->max_keys_list_length;
  Map_Class_Node* first = map->keys_list[key_index];
  Map_Class_Node* iter = first;
  Map_Class_Node* prev = first;

  while (0 != iter) {
    if (str_equal(iter->key, key, 0)) {
      found = iter->value;
      prev->next = iter->next;
      if (iter == first)
        map->keys_list[key_index] = iter->next;

      map->keys_count--;
      goto cleanup;
    }
    prev = iter;
    iter = iter->next;
  }

cleanup:
  end_profiling();
  return found;
}

Nothing
// cppcheck-suppress unusedFunction
map__class__clean(Map_Class_* map) {
  start_profiling(1);

  assert(map != 0);

  for (U64 key_index = 0; key_index < map->max_keys_list_length; key_index++) {
    map->keys_list[key_index] = 0;
  }

  end_profiling();
}

Nothing
// cppcheck-suppress unusedFunction
map__class__keys(Map_Class_* map, Str** keys, U64* length) {
  start_profiling(1);

  assert(map != 0);
  assert(keys != 0);
  assert(length != 0);

  U64 key_index = 0;
  *keys = arena_push(map->arena, sizeof(Str) * map->keys_count, alignof(Str), TRUE);

  for (U64 key_list_index = 0; key_list_index < map->max_keys_list_length; key_list_index++) {
    const Map_Class_Node* iter = map->keys_list[key_list_index];
    while(0 != iter) {
      (*keys)[key_index++] = iter->key;
      iter = iter->next;
    }
  }

  *length = map->keys_count;
  assert(map->keys_count == key_index);

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_MAP__CLASS__IMPLEMENTATION */
#endif /* SEPI_MAP__CLASS__H */
