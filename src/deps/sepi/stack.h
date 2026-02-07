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

#define StackOf(T) Stack*

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Stack* stack_create(Arena* arena);
Nothing stack_destroy(Stack* stack);
Nothing stack_push(Stack* stack, RawPtr ptr);
RawPtr stack_pop(Stack* stack);
#define stack_length(s) (s)->length

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_STACK_IMPLEMENTATION

SLAVE_PROFILING_CONTEXT;

Stack*
stack_create(Arena* arena) {
  START_PROFILING(1);

  Stack* stack = arena_push(arena, sizeof(Stack), AlignOf(Stack), FALSE);
  stack->arena = arena;
  stack->length = 0;
  stack->bottom = 0;
  stack->top = 0;

  END_PROFILING();
  return stack;
}

Nothing
stack_destroy(Stack* stack) {
  START_PROFILING(1);

  for (; stack->length > 0;) {
    stack_pop(stack);
  }

  stack->arena = 0;
  stack->length = 0;
  stack->bottom = 0;
  stack->top = 0;

  END_PROFILING();
}

Nothing
stack_push(Stack* stack, RawPtr ptr) {
  START_PROFILING(1);

  StackNode* node =
      arena_push(stack->arena, sizeof(StackNode), AlignOf(StackNode), FALSE);
  node->ptr = ptr;
  node->previous = stack->top;
  stack->top = node;
  stack->length++;

  END_PROFILING();
}

RawPtr
stack_pop(Stack* stack) {
  START_PROFILING(1);

  RawPtr ptr = 0;

  if (!stack->length) {
    goto cleanup;
  }

  StackNode* old_top = stack->top;
  ptr = old_top->ptr;

  stack->top = old_top->previous;
  old_top->previous = 0;
  old_top->ptr = 0;

  stack->length--;

cleanup:
  END_PROFILING();
  return ptr;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STACK_IMPLEMENTATION */
#endif /* SEPI_STACK_H */
