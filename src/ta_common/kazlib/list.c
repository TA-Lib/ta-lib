/*
 * List Abstract Data Type
 * Copyright (C) 1997 Kaz Kylheku <kaz@ashi.footprints.net>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 * $Id$
 * $Name$
 */

/**************** Additional Important Notice **************************
 *
 *       This file is NOT the original kazlib source.
 *
 *       This file have been modified for integration within TA-LIB.
 *
 *       If you wish to use the kazlib in your application, you MUST
 *       contact the original author or go get the latest and original
 *       work on the 'kazlib' web site.
 *
 *       This Notice should NEVER be removed from this derivative work for
 *       the respect of the "Free Software License" find at the top of this
 *       file.
 *
 * The modifications from the original are mainly:
 *   - 'assert' have been replaced by TA_ASSERT.
 *   - 'malloc' and 'free' have been replaced by TA_Malloc() and TA_Free().
 *   - Some modif eliminates warnings.
 *   - Search for the string 'TA_' for most of the changes.
 *************************************************************************/

#include <stdlib.h>
#include <stddef.h>

/* #include <assert.h> TA_ - Removed */
#include "ta_common.h" /* TA_ - Added */
#include "ta_trace.h"  /* TA_ - Added */
#include "ta_memory.h" /* TA_ - Added */

TA_FILE_INFO;

#define LIST_IMPLEMENTATION
#include "kaz_list.h" /* TA_ - Rename because conflict with MSVC */

#define next list_next
#define prev list_prev
#define data list_data

#define pool list_pool
#define fre list_free
#define size list_size

#define nilnode list_nilnode
#define nodecount list_nodecount
#define maxcount list_maxcount

#define list_nil(L)		(&(L)->nilnode)
#define list_first_priv(L)	((L)->nilnode.next)
#define list_last_priv(L)	((L)->nilnode.prev)
#define lnode_next(N)		((N)->next)
#define lnode_prev(N)		((N)->prev)

#ifdef KAZLIB_RCSID
static const char rcsid[] = "$Id$";
#endif

/*
 * Initialize a list object supplied by the client such that it becomes a valid
 * empty list. If the list is to be ``unbounded'', the maxcount should be
 * specified as LISTCOUNT_T_MAX, or, alternately, as -1. The value zero
 * is not permitted.
 */

list_t *list_init( TA_Libc *libHandle, list_t *list, listcount_t maxcount)
{
    TA_ASSERT_RET( libHandle, list != NULL, (list_t *)NULL );
    TA_ASSERT_RET( libHandle, maxcount != 0, (list_t *)NULL );
    list->nilnode.next = &list->nilnode;
    list->nilnode.prev = &list->nilnode;
    list->nodecount = 0;
    list->maxcount = maxcount;
    return list;
}

/*
 * Dynamically allocate a list object using malloc(), and initialize it so that
 * it is a valid empty list. If the list is to be ``unbounded'', the maxcount
 * should be specified as LISTCOUNT_T_MAX, or, alternately, as -1.
 */

list_t *list_create(TA_Libc *libHandle, listcount_t maxcount)
{
    list_t *new = TA_Malloc( libHandle, sizeof *new);
    if (new) {
	TA_ASSERT_RET( libHandle,maxcount != 0, (list_t *)NULL );
	new->nilnode.next = &new->nilnode;
	new->nilnode.prev = &new->nilnode;
	new->nodecount = 0;
	new->maxcount = maxcount;
    }
    return new;
}

/*
 * Destroy a dynamically allocated list object.
 * The client must remove the nodes first.
 */

void list_destroy(TA_Libc *libHandle, list_t *list)
{
    TA_ASSERT_NO_RET( libHandle, list != NULL );
    TA_ASSERT_NO_RET( libHandle, list_isempty(list));

    TA_Free( libHandle, list);
}

/*
 * Free all of the nodes of a list. The list must contain only
 * dynamically allocated nodes. After this call, the list
 * is empty.
 */

