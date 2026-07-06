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
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
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
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
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
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
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
                              TA_FUNC_UNST_NONE,
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

      /* The reported lookback is the middle-band MA lookback (independent of
       * startIdx and of the stddev clamp), which also sizes the output buffers:
       * it must NOT grow with the clamp or the buffers would be too small for
       * the MA that ma() writes. The first output may begin after the lookback,
       * but never before it. */
      lookback = TA_BBANDS_Lookback( period, nbDev, nbDev, TA_MAType_MAMA );
      if( lookback != TA_MA_Lookback( period, TA_MAType_MAMA ) || bbBeg < lookback )
      {
         printf( "BBANDS/MAMA #99: startIdx=%d period=%d lookback=%d begIdx=%d\n",
                 s, period, lookback, (int)bbBeg );
         errNb = TA_TEST_TFFR_BAD_MA_LOOKBACK;
         goto done;
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

      /* Buffer sufficiency (startIdx 0 only, where ma() writes the most into the
       * lookback-sized buffer). A caller sizing outputs from the reported
       * lookback allocates (endIdx + 1 - lookback) slots. ma() writes exactly
       * that many into the middle-band buffer and the realignment only re-reads
       * within that region, so no extra room is required. Verify with buffers
       * cut to that size plus a one-element guard that must remain untouched
       * (this also fails loudly if the lookback is ever grown past the MA
       * lookback). */
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
