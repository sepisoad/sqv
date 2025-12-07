#ifndef SEPI_LIST_H
#define SEPI_LIST_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

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

typedef enum {
  LIST_ERR_SUCCESS = 1,
  LIST_INDEX_ERROR,
  LIST_NOTE_FOUND,
  LIST_ERR__COUNT,
} ListError;

typedef struct ListNode ListNode;
struct ListNode {
  ListNode* next;
  ListNode* previous;
  RawPtr data;
};

typedef struct {
  U64 count;
  ListNode* head;
  ListNode* tail;
} List;

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

ListError list_purge(List* l);
ListError list_remove(List* l, U32 index);
ListError list_push(Arena* a, List* l, RawPtr data);
ListError list_pop(List* l, RawPtr data);
ListError list_get(List* hm, U32 index, RawPtr* data);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_LIST_IMPLEMENTATION

ListError
list_purge(List* l) {
  Dbg("list_purge() ...");

  Assert(l != 0);

  // yes we leak memory for the life time of this arena!
  // it's fine, cuz this list is not supposed to grow and
  // shrink that much often, if the need/use case emerges
  // then i think about it, i am not interested in generic
  // nonsense solutions!
  l->head = 0;
  l->tail = 0;
  l->count = 0;

  return LIST_ERR_SUCCESS;
}

ListError
list_remove(List* l, U32 index) {
  Dbg("list_remove() ...");

  Assert(l != 0);

  if (index >= l->count) {
    return LIST_INDEX_ERROR;
  }

  U32 step = 0;
  for (ListNode* iter = l->head; iter != 0; iter = iter->next) {
    if (step == index) {
      iter->previous->next = iter->next;
      iter->next->previous = iter->previous;
      l->count--;
      return LIST_ERR_SUCCESS;
    }
    step++;
  }

  return LIST_NOTE_FOUND;
}

ListError
list_push(Arena* a, List* l, RawPtr data) {
  Dbg("list_push() ...");

  Assert(a != 0);
  Assert(l != 0);
  Assert(data != 0);

  ListNode* node =
      (ListNode*)arena_push(a, sizeof(ListNode), AlignOf(ListNode), TRUE);

  node->data = data;
  node->previous = l->tail;
  l->tail->next = node;
  l->tail = node;
  l->count++;

  return LIST_ERR_SUCCESS;
}

ListError
list_pop(List* l, RawPtr data) {
  Dbg("list_pop() ...");

  Assert(l != 0);
  Assert(data != 0);

  l->tail = l->tail->previous;
  l->tail->next = 0;
  l->count--;

  return LIST_ERR_SUCCESS;
}

ListError
list_get(List* l, U32 index, RawPtr* data) {
  Dbg("list_get() ...");

  Assert(l != 0);
  Assert(data != 0);

  if (index >= l->count) {
    return LIST_INDEX_ERROR;
  }

  U32 step = 0;
  for (ListNode* iter = l->head; iter != 0; iter = iter->next) {
    if (step == index) {
      iter->previous->next = iter->next;
      iter->next->previous = iter->previous;
      *data = iter->data;
      return LIST_ERR_SUCCESS;
    }
    step++;
  }

  return LIST_NOTE_FOUND;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_LIST_IMPLEMENTATION */
#endif /* SEPI_LIST_H */
