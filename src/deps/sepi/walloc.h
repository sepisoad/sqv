#ifndef SEPI_DBGALLOC_H
#define SEPI_DBGALLOC_H

#include "base.h"

#if defined(DEBUG_MODE) && defined(USE_MEM_DEBUGGER)

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ====================================================== */
/*                          API                           */
/* ====================================================== */

void* dbg_malloc(sz, cstr, i32);
void* dbg_calloc(sz, sz, cstr, i32);
void* dbg_realloc(void*, sz, cstr, i32);
void dbg_free(void*, cstr, i32);

#define wmalloc(n) dbg_malloc((n), __FILE__, __LINE__)
#define wcalloc(n, m) dbg_calloc((n), (m), __FILE__, __LINE__)
#define wrealloc(p, n) dbg_realloc((p), (n), __FILE__, __LINE__)
#define wfree(p) dbg_free((p), __FILE__, __LINE__)

#else /* release version */

#define wmalloc(n) malloc(n)
#define wcalloc(n, m) calloc(n, m)
#define wrealloc(p, n) realloc(p, n)
#define wfree(p) free(p)

#endif /* DEBUG_MODE && USE_MEM_DEBUGGER */

/* ====================================================== */
/*                      IMPLEMENTATION                    */
/* ====================================================== */

#if defined(SEPI_DBGALLOC_IMPLEMENTATION)

#if defined(DEBUG_MODE) && defined(USE_MEM_DEBUGGER)

#include <stdlib.h>
#include <stdatomic.h>

#define DBGALLOC_BUCKETS_SIZE 4096u

typedef struct dbg_node {
  void* ptr;
  sz size;
  u64 id;
  i32 line;
  cstr file;
  struct dbg_node* next;
} dbg_node;

typedef struct {
  dbg_node* buckets[DBGALLOC_BUCKETS_SIZE];
  sz current_size;
  sz peak_size;
  u64 alloc_call_count;
  u64 free_call_count;
  u64 realloc_call_count;
  bool installed;
} dbg_state;

static dbg_state g_state = {0};

/* === HELPER FUNCTIONS === */

static void dbg_report(void) {
  printf("===== xalloc summary =====\n");
  printf("current  : %zu bytes, peak: %zu bytes\n", g_state.current_size,
         g_state.peak_size);
  printf("allocs   : %llu\nfrees    : %llu\nreallocs : %llu\n",
         (unsigned long long)g_state.alloc_call_count,
         (unsigned long long)g_state.free_call_count,
         (unsigned long long)g_state.realloc_call_count);

  sz leaks = 0, leaked_bytes = 0;
  for (u32 i = 0; i < DBGALLOC_BUCKETS_SIZE; ++i) {
    for (dbg_node* n = g_state.buckets[i]; n; n = n->next) {
      ++leaks;
      leaked_bytes += n->size;
    }
  }
  if (leaks == 0) {
    printf("leaks    : NONE!\n");
  } else {
    printf("leaks    : SHIT! %zu blocks, %zu bytes\n", leaks, leaked_bytes);
    for (u32 i = 0; i < DBGALLOC_BUCKETS_SIZE; ++i) {
      for (dbg_node* node = g_state.buckets[i]; node != NULL;
           node = node->next) {
        printf("  leak #%llu: ptr=%p size=%zu @ %s:%d\n", (u64)node->id,
               node->ptr, node->size, node->file, node->line);
      }
    }
  }
  printf("==========================\n");
}

static inline i64 dbg_find_ptr_slot(void* ptr) {
  for (i32 i = 0; i < DBGALLOC_BUCKETS_SIZE; i++) {
    dbg_node* node = g_state.buckets[i];
    if (!node)
      continue;
    if (node->ptr == ptr) {
      return i;
    }
  }

  return -1;
}

static inline u32 dbg_find_free_slot() {
  for (i32 i = 0; i < DBGALLOC_BUCKETS_SIZE; i++) {
    dbg_node* node = g_state.buckets[i];
    if (!node)
      return i;

    NOTNULL(node->ptr);
  }

  MUSTDIE("xalloc free bucket slots are axhausted!");
}

static void dbg_atexit(void) {
  dbg_report();
}

static void dbg_install(void) {
  if (!g_state.installed) {
    memset(&g_state, 0, sizeof(g_state));
    g_state.installed = true;
    atexit(dbg_atexit);
  }
}

static void dbg_add_node(void* ptr, sz size, cstr file, i32 line) {
  NOTNULL(ptr);
  ISVALID(dbg_find_ptr_slot(ptr) < 0);

  u32 slot = dbg_find_free_slot();
  dbg_node* node = (dbg_node*)malloc(sizeof(dbg_node));
  NOTNULL(node);

  node->ptr = ptr;
  node->size = size;
  node->file = file;
  node->line = line;

  g_state.buckets[slot] = node;
  g_state.current_size += size;
  g_state.alloc_call_count++;

  if (g_state.current_size > g_state.peak_size)
    g_state.peak_size = g_state.current_size;
}

static void dbg_remove_node(void* ptr, cstr file, i32 line) {
  NOTNULL(ptr);

  i64 slot = dbg_find_ptr_slot(ptr);
  ISVALID(slot >= 0);

  dbg_node* node = g_state.buckets[slot];
  NOTNULL(node);

  free(node->ptr);

  if (g_state.current_size >= node->size)
    g_state.current_size -= node->size;
  else
    g_state.current_size = 0;

  g_state.free_call_count++;
  g_state.buckets[slot] = NULL;
}

static void dbg_resize(void* old_ptr,
                       void* new_ptr,
                       sz new_size,
                       cstr file,
                       i32 line) {
  if (old_ptr == NULL && new_ptr) {
    dbg_add_node(new_ptr, new_size, file, line);
    return;
  }

  if (new_ptr == NULL) {
    dbg_remove_node(old_ptr, file, line);
    return;
  }

  i64 slot = dbg_find_ptr_slot(old_ptr);
  ISVALID(slot >= 0);

  dbg_node* node = g_state.buckets[slot];
  NOTNULL(node);

  if (new_size >= node->size) {
    sz delta = new_size - node->size;

    g_state.current_size += delta;
    if (g_state.current_size > g_state.peak_size)
      g_state.peak_size = g_state.current_size;
  } else {
    sz delta = node->size - new_size;

    if (g_state.current_size >= delta)
      g_state.current_size -= delta;
    else
      g_state.current_size = 0;
  }

  node->ptr = new_ptr;
  node->size = new_size;
  node->file = file;
  node->line = line;
}

/* === APIS === */

void* dbg_malloc(sz size, cstr file, i32 line) {
  dbg_install();
  void* ptr = malloc(size);
  dbg_add_node(ptr, size, file, line);
  return ptr;
}

void* dbg_calloc(sz unit_size, sz count, cstr file, i32 line) {
  dbg_install();
  void* ptr = calloc(unit_size, count);
  dbg_add_node(ptr, unit_size * count, file, line);
  return ptr;
}

void* dbg_realloc(void* old_ptr, sz size, cstr file, i32 line) {
  dbg_install();
  void* new_ptr = realloc(old_ptr, size);
  dbg_resize(old_ptr, new_ptr, size, file, line);
  return new_ptr;
}

void dbg_free(void* ptr, cstr file, i32 line) {
  dbg_install();
  dbg_remove_node(ptr, file, line);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_DBGALLOC_IMPLEMENTATION */
#endif /* DEBUG_MODE && USE_MEM_DEBUGGER */
#endif /* SEPI_DBGALLOC_H */
