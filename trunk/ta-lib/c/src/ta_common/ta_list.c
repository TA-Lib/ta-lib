/* TA-LIB Copyright (c) 1999-2003, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  110199 MF      First version.
 */

/**** Headers ****/
#include "ta_common.h"
#include "ta_list.h"
#include "kazlib/kaz_list.h"
#include "ta_memory.h"
#include "ta_trace.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/

/* The TA_List->flags can use the following boolean flags. */

/* The list come from TA_ListAlloc, else assume TA_ListInit */
#define TA_LIST_FLAGS_DYNAMIC      0x00000001 

/* Indicates if the user provides pre-allocated TA_ListNode or
 * a dynamic approach.
 * The distinction could be done only follwoing a TA_ListAddXXXXX() 
 * or TA_ListNodeAddXXXXX() functions.
 */
#define TA_LIST_FLAGS_PREALLOC_NODE 0x00000002
#define TA_LIST_FLAGS_DYNAMIC_NODE  0x00000004

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_List *TA_ListAlloc( void )
{
   TA_List *theList;

   /* Allocate the TA_List. */
   theList = (TA_List *)TA_Malloc( sizeof( TA_List ) );      
   if( !theList )
      return NULL;

   /* Initialize the content of the TA_List */
   TA_ListInit( theList );
   theList->flags = TA_LIST_FLAGS_DYNAMIC;

   return theList;
}

TA_RetCode TA_ListFree( TA_List *list )
{
   if( !list )
      return TA_SUCCESS; /* Assume already freed. */

   /* Destroy all the nodes if they were
    * dynamically allocated.
    */
   if( list->flags & TA_LIST_FLAGS_DYNAMIC_NODE )
      list_destroy_nodes( &list->d );

   /* Destroy the list if it was dynamically allocated. */
   if( list->flags & TA_LIST_FLAGS_DYNAMIC )
      TA_Free( list );

   return TA_SUCCESS;
}

/* Initialize a user pre-allocated TA_List.
 *
 * (Note: speed optimized, not as safe as TA_ListAlloc)
 *
 * Example:
 *      TA_List myList;
 *      TA_ListInit(  &myList );
 *      'myList' can be used from this point.
 */
void TA_ListInit( TA_List *list )
{
   list->node = NULL;
   list->flags = 0;

   list_init( &list->d, LISTCOUNT_T_MAX );
}

