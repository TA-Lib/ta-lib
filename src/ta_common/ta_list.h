#ifndef TA_LIST_H
#define TA_LIST_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#include "kazlib/kaz_list.h"

/* This module implements an efficient list container.
 *
 * This implementation is just a list of opaque
 * pointers (void *). All value for the (void *) are
 * valid, except NULL.
 *
 * The list DOES NOT own its elements. So freeing the list
 * does not free the elements the (void *) are pointing to
 * (except if you use TA_ListFreeAll).
 *
 * The implementation is simplified by not having to manipulate 
 * 'node' structure for the elements in the list.
 *
 * This module offer the following operations:
 *        - Create/Destroy a list.
 *        - Append/Prepend a (void *) to the list.
 *        - Remove first/last (void *) in the list.
 *        - Remove a particular (void *) in the list.
 *        - Sequential access to the list.
 *        - Return the size of a list.
 *
 * Note: This module is not multithread safe, and the user must provide
 *       adequate protection in a multithread environment.
 *
 *
 * Speed Optimization with pre-allocated TA_List/TA_ListNode
 * ---------------------------------------------------------
 * It is possible for the caller to provide pre-allocated
 * TA_List and/or TA_ListNode.
 *
 * A TA_List can be pre-allocated, and the elements can be dynamic
 * (and vice-versa). The only limitation is if you start to use
 * pre-allocated TA_ListNode, all elements must be pre-allocated, 
 * else TA_INVALID_LIST_TYPE will be returned.
 * In the same way, as soon you add a dynamic element, no pre-allocated
 * element can be added to the same list from that point. All this can
 * be summarize by the following limitation rule:
 *   "For a given list, the TA_ListAddXXXX() and
 *    TA_ListNodeAddXXXX() functions cannot be mixed."
 *
 */
typedef struct
{
   /* Hidden implementation. Never touch these. Never. */
   list_t d;
   lnode_t *node;
   TA_Libc *libHandle;   
   unsigned int flags;
} TA_List;

typedef struct
{
   /* TA_ListNode is useful only if you wish to prevent
    * the TA_List module to do any memory allocation.
    */

   /* Hidden implementation. Never touch these. Never. */
   lnode_t node;
} TA_ListNode;

/* Create an empty list.
 *
 * Return: An handle used to manipulate the list or NULL if
 *         there is an allocation error.
 */
TA_List *TA_ListAlloc( TA_Libc *libHandle );

/* Free a list.
 *
 * Basically, all internal ressource to maintain the
 * TA_List are freed. Remember that the list does not
 * own its element. To free the element, the function
 * TA_ListFreeAll can make it convenient for the user
 * by calling a user provided "free" function for all
 * the elements.
 *
 * This function will not attempt to free a pre-allocated
 * TA_List (as expected). The list will be emptied though.
 * 
 * This call on a TA_List using pre-allocated
 * TA_ListNode will correctly free the list, but
 * will leave the TA_ListNode untouch (as expected).
 *
 * At this point, you probably understand that if
 * you call TA_ListFree on a pre-allocated TA_List
 * using pre-allocated TA_ListNode, nothing will be
 * done (nothing needs to be freed!).
 *
 * Return: TA_SUCCESS or TA_BAD_PARAM.
 */
TA_RetCode TA_ListFree( TA_List *list );

/* Free all elements in the list and the list itself.
 *
 * The provided 'freeFunc' will be called for each element
 * before releasing all internal ressources.
 *
 * No need to call TA_ListFree after this call.
 *
 * In other word, this function will first ALWAYS call
 * the provided function with all the (void *) currenlty 
 * in the list and then do the equivalent of a TA_ListFree.
 *
 * Return: TA_SUCCESS or TA_BAD_PARAM.
 */
TA_RetCode TA_ListFreeAll( TA_List *list,
                           TA_RetCode (*freeFunc)( TA_Libc *libHandle, void *toBeFreed ));

/* Initialize a pre-allocated list. 
 *
 * Example:
 *      TA_List myList;
 *      TA_ListInit( libHandle, &myList );
 *      'myList' can be used from this point.
 *
 * This function can replace the slower TA_ListAlloc if
 * the application has already a TA_List allocated.
 */
void TA_ListInit( TA_Libc *libHandle, TA_List *list );

/* Add a (void *) to the list.
 *
 * Return: TA_SUCCESS, TA_BAD_PARAM, TA_INVALID_LIST_TYPE or TA_ALLOC_ERR.
 *
 * Following this call, the TA_ListNodeAddXXXX() functions cannot be called.
 */
TA_RetCode TA_ListAddTail( TA_List *list, void *newElement );
TA_RetCode TA_ListAddHead( TA_List *list, void *newElement );

TA_RetCode TA_ListAddBefore( TA_List *list, void *element, void *newElement );
TA_RetCode TA_ListAddAfter ( TA_List *list, void *element, void *newElement );

