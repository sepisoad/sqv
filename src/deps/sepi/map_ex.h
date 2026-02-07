#ifndef SEPI_MAP_EX_H
#define SEPI_MAP_EX_H

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

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef enum {
  MAP_EX_ERR_SUCCESS = 1,
  MAP_EX_ERR__COUNT,
} MapError;

#define DefineMap(TYPE, CLASS, METHOD)                                         \
  typedef struct Map##CLASS##Node Map##CLASS##Node;                            \
  struct Map##CLASS##Node {                                                    \
    Map##CLASS##Node* next;                                                    \
    Str8 key;                                                                  \
    TYPE value;                                                                \
  };                                                                           \
                                                                               \
  typedef struct Map##CLASS##List Map##CLASS##List;                            \
  struct Map##CLASS##List {                                                    \
    Map##CLASS##Node* first;                                                   \
    Map##CLASS##Node* last;                                                    \
  };                                                                           \
                                                                               \
  typedef struct Map##CLASS Map##CLASS;                                        \
  struct Map##CLASS {                                                          \
    U64 length;                                                                \
    U64 capacity;                                                              \
    Map##CLASS##List* list;                                                    \
    Map##CLASS##List free_list;                                                \
    Arena* arena;                                                              \
  };                                                                           \
                                                                               \
  Nothing map_##METHOD##_list_concat_in_place(Map##CLASS##List* to,            \
                                              Map##CLASS##List* from) {        \
    START_PROFILING(1);                                                        \
                                                                               \
    if (from->first) {                                                         \
      if (to->first) {                                                         \
        to->last->next = from->first;                                          \
        to->last = from->last;                                                 \
      } else {                                                                 \
        to->first = from->first;                                               \
        to->last = from->last;                                                 \
      }                                                                        \
      MemZeroStruct(from);                                                     \
    }                                                                          \
                                                                               \
    END_PROFILING();                                                           \
  }                                                                            \
                                                                               \
  Map##CLASS##Node* map_##METHOD##_list_pop(Map##CLASS##List* list) {          \
    START_PROFILING(1);                                                        \
                                                                               \
    Map##CLASS##Node* node = list->first;                                      \
                                                                               \
    if (list->first == list->last) {                                           \
      list->first = 0;                                                         \
      list->last = 0;                                                          \
    } else {                                                                   \
      list->first = list->first->next;                                         \
    }                                                                          \
                                                                               \
    END_PROFILING();                                                           \
    return node;                                                               \
  }                                                                            \
                                                                               \
  U64 map_##METHOD##_hasher(Str8 str) {                                        \
    return rapidhash_withSeed(CS(str), SL(str), 1987);                         \
  }                                                                            \
                                                                               \
  Map##CLASS map_##METHOD##_make(Arena* arena, U64 capacity) {                 \
    START_PROFILING(1);                                                        \
                                                                               \
    Map##CLASS map = {                                                         \
        .capacity = capacity,                                                  \
        .list = arena_push_array(arena, Map##CLASS##List, capacity),           \
        .arena = arena,                                                        \
    };                                                                         \
                                                                               \
    END_PROFILING();                                                           \
    return map;                                                                \
  }                                                                            \
                                                                               \
  Nothing map_##METHOD##_clean(Map##CLASS* map) {                              \
    START_PROFILING(1);                                                        \
                                                                               \
    map->length = 0;                                                           \
                                                                               \
    for (U64 i = 0; i < map->capacity; ++i) {                                  \
      map_##METHOD##_list_concat_in_place(&map->free_list, &map->list[i]);     \
    }                                                                          \
                                                                               \
    END_PROFILING();                                                           \
  }                                                                            \
                                                                               \
  Map##CLASS##Node* map_##METHOD##_push(Map##CLASS* map, Str8 key,             \
                                        TYPE value) {                          \
    START_PROFILING(1);                                                        \
                                                                               \
    Map##CLASS##Node* map_node;                                                \
    U64 hash = map_##METHOD##_hasher(key);                                     \
                                                                               \
    if (map->free_list.first != 0) {                                           \
      map_node = map_##METHOD##_list_pop(&map->free_list);                     \
    } else {                                                                   \
      map_node = arena_push_array(map->arena, Map##CLASS##Node, 1);            \
    }                                                                          \
                                                                               \
    map_node->next = 0;                                                        \
    map_node->key = key;                                                       \
    map_node->value = value;                                                   \
                                                                               \
    U64 i = hash % map->capacity;                                              \
                                                                               \
    if (map->list[i].first == 0) {                                             \
      map->list[i].first = map->list[i].last = map_node;                       \
      map_node->next = 0;                                                      \
    } else {                                                                   \
      map->list[i].last->next = map_node;                                      \
      map->list[i].last = map_node;                                            \
      map_node->next = 0;                                                      \
    }                                                                          \
                                                                               \
    map->length += 1;                                                          \
                                                                               \
    END_PROFILING();                                                           \
    return map_node;                                                           \
  }                                                                            \
                                                                               \
  TYPE map_##METHOD##_find(Map##CLASS* map, Str8 key) {                        \
    START_PROFILING(1);                                                        \
                                                                               \
    U64 hash = map_##METHOD##_hasher(key);                                     \
    U64 i = hash % map->capacity;                                              \
    Map##CLASS##List* list = map->list + i;                                    \
    for (Map##CLASS##Node* node = list->first; node != 0; node = node->next) { \
      if (str8_equal(node->key, key, 0)) {                                     \
        END_PROFILING();                                                       \
        return node->value;                                                    \
      }                                                                        \
    }                                                                          \
                                                                               \
    END_PROFILING();                                                           \
    return (TYPE){};                                                           \
  }                                                                            \
                                                                               \
  TYPE map_##METHOD##_pop(Map##CLASS* map, Str8 key) {                         \
    START_PROFILING(1);                                                        \
                                                                               \
    TYPE value;                                                                \
    U64 hash = map_##METHOD##_hasher(key);                                     \
    U64 i = hash % map->capacity;                                              \
    Map##CLASS##List* list = map->list + i;                                    \
    Map##CLASS##Node* itr = list->first;                                       \
    Map##CLASS##Node* prv = itr;                                               \
    Bool single = list->first == list->last ? TRUE : FALSE;                    \
    for (; itr != 0; prv = itr, itr = itr->next) {                             \
      if (str8_equal(itr->key, key, 0)) {                                      \
        prv->next = itr->next;                                                 \
        value = itr->value;                                                    \
        MemZeroStruct(itr);                                                    \
        if (single) {                                                          \
          MemZeroStruct(list);                                                 \
        }                                                                      \
        map->length--;                                                         \
                                                                               \
        END_PROFILING();                                                       \
        return value;                                                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    END_PROFILING();                                                           \
    return (TYPE){};                                                           \
  }

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_MAP_EX_IMPLEMENTATION

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_MAP_EX_IMPLEMENTATION */
#endif /* SEPI_MAP_EX_H */
