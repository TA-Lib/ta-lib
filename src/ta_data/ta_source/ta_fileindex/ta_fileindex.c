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
 *  070800 MF   First version.
 *
 */

/* Description:
 *
 */

/**** Headers ****/
#include <stddef.h>
#include <string.h>
#include "ta_fileindex.h"
#include "ta_fileindex_priv.h"
#include "ta_trace.h"
#include "ta_data.h"
#include "ta_global.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_FileIndexAlloc( TA_String *path,
                              TA_String *initialCategory,
                              TA_String *initialCategoryCountry,
                              TA_String *initialCategoryExchange,
                              TA_String *initialCategoryType,
                              TA_FileIndex **newIndex )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_FileIndexPriv *fileIndexPriv;
   /*!!! TA_String *categoryToUse;*/
   /*!!! TA_StringCache *stringCache;*/

   TA_TRACE_BEGIN(  TA_FileIndexAlloc );
    
   TA_ASSERT( newIndex != NULL );
   TA_ASSERT( path != NULL );

   *newIndex = NULL;


   /* Make sure a default category and is provided.
    * Category component default must also be provided.
    */
   if( !initialCategory         || 
       !initialCategoryCountry  || 
       !initialCategoryExchange ||
       !initialCategoryType )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   /* Allocate the "TA_FileIndex" alias "TA_FileIndexPriv". */
   fileIndexPriv = TA_FileIndexPrivAlloc( 
                                          initialCategory,
                                          initialCategoryCountry,
                                          initialCategoryExchange,
                                          initialCategoryType );

   if( !fileIndexPriv )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   #if 0
   !!! Not needed!?
   stringCache = TA_GetGlobalStringCache(); 
   TA_ASSERT( stringCache != NULL );

   /* Add the default category. */
   retCode = TA_FileIndexAddCategoryData( fileIndexPriv,
                                          categoryToUse,
                                          (TA_FileIndexCategoryData **)NULL );

   if( initialCategory == NULL )
      TA_StringFree( stringCache, categoryToUse );

   if( retCode != TA_SUCCESS )
   {
      TA_FileIndexPrivFree( fileIndexPriv );
      TA_TRACE_RETURN( retCode );
   }
   #endif

   /* Parse the source pattern into TA_TokenId tokens. */
   retCode = TA_FileIndexParsePath( fileIndexPriv, path );
   if( retCode != TA_SUCCESS )
   {
      TA_FileIndexPrivFree( fileIndexPriv );
      TA_TRACE_RETURN( retCode );
   }

   /* Go through the directories to build the list of category/symbols. */
   retCode = TA_FileIndexBuildIndex( fileIndexPriv );

   if( retCode != TA_SUCCESS )
   {
      TA_FileIndexPrivFree( fileIndexPriv );
      TA_TRACE_RETURN( retCode );
   }

   /* Re-initialize some pointers used while building the index.
    * These pointers are not supposed to be used from now on.
    * Setting these to NULL may help to prevent "bad usage" of these.
    */
   fileIndexPriv->curToken = NULL;
   fileIndexPriv->nextToken = NULL;
   fileIndexPriv->prevToken = NULL;
   fileIndexPriv->currentSymbolString = NULL;
   fileIndexPriv->currentCategoryString = NULL;
   fileIndexPriv->currentCategoryCountryString = NULL;
   fileIndexPriv->currentCategoryExchangeString = NULL;
   fileIndexPriv->currentCategoryTypeString = NULL;

   /* File index completed! return the result to the caller. */
   *newIndex = (TA_FileIndex *)fileIndexPriv;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_FileIndexFree( TA_FileIndex *toBeFreed )
{
   TA_PROLOG
   TA_FileIndexPriv *fileIndexPriv;
   TA_RetCode retCode;

   if( !toBeFreed )
      return TA_SUCCESS;

   fileIndexPriv = (TA_FileIndexPriv *)toBeFreed;

   TA_TRACE_BEGIN(  TA_FileIndexFree );

   retCode = TA_FileIndexPrivFree( fileIndexPriv );

   TA_TRACE_RETURN( retCode ); 
}

TA_String *TA_FileIndexFirstCategory( TA_FileIndex *fileIndex )
{
   TA_FileIndexCategoryData *categoryData;
   TA_FileIndexPriv         *fileIndexPriv;
   TA_List                  *listCategory;

   if( !fileIndex )
      return NULL;

   fileIndexPriv = (TA_FileIndexPriv *)fileIndex;
   listCategory = fileIndexPriv->listCategory;

   TA_ASSERT_RET( listCategory != NULL, (TA_String *)NULL );
   categoryData  = (TA_FileIndexCategoryData *)TA_ListAccessHead( listCategory );

   if( !categoryData )
      return NULL;

   TA_ASSERT_RET( categoryData->string != NULL, (TA_String *)NULL );

   return categoryData->string;
}

