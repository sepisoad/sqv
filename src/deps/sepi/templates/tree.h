#ifndef SEPI_TREE__CLASS__H
#define SEPI_TREE__CLASS__H

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

#define _ARRAY__CLASS__MIN_SEGMENT 6
#define _ARRAY__CLASS__MAX_SEGMENT 26
#define _ARRAY__CLASS__64_BITS 8 * sizeof(U64)

#define _array_tree_node__class__get_segment_capacity(segment) \
  ((1 << _ARRAY__CLASS__MIN_SEGMENT) << (segment)) - \
      (1 << _ARRAY__CLASS__MIN_SEGMENT)

#define _array_tree_node__class__get_segment_from_index_(index) \
  ((U32)(_ARRAY__CLASS__64_BITS - get_leading_0_bits(index)))

#define _array_tree_node__class__get_segment_from_index(index) \
  _array_tree_node__class__get_segment_from_index_(            \
      ((index) >> _ARRAY__CLASS__MIN_SEGMENT) + 1)

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct _Class_ _Class_;
typedef struct Tree_Class_ Tree_Class_;
typedef struct TreeNode_Class_ TreeNode_Class_;
typedef struct _ArrayTreeNode_Class_ _ArrayTreeNode_Class_;

struct _ArrayTreeNode_Class_ {
  U64 capacity;
  U64 offset;
  U64 used_segments;
  TreeNode_Class_* segments[_ARRAY__CLASS__MAX_SEGMENT];
  Arena* arena;
};

struct TreeNode_Class_ {
  _Type_ data;
  TreeNode_Class_* parent;
  _ArrayTreeNode_Class_* children;
};

struct Tree_Class_ {
  TreeNode_Class_ root;
  Arena* arena;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

fn Tree_Class_ tree__class__make(Arena* arena);
fn TreeNode_Class_* tree__class__push(Tree_Class_* tree,
                                   TreeNode_Class_* node,
                                   _Type_ data);
fn U64 tree__class__node_length(TreeNode_Class_* node);
fn TreeNode_Class_* tree__class__node_get_child(TreeNode_Class_* node, U64 index);
fn _Type_ tree__class__node_get_data(TreeNode_Class_* node, U64 index);

fn _ArrayTreeNode_Class_ _array_tree_node__class__make(Arena* arena);
fn Nothing _array_tree_node__class__init(Arena* arena, _ArrayTreeNode_Class_* array);
fn TreeNode_Class_* _array_tree_node__class__get(_ArrayTreeNode_Class_* array, U64 index);
fn TreeNode_Class_* _array_tree_node__class__push(_ArrayTreeNode_Class_* array, TreeNode_Class_* ptr);
fn U32 _array_tree_node__class__length(_ArrayTreeNode_Class_* array);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_TREE__CLASS__IMPLEMENTATION

fn Tree_Class_
tree__class__make(Arena* arena) {
  start_profiling(1);

  assert(arena != 0);

  Tree_Class_ tree;

  tree.root.children = arena_push(arena, sizeof(_ArrayTreeNode_Class_),
                                  alignof(_ArrayTreeNode_Class_), TRUE);

  _array_tree_node__class__init(arena, tree.root.children);

  tree.arena = arena;
  tree.root.parent = 0;

  end_profiling();
  return tree;
}

fn TreeNode_Class_*
tree__class__push(Tree_Class_* tree, TreeNode_Class_* node, _Type_ data) {
  start_profiling(1);

  assert(tree != 0);
  assert(node != 0);

  _ArrayTreeNode_Class_* children =
      arena_push(tree->arena, sizeof(_ArrayTreeNode_Class_),
                 alignof(_ArrayTreeNode_Class_), TRUE);

  _array_tree_node__class__init(tree->arena, children);

  TreeNode_Class_ child = {
      .data = data,
      .parent = node,
      .children = children,
  };

  TreeNode_Class_* stored_node =
      _array_tree_node__class__push(node->children, &child);

  end_profiling();
  return stored_node;
}

fn U64
tree__class__node_length(TreeNode_Class_* node) {
  return _array_tree_node__class__length(node->children);
}

fn TreeNode_Class_*
tree__class__node_get_child(TreeNode_Class_* node, U64 index) {
  return _array_tree_node__class__get(node->children, index);
}

fn _Type_
tree__class__node_get_data(TreeNode_Class_* node, U64 index) {
  return _array_tree_node__class__get(node->children, index)->data;
}

fn _ArrayTreeNode_Class_
_array_tree_node__class__make(Arena* arena) {
  start_profiling(1);

  assert(arena != 0);

  U64 used_segments = 1;
  _ArrayTreeNode_Class_ array = {
      .arena = arena,
      .used_segments = used_segments,
      .capacity = _array_tree_node__class__get_segment_capacity(used_segments),
      .offset = 0,
  };
  array.segments[used_segments - 1] =
      arena_push(arena, sizeof(_Type_) * array.capacity, alignof(_Type_), TRUE);

  end_profiling();
  return array;
}

fn Nothing
_array_tree_node__class__init(Arena* arena, _ArrayTreeNode_Class_* array) {
  start_profiling(1);

  assert(arena != 0);
  assert(array != 0);
  assert(array->arena == 0);
  assert(array->used_segments == 0);
  assert(array->capacity == 0);
  assert(array->offset == 0);

  U64 used_segments = 1;
  array->arena = arena;
  array->used_segments = used_segments;
  array->capacity = _array_tree_node__class__get_segment_capacity(used_segments);
  array->offset = 0;

  array->segments[used_segments - 1] = arena_push(
      arena, sizeof(_Type_) * array->capacity, alignof(_Type_), TRUE);

  end_profiling();
}

fn TreeNode_Class_*
_array_tree_node__class__get(_ArrayTreeNode_Class_* array, U64 index) {
  start_profiling(1);

  assert(array != 0);
  assert(index < array->capacity);

  TreeNode_Class_* res = 0;
  U64 seg = _array_tree_node__class__get_segment_from_index(index);
  U64 base = (seg > 1) ? _array_tree_node__class__get_segment_capacity(seg - 1) : 0;
  U64 slot = index - base;
  res = array->segments[seg - 1] + slot;

  end_profiling();
  return res;
}

fn TreeNode_Class_*
_array_tree_node__class__push(_ArrayTreeNode_Class_* array, TreeNode_Class_* ptr) {
  start_profiling(1);

  assert(array != 0);
  assert(ptr != 0);

  if (array->offset >=
      (U64)_array_tree_node__class__get_segment_capacity(array->used_segments)) {
    U64 old_cap = _array_tree_node__class__get_segment_capacity(array->used_segments);
    U64 new_cap = _array_tree_node__class__get_segment_capacity(array->used_segments + 1);
    U64 seg_size = new_cap - old_cap;

    array->segments[array->used_segments] = arena_push(
        array->arena, sizeof(_Type_) * seg_size, alignof(_Type_), TRUE);

    array->used_segments++;
    array->capacity = _array_tree_node__class__get_segment_capacity(array->used_segments);
  }

  TreeNode_Class_* res = _array_tree_node__class__get(array, array->offset);
  copy_memory(res, ptr, sizeof(TreeNode_Class_));
  array->offset++;

  end_profiling();
  return res;
}

fn U32
_array_tree_node__class__length(_ArrayTreeNode_Class_* array) {
  /* NOTE: no place for profiling! */
  return array->offset;
}


/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_TREE__CLASS__IMPLEMENTATION */
#endif /* SEPI_TREE__CLASS__H */
