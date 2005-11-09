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
 *  061304 MF   Add TA_YAHOO_ONE_SYMBOL tests.
 */

/* Description:
 *    Test the Yahoo! data source.
 */

/**** Headers ****/
#include <ctype.h>
#include <string.h>

#include "ta_libc.h"
#include "ta_common.h"
#include "ta_test_priv.h"
#include "ta_system.h"

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
static ErrorNumber test_web( TA_UDBase *udb );
static ErrorNumber test_one_symbol( TA_UDBase *udb );

static ErrorNumber checkRangeSame( TA_UDBase          *udb,
                                   const TA_History    *historyRef,
                                   const TA_Timestamp  *start,
                                   const TA_Timestamp  *end,
                                   TA_Period            period,
                                   unsigned int         startIdx,
                                   unsigned int         nbPRiceBar );

/**** Global functions definitions.   ****/
ErrorNumber test_yahoo( void )
{
   ErrorNumber retValue;
   TA_UDBase *udb;

   printf( "Testing Yahoo! data source\n" );

   /* Test the TA_YAHOO_ONE_SYMBOL data source. */
   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    
   retValue = test_one_symbol( udb );
   if( retValue != TA_TEST_PASS )
   {       
      printf( "Error #%d TA_YAHOO_ONE_SYMBOL:\n", retValue );
      printf( "Note: If you do not care about Yahoo! data source\n" );
      printf( "      you can ignore the previous error messages.\n" );
   }
   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;
   
   /* Test the TA_YAHOO_WEB data source. */
   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    
   retValue = test_web( udb );
   if( retValue != TA_TEST_PASS )
   {       
      printf( "Error #%d TA_YAHOO_WEB:\n", retValue );
      printf( "Note: If you do not care about Yahoo! data source\n" );
      printf( "      you can ignore the previous error messages.\n" );
   }
   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}


