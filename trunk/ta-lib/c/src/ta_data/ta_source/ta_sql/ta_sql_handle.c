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
 *  PK       Pawel Konieczny
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  101703 PK   First version.
 *  110103 PK   Minidriver architecture
 *
 */

/* Description:
 *    Allows to allocate/de-allocate TA_DataSourceHandle structure.
 *    Executes SQL queries to build up the index of categories and symbols.
 */

/**** Headers ****/
#include <string.h>
#include <ctype.h>
#include <string.h>
#include "ta_trace.h"
#include "ta_memory.h"
#include "ta_global.h"
#include "ta_sql_handle.h"
#include "ta_sql_local.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions.    ****/
static TA_PrivateSQLHandle *allocPrivateHandle( void );
static TA_RetCode freePrivateHandle( TA_PrivateSQLHandle *privateHandle );
static TA_RetCode freeCategoryIndex( void *toBeFreed );
static TA_RetCode freeSymbolsIndex( void *toBeFreed );
static TA_RetCode registerCategoryAndSymbol( TA_List *categoryIndex, TA_String *category, TA_String *symbol );
static TA_RetCode registerCategoryAndAllSymbols( TA_PrivateSQLHandle *privateHandle, TA_String *category);

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/


TA_DataSourceHandle *TA_SQL_DataSourceHandleAlloc( const TA_AddDataSourceParamPriv *param )
{
   TA_DataSourceHandle *handle;

   TA_ASSERT_RET( param != NULL, (TA_DataSourceHandle *)NULL );
      
   handle = (TA_DataSourceHandle *)TA_Malloc(sizeof( TA_DataSourceHandle ));
   if( !handle )
      return NULL;

   /* Initialized fields. */
   handle->nbCategory = 0;

   /* Allocate the opaque data. */
   handle->opaqueData = allocPrivateHandle();
   if( !handle->opaqueData )
   {
      TA_SQL_DataSourceHandleFree( handle );
      return NULL;
   }

   ((TA_PrivateSQLHandle *)handle->opaqueData)->param = param;

   return handle;
}