TA_RetCode TA_ListFreeAll( TA_List *list,
                           TA_RetCode (*freeFunc)( void *toBeFreed ))
{
   TA_PROLOG
   TA_RetCode retCode;
   void *node;

   TA_TRACE_BEGIN( TA_ListFreeAll );

   if( list != NULL )
   {
      while( (node = TA_ListRemoveTail( list )) != NULL )
      {
         retCode = freeFunc( node );
         if( retCode != TA_SUCCESS )
         {
            TA_FATAL( NULL, node, retCode );
         }
      }

      retCode = TA_ListFree( list );
      if( retCode != TA_SUCCESS )
      {
         TA_FATAL( NULL, list, retCode );
      }
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_ListAddTail( TA_List *list, void *newElement )
{
   lnode_t *node;

   if( list == NULL )
      return TA_BAD_PARAM;

   if( list->flags & TA_LIST_FLAGS_PREALLOC_NODE )
      return TA_INVALID_LIST_TYPE;

   if( !(list->flags & TA_LIST_FLAGS_DYNAMIC_NODE) )
      list->flags |= TA_LIST_FLAGS_DYNAMIC_NODE;

   /* Allocate node. */
   node = lnode_create( newElement );

   if( !node )
      return TA_ALLOC_ERR;

   list_append( &list->d, node );

   return TA_SUCCESS;
}

TA_RetCode TA_ListAddHead( TA_List *list, void *newElement )
{
   lnode_t *node;

   if( list == NULL )
      return TA_BAD_PARAM;

   if( list->flags & TA_LIST_FLAGS_PREALLOC_NODE )
      return TA_INVALID_LIST_TYPE;

   if( !(list->flags & TA_LIST_FLAGS_DYNAMIC_NODE) )
      list->flags |= TA_LIST_FLAGS_DYNAMIC_NODE;

   /* Allocate node. */
   node = lnode_create( newElement );

   if( !node )
      return TA_ALLOC_ERR;

   list_prepend( &list->d, node );

   return TA_SUCCESS;
}

TA_RetCode TA_ListAddBefore( TA_List *list, void *element, void *newElement )
{
   lnode_t *newNode;
   lnode_t *thisNode;

   if( list == NULL )
      return TA_BAD_PARAM;

   if( list->flags & TA_LIST_FLAGS_PREALLOC_NODE )
      return TA_INVALID_LIST_TYPE;

   if( !(list->flags & TA_LIST_FLAGS_DYNAMIC_NODE) )
      list->flags |= TA_LIST_FLAGS_DYNAMIC_NODE;

   /* Find 'element' node. */
   thisNode = list_find( &list->d, element, NULL );

   if( !thisNode )
      return TA_BAD_PARAM;

   /* Allocate node. */
   newNode = lnode_create( newElement );

   if( !newNode )
      return TA_ALLOC_ERR;

   list_ins_before( &list->d, newNode, thisNode );

   return TA_SUCCESS;
}

TA_RetCode TA_ListAddAfter( TA_List *list, void *element, void *newElement )
{
   lnode_t *newNode;
   lnode_t *thisNode;

   if( list == NULL )
      return TA_BAD_PARAM;

   if( list->flags & TA_LIST_FLAGS_PREALLOC_NODE )
      return TA_INVALID_LIST_TYPE;

   if( !(list->flags & TA_LIST_FLAGS_DYNAMIC_NODE) )
      list->flags |= TA_LIST_FLAGS_DYNAMIC_NODE;

   /* Find 'element' node. */
   thisNode = list_find( &list->d, element, NULL );

   if( !thisNode )
      return TA_BAD_PARAM;

   /* Allocate node. */
   newNode = lnode_create( newElement );

   if( !newNode )
      return TA_ALLOC_ERR;

   list_ins_after( &list->d, newNode, thisNode );

   return TA_SUCCESS;
}

TA_RetCode TA_ListNodeAddTail( TA_List *list, TA_ListNode *node, void *newElement )
{
   if( !list || !node )
      return TA_BAD_PARAM;

   if( list->flags & TA_LIST_FLAGS_DYNAMIC_NODE )
      return TA_INVALID_LIST_TYPE;

   if( !(list->flags & TA_LIST_FLAGS_PREALLOC_NODE) )
      list->flags |= TA_LIST_FLAGS_PREALLOC_NODE;

   lnode_init( &node->node, newElement );

   list_append( &list->d, &node->node );

   return TA_SUCCESS;
}

TA_RetCode TA_ListNodeAddHead( TA_List *list, TA_ListNode *node, void *newElement )
{
   if( !list || !node )
      return TA_BAD_PARAM;

   if( list->flags & TA_LIST_FLAGS_DYNAMIC_NODE )
      return TA_INVALID_LIST_TYPE;

   if( !(list->flags & TA_LIST_FLAGS_PREALLOC_NODE) )
      list->flags |= TA_LIST_FLAGS_PREALLOC_NODE;

   lnode_init( &node->node, newElement );

   list_prepend( &list->d, &node->node );

   return TA_SUCCESS;
}

TA_RetCode TA_ListNodeAddBefore( TA_List *list, TA_ListNode *node, void *element, void *newElement )
{
   lnode_t *newNode;
   lnode_t *thisNode;

   if( !list || !node )
      return TA_BAD_PARAM;

   if( list->flags & TA_LIST_FLAGS_DYNAMIC_NODE )
      return TA_INVALID_LIST_TYPE;

   if( !(list->flags & TA_LIST_FLAGS_PREALLOC_NODE) )
      list->flags |= TA_LIST_FLAGS_PREALLOC_NODE;

   /* Find 'element' node. */
   thisNode = list_find( &list->d, element, NULL );

   if( !thisNode )
      return TA_BAD_PARAM;

   /* Initialize new node. */
   newNode = &node->node;
   lnode_init( newNode, newElement );

   list_ins_before( &list->d, newNode, thisNode );

   return TA_SUCCESS;
}

TA_RetCode TA_ListNodeAddAfter( TA_List *list, TA_ListNode *node, void *element, void *newElement )
{
   lnode_t *newNode;
   lnode_t *thisNode;

   if( !list || !node )
      return TA_BAD_PARAM;

   if( list->flags & TA_LIST_FLAGS_DYNAMIC_NODE )
      return TA_INVALID_LIST_TYPE;

   if( !(list->flags & TA_LIST_FLAGS_PREALLOC_NODE) )
      list->flags |= TA_LIST_FLAGS_PREALLOC_NODE;

   /* Find 'element' node. */
   thisNode = list_find( &list->d, element, NULL );

   if( !thisNode )
      return TA_BAD_PARAM;

   /* Initialize new node. */
   newNode = &node->node;
   lnode_init( newNode, newElement );

   list_ins_after( &list->d, newNode, thisNode );

   return TA_SUCCESS;
}

void *TA_ListRemoveTail( TA_List *list )
{
   lnode_t *node;
   void *returnValue;

   if( (list == NULL)    ||
       list_isempty(&list->d) )
      return (void *)NULL;

   node = list_del_last( &list->d );

   returnValue = lnode_get( node );

   if( list->flags & TA_LIST_FLAGS_DYNAMIC_NODE )
      lnode_destroy( node );

   return returnValue;
}

void *TA_ListRemoveHead( TA_List *list )
{
   lnode_t *node;
   void *returnValue;

   if( (list == NULL)    ||
       list_isempty(&list->d) )
      return (void *)NULL;

   node = list_del_first( &list->d );

   returnValue = lnode_get( node );

   if( list->flags & TA_LIST_FLAGS_DYNAMIC_NODE )
      lnode_destroy( node );

   return returnValue;
}

TA_RetCode TA_ListRemoveEntry( TA_List *list, void *elementToRemove )
{
   lnode_t *node;

   if( (list == NULL)    ||
       list_isempty(&list->d) )
      return TA_BAD_PARAM;

   node = list_first( &list->d );

   while( node !=  NULL )
   {
      if( lnode_get( node ) == elementToRemove )
      {
         list_delete( &list->d, node );
         if( list->flags & TA_LIST_FLAGS_DYNAMIC_NODE )
            lnode_destroy( node );
         return TA_SUCCESS;
      }

      node = list_next( &list->d, node );
   }

   return TA_BAD_PARAM;
}

unsigned int TA_ListSize( TA_List *list )
{

   if( list == NULL )
      return 0;

   return list_count( &list->d );
}

void *TA_ListAccessHead( TA_List *list )
{
   if( (list == NULL)    ||
       list_isempty(&list->d) )
      return NULL;

   list->node = list_first( &list->d );

   if( list->node == NULL )
      return NULL;
   else
      return lnode_get( list->node );
}

void *TA_ListAccessTail( TA_List *list )
{

   if( (list == NULL)    ||
       list_isempty(&list->d) )
      return NULL;

   list->node = list_last( &list->d );

   if( list->node == NULL )
      return NULL;
   else
      return lnode_get( list->node );
}

void *TA_ListAccessNext( TA_List *list )
{
   if( (list == NULL) || 
       (list->node == NULL) )
      return NULL;

   list->node = list_next( &list->d, list->node );

   if( list->node == NULL )
      return NULL;
   else
      return lnode_get( list->node );
}

void *TA_ListAccessPrev( TA_List *list )
{
   if( (list == NULL) || (list->node == NULL) )
      return NULL;

   list->node = list_prev( &list->d, list->node );

   if( list->node == NULL )
      return NULL;
   else
      return lnode_get( list->node );
}

void *TA_ListAccessCurrent( TA_List *list )
{
   if( (list == NULL) || (list->node == NULL) )
      return NULL;

   return lnode_get( list->node );
}

TA_RetCode TA_ListIterInit( TA_ListIter *iter, TA_List *list )
{
   if( (!iter) || (!list) )
      return TA_BAD_PARAM;

   iter->list = list;
   iter->currentNode = TA_ListIterHead( iter );

   return TA_SUCCESS;
}

void *TA_ListIterHead( TA_ListIter *iter )
{
   lnode_t *node;
   TA_List *theList;

   if( !iter )
      return NULL;

   theList = iter->list;

   if( (!theList) || list_isempty(&theList->d) )
      return NULL;

   node = list_first( &theList->d );

   iter->currentNode = (void *)node;
   if( node == NULL )
      return NULL;
   else
      return lnode_get( node );
}

void *TA_ListIterNext( TA_ListIter *iter )
{
   lnode_t *node;
   TA_List *theList;

   if( (!iter) || (!iter->currentNode) )
      return NULL;

   theList = iter->list;

   if( !theList )
      return NULL;

   node = list_next( &theList->d, (lnode_t *)iter->currentNode );

   iter->currentNode = (void *)node;
   if( node == NULL )
      return NULL;
   else
      return lnode_get( node );
}

void *TA_ListIterPrev( TA_ListIter *iter )
{
   lnode_t *node;
   TA_List *theList;

   if( (!iter) || (!iter->currentNode) )
      return NULL;

   theList = iter->list;

   if( !theList )
      return NULL;

   node = list_prev( &theList->d, (lnode_t *)iter->currentNode );

   iter->currentNode = (void *)node;
   if( node == NULL )
      return NULL;
   else
      return lnode_get( node );
}

void *TA_ListIterTail( TA_ListIter *iter )
{
   lnode_t *node;
   TA_List *theList;

   theList = iter->list;

   if( (!theList) || (!iter) || list_isempty(&theList->d) )
      return NULL;

   node = list_last( &theList->d );

   iter->currentNode = (void *)node;
   if( node == NULL )
      return NULL;
   else
      return lnode_get( node );
}

void *TA_ListIterCur( TA_ListIter *iter )
{
   lnode_t *node;

   if( (!iter) || (!iter->currentNode) )
      return NULL;

   node = (lnode_t *)iter->currentNode;
   
   return lnode_get( node );
}

TA_RetCode TA_ListIterSavePos( TA_ListIter *iter, TA_ListIterPos *iterPos )
{
   if( (!iter) || (!iterPos) || (!iter->currentNode) )
      return TA_BAD_PARAM;

   iterPos->iter = iter;
   iterPos->savedNode = iter->currentNode;
   return TA_SUCCESS;
}

void *TA_ListIterRestorePos( TA_ListIterPos *iterPos )
{
   void *tmp;

   if( (!iterPos) || (!iterPos->savedNode) )
      return NULL;

   tmp = iterPos->savedNode;
   if( !tmp )
      return NULL;

   iterPos->iter->currentNode = tmp;
   return lnode_get( (lnode_t *)tmp );
}


TA_RetCode TA_ListSort( TA_List *list, int compare(const void *, const void *) )
{
   if( list == NULL )
      return TA_BAD_PARAM;

   kazlist_sort( &list->d, compare );
   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/
/* None */

