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
 *
 */

/* Description:
 *    This file helps to make abstraction of the SQL access library
 *
 *    It defines basically two global dispatch table for calling
 *    low-level database access functions
 *
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include "ta_trace.h"
#include "ta_sql_minidriver.h"
#include "ta_sql_mysql.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
const TA_SQL_Minidriver TA_gSQLMinidriverTable[] =
{
   {
      "odbc",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
   },
   {
      "mysql",
#ifdef TA_SUPPORT_MYSQL
      TA_SQL_MySQL_OpenConnection,
      TA_SQL_MySQL_ExecuteQuery,
      TA_SQL_MySQL_GetNumColumns,
      TA_SQL_MySQL_GetNumRows,
      TA_SQL_MySQL_GetColumnName,
      TA_SQL_MySQL_GetRowString,
      TA_SQL_MySQL_GetRowReal,
      TA_SQL_MySQL_GetRowInteger,
      TA_SQL_MySQL_ReleaseQuery,
      TA_SQL_MySQL_CloseConnection
#else
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
#endif
   }
};

const unsigned int TA_gSQLMinidriverTableSize = (sizeof(TA_gSQLMinidriverTable)/sizeof(TA_SQL_Minidriver));

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_SQL_MinidriverID TA_SQL_IdentifyMinidriver(const char scheme[])
{
   unsigned int index;

   TA_ASSERT_RET( TA_gSQLMinidriverTableSize == TA_SQL_NUM_OF_MINIDRIVERS,  TA_SQL_NUM_OF_MINIDRIVERS);

   for( index = 0;  index < TA_gSQLMinidriverTableSize;  index++)
   {
      if( strcmp( scheme, TA_gSQLMinidriverTable[index].scheme ) == 0 )
      {
         break;
      }
   }

   return (TA_SQL_MinidriverID)index;
}

/**** Local functions definitions.     ****/
/* None */