void list_destroy_nodes(TA_Libc *libHandle, list_t *list)
{
    lnode_t *lnode, *nil, *tmp;
    
    TA_ASSERT_NO_RET( libHandle, list != NULL );
    lnode = list_first_priv(list);
    nil = list_nil(list);

    while (lnode != nil) {
	tmp = lnode->next;
	lnode->next = NULL;
	lnode->prev = NULL;
	lnode_destroy(libHandle,lnode);
	lnode = tmp;
    }

    list_init(libHandle,list, list->maxcount);
}

/*
 * Return all of the nodes of a list to a node pool. The nodes in
 * the list must all have come from the same pool.
 */

void list_return_nodes(TA_Libc *libHandle, list_t *list, lnodepool_t *pool)
{
    lnode_t *lnode, *tmp, *nil;

    TA_ASSERT_NO_RET( libHandle, list != NULL );
    TA_ASSERT_NO_RET( libHandle, pool != NULL );

    lnode = list_first_priv(list);
    nil = list_nil(list);

    while (lnode != nil) {
	tmp = lnode->next;
	lnode->next = NULL;
	lnode->prev = NULL;
	lnode_return(libHandle,pool, lnode);
	lnode = tmp;
    }

    list_init(libHandle,list, list->maxcount);
}

/*
 * Insert the node ``new'' into the list immediately after ``this'' node.
 */

void list_ins_after(TA_Libc *libHandle, list_t *list, lnode_t *new, lnode_t *this)
{
    lnode_t *that;
    
    TA_ASSERT_NO_RET( libHandle, list != NULL);
    TA_ASSERT_NO_RET( libHandle, new != NULL);
    TA_ASSERT_NO_RET( libHandle, this != NULL);
    that = this->next;

    TA_ASSERT_NO_RET( libHandle,!list_contains(libHandle,list, new));
    TA_ASSERT_NO_RET( libHandle,!lnode_is_in_a_list(libHandle,new));
    TA_ASSERT_NO_RET( libHandle,this == list_nil(list) || list_contains(libHandle,list, this));
    TA_ASSERT_NO_RET( libHandle,list->nodecount + 1 > list->nodecount);

    new->prev = this;
    new->next = that;
    that->prev = new;
    this->next = new;
    list->nodecount++;

    TA_ASSERT_NO_RET( libHandle,list->nodecount <= list->maxcount);
}

/*
 * Insert the node ``new'' into the list immediately before ``this'' node.
 */

void list_ins_before(TA_Libc *libHandle, list_t *list, lnode_t *new, lnode_t *this)
{
    lnode_t *that;

    TA_ASSERT_NO_RET( libHandle, list != NULL);
    TA_ASSERT_NO_RET( libHandle, new != NULL);
    TA_ASSERT_NO_RET( libHandle, this != NULL);
    that = this->prev;

    TA_ASSERT_NO_RET( libHandle,!list_contains(libHandle,list, new));
    TA_ASSERT_NO_RET( libHandle,!lnode_is_in_a_list(libHandle,new));
    TA_ASSERT_NO_RET( libHandle,this == list_nil(list) || list_contains(libHandle,list, this));
    TA_ASSERT_NO_RET( libHandle,list->nodecount + 1 > list->nodecount);

    new->next = this;
    new->prev = that;
    that->next = new;
    this->prev = new;
    list->nodecount++;

    TA_ASSERT_NO_RET( libHandle,list->nodecount <= list->maxcount);
}

/*
 * Delete the given node from the list.
 */

lnode_t *list_delete(TA_Libc *libHandle, list_t *list, lnode_t *del)
{
    lnode_t *next;
    lnode_t *prev;

    TA_ASSERT_RET( libHandle, list != NULL, (lnode_t *)NULL );
    TA_ASSERT_RET( libHandle, del != NULL, (lnode_t *)NULL );

    next = del->next;
    prev = del->prev;

    TA_ASSERT_RET( libHandle,list_contains(libHandle,list, del), (lnode_t *)NULL );

    prev->next = next;
    next->prev = prev;
    list->nodecount--;

    del->next = del->prev = NULL;

    return del;
}