/**** Local functions definitions.     ****/
static ErrorNumber test_web( TA_UDBase *udb )
{
   TA_RetCode retCode;
   TA_AddDataSourceParam param;
   TA_History *history;
   ErrorNumber errNumber;
   TA_HistoryAllocParam histParam;

   /* Add the Yaho! data source. */
   memset( &param, 0, sizeof( param ) );
   param.id = TA_YAHOO_WEB;
   param.location = "us;server=ichart7.finance.dcn.yahoo.com"; /* ichart7.finance.dcn.yahoo.com */

   retCode = TA_AddDataSource( udb, &param );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_AddDataSource", retCode );

      return TA_YAHOO_ADDDATASOURCE_USA_FAILED;
   }

   /* Get something from NASDAQ. */
   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.category = "US.NASDAQ.STOCK";
   histParam.symbol   = "MSFT";
   histParam.field    = TA_CLOSE|TA_TIMESTAMP|TA_VOLUME;
   histParam.period   = TA_DAILY;
   retCode = TA_HistoryAlloc( udb, &histParam, &history );

   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_YAHOO_HISTORYALLOC_1_FAILED;
   }

   if( history->nbBars < 3000 )
   {
      printf( "Insufficient nbBars returned for MSFT ticker test (%d < 3000)\n", history->nbBars );
      return TA_YAHOO_VALUE_1_FAILED;
   }

   if( !history->close || !history->timestamp || !history->volume )
   {
      return TA_YAHOO_FIELD_MISSING_1;
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_YAHOO_HISTORYFREE_FAILED;
   }

   /* Add canadian index. */
   param.id = TA_YAHOO_WEB;
   param.location = "ca";
   /*param.flags = TA_DO_NOT_SPLIT_ADJUST|TA_DO_NOT_VALUE_ADJUST;*/

   retCode = TA_AddDataSource( udb, &param );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_AddDataSource", retCode );

      return TA_YAHOO_ADDDATASOURCE_CAN_FAILED;
   }

   /* Get something from NYSE. */
   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.category = "US.NYSE.STOCK";
   histParam.symbol   = "IBM";
   histParam.field    = TA_OPEN;
   histParam.period   = TA_WEEKLY;
   retCode = TA_HistoryAlloc( udb, &histParam, &history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_YAHOO_HISTORYALLOC_2_FAILED;
   }

   if( history->nbBars < 2065 )
   {
      return TA_YAHOO_VALUE_2_FAILED;
   }

   if( !history->open )
   {
      return TA_YAHOO_FIELD_MISSING_2;
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_YAHOO_HISTORYFREE_FAILED;
   }

   /* Get something from canadian market. 
    * Also test stock using 200 price bar slice.
    */
   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.category = "CA.TSE.STOCK";
   histParam.symbol   = "NT";
   histParam.field    = TA_ALL,
   histParam.period   = TA_DAILY;
   retCode = TA_HistoryAlloc( udb, &histParam, &history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_YAHOO_HISTORYALLOC_3_FAILED;
   }

   if( history->nbBars < 700 )
   {
      return TA_YAHOO_VALUE_3_FAILED;
   }

   if( !history->open   ||
       !history->high   ||
       !history->low    ||
       !history->close  ||
       !history->volume ||
       !history->timestamp )
   {
      return TA_YAHOO_FIELD_MISSING_3;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[0], &history->timestamp[0], TA_DAILY, 0, 1 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting first price bar only.\n" );
      return errNumber;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[1], &history->timestamp[1], TA_DAILY, 1, 1 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting second price bar only.\n" );
      return errNumber;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[history->nbBars-2], &history->timestamp[history->nbBars-2], TA_DAILY, history->nbBars-2, 1 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting before last price bar only.\n" );
      return errNumber;
   }
   
   errNumber = checkRangeSame( udb, history, &history->timestamp[history->nbBars-1], &history->timestamp[history->nbBars-1], TA_DAILY, history->nbBars-1, 1 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting last price bar only.\n" );
      return errNumber;
   }
   
   errNumber = checkRangeSame( udb, history, &history->timestamp[history->nbBars-200], &history->timestamp[history->nbBars-1], TA_DAILY, history->nbBars-200, 200 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting last 200 price bars only.\n" );
      return errNumber;
   }
   
   errNumber = checkRangeSame( udb, history, &history->timestamp[0], &history->timestamp[199], TA_DAILY, 0, 200 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting first 200 price bars only.\n" );
      return errNumber;
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_YAHOO_HISTORYFREE_FAILED;
   }

   /* Do again the same test, but using Monthly data this time. */      
   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.category = "CA.TSE.STOCK";
   histParam.symbol   = "NT";
   histParam.field    = TA_ALL;
   histParam.period   = TA_MONTHLY;
   retCode = TA_HistoryAlloc( udb, &histParam, &history );

   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc for Monthly data", retCode );
      return TA_YAHOO_HISTORYALLOC_3_FAILED;
   }

   /* printf( "Nb Bars= %d\n", history->nbBars ); */
   
   errNumber = checkRangeSame( udb, history, &history->timestamp[0], &history->timestamp[0], TA_MONTHLY, 0, 1 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting first price bar only. (Monthly)\n" );
      return errNumber;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[1], &history->timestamp[1], TA_MONTHLY, 1, 1 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting second price bar only. (Monthly)\n" );
      return errNumber;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[history->nbBars-2], &history->timestamp[history->nbBars-2], TA_MONTHLY, history->nbBars-2, 1 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting before last price bar only. (Monthly)\n" );
      return errNumber;
   }
   
   errNumber = checkRangeSame( udb, history, &history->timestamp[history->nbBars-1], &history->timestamp[history->nbBars-1], TA_MONTHLY, history->nbBars-1, 1 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting last price bar only. (Monthly)\n" );
      return errNumber;
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_YAHOO_HISTORYFREE_FAILED;
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_one_symbol( TA_UDBase *udb )
{
   TA_RetCode retCode;
   TA_AddDataSourceParam param;
   TA_History *history;
   ErrorNumber errNumber;
   TA_HistoryAllocParam histParam;

#if 0
   /* Get KPN.AS 
    *
    * Around 9/5/2004 that symbol had a negative dividend returned from Yahoo!.
    * Un-comment this section of the code to test the TA_INVALID_NEGATIVE_DIVIDEND
    * return value.
    */
   memset( &param, 0, sizeof( param ) );
   param.id = TA_YAHOO_ONE_SYMBOL;
   param.info = "KPN.AS";
   retCode = TA_AddDataSource( udb, &param );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_AddDataSource", retCode );
      return TA_YAHOO_ADDDATASOURCE_KPN_AS_FAILED;
   }

   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.symbol   = "KPN.AS";
   histParam.field    = TA_CLOSE|TA_VOLUME;
   histParam.period   = TA_DAILY;
   retCode = TA_HistoryAlloc( udb, &histParam, &history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_YAHOO_HISTORYALLOC_KPN_AS_FAILED;
   } 
   TA_HistoryFree( history );
#endif

   /* Test with MSFT on NASDAQ stock. */
   memset( &param, 0, sizeof( param ) );
   param.id = TA_YAHOO_ONE_SYMBOL;
   param.info = "MSFT";
   param.category = "Whatever.US.NASDAQ.STOCK";
   param.symbol = "Whatever.MSFT";

   retCode = TA_AddDataSource( udb, &param );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_AddDataSource", retCode );

      return TA_YAHOO_ADDDATASOURCE_USA_FAILED;
   }

   /* Get something from NASDAQ. */
   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.category = "Whatever.US.NASDAQ.STOCK";
   histParam.symbol   = "Whatever.MSFT";
   histParam.field    = TA_CLOSE|TA_TIMESTAMP|TA_VOLUME;
   histParam.period   = TA_DAILY;
   retCode = TA_HistoryAlloc( udb, &histParam, &history );

   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_YAHOO_HISTORYALLOC_1_FAILED;
   }

   if( history->nbBars < 3000 )
   {
      printf( "Insufficient nbBars returned for MSFT ticker test (%d < 3000)\n", history->nbBars );
      return TA_YAHOO_VALUE_1_FAILED;
   }

   if( !history->close || !history->timestamp || !history->volume )
   {
      return TA_YAHOO_FIELD_MISSING_1;
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_YAHOO_HISTORYFREE_FAILED;
   }

   /* Add complete canadian index. */
   memset( &param, 0, sizeof( param ) );
   param.id = TA_YAHOO_WEB;
   param.location = "ca";
   retCode = TA_AddDataSource( udb, &param );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_AddDataSource", retCode );

      return TA_YAHOO_ADDDATASOURCE_CAN_FAILED;
   }

   /* Add NT. */
   memset( &param, 0, sizeof( param ) );
   param.id = TA_YAHOO_ONE_SYMBOL;
   param.info = "NT.TO";

   retCode = TA_AddDataSource( udb, &param );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_AddDataSource", retCode );

      return TA_YAHOO_ADDDATASOURCE_CAN_FAILED;
   }

   /* Get NT from TA_YAHOO_ONE_STMBOL data source. */
   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.symbol   = "NT.TO";   
   histParam.field    = TA_ALL,
   histParam.period   = TA_DAILY;
   retCode = TA_HistoryAlloc( udb, &histParam, &history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_YAHOO_HISTORYALLOC_3_FAILED;
   }

   if( history->nbBars < 700 )
   {
      return TA_YAHOO_VALUE_3_FAILED;
   }

   if( !history->open   ||
       !history->high   ||
       !history->low    ||
       !history->close  ||
       !history->volume ||
       !history->timestamp )
   {
      return TA_YAHOO_FIELD_MISSING_3;
   }

   /* Get NT from the TA_YAHOO_WEB and make sure they return the same data. */
   errNumber = checkRangeSame( udb, history,
                               &history->timestamp[history->nbBars-200],
                               &history->timestamp[history->nbBars-1],
                               TA_DAILY, history->nbBars-200, 200 );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting last 200 price bars only.\n" );
      return errNumber;
   }
   
   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_YAHOO_HISTORYFREE_FAILED;
   }
   
   return TA_TEST_PASS;
}

