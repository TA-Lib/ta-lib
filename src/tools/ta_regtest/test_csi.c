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
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  032004 MF   First version.
 *
 */

/* Description:
 *    Test the CSI data source.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
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
static ErrorNumber test_source( TA_UDBase *udb, TA_SourceId id, const char *symbol );

static ErrorNumber checkRangeSame( TA_UDBase           *udb,
                                   const TA_History    *historyRef,
                                   const TA_Timestamp  *start,
                                   const TA_Timestamp  *end,
                                   TA_Period            period,
                                   unsigned int         startIdx,
                                   unsigned int         nbPRiceBar,
                                   const char          *symbol );

/**** Global functions definitions.   ****/
ErrorNumber test_csi( void )
{
   ErrorNumber retValue;
   TA_UDBase *udb;

   printf( "Testing CSI data source\n" );

   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    

   retValue = test_source( udb, TA_CSI, "9" );
   if( retValue != TA_TEST_PASS )
   {       
      printf( "Error #%d\n", retValue );
      return retValue;
   }

   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    

   retValue = test_source( udb, TA_CSIM, "0" );
   if( retValue != TA_TEST_PASS )
   {       
      printf( "Error #%d\n", retValue );
      return retValue;
   }

   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}


/**** Local functions definitions.     ****/
static ErrorNumber test_source( TA_UDBase *udb, TA_SourceId id, const char *symbol )
{
   TA_RetCode retCode;
   TA_AddDataSourceParam param;
   TA_History *history;
   ErrorNumber errNumber;

   /* Add the CSI data source. */
   memset( &param, 0, sizeof( param ) );
   param.id = id;
   param.location = "..\\src\\tools\\ta_regtest\\csidata";

   retCode = TA_AddDataSource( udb, &param );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_AddDataSource", retCode );

      return TA_CSI_ADDDATASOURCE_FAILED;
   }

   retCode = TA_HistoryAlloc( udb, "CSI_ID", symbol,
                              TA_DAILY, 0, 0, TA_ALL,
                              &history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_CSI_HISTORYALLOC_1_FAILED;
   }

   if( history->nbBars != 29 )
   {
      printf( "Insufficient nbBars returned for ticker test (%d != 29)\n", history->nbBars );
      return TA_CSI_VALUE_1_FAILED;
   }

   if( !history->close || !history->open || !history->low || !history->high ||
       !history->timestamp || !history->volume )
   {
      return TA_CSI_FIELD_MISSING_1;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[0], &history->timestamp[0], TA_DAILY, 0, 1, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting first price bar only.\n" );
      return errNumber;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[1], &history->timestamp[1], TA_DAILY, 1, 1, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting second price bar only.\n" );
      return errNumber;
   }

   errNumber = checkRangeSame( udb, history,
                               &history->timestamp[history->nbBars-2],
                               &history->timestamp[history->nbBars-2],
                               TA_DAILY, history->nbBars-2, 1, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting before last price bar only.\n" );
      return errNumber;
   }
   
   errNumber = checkRangeSame( udb, history,
                               &history->timestamp[history->nbBars-1],
                               &history->timestamp[history->nbBars-1],
                               TA_DAILY, history->nbBars-1, 1, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting last price bar only.\n" );
      return errNumber;
   }
   
   errNumber = checkRangeSame( udb, history,
                               &history->timestamp[history->nbBars-9],
                               &history->timestamp[history->nbBars-1],
                               TA_DAILY, history->nbBars-9, 9, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting last 9 price bars only.\n" );
      return errNumber;
   }
   
   errNumber = checkRangeSame( udb, history, &history->timestamp[0], &history->timestamp[10], TA_DAILY, 0, 11, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting first 11 price bars only.\n" );
      return errNumber;
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_CSI_HISTORYFREE_FAILED;
   }

   /* Do again the same test, but using Monthly data this time. */      
   retCode = TA_HistoryAlloc( udb, "CSI_ID", symbol,
                              TA_MONTHLY, 0, 0, TA_ALL,
                              &history );

   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc for Monthly data", retCode );
      return TA_CSI_HISTORYALLOC_3_FAILED;
   }
   
   errNumber = checkRangeSame( udb, history, &history->timestamp[0], &history->timestamp[0], TA_MONTHLY, 0, 1, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting first price bar only. (Monthly)\n" );
      return errNumber;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[1], &history->timestamp[1], TA_MONTHLY, 1, 1, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting second price bar only. (Monthly)\n" );
      return errNumber;
   }

   errNumber = checkRangeSame( udb, history, &history->timestamp[history->nbBars-2], &history->timestamp[history->nbBars-2], TA_MONTHLY, history->nbBars-2, 1, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting before last price bar only. (Monthly)\n" );
      return errNumber;
   }
   
   errNumber = checkRangeSame( udb, history, &history->timestamp[history->nbBars-1], &history->timestamp[history->nbBars-1], TA_MONTHLY, history->nbBars-1, 1, symbol );
   if( errNumber != TA_TEST_PASS )
   {
      printf( "Failed: Test getting last price bar only. (Monthly)\n" );
      return errNumber;
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_CSI_HISTORYFREE_FAILED;
   }

   return TA_TEST_PASS;
}

static ErrorNumber checkRangeSame( TA_UDBase          *udb,
                                   const TA_History   *historyRef,
                                   const TA_Timestamp *start,
                                   const TA_Timestamp *end,      
                                   TA_Period           period,                             
                                   unsigned int        startIdx,
                                   unsigned int        nbPriceBar,
                                   const char         *symbol )
{
   TA_History *history;
   unsigned int i;
   TA_RetCode retCode;

   retCode = TA_HistoryAlloc( udb, "CSI_ID", symbol,
                              period, start, end, TA_ALL,
                              &history );

   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryAlloc", retCode );
      return TA_CSI_CRS_HISTORYALLOC_FAILED;
   }

   /* Check that the expected number of price bar is returned. */
   if( history->nbBars != nbPriceBar )
   {
      printf( "Failed: nbBars (received != requested)=(%d != %d)\n",
              history->nbBars, nbPriceBar );

      TA_HistoryFree( history );
      return TA_CSI_CRS_NBBARSBAD;
   }

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
         return TA_CSI_CRS_PRICEBARBAD;
      }
   }

   retCode = TA_HistoryFree( history );
   if( retCode != TA_SUCCESS )
   {
      reportError( "TA_HistoryFree", retCode );
      return TA_CSI_HISTORYFREE_FAILED;
   }

   return TA_TEST_PASS;
}