/*
 * For each node in the list, execute the given function. The list,
 * current node and the given context pointer are passed on each
 * call to the function.
 */

void list_process(TA_Libc *libHandle, list_t *list, void *context,
	void (* function)(TA_Libc *libHandle, list_t *list, lnode_t *lnode, void *context))
{
    lnode_t *node, *next, *nil;
    
    TA_ASSERT_NO_RET( libHandle, function != NULL );
    TA_ASSERT_NO_RET( libHandle, list != NULL );

    node = list_first_priv(list);
    nil = list_nil(list);

    while (node != nil) {
	/* check for callback function deleting	*/
	/* the next node from under us		*/
	TA_ASSERT_NO_RET( libHandle,list_contains(libHandle,list, node));
	next = node->next;
	function(libHandle, list, node, context);
	node = next;
    }
}

/*
 * Dynamically allocate a list node and assign it the given piece of data.
 */

lnode_t *lnode_create(TA_Libc *libHandle, void *data)
{
    lnode_t *new ;
    
    TA_ASSERT_RET( libHandle, data != NULL, (lnode_t *)NULL );

    new = TA_Malloc( libHandle, sizeof *new);
    if (new) {
	new->data = data;
	new->next = NULL;
	new->prev = NULL;
    }
    return new;
}

/*
 * Initialize a user-supplied lnode.
 */
lnode_t *lnode_init(TA_Libc *libHandle, lnode_t *lnode, void *data)
{
    TA_ASSERT_RET( libHandle, lnode != NULL, (lnode_t *)NULL );

    lnode->data = data;
    lnode->next = NULL;
    lnode->prev = NULL;
    return lnode;
}

/*
 * Destroy a dynamically allocated node.
 */

void lnode_destroy(TA_Libc *libHandle, lnode_t *lnode)
{
    TA_ASSERT_NO_RET( libHandle, lnode != NULL );
    TA_ASSERT_NO_RET( libHandle,!lnode_is_in_a_list(libHandle,lnode));
    TA_Free( libHandle, lnode);
}

/*
 * Initialize a node pool object to use a user-supplied set of nodes.
 * The ``nodes'' pointer refers to an array of lnode_t objects, containing
 * ``n'' elements.
 */

lnodepool_t *lnode_pool_init(TA_Libc *libHandle, lnodepool_t *pool, lnode_t *nodes, listcount_t n)
{
    listcount_t i;

    TA_ASSERT_RET( libHandle,pool != NULL, (lnodepool_t *)NULL );
    TA_ASSERT_RET( libHandle,nodes != NULL, (lnodepool_t *)NULL );
    TA_ASSERT_RET( libHandle,n != 0, (lnodepool_t *)NULL );

    pool->pool = nodes;
    pool->fre = nodes;
    pool->size = n;
    for (i = 0; i < n - 1; i++) {
	nodes[i].next = nodes + i + 1;
    }
    nodes[i].next = NULL;
    nodes[i].prev = nodes;	/* to make sure node is marked ``on list'' */
    return pool;
}

/*
 * Create a dynamically allocated pool of n nodes.
 */

lnodepool_t *lnode_pool_create(TA_Libc *libHandle, listcount_t n)
{
    lnodepool_t *pool;
    lnode_t *nodes;

    TA_ASSERT_RET( libHandle,n != 0,(lnodepool_t *)NULL);

    pool = TA_Malloc( libHandle, sizeof *pool);
    if (!pool)
	return NULL;
    nodes = TA_Malloc( libHandle, n * sizeof *nodes);
    if (!nodes) {
	TA_Free( libHandle, pool);
	return NULL;
    }
    lnode_pool_init(libHandle, pool, nodes, n);
    return pool;
}