static ErrorNumber checkRangeSame( TA_UDBase          *udb,
                                   const TA_History   *historyRef,
                                   const TA_Timestamp *start,
                                   const TA_Timestamp *end,      
                                   TA_Period           period,                             
                                   unsigned int        startIdx,
                                   unsigned int        nbPriceBar )
{
   TA_History *history;
   unsigned int i;
   TA_RetCode retCode;
   TA_HistoryAllocParam histParam;

   int retry, again;
   /* Try up to 10 times to get the requested number of 
    * price bar (sometimes Yahoo! or the Web do fail to
    * return the data).
    */
   again = 1;
   for( retry=0; (retry < 10) && again; retry++ )
   {
      memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
      histParam.category = "CA.TSE.STOCK";
      histParam.symbol   = "NT";
      histParam.field    = TA_ALL;
      histParam.start    = *start;
      histParam.end      = *end;
      histParam.period   = period;
      retCode = TA_HistoryAlloc( udb, &histParam, &history );

      if( (retCode == TA_SUCCESS) && (history->nbBars == nbPriceBar) )
         again = 0;
      else
      {
         printf( "Warning: Yahoo! history alloc retry #%d of 10\n", retry+1 );
         if( retCode == TA_SUCCESS )
         {
            TA_HistoryFree( history );
         }
         TA_Sleep( 10 /* seconds */ );
      }
   }

   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_YAHOO_CRS_HISTORYALLOC_FAILED;
   }

   /* Check that the expected number of price bar is returned. */
   if( history->nbBars != nbPriceBar )
   {
      printf( "Failed: nbBars (received != requested)=(%d != %d)\n",
              history->nbBars, nbPriceBar );

      TA_HistoryFree( history );
      return TA_YAHOO_CRS_NBBARSBAD;
   }

   /* printf( "startIdx=%d\n", startIdx ); */

   /* Check that the data is the same for the range. */
   for( i=0; i < nbPriceBar; i++ )
   {
      if( !TA_TimestampEqual( &history->timestamp[i], &historyRef->timestamp[startIdx+i] ) ||
          (history->open[i] != historyRef->open[startIdx+i]) ||
          (history->high[i] != historyRef->high[startIdx+i]) ||
          (history->low[i] != historyRef->low[startIdx+i]) ||
          (history->close[i] != historyRef->close[startIdx+i]) ||
          (history->volume[i] != historyRef->volume[startIdx+i]) )
      {
         printf( "Failed: Price Bar value different\n" );
         printf( "Failed: Data = %d/%d/%d : %f,%f,%f,%f,%d\n",
                                             TA_GetMonth(&history->timestamp[i]),
                                             TA_GetDay(&history->timestamp[i]),
                                             TA_GetYear(&history->timestamp[i]),
                                             history->open[i],
                                             history->high[i],         
                                             history->low[i],
                                             history->close[i],
                                             history->volume[i] );
         printf( "Failed: Ref  = %d/%d/%d : %f,%f,%f,%f,%d\n",
                                             TA_GetMonth(&historyRef->timestamp[i]),
                                             TA_GetDay(&historyRef->timestamp[i]),
                                             TA_GetYear(&historyRef->timestamp[i]),
                                             historyRef->open[startIdx+i],
                                             historyRef->high[startIdx+i],     
                                             historyRef->low[startIdx+i],
                                             historyRef->close[startIdx+i],
                                             historyRef->volume[startIdx+i] );

         TA_HistoryFree( history );
         return TA_YAHOO_CRS_PRICEBARBAD;
      }
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_YAHOO_HISTORYFREE_FAILED;
   }

   return TA_TEST_PASS;
}
