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

List* list_create(Arena* a);
Nothing list_destroy(List* l);
ListNode* list_push_tail(List* l, RawPtr ptr);
ListNode* list_push_head(List* l, RawPtr ptr);
ListNode* list_push_after(List* l, ListNode* n, RawPtr ptr);
ListNode* list_push_before(List* l, ListNode* n, RawPtr ptr);
ListNode* list_pop_tail(List* l);
ListNode* list_pop_head(List* l);
ListNode* list_pop(List* l, ListNode* n);
RawPtr list_get_at(List* l, U64 index);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_LIST_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

List* list_create(Arena* a) {
  TracyCZoneN(tracyctx, "list_create", 1);

  List* l = arena_push(a, sizeof(List), AlignOf(List), FALSE);
  l->arena = a;
  l->head = 0;
  l->tail = 0;
  l->length = 0;

  TracyCZoneEnd(tracyctx);
  return l;
}

Nothing list_destroy(List* l) {
  TracyCZoneN(tracyctx, "list_destroy", 1);

  for(; l->length > 0; ) {
    list_pop_tail(l);
  }

  TracyCZoneEnd(tracyctx);
}

ListNode* list_push_tail(List* l, RawPtr ptr) {
  TracyCZoneN(tracyctx, "list_push_tail", 1);

  ListNode* n = arena_push(l->arena, sizeof(ListNode), AlignOf(ListNode), TRUE);
  n->ptr = ptr;
  l->length++;

  if(!l->tail) {
    l->tail = n;
    l->head = n;
  } else {
    n->previous = l->tail;
    l->tail->next = n;
    l->tail = n;
  }

  TracyCZoneEnd(tracyctx);

  return n;
}

ListNode* list_push_head(List* l, RawPtr ptr) {
  TracyCZoneN(tracyctx, "list_push_head", 1);

  ListNode* n = arena_push(l->arena, sizeof(ListNode), AlignOf(ListNode), TRUE);
  n->ptr = ptr;
  l->length++;

  if(!l->head) {
    l->tail = n;
    l->head = n;
  } else {
    n->next = l->head;
    l->head->previous = n;
    l->head = n;
  }

  TracyCZoneEnd(tracyctx);
  return n;
}

ListNode* list_push_after(List* l, ListNode* n, RawPtr ptr) {
  TracyCZoneN(tracyctx, "list_push_after", 1);

  ListNode* nn = arena_push(l->arena, sizeof(ListNode), AlignOf(ListNode), TRUE);
  nn->ptr = ptr;
  if (n->next) {
    n->next->previous = nn;
  }
  nn->next = n->next;
  nn->previous = n;
  n->next = nn;

  l->length++;

  TracyCZoneEnd(tracyctx);

  return nn;
}

ListNode* list_push_before(List* l, ListNode* n, RawPtr ptr) {
  TracyCZoneN(tracyctx, "list_push_before", 1);

  ListNode* nn = arena_push(l->arena, sizeof(ListNode), AlignOf(ListNode), TRUE);
  nn->ptr = ptr;
  nn->next = n;
  if (n->previous) {
    nn->previous = n->previous;
    n->previous->next = nn;
  } else {
    l->head = nn;
  }

  n->previous = nn;

  l->length++;

  TracyCZoneEnd(tracyctx);

  return nn;
}

ListNode* list_pop_tail(List* l) {
  TracyCZoneN(tracyctx, "list_pop_tail", 1);

  ListNode* res = 0;

  if(!l->tail) {
    goto cleanup;
  }

  l->length--;
  res = l->tail;

  if(!l->tail->previous) {
    goto cleanup;
  }

  l->tail = l->tail->previous;
  l->tail->next = 0;


cleanup:
  TracyCZoneEnd(tracyctx);

  return res;
}

ListNode* list_pop_head(List* l) {
  TracyCZoneN(tracyctx, "list_pop_head", 1);

  ListNode* res = 0;

  if(!l->head) {
    goto cleanup;
  }

  l->length--;
  res = l->head;

  if(!l->head->next) {
    goto cleanup;
  }

  l->head = l->head->next;
  l->head->previous = 0;


cleanup:
  TracyCZoneEnd(tracyctx);

  return res;
}

ListNode* list_pop(List* l, ListNode* n) {
  TracyCZoneN(tracyctx, "list_pop", 1);

  ListNode* res = 0;

  if (!l->head || !n) {
    goto cleanup;
  }

  if (n == l->tail) {
    res = list_pop_tail(l);
    goto cleanup;
  }

  if (n == l->head) {
    res = list_pop_head(l);
    goto cleanup;
  }

  l->length--;
  n->previous->next = n->next;
  n->next->previous = n->previous;
  n->next = 0;
  n->previous = 0;

cleanup:
  TracyCZoneEnd(tracyctx);

  return res;
}

RawPtr list_get_at(List* l, U64 index) {
  TracyCZoneN(tracyctx, "list_get_at", 1);

  ListNode* res = 0;

  if (!l->head) {
    goto cleanup;
  }

  if (index >= l->length) {
    goto cleanup;
  }

  res = l->head;
  U64 pos = 0;

  for( ; pos != index ; res = res->next, pos++);

cleanup:
  TracyCZoneEnd(tracyctx);

  return res->ptr;
}



/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_LIST_IMPLEMENTATION */
#endif /* SEPI_LIST_H */
