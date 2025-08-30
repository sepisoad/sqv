#ifndef PAK_TREE_HEADER_
#define PAK_TREE_HEADER_

#include "../deps/stb/stb_ds.h"
#include "../deps/sepi/types.h"
#include "../deps/sepi/arena.h"
#include "../deps/sepi/alloc.h"
#include "pak.h"

typedef struct pak_tree_node {
  char* name;
  int depth;
  int subtree_size;
  bool is_dir;
} pak_tree_node;

typedef struct {
  pak_tree_node* nodes;
  int node_count;
  char* sorted;  // Sorted array of entry names
  arena mem;
} pak_tree;

/* ****************** API ****************** */
void pak_tree_make(pak_tree* tree, pak* pak);
/* ****************** API ****************** */

#ifdef PAK_TREE_IMPLEMENTATION

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

#include <string.h>
#include <stdlib.h>

#define MAX_PAK_ITEM_PATH_DEPTH 128

static int pak_tree_compare_entries(const void* a, const void* b) {
  return strcmp((const char*)a, (const char*)b);
}

static void pak_tree_estimate_memory(pak_tree* tree,
                                     pak* pak,
                                     sz* out_node_count,
                                     sz* out_string_len) {
  *out_node_count = 0;
  *out_string_len = 0;
  if (pak->entries_count == 0)
    return;

  sepi_alloc temp_alloc;
  alloc_init(&temp_alloc);
  char* sorted = alloc(&temp_alloc, PAK_ENTRY_NAME_LEN, pak->entries_count);
  notnull(sorted);
  for (sz i = 0; i < pak->entries_count; i++) {
    char* dst = sorted + i * PAK_ENTRY_NAME_LEN;
    strcpy(dst, pak->entries[i].name);
  }
  qsort(sorted, pak->entries_count, PAK_ENTRY_NAME_LEN,
        pak_tree_compare_entries);

  char last_path[PAK_ENTRY_NAME_LEN] = {0};
  int stack[MAX_PAK_ITEM_PATH_DEPTH] = {0};
  int stack_top = 0;

  for (sz i = 0; i < pak->entries_count; i++) {
    char* path = sorted + i * PAK_ENTRY_NAME_LEN;
    sz common_prefix_len = 0;
    int last_slash_pos = -1;
    sz path_len = strlen(path);
    sz last_path_len = strlen(last_path);
    sz min_len = path_len < last_path_len ? path_len : last_path_len;

    for (common_prefix_len = 0; common_prefix_len < min_len;
         common_prefix_len++) {
      if (path[common_prefix_len] != last_path[common_prefix_len])
        break;
      if (path[common_prefix_len] == '/')
        last_slash_pos = common_prefix_len;
    }
    if (common_prefix_len > 0 &&
        (common_prefix_len == min_len || path[common_prefix_len - 1] != '/')) {
      common_prefix_len = (last_slash_pos >= 0) ? last_slash_pos + 1 : 0;
    }

    int common_levels = 0;
    for (sz k = 0; k < common_prefix_len; k++) {
      if (path[k] == '/')
        common_levels++;
    }

    while (stack_top > common_levels)
      stack_top--;

    char* remaining = path + common_prefix_len;
    char* seg_start = remaining;
    char* seg_end;
    int current_depth = common_levels;

    while ((seg_end = strchr(seg_start, '/'))) {
      sz seg_len = seg_end - seg_start;
      if (seg_len > 0 && current_depth >= stack_top) {  // Unique dir
        (*out_node_count)++;
        (*out_string_len) += seg_len + 1;  // Include null
        stack[stack_top++] = current_depth;
      }
      current_depth++;
      seg_start = seg_end + 1;
    }

    sz file_len = strlen(seg_start);
    if (file_len > 0) {
      (*out_node_count)++;
      (*out_string_len) += file_len + 1;
    }

    strcpy(last_path, path);
  }

  alloc_free_all(&temp_alloc);

  // Setup arena
  arena* mem = &tree->mem;
  arena_begin_estimate(mem);
  arena_add_estimate(mem, *out_node_count * sizeof(pak_tree_node), ALIGNMENT);
  arena_add_estimate(mem, *out_string_len, ALIGNMENT);
  arena_add_estimate(mem, pak->entries_count * PAK_ENTRY_NAME_LEN, ALIGNMENT);
  arena_end_estimate(mem);
}