/*
 * Determine whether the given pool is from this pool.
 */

int lnode_pool_isfrom(TA_Libc *libHandle, lnodepool_t *pool, lnode_t *node)
{
    listcount_t i;

    TA_ASSERT_RET( libHandle, pool != NULL, 0 );
    TA_ASSERT_RET( libHandle, node != NULL, 0 );

    /* this is carefully coded this way because ANSI C forbids pointers
       to different objects from being subtracted or compared other
       than for exact equality */

    for (i = 0; i < pool->size; i++) {
	if (pool->pool + i == node)
	    return 1;
    }
    return 0;
}

/*
 * Destroy a dynamically allocated pool of nodes.
 */

void lnode_pool_destroy(TA_Libc *libHandle, lnodepool_t *p)
{
   TA_ASSERT_NO_RET( libHandle, p != NULL );

   TA_Free( libHandle, p->pool);
   TA_Free( libHandle, p);
}

/*
 * Borrow a node from a node pool. Returns a null pointer if the pool
 * is exhausted.
 */

lnode_t *lnode_borrow(TA_Libc *libHandle, lnodepool_t *pool, void *data)
{
    lnode_t *new;

    TA_ASSERT_RET( libHandle, pool != NULL, (lnode_t *)NULL );
    TA_ASSERT_RET( libHandle, data != NULL, (lnode_t *)NULL );

    new = pool->fre;
    if (new) {
	pool->fre = new->next;
	new->data = data;
	new->next = NULL;
	new->prev = NULL;
    }
    return new;
}

/*
 * Return a node to a node pool. A node must be returned to the pool
 * from which it came.
 */

void lnode_return(TA_Libc *libHandle, lnodepool_t *pool, lnode_t *node)
{
   TA_ASSERT_NO_RET( libHandle, pool != NULL );
   TA_ASSERT_NO_RET( libHandle, node != NULL );
   TA_ASSERT_NO_RET( libHandle,lnode_pool_isfrom(libHandle, pool, node));
   TA_ASSERT_NO_RET( libHandle,!lnode_is_in_a_list(libHandle, node));

   node->next = pool->fre;
   node->prev = node;
   pool->fre = node;
}

/*
 * Determine whether the given list contains the given node.
 * According to this function, a list does not contain its nilnode.
 */

int list_contains(TA_Libc *libHandle, list_t *list, lnode_t *node)
{
    lnode_t *n, *nil;
    
    TA_ASSERT_RET( libHandle, list != NULL, 0 );
    TA_ASSERT_RET( libHandle, node != NULL, 0 );

    nil = list_nil(list);

    for (n = list_first_priv(list); n != nil; n = lnode_next(n)) {
	if (node == n)
	    return 1;
    }

    return 0;
}

/*
 * A more generalized variant of list_transfer. This one removes a
 * ``slice'' from the source list and appends it to the destination
 * list.
 */
#if 0
!!! Currently unused in TA_LIB
void list_extract(TA_Libc *libHandle, list_t *dest, list_t *source, lnode_t *first, lnode_t *last)
{
    listcount_t moved;
   
    TA_ASSERT_NO_RET( libHandle,first == NULL || list_contains(libHandle, source, first));
    TA_ASSERT_NO_RET( libHandle,last == NULL || list_contains(libHandle, source, last));

    moved = 1;

    if (first == NULL || last == NULL)
	return;

    /* adjust the destination list so that the slice is spliced out */

    first->prev->next = last->next;
    last->next->prev = first->prev;

    /* graft the splice at the end of the dest list */

    last->next = &dest->nilnode;
    first->prev = dest->nilnode.prev;
    dest->nilnode.prev->next = first;
    dest->nilnode.prev = last;

    while (first != last) {
	first = first->next;
	TA_ASSERT_NO_RET( libHandle,first != list_nil(source));	/* oops, last before first! */
	moved++;
    }

    /* assert no overflows */
    TA_ASSERT_NO_RET( libHandle,source->nodecount - moved <= source->nodecount);
    TA_ASSERT_NO_RET( libHandle,dest->nodecount + moved >= dest->nodecount);

    /* assert no weirdness */
    TA_ASSERT_NO_RET( libHandle,moved <= source->nodecount);

    source->nodecount -= moved;
    dest->nodecount += moved;

    /* assert list sanity */
    TA_ASSERT_NO_RET( libHandle,list_verify(libHandle,source));
    TA_ASSERT_NO_RET( libHandle,list_verify(libHandle,dest));
}
#endif


