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
 *       From the TA_AsciiFileData->listLocateToken Do the
 *       following for each identified category:
 *           - Add a TA_AsciiFileData->listCategory entry.
 *           - Add all symbols in the TA_FileIndexCategoryData->listSymbol
 */

/**** Headers ****/
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include "ta_memory.h"
#include "ta_system.h"
#include "ta_trace.h"
#include "ta_data.h"
#include "ta_fileindex_priv.h"
#include "ta_global.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_ListIter iter;
   TA_TokenInfo *curToken;
   const char   *curPosInToken;
} TA_PatternIter;

/**** Local functions declarations.    ****/
static TA_RetCode buildIndexList  ( TA_FileIndexPriv *fileIndexPriv, unsigned int fileDepth );
static TA_RetCode processDirectory( TA_FileIndexPriv *fileIndexPriv, unsigned int fileDepth );
static TA_RetCode processFiles    ( TA_FileIndexPriv *fileIndexPriv, unsigned int fileDepth );

static TA_RetCode makePathPattern( TA_FileIndexPriv *fileIndexPriv,
                                   TA_ValueTreeNode *node, int maxDepth,
                                   char *bufferToUse, int maxBufferSize );


static TA_RetCode fieldToStr( TA_TokenInfo *currentToken,
                              TA_ValueTreeNode *currentValue,
                              char *str, int maxStrLength,
                              int *nbCharAdded, int *valueConsumed );

static TA_RetCode extractTokenValue( TA_FileIndexPriv *fileIndexPriv,
                                     TA_ValueTreeNode **addedHead,
                                     const char *string,
                                     int allowIndexUpdate );

static TA_RetCode extractTokenValueRecursive( TA_FileIndexPriv *fileIndexPriv,
                                              TA_PatternIter *iter,
                                              const char *string,
                                              unsigned int firstOfOneOrMore );

static TA_RetCode addValueDown( TA_FileIndexPriv *fileIndexPriv,
                                TA_TokenId id,
                                const char *string );

static TA_RetCode addCurrentDataToIndex( TA_FileIndexPriv *fileIndexPriv,
                                         TA_ValueTreeNode *treeNodeValue );

static TA_RetCode TA_PatternInit( TA_FileIndexPriv *fileIndexPriv, TA_PatternIter *iter );
static char getPatternChar ( TA_PatternIter *iter );
static char nextPatternChar( TA_PatternIter *iter );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_FileIndexBuildIndex( TA_FileIndexPriv *fileIndexPriv )
{
   TA_PROLOG
   TA_RetCode retCode;
   unsigned int fileDepth;

   if( !fileIndexPriv )
      return TA_ALLOC_ERR;

   TA_TRACE_BEGIN(  TA_FileIndexBuildIndex );

   /* Verify parameters. */
   if( !fileIndexPriv->listLocationToken )
   {
      TA_FATAL( NULL, fileIndexPriv, fileIndexPriv->listLocationToken );
   }

   /* Identify at which level that files are
    * processed instead of directories.
    */
   fileDepth = TA_FileIndexIdentifyFileDepth( fileIndexPriv );

   if( fileDepth == 0 )
   {
      TA_TRACE_RETURN( TA_INVALID_PATH );
   }

   retCode = buildIndexList( fileIndexPriv, fileDepth );

   TA_TRACE_RETURN( retCode );
}


/* The path is build in the provided buffer.
 * All wildcards are resolved using the provided TA_ValueTreeNode.
 */
TA_RetCode TA_FileIndexMakePathPattern( TA_FileIndexPriv *fileIndexPriv,
                                        TA_ValueTreeNode *node,
                                        char *bufferToUse,
                                        int maxBufferSize )
{
   TA_PROLOG
   int maxDepth;
   TA_RetCode retCode;

   TA_TRACE_BEGIN(  TA_FileIndexMakePathPattern );

   TA_ASSERT( fileIndexPriv != NULL );
   TA_ASSERT( node !=  NULL );
   TA_ASSERT( bufferToUse != NULL );
   TA_ASSERT( maxBufferSize >= 1 );

   TA_ASSERT( fileIndexPriv->listLocationToken != NULL );
   maxDepth = TA_ListSize( fileIndexPriv->listLocationToken ) - 1;
   TA_ASSERT( maxDepth >= 1 );

   bufferToUse[0] = '\0';

   retCode = makePathPattern( fileIndexPriv, node, maxDepth,
                           bufferToUse, maxBufferSize );

   TA_TRACE_RETURN( retCode );
}


/**** Local functions definitions.     ****/
static TA_RetCode buildIndexList( TA_FileIndexPriv *fileIndexPriv, unsigned int fileDepth )
{
   TA_PROLOG
   TA_RetCode retCode;

   /* Note: this function is recursively called. */

   /* Validate parameters. */
   if( fileIndexPriv == NULL )
      return TA_ALLOC_ERR;

   TA_TRACE_BEGIN(  buildIndexList );

   if( fileIndexPriv->curTokenDepth > 500 )
   {
      TA_FATAL(  NULL, fileIndexPriv->curTokenDepth, 0 );
   }

   if( fileIndexPriv->curToken )
   {
      if( (fileIndexPriv->curToken->id == TA_INVALID_TOKEN_ID) ||
          (fileIndexPriv->curToken->id > TA_NB_TOKEN_ID) ||
          (fileIndexPriv->curToken->id == TA_TOK_END) )
      {
         TA_FATAL(  NULL, fileIndexPriv->curTokenDepth, fileIndexPriv->curToken->id );
      }
   }

   /* Get next (first?) token. */
   retCode = TA_FileIndexMoveToNextToken( fileIndexPriv );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   if( fileIndexPriv->curToken == NULL )
   {
      TA_FATAL(  NULL, fileIndexPriv->prevToken, fileIndexPriv->nextToken );
   }

   /* Skip all fix part not requiring processing. */
   while( (fileIndexPriv->curTokenDepth < fileDepth) &&
          ((fileIndexPriv->curToken->id == TA_TOK_FIX) || (fileIndexPriv->curToken->id == TA_TOK_SEP)) )
   {
      retCode = TA_FileIndexMoveToNextToken( fileIndexPriv );
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
   }

   /* At this point one of the following may have happen:
    *  - It hit a field or wildcard that needs to be resolved.
    *  - It hit the file depth.
    */
   if( fileIndexPriv->curTokenDepth < fileDepth )
   {
      retCode = processDirectory( fileIndexPriv, fileDepth );
      TA_TRACE_RETURN( retCode );
   }
   else
   {
      retCode = processFiles( fileIndexPriv, fileDepth );
      TA_TRACE_RETURN( retCode );
   }
}