TA_RetCode TA_SQL_DataSourceHandleFree( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_PrivateSQLHandle *privateHandle;

   if( !handle )
      return (TA_RetCode)TA_INTERNAL_ERROR(60);

   TA_TRACE_BEGIN(  TA_SQL_DataSourceHandleFree );

   privateHandle = (TA_PrivateSQLHandle *)handle->opaqueData;

   if( handle )
   {
      if( freePrivateHandle( (TA_PrivateSQLHandle *)handle->opaqueData ) != TA_SUCCESS )
      {
         TA_FATAL(  NULL, handle, 0 );
      }

      TA_Free( handle );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}



TA_RetCode TA_SQL_BuildSymbolsIndex( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_RetCode retCode = TA_SUCCESS;
   TA_PrivateSQLHandle *privateHandle;
   TA_StringCache *stringCache = TA_GetGlobalStringCache();

   if( !handle )
      return (TA_RetCode)TA_INTERNAL_ERROR(61);

   TA_TRACE_BEGIN(  TA_SQL_BuildSymbolsIndex );

   privateHandle = (TA_PrivateSQLHandle *)handle->opaqueData;

   TA_ASSERT( privateHandle != NULL );
   TA_ASSERT( privateHandle->param != NULL );
   TA_ASSERT( privateHandle->param->category != NULL );
   TA_ASSERT( privateHandle->param->location != NULL );
   TA_ASSERT( privateHandle->connection != NULL );
   TA_ASSERT( TA_gSQLMinidriverTable[privateHandle->minidriver].executeQuery != NULL );

   /* De-allocate potentialy already existing category index. */
   if( privateHandle->theCategoryIndex != NULL )
   {
      retCode = TA_ListFreeAll(privateHandle->theCategoryIndex, &freeCategoryIndex);
      privateHandle->theCategoryIndex = NULL;
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
   }

   /* Allocate new category index. */
   privateHandle->theCategoryIndex = TA_ListAlloc();
   if( !privateHandle->theCategoryIndex )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   if( strnicmp("SELECT ", TA_StringToChar(privateHandle->param->category), 7) == 0)
   {
      /* This is an SQL query; execute it to obtain the list of categories */

      void *query_result;
      int res_columns, res_rows;
      int cat_col, sym_col;
      int rownum,

      retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].executeQuery)(
                  privateHandle->connection,
                  TA_StringToChar(privateHandle->param->category),
                  &query_result);
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
      

      /* from now on: the query has to be released upon premature return */
#define RETURN_ON_ERROR( rc )              \
         if( rc != TA_SUCCESS )           \
         {                                \
            (*TA_gSQLMinidriverTable[privateHandle->minidriver].releaseQuery)(query_result); \
            TA_TRACE_RETURN( rc );        \
         }

      /* find the category column number, if present */
      retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getNumColumns)(
                     query_result,
                     &res_columns );
      RETURN_ON_ERROR( retCode )
      
      for ( cat_col = 0; cat_col < res_columns; cat_col++) 
      { 
         const char *name;
         retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getColumnName)(
                              query_result,
                              cat_col,
                              &name );
         RETURN_ON_ERROR( retCode )
         if( stricmp(name, TA_SQL_CATEGORY_COLUMN) == 0 )
            break;
      } 
      if( cat_col == res_columns )
      {
         RETURN_ON_ERROR( TA_CATEGORY_NOT_FOUND )
      }

      /* find the symbol column number, if present */
      for ( sym_col = 0; sym_col < res_columns; sym_col++) 
      { 
         const char *name;
         retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getColumnName)(
                              query_result,
                              sym_col,
                              &name );
         RETURN_ON_ERROR( retCode )
         if( stricmp(name, TA_SQL_SYMBOL_COLUMN) == 0 )
            break;
      }

      /* iterate through all rows */
      retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getNumRows)(
                     query_result,
                     &res_rows );
      RETURN_ON_ERROR( retCode )

      if( res_rows == 0 )
      {                    
         RETURN_ON_ERROR( TA_CATEGORY_NOT_FOUND )
      } 
         
      for( rownum = 0;  rownum < res_rows;  rownum++) 
      {
         char *strval;
         TA_String *cat_name = NULL;
         TA_String *sym_name = NULL;

         retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getRowString)(
                              query_result,
                              rownum, 
                              cat_col,
                              &strval );
         RETURN_ON_ERROR( retCode )

         if( strval && strval[0] != '\0' )  
         {
            cat_name = TA_StringAlloc( stringCache, strval );
         }
         else // for NULL values fall back to default
         {
            cat_name = TA_StringAlloc( stringCache, TA_DEFAULT_CATEGORY );
         }

         if( strval )
         {
            TA_Free( strval );
         }

         if( !cat_name )
         {
            RETURN_ON_ERROR( TA_ALLOC_ERR )
         }

         if( sym_col < res_columns )  /* we can collect symbols as well */
         {
            retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getRowString)(
                                 query_result,
                                 rownum, 
                                 sym_col,
                                 &strval );
            if ( retCode != TA_SUCCESS )
            {
               TA_StringFree(stringCache, cat_name);
               RETURN_ON_ERROR( retCode )
            }
       
            if( strval ) 
            {
               if( strval[0] != '\0' )  /* not NULL value */
               {
                  sym_name = TA_StringAlloc( stringCache, strval );

                  if( !sym_name )
                  {
                     TA_Free( strval );
                     RETURN_ON_ERROR( TA_ALLOC_ERR )
                  }
               }
               TA_Free( strval );
            }
         }

         if ( sym_name )
         {
            retCode = registerCategoryAndSymbol(privateHandle->theCategoryIndex, 
                                                cat_name,
                                                sym_name);
         }
         else
         {
            retCode = registerCategoryAndAllSymbols(privateHandle,
                                                    cat_name);
         }

         TA_StringFree(stringCache, cat_name);
         if( sym_name )
            TA_StringFree(stringCache, sym_name);

         if( retCode != TA_SUCCESS )
         {
            break;
         }
      } 
   }
   else
   {
      /* Create one category, taking the category sting literally */
      retCode = registerCategoryAndAllSymbols(privateHandle, 
                                              privateHandle->param->category);
   }

#undef RETURN_ON_ERROR
   
   TA_TRACE_RETURN( retCode );
}



/**** Local functions definitions.     ****/

static TA_PrivateSQLHandle *allocPrivateHandle( void  )
{
   TA_PrivateSQLHandle *privateHandle;

   privateHandle = (TA_PrivateSQLHandle *)TA_Malloc( sizeof( TA_PrivateSQLHandle ) );
   if( !privateHandle )
      return NULL;

   memset( privateHandle, 0, sizeof( TA_PrivateSQLHandle ) );

   return privateHandle;
}



