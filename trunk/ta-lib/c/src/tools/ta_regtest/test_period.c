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
 *  PK       Pawel Konieczny
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  070401 MF   First version.
 *  062203 MF   Add intra-day to daily test using the "ES.CSV" file.
 *  082404 MF   Adapt to new date logic for price bar. Now the
 *              date represent the begining of an interval instead
 *              of the end.
 *  062105 PK   Added end-of-period tests.
 */

/* Description:
 *         Regression testing of the functionality provided
 *         by the file ta_period.c
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ta_test_priv.h"
#include "ta_utility.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   const char *symbol;
   TA_Period period;
   TA_HistoryFlag flags;
   unsigned int nbExpectedPriceBar;
   TA_Integer thePriceBarToCheck;
   unsigned int expected_year;
   unsigned int expected_month;
   unsigned int expected_day;
   unsigned int expected_hour;
   unsigned int expected_min;
   unsigned int expected_sec;
   TA_Real expected_open;
   TA_Real expected_high;
   TA_Real expected_low;
   TA_Real expected_close;
   TA_Integer expected_volume;
   TA_Integer expected_openInterest;
} TA_PriceBarCheck;

typedef struct
{
   TA_Timestamp *start;
   TA_Timestamp *end;
   unsigned int  expectedDelta;
} WeekDayCheck;

/**** Local functions declarations.    ****/
static ErrorNumber compare_period_series( TA_History *history1, TA_History *history2 );
static ErrorNumber validate_end_of_period_series_intraday( TA_History *historyBop, TA_History *historyEop, TA_Period period );
static ErrorNumber validate_end_of_period_series_dailyplus( TA_History *historyBop, TA_History *historyEop, TA_Period period );
static ErrorNumber testTimestampDelta( void );

/**** Local variables definitions.     ****/
const char *fail_header = "Fail: test_period,Test #";

static TA_PriceBarCheck checkTable[] = 
{

  { "ES", TA_WEEKLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
     1,   0, 2003,  5, 18,   0,  0,  0,  921.50,  925.75,  911.25,  920.00,  (713627+765554)/2, 0 },

  { "ES", TA_WEEKLY, 0,
    0, 0, 0,  0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0, 0 },

  { "ES", TA_5MINS, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    482, 258, 2003,  5, 21,   0,  0,  0,  920.75,  920.75,  920.25,  920.50,  44, 0 },

  { "ES", TA_5MINS, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    482,   0, 2003,  5, 20,   0, 45,  0,  921.50,  922.00,  921.50,  922.00,  12, 0 },

  { "ES", TA_5MINS, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    482, 257, 2003,  5, 20,  22, 55,  0,  920.50,  920.50,  920.50,  920.50,   1, 0 },

  { "ES", TA_5MINS, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    482, 481, 2003,  5, 21,  19, 15,  0,  920.25,  920.25,  920.00,  920.00,  19, 0 },

  { "ES", TA_DAILY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    2,   1, 2003,  5, 21,   0,  0,  0,  920.75,  924.25,  913.25,  920.00,  765554, 0 },

  { "ES", TA_DAILY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    2,   0, 2003,  5, 20,   0,  0,  0,  921.50,  925.75,  911.25,  920.50,  713627, 0 },

  { "ES", TA_MONTHLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    1,   0, 2003,  5,  1,   0,  0,  0,  921.50,  925.75,  911.25,  920.00,  (713627+765554)/2, 0 },

  { "ES", TA_QUARTERLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    1,   0, 2003,  4,  1,   0,  0,  0,  921.50,  925.75,  911.25,  920.00,  (713627+765554)/2, 0 },
 
  { "ES", TA_YEARLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    1,   0, 2003,  1,  1,   0,  0,  0,  921.50,  925.75,  911.25,  920.00,  (713627+765554)/2, 0 },


  { "DAILY_REF_0", TA_WEEKLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    52,   0, 1999,  1,  3,   0,  0,  0,  92.500,  96.375,  90.750,  93.780,  4511420, 0 },

  { "DAILY_REF_0", TA_WEEKLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    52,   1, 1999,  1, 10,   0,  0,  0,  94.500,  95.000,  89.440,  92.470,  3973160, 0 },

  { "DAILY_REF_0", TA_WEEKLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    52,  50, 1999, 12, 19,   0,  0,  0,  109.060, 110.440, 107.750, 108.620,  4539425, 0 },

  { "DAILY_REF_0", TA_WEEKLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    52,  51, 1999, 12, 26,   0,  0,  0,  109.690, 110.750, 106.620, 107.870,  3363920, 0 },

  { "DAILY_REF_0", TA_MONTHLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    12,   0, 1999,  1,  1,   0,  0,  0,  92.500,  99.625,  86.750,  91.625,  6494478, 0 },

  { "DAILY_REF_0", TA_MONTHLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    12,   1, 1999,  2,  1,   0,  0,  0,  92.250,  92.250,  80.875,  84.875,  5347410, 0 },

  { "DAILY_REF_0", TA_MONTHLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    12,  11, 1999, 12,  1,   0,  0,  0,  102.560, 122.120, 102.250, 107.870,  7213263, 0 },

  { "DAILY_REF_0", TA_QUARTERLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    4,   0, 1999,  1,  1,   0,  0,  0,  92.500,  99.625,  80.875,  88.625,  5580475, 0 },

  { "DAILY_REF_0", TA_QUARTERLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    4,   1, 1999,  4,  1,   0,  0,  0,  88.655, 132.000,  81.500, 129.250,  6024341, 0 },

  { "DAILY_REF_0", TA_QUARTERLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    4,   2, 1999,  7,  1,   0,  0,  0,  130.000, 139.190, 117.560, 121.000,  6397878, 0 },

  { "DAILY_REF_0", TA_QUARTERLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    4,   3, 1999, 10,  1,   0,  0,  0,  121.000, 123.250,  89.000, 107.870, 10614132, 0 },

  { "DAILY_REF_0", TA_YEARLY, TA_ALLOW_INCOMPLETE_PRICE_BARS,
    1,   0, 1999,  1,  1,   0,  0,  0,  92.500, 139.190,  80.875, 107.870,  7177425, 0 }

};
#define CHECK_TABLE_SIZE (sizeof(checkTable)/sizeof(TA_PriceBarCheck))

