/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflnode.h
    Title:      Linked-list functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/03  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Provides functions to maintain doubly-linked lists.  You can
                use these functions to work with lists of any structure.  To
                make this work, all structures must start with two pointers,
                "void *next, *prev;".  When you want to attach a linked-list
                to another structure, declare the list head as a NODE.  You
                can then refer to this variable when you attach items to the
                list head.  The code sets the global node_unsafe to TRUE
                whenever it is changing a list.  NOTE: DEPRECATED IN FAVOUR
                OF SFLLIST.C.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLNODE_INCLUDED               /*  Allow multiple inclusions        */
#define SFLNODE_INCLUDED


/*  The node descriptor simply contains two pointers.  All blocks that are   */
/*  descriptors that are held in lists.  We can (a) allocate a dummy node    */
/*  instead of a complete block for a list head, and (b) use the same list   */
/*  handling functions for all descriptors.                                  */

typedef struct {                        /*  Node descriptor                  */
    void *next, *prev;                  /*    for a doubly-linked list       */
} NODE;

/*  Global variables                                                         */

extern Bool
    node_unsafe;                        /*  TRUE if we're changing a list    */

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

void *node_create        (void *after, size_t size);
void  node_destroy       (void *node);
void *node_unlink        (void *node);
void *node_relink        (void *left, void *node, void *right);
void *node_relink_after  (void *node, void *after);
void *node_relink_before (void *node, void *before);

#ifdef __cplusplus
}
#endif

/*  Macros                                                                   */

#define node_reset(node)          (node)-> prev = (node)-> next = (node)

#endif