static TA_RetCode freePrivateHandle( TA_PrivateSQLHandle *privateHandle )
{
   TA_RetCode retCode = TA_SUCCESS;

   if( privateHandle )
   {
      if( privateHandle->database )
      {
         TA_StringFree( TA_GetGlobalStringCache(), privateHandle->database );
      }

      if( privateHandle->theCategoryIndex )
      {
         retCode = TA_ListFreeAll(privateHandle->theCategoryIndex, &freeCategoryIndex);
         privateHandle->theCategoryIndex = NULL;
      }

      if( privateHandle->connection )
      {
         if( TA_gSQLMinidriverTable[privateHandle->minidriver].closeConnection )
         {
            TA_RetCode retCode2 = 
               (*TA_gSQLMinidriverTable[privateHandle->minidriver].closeConnection)( privateHandle->connection );
            if( retCode2 != TA_SUCCESS )
            {
               retCode = retCode2;
            }
         }
         else
         {
            retCode = TA_INTERNAL_ERR;
         }
         privateHandle->connection = NULL;
      }

      TA_Free( privateHandle );
   }

   return retCode;
}



static TA_RetCode freeCategoryIndex( void *toBeFreed )
{
   TA_SQLCategoryNode *node = (TA_SQLCategoryNode*)toBeFreed;
   TA_RetCode retCode = TA_SUCCESS;

   if( !node )
      return TA_SUCCESS;

   if( node->category && retCode == TA_SUCCESS )
   {
      TA_StringFree( TA_GetGlobalStringCache(), node->category );
      node->category = NULL;
   }

   if( node->theSymbols && retCode == TA_SUCCESS )
   {
      retCode = TA_ListFreeAll( node->theSymbols, &freeSymbolsIndex);
      node->theSymbols = NULL;
   }
   
   if( retCode == TA_SUCCESS )
   {
      TA_Free(node);
   }

   return retCode;
}



static TA_RetCode freeSymbolsIndex( void *toBeFreed )
{
   TA_String *symbol = (TA_String*)toBeFreed;
   TA_RetCode retCode = TA_SUCCESS;

   if( symbol )
   {
      TA_StringFree( TA_GetGlobalStringCache(), symbol );
      symbol = NULL;
   }

   return retCode;
}



/* registerCategoryAndSymbol takes care of avoiding duplicates
 * The caller keeps ownership to passed parameters.
 * We will do dup here if needed.
 */
static TA_RetCode registerCategoryAndSymbol( TA_List *categoryIndex,
                                             TA_String *category, 
                                             TA_String *symbol )
{
   TA_SQLCategoryNode *categoryNode;
   TA_RetCode retCode;

   if( !category )
      return TA_BAD_PARAM;

   /* Find out if the category is already registered */
   categoryNode = (TA_SQLCategoryNode*)TA_ListAccessHead(categoryIndex);
   while ( categoryNode 
        && strcmp(TA_StringToChar(categoryNode->category), TA_StringToChar(category)) != 0)
   {
      categoryNode = (TA_SQLCategoryNode*)TA_ListAccessNext(categoryIndex);
   }
 
   if( !categoryNode )
   {
      /* New category, allocate node for it */
      categoryNode = (TA_SQLCategoryNode*)TA_Malloc(sizeof( TA_SQLCategoryNode ));
      if( !categoryNode )
      {
         return TA_ALLOC_ERR;
      }
      memset(categoryNode, 0, sizeof( TA_SQLCategoryNode ));
      retCode = TA_ListAddTail( categoryIndex, categoryNode );
      if( retCode != TA_SUCCESS )
      {
         TA_Free(categoryNode);
         return retCode;
      }
      categoryNode->category = TA_StringDup(TA_GetGlobalStringCache(), category);
   }

   /* Register symbol, if not yet registered */
   if( symbol )
   {
      /* Find out if the symbol is already registered */
      TA_String *known_symbol = (TA_String*)TA_ListAccessHead(categoryNode->theSymbols);
      while ( known_symbol 
           && strcmp(TA_StringToChar(known_symbol), TA_StringToChar(symbol)) != 0)
      {
         known_symbol = (TA_String*)TA_ListAccessNext(categoryNode->theSymbols);
      }

      if( !known_symbol )
      {
         /* New symbol, add it to the list */
         if( !categoryNode->theSymbols )
         {
            categoryNode->theSymbols = TA_ListAlloc();
            if( !categoryNode->theSymbols )
               return TA_ALLOC_ERR;
         }

         retCode = TA_ListAddTail( categoryNode->theSymbols, 
                                    (void*)TA_StringDup(TA_GetGlobalStringCache(), symbol) );
         if( retCode != TA_SUCCESS )
         {
            return retCode;
         }
      }
   }

   return TA_SUCCESS;
}



