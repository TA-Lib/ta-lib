/*  ----------------------------------------------------------------<Prolog>-
    Name:       testtree.c
    Title:      Test program for bitstring functions
    Package:    Standard Function Library (SFL)

    Written:    1997/11/19  Jonathan Schultz <jonathan@imatix.com>
    Revised:    1998/01/03  Jonathan Schultz <jonathan@imatix.com>

    Synopsis:   Runs various tests on tree manipulation functions.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"
#include "sfltree.h"

static TREE
    *root;

typedef struct {
    void
        *left, *right, *parent;
    TREE_COLOUR
         colour;
    int n;  /* The actual data in the tree */
} MY_TREE;

#define NODE_SIZE sizeof(MY_TREE)

int comp (void *t1, void *t2)
{
    int diff;

    diff = ((MY_TREE *) t1)-> n - ((MY_TREE *) t2)-> n;

    if (diff < 0)
        return -1;
    else if (diff > 0)
        return 1;
    else
        return 0;
}

void printnode (void *t)
{
    printf ("%u\n", ((MY_TREE *) t)-> n);
}

void node_insert (int n, Bool dup)
{
    MY_TREE
        *node;

    printf ("Inserting %i...", n);

    node = mem_alloc (NODE_SIZE);
    node-> n = n;
    if (tree_insert (&root, node, comp, dup))
      {
        printf (" Duplicate\n");
        mem_free (node);
      }
    else
        printf (" OK\n");
}

void node_delete (int n)
{
    MY_TREE
         find,
        *node;

    printf ("Deleting %i...", n);

    find.n = n;
    node = tree_find_eq (&root, &find, comp);
    if (node)
      {
        printf (" OK\n");
        tree_delete (&root, node);
        mem_free (node);
      }
    else
        printf (" Not found\n");
}

void runtest (Bool dup)
{
    MY_TREE
        *tree;

    tree_init (&root);

    node_insert (1, dup);
    node_insert (2, dup);
    node_insert (3, dup);
    node_insert (3, dup);

    printf ("Traversing...\n");
    tree_traverse (root, printnode, 0);

    printf ("First, next...\n");
    tree = tree_first (root);
    while (tree)
      {
        printnode (tree);
        tree = tree_next (tree);
      }

    printf ("Last, previous...\n");
    tree = tree_last (root);
    while (tree)
      {
        printnode (tree);
        tree = tree_prev (tree);
      }

    node_delete (1);
    node_delete (2);
    node_delete (3);
    node_delete (3);
}

int main (int argc, char *argv [])
{
    printf ("Without duplicates\n");
    runtest (FALSE);

    printf ("With duplicates\n");
    runtest (TRUE);

    mem_assert ();
    return (EXIT_SUCCESS);
}
