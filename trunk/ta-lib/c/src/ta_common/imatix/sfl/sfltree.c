/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfltree.c
    Title:      Linked-list functions
    Package:    Standard Function Library (SFL)

    Written:    1997/11/18  Jonathan Schultz <jonathan@imatix.com>
    Revised:    1998/12/08  Jonathan Schultz <jonathan@imatix.com>

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfltree.h"                    /*  Prototypes for functions         */

/*  Constants                                                                */

TREE
    TREE_EMPTY = {TREE_NULL, TREE_NULL, NULL, BLACK};

/*  Internal function prototypes                                             */

static void insert_fixup (TREE **root, TREE *tree);
static void rotate_left  (TREE **root, TREE *tree);
static void rotate_right (TREE **root, TREE *tree);
static void delete_fixup (TREE **root, TREE *tree);


/*  ---------------------------------------------------------------------[<]-
    Function: tree_init

    Synopsis: Initialises an empty tree.
    ---------------------------------------------------------------------[>]-*/

void tree_init (TREE **root)
{
    *root = TREE_NULL;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_insert

    Synopsis: Inserts a node into an existing tree.  Initialises node
    pointers and colour to correct values.  The data used by the compare
    functions must be filled in so that tree_insert can find the correct
    place in the tree to insert the node.
    ---------------------------------------------------------------------[>]-*/

int tree_insert (TREE **root, void *tree, TREE_COMPARE *comp,
                 Bool allow_duplicates)
{
    TREE
       *current,
       *parent;
    int
        last_comp = 0;

    /* find where node belongs */
    current = *root;
    parent  = NULL;
    while (current != TREE_NULL)
      {
        parent  = current;
        last_comp = (comp) (tree, current);
        switch (last_comp)
          {
            case -1: current = current-> left;  break;
            case  1: current = current-> right; break;
            default: if (allow_duplicates)
                         current = current-> left;
                     else
                         return TREE_DUPLICATE;

          }
      }

    /* setup new node */
    ((TREE *) tree)-> parent = parent;
    ((TREE *) tree)-> left   = TREE_NULL;
    ((TREE *) tree)-> right  = TREE_NULL;
    ((TREE *) tree)-> colour = RED;

    /* insert node in tree */
    if (parent)
        switch (last_comp)
          {
            case  1: parent-> right = tree;  break;
            default: parent-> left  = tree;
          }
    else
        *root = tree;

    insert_fixup (root, tree);
    return (TREE_OK);
}

/*  -------------------------------------------------------------------------
    Internal Function: insert_fixup

    Synopsis: Maintains the Red-Black tree balance after a node has been
    inserted.
    -------------------------------------------------------------------------*/

static void
insert_fixup (TREE **root, TREE *tree)
{
    TREE *uncle;

    /* check red-black properties */
    while ((tree != *root)
       &&  (tree-> parent-> colour == RED))
      {
        /* we have a violation */
        if (tree-> parent == tree-> parent-> parent-> left)
          {
            uncle = tree-> parent-> parent-> right;
            if (uncle-> colour == RED)
              {
                /* uncle is RED */
                tree -> parent->          colour = BLACK;
                uncle->                   colour = BLACK;
                tree -> parent-> parent-> colour = RED;

                tree = tree-> parent-> parent;
              }
            else
              {
                /* uncle is BLACK */
                if (tree == tree-> parent-> right)
                  {
                    /* make tree a left child */
                    tree = tree-> parent;
                    rotate_left (root, tree);
                  }

                /* recolor and rotate */
                tree-> parent->          colour = BLACK;
                tree-> parent-> parent-> colour = RED;
                rotate_right (root, tree-> parent-> parent);
              }
          }
        else
          {
            /* mirror image of above code */
            uncle = tree-> parent-> parent-> left;
            if (uncle-> colour == RED)
              {
                /* uncle is RED */
                tree -> parent->          colour = BLACK;
                uncle->                   colour = BLACK;
                tree -> parent-> parent-> colour = RED;

                tree = tree-> parent-> parent;
              }
            else
              {
                /* uncle is BLACK */
                if (tree == tree-> parent-> left)
                  {
                    tree = tree-> parent;
                    rotate_right (root, tree);
                  }
                tree-> parent->          colour = BLACK;
                tree-> parent-> parent-> colour = RED;
                rotate_left (root, tree-> parent-> parent);
              }
          }
      }
    (*root)-> colour = BLACK;
}

/*  -------------------------------------------------------------------------
    Internal Function: rotate_left

    Synopsis: Rotates tree to left.
    -------------------------------------------------------------------------*/

static void
rotate_left (TREE **root, TREE *tree)
{
    TREE *other = tree-> right;

    /* establish tree-> right link */
    tree-> right = other-> left;
    if (other-> left != TREE_NULL)
        other-> left-> parent = tree;

    /* establish other-> parent link */
    if (other != TREE_NULL)
        other-> parent = tree-> parent;

    if (tree-> parent)
      {
        if (tree == tree-> parent-> left)
            tree-> parent-> left  = other;
        else
            tree-> parent-> right = other;
      }
    else
        *root = other;

    /* link tree and other */
    other-> left = tree;
    if (tree != TREE_NULL)
        tree-> parent = other;
}

/*  -------------------------------------------------------------------------
    Internal Function: rotate_right

    Synopsis: Rotates tree to right.
    -------------------------------------------------------------------------*/

static void
rotate_right (TREE **root, TREE *tree)
{
    TREE *other;

    other = tree-> left;

    /* establish tree-> left link */
    tree-> left = other-> right;
    if (other-> right != TREE_NULL)
        other-> right-> parent = tree;

    /* establish other-> parent link */
    if (other != TREE_NULL)
        other-> parent = tree-> parent;

    if (tree-> parent)
      {
        if (tree == tree-> parent-> right)
            tree-> parent-> right = other;
        else
            tree-> parent-> left  = other;
      }
    else
        *root = other;

    /* link tree and other */
    other-> right = tree;
    if (tree != TREE_NULL)
        tree-> parent = other;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_delete

    Synopsis: Deletes a node from a tree.  Does not deallocate any memory.
    ---------------------------------------------------------------------[>]-*/

void tree_delete (TREE **root, void *tree)
{
    TREE
       *youngest, *descendent;
    TREE_COLOUR
        colour;

    if ((!tree)
    ||  (tree == TREE_NULL))
        return;

    if ((((TREE *) tree)-> left  == TREE_NULL)
    ||  (((TREE *) tree)-> right == TREE_NULL))
        /* descendent has a TREE_NULL node as a child */
        descendent = tree;
    else
      {
        /* find tree successor with a TREE_NULL node as a child */
        descendent = ((TREE *) tree)-> right;
        while (descendent-> left != TREE_NULL)
            descendent = descendent-> left;
      }

    /* youngest is descendent's only child, if there is one, else TREE_NULL */
    if (descendent-> left != TREE_NULL)
        youngest = descendent-> left;
    else
        youngest = descendent-> right;

    /* remove descendent from the parent chain */
    if (youngest != TREE_NULL)
        youngest-> parent = descendent-> parent;
    if (descendent-> parent)
        if (descendent == descendent-> parent-> left)
            descendent-> parent-> left  = youngest;
        else
            descendent-> parent-> right = youngest;
    else
        *root = youngest;

    colour = descendent-> colour;

    if (descendent != (TREE *) tree)
      {
        /* Conceptually what we are doing here is moving the data from       */
        /* descendent to tree.  In fact we do this by linking descendent     */
        /* into the structure in the place of tree.                          */
        descendent-> left   = ((TREE *) tree)-> left;
        descendent-> right  = ((TREE *) tree)-> right;
        descendent-> parent = ((TREE *) tree)-> parent;
        descendent-> colour = ((TREE *) tree)-> colour;

        if (descendent-> parent)
          {
            if (tree == descendent-> parent-> left)
                descendent-> parent-> left  = descendent;
            else
                descendent-> parent-> right = descendent;
          }
        else
            *root = descendent;

        if (descendent-> left != TREE_NULL)
            descendent-> left-> parent = descendent;

        if (descendent-> right != TREE_NULL)
            descendent-> right-> parent = descendent;
      }

    if ((youngest != TREE_NULL)
    &&  (colour   == BLACK))
        delete_fixup (root, youngest);
}

/*  -------------------------------------------------------------------------
    Internal Function: delete_fixup

    Synopsis: Maintains Red-Black tree balance after deleting a node.
    -------------------------------------------------------------------------*/

static void
delete_fixup (TREE **root, TREE *tree)
{
    TREE
       *sibling;

    while (tree != *root && tree-> colour == BLACK)
      {
        if (tree == tree-> parent-> left)
          {
            sibling = tree-> parent-> right;
            if (sibling-> colour == RED)
              {
                sibling->       colour = BLACK;
                tree-> parent-> colour = RED;
                rotate_left (root, tree-> parent);
                sibling = tree-> parent-> right;
              }
            if ((sibling-> left->  colour == BLACK)
            &&  (sibling-> right-> colour == BLACK))
              {
                sibling-> colour = RED;
                tree = tree-> parent;
              }
            else
              {
                if (sibling-> right-> colour == BLACK)
                  {
                    sibling-> left-> colour = BLACK;
                    sibling->        colour = RED;
                    rotate_right (root, sibling);
                    sibling = tree-> parent-> right;
                  }
                sibling-> colour = tree-> parent-> colour;
                tree->    parent-> colour = BLACK;
                sibling-> right->  colour = BLACK;
                rotate_left (root, tree-> parent);
                tree = *root;
              }
          }
        else
          {
            sibling = tree-> parent-> left;
            if (sibling-> colour == RED)
              {
                sibling->       colour = BLACK;
                tree-> parent-> colour = RED;
                rotate_right (root, tree-> parent);
                sibling = tree-> parent-> left;
              }
            if ((sibling-> right-> colour == BLACK)
            &&  (sibling-> left->  colour == BLACK))
              {
                sibling-> colour = RED;
                tree = tree-> parent;
              }
            else
              {
                if (sibling-> left-> colour == BLACK)
                  {
                    sibling-> right-> colour = BLACK;
                    sibling->         colour = RED;
                    rotate_left (root, sibling);
                    sibling = tree-> parent-> left;
                  }
                sibling-> colour = tree-> parent-> colour;
                tree->    parent-> colour = BLACK;
                sibling-> left->   colour = BLACK;
                rotate_right (root, tree-> parent);
                tree = *root;
              }
          }
      }
    tree-> colour = BLACK;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_find_eq

    Synopsis: Finds a node with data exactly matching that provided.
    ---------------------------------------------------------------------[>]-*/

void *tree_find_eq (TREE **root, void *tree, TREE_COMPARE *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch ((comp) (current, tree))
          {
            case -1: current = current-> right; break;
            case  1: current = current-> left;  break;
            default: found = current;           /*  In case of duplicates,   */
                     current = current-> left;  /*  get the first one.       */
          }

    return found;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_find_lt

    Synopsis: Finds node with data less than that provided.
    ---------------------------------------------------------------------[>]-*/

void *tree_find_lt (TREE **root, void *tree, TREE_COMPARE *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch ((comp) (current, tree))
          {
            case -1: found = current;
                     current = current-> right; break;
            default: current = current-> left;
          }

    return found;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_find_le

    Synopsis: Finds node with data less than or equal to that provided.
    ---------------------------------------------------------------------[>]-*/

void *tree_find_le (TREE **root, void *tree, TREE_COMPARE *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch ((comp) (current, tree))
          {
            case 1 : current = current-> left;  break;
            default: found = current;
                     current = current-> right;
          }

    return found;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_find_gt

    Synopsis: Finds node with data greater than that provided.
    ---------------------------------------------------------------------[>]-*/

void *tree_find_gt (TREE **root, void *tree, TREE_COMPARE *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch ((comp) (current, tree))
          {
            case 1 : found = current;
                     current = current-> left; break;
            default: current = current-> right;
          }

    return found;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_find_ge

    Synopsis: Finds node with data greater than or equal to that provided.
    ---------------------------------------------------------------------[>]-*/

void *tree_find_ge (TREE **root, void *tree, TREE_COMPARE *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch ((comp) (current, tree))
          {
            case -1: current = current-> right; break;
            default: found = current;
                     current = current-> left;
          }

    return found;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_traverse

    Synopsis: Traverse the tree, calling a processing function at each
    node.
    ---------------------------------------------------------------------[>]-*/

void tree_traverse (void *tree, TREE_PROCESS *process, int method)
{
    if ((!tree)
    ||  (tree == TREE_NULL))
        return;

    if (method == 1)
      {
        (process) (tree);
        tree_traverse (((TREE *) tree)-> left,  process, method);
        tree_traverse (((TREE *) tree)-> right, process, method);
      }
    else if (method == 2)
      {
        tree_traverse (((TREE *) tree)-> left,  process, method);
        tree_traverse (((TREE *) tree)-> right, process, method);
        (process) (tree);
      }
    else
      {
        tree_traverse (((TREE *) tree)-> left,  process, method);
        (process) (tree);
        tree_traverse (((TREE *) tree)-> right, process, method);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_first

    Synopsis: Finds and returns the first node in a (sub-)tree.
    ---------------------------------------------------------------------[>]-*/

void *tree_first (void *tree)
{
    TREE
       *current;

    if ((!tree)
    ||  (tree == TREE_NULL))
        return NULL;

    current = tree;
    while (current-> left != TREE_NULL)
        current = current-> left;

    return current;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_last

    Synopsis: Finds and returns the last node in a (sub-)tree.
    ---------------------------------------------------------------------[>]-*/

void *tree_last (void *tree)
{
    TREE
       *current;

    if ((!tree)
    ||  (tree == TREE_NULL))
        return NULL;

    current = tree;
    while (current-> right != TREE_NULL)
        current = current-> right;

    return current;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_next

    Synopsis: Finds and returns the next node in a tree.
    ---------------------------------------------------------------------[>]-*/

void *tree_next (void *tree)
{
    TREE
       *current,
       *child;

    if ((!tree)
    ||  (tree == TREE_NULL))
        return NULL;

    current = tree;
    if (current-> right != TREE_NULL)
        return tree_first (current-> right);
    else
      {
        current = tree;
        child   = TREE_NULL;
        while ((current-> parent)
           &&  (current-> right == child))
          {
            child = current;
            current = current-> parent;
          }
        if (current-> right != child)
            return current;
        else
            return NULL;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: tree_prev

    Synopsis: Finds and returns the previous node in a tree.
    ---------------------------------------------------------------------[>]-*/

void *tree_prev (void *tree)
{
    TREE
       *current,
       *child;

    if ((!tree)
    ||  (tree == TREE_NULL))
        return NULL;

    current = tree;
    if (current-> left != TREE_NULL)
        return tree_last (current-> left);
    else
      {
        current = tree;
        child   = TREE_NULL;
        while ((current-> parent)
           &&  (current-> left == child))
          {
            child = current;
            current = current-> parent;
          }
        if (current-> left != child)
            return current;
        else
            return NULL;
      }
}