/*
 * Split off a trailing sequence of nodes from the source list and relocate
 * them to the tail of the destination list. The trailing sequence begins
 * with node ``first'' and terminates with the last node of the source
 * list. The nodes are added to the end of the new list in their original
 * order.
 */
void list_transfer(TA_Libc *libHandle, list_t *dest, list_t *source, lnode_t *first)
{
    listcount_t moved = 1;
    lnode_t *last;

    TA_ASSERT_NO_RET( libHandle,first == NULL || list_contains(libHandle, source, first));

    if (first == NULL)
	return;

    last = source->nilnode.prev;

    source->nilnode.prev = first->prev;
    first->prev->next = &source->nilnode;

    last->next = &dest->nilnode;
    first->prev = dest->nilnode.prev;
    dest->nilnode.prev->next = first;
    dest->nilnode.prev = last;

    while (first != last) {
	first = first->next;
	moved++;
    }

    /* assert no overflows */
    TA_ASSERT_NO_RET( libHandle,source->nodecount - moved <= source->nodecount);
    TA_ASSERT_NO_RET( libHandle,dest->nodecount + moved >= dest->nodecount);

    /* assert no weirdness */
    TA_ASSERT_NO_RET( libHandle,moved <= source->nodecount);

    source->nodecount -= moved;
    dest->nodecount += moved;

    /* assert list sanity */
    TA_ASSERT_NO_RET( libHandle,list_verify(libHandle,source));
    TA_ASSERT_NO_RET( libHandle,list_verify(libHandle,dest));
}

void list_merge(TA_Libc *libHandle, list_t *dest, list_t *sour,
	int compare (const void *, const void *))
{
    lnode_t *dn, *sn, *tn;
    lnode_t *d_nil = list_nil(dest), *s_nil = list_nil(sour);

    /* Nothing to do if source and destination list are the same. */
    if (dest == sour)
	return;

    TA_ASSERT_NO_RET( libHandle, compare != NULL );

    /* overflow check */
    TA_ASSERT_NO_RET( libHandle,list_count(sour) + list_count(dest) >= list_count(sour));

    /* lists must be sorted */
    TA_ASSERT_NO_RET( libHandle,list_is_sorted(libHandle,sour, compare));
    TA_ASSERT_NO_RET( libHandle,list_is_sorted(libHandle,dest, compare));

    dn = list_first_priv(dest);
    sn = list_first_priv(sour);

    while (dn != d_nil && sn != s_nil) {
	if (compare(lnode_get(dn), lnode_get(sn)) >= 0) {
	    tn = lnode_next(sn);
	    list_delete(libHandle,sour, sn);
	    list_ins_before(libHandle,dest, sn, dn);
	    sn = tn;
	} else {
	    dn = lnode_next(dn);
	}
    }

    if (dn != d_nil)
	return;

    if (sn != s_nil)
	list_transfer(libHandle,dest, sour, sn);
}

