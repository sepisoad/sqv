#ifndef SEPI_STACK_H
#define SEPI_STACK_H

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

#define STACK_DEFAULT_RESERVE_SIZE MB(64)
#define STACK_DEFAULT_COMMIT_SIZE MB(64)

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct StackNode StackNode;
struct StackNode {
  RawPtr ptr;
  StackNode* previous;
};

typedef struct Stack Stack;
struct Stack {
  U64 length;
  Arena* arena;
  StackNode* top;
  StackNode* bottom;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Stack* stack_create(Arena* a);
Nothing stack_destroy(Stack* s);
Nothing stack_push(Stack* s, RawPtr ptr);
RawPtr stack_pop(Stack* s);

#define StackOf(T) Stack*

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_STACK_IMPLEMENTATION

SLAVE_PROFILING_CONTEXT;

Stack*
stack_create(Arena* a) {
  START_PROFILING(1);

  Stack* s = arena_push(a, sizeof(Stack), AlignOf(Stack), FALSE);
  s->arena = a;
  s->length = 0;
  s->bottom = 0;
  s->top = 0;

  END_PROFILING();
  return s;
}

Nothing
stack_destroy(Stack* s) {
  START_PROFILING(1);

  for (; s->length > 0;) {
    stack_pop(s);
  }

  s->arena = 0;
  s->length = 0;
  s->bottom = 0;
  s->top = 0;

  END_PROFILING();
}

Nothing
stack_push(Stack* s, RawPtr ptr) {
  START_PROFILING(1);

  StackNode* node =
      arena_push(s->arena, sizeof(StackNode), AlignOf(StackNode), FALSE);
  node->ptr = ptr;
  node->previous = s->top;
  s->top = node;
  s->length++;

  END_PROFILING();
}

RawPtr
stack_pop(Stack* s) {
  START_PROFILING(1);

  if (!s->length) {
    return 0;
  }

  StackNode* old_top = s->top;
  RawPtr ptr = old_top->ptr;

  s->top = old_top->previous;
  old_top->previous = 0;
  old_top->ptr = 0;

  s->length--;

  END_PROFILING();
  return ptr;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STACK_IMPLEMENTATION */
#endif /* SEPI_STACK_H */
