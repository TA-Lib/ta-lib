/* TA-LIB Copyright (c) 1999-2004, Mario Fortier
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
 *  MF       Mario Fortier
 *  JS       Jon Sudul
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  110103 PK    First version.
 *  112203 MF    Allows dynamic addition of mini drivers.
 *  012504 MF,JS Fix memory leak for Bug #881950.
 */

/* Description:
 *    This file helps to make abstraction of the SQL access library
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include "ta_trace.h"
#include "ta_sql_minidriver.h"
#include "ta_dict.h"
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
static TA_Dict *minidriverDict = NULL;

/**** Global functions definitions.   ****/

/* Allows to dynamically add a mini-driver. */
TA_RetCode TA_SQL_AddMinidriver( const char scheme[], const TA_SQL_Minidriver *minidriver )
{
   TA_PROLOG
   TA_String *schemeStr;
   TA_StringCache *cache;

   TA_RetCode retCode;

   TA_TRACE_BEGIN( TA_SQL_AddMinidriver );

   if( !minidriverDict )
   {
      minidriverDict = TA_DictAlloc( TA_DICT_KEY_ONE_STRING, NULL );
      if( !minidriverDict )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }
   }

   cache = TA_GetGlobalStringCache();
   TA_ASSERT( cache != NULL );
   schemeStr = TA_StringAlloc( cache, scheme );
   if( !schemeStr )
   {
      TA_DictFree(minidriverDict);
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   retCode = TA_DictAddPair_S( minidriverDict, schemeStr, (void *)minidriver );
   TA_StringFree( cache, schemeStr );

   TA_TRACE_RETURN( retCode );
}

/* Get the minidriver for the specified scheme */
const TA_SQL_Minidriver *TA_SQL_GetMinidriver( const char scheme[] )
{
   /* Return NULL if scheme not found, or dictionary not initialized. */
   return (TA_SQL_Minidriver *)TA_DictGetValue_S( minidriverDict, scheme);
}

/* Clean-up function called on TA_Shutdown */
void TA_SQL_ShutdownMinidriver( void )
{
   if( minidriverDict )
      TA_DictFree( minidriverDict );
}

/**** Local functions definitions.     ****/
/* None */