static TA_RetCode makePathPattern( TA_FileIndexPriv *fileIndexPriv,
                                   TA_ValueTreeNode *node, int maxDepth,
                                   char *bufferToUse, int maxBufferSize )
{
   TA_PROLOG
   TA_List *listOfValue;
   TA_TokenInfo *currentToken;
   TA_ValueTreeNode *currentValue;
   TA_RetCode retCode;

   int i;
   int nbCharAdded, valueConsumed;
   unsigned int nbChar;

   if( !fileIndexPriv )
      return TA_INTERNAL_ERROR(63);

   TA_TRACE_BEGIN( makePathPattern );

   /* Build a list of value up to the root. */
   listOfValue = TA_ListAlloc( );

   if( !listOfValue )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   while( node != fileIndexPriv->root )
   {
      retCode = TA_ListAddHead( listOfValue, node );
      if( retCode != TA_SUCCESS )
      {
         TA_ListFree( listOfValue );
         TA_TRACE_RETURN( retCode );
      }
      node = node->parent;
   }

   /* Build the complete path in the provided buffer. */
   currentToken = (TA_TokenInfo *)     TA_ListAccessHead( fileIndexPriv->listLocationToken  );
   currentValue = (TA_ValueTreeNode *) TA_ListAccessHead( listOfValue );
   nbChar = 0;

   for( i=0; i < maxDepth; i++ )
   {
      if( !currentToken )
      {
         TA_ListFree( listOfValue );
         TA_TRACE_RETURN( TA_INTERNAL_ERROR(64) );
      }

      /* Add the field. */
      valueConsumed = 0;
      nbCharAdded = 0;
      retCode = fieldToStr( currentToken, currentValue,
                            &bufferToUse[nbChar], maxBufferSize,
                            &nbCharAdded, &valueConsumed );

      if( retCode != TA_SUCCESS )
      {
         TA_ListFree( listOfValue );
         TA_TRACE_RETURN( retCode );
      }

      nbChar += nbCharAdded;
      maxBufferSize -= nbCharAdded;

      /* Go to the next token. */
      if( valueConsumed )
         currentValue = TA_ListAccessNext( listOfValue );
      currentToken = TA_ListAccessNext( fileIndexPriv->listLocationToken );
   }

   TA_ListFree( listOfValue );

   /* Note: When returning from 'makePathPattern' it should still
    * be in the same position we were in the 'fileIndexPriv->listLocationToken'.
    * (this is becoming ugly to have this kind of dependency... )
    */
   currentToken = TA_ListAccessNext( fileIndexPriv->listLocationToken );
   if( fileIndexPriv->curToken )
   {
      if( currentToken != fileIndexPriv->nextToken )
      {
         TA_FATAL( NULL, currentToken->id, fileIndexPriv->curToken->id );
      }
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode fieldToStr( TA_TokenInfo *currentToken,
                              TA_ValueTreeNode *currentValue,
                              char *str, int maxStrLength,
                              int *nbCharAdded, int *valueConsumed )
{
   int strLength;
   const char *strToUse;

   if( (!currentToken) || (!str) )
      return TA_INTERNAL_ERROR(65);

   if( maxStrLength == 0 )
      return TA_INTERNAL_ERROR(66);

   if( nbCharAdded )
      *nbCharAdded = 0;

   if( valueConsumed )
      *valueConsumed = 0;

   switch( currentToken->id )
   {
   case TA_TOK_FIX:
   case TA_TOK_SEP:
      if( currentToken->value )
         strToUse  = TA_StringToChar( currentToken->value );
      else
         return TA_INTERNAL_ERROR(67);
      break;

   case TA_TOK_END:
      return TA_INTERNAL_ERROR(68);

   case TA_TOK_WILD:
   case TA_TOK_WILD_CHAR:
   case TA_TOK_SYM:
   case TA_TOK_CAT:
   case TA_TOK_CATC:
   case TA_TOK_CATX:
   case TA_TOK_CATT:
      if( !currentValue )
      {
         if( currentToken->value )
            strToUse  = TA_StringToChar( currentToken->value );
         else
            return TA_INTERNAL_ERROR(69);
      }
      else
      {
         if( currentValue->string )
            strToUse = TA_StringToChar( currentValue->string );
         else
         {
            if( currentToken->id != TA_TOK_WILD )
               return TA_INTERNAL_ERROR(70);
            strToUse = "";
         }

         if( valueConsumed )
            *valueConsumed = 1;
      }
      break;

   default:
      if( (!currentValue) || (!currentValue->string) )
      {
         if( currentToken->value )
            strToUse  = TA_StringToChar( currentToken->value );
         else
            return TA_INTERNAL_ERROR(71);
      }
      else
      {
         strToUse = TA_StringToChar( currentValue->string );
         if( valueConsumed )
            *valueConsumed = 1;
      }
      break;
   }

   strLength = strlen( strToUse );

   if( strLength >= maxStrLength )
      return TA_INTERNAL_ERROR(72);

   strcpy( str, strToUse );
   if( nbCharAdded )
      *nbCharAdded = strLength;

   return TA_SUCCESS;
}

static TA_RetCode processDirectory( TA_FileIndexPriv *fileIndexPriv, unsigned int fileDepth )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_Directory *directory;
   char *basename;
   int rememberDepth;
   TA_ValueTreeNode *addedHead;
   TA_String *curDirectoryString;
   int dataSourceEmpty;
   TA_StringCache *stringCache;

   if( !fileIndexPriv )
      return TA_INTERNAL_ERROR(73);

   TA_TRACE_BEGIN(  processDirectory );

   stringCache = TA_GetGlobalStringCache( );

   /* Note: This function is recursively called. */

   /* Assume the fileIndexPriv source is empty from this level, until
    * the contrary is found.
    */
   dataSourceEmpty = 1;

   /* Locally allocated ressources. */
   addedHead = NULL;
   directory = NULL;
   curDirectoryString = NULL;

   /* Define a local MACRO for clean return of this function. */
   #define CLEAN_UP \
   { \
      if(addedHead) \
      { \
         fileIndexPriv->currentNode=addedHead->parent; \
         TA_FileIndexFreeValueTree(addedHead); \
      } \
      if(directory) \
         TA_DirectoryFree(directory); \
      if(curDirectoryString) \
         TA_StringFree(stringCache,curDirectoryString);  \
   }
       
   #define RETURN(retCode ) \
   { \
      CLEAN_UP;   \
      TA_TRACE_RETURN( retCode ); \
   }

   /* Some quick sanity check. */
   if( !(fileIndexPriv->nextToken) || !(fileIndexPriv->curToken) )
   {
      CLEAN_UP;
      TA_FATAL(  NULL, fileIndexPriv->nextToken, fileIndexPriv->curToken );
   }

   if( (fileIndexPriv->curToken->id == TA_TOK_END) ||
       (fileIndexPriv->nextToken->id == TA_TOK_END) )
   {
      CLEAN_UP;
      TA_FATAL(  NULL, fileIndexPriv->nextToken->id, fileIndexPriv->curToken->id );
   }

   /* Skip tokens until the next TA_TOK_SEP. The directory
    * pattern will be build up to this token (excluding the
    * trailing TA_TOK_SEP of course).
    */
   while( fileIndexPriv->curToken->id != TA_TOK_SEP )
   {
      /* Skip to the next token. */
      TA_FileIndexMoveToNextToken( fileIndexPriv );
      if( !fileIndexPriv->curToken || (fileIndexPriv->curToken->id == TA_TOK_END) )
         RETURN( TA_INTERNAL_ERROR(74) );
   }

   /* Remember the token depth for the processing of directory at this level. */
   rememberDepth = fileIndexPriv->curTokenDepth;

   /* Identify the basename and the pattern. */
   retCode = makePathPattern( fileIndexPriv, fileIndexPriv->currentNode, fileIndexPriv->curTokenDepth-1,
                              fileIndexPriv->scratchPad, TA_SOURCELOCATION_MAX_LENGTH );
   if( retCode != TA_SUCCESS )
      RETURN( retCode );

   basename = fileIndexPriv->scratchPad;

   /* Get all the corresponding directory entries. */
   retCode = TA_DirectoryAlloc( basename, &directory );
   if( retCode != TA_SUCCESS )
      RETURN( retCode );

   /* Iterate for each sub-directory. */
   while( directory->nbDirectory )
   {
      curDirectoryString = (TA_String *) TA_ListRemoveHead( directory->listOfDirectory );
      if( !curDirectoryString )
         RETURN( TA_INTERNAL_ERROR(75) );

      /* For each sub-directory entry, extract potential
       * new token values.
       *
       * About 'addedHead': if fields values are added for this path, keep
       * track of the root of it. If there is no fileIndexPriv found under the
       * specified path, all corresponding values are going to be freed.
       *
       * When an internal error occured, we do some clean-up, including the
       * removal of all value added from addedHead.
       */
      addedHead = NULL;
      retCode = extractTokenValue( fileIndexPriv,
                                   &addedHead,
                                   TA_StringToChar( curDirectoryString ),
                                   0 );

      /* Some sanity check. */
      if( !fileIndexPriv->nextToken )
         RETURN( TA_INTERNAL_ERROR(76) );

      if( fileIndexPriv->nextToken->id == TA_TOK_END )
      {
         /* End-up here if TA_TOK_END is prematury seen... */
         TA_FATAL(  NULL, fileIndexPriv->curTokenDepth, fileIndexPriv->nextToken->id );         
      }

      /* Trap all internal error from extractTokenValue */
      if( (retCode != TA_SUCCESS) && (retCode != TA_NO_DATA_SOURCE) )
         RETURN( retCode );

      /* Just keep processing recursively for the next leg if that one
       * was valid.
       */
      if( retCode == TA_SUCCESS )
      {
         /* Recursive call. */
         retCode = buildIndexList( fileIndexPriv, fileDepth );

         if( retCode == TA_SUCCESS )
            dataSourceEmpty = 0; /* We got some fileIndexPriv! */
         else if( retCode != TA_NO_DATA_SOURCE )
         {
            /* An error other than TA_NO_DATA_SOURCE occured. */
            RETURN( retCode );
         }
      }

      if( addedHead != NULL )
      {
         fileIndexPriv->currentNode = addedHead->parent;

         if( retCode == TA_NO_DATA_SOURCE )
         {
            /* TA_NO_DATA_SOURCE was establish and some value are still in
             * the value tree... make sure all added values are freed.
             */
            TA_FileIndexFreeValueTree( addedHead );
         }

         /* At this point the value are either good and belongs to the
          * tree value, or the values has been freed. In either case, we do
          * not wish to free these values again...
          */
         addedHead = NULL;
      }

      /* Re-position the processing of the token at the same
       * level for processing another possible directory.
       */
      TA_FileIndexMoveToDepth( fileIndexPriv, rememberDepth );

      /* Proceed to the next directory entry. */
      TA_StringFree( stringCache, curDirectoryString );
      curDirectoryString = NULL;
      directory->nbDirectory--;
   }

   /* Clean-up and return successfully. */
   retCode = TA_DirectoryFree( directory );
   directory = NULL;
   if( retCode != TA_SUCCESS )
      RETURN( TA_ALLOC_ERR );

   if( dataSourceEmpty )
      RETURN( TA_NO_DATA_SOURCE );

   RETURN( TA_SUCCESS );
   #undef RETURN
   #undef CLEAN_UP
}

static TA_RetCode processFiles( TA_FileIndexPriv *fileIndexPriv, unsigned int fileDepth )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_Directory *directory;
   char *basename;
   int rememberDepth;
   TA_ValueTreeNode *addedHead;
   TA_String *curFileString;
   TA_StringCache *stringCache;

   int dataSourceEmpty;

   (void)fileDepth; /* Get ride of compiler warning. */

   if( !fileIndexPriv )
      return TA_INTERNAL_ERROR(78);

   TA_TRACE_BEGIN(  processFiles );

   stringCache = TA_GetGlobalStringCache( );

   /* Processing Files. */

   /* Note: This function is recursively called. */

   /* Initialize local variable. */
   addedHead = NULL;
   directory = NULL;
   curFileString = NULL;

   /* Assume the fileIndexPriv source is empty from this level, until
    * the contrary is found.
    */
   dataSourceEmpty = 1;

   /* Define a local MACRO for clean return of this function. */

   #define CLEAN_UP { \
   if(addedHead) \
   { \
      fileIndexPriv->currentNode=addedHead->parent; \
      TA_FileIndexFreeValueTree(addedHead);\
   } \
   if(directory) \
      TA_DirectoryFree(directory); \
   if(curFileString) \
      TA_StringFree(stringCache,curFileString); \
   }

   #define RETURN(retCode) { \
      CLEAN_UP; \
      TA_TRACE_RETURN( retCode ); \
   }

   /* Some quick sanity check. */
   if( !(fileIndexPriv->nextToken) || !(fileIndexPriv->curToken) )
   {
      CLEAN_UP;
      TA_FATAL( NULL, fileIndexPriv->nextToken, fileIndexPriv->curToken );
   }

   if( fileIndexPriv->curToken->id == TA_TOK_END )
   {
      CLEAN_UP;
      TA_FATAL( NULL, fileIndexPriv->nextToken->id, fileIndexPriv->curToken->id );      
   }

   /* Skip tokens until the TA_TOK_END. The directory pattern
    * pattern will be build up to this token (excluding TA_TOK_END
    * itself of course).
    */
   while( fileIndexPriv->curToken->id != TA_TOK_END )
      TA_FileIndexMoveToNextToken( fileIndexPriv );

   /* Remember the token depth for the processing of files at this level. */
   rememberDepth = fileIndexPriv->curTokenDepth;

   /* Identify the basename and the pattern. */
   retCode = makePathPattern( fileIndexPriv, 
                              fileIndexPriv->currentNode,
                              fileIndexPriv->curTokenDepth-1,
                              fileIndexPriv->scratchPad,
                              TA_SOURCELOCATION_MAX_LENGTH );
   if( retCode != TA_SUCCESS )
      RETURN( retCode );

   basename = fileIndexPriv->scratchPad;

   /* Get all the corresponding files entries. */
   retCode = TA_DirectoryAlloc( basename, &directory );
   if( retCode != TA_SUCCESS )
      RETURN( retCode );

   /* Iterate for each file. */
   while( directory->nbFile )
   {
      curFileString = (TA_String *) TA_ListRemoveHead( directory->listOfFile );
      if( !curFileString )
         RETURN( TA_INTERNAL_ERROR(79) );

      /* For each files, extract potential new token values.
       *
       * About 'addedHead': if fields values are added for this path, keep
       * track of the root of it. If an error occured these fields must
       * be freed.
       */

      /* printf( " File:[%s]\n", TA_StringToChar( curFileString ) ); */

      addedHead = NULL;
      retCode = extractTokenValue( fileIndexPriv, &addedHead,
                                   TA_StringToChar( curFileString ),
                                   1 );

      if( retCode == TA_SUCCESS )
         dataSourceEmpty = 0; /* We got at least one file... */
      else if( retCode != TA_NO_DATA_SOURCE )
         RETURN( retCode );

      if( addedHead != NULL )
      {
         fileIndexPriv->currentNode = addedHead->parent;

         if( retCode == TA_NO_DATA_SOURCE )
         {
            /* TA_NO_DATA_SOURCE was establish and some value are still in
             * the value tree... make sure all added values are freed.
             */
            TA_FileIndexFreeValueTree( addedHead );
         }

         /* At this point the value are either good and belongs to the
          * tree value, or the values has been freed. In either case, we do
          * not wish to free these values again...
          */
         addedHead = NULL;
      }

      /* Just keep processing for the next file. */

      /* Re-position currentNode for adding at the correct place
       * in the tree.
       */
      if( addedHead )
      {
         fileIndexPriv->currentNode = addedHead->parent;
         addedHead = NULL;
      }

      /* Re-position the processing of the token at the same
       * level for processing another possible file.
       */
      TA_FileIndexMoveToDepth( fileIndexPriv, rememberDepth );

      /* Proceed to the next file entry. */
      TA_StringFree( stringCache, curFileString );
      curFileString = NULL;
      directory->nbFile--;
   }

   /* Clean-up and return successfully. */
   retCode = TA_DirectoryFree( directory );
   directory = NULL;
   if( retCode != TA_SUCCESS )
      RETURN( retCode );

   if( dataSourceEmpty )
      RETURN( TA_NO_DATA_SOURCE );

   RETURN( TA_SUCCESS );
   #undef RETURN
   #undef CLEAN_UP
}

