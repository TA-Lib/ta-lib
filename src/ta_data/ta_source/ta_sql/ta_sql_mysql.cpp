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
 *  MF       Pawel Konieczny
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  110103 PK   First version.
 *  110103 PK   Minidriver architecture
 *
 */

/* Description:
 *    This file implements the SQL minidriver using MyQSL++ library
 *
 */

/**** Headers ****/
extern "C" {
#include <stdio.h>
#include <string.h>
#include "ta_system.h"
#include "ta_trace.h"
#include "ta_sql_minidriver.h"
#include "ta_sql_mysql.h"
}

#if defined(_MSC_VER)
/* disable some C++ warnings in MySQL++ for Microsoft compiler */
#pragma warning( disable: 4800 4355 )
#endif
#include <mysql++>

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/

typedef struct
{
#if !defined( TA_SINGLE_THREAD )
   TA_Sema sema;
#endif
   Connection *con;

} TA_SQL_MySQL_Connection;


/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.    ****/



TA_RetCode TA_SQL_MySQL_OpenConnection(const char database[], 
                                       const char host[], 
                                       const char username[], 
                                       const char password[],
                                       unsigned int port,
                                       void **connection)
{
   TA_PROLOG
   TA_SQL_MySQL_Connection *privConnection;
   TA_RetCode retCode = TA_SUCCESS;

   TA_TRACE_BEGIN( TA_SQL_MySQL_OpenConnection );

   privConnection = (TA_SQL_MySQL_Connection*)TA_Malloc( sizeof(TA_SQL_MySQL_Connection) );
   if( privConnection == NULL )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   memset(privConnection, 0, sizeof(TA_SQL_MySQL_Connection) );

#if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaInit( &privConnection->sema, 1);
   if( retCode != TA_SUCCESS )
   {
      TA_Free( privConnection );
      TA_TRACE_RETURN( retCode );
   }
#endif

   /* Establish the connection with the MySQL server */
   try 
   {
      privConnection->con = new Connection(
         database, 
         host, 
         username, 
         password, 
         port);
   }
   catch (...)
   {
#if !defined( TA_SINGLE_THREAD )
      TA_SemaDestroy( &privConnection->sema );
#endif
      TA_Free( privConnection );
      TA_TRACE_RETURN( TA_ACCESS_FAILED );
   }

   *connection = privConnection;

   TA_TRACE_RETURN( retCode );
}



TA_RetCode TA_SQL_MySQL_ExecuteQuery(void *connection, 
                           const char sql_query[], 
                           void **query_result)
{
   TA_PROLOG
   TA_SQL_MySQL_Connection *privConnection;
   TA_RetCode retCode = TA_SUCCESS;

   TA_TRACE_BEGIN( TA_SQL_MySQL_ExecuteQuery );
   
   TA_ASSERT( connection != NULL );

   privConnection = (TA_SQL_MySQL_Connection*)connection;
   *query_result = NULL;

#if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaWait( &privConnection->sema );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }
#endif

   try 
   {
      // This creates a query object that is bound to con.
      Query query = privConnection->con->query();

      // You can write to the query object like you would any other ostrem
      query << sql_query;

      // Query::store() executes the query and returns the results
      *query_result = new Result(query.store());
   }
   catch (...)
   {                    
      // handle any connection or query errors that may come up
      if( *query_result )
      {
         delete *query_result;
         *query_result = NULL;
      }
      retCode = TA_BAD_PARAM;  // should be TA_BAD_QUERY
   } 

#if !defined( TA_SINGLE_THREAD )
   TA_SemaPost( &privConnection->sema );
#endif

   TA_TRACE_RETURN( retCode );
}



TA_RetCode TA_SQL_MySQL_GetNumColumns(void *query_result, int *num)
{
   TA_PROLOG

   Result *res = (Result*)query_result;
   
   TA_TRACE_BEGIN( TA_SQL_MySQL_GetNumColumns );

   TA_ASSERT( res != NULL );
   TA_ASSERT( num != NULL );

   try
   {
      *num = res->columns();
   }
   catch (...)
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERR );  
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}