/* registerCategoryAndAllSymbols executes SQL query for the symbol and
 * registers all symbols in the same category
 */
static TA_RetCode registerCategoryAndAllSymbols( TA_PrivateSQLHandle *privateHandle,
                                                 TA_String *category)
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_StringCache *stringCache = TA_GetGlobalStringCache();

   /* is trace allowed through static fuctions? */
   TA_TRACE_BEGIN( registerCategoryAndAllSymbols );

   if( !category )
   {
      TA_TRACE_RETURN(TA_BAD_PARAM);
   }

   if( privateHandle->param->symbol 
       && strnicmp("SELECT ", TA_StringToChar(privateHandle->param->symbol), 7) == 0)
   {
      /* This is an SQL query; execute it to obtain the list of symbols */
      void *query_result;
      int res_columns, res_rows;
      int sym_col;
      int rownum;
      char *sym_query;
   
      /* Because the query may return no results, we must make sure that
       * at leas the category will be registered.
       */
      retCode = registerCategoryAndSymbol(privateHandle->theCategoryIndex, 
                                          category,
                                          NULL);
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN(retCode);
      }

      /* Now the SQL query */
      sym_query = TA_SQL_ExpandPlaceholders(TA_StringToChar(privateHandle->param->symbol),
                                            TA_SQL_CATEGORY_PLACEHOLDER,
                                            TA_StringToChar(category));
      if( !sym_query )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].executeQuery)(
                  privateHandle->connection,
                  sym_query,
                  &query_result);
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
      
      /* from now on: the query has to be released upon premature return */
#define RETURN_ON_ERROR( rc )              \
         if( rc != TA_SUCCESS )           \
         {                                \
            (*TA_gSQLMinidriverTable[privateHandle->minidriver].releaseQuery)(query_result); \
            TA_TRACE_RETURN( rc );        \
         }


      /* find the symbol column number, if present */
      retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getNumColumns)(
                     query_result,
                     &res_columns );
      RETURN_ON_ERROR( retCode )

      for ( sym_col = 0; sym_col < res_columns; sym_col++) 
      { 
         const char *name = NULL;

         retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getColumnName)(
                              query_result,
                              sym_col,
                              &name );
         RETURN_ON_ERROR( retCode )

         if( (stricmp(name, TA_SQL_SYMBOL_COLUMN) == 0) )
         {
            break;
         }
      }
      if( sym_col == res_columns )
      {
         RETURN_ON_ERROR( TA_BAD_PARAM )
         /* I would prefer: TA_BAD_QUERY */
      }

      /* iterate through all rows */
      retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getNumRows)(
                     query_result,
                     &res_rows );
      RETURN_ON_ERROR( retCode )
         
      for( rownum = 0;  rownum < res_rows;  rownum++) 
      {
         char *strval = NULL;
         TA_String *sym_name = NULL;

         retCode = (*TA_gSQLMinidriverTable[privateHandle->minidriver].getRowString)(
                              query_result,
                              rownum, 
                              sym_col,
                              &strval );
         RETURN_ON_ERROR( retCode )

         if( strval )
         {
            sym_name = TA_StringAlloc( stringCache, strval );
            TA_Free( strval );
         }

         if( !sym_name )
         {
            RETURN_ON_ERROR( TA_ALLOC_ERR )
         }

         if( strcmp(TA_StringToChar(sym_name), "") != 0 )  // ignore NULL fields
         {
            retCode = registerCategoryAndSymbol(privateHandle->theCategoryIndex, 
                                                category,
                                                sym_name);
         }
         TA_StringFree(stringCache, sym_name);

         if( retCode != TA_SUCCESS )
         {
            break;
         }
      }

      TA_Free(sym_query);
   }
   else if ( privateHandle->param->symbol
             && *TA_StringToChar(privateHandle->param->symbol) != '\0' )
   {
      /* Create one symbol, taking the symbol sting literally */
      retCode = registerCategoryAndSymbol(privateHandle->theCategoryIndex, 
                                          category,
                                          privateHandle->param->symbol);
   }
   else
   {
      /* Create one symbol, falling back to the database name */
      retCode = registerCategoryAndSymbol(privateHandle->theCategoryIndex, 
                                          category,
                                          privateHandle->database);
   }

#undef RETURN_ON_ERROR
      
   TA_TRACE_RETURN( retCode );   
}


