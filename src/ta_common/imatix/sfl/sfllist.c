/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfllist.c
    Title:      Linked-list functions
    Package:    Standard Function Library (SFL)

    Written:    1997/07/28  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/02/15

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfllist.h"                    /*  Prototypes for functions         */
#include "sflmem.h"                     /*  Memory allocation functions      */


Bool
    list_unsafe = FALSE;                /*  When we're changing a list       */


/*  ---------------------------------------------------------------------[<]-
    Function: list_unlink

    Synopsis: Unlinks the list from any list it may be in.  Returns list.
    ---------------------------------------------------------------------[>]-*/

void *
list_unlink (
    void *list)
{
    list_unsafe = TRUE;

    /*  Join together next and previous nodes */
    ((LIST *) ((LIST *) list)-> prev)-> next = ((LIST *) list)-> next;
    ((LIST *) ((LIST *) list)-> next)-> prev = ((LIST *) list)-> prev;

    /*  The list is now empty */
    list_reset ((LIST *) list);

    list_unsafe = FALSE;
    return (list);
}


/*  ---------------------------------------------------------------------[<]-
    Function: list_relink

    Synopsis: Links the list into a linked list.  This is a general-purpose
    function that can be used to attach and remove lists anywhere in a list.
    Sets the global variable 'list_unsafe' while the list is being changed.
    Returns the address of list.
    ---------------------------------------------------------------------[>]-*/

void *
list_relink (
    void *left,
    void *list,
    void *right)
{
    LIST *swap;

    list_unsafe = TRUE;
    swap = ((LIST *) left)-> next;      /*  Exchange left pointers           */
           ((LIST *) left)-> next = list;
    ((LIST *) ((LIST *) list)-> prev)-> next = swap;

    swap = ((LIST *) right)-> prev;     /*  Exchange right pointers          */
           ((LIST *) right)-> prev = ((LIST *) list)-> prev;
                                     ((LIST *) list)-> prev = swap;

    list_unsafe = FALSE;
    return (list);
}


/*  ---------------------------------------------------------------------[<]-
    Function: list_add

    Synopsis: Creates a node at the head of a list of a specified size and
    copies the specified data into it.  Use with the stack & queue macros
    list_push and list_queue.
    ---------------------------------------------------------------------[>]-*/

void *
list_add (LIST *list, void *data, size_t size)
{
    LIST
        *node;

    node = mem_alloc (sizeof (LIST) + size);
    if (node)
      {
        list_reset (node);
        list_relink_after (node, list);
        memcpy ((char *) node + sizeof (LIST), (char *) data, size);
      }
    return node;
}
    

/*  ---------------------------------------------------------------------[<]-
    Function: list_remove

    Synopsis: Removes the node at the head of a list, copying the specified
    amount of data to the specified data block.  This size must be exactly
    equal to the size of the data block.  The list may not be empty.  Use
    this with the macro list_pop.
    ---------------------------------------------------------------------[>]-*/

void
list_remove (LIST *list, void *data, size_t size)
{
LIST
    *node;

    ASSERT (!list_empty (list));

    node = list-> next;
    ASSERT (mem_size (node) - sizeof (LIST) == size);

    memcpy ((char *) data, (char *) node + sizeof (LIST), size);
    list_unlink (node);
    mem_free (node);
}


/*  ---------------------------------------------------------------------[<]-
    Function: list_sort

    Synopsis: Sorts a list using the "comb-sort" algorithm.
    ---------------------------------------------------------------------[>]-*/

void list_sort (void *list, NODE_COMPARE comp)
{
    int
        jump_size,
        i;
    LIST
        *base,
        *swap,
        *temp;
    Bool
        swapped;

    jump_size = 0;
    FORLIST (base, * (LIST *) list)
        jump_size++;

    swapped = TRUE;
    while ((jump_size > 1) || swapped)
      {
        jump_size = (10 * jump_size + 3) / 13;
        base = ((LIST *) list)-> next;
        swap = base;
        for (i = 0; i < jump_size; i++)
            swap = swap-> next;

        swapped = FALSE;
        while (swap != (LIST *) list)
          {
            if ((*comp) (base, swap))
              {
                temp = base-> prev;
                list_unlink (base);
                list_relink_after (base, swap);
                list_unlink (swap);
                list_relink_after (swap, temp);
                temp = base;
                base = swap;
                swap = temp;
                swapped = TRUE;
              }
            base = base-> next;
            swap = swap-> next;
          }
      }
}
