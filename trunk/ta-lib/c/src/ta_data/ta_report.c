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
 *    Output a report representing the content of the unified database.
 *    This module serve also as a demontration on how the index of the
 *    unified database can be obtained.
 */

/**** Headers ****/
#include <stdio.h>
#include "ta_libc.h"
#include "ta_trace.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static TA_RetCode reportSymbol( TA_UDBase *unifiedDatabase,
                                FILE *out, TA_ReportFlag flags,
                                const char *categoryString,
                                unsigned int *nbSymbolTotal );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
TA_RetCode TA_Report( TA_UDBase *unifiedDatabase,
                      FILE *out,
                      TA_ReportFlag flags )
{
   TA_StringTable *table;
   TA_RetCode retCode;
   unsigned int i;

   unsigned int nbCategoryTotal;
   unsigned int nbSymbolTotal;

   nbCategoryTotal = 0;
   nbSymbolTotal   = 0;

   if( out == NULL )
      return TA_BAD_PARAM;

   /* Output an header */
   fprintf( out, "\nTA-LIB: Content of unified database\n" );

   /* Get all the category to iterate. */
   retCode = TA_CategoryTableAlloc( unifiedDatabase, &table );

   if( retCode == TA_SUCCESS )
   {
      for( i=0; i < table->size; i++ )
      {
         if( !flags || (flags & TA_REPORT_CATEGORY) )
            fprintf( out, "\nCategory [%s]\n", table->string[i] );

         if( !flags || (flags & TA_REPORT_SYMBOL) )
         {
            retCode = reportSymbol( unifiedDatabase, out, flags, table->string[i], &nbSymbolTotal );
            if( retCode != TA_SUCCESS )
            {
               TA_CategoryTableFree( table );
               return retCode;
            }
         }
      }

      nbCategoryTotal = table->size;

      TA_CategoryTableFree( table );
   }
   else
      return retCode;

   if( !flags || (flags & TA_REPORT_TOTAL) )
   {
      fprintf( out, "\n==============================================");
      fprintf( out, "\nTotal nb symbols   : %d", nbSymbolTotal );
      fprintf( out, "\nTotal nb categories: %d", nbCategoryTotal );
      fprintf( out, "\n==============================================");
   }

   /* Output a trailer. */
   fprintf( out, "\nTA-LIB: End of report\n" );

   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/
static TA_RetCode reportSymbol( TA_UDBase *unifiedDatabase,
                                FILE *out, TA_ReportFlag flags,
                                const char *categoryString,
                                unsigned int *nbSymbolTotal )
{
   TA_StringTable *table;
   TA_RetCode retCode;
   unsigned int i, nbSymbolInThisCategory;

   nbSymbolInThisCategory = 0;

   /* Get all the symbols to iterate. */
   retCode = TA_SymbolTableAlloc( unifiedDatabase, categoryString, &table );

   if( retCode == TA_SUCCESS )
   {
      for( i=0; i < table->size; i++ )
      {
         fprintf( out, "   [%s]\n", table->string[i] );
         nbSymbolInThisCategory++;
         *nbSymbolTotal = (*nbSymbolTotal)+1;
      }

      TA_SymbolTableFree( table );
   }
   else
      return retCode;

   if( !flags || (flags & TA_REPORT_TOTAL) )
   {
      fprintf( out, "Nb of symbol in [%s] = %d\n", categoryString,
               nbSymbolInThisCategory );
   }

   return TA_SUCCESS;
}


