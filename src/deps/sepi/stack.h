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

extern TracyCZoneCtx trcyctx;

Stack*
stack_create(Arena* a) {
  TracyCZoneN(trcyctx, "stack_create", 1);

  Stack* s = arena_push(a, sizeof(Stack), AlignOf(Stack), FALSE);
  s->arena = a;
  s->length = 0;
  s->bottom = 0;
  s->top = 0;

  TracyCZoneEnd(trcyctx);
  return s;
}

Nothing
stack_destroy(Stack* s) {
  TracyCZoneN(trcyctx, "stack_destroy", 1);

  for (; s->length > 0;) {
    stack_pop(s);
  }

  s->arena = 0;
  s->length = 0;
  s->bottom = 0;
  s->top = 0;

  TracyCZoneEnd(trcyctx);
}

Nothing
stack_push(Stack* s, RawPtr ptr) {
  TracyCZoneN(trcyctx, "stack_push", 1);

  StackNode* node =
      arena_push(s->arena, sizeof(StackNode), AlignOf(StackNode), FALSE);
  node->ptr = ptr;
  node->previous = s->top;
  s->top = node;
  s->length++;

  TracyCZoneEnd(trcyctx);
}

RawPtr
stack_pop(Stack* s) {
  TracyCZoneN(trcyctx, "stack_pop", 1);

  if (!s->length) {
    return 0;
  }

  StackNode* old_top = s->top;
  RawPtr ptr = old_top->ptr;

  s->top = old_top->previous;
  old_top->previous = 0;
  old_top->ptr = 0;

  s->length--;

  TracyCZoneEnd(trcyctx);
  return ptr;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STACK_IMPLEMENTATION */
#endif /* SEPI_STACK_H */