static TA_RetCode TA_PatternInit( TA_FileIndexPriv *fileIndexPriv,
                                  TA_PatternIter *iter )
{
   TA_PROLOG
   TA_RetCode retCode;

   TA_TRACE_BEGIN(  TA_PatternInit );

   TA_ASSERT( fileIndexPriv != NULL );
   TA_ASSERT( iter != NULL );
   TA_ASSERT( fileIndexPriv->curToken != NULL );
   TA_ASSERT( fileIndexPriv->curToken->id != TA_TOK_END );
   TA_ASSERT( fileIndexPriv->listLocationToken != NULL );

   retCode = TA_ListIterInit( &iter->iter, fileIndexPriv->listLocationToken );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Position 'iter->curToken' on the currentToken of 'fileIndexPriv'. */
   iter->curToken = TA_ListIterHead( &iter->iter );
   if( !iter->curToken )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(80) );
   }

   while( iter->curToken != fileIndexPriv->curToken )
   {
      iter->curToken = TA_ListIterNext( &iter->iter );
      if( !iter->curToken )
      {
         TA_TRACE_RETURN( TA_INTERNAL_ERROR(81) );
      }
   }

   /* Some fields receive special iteration within their value string.
    *   TA_TOK_FIX, TA_TOK_SYMF, TA_TOK_SYM, TA_TOK_CAT,
    *   TA_TOK_CATC, TA_TOK_CATX, TA_TOK_CATT
    *
    * All other fields go always to the next token.
    */
   switch( iter->curToken->id )
   {
   case TA_TOK_FIX:
   case TA_TOK_SYMF:
   case TA_TOK_SYM:
   case TA_TOK_CAT:
   case TA_TOK_CATC:
   case TA_TOK_CATX:
   case TA_TOK_CATT:
      iter->curPosInToken = TA_StringToChar( iter->curToken->value );
      break;
   default:
      iter->curPosInToken = NULL;
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static char nextPatternChar( TA_PatternIter *iter )
{
   TA_ASSERT_RET( iter != NULL, '\0' );
   TA_ASSERT_RET( iter->curToken != NULL, '\0' );

   if( ((iter->curToken->id) == TA_TOK_END) ||
       ((iter->curToken->id) == TA_TOK_SEP) )
      return '\0';

   /* Some fields receive special iteration within their value string.
    *   TA_TOK_FIX, TA_TOK_SF, TA_X_ and TA_TOK_S
    *
    * All other fields go always to the next token.
    */
    if( !iter->curPosInToken || (*(iter->curPosInToken+1) == '\0') )
    {
       /* Move to next token. */
       iter->curToken = TA_ListIterNext( &iter->iter );

       if( !iter->curToken )
       {
          TA_FATAL_RET( NULL, 0, 0, '\0' );
       }

       switch( iter->curToken->id )
       {
       case TA_TOK_FIX:
       case TA_TOK_SYMF:
       case TA_TOK_SYM:
       case TA_TOK_CAT:
       case TA_TOK_CATC:
       case TA_TOK_CATX:
       case TA_TOK_CATT:
          iter->curPosInToken = TA_StringToChar( iter->curToken->value );
          break;

       case TA_TOK_SEP:
       case TA_TOK_END:
          return '\0';

       default:
          iter->curPosInToken = NULL;
       }
    }
    else
    {
       /* Move to next character. */
       iter->curPosInToken++;
    }

    return getPatternChar( iter);
}

static char getPatternChar ( TA_PatternIter *iter )
{
   TA_ASSERT_RET( iter != NULL, '\0' );
   TA_ASSERT_RET( iter->curToken != NULL,'\0' );

   /* These token delimitate the end of the pattern... */
   if( (iter->curToken->id == TA_TOK_SEP) ||
       (iter->curToken->id == TA_TOK_END) )
      return '\0';

   if( iter->curToken->id == TA_TOK_WILD )
      return '*'; /* Zero or more character */

   if( iter->curToken->id == TA_TOK_WILD_CHAR )
      return '?'; /* One character. */

   TA_ASSERT_RET( iter->curPosInToken != NULL, 0 );

   switch( iter->curToken->id )
   {
      case TA_TOK_SYM:
      case TA_TOK_CAT:
      case TA_TOK_CATC:
      case TA_TOK_CATX:
      case TA_TOK_CATT:
         /* A trick to get the logic for "one or more character" */
         if( *(iter->curPosInToken+1) != '\0' )
            return '|'; /* One character kept in memory for next '*'. */
         else
            return '*'; /* Zero or more character following the '|' */
      default:
         /* Do nothing */
         break;
   }

   return *iter->curPosInToken;
}

static TA_RetCode extractTokenValue( TA_FileIndexPriv *fileIndexPriv,
                                     TA_ValueTreeNode **addedHead,
                                     const char *string,
                                     int allowIndexUpdate )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_PatternIter iterPattern;
   unsigned int atLeastOneValueToExtract; /* Boolean */
   int rememberStartOfLeg, rememberEndOfLeg;
   TA_ValueTreeNode *rememberTail;

   TA_TRACE_BEGIN(  extractTokenValue );

   /* Note: for proper clean-up of ressources used by this function, you must
    * exit by jumping at  extractTokenValue_exit.
    */
   TA_ASSERT( fileIndexPriv != NULL );
   TA_ASSERT( addedHead != NULL );
   TA_ASSERT( string != NULL );
   TA_ASSERT( (allowIndexUpdate == 0) || (allowIndexUpdate == 1) );

   /* When calling this function, it is assumed that the fileIndexPriv->curtoken is
    * somewhere in the leg that needs to be resolved.
    *
    * A leg is all the character included between two separator (the separator
    * may not be there though).
    *
    * When this function return, the fileIndexPriv->curToken will be position at the end
    * of the processed leg (either on the TA_TOK_SEP or the TA_TOK_END).
    */
   rememberEndOfLeg = -1;
   rememberStartOfLeg = -1;

   /* The addedHead will be set and returned to the user only on success. */
   *addedHead = NULL;

   /* Remember tail is needed to re-build the complete path and it must
    * point to the last stored valued in the value tree (it will be a leaf).
    */
   rememberTail = fileIndexPriv->currentNode;

   /* Move back to the first token starting this leg. */
   do
   {
      retCode = TA_FileIndexMoveToPrevToken( fileIndexPriv );
      if( retCode != TA_SUCCESS )
         goto extractTokenValue_exit;
   }
   while( (fileIndexPriv->curToken->id != TA_TOK_SEP) && (fileIndexPriv->prevToken != NULL) );

   if( fileIndexPriv->prevToken != NULL )
   {
      retCode = TA_FileIndexMoveToNextToken( fileIndexPriv );
      if( retCode != TA_SUCCESS )
         goto extractTokenValue_exit;
   }

   /* Pre-allocate all the values needed in the value tree. */
   rememberStartOfLeg = fileIndexPriv->curTokenDepth;
   atLeastOneValueToExtract = 0;
   while( (fileIndexPriv->curToken->id != TA_TOK_END) &&
          (fileIndexPriv->curToken->id != TA_TOK_SEP) )
   {
      switch( fileIndexPriv->curToken->id )
      {
      case TA_TOK_WILD:
      case TA_TOK_WILD_CHAR:
      case TA_TOK_SYM:
      case TA_TOK_CAT:
      case TA_TOK_CATC:
      case TA_TOK_CATX:
      case TA_TOK_CATT:
         if( atLeastOneValueToExtract == 0 )
         {
            retCode = TA_FileIndexAddTreeValue( fileIndexPriv, NULL, addedHead );
            rememberTail = *addedHead;
         }
         else
            retCode = TA_FileIndexAddTreeValue( fileIndexPriv, NULL, &rememberTail );

         if( retCode != TA_SUCCESS )
            goto extractTokenValue_exit;

         if( *addedHead == NULL )
         {
            TA_FATAL( NULL, *addedHead, 0 );
         }

         atLeastOneValueToExtract = 1;
         break;

      case TA_TOK_SYMF:
         fileIndexPriv->currentSymbolString = fileIndexPriv->curToken->value;
         break;

      default:
         /* Do nothing */
         break;
      }

      retCode = TA_FileIndexMoveToNextToken( fileIndexPriv );
      if( retCode != TA_SUCCESS )
         goto extractTokenValue_exit;
   }

   if( !atLeastOneValueToExtract )
   {
      /* fileIndexPriv->currentCategoryString = fileIndexPriv->initialCategoryString; */

      retCode = addCurrentDataToIndex( fileIndexPriv,  rememberTail );

      goto extractTokenValue_exit;
   }

   /* Remember where this leg finish, since it needs to be in that position
    * before returning from this function.
    */
   rememberEndOfLeg = fileIndexPriv->curTokenDepth;

   /* Re-position the "fileIndexPriv->curToken" to the beginning. */
   retCode = TA_FileIndexMoveToDepth( fileIndexPriv, rememberStartOfLeg );
   if( retCode != TA_SUCCESS )
      goto extractTokenValue_exit;

   /* Reposition the "fileIndexPriv->currentNode" to the beginning. */
   retCode = TA_FileIndexSetCurrentTreeValueNode( fileIndexPriv, *addedHead );
   if( retCode != TA_SUCCESS )
      goto extractTokenValue_exit;

   /* Start the mechanism for iterating in the pattern. */
   retCode = TA_PatternInit( fileIndexPriv, &iterPattern );
   if( retCode != TA_SUCCESS )
      goto extractTokenValue_exit;

   /* Start to recursively extract the token values while
    * modifying the value in the value tree.
    */
   retCode = extractTokenValueRecursive( fileIndexPriv, &iterPattern, string, 0 );

   if( retCode != TA_SUCCESS )
      goto extractTokenValue_exit;

   /* Everything got parsed/extracted correctly, create the symbol
    * and the category if the caller asked for it.
    */
   if( allowIndexUpdate )
   {
      if( !fileIndexPriv->currentSymbolString )
      {
         retCode = TA_INTERNAL_ERROR(83);
         goto extractTokenValue_exit;
      }

      #if 0
      !!! To be removed?
      /* If there is no fileIndexPriv->currentCategoryString extracted
       * from the path, take the user provided categoryString.
       */
      if( !fileIndexPriv->currentCategoryString )
         fileIndexPriv->currentCategoryString = fileIndexPriv->initialCategoryString;
      #endif

      retCode = addCurrentDataToIndex( fileIndexPriv, rememberTail );
   }

   /* Must stay the only possible exit point of this function. */
extractTokenValue_exit:
   /* retCode and rememberEndOfLeg must be set before jumping here. */

   if( retCode != TA_SUCCESS )
   {
      if( *addedHead )
      {
         /* Reset the currentNode and get ride of all values that were
          * used while going down on this path.
          */
         fileIndexPriv->currentNode = (*addedHead)->parent;
         TA_FileIndexFreeValueTree( *addedHead );
         *addedHead = NULL;
      }
   }

   /* Attempt to re-position the "fileIndexPriv->curToken" to the end of the leg. */
   if( rememberEndOfLeg != -1 )
      TA_FileIndexMoveToDepth( fileIndexPriv, rememberEndOfLeg );

   TA_TRACE_RETURN( retCode );
}

