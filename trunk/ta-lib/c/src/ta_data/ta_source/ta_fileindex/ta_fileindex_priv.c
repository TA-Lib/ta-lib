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
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  110199 MF   First version.
 *
 */

/* Description:
 *    Allocate/de-allocate a TA_FileIndexPriv.
 *    - Provides functions to add values/data to the TA_FileIndexPriv.
 */

/**** Headers ****/
#include <string.h>
#include "ta_data.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_fileindex_priv.h"
#include "ta_global.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions.    ****/


/* functions for freeing lists of elements. */
static TA_RetCode freeListAndElement( TA_List *list,
                                      TA_RetCode (*freeFunc)( void *toBeFreed ));
static TA_RetCode freeFileIndexPriv( void *toBeFreed );
static TA_RetCode freeCategoryData( void *toBeFreed );
static TA_RetCode freeSymbolData( void *toBeFreed );
static TA_RetCode freeTokenInfo( void *toBeFreed );

static TA_ValueTreeNode *allocTreeNode( TA_ValueTreeNode *parent, TA_String *string );
static TA_RetCode freeTreeNode( void *toBeFreed );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_FileIndexPriv *TA_FileIndexPrivAlloc( TA_String *initialCategory,
                                         TA_String *initialCategoryCountry,
                                         TA_String *initialCategoryExchange,
                                         TA_String *initialCategoryType )
{
   TA_FileIndexPriv *fileIndexPrivData;
   TA_StringCache *stringCache;

   stringCache = TA_GetGlobalStringCache();

   /* Initialize the TA_FileIndexPriv element. */
   fileIndexPrivData = (TA_FileIndexPriv *)TA_Malloc( sizeof( TA_FileIndexPriv ) );

   if( !fileIndexPrivData )
      return NULL;

   /* initialize all fields to NULL. */
   memset( fileIndexPrivData, 0, sizeof( TA_FileIndexPriv ) );

   /* Now attempt to allocate all sub-elements. */
   stringCache = TA_GetGlobalStringCache();
   fileIndexPrivData->initialCategoryString         = TA_StringDup( stringCache, initialCategory );
   fileIndexPrivData->initialCategoryCountryString  = TA_StringDup( stringCache, initialCategoryCountry);
   fileIndexPrivData->initialCategoryExchangeString = TA_StringDup( stringCache, initialCategoryExchange );
   fileIndexPrivData->initialCategoryTypeString     = TA_StringDup( stringCache, initialCategoryType );

   if( (fileIndexPrivData->initialCategoryString         == NULL) ||
       (fileIndexPrivData->initialCategoryCountryString  == NULL) ||
       (fileIndexPrivData->initialCategoryExchangeString == NULL) ||
       (fileIndexPrivData->initialCategoryTypeString     == NULL) )
   {
      freeFileIndexPriv( (void *)fileIndexPrivData );
      return NULL;
   }

   fileIndexPrivData->scratchPad = (char *)TA_Malloc( TA_SOURCELOCATION_MAX_LENGTH+2 );
   if( !fileIndexPrivData->scratchPad )
   {
      freeFileIndexPriv( (void *)fileIndexPrivData );
      return NULL;
   }

   fileIndexPrivData->listLocationToken = TA_ListAlloc();
   if( !fileIndexPrivData->listLocationToken )
   {
      freeFileIndexPriv( (void *)fileIndexPrivData );
      return NULL;
   }

   fileIndexPrivData->listCategory = TA_ListAlloc();
   if( !fileIndexPrivData->listCategory )
   {
      freeFileIndexPriv( (void *)fileIndexPrivData );
      return NULL;
   }

   fileIndexPrivData->root = allocTreeNode( NULL, NULL );
   if( !fileIndexPrivData->root )
   {
      freeFileIndexPriv( (void *)fileIndexPrivData );
      return NULL;
   }

   fileIndexPrivData->currentNode = fileIndexPrivData->root;

   fileIndexPrivData->wildOneChar = TA_StringAlloc( stringCache, "?" );
   fileIndexPrivData->wildZeroOrMoreChar = TA_StringAlloc( stringCache, "*" );
   fileIndexPrivData->wildOneOrMoreChar = TA_StringAlloc( stringCache, "?*" );

   if( (!fileIndexPrivData->wildOneChar) ||
       (!fileIndexPrivData->wildZeroOrMoreChar) ||
       (!fileIndexPrivData->wildOneOrMoreChar) )
   {
      freeFileIndexPriv( (void *)fileIndexPrivData );
      return NULL;
   }

   return fileIndexPrivData;
}

