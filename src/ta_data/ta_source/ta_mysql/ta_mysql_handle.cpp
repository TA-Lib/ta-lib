/* TA-LIB MySQL Data Source Driver Copyright (c) 2003, Pawel A. Konieczny
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
 *
 */

/* Description:
 *    Allows to allocate/de-allocate TA_DataSourceHandle structure.
 *    Executes SQL queries to build up the index of categories and symbols.
 */

/**** Headers ****/
extern "C" {
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_fileindex.h"
#include "ta_token.h"
#include "ta_global.h"
}
#include "ta_mysql_handle.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions.    ****/
static TA_PrivateMySQLHandle *allocPrivateHandle( void );
static TA_RetCode freePrivateHandle( TA_PrivateMySQLHandle *privateHandle );
static TA_RetCode freeCategoryIndex( void *toBeFreed );
static TA_RetCode freeSymbolsIndex( void *toBeFreed );
static TA_RetCode registerCategoryAndSymbol( TA_List *categoryIndex, TA_String *category, TA_String *symbol );
static TA_RetCode registerCategoryAndAllSymbols( TA_PrivateMySQLHandle *privateHandle, TA_String *category);

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

TA_DataSourceHandle *TA_MYSQL_DataSourceHandleAlloc( const TA_AddDataSourceParamPriv *param )
{
   TA_DataSourceHandle *handle;
   TA_PrivateMySQLHandle *privateHandle;

   TA_ASSERT_RET( param != NULL, (TA_DataSourceHandle *)NULL );
      
   handle = (TA_DataSourceHandle *)TA_Malloc(sizeof( TA_DataSourceHandle ));
   if( !handle )
      return (TA_DataSourceHandle *)NULL;

   /* Initialized fields. */
   handle->nbCategory = 0;

   /* Allocate the opaque data. */
   handle->opaqueData = allocPrivateHandle();
   if( !handle->opaqueData )
   {
      TA_MYSQL_DataSourceHandleFree( handle );
      return (TA_DataSourceHandle *)NULL;
   }

   privateHandle = (TA_PrivateMySQLHandle *)handle->opaqueData;
   privateHandle->param = param;

   return handle;
}