void pak_tree_make(pak_tree* tree, pak* pak) {
  if (pak->entries_count == 0) {
    tree->nodes = NULL;
    tree->node_count = 0;
    tree->sorted = NULL;
    return;
  }

  sz node_count = 0;
  sz string_len = 0;
  pak_tree_estimate_memory(tree, pak, &node_count, &string_len);

  // Allocate sorted array
  arena* mem = &tree->mem;
  tree->sorted =
      arena_alloc(mem, pak->entries_count * PAK_ENTRY_NAME_LEN, ALIGNMENT);
  notnull(tree->sorted);

  for (sz i = 0; i < pak->entries_count; i++) {
    char* dst = tree->sorted + i * PAK_ENTRY_NAME_LEN;
    strcpy(dst, pak->entries[i].name);
  }

  qsort(tree->sorted, pak->entries_count, PAK_ENTRY_NAME_LEN,
        pak_tree_compare_entries);

  // Allocate nodes
  tree->nodes = arena_alloc(mem, node_count * sizeof(pak_tree_node), ALIGNMENT);
  notnull(tree->nodes);

  tree->node_count = node_count;

  // Build tree
  int current_index = 0;
  char last_path[PAK_ENTRY_NAME_LEN] = {0};
  int stack[MAX_PAK_ITEM_PATH_DEPTH] = {0};
  int stack_top = 0;
  int current_depth = 0;

  for (sz i = 0; i < pak->entries_count; i++) {
    char* path = tree->sorted + i * PAK_ENTRY_NAME_LEN;
    sz common_prefix_len = 0;
    int last_slash_pos = -1;
    sz path_len = strlen(path);
    sz last_path_len = strlen(last_path);
    sz min_len = path_len < last_path_len ? path_len : last_path_len;

    for (common_prefix_len = 0; common_prefix_len < min_len;
         common_prefix_len++) {
      if (path[common_prefix_len] != last_path[common_prefix_len])
        break;
      if (path[common_prefix_len] == '/')
        last_slash_pos = common_prefix_len;
    }
    if (common_prefix_len > 0 &&
        (common_prefix_len == min_len || path[common_prefix_len - 1] != '/')) {
      common_prefix_len = (last_slash_pos >= 0) ? last_slash_pos + 1 : 0;
    }

    int common_levels = 0;
    for (sz k = 0; k < common_prefix_len; k++) {
      if (path[k] == '/')
        common_levels++;
    }

    while (stack_top > common_levels) {
      stack_top--;
      int dir_idx = stack[stack_top];
      tree->nodes[dir_idx].subtree_size = current_index - dir_idx - 1;
    }

    char* remaining = path + common_prefix_len;
    char* seg_start = remaining;
    char* seg_end;
    current_depth = common_levels;

    while ((seg_end = strchr(seg_start, '/'))) {
      sz seg_len = seg_end - seg_start;
      if (seg_len > 0 && current_depth >= stack_top) {  // Unique dir
        pak_tree_node* node = &tree->nodes[current_index];
        node->name = arena_alloc(mem, seg_len + 1, ALIGNMENT);
        notnull(node->name);

        strncpy(node->name, seg_start, seg_len);
        node->name[seg_len] = '\0';
        node->depth = current_depth;
        node->subtree_size = 0;  // Set later
        node->is_dir = true;
        stack[stack_top++] = current_index;
        current_index++;
        current_depth++;
      }
      seg_start = seg_end + 1;
    }

    sz file_len = strlen(seg_start);
    if (file_len > 0) {
      pak_tree_node* node = &tree->nodes[current_index];
      node->name = arena_alloc(mem, file_len + 1, ALIGNMENT);
      if ((void*)node->name == NULL) {
        DBG("hey");
      }
      notnull((void*)node->name);

      strcpy(node->name, seg_start);
      node->depth = current_depth;
      node->subtree_size = 0;
      node->is_dir = false;
      current_index++;
    }

    strcpy(last_path, path);
  }

  while (stack_top > 0) {
    stack_top--;
    int dir_idx = stack[stack_top];
    tree->nodes[dir_idx].subtree_size = current_index - dir_idx - 1;
  }

#ifdef DEBUG
  if (current_index != node_count) {
    DBG("Tree build error: expected %zu nodes, got %d", node_count,
        current_index);
  }
#endif
}

#endif  // PAK_TREE_IMPLEMENTATION
#endif  // PAK_TREE_HEADER_
