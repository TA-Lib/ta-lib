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
 *
 */

/* Description:
 *    This is the entry points of the data source driver for MySQL database.
 *    It depend on the mysql++ library
 *
 *    It provides ALL the functions needed by the "TA_DataSourceDriver"
 *    structure (see ta_source.h).
 */

/**** Headers ****/
extern "C" {
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ta_source.h"
#include "ta_mysql.h"
#include "ta_common.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_list.h"
#include "ta_data.h"
#include "ta_system.h"
#include "ta_fileindex.h"
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

/**** Local functions declarations.    ****/
TA_RetCode TA_MYSQL_ParseLocation(const char location[],       
                                  char host[],
                                  unsigned int *port,
                                  char database[]);

const char *formatISODate(const TA_Timestamp *ts);
const char *formatISOTime(const TA_Timestamp *ts);


/**** Local variables definitions.     ****/
TA_FILE_INFO;

#if !defined( TA_SINGLE_THREAD )
TA_Sema mod_sema;
#endif

/**** Global functions definitions.   ****/
TA_RetCode TA_MYSQL_InitializeSourceDriver( void )
{
   TA_PROLOG
   TA_RetCode retCode = TA_SUCCESS;

   TA_TRACE_BEGIN(  TA_SQL_InitializeSourceDriver );

#if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaInit( &mod_sema, 1);
#endif

   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_MYSQL_ShutdownSourceDriver( void )
{
   TA_PROLOG
   TA_RetCode retCode = TA_SUCCESS;

   TA_TRACE_BEGIN(  TA_SQL_ShutdownSourceDriver );

#if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaDestroy( &mod_sema );
#endif

   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_MYSQL_GetParameters( TA_DataSourceParameters *param )
{
   TA_PROLOG

   TA_TRACE_BEGIN( TA_MYSQL_GetParameters );

   memset( param, 0, sizeof( TA_DataSourceParameters ) );

   // Parameters supported by TA_MYSQL
   param->flags = TA_REPLACE_ZERO_PRICE_BAR;

   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_MYSQL_OpenSource( const TA_AddDataSourceParamPriv *param,
                                TA_DataSourceHandle **handle )
{
   TA_PROLOG
   TA_DataSourceHandle *tmpHandle;
   TA_PrivateMySQLHandle *privData;
   char *host, *dbase;
   unsigned port;
   TA_RetCode retCode;

   *handle = NULL;

   TA_TRACE_BEGIN( TA_MYSQL_OpenSource );

   /* 'info' and 'location' are mandatory. */
   if( (!param->info) || (!param->location) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   /* get parameters for MySQL connection */
   host = (char*)TA_Malloc(strlen(TA_StringToChar(param->location)));
   if (host == NULL)
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   dbase = (char*)TA_Malloc(strlen(TA_StringToChar(param->location)));
   if (dbase == NULL)
   {
      TA_Free(host);
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   retCode = TA_MYSQL_ParseLocation(TA_StringToChar(param->location), host, &port, dbase);
   if( retCode != TA_SUCCESS )
   {
      TA_Free(host);
      TA_Free(dbase);
      TA_TRACE_RETURN( retCode );
   }

   /* Allocate and initialize the handle. This function will also allocate the
    * private handle (opaque data).
    */
   tmpHandle = TA_MYSQL_DataSourceHandleAlloc(param);

   if( tmpHandle == NULL )
   {
      TA_Free(host);
      TA_Free(dbase);
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   privData = (TA_PrivateMySQLHandle *)(tmpHandle->opaqueData);

   /* Establish the connection with the MySQL server */
   try 
   {
      privData->con = new Connection(
         dbase, 
         host, 
         TA_StringToChar(param->username), 
         TA_StringToChar(param->password), 
         port);
   }
   catch (...)
   {
      TA_Free(host);
      TA_Free(dbase);
      TA_MYSQL_DataSourceHandleFree( tmpHandle );
      TA_TRACE_RETURN( TA_ACCESS_FAILED );
   }

   /* Database name is a fallback symbol name when not available from parameters */
   privData->database = TA_StringAlloc(TA_GetGlobalStringCache(), dbase);
   TA_Free(host);
   TA_Free(dbase);
   if( !privData->database )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Now build the symbols index. */
   retCode = TA_MYSQL_BuildSymbolsIndex( tmpHandle );

   if( retCode != TA_SUCCESS )
   {
      TA_MYSQL_DataSourceHandleFree( tmpHandle );
      TA_TRACE_RETURN( retCode );
   }

   /* Set the total number of distinct categories. */
   tmpHandle->nbCategory = TA_ListSize(privData->theCategoryIndex);

   *handle = tmpHandle;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_CloseSource( TA_DataSourceHandle *handle )
{
   TA_PROLOG

   TA_TRACE_BEGIN(  TA_MYSQL_CloseSource );

   /* Free all ressource used by this handle. */
   if( handle )
      TA_MYSQL_DataSourceHandleFree( handle );

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetFirstCategoryHandle( TA_DataSourceHandle *handle,
                                            TA_CategoryHandle   *categoryHandle )
{
   TA_PROLOG
   TA_PrivateMySQLHandle *privData;
   TA_List               *categoryIndex;
   TA_MySQLCategoryNode  *categoryNode;

   TA_TRACE_BEGIN(  TA_MYSQL_GetFirstCategoryHandle );

   if( (handle == NULL) || (categoryHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateMySQLHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(49) );
   }

   categoryIndex = privData->theCategoryIndex;

   if( !categoryIndex )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(50) );
   }

   /* Get the first category from the category index */
   categoryNode = (TA_MySQLCategoryNode*)TA_ListAccessHead(categoryIndex);

   if( !categoryNode || !categoryNode->category )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(51) ); /* At least one category must exist. */
   }

   /* Set the categoryHandle. */
   categoryHandle->string = categoryNode->category;
   categoryHandle->nbSymbol = TA_ListSize(categoryNode->theSymbols);
   categoryHandle->opaqueData = categoryNode->theSymbols;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetNextCategoryHandle( TA_DataSourceHandle *handle,
                                           TA_CategoryHandle   *categoryHandle,
                                           unsigned int index )
{
   TA_PROLOG
   TA_PrivateMySQLHandle *privData;
   TA_List               *categoryIndex;
   TA_MySQLCategoryNode  *categoryNode;

   TA_TRACE_BEGIN(  TA_MYSQL_GetNextCategoryHandle );

   (void)index; /* Get rid of compiler warnings. */

   if( (handle == NULL) || (categoryHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateMySQLHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(52) );
   }

   categoryIndex = privData->theCategoryIndex;

   if( !categoryIndex )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(50) );
   }

   /* Get the next category from the category index */
   categoryNode = (TA_MySQLCategoryNode*)TA_ListAccessNext(categoryIndex);

   if( !categoryNode )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Set the categoryHandle. */
   categoryHandle->string = categoryNode->category;
   categoryHandle->nbSymbol = TA_ListSize(categoryNode->theSymbols);
   categoryHandle->opaqueData = categoryNode->theSymbols;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetFirstSymbolHandle( TA_DataSourceHandle *handle,
                                          TA_CategoryHandle   *categoryHandle,
                                          TA_SymbolHandle     *symbolHandle )
{
   TA_PROLOG
   TA_PrivateMySQLHandle *privData;
   TA_List               *symbolsIndex;
   TA_String             *symbol;

   TA_TRACE_BEGIN(  TA_MYSQL_GetFirstSymbolHandle );

   if( (handle == NULL) || (categoryHandle == NULL) || (symbolHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateMySQLHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(49) );
   }

   symbolsIndex = (TA_List*)categoryHandle->opaqueData;

   /* Get the first symbol in this category. */
   symbol = (TA_String*)TA_ListAccessHead(symbolsIndex);

   if( !symbol )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Set the symbolHandle. */
   symbolHandle->string = symbol;
   symbolHandle->opaqueData = NULL;  /* not needed */

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetNextSymbolHandle( TA_DataSourceHandle *handle,
                                         TA_CategoryHandle   *categoryHandle,
                                         TA_SymbolHandle     *symbolHandle,
                                         unsigned int index )
{
   TA_PROLOG
   TA_PrivateMySQLHandle *privData;
   TA_List               *symbolsIndex;
   TA_String             *symbol;

   TA_TRACE_BEGIN(  TA_MYSQL_GetNextSymbolHandle );

   (void)index; /* Get rid of compiler warnings. */

   if( (handle == NULL) || (categoryHandle == NULL) || (symbolHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateMySQLHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(57) );
   }

   symbolsIndex = (TA_List*)categoryHandle->opaqueData;

   /* Get the first symbol in this category. */
   symbol = (TA_String*)TA_ListAccessNext(symbolsIndex);

   if( !symbol )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Set the symbolHandle. */
   symbolHandle->string = symbol;
   symbolHandle->opaqueData = NULL;  /* not needed */

   TA_TRACE_RETURN( TA_SUCCESS );
}




TA_RetCode TA_MYSQL_GetHistoryData( TA_DataSourceHandle *handle,
                                    TA_CategoryHandle   *categoryHandle,
                                    TA_SymbolHandle     *symbolHandle,
                                    TA_Period            period,
                                    const TA_Timestamp  *start,
                                    const TA_Timestamp  *end,
                                    TA_Field             fieldToAlloc,
                                    TA_ParamForAddData  *paramForAddData )
{
   TA_PROLOG
   TA_RetCode retCode = TA_SUCCESS;
   TA_PrivateMySQLHandle *privateHandle;
   unsigned int time_required = (period < TA_DAILY );    /* Boolean */

   TA_TRACE_BEGIN(  TA_MYSQL_GetHistoryData );

   TA_ASSERT( handle != NULL );

   if (  (start && TA_TimestampValidate(start) != TA_SUCCESS)
      || (end && TA_TimestampValidate(end) != TA_SUCCESS))
       return TA_BAD_PARAM;

   privateHandle = (TA_PrivateMySQLHandle *)handle->opaqueData;
   TA_ASSERT( privateHandle != NULL );

   TA_ASSERT( paramForAddData != NULL );
   TA_ASSERT( categoryHandle != NULL );
   TA_ASSERT( symbolHandle != NULL );

   /* result vectors */
   TA_Timestamp *timestamp_vec = NULL;
   TA_Real *open_vec = NULL;
   TA_Real *high_vec = NULL;
   TA_Real *low_vec = NULL;
   TA_Real *close_vec = NULL;
   TA_Integer *volume_vec = NULL;
   TA_Integer *oi_vec = NULL;

#if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaWait( &mod_sema );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN(retCode);
   }
#endif

   // Now the MySQL query
   char *queryStr = NULL;
   try 
   {
      Query query = privateHandle->con->query();
      // This creates a query object that is bound to con.

      // Replace all relevant placeholders
      char *tempStr;
      queryStr = TA_MYSQL_ExpandPlaceholders(TA_StringToChar(privateHandle->param->info),
                                             TA_MYSQL_CATEGORY_PLACEHOLDER,
                                             TA_StringToChar(categoryHandle->string));

      tempStr = queryStr;
      queryStr = TA_MYSQL_ExpandPlaceholders(tempStr,
                                             TA_MYSQL_SYMBOL_PLACEHOLDER,
                                             TA_StringToChar(symbolHandle->string));
      if ( tempStr)
         TA_Free( tempStr );

      tempStr = queryStr;
      queryStr = TA_MYSQL_ExpandPlaceholders(tempStr,
                                             TA_MYSQL_START_DATE_PLACEHOLDER,
                                             formatISODate(start));

      if ( tempStr)
         TA_Free( tempStr );

      TA_Timestamp true_end;
      if ( end )
      {
         true_end = *end;
      }
      else /* default value 0 means no upper bound */
      {
         TA_SetDate(9999,12,31,&true_end);  /* need some high value to substitute in SQL */
         TA_SetTime(23,59,59,&true_end);
      }
      tempStr = queryStr;
      queryStr = TA_MYSQL_ExpandPlaceholders(tempStr,
                                             TA_MYSQL_END_DATE_PLACEHOLDER,
                                             formatISODate(&true_end));

      if ( tempStr)
         TA_Free( tempStr );

      tempStr = queryStr;
      queryStr = TA_MYSQL_ExpandPlaceholders(tempStr,
                                             TA_MYSQL_START_TIME_PLACEHOLDER,
                                             formatISOTime(start));

      if ( tempStr)
         TA_Free( tempStr );

      tempStr = queryStr;
      queryStr = TA_MYSQL_ExpandPlaceholders(tempStr,
                                             TA_MYSQL_END_TIME_PLACEHOLDER,
                                             formatISOTime(&true_end));

      if ( tempStr)
         TA_Free( tempStr );

      if( !queryStr )
      {
         throw TA_ALLOC_ERR;
      }

      query << queryStr;
      // You can write to the query object like you would any other ostrem

      Result res = query.store();
      // Query::store() executes the query and returns the results

      // find recognized columns
      unsigned int date_col, time_col, open_col, high_col, low_col, close_col, volume_col, oi_col;
      date_col = time_col = open_col = high_col = low_col = close_col = volume_col = oi_col = UINT_MAX;

      for (unsigned int col = 0; col < res.columns(); col++) 
      { 
         if( stricmp(res.names(col).c_str(), TA_MYSQL_DATE_COLUMN_NAME) == 0 )
            date_col = col;
         else
         if( stricmp(res.names(col).c_str(), TA_MYSQL_TIME_COLUMN_NAME) == 0 )
            time_col = col;
         else
         if( stricmp(res.names(col).c_str(), TA_MYSQL_OPEN_COLUMN_NAME) == 0 )
            open_col = col;
         else
         if( stricmp(res.names(col).c_str(), TA_MYSQL_HIGH_COLUMN_NAME) == 0 )
            high_col = col;
         else
         if( stricmp(res.names(col).c_str(), TA_MYSQL_LOW_COLUMN_NAME) == 0 )
            low_col = col;
         else
         if( stricmp(res.names(col).c_str(), TA_MYSQL_CLOSE_COLUMN_NAME) == 0 )
            close_col = col;
         else
         if( stricmp(res.names(col).c_str(), TA_MYSQL_VOLUME_COLUMN_NAME) == 0 )
            volume_col = col;
         else
         if( stricmp(res.names(col).c_str(), TA_MYSQL_OI_COLUMN_NAME) == 0 )
            oi_col = col;
      } 

      // verify whether all required columns are present
      if( (time_required && time_col == UINT_MAX ) )
      {
         throw TA_PERIOD_NOT_AVAILABLE;
      }
      if(  date_col == UINT_MAX
        || (fieldToAlloc & TA_OPEN   && open_col   == UINT_MAX)
        || (fieldToAlloc & TA_HIGH   && high_col   == UINT_MAX)
        || (fieldToAlloc & TA_LOW    && low_col    == UINT_MAX)
        || (fieldToAlloc & TA_CLOSE  && close_col  == UINT_MAX)
        || (fieldToAlloc & TA_VOLUME && volume_col == UINT_MAX)
        || (fieldToAlloc & TA_OPENINTEREST && oi_col == UINT_MAX) )
      {
         throw BadQuery("Required column not found");
      }
      
      // Preallocate vectors memory
      if ( !(timestamp_vec = (TA_Timestamp*)TA_Malloc( res.rows() * sizeof(TA_Timestamp))))
            throw TA_ALLOC_ERR;
      memset(timestamp_vec, 0, res.rows() * sizeof(TA_Timestamp));

      if ( open_col != UINT_MAX 
         && (fieldToAlloc & TA_OPEN || fieldToAlloc == TA_ALL)
         && !(open_vec = (TA_Real*)TA_Malloc( res.rows() * sizeof(TA_Real) )))
            throw TA_ALLOC_ERR;

      if ( high_col != UINT_MAX 
         && (fieldToAlloc & TA_HIGH || fieldToAlloc == TA_ALL)
         && !(high_vec = (TA_Real*)TA_Malloc( res.rows() * sizeof(TA_Real) )))
            throw TA_ALLOC_ERR;

      if ( low_col != UINT_MAX 
         && (fieldToAlloc & TA_LOW || fieldToAlloc == TA_ALL)
         && !(low_vec = (TA_Real*)TA_Malloc( res.rows() * sizeof(TA_Real) )))
            throw TA_ALLOC_ERR;

      if ( close_col != UINT_MAX 
         && (fieldToAlloc & TA_CLOSE || fieldToAlloc == TA_ALL)
         && !(close_vec = (TA_Real*)TA_Malloc( res.rows() * sizeof(TA_Real) )))
            throw TA_ALLOC_ERR;

      if ( volume_col != UINT_MAX 
         && (fieldToAlloc & TA_VOLUME || fieldToAlloc == TA_ALL)
         && !(volume_vec = (TA_Integer*)TA_Malloc( res.rows() * sizeof(TA_Integer) )))
            throw TA_ALLOC_ERR;

      if ( oi_col != UINT_MAX 
         && (fieldToAlloc & TA_OPENINTEREST || fieldToAlloc == TA_ALL)
         && !(oi_vec = (TA_Integer*)TA_Malloc( res.rows() * sizeof(TA_Integer) )))
            throw TA_ALLOC_ERR;

      if (fieldToAlloc != TA_ALL)
         fieldToAlloc |= TA_TIMESTAMP;   /* timestamp is always needed */

      /* When the TA_REPLACE_ZERO_PRICE_BAR flag is set, we must
       * always process the close.
       */
      if( privateHandle->param->flags & TA_REPLACE_ZERO_PRICE_BAR )
      {   
         fieldToAlloc |= TA_CLOSE;
      }

      Row row;
      Result::iterator i;
      int bar;
      // The Result class has a read-only Random Access Iterator
      for (i = res.begin(), bar = 0; i != res.end(); i++, bar++) 
      { 
  	     row = *i;
         unsigned int u1, u2, u3;

         // date must be always present
         if ( sscanf(row[date_col], "%4u-%2u-%2u", &u1, &u2, &u3) != 3 )
            throw TA_BAD_PARAM;  // other error?

         if ( TA_SetDate(u1, u2, u3, &timestamp_vec[bar]) != TA_SUCCESS )
            throw TA_BAD_PARAM;

         if (time_required)
         {
            const char *timestr = row[time_col];
            
            if ( timestr && *timestr) {  
               if (sscanf(row[time_col], "%2u:%2u:%2u", &u1, &u2, &u3) != 3 )
                  throw TA_BAD_PARAM;

               if ( TA_SetTime(u1, u2, u3, &timestamp_vec[bar]) != TA_SUCCESS )
                   throw TA_BAD_PARAM;
            }
            else { // ignore NULL fields
               bar--;
               continue;
            }
         }

         #define TA_SQL_STORE_VALUE(type,vec,col,flag)              \
            if (vec) {                                         \
               vec[bar]   = row[col];                          \
               if (vec[bar] == 0 && (privateHandle->param->flags & flag) ) \
                  vec[bar] = (type) ( (bar > 0)? close_vec[bar-1] : 0 ); \
               if (vec[bar] == 0) {                            \
                  bar--;                                       \
                  continue;                                    \
               }                                               \
            }
         
         TA_SQL_STORE_VALUE(TA_Real, open_vec, open_col, TA_REPLACE_ZERO_PRICE_BAR)
         TA_SQL_STORE_VALUE(TA_Real, high_vec, high_col, TA_REPLACE_ZERO_PRICE_BAR)
         TA_SQL_STORE_VALUE(TA_Real, low_vec, low_col, TA_REPLACE_ZERO_PRICE_BAR)
         TA_SQL_STORE_VALUE(TA_Real, close_vec, close_col, TA_REPLACE_ZERO_PRICE_BAR)
         TA_SQL_STORE_VALUE(TA_Integer, volume_vec, volume_col, TA_NO_FLAGS)
         TA_SQL_STORE_VALUE(TA_Integer, oi_vec, oi_col, TA_NO_FLAGS)

         #undef TA_SQL_STORE_VALUE
      }
      // uff... done! 
      // now pass collected data to history module
      retCode = TA_HistoryAddData( paramForAddData,
                              bar,
                              period,
                              timestamp_vec,
                              open_vec,
                              high_vec,
                              low_vec,
                              close_vec,
                              volume_vec,
                              oi_vec );

      // whatever happened, the vectors are not belonging to us anymore
      timestamp_vec = NULL;
      open_vec = high_vec = low_vec = close_vec = NULL;
      volume_vec = oi_vec = NULL;
   } 
   catch (BadQuery er)
   {                    
      // handle any connection or query errors that may come up
      retCode = TA_BAD_PARAM;  // I would prefer: TA_BAD_SQL_QUERY...
   }
   catch (TA_RetCode rc)
   {
      retCode = rc;
   }

#if !defined( TA_SINGLE_THREAD )
   if( retCode != TA_SUCCESS )
   {
      TA_SemaPost( &mod_sema );
   }
   else
   {
      retCode = TA_SemaPost( &mod_sema );
   }
#endif

   /* cleanup */
   if ( queryStr      ) TA_Free(queryStr);
   if ( timestamp_vec ) TA_Free(timestamp_vec);
   if ( open_vec      ) TA_Free(open_vec);
   if ( high_vec      ) TA_Free(high_vec);
   if ( low_vec       ) TA_Free(low_vec);
   if ( close_vec     ) TA_Free(close_vec);
   if ( volume_vec    ) TA_Free(volume_vec);
   if ( oi_vec        ) TA_Free(oi_vec);

   TA_TRACE_RETURN( retCode );
}

/**** Local functions definitions.     ****/

/* Extract host, port, and database name from location URL 
 * IN: location string
 * OUT: host (array must be preallocated to the size at least the length of location)
 * OUT: port (0 when not specified)
 * OUT: database (array must be preallocated to the size at least the length of location)
 */
TA_RetCode TA_MYSQL_ParseLocation(const char location[],       
                                  char host[],
                                  unsigned int *port,
                                  char database[])
{
   TA_PROLOG
   const char *b;

   TA_TRACE_BEGIN( TA_MYSQL_ParseLocation );

   /* check whether location starts with mysql: scheme */ 
   if (strncmp(location, "mysql:/", 7) != 0)
   {
      TA_TRACE_RETURN( TA_INVALID_PATH );
   }

   b = &location[7];  /* begin of position to parse */
   host[0] = '\0';
   if (*b == '/')
   {
      /* host name specified */
      const char *e = strchr(b+1, '/');  /* end of host part */
      const char *p = strchr(b+1, ':');  /* position of port number */

      if (!e)  /* no database name */
      {
         TA_TRACE_RETURN( TA_INVALID_PATH );
      }

      if (p && p < e)  /* port specified */
      {
         *port = (unsigned)atoi(p+1);
         e = p;
      }
      else
      {
         *port = 0;
      }
      b++;
      memcpy(host, b, e - b);
      host[e - b] = '\0';
      b = strchr(b, '/') + 1;
   }
   strcpy(database, b);

   TA_TRACE_RETURN( TA_SUCCESS );
}


/* Format TA_Timestamp to ISO date as used by SQL
 * Not multithread-safe ;-)
 */
const char *formatISODate(const TA_Timestamp *ts)
{
   static char str[11];  /* CCYY-MM-DD\0 */

   sprintf(str, "%04u-%02u-%02u", TA_GetYear(ts), TA_GetMonth(ts), TA_GetDay(ts));
   return str;
}

const char *formatISOTime(const TA_Timestamp *ts)
{
   static char str[9];  /* hh:mm:ss\0 */

   sprintf(str, "%02u:%02u:%02u", TA_GetHour(ts), TA_GetMin(ts), TA_GetSec(ts));
   return str;
}
