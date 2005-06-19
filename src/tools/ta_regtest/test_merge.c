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
 *  061805 MF   Add test_AsciiExample
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
static ErrorNumber test_AsciiExample( TA_UDBase *udBase );

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

   /* Free and re-alloc just to detect potential memory leak. */
   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    

   /* Test merging of ASCII files mrgX.csv where X are 1,2,3 and 4 */
   retValue = test_AsciiExample( udb );
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

static ErrorNumber test_AsciiExample( TA_UDBase *udBase )
{
   TA_RetCode retCode;
   TA_AddDataSourceParam addParam;
   TA_HistoryAllocParam allocParam;
   TA_History *history;
   TA_Timestamp curTs;
   double open, high, low, close;
   double tmp1, tmp2;
   int i;

   /* Add the 4 ASCII files. See tools\gen_data\mrg_gen.pl for 
    * a lengthy description of what is expected when these
    * 4 files are merged.
    */
   memset( &addParam, 0, sizeof(TA_AddDataSourceParam) );
   addParam.id = TA_ASCII_FILE; 
   addParam.location = "..\\src\\tools\\gen_data\\[SYM]1.csv";
   addParam.info = "[M][D][Y][O][H][L][C][V][OI]"; 
   retCode = TA_AddDataSource(udBase, &addParam);
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_ASCII_ADDFAILED_1;
   }

   addParam.location = "..\\src\\tools\\gen_data\\[SYM]2.csv";
   retCode = TA_AddDataSource(udBase, &addParam);
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_ASCII_ADDFAILED_2;
   }

   addParam.location = "..\\src\\tools\\gen_data\\[SYM]3.csv";
   retCode = TA_AddDataSource(udBase, &addParam);
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_ASCII_ADDFAILED_3;
   }

   addParam.location = "..\\src\\tools\\gen_data\\[SYM]4.csv";   
   retCode = TA_AddDataSource(udBase, &addParam);
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_ASCII_ADDFAILED_4;
   }
   
   memset( &allocParam, 0, sizeof(TA_HistoryAllocParam) );
   allocParam.field = TA_ALL;
   allocParam.symbol = "mrg";
   allocParam.period = TA_DAILY;
   retCode = TA_HistoryAlloc( udBase, &allocParam, &history );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_ASCII_HISTALLOC;
   }

   /* Check that the expected pattern is returned. Again,
    * See tools\gen_data\mrg_gen.pl for a description of the
    * pattern. 
    */
   if( history->nbBars != 1000 )
   {
      printf( "\nNumber of price bar 1000 != %d", history->nbBars );
      return TA_TSTMERGE_ASCII_BAD_PATTERN_1;
   }
   
   low   = 1000.0001;
   open  = 2000.0001;
   close = 3000.0001;
   high  = 4000.0001;
   TA_TimestampCopy(&curTs, &history->timestamp[0] );
   for( i=0; i < 1000; i++ )
   {
      if( history->volume[i] != (10000+i) )
      {
         printf( "\nBad volume at index %d. %d != %d\n", i, history->volume[i], (10000+i) );
         return TA_TSTMERGE_ASCII_BAD_PATTERN_VOL;
      }
      if( history->openInterest[i] != (20000+i) )
      {
         printf( "\nBad openInterest at index %d. %d != %d\n", i, history->openInterest[i], (20000+i) );
         return TA_TSTMERGE_ASCII_BAD_PATTERN_OI;
      }

      #define CHECK_PRICE( field, errorCode ) \
      { \
        tmp1 = ((int)((history->field[i]+0.00005)*10000.0))/10000.0;  \
        tmp2 = ((int)((field+0.00005)*10000.0))/10000.0; \
        if( tmp1 != tmp2 ) \
        { \
           printf( "\nBad price at index %d. %f != %f\n", i, tmp1, tmp2 ); \
           return errorCode; \
        } \
        field += 1.0001; \
      }
      
      CHECK_PRICE( open, TA_TSTMERGE_ASCII_BAD_PATTERN_OPEN );
      CHECK_PRICE( high, TA_TSTMERGE_ASCII_BAD_PATTERN_HIGH );
      CHECK_PRICE( low,  TA_TSTMERGE_ASCII_BAD_PATTERN_LOW );
      CHECK_PRICE( close,TA_TSTMERGE_ASCII_BAD_PATTERN_CLOSE );      
      #undef CHECK_PRICE

      if( !TA_TimestampEqual(&history->timestamp[i], &curTs ) )
      {
         printf( "\nBad timestamp at index %d.\n", i );
         return TA_TSTMERGE_ASCII_BAD_PATTERN_TS;
      }
      TA_NextDay(&curTs);
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTest failed with retCode %d", retCode );
      return TA_TSTMERGE_ASCII_HISTFREE;
   }

   return TA_TEST_PASS;
}

