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
 * Prototypes of auxiliary functions, local to ta_sql driver
 */

/**** Headers ****/
#include <stdlib.h>
#include <string.h>
#include "ta_trace.h"
#include "ta_memory.h"
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
/* None */

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

/* Extract host, port, and database name from location URL 
 * IN: location string
 * OUT: scheme (array must be preallocated to the size at least the length of location)
 * OUT: host (array must be preallocated to the size at least the length of location)
 * OUT: port (0 when not specified)
 * OUT: database (array must be preallocated to the size at least the length of location)
 */
TA_RetCode TA_SQL_ParseLocation(const char location[],
                                char scheme[],
                                char host[],
                                unsigned int *port,
                                char database[])
{
   TA_PROLOG
   const char *b;

   TA_TRACE_BEGIN( TA_SQL_ParseLocation );

   TA_ASSERT( port != NULL );

   /* extract scheme */
   b = strchr(location, ':');
   if (!b)  /* no scheme */
   {
      TA_TRACE_RETURN( TA_INVALID_PATH );
   }
   memcpy(scheme, location, b - location);
   scheme[b - location] = '\0';
   b++;  /* skip ':' */
   if ( *b == '/' )
      b++;  /* skip first slash */

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



/* Copy templateStr to resultStr replacing holderStr by valueStr
 * Return value of expandPlaceholders is a string allocated by TA_Malloc.
 * It is the caller's responsibility to TA_Free it.
 */
char * TA_SQL_ExpandPlaceholders(const char *templateStr, 
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
/* None */