static TA_Timestamp sundayTS,  mondayTS,  tuesdayTS,  wednesdayTS,  thursdayTS,  fridayTS,  saturdayTS,
                    sunday2TS, monday2TS, tuesday2TS, wednesday2TS, thursday2TS, friday2TS, saturday2TS;                    
static WeekDayCheck toCheck[] = { 
   { &sundayTS, &sundayTS, 0 },
   { &sundayTS, &mondayTS, 1 },
   { &sundayTS, &tuesdayTS, 2 },
   { &sundayTS, &wednesdayTS, 3 },
   { &sundayTS, &thursdayTS, 4 },
   { &sundayTS, &fridayTS, 5 },
   { &sundayTS, &saturdayTS, 5 },
   { &sundayTS, &sunday2TS, 5 },
   { &sundayTS, &monday2TS, 6 },
   { &sundayTS, &tuesday2TS, 7 },
   { &sundayTS, &wednesday2TS, 8 },
   { &sundayTS, &thursday2TS, 9 },
   { &sundayTS, &friday2TS, 10 },
   { &sundayTS, &saturday2TS, 10 },

   { &mondayTS, &sundayTS, 1}, /* Test #14 */
   { &mondayTS, &mondayTS, 1 },
   { &mondayTS, &tuesdayTS, 2 },
   { &mondayTS, &wednesdayTS, 3 },
   { &mondayTS, &thursdayTS, 4 },
   { &mondayTS, &fridayTS, 5 },
   { &mondayTS, &saturdayTS, 5 },
   { &mondayTS, &sunday2TS, 5 },
   { &mondayTS, &monday2TS, 6 },
   { &mondayTS, &tuesday2TS, 7 },
   { &mondayTS, &wednesday2TS, 8 },
   { &mondayTS, &thursday2TS, 9 },
   { &mondayTS, &friday2TS, 10 },
   { &mondayTS, &saturday2TS, 10 } };

#define NB_WEEKDAY_CHECK_TO_DO (sizeof(toCheck)/sizeof(WeekDayCheck))

/**** Global functions definitions.   ****/
/* None */