/* Add a (void *) to the list.
 *
 * Return: TA_SUCCESS, TA_BAD_PARAM, TA_INVALID_LIST_TYPE or TA_ALLOC_ERR.
 *
 * Following this call, the TA_ListAddXXXX() functions cannot be called.
 */
TA_RetCode TA_ListNodeAddTail( TA_List *list, TA_ListNode *node, void *newElement );
TA_RetCode TA_ListNodeAddHead( TA_List *list, TA_ListNode *node, void *newElement );

#if 0
!!! Not yet implemented
TA_RetCode TA_ListNodeAddBefore( TA_List *list, TA_ListNode *node, void *element, void *newElement );
TA_RetCode TA_ListNodeAddAfter ( TA_List *list, TA_ListNode *node, void *element, void *newElement );
#endif

/* Remove a (void *) from the list.
 *
 * Return: The removed (void *) or NULL if empty list.
 */
void *TA_ListRemoveTail( TA_List *list );
void *TA_ListRemoveHead( TA_List *list );

/* Remove a particular (void *) from the list.
 *
 * Return: TA_SUCCESS or TA_BAD_PARAM.
 */
TA_RetCode TA_ListRemoveEntry( TA_List *list, void *elementToRemove );

/* Return the number of element in the list. */
unsigned int TA_ListSize( TA_List *list );

/* The built-in iterator
 *
 * This is a very simplified iterator for accessing sequentially (only)
 * a TA_List.
 *
 * Note: This is a speed efficient/simplified implementation of an iterator.
 *       Since this simplified iterator is embedded within the TA_List, it must
 *       be used only by one thread (context). For using multiple iterator
 *       on the same list, use the TA_ListIterXXXXX functions.
 *
 * Here is a code snippet printing all the content of 'list':
 * (Assuming the 'void *' in the 'list' are pointer to string).
 *
 * void printContent( TA_List *list )
 * {
 *    char *element;
 *
 *    element = TA_ListAccessHead( list );
 *
 *    if( element == NULL )
 *       printf( "List is empty!\n" );
 *    else
 *    {
 *       do
 *       {
 *          printf( "%s\n", (const char *)element );
 *          element = TA_ListAccessNext( list );
 *       } while( element );
 *    }
 * }
 */
void *TA_ListAccessHead( TA_List *list );
void *TA_ListAccessTail( TA_List *list );
void *TA_ListAccessNext( TA_List *list );
void *TA_ListAccessPrev( TA_List *list );

void *TA_ListAccessCurrent( TA_List *list );

/* Access the list using an independant iterator.
 *
 * This iterator allows sequential and random access to the list. Multiple
 * iterator can be used on the same TA_List.
 *
 * Random access can be achieved by keeping a copy of the actual position in
 * the list with the TA_ListIterSavePos, TA_ListIterRestorePos functions.
 * Multiple position can be indepedently saved. It is the responsibility of the
 * caller to make sure that the list is not modified while these saved position
 * are used! So keep in mind that these random access are working only with
 * un-modified TA_List.
 *
 * Note: The TA_ListIter functions can be used simultaneously and indepedently
 *       of the simpler TA_ListAccess functions.
 *
 * Example:
 * In this example, we print the content of a TA_List. For demo
 * purpose we re-print the first element by using the save/restore
 * capability (would be easier to simply call again TA_ListIterHead...).
 *
 * void printContent( TA_List *list )
 * {
 *    char *element;
 *    TA_ListIter iter;
 *    TA_ListIterPos iterPos;
 *
 *    TA_ListIterInit( &iter, list );
 *    element = TA_ListIterHead( &iter );
 *
 *    if( element == NULL )
 *       printf( "List is empty!\n" );
 *    else
 *    {
 *       TA_ListIterSavePos( &iter, &iterPos );
 *
 *       do
 *       {
 *          printf( "%s\n", element );
 *          element = TA_ListIterNext( &iter );
 *       } while( element );
 *
 *       element = TA_ListIterRestorePos( &iterPos );
 *       printf( "First element is %s", element );
 *    }
 * }
 */

typedef struct
{
  /* User should not access these fields directly. */
  TA_List *list;
  void *currentNode;
} TA_ListIter;

typedef struct
{
  /* User should not access these fields directly. */
  TA_ListIter *iter;
  void *savedNode;
} TA_ListIterPos;

TA_RetCode TA_ListIterInit( TA_ListIter *iter, TA_List *list );

void *TA_ListIterHead( TA_ListIter *iter );
void *TA_ListIterNext( TA_ListIter *iter );
void *TA_ListIterPrev( TA_ListIter *iter );
void *TA_ListIterTail( TA_ListIter *iter );
void *TA_ListIterCur ( TA_ListIter *iter );

TA_RetCode TA_ListIterSavePos   ( TA_ListIter *iter, TA_ListIterPos *iterPos );
void      *TA_ListIterRestorePos( TA_ListIterPos *iterPos );

/* Sort a list using the provided compare function:
 *    int compare(TA_Libc *, const void *, const void *)
 */
TA_RetCode TA_ListSort( TA_List *list, int compare(const void *, const void *) );


#endif

