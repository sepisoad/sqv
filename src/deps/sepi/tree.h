#ifndef SEPI_TREE_H
#define SEPI_TREE_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../tracy/tracy.h"

#include "base.h"
#include "string.h"
#include "arena.h"
#include "array.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

// --

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct TreeNode TreeNode;
struct TreeNode {
  RawPtr data;
  TreeNode* parent;
  ArrayOf(TreeNode) children;
};

typedef struct Tree Tree;
struct Tree {
  Arena* arena;
  TreeNode* root;
  struct {
    Sz item_size;
    Sz item_alignment;
  } meta;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Tree* tree_create(Arena* arena, RawPtr data, Sz item_size, Sz item_alignment);
Nothing tree_destroy(Tree* tree);
TreeNode* tree_push(Tree* tree, TreeNode* node, RawPtr data);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_TREE_IMPLEMENTATION

SLAVE_PROFILING_CONTEXT;

Tree*
tree_create(Arena* arena, RawPtr data, Sz item_size, Sz item_alignment) {
  START_PROFILING(1);

  Assert(arena != 0);

  Tree* tree = arena_push(arena, sizeof(Tree), AlignOf(Tree), TRUE);

  tree->arena = arena;
  tree->meta.item_size = item_size;
  tree->meta.item_alignment = item_alignment;

  tree->root = arena_push(arena, sizeof(TreeNode), AlignOf(TreeNode), TRUE);
  tree->root->data = data;
  tree->root->parent = 0;
  tree->root->children = array_create(tree->arena, tree->meta.item_size,
                                tree->meta.item_alignment);

  END_PROFILING();
  return tree;
}

Nothing
tree_destroy(Tree* tree) {
  START_PROFILING(1);

  Assert(tree != 0);
  MemZero(tree, sizeof(Tree));

  END_PROFILING();
}

TreeNode*
tree_push(Tree* tree, TreeNode* node, RawPtr data) {
  START_PROFILING(1);

  Assert(data != 0);
  Assert(node != 0);
  Assert(node->children != 0);

  TreeNode data_node = {
      .data = data,
      .parent = node,
      .children = array_create(tree->arena, tree->meta.item_size,
                               tree->meta.item_alignment),
  };

  TreeNode* stored_node = array_push(node->children, &data_node);

  END_PROFILING();
  return stored_node;
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_TREE_IMPLEMENTATION */
#endif /* SEPI_TREE_H */