/**** Local functions definitions.     ****/
ErrorNumber test_period( TA_UDBase *unifiedDatabase )
{
   TA_RetCode retCode;
   TA_History *history;
   const TA_Timestamp *timestamp;
   unsigned int i;
   ErrorNumber retValue;
   TA_AddDataSourceParam addParam;
   TA_HistoryAllocParam histParam;

   printf( "Testing period/timeframe conversion\n" );

   /* Add access to some intra-day data. */
   memset( &addParam, 0, sizeof( TA_AddDataSourceParam) );
   addParam.id = TA_ASCII_FILE;
   addParam.location = "..\\src\\tools\\ta_regtest\\sampling\\ES.csv";
   addParam.info ="[YY][MM][DD][HH][MN=5][O][H][L][C][V]";
   addParam.category = "TA_SIM_REF";
   retCode = TA_AddDataSource( unifiedDatabase,&addParam );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't access [%s] (%d)\n", addParam.location, retCode );
      return TA_PERIOD_HISTORYALLOC_FAILED;
   }

   /* Because period transformation are very
    * dependent on the "delta" timestamp functions,
    * let's independently verify some of these.
    */
   retValue = testTimestampDelta();
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Verify everything from the checkTable. */
   for( i=0; i < CHECK_TABLE_SIZE; i++ )
   {
      /* Validate by requesting all the data in a
       * different timeframe.
       */
      memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
      histParam.category = "TA_SIM_REF";
      histParam.symbol   = checkTable[i].symbol;
      histParam.field    = TA_ALL;
      histParam.period   = checkTable[i].period;
      histParam.flags    = checkTable[i].flags;
      retCode = TA_HistoryAlloc( unifiedDatabase, &histParam, &history );

      if( retCode != TA_SUCCESS )
      {
         printf( "%s%d.1 [%d]\n", fail_header, i, retCode );
         return TA_PERIOD_HISTORYALLOC_FAILED;
      }

      if( history->nbBars != checkTable[i].nbExpectedPriceBar )
      {
         printf( "%s%d.2 [%d != %d]\n",
                 fail_header, i,
                 history->nbBars,
                 checkTable[i].nbExpectedPriceBar );
         TA_HistoryFree( history );
         return TA_PERIOD_NBBAR_INCORRECT;
      }

      /* If the call is expected to return an empty history, no further check are done. */
      if( checkTable[i].nbExpectedPriceBar == 0 )
      {
         retCode = TA_HistoryFree( history );
         if( retCode != TA_SUCCESS )
         {
           printf( "%s%d.16 [%d]\n", fail_header, i, retCode );
           return TA_PERIOD_HISTORYFREE_FAILED;
         }
         continue;
      }

      #define CHECK_VALUE_OK(varName, subtestNo ) \
      { \
         if( !TA_REAL_EQ( history->varName[checkTable[i].thePriceBarToCheck], checkTable[i].expected_##varName, 0.01 ) ) \
         { \
            printf( "%s%d.%d [%f != %f]\n", \
                    fail_header, i, subtestNo, \
                    (TA_Real) history->varName[checkTable[i].thePriceBarToCheck], \
                    (TA_Real) checkTable[i].expected_##varName ); \
            TA_HistoryFree( history ); \
            return TA_PERIOD_PRICE_INCORRECT; \
         } \
      }

      CHECK_VALUE_OK( open,   3 );
      CHECK_VALUE_OK( high,   4 );
      CHECK_VALUE_OK( low,    5 );
      CHECK_VALUE_OK( close,  6 );
      CHECK_VALUE_OK( volume, 7 );

      if( history->openInterest != NULL )
      {
         printf( "%s%d.8\n", fail_header, i );
         TA_HistoryFree( history );
         return TA_PERIOD_OPENINTEREST_INCORRECT;
      }

      timestamp = &history->timestamp[checkTable[i].thePriceBarToCheck];
      if( TA_GetYear( timestamp ) != checkTable[i].expected_year )
      {
         printf( "%s%d.9 %d\n", fail_header, i, TA_GetYear(timestamp) );
         TA_HistoryFree( history );
         return TA_PERIOD_TIMESTAMP_YEAR_INCORRECT;
      }

      if( TA_GetMonth( timestamp ) != checkTable[i].expected_month )
      {
         printf( "%s%d.10 %d\n", fail_header, i, TA_GetMonth(timestamp) );
         TA_HistoryFree( history );
         return TA_PERIOD_TIMESTAMP_MONTH_INCORRECT;
      }

      if( TA_GetDay( timestamp ) != checkTable[i].expected_day )
      {
         printf( "%s%d.11 %d\n", fail_header, i, TA_GetDay(timestamp) );
         TA_HistoryFree( history );
         return TA_PERIOD_TIMESTAMP_DAY_INCORRECT;
      }

      if( TA_GetHour( timestamp ) != checkTable[i].expected_hour )
      {
         printf( "%s%d.12 %d\n", fail_header, i, TA_GetHour(timestamp) );
         TA_HistoryFree( history );
         return TA_PERIOD_TIMESTAMP_DAY_INCORRECT;
      }

      if( TA_GetMin( timestamp ) != checkTable[i].expected_min )
      {
         printf( "%s%d.13 %d\n", fail_header, i, TA_GetMin(timestamp) );
         TA_HistoryFree( history );
         return TA_PERIOD_TIMESTAMP_DAY_INCORRECT;
      }

      if( TA_GetSec( timestamp ) != checkTable[i].expected_sec )
      {
         printf( "%s%d.14 %d\n", fail_header, i, TA_GetSec(timestamp) );
         TA_HistoryFree( history );
         return TA_PERIOD_TIMESTAMP_DAY_INCORRECT;
      }

      retCode = TA_HistoryFree( history );
      if( retCode != TA_SUCCESS )
      {
         printf( "%s%d.15 [%d]\n", fail_header, i, retCode );
         return TA_PERIOD_HISTORYFREE_FAILED;
      }
   }

   return 0; /* Succcess. */
}


ErrorNumber test_end_of_period( TA_UDBase *mainDatabase )
{
   TA_RetCode  retCode;
   ErrorNumber retValue;
   TA_AddDataSourceParam  sourceParam;
   TA_HistoryAllocParam  histParam;
   TA_History  *historyBop, *historyEop;
   TA_UDBase  *localDatabase;

#define CLEANUP_AND_RETURN_WITH_ERROR(errorMsg,retCode,retValue)  \
   {                                                              \
      printf(errorMsg, retCode);                                  \
      if (historyBop)                                             \
         TA_HistoryFree( historyBop );                            \
      if (historyEop)                                             \
         TA_HistoryFree( historyEop );                            \
                                                                  \
      retCode = TA_UDBaseFree( localDatabase );                   \
      if( retCode != TA_SUCCESS )                                 \
      {                                                           \
         printf( "TA_UDBaseFree failed [%d]\n", retCode );        \
      }                                                           \
                                                                  \
      return retValue;                                            \
   }

   /* The main database uses the beginning-of-period logic.
    * The "localDatabase" will use end-of-period logic.
    */
   retCode = TA_UDBaseAlloc( &localDatabase );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_UDBaseAlloc failed [%d]\n", retCode );
      return TA_TESTUTIL_UDBASE_ALLOC_FAILED;
   }

   /* Adding end-of-period source to an empty database 
    * should always succeed.
    */
   memset( &sourceParam, 0, sizeof( TA_AddDataSourceParam ) );
   sourceParam.id = TA_SIMULATOR;
   sourceParam.flags = TA_SOURCE_USES_END_OF_PERIOD;
   historyBop = historyEop = NULL;
   retCode = TA_AddDataSource( localDatabase, &sourceParam );

   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_AddDataSource (end->empty) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_ADD_FAILED
      );

   /* Check whether history flags are treated consistently */
   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.category = "TA_SIM_REF";
   histParam.symbol   = "INTRA_REF_0";
   histParam.period   = TA_10MINS;  /* no consolidation */
   histParam.field    = TA_ALL;
   histParam.flags    = TA_NO_FLAGS;
   
   /* Allocating regular history (no conversion) should succeed */
   histParam.flags   &= ~TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( mainDatabase, &histParam, &historyBop );

   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (begin->begin) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_HISTORY_FAILED
      );

   /* No conversion but end-of-period logic history */
   histParam.flags   |= TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( localDatabase, &histParam, &historyEop );

   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (end->end) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_HISTORY_FAILED
      );

   /* Test whether the EOP history is indeed end-of-period stamped */
   retValue = validate_end_of_period_series_intraday( historyBop, historyEop, TA_10MINS );

   if( retValue != TA_TEST_PASS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "End-of-period stamped quotes are invalid [%d]\n",
         retCode,
         retValue
      );

   /* Test whether conversion from BOP to EOP works */
   /* Use the same history parameters as before, but request from the main database (BOP) */
   TA_HistoryFree( historyEop );
   historyEop = NULL;
   retCode = TA_HistoryAlloc( mainDatabase, &histParam, &historyEop );

   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (begin->end) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_HISTORY_FAILED
      );

   /* Test whether the EOP history is indeed end-of-period stamped */
   retValue = validate_end_of_period_series_intraday( historyBop, historyEop, TA_10MINS );

   if( retValue != TA_TEST_PASS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "Converted end-of-period stamped quotes are invalid [%d]\n",
         retCode,
         retValue
      );

   TA_HistoryFree( historyBop );
   TA_HistoryFree( historyEop );
   historyBop = historyEop = NULL;

   /* Try retrieving some consolidated data */
   histParam.period   = TA_30MINS;
   histParam.flags   &= ~TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( mainDatabase, &histParam, &historyBop );

   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (begin consolidated) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_CONSOLIDATED_FAILED
      );

   histParam.flags   |= TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( localDatabase, &histParam, &historyEop );

   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (end consolidated) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_CONSOLIDATED_FAILED
      );

   /* Test whether the consolidated EOP history is correct */
   retValue = validate_end_of_period_series_intraday( historyBop, historyEop, TA_30MINS );

   if( retValue != TA_TEST_PASS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "End-of-period stamped consolidated quotes are invalid [%d]\n",
         retCode,
         retValue
      );

   TA_HistoryFree( historyBop );
   TA_HistoryFree( historyEop );
   historyBop = historyEop = NULL;

   /* testing consolidation from intraday to daily */
   histParam.period   = TA_DAILY;
   histParam.flags   &= ~TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( mainDatabase, &histParam, &historyBop );
   
   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (begin consolidated, daily) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_CONSOLIDATED_FAILED
      );
   
   histParam.flags   |= TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( localDatabase, &histParam, &historyEop );
   
   if( retCode != TA_SUCCESS && retCode != TA_PERIOD_NOT_AVAILABLE )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (end consolidated, daily) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_CONSOLIDATED_FAILED
      );
   
   /* If the consolidation is implemented, test whether the consolidated EOP history is correct */
   if( retCode == TA_SUCCESS )
   {
      retValue = validate_end_of_period_series_dailyplus( historyBop, historyEop, TA_DAILY );
   
      if( retValue != TA_TEST_PASS )
         CLEANUP_AND_RETURN_WITH_ERROR(
            "End-of-period stamped consolidated quotes (daily) are invalid [%d]\n",
            retCode,
            retValue
         );
   }

   /* Test whether conversion & consolidation from EOP to BOP works */
   /* Request BOP from the local database (EOP) */
   TA_HistoryFree( historyEop );  /* reusing historyEop as historyBop2 */
   historyEop = NULL;
   histParam.flags   &= ~TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( localDatabase, &histParam, &historyEop );

   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (end->begin consolidated, daily) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_HISTORY_FAILED
      );

   /* Test whether the converted BOP2 history is identical to the original */
   retValue = compare_period_series( historyBop, historyEop );

   if( retValue != TA_TEST_PASS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "Converted end-of-period stamped quotes (daily) are invalid [%d]\n",
         retCode,
         retValue
      );

   TA_HistoryFree( historyBop );
   TA_HistoryFree( historyEop );
   historyBop = historyEop = NULL;

   /* testing EOD daily data, unconsolidated */
   histParam.symbol   = "DAILY_REF_0";
   histParam.period   = TA_DAILY;  /* no consolidation */
   histParam.flags   &= ~TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( mainDatabase, &histParam, &historyBop );
   
   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (begin, daily) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_HISTORY_FAILED
      );
   
   histParam.flags   |= TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( localDatabase, &histParam, &historyEop );
   
   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (end, daily) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_HISTORY_FAILED
      );
   
   /* Test whether the consolidated EOP history is correct */
   retValue = validate_end_of_period_series_dailyplus( historyBop, historyEop, TA_DAILY );
   
   if( retValue != TA_TEST_PASS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "End-of-period stamped quotes (daily) are invalid [%d]\n",
         retCode,
         retValue
      );
   
   TA_HistoryFree( historyBop );
   TA_HistoryFree( historyEop );
   historyBop = historyEop = NULL;


   /* testing EOD consolidation to weekly data */
   histParam.period   = TA_WEEKLY;  /* consolidate */
   histParam.flags   &= ~TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( mainDatabase, &histParam, &historyBop );
   
   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
         "TA_HistoryAlloc (begin consolidated, weekly) failed [%d]\n",
         retCode,
         TA_PERIOD_END_OF_PERIOD_CONSOLIDATED_FAILED
      );
   
   histParam.flags   |= TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( localDatabase, &histParam, &historyEop );
   
   if( retCode != TA_SUCCESS && retCode != TA_PERIOD_NOT_AVAILABLE )
      CLEANUP_AND_RETURN_WITH_ERROR(
      "TA_HistoryAlloc (end consolidated, weekly) failed [%d]\n",
      retCode,
      TA_PERIOD_END_OF_PERIOD_CONSOLIDATED_FAILED
      );
   
   /* If the consolidation is implemented, test whether the consolidated EOP history is correct */
   if( retCode == TA_SUCCESS )
   {
      retValue = validate_end_of_period_series_dailyplus( historyBop, historyEop, TA_WEEKLY );

      if( retValue != TA_TEST_PASS )
         CLEANUP_AND_RETURN_WITH_ERROR(
            "End-of-period stamped quotes (weekly) are invalid [%d]\n",
            retCode,
            retValue
         );
   }
   
   TA_HistoryFree( historyBop );
   TA_HistoryFree( historyEop );
   historyBop = historyEop = NULL;
   

   /* testing EOD consolidation to monthly data */
   histParam.period   = TA_MONTHLY;  /* consolidate */
   histParam.flags   &= ~TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( mainDatabase, &histParam, &historyBop );
   
   if( retCode != TA_SUCCESS )
      CLEANUP_AND_RETURN_WITH_ERROR(
      "TA_HistoryAlloc (begin consolidated, monthly) failed [%d]\n",
      retCode,
      TA_PERIOD_END_OF_PERIOD_CONSOLIDATED_FAILED
      );
   
   histParam.flags   |= TA_USE_END_OF_PERIOD;
   retCode = TA_HistoryAlloc( localDatabase, &histParam, &historyEop );
   
   if( retCode != TA_SUCCESS && retCode != TA_PERIOD_NOT_AVAILABLE )
      CLEANUP_AND_RETURN_WITH_ERROR(
      "TA_HistoryAlloc (end consolidated, monthly) failed [%d]\n",
      retCode,
      TA_PERIOD_END_OF_PERIOD_CONSOLIDATED_FAILED
      );
   
   /* If the consolidation is implemented, test whether the consolidated EOP history is correct */
   if( retCode == TA_SUCCESS )
   {
      retValue = validate_end_of_period_series_dailyplus( historyBop, historyEop, TA_MONTHLY );
 
      if( retValue != TA_TEST_PASS )
         CLEANUP_AND_RETURN_WITH_ERROR(
         "End-of-period stamped quotes (monthly) are invalid [%d]\n",
         retCode,
         retValue
         );
   }
   
   TA_HistoryFree( historyBop );
   TA_HistoryFree( historyEop );
   historyBop = historyEop = NULL;
   
   

