/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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
 *  AB       Anatoliy Belsky
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 AB   First version.
 *
 */

/* Description:
 *     Test IMI function.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_Integer doRangeTestFlag; /* One will do a call to doRangeTest */

   TA_Integer unstablePeriod;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer optInTimePeriod;

   TA_RetCode expectedRetCode;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;
} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *close;
   const TA_Real *open;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );
static ErrorNumber test_imi_degenerate_windows( void );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /*************************/
   /*        IMI TEST       */
   /*************************/
   { 1, 0, 0, 251, 5, TA_SUCCESS,     0, 55.9194,   4,  252-4 }, /* First Value */
   { 0, 0, 0, 251, 5, TA_SUCCESS,     1, 64.6143,   4,  252-4 },
   { 0, 0, 0, 251, 5, TA_SUCCESS, 252-5, 7.730673,  4,  252-4 }, /* Last Value */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_imi( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   for( i=0; i < NB_TEST; i++ )
   {
      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "%s Failed Bad Parameter for Test #%d (%d,%d)\n", __FILE__,
                 i, tableTest[i].expectedNbElement, history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "%s Failed Test #%d (Code=%d)\n", __FILE__,
                 i, retValue );
         return retValue;
      }
   }

   /* Issue #112: a degenerate all-flat window must return the neutral 50.0,
    * never NaN — and the guard must fire ONLY on that window. These cases are
    * data-independent of the sample history, so they run from synthetic buffers. */
   retValue = test_imi_degenerate_windows();
   if( retValue != 0 )
   {
      printf( "%s Failed degenerate-window test (Code=%d)\n", __FILE__, retValue );
      return retValue;
   }

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* All test succeed. */
   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/
static TA_RetCode rangeTestFunction( TA_Integer    startIdx,
                                     TA_Integer    endIdx,
                                     TA_Real      *outputBuffer,
                                     TA_Integer   *outputBufferInt,
                                     TA_Integer   *outBegIdx,
                                     TA_Integer   *outNbElement,
                                     TA_Integer   *lookback,
                                     void         *opaqueData,
                                     unsigned int  outputNb,
                                     unsigned int *isOutputInteger )
{
   TA_RetCode retCode;
   TA_RangeTestParam *testParam;

   (void)outputNb;
   (void)outputBufferInt;

   *isOutputInteger = 0;

   testParam = (TA_RangeTestParam *)opaqueData;

   retCode = TA_IMI(
                        startIdx,
                        endIdx,
						testParam->open,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );


   *lookback = TA_IMI_Lookback( testParam->test->optInTimePeriod );

   return retCode;
}

