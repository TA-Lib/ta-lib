/* TA-LIB Copyright (c) 1999-2005, Mario Fortier
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
 *  082301 MF   First version.
 *  060605 MF   Add test_acExample
 *
 */

/* Description:
 *    Test the merging capability of two data source.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "ta_libc.h"
#include "ta_common.h"
#include "ta_test_priv.h"

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
static ErrorNumber test_refmerge( TA_UDBase *udBase );
static ErrorNumber test_acExample( TA_UDBase *udBase );

/**** Global functions definitions.   ****/
ErrorNumber test_datasource_merge( void )
{
   ErrorNumber retValue;
   TA_UDBase *udb;

   printf( "Testing multiple data source merging\n" );

   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    

   retValue = test_acExample( udb );
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nTest failed with value %d", retValue );
      return retValue;
   }

   retValue = test_refmerge( udb );
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nTest failed with value %d", retValue );
      return retValue;
   }

   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}


/**** Local functions definitions.     ****/
static ErrorNumber test_refmerge( TA_UDBase *udBase )
{
   (void)udBase;

   /* !!! Not implemented yet !!! */

   return TA_TEST_PASS;
}

static ErrorNumber test_acExample( TA_UDBase *udBase )
{
   TA_RetCode retCode;
   TA_AddDataSourceParam addParam;
   TA_HistoryAllocParam allocParam;
   TA_History *history;

   /* Test for Angelo Ciceri setup. */

   memset( &addParam, 0, sizeof(TA_AddDataSourceParam) );
   addParam.id = TA_ASCII_FILE; 
   addParam.location = "..\\src\\tools\\ta_regtest\\sampling\\[SYM]?.txt";
   addParam.info = "[-H=1][YYYY][MM][DD][-C=6][O][H][L][C][V][OI]"; 
   addParam.category = "IT.BI.FUND"; 
   retCode = TA_AddDataSource(udBase, &addParam);
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_AC_ADDFAILED_1;
   }

   addParam.location = "..\\src\\tools\\ta_regtest\\sampling\\[SYM].txt"; 
   addParam.info = "[DD][MM][YY][C]"; 
   retCode = TA_AddDataSource(udBase, &addParam); 
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_AC_ADDFAILED_2;
   }

   
   memset( &allocParam, 0, sizeof(TA_HistoryAllocParam) );
   allocParam.category= addParam.category;
   allocParam.field = TA_ALL;
   allocParam.symbol = "TestMergeAC";
   allocParam.period = TA_DAILY;
   retCode = TA_HistoryAlloc( udBase, &allocParam, &history );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_AC_HISTALLOC_1;
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_AC_HISTFREE_1;
   }

   return TA_TEST_PASS;
}