void kazlist_sort(TA_Libc *libHandle, list_t *list, int compare(const void *, const void *))
{
    list_t extra;
    listcount_t middle;
    lnode_t *node;

    TA_ASSERT_NO_RET( libHandle, compare != NULL );
    TA_ASSERT_NO_RET( libHandle, list != NULL );

    if (list_count(list) > 1) {
	middle = list_count(list) / 2;
	node = list_first_priv(list);

	list_init(libHandle,&extra, list_count(list) - middle);

	while (middle--)
	    node = lnode_next(node);

	list_transfer(libHandle,&extra, list, node);
	kazlist_sort(libHandle,list, compare);
	kazlist_sort(libHandle,&extra, compare);
	list_merge(libHandle,list, &extra, compare);
    }
    TA_ASSERT_NO_RET( libHandle,list_is_sorted(libHandle,list, compare));
}

lnode_t *list_find(TA_Libc *libHandle, list_t *list, const void *key, int compare(const void *, const void *))
{
    lnode_t *node;
    lnode_t *tmpNode;

    TA_ASSERT_RET( libHandle, list != NULL, (lnode_t *)NULL );
    TA_ASSERT_RET( libHandle, key != NULL, (lnode_t *)NULL );

    for (node = list_first_priv(list); node != list_nil(list); node = node->next) {
    tmpNode = lnode_get(node);
    if( (compare && (compare(tmpNode, key) == 0)) ||  
        ((void *)tmpNode == key) )
	    return node;
    }

    return (lnode_t *)NULL;
}

/*
 * Return 1 if the list is in sorted order, 0 otherwise
 */
int list_is_sorted(TA_Libc *libHandle, list_t *list, int compare(const void *, const void *))
{
    lnode_t *node, *next, *nil;

    TA_ASSERT_RET( libHandle, compare != NULL, 0 );
    TA_ASSERT_RET( libHandle, list != NULL, 0 );

    next = nil = list_nil(list);
    node = list_first_priv(list);

    if (node != nil)
	next = lnode_next(node);

    for (; next != nil; node = next, next = lnode_next(next)) {
	if (compare(lnode_get(node), lnode_get(next)) > 0)
	    return 0;
    }

    return 1;
}

/*
 * Get rid of macro functions definitions so they don't interfere
 * with the actual definitions
 */

#undef list_isempty
#undef list_isfull
#undef lnode_pool_isempty
#undef list_append
#undef list_prepend
#undef list_first
#undef list_last
#undef list_next
#undef list_prev
#undef list_count
#undef list_del_first
#undef list_del_last
#undef lnode_put
#undef lnode_get

/*
 * Return 1 if the list is empty, 0 otherwise
 */

int list_isempty(TA_Libc *libHandle, list_t *list)
{
   TA_ASSERT_RET( libHandle, list != NULL, 0 );
    
   return list->nodecount == 0;
}

/*
 * Return 1 if the list is full, 0 otherwise
 * Permitted only on bounded lists.
 */

int list_isfull(TA_Libc *libHandle, list_t *list)
{
   TA_ASSERT_RET( libHandle, list != NULL, 0 );

   return list->nodecount == list->maxcount;
}

/*
 * Check if the node pool is empty.
 */

int lnode_pool_isempty(TA_Libc *libHandle, lnodepool_t *pool)
{
   TA_ASSERT_RET( libHandle, pool != NULL, 0 );

   return (pool->fre == NULL);
}

/*
 * Add the given node at the end of the list
 */

void list_append(TA_Libc *libHandle, list_t *list, lnode_t *node)
{
   TA_ASSERT_NO_RET( libHandle, list != NULL );
   TA_ASSERT_NO_RET( libHandle, node != NULL );

   list_ins_before(libHandle,list, node, &list->nilnode);
}

/*
 * Add the given node at the beginning of the list.
 */

void list_prepend(TA_Libc *libHandle, list_t *list, lnode_t *node)
{
   TA_ASSERT_NO_RET( libHandle, list != NULL );
   TA_ASSERT_NO_RET( libHandle, node != NULL );

   list_ins_after(libHandle,list, node, &list->nilnode);
}