/* Set the 'fileIndexPriv->currentNode' and move down to the first child. */
static TA_RetCode addValueDown( TA_FileIndexPriv *fileIndexPriv,
                                TA_TokenId id,
                                const char *string )
{
   TA_String *newStr;
   TA_RetCode retCode;
   TA_StringCache *stringCache;
   
   stringCache = TA_GetGlobalStringCache( );

   newStr = TA_StringAlloc( stringCache, string );

   if( !newStr )
      return TA_ALLOC_ERR;

   retCode = TA_FileIndexChangeValueTreeNodeValue( fileIndexPriv->currentNode, newStr );

   switch( id )
   {
   case TA_TOK_SYM:
      fileIndexPriv->currentSymbolString = fileIndexPriv->currentNode->string;
      break;
   case TA_TOK_CAT:
      fileIndexPriv->currentCategoryString = fileIndexPriv->currentNode->string;
      break;
   case TA_TOK_CATC:
      fileIndexPriv->currentCategoryCountryString = fileIndexPriv->currentNode->string;
      break;
   case TA_TOK_CATX:
      fileIndexPriv->currentCategoryExchangeString = fileIndexPriv->currentNode->string;
      break;
   case TA_TOK_CATT:
      fileIndexPriv->currentCategoryTypeString = fileIndexPriv->currentNode->string;
      break;

   default:
      /* Do nothing */
      break;
   }

   TA_StringFree( stringCache, newStr );

   if( retCode != TA_SUCCESS )
      return retCode;

   TA_FileIndexGoDownTreeValue( fileIndexPriv );

   return TA_SUCCESS;
}