TA_String *TA_FileIndexNextCategory ( TA_FileIndex *fileIndex )
{
   TA_FileIndexCategoryData *categoryData;
   TA_FileIndexPriv         *fileIndexPriv;
   TA_List                  *listCategory;

   if( fileIndex == NULL )
      return NULL;

   fileIndexPriv = (TA_FileIndexPriv *)fileIndex;
   listCategory = fileIndexPriv->listCategory;

   TA_ASSERT_RET( listCategory != NULL, (TA_String *)NULL );
   categoryData  = (TA_FileIndexCategoryData *)TA_ListAccessNext( listCategory );

   if( !categoryData )
      return NULL;

   TA_ASSERT_RET( categoryData->string != NULL, (TA_String *)NULL );

   return categoryData->string;
}

TA_RetCode TA_FileIndexSelectCategory( TA_FileIndex *fileIndex,
                                       TA_String *category )
{
   TA_PROLOG
   TA_FileIndexPriv *fileIndexPriv;
   TA_String        *currentCategoryString;
   TA_List          *listCategory;
   TA_FileIndexCategoryData *categoryData;

   if( !fileIndex )
      return TA_INTERNAL_ERROR(85);
        
   fileIndexPriv = (TA_FileIndexPriv *)fileIndex;
   TA_TRACE_BEGIN(  TA_FileIndexSelectCategory );

   TA_ASSERT( category != NULL );

   listCategory = fileIndexPriv->listCategory;
   TA_ASSERT( listCategory != NULL );

   categoryData = (TA_FileIndexCategoryData *)TA_ListAccessCurrent( listCategory );

   if( categoryData == NULL )
   {
      currentCategoryString = TA_FileIndexFirstCategory( fileIndex );
      if( currentCategoryString == NULL )
      {
         TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
      }
   }
   else
      currentCategoryString = categoryData->string;

   /* Sanity check. */
   TA_ASSERT( currentCategoryString != NULL );

   /* Need to search for the category. */
   while( currentCategoryString )
   {
      /* Verify if we are positioned on the right category. if yes, return. */
      if( strcmp( TA_StringToChar( category ),
                  TA_StringToChar( currentCategoryString )) == 0 )
      {
         TA_TRACE_RETURN( TA_SUCCESS );
      }

      /* Go to next category string. */
      currentCategoryString = TA_FileIndexNextCategory( fileIndex );
   }

   TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
}

TA_FileInfo *TA_FileIndexFirstSymbol( TA_FileIndex *fileIndex )
{
   TA_FileIndexCategoryData *categoryData;
   TA_FileIndexSymbolData   *symbolData;
   TA_FileIndexPriv         *fileIndexPriv;
   TA_List                  *listCategory;
   TA_List                  *listSymbol;

   if( !fileIndex )
      return NULL;

   fileIndexPriv = (TA_FileIndexPriv *)fileIndex;
   listCategory = fileIndexPriv->listCategory;

   /* Make sure we are positioned on a valid category. */
   TA_ASSERT_RET( listCategory != NULL, (TA_FileInfo *)NULL );
   categoryData = (TA_FileIndexCategoryData *)TA_ListAccessCurrent( listCategory );

   if( !categoryData )
      return NULL;

   listSymbol = categoryData->listSymbol;

   TA_ASSERT_RET( listSymbol != NULL, (TA_FileInfo *)NULL );
   symbolData = (TA_FileIndexSymbolData *)TA_ListAccessHead( listSymbol );

   if( !symbolData )
      return (TA_FileInfo *)NULL;

   /* Parano sanity check: Verify that all the fields of the symbolData are ok.
    * This is a good place to verify sanity, since this is the last "exit point"
    * of the symbolData toward the user of the TA_FileIndex...
    */
   TA_ASSERT_RET( symbolData->parent == categoryData, (TA_FileInfo *)NULL );
   TA_ASSERT_RET( symbolData->string != NULL, (TA_FileInfo *)NULL );

   return (TA_FileInfo *)symbolData;
}