#undef CLEANUP_AND_RETURN_WITH_ERROR

   retCode = TA_UDBaseFree( localDatabase );
   if( retCode != TA_SUCCESS )
   {
       printf( "TA_UDBaseFree failed [%d]\n", retCode );
   }

   return TA_TEST_PASS; /* Success. */
}



static ErrorNumber compare_period_series( TA_History *history1, TA_History *history2 )
{
   TA_RetCode retCode;
   unsigned int i;

   if( !history1 || !history2 )
   {
      printf( "History series is null\n");
      return TA_PERIOD_HISTORYCOMPARE_FAILED;
   }
   
   if( history1->nbBars != history2->nbBars )
   {
       printf( "History series not equal length\n");
       return TA_PERIOD_HISTORYCOMPARE_FAILED;
   }

   for ( i = 0;  i < history1->nbBars && i < history2->nbBars;  i++)
   {
       retCode = TA_TimestampValidate( &history1->timestamp[i] );
       if( retCode != TA_SUCCESS )
       {
           printf( "History 1 timestamp invalid [%d]\n", retCode );
           return TA_PERIOD_HISTORYCOMPARE_FAILED;
       }

       retCode = TA_TimestampValidate( &history2->timestamp[i] );
       if( retCode != TA_SUCCESS )
       {
           printf( "History 2 timestamp invalid [%d]\n", retCode );
           return TA_PERIOD_HISTORYCOMPARE_FAILED;
       }

       /* the timestamps of both series should be shifted by period */
       if( ! TA_TimestampEqual( &history1->timestamp[i], &history2->timestamp[i] ) )
       {
           printf( "Compared history timestamp unequal\n" );
           return TA_PERIOD_HISTORYCOMPARE_FAILED;
       }

       /* check whether quotes match */
#define CHECK_HISTORY_FIELD(field)                                        \
       if( history1->field && history2->field )                           \
       {                                                                  \
           if( history1->field[i] != history2->field[i] )                 \
           {                                                              \
               printf( "History series unequal, field: %s, record: %d\n", #field, i ); \
               return TA_PERIOD_HISTORYCOMPARE_FAILED;                    \
           }                                                              \
       } else if( history1->field != history2->field )                    \
       {                                                                  \
           printf( "History series unequal, field: %s\n", #field );       \
           return TA_PERIOD_HISTORYCOMPARE_FAILED;                        \
       }

       CHECK_HISTORY_FIELD(open)
       CHECK_HISTORY_FIELD(high)
       CHECK_HISTORY_FIELD(low)
       CHECK_HISTORY_FIELD(close)
       CHECK_HISTORY_FIELD(volume)
       CHECK_HISTORY_FIELD(openInterest)

#undef CHECK_HISTORY_FIELD

   }

   return TA_TEST_PASS;
}



static ErrorNumber validate_end_of_period_series_intraday( TA_History *historyBop, TA_History *historyEop, TA_Period period )
{
   TA_RetCode retCode;
   TA_Timestamp ts;
   unsigned int i;

   if( !historyBop || !historyEop )
   {
      printf( "History series is null\n");
      return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
   }
   
   if( historyBop->nbBars != historyEop->nbBars )
   {
       printf( "History series not equal length\n");
       return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
   }

   for ( i = 0;  i < historyBop->nbBars && i < historyEop->nbBars;  i++)
   {
       retCode = TA_TimestampValidate( &historyBop->timestamp[i] );
       if( retCode != TA_SUCCESS )
       {
           printf( "History BOP timestamp invalid [%d]\n", retCode );
           return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
       }

       retCode = TA_TimestampValidate( &historyEop->timestamp[i] );
       if( retCode != TA_SUCCESS )
       {
           printf( "History EOP timestamp invalid [%d]\n", retCode );
           return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
       }

       /* the timestamps of both series should be shifted by period */
       TA_AddTimeToTimestamp( &ts, &historyBop->timestamp[i], period );
       if( ! TA_TimestampEqual( &ts, &historyEop->timestamp[i] ) )
       {
           printf( "End-of-period history timestamp incorrect\n" );
           return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
       }

       /* check whether quotes match */
#define CHECK_HISTORY_FIELD(field)                                        \
       if( historyBop->field && historyEop->field )                       \
       {                                                                  \
           if( historyBop->field[i] != historyEop->field[i] )             \
           {                                                              \
               printf( "History series inconsistent, field: %s, record: %d\n", #field, i ); \
               return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;              \
           }                                                              \
       } else if( historyBop->field != historyEop->field )                \
       {                                                                  \
           printf( "History series inconsistent, field: %s\n", #field );  \
           return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;                  \
       }

       CHECK_HISTORY_FIELD(open)
       CHECK_HISTORY_FIELD(high)
       CHECK_HISTORY_FIELD(low)
       CHECK_HISTORY_FIELD(close)
       CHECK_HISTORY_FIELD(volume)
       CHECK_HISTORY_FIELD(openInterest)

#undef CHECK_HISTORY_FIELD

   }

   return TA_TEST_PASS;
}



static ErrorNumber validate_end_of_period_series_dailyplus( TA_History *historyBop, TA_History *historyEop, TA_Period period )
{
   TA_RetCode retCode;
   unsigned int i;
   unsigned int delta;

   if( !historyBop || !historyEop )
   {
      printf( "History series is null\n");
      return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
   }
   
   if( historyBop->nbBars != historyEop->nbBars )
   {
      printf( "History series not equal length (%u != %u)\n", historyBop->nbBars, historyEop->nbBars);
      return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
   }
   
   for ( i = 0;  i < historyBop->nbBars && i < historyEop->nbBars;  i++)
   {
      retCode = TA_TimestampValidate( &historyBop->timestamp[i] );
      if( retCode != TA_SUCCESS )
      {
         printf( "History BOP timestamp invalid [%d]\n", retCode );
         return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
      }
      
      retCode = TA_TimestampValidate( &historyEop->timestamp[i] );
      if( retCode != TA_SUCCESS )
      {
         printf( "History EOP timestamp invalid [%d]\n", retCode );
         return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
      }
      
      /* for daily (and up) data, time should be 00:00:00
       */
      if ( TA_GetHour(&historyBop->timestamp[i]) != 0 ||
           TA_GetMin( &historyBop->timestamp[i]) != 0 ||
           TA_GetSec( &historyBop->timestamp[i]) != 0 ) 
      {
         printf( "History BOP timestamp time incorrect\n" );
         return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
      }

      if ( TA_GetHour(&historyEop->timestamp[i]) != 0 ||
           TA_GetMin( &historyEop->timestamp[i]) != 0 ||
           TA_GetSec( &historyEop->timestamp[i]) != 0 ) 
      {
         printf( "History EOP timestamp time incorrect\n" );
         return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
      }
      
      switch( period) 
      {
      case TA_DAILY:
         /* EOP should be a day after BOP */
         TA_TimestampDeltaDay( &historyBop->timestamp[i], &historyEop->timestamp[i], &delta );
         if( ! TA_TimestampDateLess( &historyBop->timestamp[i], &historyEop->timestamp[i] )
            || (delta != 2) )
         {
            printf( "End-of-period history timestamp incorrect\n" );
            return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
         }
         break;

      case TA_WEEKLY:
         /* weeks are timestamped on Sunday */
         if ( TA_GetDayOfTheWeek( &historyBop->timestamp[i] ) != TA_SUNDAY )
         {
            printf( "Weekly history BOP timestamp date incorrect\n" );
            return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
         }
         if ( TA_GetDayOfTheWeek( &historyEop->timestamp[i] ) != TA_SUNDAY )
         {
            printf( "Weekly history EOP timestamp date incorrect\n" );
            return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
         }
         /* as for the rest, EOP has to be one week after BOP */
         if( TA_GetWeekOfTheYear( &historyBop->timestamp[i] )+1 != TA_GetWeekOfTheYear( &historyEop->timestamp[i] ) )
         {
            /* condone special case - end of year */
            if (TA_GetWeekOfTheYear( &historyBop->timestamp[i] ) == 51 && TA_GetWeekOfTheYear( &historyEop->timestamp[i] ) == 0)
               break;
            if (TA_GetWeekOfTheYear( &historyBop->timestamp[i] ) == 52 && TA_GetWeekOfTheYear( &historyEop->timestamp[i] ) == 1)
               break;

            printf( "End-of-period weekly history timestamp incorrect\n" );
            return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
         }
         break;

      case TA_MONTHLY:
         /* months are timestamped on 1st */
         if ( TA_GetDay( &historyBop->timestamp[i] ) != 1 )
         {
            printf( "Monthly history BOP timestamp date incorrect\n" );
            return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
         }
         if ( TA_GetDay( &historyBop->timestamp[i] ) != 1 )
         {
            printf( "Monthly history EOP timestamp date incorrect\n" );
            return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
         }
         /* as for the rest, EOP has to be one month after BOP */
         if ( TA_GetMonth( &historyBop->timestamp[i] )+1 != TA_GetMonth( &historyEop->timestamp[i] ) )
         {
            if ( TA_GetMonth( &historyBop->timestamp[i] ) != 12 
                && TA_GetMonth( &historyEop->timestamp[i]) != 1 
                && TA_GetYear( &historyBop->timestamp[i] )+1 != TA_GetYear( &historyEop->timestamp[i] ) )
            {
               printf( "End-of-period monthly history timestamp incorrect\n" );
               return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
            }
         }
         else if ( TA_GetYear( &historyBop->timestamp[i] ) != TA_GetYear( &historyEop->timestamp[i] ) )
         {
            printf( "End-of-period monthly history timestamp incorrect\n" );
            return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;
         }
         break;

      default:
         ; /* no further checks */
      }

      /* check whether quotes match */
#define CHECK_HISTORY_FIELD(field)                                                 \
   if( historyBop->field && historyEop->field )                                    \
      {                                                                            \
         if( historyBop->field[i] != historyEop->field[i] )                        \
         {                                                                         \
            printf( "History series inconsistent, field: %s, record: %d\n", #field, i ); \
            return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;                          \
         }                                                                         \
      } else if( historyBop->field != historyEop->field )                          \
      {                                                                            \
         printf( "History series inconsistent, field: %s\n", #field );             \
         return TA_PERIOD_END_OF_PERIOD_WRONG_HISTORY;                             \
      }
      
      CHECK_HISTORY_FIELD(open)
         CHECK_HISTORY_FIELD(high)
         CHECK_HISTORY_FIELD(low)
         CHECK_HISTORY_FIELD(close)
         CHECK_HISTORY_FIELD(volume)
         CHECK_HISTORY_FIELD(openInterest)
         
#undef CHECK_HISTORY_FIELD
         
   }
   
   return TA_TEST_PASS;
}






/* Period transformation is highly dependable on
 * the function evaluating the 'delta' between
 * two timestamp, so this is verified here.
 */
static ErrorNumber testTimestampDelta( void )
{
   TA_RetCode retCode;
   unsigned int i, delta; 

   /* !!! A lot more of testing could be added !!! */

   /* Test weekday delta. */
   TA_SetDate( 2002, 12, 29, &sundayTS );
   TA_SetDate( 2002, 12, 30, &mondayTS );
   TA_SetDate( 2002, 12, 31, &tuesdayTS );
   TA_SetDate( 2003,  1,  1, &wednesdayTS );
   TA_SetDate( 2003,  1,  2, &thursdayTS );
   TA_SetDate( 2003,  1,  3, &fridayTS );
   TA_SetDate( 2003,  1,  4, &saturdayTS );

   TA_SetDate( 2003,  1,  5, &sunday2TS );
   TA_SetDate( 2003,  1,  6, &monday2TS );
   TA_SetDate( 2003,  1,  7, &tuesday2TS );
   TA_SetDate( 2003,  1,  8, &wednesday2TS );
   TA_SetDate( 2003,  1,  9, &thursday2TS );
   TA_SetDate( 2003,  1, 10, &friday2TS );
   TA_SetDate( 2003,  1, 11, &saturday2TS );

   for( i=0; i < NB_WEEKDAY_CHECK_TO_DO; i++ )
   {
      retCode = TA_TimestampDeltaWeekday( toCheck[i].start,
                                          toCheck[i].end,                                          
                                          &delta );
      if( retCode != TA_SUCCESS )
      {
         printf( "Failed: Weekday delta test #%d\n", i );
         return TA_PERIOD_DELTA_WEEKDAY_FAILED;
      }

      if( delta != toCheck[i].expectedDelta )
      {
         printf( "Failed: Expected delta != delta (%d!=%d) for test #%d\n", toCheck[i].expectedDelta, delta, i );
         return TA_PERIOD_DELTA_WEEKDAY_FAILED_1;
      }
   }

   return TA_TEST_PASS; /* Success. */
}