/*
 * Retrieve the first node of the list
 */

lnode_t *list_first(TA_Libc *libHandle, list_t *list)
{
   TA_ASSERT_RET( libHandle, list != NULL, (lnode_t *)NULL );

   if (list->nilnode.next == &list->nilnode)
	  return NULL;

   return list->nilnode.next;
}

/*
 * Retrieve the last node of the list
 */

lnode_t *list_last(TA_Libc *libHandle, list_t *list)
{
   TA_ASSERT_RET( libHandle, list != NULL, (lnode_t *)NULL );

   if (list->nilnode.prev == &list->nilnode)
	   return (lnode_t *)NULL;

   return list->nilnode.prev;
}

/*
 * Retrieve the count of nodes in the list
 */

listcount_t list_count(TA_Libc *libHandle, list_t *list)
{
   TA_ASSERT_RET( libHandle, list != NULL, 0 );

   return list->nodecount;
}

/*
 * Remove the first node from the list and return it.
 */

lnode_t *list_del_first(TA_Libc *libHandle, list_t *list)
{
   TA_ASSERT_RET( libHandle, list != NULL, (lnode_t *)NULL );

   return list_delete(libHandle,list, list->nilnode.next);
}

/*
 * Remove the last node from the list and return it.
 */

lnode_t *list_del_last(TA_Libc *libHandle, list_t *list)
{
   TA_ASSERT_RET( libHandle, list != NULL, (lnode_t *)NULL );

   return list_delete(libHandle,list, list->nilnode.prev);
}


/*
 * Associate a data item with the given node.
 */

void lnode_put(TA_Libc *libHandle, lnode_t *lnode, void *data)
{
   TA_ASSERT_NO_RET( libHandle, data != NULL );
   TA_ASSERT_NO_RET( libHandle, lnode != NULL );

   lnode->data = data;
}

/*
 * Retrieve the data item associated with the node.
 */

void *lnode_get(TA_Libc *libHandle, lnode_t *lnode)
{
   TA_ASSERT_RET( libHandle, lnode != NULL, NULL );

   return lnode->data;
}

/*
 * Retrieve the node's successor. If there is no successor,
 * NULL is returned.
 */

lnode_t *list_next(TA_Libc *libHandle, list_t *list, lnode_t *lnode)
{
   TA_ASSERT_RET( libHandle, lnode != NULL, (lnode_t *)NULL );
   TA_ASSERT_RET( libHandle, list != NULL, (lnode_t *)NULL );
   TA_ASSERT_RET( libHandle, list_contains(libHandle,list, lnode), (lnode_t *)NULL );

   if (lnode->next == list_nil(list))
	   return (lnode_t *)NULL;
   return lnode->next;
}

/*
 * Retrieve the node's predecessor. See comment for lnode_next().
 */

lnode_t *list_prev(TA_Libc *libHandle, list_t *list, lnode_t *lnode)
{
   TA_ASSERT_RET( libHandle, lnode != NULL, (lnode_t *)NULL );
   TA_ASSERT_RET( libHandle, list != NULL, (lnode_t *)NULL );
   TA_ASSERT_RET( libHandle,list_contains(libHandle, list, lnode), (lnode_t *)NULL );

   if (lnode->prev == list_nil(list))
	   return (lnode_t *)NULL;
   return lnode->prev;
}

/*
 * Return 1 if the lnode is in some list, otherwise return 0.
 */

int lnode_is_in_a_list(TA_Libc *libHandle, lnode_t *lnode)
{
   TA_ASSERT_RET( libHandle, lnode != NULL, 0 );

   return (lnode->next != NULL || lnode->prev != NULL);
}


