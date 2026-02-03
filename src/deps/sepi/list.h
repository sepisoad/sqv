#ifndef SEPI_LIST_H
#define SEPI_LIST_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../tracy/tracy.h"

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

typedef struct ListNode ListNode;
struct ListNode {
  RawPtr ptr;
  ListNode* next;
  ListNode* previous;
};

typedef struct List List;
struct List {
  Arena* arena;
  ListNode* head;
  ListNode* tail;
  U64 length;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

List* list_create(Arena* arena);
Nothing list_destroy(List* list);
ListNode* list_push_tail(List* list, RawPtr ptr);
ListNode* list_push_head(List* list, RawPtr ptr);
ListNode* list_push_after(List* list, ListNode* node, RawPtr ptr);
ListNode* list_push_before(List* list, ListNode* node, RawPtr ptr);
ListNode* list_pop_tail(List* list);
ListNode* list_pop_head(List* list);
ListNode* list_pop(List* list, ListNode* node);
RawPtr list_get_at(List* list, U64 index);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_LIST_IMPLEMENTATION

SLAVE_PROFILING_CONTEXT;

List*
list_create(Arena* arena) {
  START_PROFILING(1);

  Assert(arena != 0);

  List* list = arena_push(arena, sizeof(List), AlignOf(List), FALSE);
  list->arena = arena;
  list->head = 0;
  list->tail = 0;
  list->length = 0;

  END_PROFILING();
  return list;
}

Nothing
list_destroy(List* list) {
  START_PROFILING(1);

  Assert(list != 0);

  for (; list->length > 0;) {
    list_pop_tail(list);
  }

  END_PROFILING();
}

ListNode*
list_push_tail(List* list, RawPtr ptr) {
  START_PROFILING(1);

  Assert(list != 0);
  Assert(ptr != 0);

  ListNode* node =
      arena_push(list->arena, sizeof(ListNode), AlignOf(ListNode), TRUE);
  node->ptr = ptr;
  list->length++;

  if (!list->tail) {
    list->tail = node;
    list->head = node;
  } else {
    node->previous = list->tail;
    list->tail->next = node;
    list->tail = node;
  }

  END_PROFILING();

  return node;
}

ListNode*
list_push_head(List* list, RawPtr ptr) {
  START_PROFILING(1);

  Assert(list != 0);
  Assert(ptr != 0);

  ListNode* node =
      arena_push(list->arena, sizeof(ListNode), AlignOf(ListNode), TRUE);
  node->ptr = ptr;
  list->length++;

  if (!list->head) {
    list->tail = node;
    list->head = node;
  } else {
    node->next = list->head;
    list->head->previous = node;
    list->head = node;
  }

  END_PROFILING();
  return node;
}

ListNode*
list_push_after(List* list, ListNode* node, RawPtr ptr) {
  START_PROFILING(1);

  Assert(list != 0);
  Assert(node != 0);
  Assert(ptr != 0);

  ListNode* new_node =
      arena_push(list->arena, sizeof(ListNode), AlignOf(ListNode), TRUE);
  new_node->ptr = ptr;
  if (node->next) {
    node->next->previous = new_node;
  }
  new_node->next = node->next;
  new_node->previous = node;
  node->next = new_node;

  list->length++;

  END_PROFILING();

  return new_node;
}

ListNode*
list_push_before(List* list, ListNode* node, RawPtr ptr) {
  START_PROFILING(1);

  Assert(list != 0);
  Assert(node != 0);
  Assert(ptr != 0);

  ListNode* new_node =
      arena_push(list->arena, sizeof(ListNode), AlignOf(ListNode), TRUE);
  new_node->ptr = ptr;
  new_node->next = node;
  if (node->previous) {
    new_node->previous = node->previous;
    node->previous->next = new_node;
  } else {
    list->head = new_node;
  }

  node->previous = new_node;

  list->length++;

  END_PROFILING();

  return new_node;
}

ListNode*
list_pop_tail(List* list) {
  START_PROFILING(1);

  Assert(list != 0);

  ListNode* res = 0;

  if (!list->tail) {
    goto cleanup;
  }

  list->length--;
  res = list->tail;

  if (!list->tail->previous) {
    goto cleanup;
  }

  list->tail = list->tail->previous;
  list->tail->next = 0;

cleanup:
  END_PROFILING();

  return res;
}

ListNode*
list_pop_head(List* list) {
  START_PROFILING(1);

  Assert(list != 0);

  ListNode* res = 0;

  if (!list->head) {
    goto cleanup;
  }

  list->length--;
  res = list->head;

  if (!list->head->next) {
    goto cleanup;
  }

  list->head = list->head->next;
  list->head->previous = 0;

cleanup:
  END_PROFILING();

  return res;
}

ListNode*
list_pop(List* list, ListNode* node) {
  START_PROFILING(1);

  Assert(list != 0);
  Assert(node != 0);

  ListNode* res = 0;

  if (!list->head || !node) {
    goto cleanup;
  }

  if (node == list->tail) {
    res = list_pop_tail(list);
    goto cleanup;
  }

  if (node == list->head) {
    res = list_pop_head(list);
    goto cleanup;
  }

  list->length--;
  node->previous->next = node->next;
  node->next->previous = node->previous;
  node->next = 0;
  node->previous = 0;
  res = node;

cleanup:
  END_PROFILING();

  return res;
}

RawPtr
list_get_at(List* list, U64 index) {
  START_PROFILING(1);

  Assert(list != 0);

  ListNode* res = 0;

  if (!list->head) {
    goto cleanup;
  }

  if (index >= list->length) {
    goto cleanup;
  }

  res = list->head;
  U64 pos = 0;

  for (; pos != index; res = res->next, pos++)
    ;

cleanup:
  END_PROFILING();

  return res->ptr;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_LIST_IMPLEMENTATION */
#endif /* SEPI_LIST_H */
