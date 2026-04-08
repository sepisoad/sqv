#ifndef SEPI_LIST__CLASS__H
#define SEPI_LIST__CLASS__H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#define _EXTRA_DEFINES_

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/arena.h>
#include <sepi/platform.h>

#define _EXTRA_HEADERS_

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct List_Class_Node List_Class_Node;
struct List_Class_Node {
  _Type_ data;
  List_Class_Node* previous;
  List_Class_Node* next;
};

typedef struct List_Class_ List_Class_;
struct List_Class_ {
  U64 length;
  List_Class_Node* start;
  List_Class_Node* end;
  Arena* arena;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

fn List_Class_ list__class__make(Arena* arena);
fn List_Class_Node* list__class__push_start(List_Class_* list, _Type_ data);
fn List_Class_Node* list__class__push_end(List_Class_* list, _Type_ data);
fn List_Class_Node* list__class__pop_end(List_Class_* list);
fn List_Class_Node* list__class__pop_head(List_Class_* list);
fn _Type_ list__class__get(List_Class_* list, U64 index);
fn Nothing list__class__clean(List_Class_* list);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_LIST__CLASS__IMPLEMENTATION

mount_slave_profiling_context();

fn List_Class_
list__class__make(Arena* arena) {
  start_profiling(1);

  assert(arena != 0);

  List_Class_ list = {
      .length = 0,
      .start = 0,
      .end = 0,
      .arena = arena,
  };

  end_profiling();
  return list;
}

fn List_Class_Node*
list__class__push_start(List_Class_* list, _Type_ data) {
  start_profiling(1);

  assert(list != 0);

  List_Class_Node* node = arena_push(list->arena, sizeof(List_Class_Node),
                                     alignof(List_Class_Node), TRUE);
  node->data = data;
  list->length++;

  if (!list->end) {
    list->end = node;
    list->start = node;
  } else {
    node->next = list->start;
    list->start->previous = node;
    list->start = node;
  }

  end_profiling();

  return node;
}

fn List_Class_Node*
list__class__push_end(List_Class_* list, _Type_ data) {
  start_profiling(1);

  assert(list != 0);

  List_Class_Node* node = arena_push(list->arena, sizeof(List_Class_Node),
                                     alignof(List_Class_Node), TRUE);
  node->data = data;
  list->length++;

  if (!list->start) {
    list->end = node;
    list->start = node;
  } else {
    node->previous = list->end;
    list->end->next = node;
    list->end = node;
  }

  end_profiling();
  return node;
}

fn List_Class_Node*
list__class__pop_end(List_Class_* list) {
  start_profiling(1);

  assert(list != 0);

  List_Class_Node* res = 0;

  if (!list->end) {
    goto cleanup;
  }

  list->length--;
  res = list->end;

  if (!list->end->previous) {
    list->end = list->start = 0;
    goto cleanup;
  }

  list->end = list->end->previous;
  list->end->next = 0;

cleanup:
  end_profiling();

  return res;
}

fn List_Class_Node*
list__class__pop_head(List_Class_* list) {
  start_profiling(1);

  assert(list != 0);

  List_Class_Node* res = 0;

  if (!list->start) {
    goto cleanup;
  }

  list->length--;
  res = list->start;

  if (!list->start->next) {
    list->end = list->start = 0;
    goto cleanup;
  }

  list->start = list->start->next;
  list->start->previous = 0;

cleanup:
  end_profiling();

  return res;
}

fn _Type_
list__class__get(List_Class_* list, U64 index) {
  start_profiling(1);

  assert(list != 0);

  List_Class_Node* res = 0;

  if (!list->start) {
    goto cleanup;
  }

  if (index >= list->length) {
    goto cleanup;
  }

  res = list->start;
  U64 pos = 0;

  for (; pos != index; res = res->next, pos++)
    ;

cleanup:
  end_profiling();

  return res->data;
}

fn Nothing
list__class__clean(List_Class_* list) {
  start_profiling(1);

  assert(list != 0);

  for (; list->length > 0;) {
    list__class__pop_end(list);
  }

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_LIST__CLASS__IMPLEMENTATION */
#endif /* SEPI_LIST__CLASS__H */
