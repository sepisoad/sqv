#ifndef SEPI_LIST_EX_H
#define SEPI_LIST_EX_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../tracy/tracy.h"

#include "base.h"
#include "arena.h"
#include "platform.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

#define DefineList(TYPE, CLASS, METHOD)                                       \
  typedef struct List##CLASS##Node List##CLASS##Node;                         \
  struct List##CLASS##Node {                                                  \
    TYPE data;                                                                \
    List##CLASS##Node* previous;                                              \
    List##CLASS##Node* next;                                                  \
  };                                                                          \
                                                                              \
  typedef struct List##CLASS List##CLASS;                                     \
  struct List##CLASS {                                                        \
    U64 length;                                                               \
    List##CLASS##Node* start;                                                 \
    List##CLASS##Node* end;                                                   \
    Arena* arena;                                                             \
  };                                                                          \
                                                                              \
  List##CLASS list_##METHOD##_make(Arena* arena) {                            \
    START_PROFILING(1);                                                       \
                                                                              \
    Assert(arena != 0);                                                       \
                                                                              \
    List##CLASS list = {                                                      \
        .length = 0,                                                          \
        .start = 0,                                                           \
        .end = 0,                                                             \
        .arena = arena,                                                       \
    };                                                                        \
                                                                              \
    END_PROFILING();                                                          \
    return list;                                                              \
  }                                                                           \
                                                                              \
  List##CLASS##Node* list_##METHOD##_push_start(List##CLASS* list,            \
                                                TYPE data) {                  \
    START_PROFILING(1);                                                       \
                                                                              \
    Assert(list != 0);                                                        \
                                                                              \
    List##CLASS##Node* node =                                                 \
        arena_push(list->arena, sizeof(List##CLASS##Node),                    \
                   AlignOf(List##CLASS##Node), TRUE);                         \
    node->data = data;                                                        \
    list->length++;                                                           \
                                                                              \
    if (!list->end) {                                                         \
      list->end = node;                                                       \
      list->start = node;                                                     \
    } else {                                                                  \
      node->next = list->start;                                               \
      list->start->previous = node;                                           \
      list->start = node;                                                     \
    }                                                                         \
                                                                              \
    END_PROFILING();                                                          \
                                                                              \
    return node;                                                              \
  }                                                                           \
                                                                              \
  List##CLASS##Node* list_##METHOD##_push_end(List##CLASS* list, TYPE data) { \
    START_PROFILING(1);                                                       \
                                                                              \
    Assert(list != 0);                                                        \
                                                                              \
    List##CLASS##Node* node =                                                 \
        arena_push(list->arena, sizeof(List##CLASS##Node),                    \
                   AlignOf(List##CLASS##Node), TRUE);                         \
    node->data = data;                                                        \
    list->length++;                                                           \
                                                                              \
    if (!list->start) {                                                       \
      list->end = node;                                                       \
      list->start = node;                                                     \
    } else {                                                                  \
      node->previous = list->end;                                             \
      list->end->next = node;                                                 \
      list->end = node;                                                       \
    }                                                                         \
                                                                              \
    END_PROFILING();                                                          \
    return node;                                                              \
  }                                                                           \
                                                                              \
  List##CLASS##Node* list_##METHOD##_pop_end(List##CLASS* list) {             \
    START_PROFILING(1);                                                       \
                                                                              \
    Assert(list != 0);                                                        \
                                                                              \
    List##CLASS##Node* res = 0;                                               \
                                                                              \
    if (!list->end) {                                                         \
      goto cleanup;                                                           \
    }                                                                         \
                                                                              \
    list->length--;                                                           \
    res = list->end;                                                          \
                                                                              \
    if (!list->end->previous) {                                               \
      list->end = list->start = 0;                                            \
      goto cleanup;                                                           \
    }                                                                         \
                                                                              \
    list->end = list->end->previous;                                          \
    list->end->next = 0;                                                      \
                                                                              \
  cleanup:                                                                    \
    END_PROFILING();                                                          \
                                                                              \
    return res;                                                               \
  }                                                                           \
                                                                              \
  List##CLASS##Node* list_##METHOD##_pop_head(List##CLASS* list) {            \
    START_PROFILING(1);                                                       \
                                                                              \
    Assert(list != 0);                                                        \
                                                                              \
    List##CLASS##Node* res = 0;                                               \
                                                                              \
    if (!list->start) {                                                       \
      goto cleanup;                                                           \
    }                                                                         \
                                                                              \
    list->length--;                                                           \
    res = list->start;                                                        \
                                                                              \
    if (!list->start->next) {                                                 \
      list->end = list->start = 0;                                            \
      goto cleanup;                                                           \
    }                                                                         \
                                                                              \
    list->start = list->start->next;                                          \
    list->start->previous = 0;                                                \
                                                                              \
  cleanup:                                                                    \
    END_PROFILING();                                                          \
                                                                              \
    return res;                                                               \
  }                                                                           \
                                                                              \
  TYPE list_##METHOD##_get(List##CLASS* list, U64 index) {                    \
    START_PROFILING(1);                                                       \
                                                                              \
    Assert(list != 0);                                                        \
                                                                              \
    List##CLASS##Node* res = 0;                                               \
                                                                              \
    if (!list->start) {                                                       \
      goto cleanup;                                                           \
    }                                                                         \
                                                                              \
    if (index >= list->length) {                                              \
      goto cleanup;                                                           \
    }                                                                         \
                                                                              \
    res = list->start;                                                        \
    U64 pos = 0;                                                              \
                                                                              \
    for (; pos != index; res = res->next, pos++)                              \
      ;                                                                       \
                                                                              \
  cleanup:                                                                    \
    END_PROFILING();                                                          \
                                                                              \
    return res->data;                                                         \
  }                                                                           \
                                                                              \
  Nothing list_##METHOD##_clean(List##CLASS* list) {                          \
    START_PROFILING(1);                                                       \
                                                                              \
    Assert(list != 0);                                                        \
                                                                              \
    for (; list->length > 0;) {                                               \
      list_##METHOD##_pop_end(list);                                          \
    }                                                                         \
                                                                              \
    END_PROFILING();                                                          \
  }

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// --

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_LIST_EX_IMPLEMENTATION

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_LIST_EX_IMPLEMENTATION */
#endif /* SEPI_LIST_EX_H */