static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx;
   TA_Integer outNbElement;
   TA_RangeTestParam testParam;

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->open, history->nbBars );
   setInputBuffer( 1, history->close, history->nbBars );
   setInputBuffer( 2, history->open, history->nbBars );

   /* Make a simple first call. */
   retCode = TA_IMI(
                        test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
						gBuffer[1].in,
                        test->optInTimePeriod,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );

   /* Verify that the inputs were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->open,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkDataSame( gBuffer[1].in, history->close, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   if( server_verify_active() )
   {
      errNb = server_verify("IMI", test->startIdx, test->endIdx, history->nbBars,
                            retCode, outBegIdx, outNbElement,
                            (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, NULL },
                            (double[]){ (double)test->optInTimePeriod }, 1,
                            (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_IMI(
                        test->startIdx,
                        test->endIdx,
                        gBuffer[2].in,
						gBuffer[1].in,
                        test->optInTimePeriod,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[2].in );

   /* Verify that the inputs were preserved. */
   errNb = checkDataSame( gBuffer[1].in, history->close, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call should have the same output as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[2].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[2].in, 0 );

   if( errNb != TA_TEST_PASS )
      return errNb;

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    *
    * IMI recomputes upsum/downsum fresh over its finite window every output
    * bar (no running accumulator, no recursion), so its output is bit-exact
    * across any startIdx/endIdx. This sweep was previously disabled with a
    * "the test might be wrong!?" note, but the test was right: it was catching
    * fix #98 (the unstable period used to grow the summation window). With
    * that fixed, compare as TA_FUNC_UNST_NONE to enforce the exactness.
    */
   testParam.test  = test;
   testParam.close = history->close;
   testParam.open = history->open;

   if( test->doRangeTestFlag )
   {
      /* IMI recomputes its up/down window fresh every bar (no accumulator, no
       * recursion), so it is bit-exact across ranges. Assert that explicitly
       * with TA_STABLE_EXACT rather than settling for the tight-but-fuzzy
       * epsilon tolerance; this is the range test that caught issue #14. */
      errNb = doRangeTestEx(
                           rangeTestFunction,
                           TA_STABLE_EXACT,
                           TA_FUNC_UNST_NONE,
                           (void *)&testParam, 1, 0 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

/* Issue #112: a *successful* TA call must never emit NaN. On an all-flat window
 * (every bar close==open — a halted/illiquid instrument, a constant series) IMI's
 * upsum and downsum are both zero, so 100*(upsum/(upsum+downsum)) is 100*(0/0) =
 * NaN. The guard returns IMI's neutral center, 50.0 (equal up/down bias -> the
 * midpoint of the 0..100 oscillator; contrast CCI #7, centered at 0).
 *
 * Run IMI over a uniform (open,close) buffer and assert every output equals
 * `expected` and is not NaN; also drive the language servers (when --codegen is
 * active) so every backend is held to the same value. */
#define IMI_UNIFORM_N 30
static ErrorNumber imi_check_uniform( double openVal, double closeVal,
                                      double expected, const char *label )
{
   TA_Real    open[IMI_UNIFORM_N];
   TA_Real    close[IMI_UNIFORM_N];
   TA_Real    out[IMI_UNIFORM_N];
   TA_Integer i, outBegIdx, outNbElement;
   TA_RetCode retCode;
   const TA_Integer period = 14;

   for( i = 0; i < IMI_UNIFORM_N; i++ )
   {
      open[i]  = openVal;
      close[i] = closeVal;
   }

   retCode = TA_IMI( 0, IMI_UNIFORM_N - 1, open, close, period,
                     &outBegIdx, &outNbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "IMI %s: expected TA_SUCCESS, got retCode=%d\n", label, retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( outNbElement <= 0 )
   {
      printf( "IMI %s: expected a non-empty output, got %d elements\n",
              label, outNbElement );
      return TA_TESTUTIL_TFRR_BAD_OUTNBELEMENT;
   }
   for( i = 0; i < outNbElement; i++ )
   {
      if( out[i] != out[i] )   /* self-compare is true only for NaN */
      {
         printf( "IMI %s: out[%d] is NaN (issue #112: 0/0 must be guarded)\n", label, i );
         return TA_TEST_TFRR_BAD_OVERLAP_OR_NAN;
      }
      if( out[i] != expected )
      {
         printf( "IMI %s: out[%d]=%.17g, expected %g (issue #112)\n",
                 label, i, out[i], expected );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* Hold every language backend to the same result when servers are active. */
   if( server_verify_active() )
   {
      ErrorNumber errNb = server_verify( "IMI", 0, IMI_UNIFORM_N - 1, IMI_UNIFORM_N,
                            retCode, outBegIdx, outNbElement,
                            (const TA_Real*[]){ open, close, NULL },
                            (double[]){ (double)period }, 1,
                            (const TA_Real*[]){ out, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_imi_degenerate_windows( void )
{
   ErrorNumber errNb;

   /* The degenerate case (#112): every bar close==open -> 0/0 -> the guard must
    * return the neutral 50.0, never NaN. This is the red-green gate for the fix
    * (it fails NaN!=50.0 against the pre-fix code). */
   errNb = imi_check_uniform( 42.0, 42.0, 50.0, "flat-window -> 50" );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* The guard must fire ONLY on the zero-movement window: a window with genuine
    * one-sided movement must NOT collapse to 50.0. All-up (close>open every bar)
    * is upsum>0, downsum==0 -> exactly 100.0; all-down -> exactly 0.0. This keeps
    * the *default* C suite (not just the opt-in fuzz-064) able to catch a guard
    * that OVER-fires, e.g. `upsum==downsum` instead of `upsum+downsum==0`. */
   errNb = imi_check_uniform( 42.0, 43.0, 100.0, "all-up -> 100" );
   if( errNb != TA_TEST_PASS ) return errNb;
   errNb = imi_check_uniform( 43.0, 42.0, 0.0, "all-down -> 0" );
   if( errNb != TA_TEST_PASS ) return errNb;

   return TA_TEST_PASS;
}
#undef IMI_UNIFORM_N

