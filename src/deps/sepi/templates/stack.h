#ifndef SEPI_STACK__CLASS__H
#define SEPI_STACK__CLASS__H

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

typedef struct _Class_ _Class_;
typedef struct Stack_Class_Node Stack_Class_Node;
struct Stack_Class_Node {
  _Type_* _class_;
  Stack_Class_Node* previous;
};

typedef struct Stack_Class_ Stack_Class_;
struct Stack_Class_ {
  U64 length;
  Arena* arena;
  Stack_Class_Node* top;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Stack_Class_* stack__class__create(Arena* arena);
Nothing stack__class__push(Stack_Class_* stack, _Type_* _class_);
_Type_* stack__class__pop(Stack_Class_* stack);
Nothing stack__class__clean(Stack_Class_* stack);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_STACK__CLASS__IMPLEMENTATION

mount_slave_profiling_context();

Stack_Class_*
stack__class__create(Arena* arena) {
  start_profiling();

  Stack_Class_* stack =
      arena_push(arena, sizeof(Stack_Class_), alignof(Stack_Class_), TRUE);
  stack->arena = arena;

  end_profiling();
  return stack;
}

Nothing
stack__class__push(Stack_Class_* stack, _Type_* _class_) {
  start_profiling();

  Stack_Class_Node* node = arena_push(stack->arena, sizeof(Stack_Class_Node),
                                      alignof(Stack_Class_Node), FALSE);
  node->_class_ = _class_;
  node->previous = stack->top;
  stack->top = node;
  stack->length++;

  end_profiling();
}

_Type_*
stack__class__pop(Stack_Class_* stack) {
  start_profiling();

  _Type_* _class_ = 0;
  if (!stack->length) {
    goto cleanup;
  }
  Stack_Class_Node* old_top = stack->top;
  _class_ = old_top->_class_;
  stack->top = old_top->previous;
  old_top->previous = 0;
  stack->length--;

cleanup:
  end_profiling();
  return _class_;
}

Nothing
stack__class__clean(Stack_Class_* stack) {
  start_profiling();

  for (; stack->length > 0;) {
    stack__class__pop(stack);
  }
  stack->arena = 0;
  stack->length = 0;
  stack->top = 0;

  end_profiling();
}

U64
// cppcheck-suppress unusedFunction
stack__class__length(const Stack_Class_* stack) {
  return stack->length;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_STACK__CLASS__IMPLEMENTATION */
#endif /* SEPI_STACK__CLASS__H */