TA_RetCode TA_SQL_MySQL_GetNumRows(void *query_result, int *num)
{
   TA_PROLOG

   Result *res = (Result*)query_result;
   
   TA_TRACE_BEGIN( TA_SQL_MySQL_GetNumRows );

   TA_ASSERT( res != NULL );
   TA_ASSERT( num != NULL );

   try
   {
      *num = res->rows();
   }
   catch (...)
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERR );  
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}



TA_RetCode TA_SQL_MySQL_GetColumnName(void *query_result, int column, const char **name)
{
   TA_PROLOG

   Result *res = (Result*)query_result;
   
   TA_TRACE_BEGIN( TA_SQL_MySQL_GetColumnName );

   TA_ASSERT( res != NULL );
   TA_ASSERT( name != NULL );

   try
   {
      *name = res->names(column).c_str();
   }
   catch (...)
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERR );  
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}



TA_RetCode TA_SQL_MySQL_GetRowString(void *query_result, int row, int column, char **value)
{
   TA_PROLOG

   Result *res = (Result*)query_result;
   
   TA_TRACE_BEGIN( TA_SQL_MySQL_GetRowString );

   TA_ASSERT( res != NULL );
   TA_ASSERT( value != NULL );

   *value = NULL;
   try
   {
      Row res_row = (*res)[row];

      size_t len = strlen(res_row[column]);
      *value = (char*)TA_Malloc(len+1);
      if( *value == NULL )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );  
      }
      strcpy(*value, res_row[column]);
   }
   catch (...)
   {
      if( *value )
      {
         TA_Free( *value );
         *value = NULL;
      }
      TA_TRACE_RETURN( TA_INTERNAL_ERR );  
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}



TA_RetCode TA_SQL_MySQL_GetRowReal(void *query_result, int row, int column, TA_Real *value)
{
   TA_PROLOG

   Result *res = (Result*)query_result;
   
   TA_TRACE_BEGIN( TA_SQL_MySQL_GetRowReal );

   TA_ASSERT( res != NULL );
   TA_ASSERT( value != NULL );

   try
   {
      Row res_row = (*res)[row];
      *value = res_row[column];
   }
   catch (...)
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERR );  
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}



TA_RetCode TA_SQL_MySQL_GetRowInteger(void *query_result, int row, int column, TA_Integer *value)
{
   TA_PROLOG

   Result *res = (Result*)query_result;
   
   TA_TRACE_BEGIN( TA_SQL_MySQL_GetRowInteger );

   TA_ASSERT( res != NULL );
   TA_ASSERT( value != NULL );

   try
   {
      Row res_row = (*res)[row];
      *value = res_row[column];
   }
   catch (...)
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERR );  
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}



TA_RetCode TA_SQL_MySQL_ReleaseQuery(void *query_result)
{
   TA_PROLOG

   Result *res = (Result*)query_result;
   
   TA_TRACE_BEGIN( TA_SQL_MySQL_ReleaseQuery );
   
   if( res )
   {
      try
      {
         delete res;
      }
      catch (...)
      {
         TA_TRACE_RETURN( TA_INTERNAL_ERR );  
      }
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}



TA_RetCode TA_SQL_MySQL_CloseConnection(void *connection)
{
   TA_PROLOG
   TA_SQL_MySQL_Connection *privConnection;
   TA_RetCode retCode = TA_SUCCESS;

   TA_TRACE_BEGIN( TA_SQL_MySQL_CloseConnection );

   privConnection = (TA_SQL_MySQL_Connection*)connection;
   if ( privConnection )
   {
      try 
      {
         delete privConnection->con;
      }
      catch (...)
      {
         retCode = TA_INVALID_HANDLE;
      }
#if !defined( TA_SINGLE_THREAD )
      TA_SemaDestroy( &privConnection->sema );
#endif
      TA_Free( privConnection );
   }

   TA_TRACE_RETURN( retCode );
}



/**** Local functions definitions.     ****/
/* None */