static TA_RetCode extractTokenValueRecursive( TA_FileIndexPriv *fileIndexPriv,
                                              TA_PatternIter *iter,
                                              const char *string,
                                              unsigned int firstOfOneOrMore )
{
    TA_PROLOG
    int i, again;
    int patternChar, stringChar;
    /* int caseSensitive; */
    TA_RetCode retCode;
    TA_ValueTreeNode *currentNode;
    TA_PatternIter iterCopy;
    char tmpOneCharBuf[2];
    TA_TokenId curTokenId;

    TA_TRACE_BEGIN( extractTokenValueRecursive );

    firstOfOneOrMore = 0;

    tmpOneCharBuf[0] = '\0';
    tmpOneCharBuf[1] = '\0';

    /* caseSensitive = TA_IsFileSystemCaseSensitive(); */

    patternChar = getPatternChar( iter);

    /* ScratchPad used as temporary buffer. */
    fileIndexPriv->scratchPad[0] = '\0';
    again = 1;

    while( patternChar != '\0')
    {
        stringChar = *string;

        /* Keep track of the token id being processed. */
        curTokenId = iter->curToken->id;

        /* Move to the next character in the pattern. */
        nextPatternChar( iter);

        switch( patternChar )
        {
        case '?':
           /* Eat the character for the '?' */
           if( stringChar == '\0' )
           {
              TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
           }
           string++;

           /* Save the value in the "value tree". */
           tmpOneCharBuf[0] = (char)stringChar;
           retCode = addValueDown( fileIndexPriv, iter->curToken->id, &tmpOneCharBuf[0] );
           tmpOneCharBuf[0] = '\0';

           if( retCode != TA_SUCCESS )
           {
              TA_TRACE_RETURN( retCode );
           }

           break;

        case '|': /* One or more character. */

            /* This special pattern is used for fields like TA_TOK_SYM, TA_TOK_CAT etc... */
            if( firstOfOneOrMore != '\0' )
            {
               TA_TRACE_RETURN( TA_INTERNAL_ERROR(84) );
            }

            if( stringChar == '\0' )
            {
               TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
            }

            string++;

            /* Keep track of this character. It will be
             * concatenated with the character extracted from the
             * following '*' processing.
             */
            tmpOneCharBuf[0] = (char)stringChar;
            firstOfOneOrMore = stringChar;
            break;

        case '*':
            /* Replace by strlen !?!?!? */
            i = 0;
            while (string[i] != '\0')
               i++;

            if( getPatternChar(iter) == '\0' )
            {
               /* There is not pattern following, that means the '*'
                * is the last pattern on the line. Simply take the
                * remaining of the string.
                */
               again = 0; /* Indicates sub string resolved. */
            }
            else if( i != 0 )
            {
               again = 1;
               while( i  && again )
               {
                  /* Call recursively while saving/restoring the currentNode. */
                  currentNode = TA_FileIndexGetCurrentTreeValueNode( fileIndexPriv );
                  TA_ASSERT( currentNode != NULL );
                  TA_FileIndexGoDownTreeValue( fileIndexPriv );
                  iterCopy = *iter; /* Recursive call use their own iterator... */
                  retCode = extractTokenValueRecursive( fileIndexPriv, &iterCopy, ( const char *)&string[i-1], firstOfOneOrMore );
                  TA_FileIndexSetCurrentTreeValueNode( fileIndexPriv, currentNode );

                  if( retCode != TA_SUCCESS )
                  {
                     if( retCode == TA_NO_DATA_SOURCE )
                        i--;  /* Cannot resolve remaining of pattern. Try again. */
                     else
                        return retCode; /* Something wrong happen. */
                  }
                  else
                  {
                     again = 0; /* Exit the loop. Sub pattern resolved */
                     i--;
                  }
               }
            }

            /* The remaining of the pattern is resolved starting at 'string[i-1]'.
             * Put in scratchPad all the potential character forming the '*'
             * between string[0..i-1]
             *
             * (Concatenate to possibly already extracted firstOfOneChar when
             *  processing the '*' for a '|').
             */
            if( firstOfOneOrMore )
            {
               fileIndexPriv->scratchPad[0] = tmpOneCharBuf[0];
               strcpy( &fileIndexPriv->scratchPad[1], string );
               if( i == 0 )
                  fileIndexPriv->scratchPad[1] = '\0';
               else
                  fileIndexPriv->scratchPad[i+1] = '\0';
               firstOfOneOrMore = 0;
            }
            else
            {
               if( i == 0 )
                  fileIndexPriv->scratchPad[0] = '\0';
               else
               {
                  strcpy( fileIndexPriv->scratchPad, string );
                  fileIndexPriv->scratchPad[i] = '\0';
               }
            }

            /* Always do the writing even if empty string. */
            retCode = addValueDown( fileIndexPriv, curTokenId, fileIndexPriv->scratchPad );
            if( retCode != TA_SUCCESS )
            {
               TA_TRACE_RETURN( retCode );
            }

            if( again == 0 )
            {
               TA_TRACE_RETURN( TA_SUCCESS );
            }
            else
            {
               TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
            }

        default:
            /* Validate that it is a good character indeed... */
/*            if( caseSensitive )
            {
               if( stringChar != patternChar )
               {
                  TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
               }
            }
            else */
            if( toupper(stringChar) != toupper(patternChar) )
            {
               TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
            }

            string++;
            break;
        }

        patternChar = getPatternChar(iter);
    }

    /* Trap case where string is longer than the expected pattern... */
    if( *string != '\0' )
    {
        TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
    }

    /* Pattern correctly identify and all token extracted!
     * (except may be some '*' on the way back of the recursion...)
     */
    TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode addCurrentDataToIndex( TA_FileIndexPriv *fileIndexPriv,
                                         TA_ValueTreeNode *treeNodeValue )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_FileIndexCategoryData *addedCategory;
   TA_String *theCategoryString;
   char *tmpBuffer;
   unsigned int tmpBufferSize;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN(  addCurrentDataToIndex );
   TA_ASSERT( fileIndexPriv != NULL );

   stringCache = TA_GetGlobalStringCache( );

   if( fileIndexPriv->currentSymbolString == NULL )
   {
      TA_FATAL(  NULL, fileIndexPriv->currentSymbolString, fileIndexPriv->currentCategoryString );
   }

   /* Build the category string. */
   if( fileIndexPriv->currentCategoryCountryString ||
       fileIndexPriv->currentCategoryExchangeString ||
       fileIndexPriv->currentCategoryTypeString )
   {
      /* A partial category was provided. Complete the
       * missing component and build a category string.
       */
      if( !fileIndexPriv->currentCategoryCountryString )
         fileIndexPriv->currentCategoryCountryString = fileIndexPriv->initialCategoryCountryString;

      if( !fileIndexPriv->currentCategoryExchangeString )
         fileIndexPriv->currentCategoryExchangeString = fileIndexPriv->initialCategoryExchangeString;

      if( !fileIndexPriv->currentCategoryTypeString )
         fileIndexPriv->currentCategoryTypeString = fileIndexPriv->initialCategoryTypeString;

      tmpBufferSize  = strlen( TA_StringToChar(fileIndexPriv->currentCategoryCountryString) );
      tmpBufferSize += strlen( TA_StringToChar(fileIndexPriv->currentCategoryExchangeString) );
      tmpBufferSize += strlen( TA_StringToChar(fileIndexPriv->currentCategoryTypeString) );
      tmpBufferSize += 3;

      tmpBuffer = TA_Malloc( tmpBufferSize );
      if( !tmpBuffer )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      sprintf( tmpBuffer, "%s.%s.%s",
               TA_StringToChar(fileIndexPriv->currentCategoryCountryString),
               TA_StringToChar(fileIndexPriv->currentCategoryExchangeString),
               TA_StringToChar(fileIndexPriv->currentCategoryTypeString ));
               
      theCategoryString = TA_StringAlloc( stringCache, tmpBuffer );
      TA_Free(  tmpBuffer );
   }
   else if( !fileIndexPriv->currentCategoryString )
   {
      /* Use the default category. */
      theCategoryString = TA_StringDup( stringCache, fileIndexPriv->initialCategoryString );
   }
   else
      theCategoryString = TA_StringDup( stringCache, fileIndexPriv->currentCategoryString );

   retCode = TA_FileIndexAddCategoryData( fileIndexPriv,
                                          theCategoryString,
                                          &addedCategory );

   TA_StringFree( stringCache, theCategoryString );

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   retCode = TA_FileIndexAddSymbolData( addedCategory,
                                        fileIndexPriv->currentSymbolString,
                                        treeNodeValue, NULL );

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}