TA_FileInfo *TA_FileIndexNextSymbol( TA_FileIndex *fileIndex )
{
   TA_FileIndexCategoryData *categoryData;
   TA_FileIndexSymbolData   *symbolData;
   TA_FileIndexPriv         *fileIndexPriv;
   TA_List                  *listCategory;
   TA_List                  *listSymbol;

   if( !fileIndex )
      return NULL;

   fileIndexPriv = (TA_FileIndexPriv *)fileIndex;
   listCategory = fileIndexPriv->listCategory;

   /* Make sure we are positioned on a valid category. */
   TA_ASSERT_RET( listCategory != NULL, (TA_FileInfo *)NULL );
   categoryData = (TA_FileIndexCategoryData *)TA_ListAccessCurrent( listCategory );

   if( !categoryData )
      return NULL;

   listSymbol = categoryData->listSymbol;

   TA_ASSERT_RET( listSymbol != NULL, (TA_FileInfo *)NULL );
   symbolData = (TA_FileIndexSymbolData *)TA_ListAccessNext( listSymbol );

   if( !symbolData )
      return (TA_FileInfo *)NULL;

   /* Parano sanity check: Verify that all the fields of the symbolData are ok.
    * This is a good place to verify sanity, since this is the last "exit point"
    * of the symbolData toward the user of the TA_FileIndex...
    */
   TA_ASSERT_RET( symbolData->parent == categoryData, (TA_FileInfo *)NULL );
   TA_ASSERT_RET( symbolData->string != NULL, (TA_FileInfo *)NULL );

   return (TA_FileInfo *)symbolData;
}

/* Return the number of category. */
unsigned int TA_FileIndexNbCategory( TA_FileIndex *fileIndex )
{
   TA_FileIndexPriv *fileIndexPriv;
   TA_List          *listCategory;

   if( !fileIndex )
      return 0;

   fileIndexPriv = (TA_FileIndexPriv *)fileIndex;
   listCategory = fileIndexPriv->listCategory;
   TA_ASSERT_RET( listCategory != NULL, 0 );

   return TA_ListSize( listCategory );
}

/* Return the number of symbol for the current category. */
unsigned int TA_FileIndexNbSymbol( TA_FileIndex *fileIndex )
{
   TA_FileIndexCategoryData *categoryData;
   TA_FileIndexPriv         *fileIndexPriv;
   TA_List                  *listCategory;
   TA_List                  *listSymbol;

   if( !fileIndex )
       return 0;
   fileIndexPriv = (TA_FileIndexPriv *)fileIndex;
   listCategory = fileIndexPriv->listCategory;

   /* Make sure we are positioned on a valid category. */
   TA_ASSERT_RET( listCategory != NULL, 0 );
   categoryData = (TA_FileIndexCategoryData *)TA_ListAccessCurrent( listCategory );

   if( !categoryData )
      return 0;

   listSymbol = categoryData->listSymbol;

   TA_ASSERT_RET( listSymbol != NULL, 0 );

   return TA_ListSize( listSymbol );
}

/* Function to extract information from a TA_FileInfo. */
TA_String *TA_FileInfoSymbol( TA_FileInfo *fileInfo )
{
   TA_FileIndexSymbolData *symbolData;

   TA_ASSERT_RET( fileInfo != NULL, (TA_String *)NULL );
   symbolData = (TA_FileIndexSymbolData *)fileInfo;

   TA_ASSERT_RET( symbolData->string != NULL, (TA_String *)NULL );

   return symbolData->string;
}

TA_String *TA_FileInfoCategory( TA_FileInfo *fileInfo )
{
   TA_FileIndexSymbolData *symbolData;
   TA_FileIndexCategoryData *categoryData;

   TA_ASSERT_RET( fileInfo != NULL, (TA_String *)NULL );
   symbolData = (TA_FileIndexSymbolData *)fileInfo;

   categoryData = symbolData->parent;

   TA_ASSERT_RET( categoryData != NULL, (TA_String *)NULL );
   TA_ASSERT_RET( categoryData->string != NULL, (TA_String *)NULL );

   return categoryData->string;
}

const char *TA_FileInfoPath( TA_FileInfo *fileInfo )
{
   TA_RetCode retCode;
   TA_FileIndexSymbolData *symbolData;
   TA_FileIndexCategoryData *categoryData;
   TA_FileIndexPriv *fileIndex;
   char *scratchPad;

   /* Get the corresponding scratch pad. */
   TA_ASSERT_RET( fileInfo != NULL, (char *)NULL );
   symbolData = (TA_FileIndexSymbolData *)fileInfo;
   categoryData = symbolData->parent;

   TA_ASSERT_RET( categoryData != NULL, (char *)NULL );
   fileIndex = categoryData->parent;

   TA_ASSERT_RET( fileIndex != NULL, (char *)NULL );
   scratchPad = fileIndex->scratchPad;

   /* Build the path in the scratchPad. */
   TA_ASSERT_RET( scratchPad != NULL, (char *)NULL );
   TA_ASSERT_RET( symbolData->node != NULL, (char *)NULL );
   retCode = TA_FileIndexMakePathPattern( fileIndex,
                                          symbolData->node,
                                          scratchPad,
                                          TA_SOURCELOCATION_MAX_LENGTH );

   if( retCode != TA_SUCCESS )
      return (char *)NULL;

   return scratchPad;
}

/**** Local functions definitions.     ****/
/* None */