TA_RetCode TA_FileIndexPrivFree( TA_FileIndexPriv *toBeFreed )
{
   return freeFileIndexPriv( toBeFreed );
}

TA_RetCode TA_FileIndexAddCategoryData( TA_FileIndexPriv *data,
                                        TA_String *stringCategory,
                                        TA_FileIndexCategoryData **added )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_FileIndexCategoryData *categoryData;
   unsigned int tmpInt;
   unsigned int categoryFound; /* Boolean */
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN(  TA_FileIndexAddCategoryData );

   stringCache = TA_GetGlobalStringCache();

   TA_ASSERT( data != NULL );
   TA_ASSERT( stringCategory != NULL );

   /* Trap the case where the category is already added. */
   categoryData = (TA_FileIndexCategoryData *)TA_ListAccessTail( data->listCategory );

   categoryFound = 0;
   while( categoryData && !categoryFound )
   {
      TA_ASSERT( categoryData->string != NULL );

      tmpInt = strcmp( TA_StringToChar( stringCategory ),
                       TA_StringToChar( categoryData->string ) );
      if( tmpInt == 0 )
         categoryFound = 1;
      else
         categoryData = (TA_FileIndexCategoryData *)TA_ListAccessPrev( data->listCategory );
   }

   if( !categoryFound )
   {
      /* This is a new category, so allocate the TA_FileIndexCategoryData */
      categoryData = (TA_FileIndexCategoryData *)TA_Malloc( sizeof(TA_FileIndexCategoryData) );

      if( !categoryData )
         TA_TRACE_RETURN( TA_ALLOC_ERR );

      /* Initialize the TA_FileIndexCategoryData */
      categoryData->parent = data;
      if( stringCategory )
      {
         categoryData->string = TA_StringDup( stringCache, stringCategory);    /* String for this category. Can be NULL. */
         if( !categoryData->string )
         {
            TA_Free(  categoryData );
            TA_TRACE_RETURN( TA_ALLOC_ERR );
         }
      }
      else
         categoryData->string = NULL;

      categoryData->listSymbol = TA_ListAlloc();

      if( !categoryData->listSymbol )
      {
         if( categoryData->string )
            TA_StringFree( stringCache, categoryData->string );
         TA_Free(  categoryData );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      /* Add it to the TA_FileIndexPriv */
      retCode = TA_ListAddTail( data->listCategory, categoryData );
      if( retCode != TA_SUCCESS )
      {
         TA_ListFree( categoryData->listSymbol );
         if( categoryData->string )
            TA_StringFree( stringCache, categoryData->string );
         TA_Free(  categoryData );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      #if 0
         printf( "**ADDING CATEGORY[%s]\n", TA_StringToChar( categoryData->string ) );
      #endif
   }

   /* Return the address of the object representing that category. */
   if( added )
      *added = categoryData;


   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_FileIndexAddTokenInfo( TA_FileIndexPriv *data,
                                     TA_TokenId id,
                                     TA_String *value,
                                     TA_TokenInfo *optBefore )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_TokenInfo *info;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN(  TA_FileIndexAddTokenInfo );

   stringCache = TA_GetGlobalStringCache();

   info = TA_Malloc( sizeof( TA_TokenInfo ) );

   if( !info )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   info->id = id;
   if( value == NULL )
      info->value = NULL;
   else
   {
      info->value = TA_StringDup( stringCache, value );

      if( !info->value )
      {
         TA_Free(  info );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }
   }

   if( optBefore )
      retCode = TA_ListAddBefore( data->listLocationToken, optBefore, info );
   else
      retCode = TA_ListAddTail( data->listLocationToken, info );

    if( retCode != TA_SUCCESS )
    {
      if( info->value )
         TA_StringFree( stringCache, info->value );
      TA_Free( info );
      TA_TRACE_RETURN( retCode );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_FileIndexAddSymbolData( TA_FileIndexCategoryData *categoryData,
                                      TA_String *stringSymbol,
                                      TA_ValueTreeNode *treeNodeValue,
                                      TA_FileIndexSymbolData **added )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_FileIndexSymbolData *symbolData;
   unsigned int tmpInt;
   unsigned int symbolFound; /* Boolean */
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN(  TA_FileIndexAddSymbolData );

   stringCache = TA_GetGlobalStringCache();

   TA_ASSERT( categoryData != NULL );
   TA_ASSERT( categoryData->listSymbol != NULL );
   TA_ASSERT( stringSymbol != NULL );
   TA_ASSERT( treeNodeValue != NULL );

   if( added )
      *added = NULL;

   /* Trap the case where this symbol is already there for that
    * category. In that case, the information is ignored.
    * Under the same category, for the same datasource, only one file
    * is supported for a category-symbol pair.
    */
   symbolData = (TA_FileIndexSymbolData *)TA_ListAccessTail( categoryData->listSymbol );

   symbolFound = 0;
   while( symbolData && !symbolFound )
   {
      TA_ASSERT( symbolData->string != NULL );

      tmpInt = strcmp( TA_StringToChar( stringSymbol ),
                       TA_StringToChar( symbolData->string ) );
      if( tmpInt == 0 )
         symbolFound = 1;
      else
         symbolData = (TA_FileIndexSymbolData *)TA_ListAccessPrev( categoryData->listSymbol );
   }

   if( !symbolFound )
   {
      /* This is a new symbol, so allocate the TA_FileIndexSymbolData */
      symbolData = (TA_FileIndexSymbolData *)TA_Malloc( sizeof(TA_FileIndexSymbolData) );

      if( !symbolData )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      /* Initialize the TA_FileIndexSymbolData */
      symbolData->parent = categoryData;
      symbolData->node   = treeNodeValue;
      symbolData->string = TA_StringDup( stringCache, stringSymbol );
      if( !symbolData->string )
      {
         TA_Free(  symbolData );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      /* Add it to the category list. */
      retCode = TA_ListAddTail( categoryData->listSymbol, symbolData );
      if( retCode != TA_SUCCESS )
      {
         TA_StringFree( stringCache, symbolData->string );
         TA_Free( symbolData );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }
   }

   /* Return the address of the object representing that symbol. */
   if( added )
      *added = symbolData;


   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_FileIndexSetCurrentTreeValueNode( TA_FileIndexPriv *data, TA_ValueTreeNode *node )
{
   data->currentNode = node;
   return TA_SUCCESS;
}

TA_ValueTreeNode *TA_FileIndexGetCurrentTreeValueNode( TA_FileIndexPriv *data )
{
   return data->currentNode;
}

TA_RetCode TA_FileIndexFreeValueTree( TA_ValueTreeNode *fromNode )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_ValueTreeNode *tmp;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN( TA_FileIndexFreeValueTree );

   TA_ASSERT( fromNode != NULL );

   stringCache = TA_GetGlobalStringCache();

   /* Remove itself from parent->child list if parent still around. */
   if( fromNode->parent )
   {
      tmp = ((TA_ValueTreeNode *)fromNode->parent);
      if( tmp->child )
         TA_ListRemoveEntry( tmp->child, (void *)fromNode );
   }

   if( fromNode->string )
      TA_StringFree( stringCache, fromNode->string );

   /* Deletes all childs. */
   if( fromNode->child )
   {
      retCode = freeListAndElement( fromNode->child, freeTreeNode );
      if( retCode != TA_SUCCESS )
         TA_TRACE_RETURN( retCode );
   }

   TA_Free( fromNode );

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_FileIndexAddTreeValue( TA_FileIndexPriv *data,
                                     TA_String *string,
                                     TA_ValueTreeNode **added )
{
   TA_PROLOG
   TA_ValueTreeNode *node;
   unsigned int allocateEmptyString;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN( TA_FileIndexAddTreeValue );

   stringCache = TA_GetGlobalStringCache();

   allocateEmptyString = 0;
   TA_ASSERT( data != NULL );

   if( added )
      *added = NULL;

   if( !string )
   {
      string = TA_StringAlloc( stringCache, "" );
      if( !string )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }
      allocateEmptyString = 1;
   }

   /* Alloc the TA_ValueTreeNode */
   node = allocTreeNode( data->currentNode, string );

   if( allocateEmptyString )
      TA_StringFree( stringCache, string );

   if( !node )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   data->currentNode = node;

   if( added )
      *added = node;

   TA_TRACE_RETURN( TA_SUCCESS );
}

/* Allows to change the value associated to a TA_ValueTreeNode. */
TA_RetCode TA_FileIndexChangeValueTreeNodeValue( TA_ValueTreeNode *nodeToChange,
                                                 TA_String *newValue )
{
   TA_PROLOG
   TA_String *dup;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN(  TA_FileIndexChangeValueTreeNodeValue );

   stringCache = TA_GetGlobalStringCache();

   if( !nodeToChange )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   if( nodeToChange->string )
      TA_StringFree( stringCache, nodeToChange->string );

   if( !newValue )
      nodeToChange->string = NULL;
   else
   {
      dup = TA_StringDup( stringCache, newValue );
      if( !dup )
      {
         nodeToChange->string = NULL;
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }
      else
         nodeToChange->string = dup;
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

/* Two very limited function to walk up/down in the Tree.
 * These functions are useful only when walking on a known linear portion
 * of the tree (when for each parent there is only one child).
 */
TA_ValueTreeNode *TA_FileIndexGoDownTreeValue( TA_FileIndexPriv *data )
{
   TA_ValueTreeNode *retValue;

   TA_ASSERT_RET( data != NULL, (TA_ValueTreeNode *)NULL );

   /* Go down using the first child only. */
   if( (!data->currentNode) || (!data->currentNode->child) )
      return NULL;

   retValue = TA_ListAccessHead( data->currentNode->child );

   /* Change data->currentNode only if we really can go down... */
   if( retValue )
      data->currentNode = retValue;

   return retValue;
}

TA_ValueTreeNode *TA_FileIndexGoUpTreeValue( TA_FileIndexPriv *data )
{
   TA_ValueTreeNode *retValue;

   TA_ASSERT_RET( data != NULL, (TA_ValueTreeNode *)NULL );

   if( !data->currentNode )
      return (TA_ValueTreeNode *)NULL;

   retValue = data->currentNode->parent;

   /* Change data->currentNode only if we can really go up... */
   if( retValue  )
      data->currentNode = retValue;

   return retValue;
}

/**** Local functions definitions.     ****/
static TA_ValueTreeNode *allocTreeNode( TA_ValueTreeNode *parent,
                                        TA_String *string )
{
   TA_ValueTreeNode *node;
   TA_RetCode retCode;
   TA_StringCache *stringCache;

   stringCache = TA_GetGlobalStringCache();

   node = (TA_ValueTreeNode *)TA_Malloc( sizeof( TA_ValueTreeNode ) );

   if( !node )
      return (TA_ValueTreeNode *)NULL;

   node->string = NULL;
   node->parent = NULL;

   node->child = TA_ListAlloc();
   if( !node->child )
   {
      freeTreeNode( node );
      return NULL;
   }

   if( string )
   {
      node->string = TA_StringDup( stringCache, string );
      if( !node->string )
      {
         freeTreeNode( node );
         return NULL;
      }
   }

   if( parent )
   {
      retCode = TA_ListAddTail( parent->child, node );
      if( retCode != TA_SUCCESS )
      {
         freeTreeNode( node );
         return NULL;
      }
      node->parent = parent;
   }

   return node;
}

static TA_RetCode freeFileIndexPriv( void *toBeFreed )
{
   TA_PROLOG
   TA_FileIndexPriv *asciiFileData;
   TA_StringCache   *stringCache;

   asciiFileData = (TA_FileIndexPriv *)toBeFreed;

   TA_TRACE_BEGIN(  freeFileIndexPriv );

   stringCache = TA_GetGlobalStringCache();

   if( !asciiFileData )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   if( freeListAndElement( asciiFileData->listLocationToken, freeTokenInfo ) != TA_SUCCESS )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   if( freeListAndElement( asciiFileData->listCategory, freeCategoryData ) != TA_SUCCESS )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   if( asciiFileData->root )
      TA_FileIndexFreeValueTree( asciiFileData->root );

   if( asciiFileData->scratchPad )
      TA_Free(  asciiFileData->scratchPad );

   if( asciiFileData->wildOneChar )
      TA_StringFree( stringCache, asciiFileData->wildOneChar );

   if( asciiFileData->wildZeroOrMoreChar )
      TA_StringFree( stringCache, asciiFileData->wildZeroOrMoreChar );

   if( asciiFileData->wildOneOrMoreChar )
      TA_StringFree( stringCache, asciiFileData->wildOneOrMoreChar );

   if( asciiFileData->initialCategoryString )
      TA_StringFree( stringCache, asciiFileData->initialCategoryString );

   if( asciiFileData->initialCategoryCountryString )
      TA_StringFree( stringCache, asciiFileData->initialCategoryCountryString );

   if( asciiFileData->initialCategoryExchangeString )
      TA_StringFree( stringCache, asciiFileData->initialCategoryExchangeString );

   if( asciiFileData->initialCategoryTypeString )
      TA_StringFree( stringCache, asciiFileData->initialCategoryTypeString );

   TA_Free( asciiFileData );

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeTreeNode( void *toBeFreed )
{
   /* Free this node and all descendant.
    * Force to ignore the remove of itself in the parent->child list
    * by setting parent to NULL.
    * That means that the caller will clean-up itself the
    * parent list.
     */
   TA_ValueTreeNode *node = (TA_ValueTreeNode *)toBeFreed;
   node->parent = NULL;
   return TA_FileIndexFreeValueTree( node );
}

static TA_RetCode freeTokenInfo( void *toBeFreed )
{
   TA_TokenInfo *info;
   TA_StringCache *stringCache;

   stringCache = TA_GetGlobalStringCache();

   info = (TA_TokenInfo *)toBeFreed;

   if( !info )
      return TA_ALLOC_ERR;

   if( info->value )
      TA_StringFree( stringCache, info->value );

   TA_Free(  info );

   return TA_SUCCESS;
}

static TA_RetCode freeCategoryData( void *toBeFreed )
{
   TA_FileIndexCategoryData *categoryData;
   TA_StringCache *stringCache;

   stringCache = TA_GetGlobalStringCache();

   categoryData = (TA_FileIndexCategoryData *)toBeFreed;

   if( !categoryData )
      return TA_ALLOC_ERR;

   if( categoryData->string )
      TA_StringFree( stringCache, categoryData->string );

   if( freeListAndElement( categoryData->listSymbol, freeSymbolData ) != TA_SUCCESS )
      return TA_ALLOC_ERR;

   TA_Free( categoryData );

   return TA_SUCCESS;
}

static TA_RetCode freeSymbolData( void *toBeFreed )
{
   TA_FileIndexSymbolData *symbolData;
   TA_StringCache *stringCache;

   stringCache = TA_GetGlobalStringCache();

   symbolData = (TA_FileIndexSymbolData *)toBeFreed;

   if( !symbolData )
      return TA_ALLOC_ERR;

   /* Not yet im[plemented !!!
   if( symbolData->dataPerDate )
   {
      if( freeListAndElement( symbolData->dataPerDate, freeDataPerDate ) != TA_SUCCESS )
         return TA_ALLOC_ERR;
   }
   */

   if( symbolData->string )
      TA_StringFree( stringCache, symbolData->string );

   TA_Free( symbolData );

   return TA_SUCCESS;
}

static TA_RetCode freeListAndElement( TA_List *list, TA_RetCode (*freeFunc)( void *toBeFreed ) )
{
   TA_PROLOG
   TA_RetCode retCode;
   void *node;

   TA_TRACE_BEGIN( freeListAndElement );

   if( list != NULL )
   {
      while( (node = TA_ListRemoveTail( list )) != NULL )
      {
         retCode = freeFunc(  node );
         if( retCode != TA_SUCCESS )
         {
            TA_FATAL(  NULL, node, retCode );
            /* TA_TRACE_RETURN( TA_ALLOC_ERR ); */
         }
      }

      retCode = TA_ListFree( list );
      if( retCode != TA_SUCCESS )
      {
         TA_FATAL(  NULL, list, retCode );
         /*TA_TRACE_RETURN( TA_ALLOC_ERR );*/
      }
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