int list_verify(TA_Libc *libHandle, list_t *list)
{
   lnode_t *node, *nil;
   listcount_t count;

   TA_ASSERT_RET( libHandle, list != NULL, 0 );
   node = list_first_priv(list);
   nil = list_nil(list);
   count = list_count(libHandle, list);

   if (node->prev != nil)
      return 0;

   if (count > list->maxcount)
      return 0;

   while (node != nil && count--) {
	   if (node->next->prev != node)
         return 0;
	   node = node->next;
   }

   if (count != 0 || node != nil)
      return 0;

   return 1;
}

#ifdef KAZLIB_TEST_MAIN

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef char input_t[256];

static int tokenize(char *string, ...)
{
    char **tokptr;
    va_list arglist;
    int tokcount = 0;

    va_start(arglist, string);
    tokptr = va_arg(arglist, char **);
    while (tokptr) {
	while (*string && isspace((unsigned char) *string))
	    string++;
	if (!*string)
	    break;
	*tokptr = string;
	while (*string && !isspace((unsigned char) *string))
	    string++;
	tokptr = va_arg(arglist, char **);
	tokcount++;
	if (!*string)
	    break;
	*string++ = 0;
    }
    va_end(arglist);

    return tokcount;
}

static int comparef(const void *key1, const void *key2)
{
   TA_ASSERT_RET( libHandle, key1 != NULL, 0 );
   TA_ASSERT_RET( libHandle, key2 != NULL, 0 );

   return strcmp(key1, key2);
}

static char *dupstring(char *str)
{
    int sz = strlen(str) + 1;
    char *new = TA_Malloc( libHandle, sz);
    if (new)
	memcpy(new, str, sz);
    return new;
}

int main(void)
{
    input_t in;
    list_t *l = list_create(LISTCOUNT_T_MAX);
    lnode_t *ln;
    char *tok1, *val;
    int prompt = 0;

    char *help =
	"a <val>                append value to list\n"
	"d <val>                delete value from list\n"
	"l <val>                lookup value in list\n"
	"s                      sort list\n"
	"c                      show number of entries\n"
	"t                      dump whole list\n"
	"p                      turn prompt on\n"
	"q                      quit";

    if (!l)
	puts("list_create failed");

    for (;;) {
	if (prompt)
	    putchar('>');
	fflush(stdout);

	if (!fgets(in, sizeof(input_t), stdin))
	    break;

	switch(in[0]) {
	    case '?':
		puts(help);
		break;
	    case 'a':
		if (tokenize(in+1, &tok1, (char **) 0) != 1) {
		    puts("what?");
		    break;
		}
		val = dupstring(tok1);
		ln = lnode_create(val);

		if (!val || !ln) {
		    puts("allocation failure");
		    if (ln)
			lnode_destroy(ln);
		    TA_Free( libHandle, val);
		    break;
		}

		list_append(l, ln);
		break;
	    case 'd':
		if (tokenize(in+1, &tok1, (char **) 0) != 1) {
		    puts("what?");
		    break;
		}
		ln = list_find(l, tok1, comparef);
		if (!ln) {
		    puts("list_find failed");
		    break;
		}
		list_delete(l, ln);
		val = lnode_get(ln);
		lnode_destroy(ln);
		TA_Free( libHandle, val);
		break;
	    case 'l':
		if (tokenize(in+1, &tok1, (char **) 0) != 1) {
		    puts("what?");
		    break;
		}
		ln = list_find(l, tok1, comparef);
		if (!ln)
		    puts("list_find failed");
		else
		    puts("found");
		break;
	    case 's':
		kazlist_sort(l, comparef);
		break;
	    case 'c':
		printf("%lu\n", (unsigned long) list_count(l));
		break;
	    case 't':
		for (ln = list_first(l); ln != 0; ln = list_next(l, ln))
		    puts(lnode_get(ln));
		break;
	    case 'q':
		exit(0);
		break;
	    case '\0':
		break;
	    case 'p':
		prompt = 1;
		break;
	    default:
		putchar('?');
		putchar('\n');
		break;
	}
    }

    return 0;
}

#endif	/* defined TEST_MAIN */
