/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfltree.h
    Title:      Linked-list functions
    Package:    Standard Function Library (SFL)

    Written:    1997/11/18  Jonathan Schultz <jonathan@imatix.com>
    Revised:    1998/01/03  Jonathan Schultz <jonathan@imatix.com>

    Synopsis:   Provides functions to maintain 'Red-Black' balanced binary
                trees.  You can use these functions to work with trees of any
                structure.  To make this work, all structures must start with
                the following: "void *left, *right, *parent; TREE_COLOUR
                colour;".  All trees need a pointer to the root of type TREE
                which should be initialised with tree_init - you can test
                whether a tree is empty by comparing its root with TREE_NULL.
                The order of nodes in the tree is determined by calling a
                node comparison function provided by the caller - this
                accepts two node pointers  and returns zero if the two nodes
                are equal, -1 if the first is smaller and 1 if the first is
                larger.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLTREE_INCLUDED               /*  Allow multiple inclusions        */
#define SFLTREE_INCLUDED


/* Red-Black tree description */

typedef enum {BLACK, RED} TREE_COLOUR;

/*  Node descriptor                                                          */

typedef struct _TREE {
    struct _TREE
        *left, *right, *parent;
    TREE_COLOUR
         colour;
} TREE;

/*  The tree algorithm needs to know how to sort the data.  It does this     */
/*  using a functions provided by the calling program.                       */

typedef int (TREE_COMPARE) (void *t1, void *t2);

/*  Define a function type for use with the tree traversal function          */

typedef void (TREE_PROCESS) (void *t);

/*  Global variables                                                         */

extern TREE
    TREE_EMPTY;

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

void  tree_init     (TREE **root);
int   tree_insert   (TREE **root, void *tree, TREE_COMPARE *comp,
                     Bool allow_duplicates);
void  tree_delete   (TREE **root, void *tree);
void *tree_find_eq  (TREE **root, void *tree, TREE_COMPARE *comp);
void *tree_find_lt  (TREE **root, void *tree, TREE_COMPARE *comp);
void *tree_find_le  (TREE **root, void *tree, TREE_COMPARE *comp);
void *tree_find_gt  (TREE **root, void *tree, TREE_COMPARE *comp);
void *tree_find_ge  (TREE **root, void *tree, TREE_COMPARE *comp);
void  tree_traverse (void *tree, TREE_PROCESS *process, int method);
void *tree_next     (void *tree);
void *tree_prev     (void *tree);
void *tree_first    (void *tree);
void *tree_last     (void *tree);

#ifdef __cplusplus
}
#endif

/*  Return codes                                                             */

#define TREE_DUPLICATE -1
#define TREE_OK         0

/*  Macros                                                                   */

#define TREE_NULL &TREE_EMPTY

#endif

