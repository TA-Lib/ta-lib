/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
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
 *  112400 MF   First version.
 *
 */

/* Description:
 *     Regression test of Bollinger Bands (BBANDS).
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
#include "ta_test_reference.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_Integer doRangeTestFlag;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer    optInTimePeriod;
   TA_Real       optInNbDevUp;
   TA_Real       optInNbDevDn;
   TA_Integer    optInMethod_3;
   TA_Integer    compatibility;

   TA_RetCode expectedRetCode;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer oneOfTheExpectedOutRealIndex1;
   TA_Real    oneOfTheExpectedOutReal1;

   TA_Integer oneOfTheExpectedOutRealIndex2;
   TA_Real    oneOfTheExpectedOutReal2;

} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );
static ErrorNumber test_bbands_mama_alignment( const TA_History *history );
static ErrorNumber test_bbands_sma_fastpath_equivalence( const TA_History *history );
static ErrorNumber test_bbands_sma_stable_variance( void );
static ErrorNumber test_bbands_small_scale( void );
static ErrorNumber test_bbands_reference_datasets( void );

/**** Local variables definitions.     ****/
static TA_Test tableTest[] =
{

   /****************************/
   /*   BBANDS - CLASSIC - EMA */
   /****************************/

   /* No multiplier */
   /* With upper band multiplier only. */
   /* With lower band multiplier only. */
   /* With identical upper/lower multiplier. */
   { 0, 0,  251, 20, 2.0, 2.0, TA_MAType_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,
     19, 252-19,
     13, 93.674,   /* Upper */
     13, 87.679,   /* Middle */
     13, 81.685 }, /* Lower */

   { 0, 0,  251, 20, 2.0, 2.0, TA_MAType_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,
     19, 252-19,
     0, 98.0734,   /* Upper */
     0, 92.8910,   /* Middle */
     0, 87.7086 }, /* Lower */
   /* With distinctive upper/lower multiplier. */

   /****************************/
   /*   BBANDS - CLASSIC - SMA */
   /****************************/
   /* No multiplier */
   /* With upper band multiplier only. */
   /* With lower band multiplier only. */
   /* With identical upper/lower multiplier. */
   { 1, 0,  251, 20, 2.0, 2.0, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,
     19, 252-19,
     0, 98.0734,   /* Upper */
     0, 92.8910,   /* Middle */
     0, 87.7086 }, /* Lower */
   /* With distinctive upper/lower multiplier. */


   /******************************/
   /*   BBANDS - METASTOCK - SMA */
   /******************************/

   /* No multiplier */
   /* With upper band multiplier only. */
   /* With lower band multiplier only. */

   /* With identical upper/lower multiplier. */
   { 1, 0,  251, 20, 2.0, 2.0, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     0, 98.0734,    /* Upper */
     0, 92.8910,    /* Middle */
     0, 87.7086  }, /* Lower */

   /* With distinctive upper/lower multiplier. */

   /******************************/
   /*   BBANDS - METASTOCK - EMA */
   /******************************/

   /* No multiplier */
   { 1, 0,  251, 20, 1.0, 1.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     0, 94.6914,   /* Upper  */
     0, 92.1002,   /* Middle */
     0, 89.5090 }, /* Lower  */
   { 0, 0,  251, 20, 1.0, 1.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     3, 94.0477,   /* Upper  */
     3, 90.7270,   /* Middle */
     3, 87.4063 }, /* Lower  */
   { 0, 0,  251, 20, 1.0, 1.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     252-20, 111.5415,   /* Upper  */
     252-20, 108.5265,   /* Middle */
     252-20, 105.5115 }, /* Lower  */

   /* With upper band multiplier only. */
   { 0, 0,  251, 20, 1.5, 1.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     0, 95.9870,   /* Upper */
     0, 92.1002,   /* Middle */
     0, 89.5090},  /* Lower */
   { 0, 0,  251, 20, 1.5, 1.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     3, 95.7080,  /* Upper */
     3, 90.7270,  /* Middle */
     3, 87.4063}, /* Lower */
   { 0, 0,  251, 20, 1.5, 1.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     252-20, 113.0490,   /* Upper */
     252-20, 108.5265,   /* Middle */
     252-20, 105.5115 }, /* Lower */

   /* With lower band multiplier only. */
   { 1, 0,  251, 20, 1.0, 1.5, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     0, 94.6914,   /* Upper */
     0, 92.1002,   /* Middle */
     0, 88.2134 }, /* Lower */
   { 0, 0,  251, 20, 1.0, 1.5, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     3, 94.0477,  /* Upper */
     3, 90.7270,  /* Middle */
     3, 85.7460}, /* Lower */
   { 0, 0,  251, 20, 1.0, 1.5, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     252-20, 111.5415,   /* Upper */
     252-20, 108.5265,   /* Middle */
     252-20, 104.0040},  /* Lower */

   /* With identical upper/lower multiplier. */
   { 0, 0,  251, 20, 2.0, 2.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     0, 97.2826,  /* Upper */
     0, 92.1002,  /* Middle */
     0, 86.9178}, /* Lower */
   { 0, 0,  251, 20, 2.0, 2.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     1, 97.2637,    /* Upper */
     1, 91.7454,    /* Middle */
     1, 86.2271}, /* Lower */
   { 0, 0,  251, 20, 2.0, 2.0, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     252-20, 114.5564,  /* Upper */
     252-20, 108.5265,  /* Middle */
     252-20, 102.4965}, /* Lower */

   /* With distinctive upper/lower multiplier. */
   { 0, 0,  251, 20, 2.0, 1.5, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     0, 97.2826,   /* Upper */
     0, 92.1002,   /* Middle */
     0, 88.2134 }, /* Lower */
   { 0, 0,  251, 20, 2.0, 1.5, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     3, 97.3684,    /* Upper */
     3, 90.7270,    /* Middle */
     3, 85.7460}, /* Lower */
   { 0, 0,  251, 20, 2.0, 1.5, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,
     19, 252-19,
     252-20, 114.5564, /* Upper */
     252-20, 108.5265, /* Middle */
     252-20, 104.0040} /* Lower */

};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_bbands( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   for( i=0; i < NB_TEST; i++ )
   {

      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "%s Failed Bad Parameter for Test #%d (%d,%d)\n", __FILE__,
                 i,
                 tableTest[i].expectedNbElement,
                 history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "%s Failed Test #%d (Code=%d)\n", __FILE__, i, retValue );
         return retValue;
      }
   }

   /* Regression test for issue #99: BBANDS with TA_MAType_MAMA and a period
    * large enough that the standard-deviation lookback exceeds the (constant)
    * MAMA lookback, forcing a clamp-aware realignment of the middle band.
    */
   retValue = test_bbands_mama_alignment( history );
   if( retValue != TA_TEST_PASS )
   {
      printf( "%s Failed BBANDS/MAMA alignment regression test (#99) (Code=%d)\n",
              __FILE__, retValue );
      return retValue;
   }

   /* Regression test for issue #117: the BBANDS SMA fast path must stay
    * bit-identical to the independent TA_MA(SMA) + TA_STDDEV composition that
    * the general path (and the stream) computes.
    */
   retValue = test_bbands_sma_fastpath_equivalence( history );
   if( retValue != TA_TEST_PASS )
   {
      printf( "%s Failed BBANDS/SMA fast-path equivalence regression test (#117) (Code=%d)\n",
              __FILE__, retValue );
      return retValue;
   }

   /* Cancellation-free variance on an ill-conditioned window (#118). */
   retValue = test_bbands_sma_stable_variance();
   if( retValue != TA_TEST_PASS )
   {
      printf( "%s Failed BBANDS/SMA stable-variance test (#118) (Code=%d)\n",
              __FILE__, retValue );
      return retValue;
   }

   /* Scale-relative dead-zone: small non-zero deviation, both paths (#243). */
   retValue = test_bbands_small_scale();
   if( retValue != TA_TEST_PASS )
   {
      printf( "%s Failed BBANDS small-scale ladder (#243) (Code=%d)\n",
              __FILE__, retValue );
      return retValue;
   }

   /* External reference datasets, shared with VAR/CORREL/BETA (#251). */
   retValue = test_bbands_reference_datasets();
   if( retValue != TA_TEST_PASS )
   {
      printf( "%s Failed BBANDS reference datasets (#251) (Code=%d)\n",
              __FILE__, retValue );
      return retValue;
   }

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
  TA_Real *dummyBuffer1, *dummyBuffer2;
  TA_Real *out1, *out2, *out3;

  (void)outputBufferInt;

  *isOutputInteger = 0;

  testParam = (TA_RangeTestParam *)opaqueData;

  dummyBuffer1 = TA_Malloc( ((endIdx-startIdx)+1)*sizeof(TA_Real));
  if( !dummyBuffer1 )
     return TA_ALLOC_ERR;

  dummyBuffer2 = TA_Malloc( ((endIdx-startIdx)+1)*sizeof(TA_Real));
  if( !dummyBuffer2 )
  {
     TA_Free(  dummyBuffer1 );
     return TA_ALLOC_ERR;
  }

  switch( outputNb )
  {
  case 0:
     out1 = outputBuffer;
     out2 = dummyBuffer1;
     out3 = dummyBuffer2;
     break;
  case 1:
     out2 = outputBuffer;
     out1 = dummyBuffer1;
     out3 = dummyBuffer2;
     break;
  case 2:
     out3 = outputBuffer;
     out2 = dummyBuffer1;
     out1 = dummyBuffer2;
     break;
  default:
     TA_Free(  dummyBuffer1 );
     TA_Free(  dummyBuffer2 );
     return TA_BAD_PARAM;
  }

   retCode = TA_BBANDS( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        testParam->test->optInNbDevUp,
                        testParam->test->optInNbDevDn,
                        (TA_MAType)testParam->test->optInMethod_3,
                        outBegIdx, outNbElement,
                        out1, out2, out3 );

   *lookback = TA_BBANDS_Lookback( testParam->test->optInTimePeriod,
                                   testParam->test->optInNbDevUp,
                                   testParam->test->optInNbDevDn,
                                   (TA_MAType)testParam->test->optInMethod_3 );

   TA_Free(  dummyBuffer1 );
   TA_Free(  dummyBuffer2 );

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

   retCode = TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   if( retCode != TA_SUCCESS )
      return TA_TEST_TFRR_SETUNSTABLE_PERIOD_FAIL;

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->close, history->nbBars );
   setInputBuffer( 1, history->close, history->nbBars );
   setInputBuffer( 2, history->close, history->nbBars );
   setInputBuffer( 3, history->close, history->nbBars );

   TA_SetCompatibility( (TA_Compatibility)test->compatibility );

   /* Make a simple first call. */
   retCode = TA_BBANDS( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInTimePeriod,
                        test->optInNbDevUp,
                        test->optInNbDevDn,
                        (TA_MAType)test->optInMethod_3,

                        &outBegIdx, &outNbElement,
                        gBuffer[0].out0,
                        gBuffer[0].out1,
                        gBuffer[0].out2 );

   errNb = checkDataSame( gBuffer[0].in, history->close, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );
   CHECK_EXPECTED_VALUE( gBuffer[0].out1, 1 );
   CHECK_EXPECTED_VALUE( gBuffer[0].out2, 2 );

   if( server_verify_active() )
   {
      errNb = server_verify("BBANDS", test->startIdx, test->endIdx, history->nbBars,
                            retCode, outBegIdx, outNbElement,
                            (const TA_Real*[]){ gBuffer[0].in, NULL },
                            (double[]){ (double)test->optInTimePeriod, test->optInNbDevUp,
                                        test->optInNbDevDn, (double)test->optInMethod_3 }, 4,
                            (const TA_Real*[]){ gBuffer[0].out0, gBuffer[0].out1,
                                               gBuffer[0].out2, NULL }, NULL);
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_BBANDS( test->startIdx,
                        test->endIdx,
                        gBuffer[1].in,
                        test->optInTimePeriod,
                        test->optInNbDevUp,
                        test->optInNbDevDn,
                        (TA_MAType)test->optInMethod_3,
                        &outBegIdx, &outNbElement,
                        gBuffer[1].in, gBuffer[1].out1, gBuffer[1].out2 );

   /* The previous call should have the same output
    * as this call.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[1].in,   0 );
   CHECK_EXPECTED_VALUE( gBuffer[1].out1, 1 );
   CHECK_EXPECTED_VALUE( gBuffer[1].out2, 2 );

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_BBANDS( test->startIdx,
                        test->endIdx,
                        gBuffer[2].in,
                        test->optInTimePeriod,
                        test->optInNbDevUp,
                        test->optInNbDevDn,
                        (TA_MAType)test->optInMethod_3,
                        &outBegIdx, &outNbElement,
                        gBuffer[2].out1,
                        gBuffer[2].in,
                        gBuffer[2].out2 );

   /* The previous call should have the same output
    * as this call.
    */
   errNb = checkSameContent( gBuffer[1].out1, gBuffer[2].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[2].out1, 0 );
   CHECK_EXPECTED_VALUE( gBuffer[2].in,   1 );
   CHECK_EXPECTED_VALUE( gBuffer[2].out2, 2 );

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_BBANDS( test->startIdx,
                        test->endIdx,
                        gBuffer[3].in,
                        test->optInTimePeriod,
                        test->optInNbDevUp,
                        test->optInNbDevDn,
                        (TA_MAType)test->optInMethod_3,
                        &outBegIdx, &outNbElement,
                        gBuffer[3].out0,
                        gBuffer[3].out1,
                        gBuffer[3].in );

   /* The previous call should have the same output
    * as this call.
    */
   errNb = checkSameContent( gBuffer[2].out2, gBuffer[3].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[3].out0, 0 );
   CHECK_EXPECTED_VALUE( gBuffer[3].out1, 1 );
   CHECK_EXPECTED_VALUE( gBuffer[3].in,   2 );

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {
      if( test->optInMethod_3 == TA_MAType_EMA )
      {
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_EMA,
                              (void *)&testParam, 3, 0 );
      }
      else
      {
         errNb = doRangeTest( rangeTestFunction,
                              TA_TEST_UNST_NONE,
                              (void *)&testParam, 3, 0 );
      }

      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

/* Deterministic regression test for issue #99.
 *
 * BBANDS builds the middle band from a moving average (lookback = ma_lookback)
 * and the outer bands from a simple standard deviation (lookback =
 * optInTimePeriod - 1). TA_MAType_MAMA is the only MA type whose lookback is a
 * constant (32) independent of the period, so it is the only type for which the
 * standard-deviation lookback can exceed the MA lookback. When it does
 * (optInTimePeriod >= 34, with the default MAMA unstable period), the inner
 * stddev clamps to a later begIdx than the MA did; BBANDS must realign the MA
 * results so that every output bar pairs its moving average with the standard
 * deviation computed for the SAME bar. The pre-fix code did not, misaligning the
 * middle band by (optInTimePeriod - 33) bars and combining an MA and an SD taken
 * from different bars in the upper/lower bands.
 *
 * This test is a self-contained oracle: the middle band IS the MAMA output and
 * the band offset IS nbDev times the standard deviation, so it recomputes the
 * expected bands from the library's own TA_MAMA and TA_STDDEV (both correct and
 * independent of the bug, which lived only in how BBANDS combined them) and
 * compares element by element. It fails on the pre-fix code and passes on the
 * fixed code, with no random inputs — a permanent CI gate. It also cross-checks
 * every active language server (when run under --codegen) for the same call.
 */
static ErrorNumber test_bbands_mama_alignment( const TA_History *history )
{
   /* {startIdx, period}. period>=34 clamps (stddev lookback > MAMA lookback); 33
    * is the boundary (no clamp); 20 is the reverse (MAMA lookback dominates).
    * startIdx 0 realigns off the constant MAMA base (32); a startIdx in
    * (32, period-1) exercises the realignment with a VARIABLE base (maBegIdx =
    * startIdx); a startIdx >= period-1 is a no-clamp control (shiftIdx == 0). */
   static const struct { int startIdx; int period; } cases[] = {
      {  0,  34 }, {  0,  40 }, {  0,  50 }, {  0, 100 }, {  0,  33 }, {  0,  20 },
      { 40,  50 }, { 49,  50 }, { 60,  50 }
   };
   const int nbCases = (int)( sizeof(cases) / sizeof(cases[0]) );
   const double nbDev = 2.0;
   const int endIdx = (int)history->nbBars - 1;
   int p, i;
   ErrorNumber errNb = TA_TEST_PASS;

   double *mama, *fama, *sd, *up, *mid, *low;

   TA_SetUnstablePeriod( TA_FUNC_UNST_MAMA, 0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   mama = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   fama = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   sd   = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   up   = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   mid  = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   low  = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   if( !mama || !fama || !sd || !up || !mid || !low )
   {
      errNb = TA_TESTUTIL_TFRR_BAD_PARAM;
      goto done;
   }

   for( p = 0; p < nbCases; p++ )
   {
      const int s      = cases[p].startIdx;
      const int period = cases[p].period;
      TA_RetCode rc;
      TA_Integer mamaBeg, mamaNb, sdBeg, sdNb, bbBeg, bbNb;
      int expectedBeg, expectedNb, maOff, sdOff, lookback;

      /* Independent references, computed from the SAME startIdx as BBANDS uses
       * internally: BBANDS forwards startIdx to ma()->mama(), so TA_MAMA is
       * bit-identical to BBANDS's middle band, and the standard deviation is
       * value-identical regardless of startIdx. */
      rc = TA_MAMA( s, endIdx, history->close, 0.5, 0.05,
                    &mamaBeg, &mamaNb, mama, fama );
      if( rc != TA_SUCCESS ) { errNb = TA_TESTUTIL_TFRR_BAD_RETCODE; goto done; }

      rc = TA_STDDEV( s, endIdx, history->close, period, 1.0,
                      &sdBeg, &sdNb, sd );
      if( rc != TA_SUCCESS ) { errNb = TA_TESTUTIL_TFRR_BAD_RETCODE; goto done; }

      rc = TA_BBANDS( s, endIdx, history->close, period, nbDev, nbDev,
                      TA_MAType_MAMA, &bbBeg, &bbNb, up, mid, low );
      if( rc != TA_SUCCESS ) { errNb = TA_TESTUTIL_TFRR_BAD_RETCODE; goto done; }

      /* The bands are valid only where BOTH the MA and the SD exist. */
      expectedBeg = mamaBeg > sdBeg ? mamaBeg : sdBeg;
      expectedNb  = endIdx - expectedBeg + 1;

      /* The reported lookback is the LATER of the middle-band MA lookback and the
       * standard-deviation lookback (optInTimePeriod-1) - a band value needs both
       * (issue #93 made it honest; #99 had reported only the MA lookback, so
       * outBegIdx could exceed it). For MAMA it stays the constant 32 until
       * optInTimePeriod-1 overtakes it at period >= 34, from which point the
       * lookback tracks the standard deviation. outBegIdx never begins before it. */
      lookback = TA_BBANDS_Lookback( period, nbDev, nbDev, TA_MAType_MAMA );
      {
         int maLb = TA_MA_Lookback( period, TA_MAType_MAMA );
         int sdLb = TA_STDDEV_Lookback( period, 1.0 );
         int wantLb = maLb > sdLb ? maLb : sdLb;
         if( lookback != wantLb || bbBeg < lookback )
         {
            printf( "BBANDS/MAMA #99: startIdx=%d period=%d lookback=%d begIdx=%d\n",
                    s, period, lookback, (int)bbBeg );
            errNb = TA_TEST_TFFR_BAD_MA_LOOKBACK;
            goto done;
         }
      }

      if( bbBeg != expectedBeg )
      {
         printf( "BBANDS/MAMA #99: startIdx=%d period=%d begIdx=%d expected=%d\n",
                 s, period, (int)bbBeg, expectedBeg );
         errNb = TA_TESTUTIL_TFRR_BAD_BEGIDX;
         goto done;
      }
      if( (int)bbNb != expectedNb )
      {
         printf( "BBANDS/MAMA #99: startIdx=%d period=%d nbElement=%d expected=%d\n",
                 s, period, (int)bbNb, expectedNb );
         errNb = TA_TESTUTIL_TFRR_BAD_OUTNBELEMENT;
         goto done;
      }

      /* Skip the leading MA (or SD) values that have no counterpart. */
      maOff = expectedBeg - (int)mamaBeg;
      sdOff = expectedBeg - (int)sdBeg;

      for( i = 0; i < (int)bbNb; i++ )
      {
         const double maVal  = mama[i + maOff];
         const double sdVal  = sd[i + sdOff];
         const double expMid = maVal;
         const double expUp  = maVal + nbDev * sdVal;
         const double expLow = maVal - nbDev * sdVal;

         if( fabs( mid[i] - expMid ) > 1e-8 ||
             fabs( up[i]  - expUp  ) > 1e-8 ||
             fabs( low[i] - expLow ) > 1e-8 )
         {
            printf( "BBANDS/MAMA #99: startIdx=%d period=%d i=%d (bar %d) "
                    "mid=%.10g/%.10g up=%.10g/%.10g low=%.10g/%.10g\n",
                    s, period, i, expectedBeg + i,
                    mid[i], expMid, up[i], expUp, low[i], expLow );
            errNb = TA_TESTUTIL_TFRR_BAD_CALCULATION;
            goto done;
         }
      }

      /* Buffer sufficiency (startIdx 0 only). A caller sizing outputs from the
       * reported lookback allocates (endIdx + 1 - lookback) slots. With the honest
       * lookback that is exactly outNBElement: the general path computes the MA
       * into a scratch buffer and memmoves only the realigned band values into the
       * caller's array, so it never writes past that region. Verify with buffers
       * cut to that size plus a one-element guard that must remain untouched. */
      if( s == 0 )
      {
         const int    tight = endIdx + 1 - lookback;
         const double guard = -1.7e308;
         double *gu = (double *)TA_Malloc( (tight + 1) * sizeof(double) );
         double *gm = (double *)TA_Malloc( (tight + 1) * sizeof(double) );
         double *gl = (double *)TA_Malloc( (tight + 1) * sizeof(double) );
         TA_Integer gBeg = 0, gNb = 0;

         if( !gu || !gm || !gl )
         {
            if( gu ) TA_Free( gu );
            if( gm ) TA_Free( gm );
            if( gl ) TA_Free( gl );
            errNb = TA_TESTUTIL_TFRR_BAD_PARAM;
            goto done;
         }

         gu[tight] = gm[tight] = gl[tight] = guard;
         rc = TA_BBANDS( 0, endIdx, history->close, period, nbDev, nbDev,
                         TA_MAType_MAMA, &gBeg, &gNb, gu, gm, gl );

         if( rc != TA_SUCCESS ||
             gu[tight] != guard || gm[tight] != guard || gl[tight] != guard )
         {
            printf( "BBANDS/MAMA #99: period=%d buffer overrun on lookback-sized "
                    "alloc (%d slots): guard u=%.3g m=%.3g l=%.3g\n",
                    period, tight, gu[tight], gm[tight], gl[tight] );
            TA_Free( gu );
            TA_Free( gm );
            TA_Free( gl );
            errNb = TA_TESTUTIL_TFRR_BAD_CALCULATION;
            goto done;
         }

         TA_Free( gu );
         TA_Free( gm );
         TA_Free( gl );
      }

      /* Cross-check every active language server for the same call. */
      if( server_verify_active() )
      {
         errNb = server_verify( "BBANDS", s, endIdx, (int)history->nbBars,
                                rc, bbBeg, bbNb,
                                (const TA_Real*[]){ history->close, NULL },
                                (double[]){ (double)period, nbDev, nbDev,
                                            (double)TA_MAType_MAMA }, 4,
                                (const TA_Real*[]){ up, mid, low, NULL }, NULL );
         if( errNb != TA_TEST_PASS )
            goto done;
      }
   }

done:
   if( mama ) TA_Free( mama );
   if( fama ) TA_Free( fama );
   if( sd   ) TA_Free( sd );
   if( up   ) TA_Free( up );
   if( mid  ) TA_Free( mid );
   if( low  ) TA_Free( low );

   return errNb;
}

/* Regression test for issue #117 (BBANDS SMA fast-path fusion).
 *
 * The SMA fast path must stay BIT-IDENTICAL to the TA_MA(SMA) + TA_STDDEV
 * composition the general path and the stream evaluate. This recomputes the
 * expected bands from those two independent functions and compares with EXACT
 * equality, replicating the band arithmetic (mid + std*nbDevUp, mid -
 * std*nbDevDn) so any ULP-level change in the fused loop fails loudly. Sweeps
 * periods, both band-multiplier branches, and several start indices; also
 * cross-checks every active language server (under --codegen).
 */
static ErrorNumber test_bbands_sma_fastpath_equivalence( const TA_History *history )
{
   static const int periods[] = { 2, 3, 5, 14, 20, 50 };
   static const struct { double up, dn; } devs[] = {
      { 1.0, 1.0 }, { 2.0, 2.0 }, { 2.0, 1.5 }, { 0.5, 3.0 },
      { 1.5, 2.0 }   /* distinct with a non-power-of-2 upper: exercises the fma
                      * band path where fma != mul+add (guards the #117 fix) */
   };
   static const int starts[] = { 0, 30, 100 };
   const int nbPer   = (int)( sizeof(periods) / sizeof(periods[0]) );
   const int nbDev   = (int)( sizeof(devs)    / sizeof(devs[0])    );
   const int nbStart = (int)( sizeof(starts)  / sizeof(starts[0])  );
   const int endIdx  = (int)history->nbBars - 1;
   int pi, di, si, i;
   ErrorNumber errNb = TA_TEST_PASS;

   double *sma, *sd, *up, *mid, *low;

   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   sma = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   sd  = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   up  = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   mid = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   low = (double *)TA_Malloc( history->nbBars * sizeof(double) );
   if( !sma || !sd || !up || !mid || !low )
   {
      errNb = TA_TESTUTIL_TFRR_BAD_PARAM;
      goto done;
   }

   for( pi = 0; pi < nbPer; pi++ )
   {
      const int period = periods[pi];
      for( si = 0; si < nbStart; si++ )
      {
         const int s = starts[si];
         TA_RetCode rc;
         TA_Integer maBeg, maNb, sdBeg, sdNb;

         if( s > endIdx )
            continue;

         /* Independent references from the SAME startIdx BBANDS uses internally.
          * For SMA both the MA lookback and the stddev lookback are period-1, so
          * all three functions clamp to the identical begIdx / element count. */
         rc = TA_MA( s, endIdx, history->close, period, TA_MAType_SMA,
                     &maBeg, &maNb, sma );
         if( rc != TA_SUCCESS ) { errNb = TA_TESTUTIL_TFRR_BAD_RETCODE; goto done; }

         rc = TA_STDDEV( s, endIdx, history->close, period, 1.0,
                         &sdBeg, &sdNb, sd );
         if( rc != TA_SUCCESS ) { errNb = TA_TESTUTIL_TFRR_BAD_RETCODE; goto done; }

         if( maBeg != sdBeg || maNb != sdNb )
         {
            printf( "BBANDS/SMA #117: period=%d startIdx=%d MA(beg=%d,nb=%d) != "
                    "STDDEV(beg=%d,nb=%d)\n",
                    period, s, (int)maBeg, (int)maNb, (int)sdBeg, (int)sdNb );
            errNb = TA_TESTUTIL_TFRR_BAD_BEGIDX;
            goto done;
         }

         for( di = 0; di < nbDev; di++ )
         {
            const double nbDevUp = devs[di].up;
            const double nbDevDn = devs[di].dn;
            TA_Integer bbBeg, bbNb;

            rc = TA_BBANDS( s, endIdx, history->close, period, nbDevUp, nbDevDn,
                            TA_MAType_SMA, &bbBeg, &bbNb, up, mid, low );
            if( rc != TA_SUCCESS ) { errNb = TA_TESTUTIL_TFRR_BAD_RETCODE; goto done; }

            if( bbBeg != maBeg || (int)bbNb != (int)maNb )
            {
               printf( "BBANDS/SMA #117: period=%d startIdx=%d up=%g dn=%g "
                       "BBANDS(beg=%d,nb=%d) != MA(beg=%d,nb=%d)\n",
                       period, s, nbDevUp, nbDevDn, (int)bbBeg, (int)bbNb,
                       (int)maBeg, (int)maNb );
               errNb = TA_TESTUTIL_TFRR_BAD_BEGIDX;
               goto done;
            }

            for( i = 0; i < (int)bbNb; i++ )
            {
               /* Replicate the fast path's EXACT band arithmetic so the compare
                * is bit-for-bit. Middle IS the SMA. The band loop has two forms
                * (ta_BBANDS.c): equal multipliers reuse one rounded product
                * (mid +/- dev*nbDev); distinct multipliers fuse the upper band as
                * a single-rounding fma(dev, nbDevUp, mid) and take the lower as an
                * unfused mid - dev*nbDevDn (a subtraction never fuses). Modelling
                * the upper as mid + dev*nbDevUp instead would only match when
                * dev*nbDevUp is exact (power-of-two nbDevUp), silently mis-blaming
                * the library on a future non-power-of-two multiplier. */
               const double expMid = sma[i];
               double expUp, expLow;
               if( nbDevUp == nbDevDn )
               {
                  const double off = sd[i] * nbDevUp;
                  expUp  = sma[i] + off;
                  expLow = sma[i] - off;
               }
               else
               {
                  expUp  = fma( sd[i], nbDevUp, sma[i] );
                  expLow = sma[i] - ( sd[i] * nbDevDn );
               }

               if( mid[i] != expMid || up[i] != expUp || low[i] != expLow )
               {
                  printf( "BBANDS/SMA #117: period=%d startIdx=%d up=%g dn=%g "
                          "i=%d (bar %d) mid=%.17g/%.17g up=%.17g/%.17g "
                          "low=%.17g/%.17g\n",
                          period, s, nbDevUp, nbDevDn, i, (int)bbBeg + i,
                          mid[i], expMid, up[i], expUp, low[i], expLow );
                  errNb = TA_TESTUTIL_TFRR_BAD_CALCULATION;
                  goto done;
               }
            }

            /* Cross-check every active language server for the same call. */
            if( server_verify_active() )
            {
               errNb = server_verify( "BBANDS", s, endIdx, (int)history->nbBars,
                                      rc, bbBeg, bbNb,
                                      (const TA_Real*[]){ history->close, NULL },
                                      (double[]){ (double)period, nbDevUp, nbDevDn,
                                                  (double)TA_MAType_SMA }, 4,
                                      (const TA_Real*[]){ up, mid, low, NULL }, NULL );
               if( errNb != TA_TEST_PASS )
                  goto done;
            }
         }
      }
   }

done:
   if( sma ) TA_Free( sma );
   if( sd  ) TA_Free( sd );
   if( up  ) TA_Free( up );
   if( mid ) TA_Free( mid );
   if( low ) TA_Free( low );

   return errNb;
}

/* Guard the cancellation-free variance (issue #118).
 *
 * On an ill-conditioned window (large offset, a few units of variation) the old
 * E[x^2] - mean^2 form loses most of its digits and can collapse to zero/negative
 * (SourceForge bug 90). The shifted-data variance in var.c/the BBANDS SMA path
 * stays accurate there. This pins the property on exactly such a window: the
 * deviation must match a stable mean-centered two-pass to ~1e-9 and stay non-zero,
 * and the window must be genuinely ill-conditioned (the old form measurably off),
 * else the guard proves nothing.
 */
static ErrorNumber test_bbands_sma_stable_variance( void )
{
   enum { NB = 12 };
   const int    period = NB;          /* one period-wide window -> one output */
   const double base   = 1.0e6;
   const double nbDev  = 2.0;
   double in[NB];
   double up[1], mid[1], low[1];
   double sumSq = 0.0, meanRef = 0.0, varRef, stdRef, oldVar, dev;
   TA_Integer begIdx = 0, nbElt = 0;
   TA_RetCode rc;
   int i;

   for( i = 0; i < NB; i++ )
      in[i] = base + (double)( ( i * 7 ) % 5 - 2 );   /* {-2..+2} around 1e6 */

   /* Reference: the shared two-pass (#251). It used to be a local plain-double
    * one, which on a window this ill-conditioned is only a little better than
    * the form it is meant to catch. */
   varRef = ta_test_ref_var( in, 0, period, &meanRef );
   stdRef = ta_test_ref_stddev( in, 0, period, NULL );

   /* The old cancelling form, for the non-vacuity check below. */
   for( i = 0; i < NB; i++ ) sumSq += in[i] * in[i];
   oldVar = sumSq / (double)period - meanRef * meanRef;

   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );
   rc = TA_BBANDS( 0, NB - 1, in, period, nbDev, nbDev, TA_MAType_SMA,
                   &begIdx, &nbElt, up, mid, low );
   if( rc != TA_SUCCESS || nbElt != 1 )
   {
      printf( "BBANDS/SMA #118: rc=%d nb=%d (expected SUCCESS,1)\n",
              (int)rc, (int)nbElt );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* Non-vacuity: the window must be ill-conditioned, i.e. the old E[x^2]-mean^2
    * form is measurably wrong here (otherwise this window proves nothing). */
   if( !( varRef > 1e-6 && fabs( oldVar - varRef ) / varRef > 1e-6 ) )
   {
      printf( "BBANDS/SMA #118: window not ill-conditioned (varRef=%.6g "
              "oldVar=%.6g)\n", varRef, oldVar );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* The band deviation must match the stable variance and not collapse. */
   dev = ( up[0] - mid[0] ) / nbDev;
   if( dev <= 0.0 || fabs( dev - stdRef ) / stdRef > 1e-9 )
   {
      printf( "BBANDS/SMA #118: deviation %.17g not the stable variance %.17g "
              "(rel %.3g)\n", dev, stdRef, fabs( dev - stdRef ) / stdRef );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* Stale-anchor regression: a mid-series level shift leaves the shift far from
    * the later windows, whose (small-but-not-tiny) variance is then extracted from
    * a difference of ~1e12 quantities. A reseed that fires only on total collapse
    * misses this partial cancellation and emits a silent, positive-but-wrong
    * deviation. Assert every band tracks a stable two-pass across the whole series. */
   {
      enum { M = 200 };
      const int p = 20;
      double s[M], sUp[M], sMid[M], sLow[M];
      TA_Integer sBeg, sNb;
      int kk;

      for( kk = 0; kk < M; kk++ )
         s[kk] = ( kk < 60 ) ? 1.0e6 : ( 3.0 + (double)( ( kk * 7 ) % 11 - 5 ) * 0.1 );

      rc = TA_BBANDS( 0, M - 1, s, p, nbDev, nbDev, TA_MAType_SMA,
                      &sBeg, &sNb, sUp, sMid, sLow );
      if( rc != TA_SUCCESS )
      {
         printf( "BBANDS/SMA #118 stale-anchor: rc=%d\n", (int)rc );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( kk = 0; kk < (int)sNb; kk++ )
      {
         const int bar = (int)sBeg + kk;
         double sd = ta_test_ref_stddev( s, bar - p + 1, p, NULL );
         double d  = ( sUp[kk] - sMid[kk] ) / nbDev;
         if( sd > 0.0 && fabs( d - sd ) / sd > 1e-9 )
         {
            printf( "BBANDS/SMA #118 stale-anchor: bar %d dev=%.17g stable=%.17g "
                    "(rel %.3g)\n", bar, d, sd, fabs( d - sd ) / sd );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* Issue #243: the bands must not all land on the middle band when the deviation
 * is small but plainly non-zero.
 *
 * BBANDS inherits STDDEV's dead-zone, and it had one on each of its two paths:
 * the general path through TA_STDDEV, and the fused SMA fast path, which carries
 * its own copy of the square root. Both compared a SQUARED quantity to a fixed
 * TA_EPSILON (1e-14), so a series quoted finely enough -- a $100.00 instrument on
 * a 1e-8 tick -- returned three identical bands and TA_SUCCESS.
 *
 * The deviation is refereed against the shared two-pass sigma at both ends of
 * the ladder and on both paths. The tolerance model is the one documented in
 * test_stddev.c (test_stddev_small_scale); nbDevUp != nbDevDn so the asymmetric
 * combine loop is the one exercised.
 */
static ErrorNumber test_bbands_small_scale( void )
{
   enum { N = TA_TEST_REF_TICKS60_N };
   static const double bases[2] = { 0.0, 100.0 };
   static const TA_MAType maTypes[3] = { TA_MAType_SMA, TA_MAType_EMA, TA_MAType_WMA };
   static double x[N], up[N], mid[N], low[N];
   const double devUp = 2.0, devDn = 3.0;
   const int period = 5;
   TA_Integer b, nb;
   TA_RetCode rc;
   int bi, mi, i, k, decade;

   for( bi = 0; bi < 2; bi++ )
   for( mi = 0; mi < 3; mi++ )
   {
      double tick = 1.0e-2;
      for( decade = 0; decade < 12; decade++, tick /= 10.0 )
      {
         for( i = 0; i < N; i++ ) x[i] = bases[bi] + (double)ta_test_ref_ticks60[i] * tick;

         rc = TA_BBANDS( 0, N-1, x, period, devUp, devDn, maTypes[mi],
                         &b, &nb, up, mid, low );
         if( rc != TA_SUCCESS )
         {
            printf( "BBANDS #243: rc=%d base=%g tick=%g maType=%d\n",
                    (int)rc, bases[bi], tick, (int)maTypes[mi] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( k = 0; k < (int)nb; k++ )
         {
            const int bar = (int)b + k;
            /* The oracle moved to ta_test_reference.c (#251) and stopped using
             * `long double`, which is 64 mantissa bits here and 53 on MSVC --
             * so this bound used to mean something weaker on Windows than it
             * says. The shared form carries ~106 bits on every ABI. */
            double m = 0.0;
            double refSig = ta_test_ref_stddev( x, bar - period + 1, period, &m );
            double dev, kappa, tol, d;

            /* The width carries both deviations: (up-low) = (devUp+devDn)*sigma. */
            dev = ( up[k] - low[k] ) / ( devUp + devDn );
            if( refSig == 0.0 )
            {
               if( dev != 0.0 )
               {
                  printf( "BBANDS #243: expected zero width base=%g tick=%g maType=%d "
                          "bar=%d dev=%.17g\n", bases[bi], tick, (int)maTypes[mi], bar, dev );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               continue;
            }
            if( dev == 0.0 )
            {
               printf( "BBANDS #243: bands COLLAPSED base=%g tick=%g maType=%d bar=%d "
                       "(two-pass sigma %.17g)\n",
                       bases[bi], tick, (int)maTypes[mi], bar, refSig );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            /* Two conditioning terms, and only the first is about the variance.
             * The second is the price of reading sigma back OUT of the bands:
             * up-low cancels the middle band away, so recovering a deviation of
             * refSig from two numbers of size |mid| costs |mid|*eps/(width). At
             * the bottom of the ladder that term is percent-scale, and honestly
             * so -- a 1e-13 sigma is simply not recoverable from bands centred at
             * $100. It is the `dev == 0.0` branch above, not this bound, that
             * makes the rung non-vacuous: the collapse is exact zero, always. */
            kappa = fabs( m ) / refSig;
            tol   = 1.0e-11 + 1.0e-6 * ( kappa * 2.2204460492503131e-16 )
                                     * ( kappa * 2.2204460492503131e-16 )
                            + 8.0 * fabs( mid[k] ) * 2.2204460492503131e-16
                                  / ( ( devUp + devDn ) * refSig );
            d     = fabs( dev - refSig ) / refSig;
            if( d > tol )
            {
               printf( "BBANDS #243: base=%g tick=%g maType=%d bar=%d dev=%.17g "
                       "ref=%.17g (rel %.3g > %.3g, kappa %.2g)\n",
                       bases[bi], tick, (int)maTypes[mi], bar, dev, refSig, d, tol, kappa );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}

/* Issue #251: BBANDS against the shared reference battery.
 *
 * Before this, every numerical probe BBANDS had was built from data invented in
 * this file. It now also faces the same external datasets TA_VAR, TA_CORREL and
 * TA_BETA are refereed by -- which matters because BBANDS does not call
 * TA_STDDEV on its SMA path: it carries its own fused copy of the variance and
 * the square root, so agreeing with TA_STDDEV is not the same claim as being
 * right.
 *
 * Three references, in increasing order of independence:
 *
 *   R1  NIST StRD NumAcc1..4 -- the certified population variance of a
 *       purpose-built cancellation stressor, so the band half-width IS a
 *       certified number. NumAcc4's offset is 1e7 under a spread of 0.1.
 *   R2  pandas' rolling-var adversarial arrays, against goldens computed in
 *       exact rational arithmetic outside this binary.
 *   R3  the sliding-sum ladder, likewise baked, at four periods.
 *
 * All three read the deviation back as (upper - lower) / (nbDevUp + nbDevDn),
 * which is the only way a caller can see it. That subtraction cancels the middle
 * band away, so recovering a deviation of sigma from two numbers of size |mid|
 * costs |mid|*eps/width -- a real term, and the reason each bound below carries
 * it explicitly rather than hiding it in a fudge factor.
 */
static ErrorNumber test_bbands_reference_datasets( void )
{
   const double devUp = 1.0, devDn = 1.0;
   static double buf[1001], up[1001], mid[1001], low[1001];
   TA_Integer b, nb;
   TA_RetCode rc;
   int i, k, t;

   /* R1: NIST StRD NumAcc1..4. The certified value is the SAMPLE variance;
    * TA-Lib is population, so the expected sigma is sqrt(s^2*(n-1)/n). The
    * per-case tolerances are test_stddev.c's, for the same reason: NumAcc2/3/4
    * carry decimal-representation error that grows with the offset. */
   {
      static const struct { const char *name; int n; double expVar; double tol; } cases[4] = {
         { "NumAcc1",    3, 2.0/3.0,      1.0e-12 },
         { "NumAcc2", 1001, 10.0/1001.0,  1.0e-12 },
         { "NumAcc3", 1001, 10.0/1001.0,  1.0e-8  },
         { "NumAcc4", 1001, 10.0/1001.0,  1.0e-6  } };
      for( i = 0; i < 4; i++ )
      {
         double expSig = sqrt( cases[i].expVar );
         double dev, tol, d;
         int n = ta_test_ref_numacc( i+1, buf );
         if( n != cases[i].n )
         {
            printf( "BBANDS #251 NIST %s: shared battery returned %d values, expected %d\n",
                    cases[i].name, n, cases[i].n );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         rc = TA_BBANDS( 0, n-1, buf, n, devUp, devDn, TA_MAType_SMA, &b, &nb, up, mid, low );
         if( rc != TA_SUCCESS || nb != 1 )
         {
            printf( "BBANDS #251 NIST %s: rc=%d nb=%d\n", cases[i].name, (int)rc, (int)nb );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         dev = ( up[0] - low[0] ) / ( devUp + devDn );
         tol = cases[i].tol
               + 8.0 * fabs( mid[0] ) * 2.2204460492503131e-16
                 / ( ( devUp + devDn ) * expSig );
         d   = fabs( dev - expSig ) / expSig;
         if( d > tol )
         {
            printf( "BBANDS #251 NIST %s: half-width %.17g certified sigma %.17g "
                    "(rel %.3g > %.3g)\n", cases[i].name, dev, expSig, d, tol );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* R2 and R3: baked goldens. The variance tables are compared as sigma, since
    * that is what the bands carry. */
   {
      static const struct { const char *name; const double *x; int n; int period;
                           const double *goldVar; const double *goldSig; int nbGold; } sets[] = {
         { "pandas GH47721", ta_test_ref_pd_var47721, TA_TEST_REF_PD_VAR47721_N, 6,
           ta_test_ref_golden_var47721, NULL, TA_TEST_REF_GOLDEN_VAR47721_N },
         { "pandas GH52407", ta_test_ref_pd_var52407, TA_TEST_REF_PD_VAR52407_N, 3,
           ta_test_ref_golden_var52407, NULL, TA_TEST_REF_GOLDEN_VAR52407_N },
         { "ladder p2",  ta_test_ref_ladder, TA_TEST_REF_LADDER_N,  2, NULL,
           ta_test_ref_golden_ladder_p2_sigma,  TA_TEST_REF_GOLDEN_LADDER_P2_SIGMA_N },
         { "ladder p5",  ta_test_ref_ladder, TA_TEST_REF_LADDER_N,  5, NULL,
           ta_test_ref_golden_ladder_p5_sigma,  TA_TEST_REF_GOLDEN_LADDER_P5_SIGMA_N },
         { "ladder p14", ta_test_ref_ladder, TA_TEST_REF_LADDER_N, 14, NULL,
           ta_test_ref_golden_ladder_p14_sigma, TA_TEST_REF_GOLDEN_LADDER_P14_SIGMA_N },
         { "ladder p30", ta_test_ref_ladder, TA_TEST_REF_LADDER_N, 30, NULL,
           ta_test_ref_golden_ladder_p30_sigma, TA_TEST_REF_GOLDEN_LADDER_P30_SIGMA_N } };

      for( t = 0; t < (int)( sizeof(sets)/sizeof(sets[0]) ); t++ )
      {
         const int period = sets[t].period;
         const int n = sets[t].n;
         if( n - period + 1 != sets[t].nbGold )
         {
            printf( "BBANDS #251 [%s]: %d windows but %d baked values -- this test and "
                    "scripts/gen_test_reference.py disagree about the corpus\n",
                    sets[t].name, n - period + 1, sets[t].nbGold );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         rc = TA_BBANDS( 0, n-1, sets[t].x, period, devUp, devDn, TA_MAType_SMA,
                         &b, &nb, up, mid, low );
         if( rc != TA_SUCCESS || (int)nb != sets[t].nbGold )
         {
            printf( "BBANDS #251 [%s]: rc=%d nb=%d (expected SUCCESS,%d)\n",
                    sets[t].name, (int)rc, (int)nb, sets[t].nbGold );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( k = 0; k < (int)nb; k++ )
         {
            const int bar = (int)b + k;
            double refSig = sets[t].goldSig ? sets[t].goldSig[k] : sqrt( sets[t].goldVar[k] );
            double mean = 0.0, dev, kappa, tol, d;

            /* The bands must not be able to cross, whatever the data. */
            if( !( up[k] >= mid[k] && mid[k] >= low[k] ) )
            {
               printf( "BBANDS #251 [%s]: bands out of order period=%d bar=%d "
                       "up=%.17g mid=%.17g low=%.17g\n",
                       sets[t].name, period, bar, up[k], mid[k], low[k] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            dev = ( up[k] - low[k] ) / ( devUp + devDn );
            if( refSig == 0.0 )
            {
               if( dev != 0.0 )
               {
                  printf( "BBANDS #251 [%s]: expected zero width period=%d bar=%d dev=%.17g\n",
                          sets[t].name, period, bar, dev );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               continue;
            }
            if( dev == 0.0 )
            {
               printf( "BBANDS #251 [%s]: bands COLLAPSED period=%d bar=%d (sigma %.17g)\n",
                       sets[t].name, period, bar, refSig );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            ta_test_ref_var( sets[t].x, bar - period + 1, period, &mean );
            /* Two conditioning terms, the same pair test_bbands_small_scale
             * documents: the variance's own kappa, and the price of reading
             * sigma back out of two bands centred on |mid|. */
            kappa = fabs( mean ) / refSig;
            tol   = 1.0e-11
                    + 1.0e-6 * ( kappa * 2.2204460492503131e-16 )
                             * ( kappa * 2.2204460492503131e-16 )
                    + 8.0 * fabs( mid[k] ) * 2.2204460492503131e-16
                          / ( ( devUp + devDn ) * refSig );
            d = fabs( dev - refSig ) / refSig;
            if( d > tol )
            {
               printf( "BBANDS #251 [%s]: period=%d bar=%d half-width=%.17g golden sigma=%.17g "
                       "(rel %.3g > %.3g, kappa %.2g)\n",
                       sets[t].name, period, bar, dev, refSig, d, tol, kappa );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}
