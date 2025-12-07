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
  LIST_ERR__COUNT,
} ListError;

typedef struct ListNode ListNode;
struct ListNode {
  ListNode* next;
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

ListError list_init(Arena* a, List* l);
ListError list_purge(List* l);
ListError list_remove(List* l, U32 index);
ListError list_push(Arena* a, List* l, RawPtr data);
ListError list_pop(List* l, RawPtr data);
ListError list_get(List* hm, U32 index, RawPtr data);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_LIST_IMPLEMENTATION

ListError list_init(Arena* a, List* l) {
  Dbg("list_init() ...");

  Assert(a != 0);
  Assert(l != 0);

  ListError err = LIST_ERR_SUCCESS;

  return err;
}

ListError list_purge(List* l) {
  Dbg("list_purge() ...");

  Assert(l != 0);

  ListError err = LIST_ERR_SUCCESS;

  return err;
}

ListError list_remove(List* l, U32 index) {
  Dbg("list_remove() ...");

  Assert(l != 0);

  ListError err = LIST_ERR_SUCCESS;

  return err;
}

ListError list_push(Arena* a, List* l, RawPtr data) {
  Dbg("list_push() ...");

  Assert(a != 0);
  Assert(l != 0);
  Assert(data != 0);

  ListError err = LIST_ERR_SUCCESS;

  return err;
}

ListError list_pop(List* l, RawPtr data) {
  Dbg("list_pop() ...");

  Assert(l != 0);
  Assert(data != 0);

  ListError err = LIST_ERR_SUCCESS;

  return err;
}

ListError list_get(List* l, U32 index, RawPtr data) {
  Dbg("list_get() ...");

  Assert(l != 0);
  Assert(data != 0);

  ListError err = LIST_ERR_SUCCESS;

  return err;
}


/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_LIST_IMPLEMENTATION */
#endif /* SEPI_LIST_H */