TA_RetCode TA_MYSQL_DataSourceHandleFree( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_PrivateMySQLHandle *privateHandle;

   if( !handle )
      return (TA_RetCode)TA_INTERNAL_ERROR(60);

   privateHandle = (TA_PrivateMySQLHandle *)handle->opaqueData;

   TA_TRACE_BEGIN(  TA_MYSQL_DataSourceHandleFree );

   if( handle )
   {
      if( freePrivateHandle( (TA_PrivateMySQLHandle *)handle->opaqueData ) != TA_SUCCESS )
      {
         TA_FATAL(  NULL, handle, 0 );
      }

      TA_Free( handle );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_BuildSymbolsIndex( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_RetCode retCode = TA_SUCCESS;
   TA_PrivateMySQLHandle *privateHandle;
   TA_StringCache *stringCache = TA_GetGlobalStringCache();

   if( !handle )
      return (TA_RetCode)TA_INTERNAL_ERROR(61);

   privateHandle = (TA_PrivateMySQLHandle *)handle->opaqueData;

   TA_TRACE_BEGIN(  TA_MYSQL_BuildSymbolsIndex );

   TA_ASSERT( privateHandle != NULL );
   TA_ASSERT( privateHandle->param != NULL );
   TA_ASSERT( privateHandle->param->category != NULL );
   TA_ASSERT( privateHandle->param->location != NULL );
   TA_ASSERT( privateHandle->con != NULL );

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
      try {
         Query query = privateHandle->con->query();
         // This creates a query object that is bound to con.

         query << TA_StringToChar(privateHandle->param->category);
         // You can write to the query object like you would any other ostrem

         Result res = query.store();
         // Query::store() executes the query and returns the results

         // find the category column number, if present
         for (unsigned int cat_col = 0; cat_col < res.columns(); cat_col++) 
         { 
            if( stricmp(res.names(cat_col).c_str(), TA_MYSQL_CATEGORY_COLUMN_NAME) == 0 )
               break;
         } 
         if( cat_col == res.columns() )
         {
            throw BadQuery("Column 'category' not found");
         }

         // find the symbol column number, if present
         for (unsigned int sym_col = 0; sym_col < res.columns(); sym_col++) 
         { 
            if( stricmp(res.names(sym_col).c_str(), TA_MYSQL_SYMBOL_COLUMN_NAME) == 0 )
               break;
         } 
         
         Row row;
         Result::iterator i;
         // The Result class has a read-only Random Access Iterator
         for (i = res.begin(); i != res.end(); i++) {
  	        row = *i;
            TA_String *cat_name = TA_StringAlloc( stringCache, row[cat_col] );
            TA_String *sym_name = NULL;

            if( !cat_name )
            {
               TA_TRACE_RETURN( TA_ALLOC_ERR );
            }

            if( strcmp(TA_StringToChar(cat_name), "") == 0 )  // fall back to default
            {
                  TA_StringFree(stringCache, cat_name);
                  cat_name = TA_StringAlloc( stringCache, TA_DEFAULT_CATEGORY );
                  if( !cat_name )
                  {
                     TA_TRACE_RETURN( TA_ALLOC_ERR );
                  }
            }

            if( sym_col < res.columns() )
            {
               sym_name = TA_StringAlloc( stringCache, row[sym_col] );

               if( !sym_name )
               {
                  TA_StringFree(stringCache, cat_name);
                  TA_TRACE_RETURN( TA_ALLOC_ERR );
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
      catch (BadQuery er)
      {                    
         // handle any connection or query errors that may come up
         retCode = TA_CATEGORY_NOT_FOUND;
      } 
   }
   else
   {
      /* Create one category, taking the category sting literally */
      retCode = registerCategoryAndAllSymbols(privateHandle, 
                                              privateHandle->param->category);
   }
   
   TA_TRACE_RETURN( retCode );
}


/* Copy templateStr to resultStr replacing holderStr by valueStr
 * Return value of expandPlaceholders is a string allocated by TA_Malloc.
 * It is the caller's responsibility to TA_Free it.
 */
char * TA_MYSQL_ExpandPlaceholders(const char *templateStr, 
                                 const char *holderStr, 
                                 const char *valueStr)
{
   size_t holderLength = strlen(holderStr);
   size_t valueLength = strlen(valueStr);
   size_t resSize;
   const char *pos, *backpos;
   char *resultStr;
   
   if ( !templateStr )
      return NULL;

   /* count all occurencies of placeholders, calculate the size of the result string */

   resSize = strlen(templateStr) + 1;
   for ( pos = strstr(templateStr, holderStr); 
         pos;  
         pos = strstr(pos+holderLength, holderStr) )
   {
      resSize -= holderLength;
      resSize += valueLength;
   }
   
   resultStr = (char*)TA_Malloc(resSize);
   
   if( resultStr )
   {
      /* do the replacement */
      resultStr[0] = '\0';

      for ( backpos = templateStr, pos = strstr(templateStr, holderStr); 
            pos;  
            backpos = pos+holderLength, pos = strstr(backpos, holderStr) )
      {
         /* copy the constant segment of the template */
         strncat(resultStr, backpos, pos-backpos);
         /* replace one placeholder */
         strcat(resultStr, valueStr);
      }
      
      /* remaining segment */
      strcat(resultStr, backpos);
   }

   return resultStr;
}


/**** Local functions definitions.     ****/

static TA_PrivateMySQLHandle *allocPrivateHandle( void  )
{
   TA_PrivateMySQLHandle *privateHandle;

   privateHandle = (TA_PrivateMySQLHandle *)TA_Malloc( sizeof( TA_PrivateMySQLHandle ) );
   if( !privateHandle )
      return NULL;

   memset( privateHandle, 0, sizeof( TA_PrivateMySQLHandle ) );

   return privateHandle;
}

static TA_RetCode freePrivateHandle( TA_PrivateMySQLHandle *privateHandle )
{
   TA_RetCode retCode = TA_SUCCESS;

   if( privateHandle )
   {
      if( privateHandle->theCategoryIndex )
      {
         retCode = TA_ListFreeAll(privateHandle->theCategoryIndex, &freeCategoryIndex);
         privateHandle->theCategoryIndex = NULL;
      }

      if( privateHandle->con )
      try
      {
         delete privateHandle->con;
         privateHandle->con = NULL;
      }
      catch (...)
      {
         retCode = TA_INTERNAL_ERR;
         privateHandle->con = NULL;
      }

      if( privateHandle->database )
      {
         TA_StringFree( TA_GetGlobalStringCache(), privateHandle->database );
      }

      TA_Free( privateHandle );
   }

   return retCode;
}

static TA_RetCode freeCategoryIndex( void *toBeFreed )
{
   TA_MySQLCategoryNode *node = (TA_MySQLCategoryNode*)toBeFreed;
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
   TA_MySQLCategoryNode *categoryNode;
   TA_RetCode retCode;

   if( !category )
      return TA_BAD_PARAM;

   /* Find out if the category is already registered */
   categoryNode = (TA_MySQLCategoryNode*)TA_ListAccessHead(categoryIndex);
   while ( categoryNode 
        && strcmp(TA_StringToChar(categoryNode->category), TA_StringToChar(category)) != 0)
   {
      categoryNode = (TA_MySQLCategoryNode*)TA_ListAccessNext(categoryIndex);
   }
 
   if( !categoryNode )
   {
      /* New category, allocate node for it */
      categoryNode = (TA_MySQLCategoryNode*)TA_Malloc(sizeof( TA_MySQLCategoryNode ));
      if( !categoryNode )
      {
         return TA_ALLOC_ERR;
      }
      memset(categoryNode, 0, sizeof( TA_MySQLCategoryNode ));
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
                                   TA_StringDup(TA_GetGlobalStringCache(), symbol) );
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
static TA_RetCode registerCategoryAndAllSymbols( TA_PrivateMySQLHandle *privateHandle,
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

      // Now the MySQL query
      char *sym_query = NULL;
      try {
         Query query = privateHandle->con->query();
         // This creates a query object that is bound to con.

         sym_query = TA_MYSQL_ExpandPlaceholders(TA_StringToChar(privateHandle->param->symbol),
                                                 TA_MYSQL_CATEGORY_PLACEHOLDER,
                                                 TA_StringToChar(category));
         if( !sym_query )
         {
            TA_TRACE_RETURN( TA_ALLOC_ERR );
         }

         query << sym_query;
         // You can write to the query object like you would any other ostrem

         Result res = query.store();
         // Query::store() executes the query and returns the results

         // find the symbol column number, if present
         for (unsigned int sym_col = 0; sym_col < res.columns(); sym_col++) 
         { 
            if( stricmp(res.names(sym_col).c_str(), TA_MYSQL_SYMBOL_COLUMN_NAME) == 0 )
               break;
         } 
         if( sym_col == res.columns() )
         {
            throw BadQuery("Column 'symbol' not found");
         }
         
         Row row;
         Result::iterator i;
         // The Result class has a read-only Random Access Iterator
         for (i = res.begin(); i != res.end(); i++) {
  	        row = *i;
            TA_String *sym_name = TA_StringAlloc( stringCache, row[sym_col] );

            if( !sym_name )
            {
               TA_TRACE_RETURN( TA_ALLOC_ERR );
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
      } 
      catch (BadQuery er)
      {                    
         // handle any connection or query errors that may come up
         retCode = TA_BAD_PARAM;  // I would prefer: TA_BAD_SQL_QUERY...
      }
      if ( sym_query )
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
      
   TA_TRACE_RETURN( retCode );
   
}


