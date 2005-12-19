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
 *  102001 MF   First version.
 *
 */

/* Description:
 *    Simple end-user utility for using the TA-LIB web page
 *    fetch engine.
 */

/**** Headers ****/
#include <string.h>
#include "ta_libc.h"
#include "ta_network.h"

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
/* None */

/**** Global functions definitions.   ****/

/* A little utility to fetch a web page and send the raw data
 * to the provided FILE pointer. This ptr could be "stdout" to
 * display on the console.
 *
 * Example:
 *        TA_WebFetch( "www.yahoo.com", stdout );
 *           or
 *        TA_WebFetch( "http://www.yahoo.com/mt", myFile );
 */
TA_RetCode TA_WebFetch( const char *url, FILE *out )
{
   TA_RetCode retCode;
   TA_WebPage *webPage;

   const char *stringStart;
   const char *webSitePage;
   char *allocString;

   unsigned int webSiteLength;

   if( !url )
      return TA_BAD_PARAM;

   allocString = NULL;

   /* Skip the http:// if specified. */
   stringStart = url;
   if( strncmp( url, "http://", 7 ) == 0 )
      stringStart += 7;
   
   /* Find if there is a specifc web page specified. */
   webSitePage = strchr( stringStart, '/' );
   if( webSitePage )
   {
      webSitePage++;
      if( *webSitePage == '\0' )
      {
         webSitePage = NULL;         
      }
      else
      {
         webSiteLength = webSitePage - stringStart - 1;
         allocString = (char *)TA_Malloc( webSiteLength+1 );
         if( !allocString )
            return TA_ALLOC_ERR;
         strncpy( allocString, stringStart, webSiteLength );
         allocString[webSiteLength] = '\0';
         stringStart = allocString;
      }
   }

   retCode = TA_WebPageAlloc( stringStart,
                              webSitePage,
                              NULL, NULL,
                              &webPage,
                              2, NULL );

   if( allocString )
      TA_Free(  allocString );

   if( retCode == TA_SUCCESS )
   {
      retCode = TA_StreamToFile( webPage->content, out );
      TA_WebPageFree( webPage );
   }

   return retCode;
}

/**** Local functions definitions.     ****/
/* None */
