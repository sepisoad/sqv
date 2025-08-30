#ifndef SEPI_ALLOC_HEADER_
#define SEPI_ALLOC_HEADER_

#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "macros.h"

typedef struct alloc_node {
  void* ptr;
  sz size;
#ifdef DEBUG
  const char* file;
  int line;
#endif  // DEBUG
  struct alloc_node* next;
} alloc_node;

typedef struct {
  alloc_node* head;
  sz total_size;
#ifdef DEBUG
  int alloc_count;
#endif  // DEBUG
} sepi_alloc;

/* ****************** API ****************** */
void alloc_init(sepi_alloc* sa);
void alloc_free_all(sepi_alloc* sa);
#ifdef DEBUG
void alloc_print_stats(sepi_alloc* sa);
void* _alloc(sepi_alloc* sa, sz siz, sz cnt, const char* file, int line);
void* _realloc(sepi_alloc* sa, sz siz, const char* file, int line);
#define alloc_do(sa, siz, cnt) _alloc(sa, siz, cnt, __FILE__, __LINE__)
#define realloc_do(sa, siz) _alloc(sa, siz, cnt, __FILE__, __LINE__)
// #define sepi_realloc(sa, siz, cnt) _realloc(sa, siz, cnt, __FILE__, __LINE__)
#else
void* alloc_do(sepi_alloc* sa, sz siz, sz cnt);
#endif  // DEBUG
/* ****************** API ****************** */

#ifdef SEPI_ALLOC_IMPLEMENTATION

// .--------------------------------------------------------------------------.
// | _                 _                           _        _   _             |
// |(_)               | |                         | |      | | (_)            |
// | _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __  |
// || | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \ |
// || | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | ||
// ||_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_||
// |            | |                                                           |
// |            |_|                                                           |
// '--------------------------------------------------------------------------'

void alloc_init(sepi_alloc* sa) {
  sa->head = NULL;
  sa->total_size = 0;
#ifdef DEBUG
  sa->alloc_count = 0;
#endif  // DEBUG
}

#ifdef DEBUG
void* _alloc(sepi_alloc* sa, sz siz, sz cnt, const char* file, int line) {
#else
void* alloc_do(sepi_alloc* sa, sz siz, sz cnt) {
#endif  // DEBUG

  void* ptr = calloc(cnt, siz);
  notnull(ptr);

  alloc_node* node = calloc(1, sizeof(alloc_node));
  notnull(node);

  node->ptr = ptr;
  node->size = siz * cnt;  // Track total size

#ifdef DEBUG
  node->file = file;
  node->line = line;
  DBG("Allocated %zu bytes at %p (%s:%d)", node->size, ptr, file, line);
  sa->alloc_count++;
#endif  // DEBUG

  node->next = sa->head;
  sa->head = node;
  sa->total_size += node->size;
  return ptr;
}

void alloc_free_all(sepi_alloc* sa) {
  alloc_node* current = sa->head;
  while (current) {
    alloc_node* next = current->next;

#ifdef DEBUG
    DBG("Freeing %zu bytes at %p (%s:%d)", current->size, current->ptr,
        current->file, current->line);
    sa->alloc_count--;
#endif
    free(current->ptr);
    free(current);
    current = next;
  }
  sa->head = NULL;
  sa->total_size = 0;
#ifdef DEBUG
  if (sa->alloc_count != 0) {
    DBG("Leak detected: alloc_count = %d", sa->alloc_count);
  }
#endif  // DEBUG
}

#ifdef DEBUG
void alloc_print_stats(sepi_alloc* sa) {
  DBG("Allocator stats: total_size=%zu, alloc_count=%d", sa->total_size,
      sa->alloc_count);
  alloc_node* current = sa->head;
  while (current) {
    DBG("  Allocation: %zu bytes at %p (%s:%d)", current->size, current->ptr,
        current->file, current->line);
    current = current->next;
  }
}
#endif  // DEBUG

#endif  // SEPI_ALLOC_IMPLEMENTATION
#endif  // SEPI_ALLOC_HEADER_
