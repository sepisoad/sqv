#ifndef SEPI_STACK_EX_H
#define SEPI_STACK_EX_H

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

#define DefineStack(TYPE, CLASS, METHOD)                          \
  typedef struct Stack##CLASS##Node Stack##CLASS##Node;           \
  struct Stack##CLASS##Node {                                     \
    TYPE data;                                                    \
    Stack##CLASS##Node* previous;                                 \
  };                                                              \
                                                                  \
  typedef struct Stack##CLASS Stack##CLASS;                       \
  struct Stack##CLASS {                                           \
    U64 length;                                                   \
    Arena* arena;                                                 \
    Stack##CLASS##Node* top;                                      \
  };                                                              \
                                                                  \
  Stack##CLASS stack_##METHOD##_make(Arena* arena) {              \
    START_PROFILING(1);                                           \
                                                                  \
    Stack##CLASS stack = {.arena = arena, .length = 0, .top = 0}  \
                                                                  \
    END_PROFILING();                                              \
    return stack;                                                 \
  }                                                               \
                                                                  \
  Nothing stack_##METHOD##_push(Stack##CLASS* stack, TYPE data) { \
    START_PROFILING(1);                                           \
                                                                  \
    Stack##CLASS##Node* node =                                    \
        arena_push(stack->arena, sizeof(Stack##CLASS##Node),      \
                   AlignOf(Stack##CLASS##Node), FALSE);           \
    node->data = data;                                            \
    node->previous = stack->top;                                  \
    stack->top = node;                                            \
    stack->length++;                                              \
                                                                  \
    END_PROFILING();                                              \
  }                                                               \
                                                                  \
  TYPE stack_##METHOD##_pop(Stack##CLASS* stack) {                \
    START_PROFILING(1);                                           \
                                                                  \
    TYPE data = {0};                                              \
    if (!stack->length) {                                         \
      goto cleanup;                                               \
    }                                                             \
    Stack##CLASS##Node* old_top = stack->top;                     \
    data = old_top->data;                                         \
    stack->top = old_top->previous;                               \
    old_top->previous = 0;                                        \
    stack->length--;                                              \
                                                                  \
  cleanup:                                                        \
    END_PROFILING();                                              \
    return data;                                                  \
  }                                                               \
                                                                  \
  Nothing stack_##METHOD##_clean(Stack##CLASS* stack) {           \
    START_PROFILING(1);                                           \
                                                                  \
    for (; stack->length > 0;) {                                  \
      stack_##METHOD##_pop(stack);                                \
    }                                                             \
    stack->arena = 0;                                             \
    stack->length = 0;                                            \
    stack->top = 0;                                               \
                                                                  \
    END_PROFILING();                                              \
  }                                                               \
                                                                  \
  U32 stack_##METHOD##_length(Stack##CLASS* stack) {              \
    /* NOTE: no place for profiling! */                           \
    return stack->length;                                         \
  }

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// --

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_STACK_EX_IMPLEMENTATION

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STACK_EX_IMPLEMENTATION */
#endif /* SEPI_STACK_EX_H */
