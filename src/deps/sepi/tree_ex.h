#ifndef SEPI_TREE_EX_H
#define SEPI_TREE_EX_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../tracy/tracy.h"

#include "base.h"
#include "arena.h"
#include "array_ex.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

// --

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

#define DefineTree(TYPE, CLASS, METHOD)                                      \
  typedef struct Tree##CLASS##Node Tree##CLASS##Node;                        \
  typedef struct Tree##CLASS Tree##CLASS;                                    \
  typedef struct ArrayTree##CLASS##Node ArrayTree##CLASS##Node;              \
                                                                             \
  struct Tree##CLASS##Node {                                                 \
    TYPE data;                                                               \
    Tree##CLASS##Node* parent;                                               \
    ArrayTree##CLASS##Node* children;                                        \
  };                                                                         \
                                                                             \
  struct Tree##CLASS {                                                       \
    Tree##CLASS##Node root;                                                  \
    Arena* arena;                                                            \
  };                                                                         \
                                                                             \
  DefineArray(Tree##CLASS##Node, Tree##CLASS##Node, tree_##METHOD##_node);   \
                                                                             \
  Tree##CLASS tree_##METHOD##_make(Arena* arena) {                           \
    START_PROFILING(1);                                                      \
                                                                             \
    Assert(arena != 0);                                                      \
                                                                             \
    Tree##CLASS tree;                                                        \
                                                                             \
    tree.root.children = arena_push(arena, sizeof(ArrayTree##CLASS##Node),   \
                                    AlignOf(ArrayTree##CLASS##Node), TRUE);  \
                                                                             \
    array_tree_##METHOD##_node_init(arena, tree.root.children);              \
                                                                             \
    tree.arena = arena;                                                      \
    tree.root.parent = 0;                                                    \
                                                                             \
    END_PROFILING();                                                         \
    return tree;                                                             \
  }                                                                          \
                                                                             \
  Tree##CLASS##Node* tree_##METHOD##_push(                                   \
      Tree##CLASS* tree, Tree##CLASS##Node* node, TYPE data) {               \
    START_PROFILING(1);                                                      \
                                                                             \
    Assert(tree != 0);                                                       \
    Assert(node != 0);                                                       \
                                                                             \
    ArrayTree##CLASS##Node* children =                                       \
        arena_push(tree->arena, sizeof(ArrayTree##CLASS##Node),              \
                   AlignOf(ArrayTree##CLASS##Node), TRUE);                   \
                                                                             \
    array_tree_##METHOD##_node_init(tree->arena, children);                  \
                                                                             \
    Tree##CLASS##Node child = {                                              \
        .data = data,                                                        \
        .parent = node,                                                      \
        .children = children,                                                \
    };                                                                       \
                                                                             \
    Tree##CLASS##Node* stored_node =                                         \
        array_tree_##METHOD##_node_push(node->children, &child);             \
                                                                             \
    END_PROFILING();                                                         \
    return stored_node;                                                      \
  }                                                                          \
                                                                             \
  U64 tree_##METHOD##_node_length(Tree##CLASS##Node* node) {                 \
    return array_tree_##METHOD##_node_length(node->children);                \
  }                                                                          \
                                                                             \
  Tree##CLASS##Node* tree_##METHOD##_node_get_child(Tree##CLASS##Node* node, \
                                                    U64 index) {             \
    return array_tree_##METHOD##_node_get(node->children, index);            \
  }                                                                          \
                                                                             \
  TYPE tree_##METHOD##_node_get_data(Tree##CLASS##Node* node, U64 index) {   \
    return array_tree_##METHOD##_node_get(node->children, index)->data;      \
  }

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

// --

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_TREE_EX_IMPLEMENTATION

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_TREE_EX_IMPLEMENTATION */
#endif /* SEPI_TREE_EX_H */
