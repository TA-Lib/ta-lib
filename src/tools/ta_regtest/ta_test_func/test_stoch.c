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
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  122101 MF   First version.
 *  111603 MF   Add test of TA_STOCHRSI
 */

/* Description:
 *     Test the Stochastic function.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_memory.h"
#include "server_verify.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef enum
{
  TEST_STOCH,
  TEST_STOCHF,
  TEST_STOCHRSI
} TestId;

typedef struct
{
   TestId testId;

   TA_Integer doRangeTestFlag; /* One will do a call to doRangeTest */

   TA_Integer unstablePeriod;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer    optInPeriod_0;
   TA_Integer    optInPeriod_1;
   TA_Integer    optInMAType_1;
   TA_Integer    optInPeriod_2;
   TA_Integer    optInMAType_2;

   TA_RetCode expectedRetCode;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer oneOfTheExpectedOutRealIndex1;
   TA_Real    oneOfTheExpectedOutReal1;
} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );
static ErrorNumber test_stoch_epsilon_issue107( void );
static ErrorNumber test_stochrsi_epsilon_issue107( void );

static TA_RetCode referenceStoch( TA_Integer    startIdx,
                                  TA_Integer    endIdx,
                                  const TA_Real inHigh[],
                                  const TA_Real inLow[],
                                  const TA_Real inClose[],
                                  TA_Integer    optInPeriod_0, /* From 1 to TA_INTEGER_MAX */
                                  TA_Integer    optInPeriod_1, /* From 1 to TA_INTEGER_MAX */
                                  TA_Integer    optInMAType_1,
                                  TA_Integer    optInPeriod_2, /* From 1 to TA_INTEGER_MAX */
                                  TA_Integer    optInMAType_2,
                                  TA_Integer   *outBegIdx,
                                  TA_Integer   *outNbElement,
                                  TA_Real       outSlowK_0[],
                                  TA_Real       outSlowD_1[] );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /**************/
   /* STOCH TEST */
   /**************/
   { TEST_STOCH, 1, 0, 9, 9, 5, 3, TA_MAType_SMA, 4, TA_MAType_SMA, TA_SUCCESS,  9,  1,
                                                        0, 38.139,
                                                        0, 36.725  }, /* Test one value */


   { TEST_STOCH, 0, 0, 0, 251, 5, 3, TA_MAType_SMA, 3, TA_MAType_SMA, TA_SUCCESS,  8,  252-8,
                                                          0, 24.0128,
                                                          0, 36.254,   }, /* First Value */

   { TEST_STOCH, 0, 0, 0, 251, 5, 3, TA_MAType_SMA, 4, TA_MAType_SMA, TA_SUCCESS,  9,  252-9,
                                                          252-10, 30.194,
                                                          252-10, 46.641,   }, /* Last Value */

   { TEST_STOCH, 0, 0, 0, 251, 5, 3, TA_MAType_SMA, 3, TA_MAType_SMA, TA_SUCCESS,  8,  252-8,
                                                          252-9, 30.194,
                                                          252-9, 43.69,   }, /* Last Value */

   /*****************/
   /* STOCHRSI TEST */
   /*****************/
   { TEST_STOCHRSI, 0, 0, 27, 27, 14, 14, -1, 1, TA_MAType_SMA, TA_SUCCESS,  27,  1,
                                                 0, 94.156709,
                                                 0, 94.156709 }, /* Test one Value */

   { TEST_STOCHRSI, 1, 0, 0, 251, 14, 14, -1, 1, TA_MAType_SMA, TA_SUCCESS,  27,  252-27,
                                                 0, 94.156709,
                                                 0, 94.156709 }, /* First Value */

   { TEST_STOCHRSI, 0, 0, 0, 251, 14, 14, -1, 1, TA_MAType_SMA, TA_SUCCESS,  27,  252-27,
                                                 251-27, 0.0,
                                                 251-27, 0.0 }, /* Last Value */

   { TEST_STOCHRSI, 0, 0, 0, 251, 14, 45, -1, 1, TA_MAType_SMA, TA_SUCCESS,  58,  252-58,
                                                 0, 79.729186,
                                                 0, 79.729186 }, /* First Value */

   { TEST_STOCHRSI, 0, 0, 0, 251, 14, 45, -1, 1, TA_MAType_SMA, TA_SUCCESS,  58,  252-58,
                                                 251-58, 48.1550743,
                                                 251-58, 48.1550743 }, /* Last Value */


   { TEST_STOCHRSI, 1, 0, 0, 251, 11, 13, -1, 16, TA_MAType_SMA, TA_SUCCESS,  38,  252-38,
                                                 0, 5.25947,
                                                 0, 57.1711}, /* First Value */

   { TEST_STOCHRSI, 0, 0, 0, 251, 11, 13, -1, 16, TA_MAType_SMA, TA_SUCCESS,  38,  252-38,
                                                 251-38, 0.0,
                                                 251-38, 15.7303 }, /* Last Value */

    /* More test needed!!! */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_stoch( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* Regression test for issue #107 (STOCHRSI): the STOCH/STOCHF divide guard
    * must treat a machine-epsilon-flat window as flat (output 0), not divide a
    * sub-epsilon residue into full-scale noise. Crafted degenerate data. */
   retValue = test_stoch_epsilon_issue107();
   if( retValue != TA_TEST_PASS )
   {
      printf( "%s Failed: STOCH/STOCHF epsilon issue #107 regression test (Code=%d)\n",
              __FILE__, retValue );
      return retValue;
   }

   retValue = test_stochrsi_epsilon_issue107();
   if( retValue != TA_TEST_PASS )
   {
      printf( "%s Failed: STOCHRSI epsilon issue #107 regression test (Code=%d)\n",
              __FILE__, retValue );
      return retValue;
   }

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

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* All test succeed. */
   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/
/* Issue #107 (STOCHRSI): STOCHF decides Fast-K as (close-lowest)/((highest-
 * lowest)/100). STOCHRSI feeds it a COMPUTED RSI series, so a window that is
 * flat to machine precision has highest-lowest = a sub-epsilon residue rather
 * than exactly 0. The old exact `diff != 0.0` guard then divides that residue,
 * amplifying pure rounding noise into an arbitrary value across the full [0,100]
 * range. The fix guards with TA_IS_ZERO(diff) (the same near-zero test CCI/
 * ULTOSC/NATR already use), so a flat window yields 0.
 *
 * This reproduces the exact internal state with a hand-built window of values a
 * few ULP apart (what STOCHRSI produces when RSI is flat) fed straight into
 * STOCHF and STOCH. Pre-fix, STOCHF Fast-K is e.g. [0,100,0,33.33,0,66.67];
 * post-fix it must be all 0.
 */
static ErrorNumber test_stoch_epsilon_issue107( void )
{
   /* A machine-flat window: values 0..3 ULP above 50.0 — a real move can never
    * be this small (a 1-satoshi BTC tick is ~1e5 ULP), so "flat" is correct. */
   double v[10];
   TA_Real outK[16], outD[16];
   TA_Integer outBegIdx, outNbElement;
   TA_RetCode retCode;
   int i;
   const int n = 10;
   const int fastK = 5;              /* lookback = fastK-1 + ma_lookback(1) = 4 */

   v[0] = 50.0;
   v[1] = nextafter( 50.0, 100.0 );  /* +1 ULP */
   v[2] = 50.0;
   v[3] = nextafter( v[1], 100.0 );  /* +2 ULP */
   v[4] = 50.0;
   v[5] = nextafter( v[3], 100.0 );  /* +3 ULP */
   v[6] = 50.0;
   v[7] = nextafter( 50.0, 100.0 );  /* +1 ULP */
   v[8] = 50.0;
   v[9] = nextafter( v[7], 100.0 );  /* +2 ULP */

   /* --- STOCHF (the variant STOCHRSI uses): FastD_Period=1 => raw Fast-K --- */
   retCode = TA_STOCHF( 0, n-1, v, v, v, fastK, 1, TA_MAType_SMA,
                        &outBegIdx, &outNbElement, outK, outD );
   if( retCode != TA_SUCCESS )
   {
      printf( "  issue#107 STOCHF: retCode=%d\n", retCode );
      return TA_STOCH_EPSILON_ISSUE107_BAD_RETCODE;
   }
   if( outBegIdx != fastK-1 || outNbElement != n-(fastK-1) )
   {
      printf( "  issue#107 STOCHF: bad range outBegIdx=%d outNbElement=%d\n",
              outBegIdx, outNbElement );
      return TA_STOCH_EPSILON_ISSUE107_BAD_RANGE;
   }
   for( i=0; i < outNbElement; i++ )
   {
      if( fabs(outK[i]) > 1e-9 )
      {
         printf( "  issue#107 STOCHF: Fast-K[%d]=%.6f but the window is flat "
                 "within machine epsilon, so it must be 0.0 (residue divided "
                 "into full-scale noise)\n", i, outK[i] );
         return TA_STOCH_EPSILON_ISSUE107_WRONG_VALUE;
      }
   }

   /* --- STOCH (slow variant): same guard, SlowK/SlowD periods = 1 --- */
   retCode = TA_STOCH( 0, n-1, v, v, v, fastK,
                       1, TA_MAType_SMA, 1, TA_MAType_SMA,
                       &outBegIdx, &outNbElement, outK, outD );
   if( retCode != TA_SUCCESS )
   {
      printf( "  issue#107 STOCH: retCode=%d\n", retCode );
      return TA_STOCH_EPSILON_ISSUE107_BAD_RETCODE;
   }
   for( i=0; i < outNbElement; i++ )
   {
      if( fabs(outK[i]) > 1e-9 )
      {
         printf( "  issue#107 STOCH: Slow-K[%d]=%.6f but the window is flat "
                 "within machine epsilon, so it must be 0.0\n", i, outK[i] );
         return TA_STOCH_EPSILON_ISSUE107_WRONG_VALUE;
      }
   }

   return TA_TEST_PASS;
}

/* Issue #107 end-to-end through STOCHRSI (which is where it was reported). This
 * tie-heavy integer series drives Wilder RSI onto windows whose values sit 1 ULP
 * apart, so the internal STOCHF divide guard fires and reports 0 where the old
 * exact `diff != 0.0` divided the residue into 100. These golden FastK values are
 * the FIXED output — an A/B rebuild confirmed they differ from the pre-fix output
 * (reverting the guard flips several FastK entries from 0 back to 100). This pins
 * the new behaviour directly, without comparing against the wrong 0.6.4 oracle
 * (STOCHRSI is excluded from --fuzz-064 for exactly that reason). */
static ErrorNumber test_stochrsi_epsilon_issue107( void )
{
   static const double in[40] = {
      6, 7, 7, 6, 4, 3, 5, 7, 5, 7,  5, 3, 4, 4, 3, 3, 3, 4, 6, 5,
      6, 7, 7, 3, 3, 3, 5, 4, 6, 3,  4, 6, 4, 6, 3, 4, 5, 7, 4, 4 };
   /* FastK for TA_STOCHRSI(period=14, fastK=2, fastD=3, SMA); 11 of 23 are the
    * flat-RSI-window zeros the fix produces (pre-fix several were 100). */
   static const double expFastK[23] = {
      100, 100, 0, 100, 100, 0, 0, 0, 0, 100,  0, 100, 0, 100, 100, 0, 100, 0, 100, 100,  100, 0, 0 };
   const int nbBars = 40, period = 14, fastK = 2, fastD = 3;
   const int expBeg = 17, expNb = 23;
   TA_Real outK[40], outD[40];
   TA_Integer outBegIdx, outNbElement;
   TA_RetCode retCode;
   int i;

   retCode = TA_STOCHRSI( 0, nbBars-1, in, period, fastK, fastD, TA_MAType_SMA,
                          &outBegIdx, &outNbElement, outK, outD );
   if( retCode != TA_SUCCESS )
   {
      printf( "  issue#107 STOCHRSI: retCode=%d\n", retCode );
      return TA_STOCH_EPSILON_ISSUE107_BAD_RETCODE;
   }
   if( outBegIdx != expBeg || outNbElement != expNb )
   {
      printf( "  issue#107 STOCHRSI: bad range outBegIdx=%d (exp %d) outNbElement=%d (exp %d)\n",
              outBegIdx, expBeg, outNbElement, expNb );
      return TA_STOCH_EPSILON_ISSUE107_BAD_RANGE;
   }
   for( i=0; i < outNbElement; i++ )
   {
      if( fabs(outK[i] - expFastK[i]) > 1e-6 )
      {
         printf( "  issue#107 STOCHRSI: FastK[%d]=%.6f expected %.6f "
                 "(flat-RSI-window handling changed?)\n", i, outK[i], expFastK[i] );
         return TA_STOCH_EPSILON_ISSUE107_WRONG_VALUE;
      }
   }

   return TA_TEST_PASS;
}

static TA_RetCode rangeTestFunction( TA_Integer   startIdx,
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
  TA_Real *dummyOutput;

  (void)outputBufferInt;

  *isOutputInteger = 0;

  retCode = TA_NOT_SUPPORTED;

  testParam = (TA_RangeTestParam *)opaqueData;


  dummyOutput = TA_Malloc( (endIdx-startIdx+1) * sizeof(TA_Real) );

  switch( testParam->test->testId )
  {
  case TEST_STOCH:
     if( outputNb == 0 )
     {
        retCode = TA_STOCH( startIdx,
                            endIdx,
                            testParam->high,
                            testParam->low,
                            testParam->close,
                            testParam->test->optInPeriod_0,
                            testParam->test->optInPeriod_1,
                            (TA_MAType)testParam->test->optInMAType_1,
                            testParam->test->optInPeriod_2,
                            (TA_MAType)testParam->test->optInMAType_2,
                            outBegIdx, outNbElement,
                            outputBuffer,
                            dummyOutput );

      }
      else
      {
        retCode = TA_STOCH( startIdx,
                            endIdx,
                            testParam->high,
                            testParam->low,
                            testParam->close,
                            testParam->test->optInPeriod_0,
                            testParam->test->optInPeriod_1,
                            (TA_MAType)testParam->test->optInMAType_1,
                            testParam->test->optInPeriod_2,
                            (TA_MAType)testParam->test->optInMAType_2,
                            outBegIdx, outNbElement,
                            dummyOutput,
                            outputBuffer );
      }

      *lookback = TA_STOCH_Lookback( testParam->test->optInPeriod_0,
                            testParam->test->optInPeriod_1,
                            (TA_MAType)testParam->test->optInMAType_1,
                            testParam->test->optInPeriod_2,
                            (TA_MAType)testParam->test->optInMAType_2 );
      break;
  case TEST_STOCHF:
     if( outputNb == 0 )
     {
        retCode = TA_STOCHF( startIdx,
                             endIdx,
                             testParam->high,
                             testParam->low,
                             testParam->close,
                             testParam->test->optInPeriod_0,
                             testParam->test->optInPeriod_1,
                             (TA_MAType)testParam->test->optInMAType_1,
                             outBegIdx, outNbElement,
                             outputBuffer,
                             dummyOutput );

      }
      else
      {
        retCode = TA_STOCHF( startIdx,
                             endIdx,
                             testParam->high,
                             testParam->low,
                             testParam->close,
                             testParam->test->optInPeriod_0,
                             testParam->test->optInPeriod_1,
                             (TA_MAType)testParam->test->optInMAType_1,
                             outBegIdx, outNbElement,
                             dummyOutput,
                             outputBuffer );
      }

      *lookback = TA_STOCHF_Lookback( testParam->test->optInPeriod_0,
                             testParam->test->optInPeriod_1,
                             (TA_MAType)testParam->test->optInMAType_1 );
      break;

   case TEST_STOCHRSI:
     if( outputNb == 0 )
     {
        retCode = TA_STOCHRSI( startIdx,
                            endIdx,
                            testParam->close,
                            testParam->test->optInPeriod_0,
                            testParam->test->optInPeriod_1,
                            testParam->test->optInPeriod_2,
                            (TA_MAType)testParam->test->optInMAType_2,
                            outBegIdx, outNbElement,
                            outputBuffer,
                            dummyOutput );

      }
      else
      {
        retCode = TA_STOCHRSI( startIdx,
                            endIdx,
                            testParam->close,
                            testParam->test->optInPeriod_0,
                            testParam->test->optInPeriod_1,
                            testParam->test->optInPeriod_2,
                            (TA_MAType)testParam->test->optInMAType_2,
                            outBegIdx, outNbElement,
                            dummyOutput,
                            outputBuffer );
      }

      *lookback = TA_STOCHRSI_Lookback( testParam->test->optInPeriod_0,
                            testParam->test->optInPeriod_1,
                            testParam->test->optInPeriod_2,
                            (TA_MAType)testParam->test->optInMAType_2 );
      break;
   }

   TA_Free( dummyOutput );

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

   retCode = TA_NOT_SUPPORTED;

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->high,  history->nbBars );
   setInputBuffer( 1, history->low,   history->nbBars );
   setInputBuffer( 2, history->close, history->nbBars );

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* Set the unstable period requested for that test. */
   switch( test->optInMAType_1 )
   {
   case TA_MAType_EMA:
      retCode = TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, test->unstablePeriod );
      if( retCode != TA_SUCCESS )
         return TA_TEST_TFRR_SETUNSTABLE_PERIOD_FAIL;
      break;
   default:
      /* No unstable period for other methods. */
      break;
   }

   /* Make a simple first call. */
   switch( test->testId )
   {
   case TEST_STOCH:
      retCode = TA_STOCH( test->startIdx,
                          test->endIdx,
                          gBuffer[0].in,
                          gBuffer[1].in,
                          gBuffer[2].in,
                          test->optInPeriod_0,
                          test->optInPeriod_1,
                          (TA_MAType)test->optInMAType_1,
                          test->optInPeriod_2,
                          (TA_MAType)test->optInMAType_2,
                          &outBegIdx, &outNbElement,
                          gBuffer[0].out0,
                          gBuffer[0].out1 );
      break;
   case TEST_STOCHF:
      retCode = TA_STOCHF( test->startIdx,
                           test->endIdx,
                           gBuffer[0].in,
                           gBuffer[1].in,
                           gBuffer[2].in,
                           test->optInPeriod_0,
                           test->optInPeriod_1,
                           (TA_MAType)test->optInMAType_1,
                           &outBegIdx, &outNbElement,
                           gBuffer[0].out0,
                           gBuffer[0].out1 );
      break;
   case TEST_STOCHRSI:
      retCode = TA_STOCHRSI( test->startIdx,
                             test->endIdx,
                             gBuffer[2].in,
                             test->optInPeriod_0,
                             test->optInPeriod_1,
                             test->optInPeriod_2,
                             (TA_MAType)test->optInMAType_2,
                             &outBegIdx, &outNbElement,
                             gBuffer[0].out0,
                             gBuffer[0].out1 );
      break;
   }

   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );
   CHECK_EXPECTED_VALUE( gBuffer[0].out1, 1 );

   if( server_verify_active() )
   {
      const char *funcName;
      double optBuf[5];
      int nbOpt;

      switch( test->testId )
      {
      case TEST_STOCH:
         funcName = "STOCH";
         optBuf[0] = (double)test->optInPeriod_0;
         optBuf[1] = (double)test->optInPeriod_1;
         optBuf[2] = (double)test->optInMAType_1;
         optBuf[3] = (double)test->optInPeriod_2;
         optBuf[4] = (double)test->optInMAType_2;
         nbOpt = 5;
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in,
                                                   gBuffer[2].in, NULL },
                               optBuf, nbOpt,
                               (const TA_Real*[]){ gBuffer[0].out0, gBuffer[0].out1, NULL }, NULL);
         break;
      case TEST_STOCHF:
         funcName = "STOCHF";
         optBuf[0] = (double)test->optInPeriod_0;
         optBuf[1] = (double)test->optInPeriod_1;
         optBuf[2] = (double)test->optInMAType_1;
         nbOpt = 3;
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in,
                                                   gBuffer[2].in, NULL },
                               optBuf, nbOpt,
                               (const TA_Real*[]){ gBuffer[0].out0, gBuffer[0].out1, NULL }, NULL);
         break;
      case TEST_STOCHRSI:
         funcName = "STOCHRSI";
         optBuf[0] = (double)test->optInPeriod_0;
         optBuf[1] = (double)test->optInPeriod_1;
         optBuf[2] = (double)test->optInPeriod_2;
         optBuf[3] = (double)test->optInMAType_2;
         nbOpt = 4;
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[2].in, NULL },
                               optBuf, nbOpt,
                               (const TA_Real*[]){ gBuffer[0].out0, gBuffer[0].out1, NULL }, NULL);
         break;
      }
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   if( test->testId == TEST_STOCH )
   {
      /* Call a local non-optimized version of the function.
       * This way, we make sure that the currently speed optimized
       * version in TA-Lib is not broken.
       */
      retCode = referenceStoch(
                          test->startIdx,
                          test->endIdx,
                          gBuffer[0].in,
                          gBuffer[1].in,
                          gBuffer[2].in,
                          test->optInPeriod_0,
                          test->optInPeriod_1,
                          test->optInMAType_1,
                             test->optInPeriod_2,
                          test->optInMAType_2,
                          &outBegIdx, &outNbElement,
                          gBuffer[1].out0,
                          gBuffer[1].out1 );

      errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
      if( errNb != TA_TEST_PASS )
         return errNb;
      errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
      if( errNb != TA_TEST_PASS )
         return errNb;
      errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
      if( errNb != TA_TEST_PASS )
         return errNb;

      CHECK_EXPECTED_VALUE( gBuffer[1].out0, 0 );
      CHECK_EXPECTED_VALUE( gBuffer[1].out1, 1 );

      /* The non-optimized reference shall be identical to the optimized
       * TA-Lib implementation.
       *
       * checkSameContent verify that all value different than NAN in
       * the first parameter is identical in the second parameter.
       */
      errNb = checkSameContent( gBuffer[1].out0, gBuffer[0].out0 );
      if( errNb != TA_TEST_PASS )
         return errNb;

      errNb = checkSameContent( gBuffer[1].out1, gBuffer[0].out1 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   /* Make another call where the input and the output are the
    * same buffer.
    */
   switch( test->testId )
   {
   case TEST_STOCH:
      retCode = TA_STOCH( test->startIdx,
                          test->endIdx,
                          gBuffer[0].in,
                          gBuffer[1].in,
                          gBuffer[2].in,
                          test->optInPeriod_0,
                          test->optInPeriod_1,
                          (TA_MAType)test->optInMAType_1,
                          test->optInPeriod_2,
                          (TA_MAType)test->optInMAType_2,
                          &outBegIdx, &outNbElement,
                          gBuffer[0].in,
                          gBuffer[1].in );
      break;
   case TEST_STOCHF:
      retCode = TA_STOCHF( test->startIdx,
                           test->endIdx,
                           gBuffer[0].in,
                           gBuffer[1].in,
                           gBuffer[2].in,
                           test->optInPeriod_0,
                           test->optInPeriod_1,
                           (TA_MAType)test->optInMAType_1,
                           &outBegIdx, &outNbElement,
                           gBuffer[0].in,
                           gBuffer[1].in );
      break;
   case TEST_STOCHRSI:
      retCode = TA_STOCHRSI( test->startIdx,
                             test->endIdx,
                             gBuffer[2].in,
                             test->optInPeriod_0,
                             test->optInPeriod_1,
                             test->optInPeriod_2,
                             (TA_MAType)test->optInMAType_2,
                             &outBegIdx, &outNbElement,
                             gBuffer[0].in,
                             gBuffer[1].in );
      break;

   }

   /* The previous call should have the same output as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[0].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkSameContent( gBuffer[0].out1, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].in, 0 );
   CHECK_EXPECTED_VALUE( gBuffer[1].in, 1 );

   if( errNb != TA_TEST_PASS )
      return errNb;


   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.high  = history->high;
   testParam.low   = history->low;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {
      switch( test->testId )
      {
      case TEST_STOCH:
      case TEST_STOCHF:
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_NONE,
                              (void *)&testParam, 2, 0 );
         break;
      case TEST_STOCHRSI:
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_RSI,
                              (void *)&testParam, 2, 0 );
         break;
      }

      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}


/* This is an un-optimized version of the STOCH function */
static TA_RetCode referenceStoch( TA_Integer    startIdx,
                     TA_Integer    endIdx,
                     const TA_Real inHigh[],
                     const TA_Real inLow[],
                     const TA_Real inClose[],
                     TA_Integer    optInPeriod_0, /* From 1 to TA_INTEGER_MAX */
                     TA_Integer    optInPeriod_1, /* From 1 to TA_INTEGER_MAX */
                     TA_Integer    optInMAType_1,
                     TA_Integer    optInPeriod_2, /* From 1 to TA_INTEGER_MAX */
                     TA_Integer    optInMAType_2,
                     TA_Integer   *outBegIdx,
                     TA_Integer   *outNbElement,
                     TA_Real       outSlowK_0[],
                     TA_Real       outSlowD_1[] )
{
   TA_RetCode retCode;
   double Lt, Ht, tmp, *tempBuffer;
   int outIdx;
   int lookbackTotal, lookbackK, lookbackKSlow, lookbackDSlow;
   int trailingIdx, today, i, bufferIsAllocated;

   /* Identify the lookback needed. */
   lookbackK      = optInPeriod_0-1;
   lookbackKSlow  = TA_MA_Lookback( optInPeriod_1, (TA_MAType)optInMAType_1 );
   lookbackDSlow  = TA_MA_Lookback( optInPeriod_2, (TA_MAType)optInMAType_2 );
   lookbackTotal  = lookbackK + lookbackDSlow + lookbackKSlow;

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      /* Succeed... but no data in the output. */
      *outBegIdx    = 0;
      *outNbElement = 0;
      return TA_SUCCESS;
   }

   /* Do the K calculation:
    *
    *    Kt = 100 x ((Ct-Lt)/(Ht-Lt))
    *
    * Kt is today stochastic
    * Ct is today closing price.
    * Lt is the lowest price of the last K Period (including today)
    * Ht is the highest price of the last K Period (including today)
    */

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;

   /* Calculate just enough K for ending up with the caller
    * requested range. (The range of k must consider all
    * the lookback involve with the smoothing).
    */
   trailingIdx = startIdx-lookbackTotal;
   today       = trailingIdx+lookbackK;

   /* Allocate a temporary buffer large enough to
    * store the K.
    *
    * If the output is the same as the input, great
    * we just save ourself one memory allocation.
    */
   bufferIsAllocated = 0;
   if( (outSlowK_0 == inHigh) ||
       (outSlowK_0 == inLow)  ||
       (outSlowK_0 == inClose) )
   {
      tempBuffer = outSlowK_0;
   }
   else if( (outSlowD_1 == inHigh) ||
            (outSlowD_1 == inLow)  ||
            (outSlowD_1 == inClose) )
   {
      tempBuffer = outSlowD_1;
   }
   else
   {
      bufferIsAllocated = 1;
      tempBuffer = TA_Malloc( (endIdx-today+1)*sizeof(TA_Real) );
   }

   /* Do the K calculation */
   while( today <= endIdx )
   {
      /* Find Lt and Ht for the requested K period. */
      Lt = inLow [trailingIdx];
      Ht = inHigh[trailingIdx];
      trailingIdx++;
      for( i=trailingIdx; i <= today; i++ )
      {
         tmp = inLow[i];
         if( tmp < Lt ) Lt = tmp;
         tmp = inHigh[i];
         if( tmp > Ht ) Ht = tmp;
      }

      /* Calculate stochastic. */
      tmp = Ht-Lt;
      if( tmp > 0.0 )
        tempBuffer[outIdx++] = 100.0*((inClose[today]-Lt)/tmp);
      else
        tempBuffer[outIdx++] = 100.0;

      today++;
   }

   /* Un-smoothed K calculation completed. This K calculation is not returned
    * to the caller. It is always smoothed and then return.
    * Some documentation will refer to the smoothed version as being
    * "K-Slow", but often this end up to be shorten to "K".
    */
   retCode = TA_MA( 0, outIdx-1,
                    tempBuffer, optInPeriod_1,
                    (TA_MAType)optInMAType_1,
                    outBegIdx, outNbElement, tempBuffer );


   if( (retCode != TA_SUCCESS) || (*outNbElement == 0) )
   {
      if( bufferIsAllocated )
        TA_Free(  tempBuffer );
      /* Something wrong happen? No further data? */
      *outBegIdx    = 0;
      *outNbElement = 0;
      return retCode;
   }

   /* Calculate the %D which is simply a moving average of
    * the already smoothed %K.
    */
   retCode = TA_MA( 0, (*outNbElement)-1,
                    tempBuffer, optInPeriod_2,
                    (TA_MAType)optInMAType_2,
                    outBegIdx, outNbElement, outSlowD_1 );

   /* Copy tempBuffer into the caller buffer.
    * (Calculation could not be done directly in the
    *  caller buffer because more input data then the
    *  requested range was needed for doing %D).
    */
   memmove( outSlowK_0, &tempBuffer[lookbackDSlow], (*outNbElement) * sizeof(TA_Real) );

   /* Don't need K anymore, free it if it was allocated here. */
   if( bufferIsAllocated )
      TA_Free(  tempBuffer );

   if( retCode != TA_SUCCESS )
   {
      /* Something wrong happen while processing %D? */
      *outBegIdx    = 0;
      *outNbElement = 0;
      return retCode;
   }

   /* Note: Keep the outBegIdx relative to the
    *       caller input before returning.
    */
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}


