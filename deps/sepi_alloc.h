#ifndef SEPI_ALLOC_HEADER_
#define SEPI_ALLOC_HEADER_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sepi_types.h"

/* ****************** API ****************** */
void* debug_malloc(sz, cstr, i32);
void* debug_calloc(sz, sz, cstr, i32);
void* debug_realloc(void*, sz, cstr, i32);
void debug_free(void*, cstr, i32);
void sepi_alloc_report(void);
/* ****************** API ****************** */

#ifdef DEBUG
#ifdef SEPI_ALLOC_IMPLEMENTATION

//  _                 _                           _        _   _
// (_)               | |                         | |      | | (_)
//  _ _ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_ _  ___  _ __
// | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
// | | | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | |
// |_|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
//             | |
//             |_|

typedef struct alloc_info {
  void* ptr;
  sz size;
  cstr file;
  i32 line;
  struct alloc_info* next;
} alloc_info;

static alloc_info* alloc_list = NULL;
static sz total_allocated = 0;
static sz peak_allocated = 0;

void* debug_malloc(sz size, cstr file, i32 line) {
  void* ptr = malloc(size);
  if (ptr) {
    alloc_info* info = (alloc_info*)malloc(sizeof(alloc_info));
    info->ptr = ptr;
    info->size = size;
    info->file = file;
    info->line = line;
    info->next = alloc_list;
    alloc_list = info;
    total_allocated += size;
    if (total_allocated > peak_allocated)
      peak_allocated = total_allocated;
  }
  return ptr;
}

void* debug_calloc(sz num, sz size, cstr file, i32 line) {
  void* ptr = calloc(num, size);
  if (ptr) {
    alloc_info* info = (alloc_info*)malloc(sizeof(alloc_info));
    info->ptr = ptr;
    info->size = num * size;
    info->file = file;
    info->line = line;
    info->next = alloc_list;
    alloc_list = info;
    total_allocated += num * size;
    if (total_allocated > peak_allocated)
      peak_allocated = total_allocated;
  }
  return ptr;
}

void* debug_realloc(void* ptr, sz size, cstr file, i32 line) {
  if (!ptr)
    return debug_malloc(size, file, line);
  alloc_info *prev = NULL, *curr = alloc_list;
  while (curr) {
    if (curr->ptr == ptr) {
      void* new_ptr = realloc(ptr, size);
      if (new_ptr) {
        total_allocated -= curr->size;
        total_allocated += size;
        if (total_allocated > peak_allocated)
          peak_allocated = total_allocated;
        curr->ptr = new_ptr;
        curr->size = size;
        curr->file = file;
        curr->line = line;
      }
      return new_ptr;
    }
    prev = curr;
    curr = curr->next;
  }
  printf("ERROR: Reallocating untracked poi32er %p at %s:%d\n", ptr, file,
         line);
  return realloc(ptr, size);  // Fallback to standard realloc
}

void debug_free(void* ptr, cstr file, i32 line) {
  if (!ptr)
    return;
  alloc_info *prev = NULL, *curr = alloc_list;
  while (curr) {
    if (curr->ptr == ptr) {
      total_allocated -= curr->size;
      if (prev)
        prev->next = curr->next;
      else
        alloc_list = curr->next;
      free(curr);
      free(ptr);
      return;
    }
    prev = curr;
    curr = curr->next;
  }
  printf("ERROR: Freeing untracked poi32er %p at %s:%d\n", ptr, file, line);
}

void sepi_alloc_report(void) {
  printf("Memory Report:\n");
  printf("  Total Allocated: %zu bytes\n", total_allocated);
  printf("  Peak Allocated: %zu bytes\n", peak_allocated);
  printf("  Active Allocations: %s\n", alloc_list ? "" : "None");
  alloc_info* curr = alloc_list;
  while (curr) {
    printf("  %zu bytes at %p (%s:%d)\n", curr->size, curr->ptr, curr->file,
           curr->line);
    curr = curr->next;
  }
}

#endif  // DEBUG
#endif  // SEPI_ALLOC_IMPLEMENTATION

#define malloc(s) debug_malloc(s, __FILE__, __LINE__)
#define calloc(n, s) debug_calloc(n, s, __FILE__, __LINE__)
#define realloc(p, s) debug_realloc(p, s, __FILE__, __LINE__)
#define free(p) debug_free(p, __FILE__, __LINE__)

#endif  // SEPI_ALLOC_HEADER_
