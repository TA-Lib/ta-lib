/* TA-LIB Copyright (c) 1999-2002, Mario Fortier
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
 *  070401 MF   First version.
 *
 */

/* Description:
 *         Regression testing of the functionality provided
 *         by the file ta_period.c
 */

/**** Headers ****/
#include <stdio.h>
#include "ta_test_priv.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_Period period;
   unsigned int nbExpectedPriceBar;
   TA_Integer thePriceBarToCheck;
   unsigned int expected_month;
   unsigned int expected_day;
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
static ErrorNumber testTimestampDelta( void );

/**** Local variables definitions.     ****/
const char *fail_header = "Fail: test_period,Test #";

static TA_PriceBarCheck checkTable[] = 
{
  { TA_WEEKLY,    52,  0,  1,  8,  92.500,  96.375,  90.750,  93.780,  4511420, 0 },
  { TA_WEEKLY,    52,  1,  1, 15,  94.500,  95.000,  89.440,  92.470,  3973160, 0 },
  { TA_WEEKLY,    52,  2,  1, 22,  93.875,  99.625,  88.815,  89.875,  9943350, 0 },
  { TA_WEEKLY,    52, 50, 12, 24, 109.060, 110.440, 107.750, 108.620,  4539425, 0 },
  { TA_WEEKLY,    52, 51, 12, 31, 109.690, 110.750, 106.620, 107.870,  3363920, 0 },
  { TA_MONTHLY,   12,  0,  1, 31,  92.500,  99.625,  86.750,  91.625,  6494478, 0 },
  { TA_MONTHLY,   12,  1,  2, 28,  92.250,  92.250,  80.875,  84.875,  5347410, 0 },
  { TA_MONTHLY,   12, 11, 12, 31, 102.560, 122.120, 102.250, 107.870,  7213263, 0 },
  { TA_QUARTERLY,  4,  0,  3, 31,  92.500,  99.625,  80.875,  88.625,  5580475, 0 },
  { TA_QUARTERLY,  4,  1,  6, 30,  88.655, 132.000,  81.500, 129.250,  6024341, 0 },
  { TA_QUARTERLY,  4,  2,  9, 30, 130.000, 139.190, 117.560, 121.000,  6397878, 0 },
  { TA_QUARTERLY,  4,  3, 12, 31, 121.000, 123.250,  89.000, 107.870, 10614132, 0 },
  { TA_YEARLY,     1,  0, 12, 31,  92.500, 139.190,  80.875, 107.870,  7177425, 0 }
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

   printf( "Testing period/timeframe conversion\n" );

   /* Because period transformation are very
    * dependend on the "delta" timestamp functions,
    * let's indepedently some of these first.
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
      retCode = TA_HistoryAlloc( unifiedDatabase, 
                                 "TA_SIM_REF", "DAILY_REF_0",
                                 checkTable[i].period, 0, 0, TA_ALL,
                                 &history );

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
      if( TA_GetYear( timestamp ) != 1999 )
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

      retCode = TA_HistoryFree( history );
      if( retCode != TA_SUCCESS )
      {
         printf( "%s%d.12 [%d]\n", fail_header, i, retCode );
         return TA_PERIOD_HISTORYFREE_FAILED;
      }
   }

   return 0; /* Succcess. */
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
