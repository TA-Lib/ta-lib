/* TA-LIB Copyright (c) 1999-2006, Mario Fortier
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
 *  063001 MF   First version (initial framework only).
 *  090404 MF   Add test_candlestick
 */

/* Description:
 *    Perform regression testing of the TA-LIB.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static int testTAFunction_ALL( void );
static ErrorNumber test_with_simulator( void );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
int main( int argc, char **argv )
{
   ErrorNumber retValue;

   (void)argv;

   printf( "\n" );
   printf( "ta_regtest V%s - Regression Tests of TA-Lib code\n", TA_GetVersionString() );
   printf( "\n" );

   if( argc > 1 )
   {
      printf( "Usage: ta_regtest\n" );
      printf( "\n" );
      printf( "   No parameter needed.\n" );
      printf( "\n" );
      printf( "   This tool will execute a series of tests to\n" );
      printf( "   make sure that the library is behaving as\n" );
      printf( "   expected.\n\n");

      printf( "   ** Must be run from the 'bin' directory.\n\n" );

      printf( "   On success, the exit code is 0.\n" );
      printf( "   On failure, the exit code is a number that can be\n" );
      printf( "   found in c/src/tools/ta_regtest/ta_error_number.h\n" );

      return TA_REGTEST_BAD_USER_PARAM;
   }

   /* Some tests are using randomness. */
   srand( (unsigned)time( NULL ) );

   /* Test utility like List/Stack/Dictionary/Memory Allocation etc... */
   retValue = test_internals();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed an internal test with code=%d\n", retValue );
      return retValue;
   }

   /* Test abstract interface. */
   retValue = test_abstract();
   if( retValue != TA_TEST_PASS )
   {
      printf( "Failed: Abstract interface Tests (error number = %d)\n", retValue );
      return retValue;
   }

   /* Perform all the tests using the TA_SIMULATOR data */
   retValue = test_with_simulator();
   if( retValue != TA_TEST_PASS )
      return retValue;

   printf( "\n* All tests succeed. Enjoy the library. *\n" );

   return TA_TEST_PASS; /* Everything succeed !!! */
}

/**** Local functions definitions.     ****/
static ErrorNumber test_with_simulator( void )
{
   ErrorNumber retValue;

   /* Initialize the library. */
   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Perform testing of each of the TA Functions. */
   retValue = testTAFunction_ALL();
   if( retValue != TA_TEST_PASS )
   {
      return retValue;
   }

   /* Clean-up and exit. */

   retValue = freeLib( );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS; /* All test succeed. */
}

extern TA_Real      TA_SREF_open_daily_ref_0_PRIV[];
extern TA_Real      TA_SREF_high_daily_ref_0_PRIV[];
extern TA_Real      TA_SREF_low_daily_ref_0_PRIV[];
extern TA_Real      TA_SREF_close_daily_ref_0_PRIV[];
extern TA_Integer   TA_SREF_volume_daily_ref_0_PRIV[];

static int testTAFunction_ALL( void )
{
   ErrorNumber retValue;
   TA_History history;

   history.nbBars = 252;
   history.open   = TA_SREF_open_daily_ref_0_PRIV;
   history.high   = TA_SREF_high_daily_ref_0_PRIV;
   history.low    = TA_SREF_low_daily_ref_0_PRIV;
   history.close  = TA_SREF_close_daily_ref_0_PRIV;
   history.volume = TA_SREF_volume_daily_ref_0_PRIV;

   printf( "Testing the TA functions\n" );

   initGlobalBuffer();

   /* Make tests for each TA functions. */
   #define DO_TEST(func,str) \
      { \
      printf( "%40s: Testing....", str ); \
      fflush(stdout); \
      showFeedback(); \
      TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT ); \
      retValue = func( &history ); \
      if( retValue != TA_TEST_PASS ) \
         return retValue; \
      hideFeedback(); \
      printf( "done.\n" ); \
      fflush(stdout); \
      }
   DO_TEST( test_func_per_hlc,  "CCI,WILLR,ULTOSC,NATR" );
   DO_TEST( test_func_per_ohlc, "BOP,AVGPRICE" );
   DO_TEST( test_func_rsi,      "RSI,CMO" );
   DO_TEST( test_func_1in_1out, "DCPERIOD/PHASE,TRENDLINE/MODE" );
   DO_TEST( test_func_po,       "PO,APO" );
   DO_TEST( test_func_adx,      "ADX,ADXR,DI,DM,DX" );
   DO_TEST( test_func_sar,      "SAR,SAREXT" );
   DO_TEST( test_candlestick,   "All Candlesticks" );
   DO_TEST( test_func_ma,       "All Moving Averages" );
   DO_TEST( test_func_stoch,    "STOCH,STOCHF,STOCHRSI" );
   DO_TEST( test_func_per_hl,   "AROON,CORREL" );
   DO_TEST( test_func_per_hlcv, "MFI,AD,ADOSC" );
   DO_TEST( test_func_1in_2out, "PHASOR,SINE" );   
   DO_TEST( test_func_per_ema,  "TRIX" );
   DO_TEST( test_func_minmax,   "MIN,MAX" );
   DO_TEST( test_func_macd,     "MACD,MACDFIX,MACDEXT" );
   DO_TEST( test_func_mom_roc,  "MOM,ROC,ROCP,ROCR,ROCR100" );
   DO_TEST( test_func_trange,   "TRANGE,ATR" );
   DO_TEST( test_func_stddev,   "STDDEV,VAR" );
   DO_TEST( test_func_bbands,   "BBANDS" );

   return TA_TEST_PASS; /* All test succeed. */
}
