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
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   First version.
 *  020605 MF   Add regression test with inverted slow/fast period.
 *  020805 AA   Fix one of the TA_PPO call (wrong buffer was pass).
 *  092726 MF,CC  PPO/PVO leg: a slow window that goes dead after fractional
 *                values (#454).
 */

/* Description:
 *     Regression test of APO(Absolute Price Oscillator).
 *     Regression test of PPO (Percentage Price Oscillator).
 *     PVO shares PPO's loop, so the dead-window leg covers both.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
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

   TA_Integer doPercentage;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer optInFastPeriod; /* From 1 to 200 */
   TA_Integer optInSlowPeriod; /* From 1 to 200 */
   TA_Integer optInMethod_2;

   TA_RetCode expectedRetCode;

   TA_Integer oneOfTheExpectedOutRealIndex;
   TA_Real    oneOfTheExpectedOutReal;


   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;
} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

static ErrorNumber test_default_is_ema( const TA_History *history,
                                        const char *funcName,
                                        int doPercentage );

static ErrorNumber test_dead_after_fractional( int pvo );
static ErrorNumber test_sma_fusion( void );

/**** Local variables definitions.     ****/
static TA_Test tableTest[] =
{
   /************************/
   /*    APO TEST - SIMPLE */
   /************************/
   { 1, 0, 0, 251, 26, 12, TA_MAType_SMA, TA_SUCCESS,      0, -3.3124, 25,  252-25 }, /* First Value */
   { 1, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_SUCCESS,      0, -3.3124, 25,  252-25 }, /* First Value */
   { 0, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_SUCCESS,      1, -3.5876, 25,  252-25 },
   { 0, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_SUCCESS, 252-26, -0.1667, 25,  252-25 }, /* Last Value */

   { 0, 0, 0,   1, 12, 26, TA_MAType_SMA, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 1,   1, 12, 26, TA_MAType_SMA, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 25,  25, 12, 26, TA_MAType_SMA, TA_SUCCESS,   0,  -3.3124,   25,  1 }, /* First/Last Value */
   { 0, 0, 250, 251, 12, 26, TA_MAType_SMA, TA_SUCCESS,   1,  -0.1667,  250,  2 }, /* Last  Value */

   /*****************************/
   /*    APO TEST - EXPONENTIAL */
   /*****************************/
   /* The EMA arm is range-DEPENDENT: seeded with the SMA of its own first
    * window, a call starting at bar S warms the fast EMA from S-lookback while
    * a full-range call has run it since bar 11. Both regimes are pinned; a
    * single-output range is the seed itself, which is why those rows carry the
    * SMA value. doRangeTest is handed TA_FUNC_UNST_EMA so it compares the
    * stable tail. */
   { 1, 0, 0, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS,      0, -4.1103, 25,  252-25 }, /* First Value */
   { 0, 0, 0, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS,      1, -4.0027, 25,  252-25 },
   { 0, 0, 0, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS, 252-26, 0.90401, 25,  252-25 }, /* Last Value */

   { 0, 0, 0,   1, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 1,   1, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 25,  25, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,  -3.3124,   25,  1 }, /* Just enough to calculate first. */
   { 0, 0, 26,  26, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,  -3.5876,   26,  1 }, /* Just enough to calculate second. */
   { 0, 0, 250, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS,   1, -0.07817,  250,  2 }, /* Last  Value */
   { 0, 0, 251, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,  -0.1667,  251,  1 }, /* Last  Value */

   /************************/
   /*    PPO TEST - SIMPLE */
   /************************/
   { 1, 1, 0, 251, 2, 3, TA_MAType_SMA, TA_SUCCESS,   0,  1.10264, 2,  252-2 }, /* First Value */
   /* Was -0.02813 (#188). Closes 94.815 / 94.375 / 95.095 give SMA2 = 94.735 and
    * SMA3 = 94.7616666..., so (SMA2-SMA3)/SMA3*100 = -0.0281407742230402. A
    * search over percentage-oscillator variants (/slow, /fast, /mean, absolute)
    * across period pairs and bars finds nothing yielding -0.02813. */
   { 0, 1, 0, 251, 2, 3, TA_MAType_SMA, TA_SUCCESS,   1, -0.0281407742, 2,  252-2 },
   { 0, 1, 0, 251, 2, 3, TA_MAType_SMA, TA_SUCCESS, 249, -0.21191, 2,  252-2 }, /* Last Value */

   { 0, 1, 0,   1, 2, 3, TA_MAType_SMA, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 0, 1, 1,   1, 2, 3, TA_MAType_SMA, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 0, 1, 2,   2, 2, 3, TA_MAType_SMA, TA_SUCCESS,   0,  1.10264,   2,  1 }, /* First/Last Value */
   { 0, 1, 250, 251, 2, 3, TA_MAType_SMA, TA_SUCCESS,   1, -0.21191, 250,  2 }, /* Last  Value */

   /* Test period inversion */
   { 1, 1, 0, 251, 3, 2, TA_MAType_SMA, TA_SUCCESS,   0,  1.10264, 2,  252-2 }, /* First Value */
   { 0, 1, 0, 251, 3, 2, TA_MAType_SMA, TA_SUCCESS, 249, -0.21191, 2,  252-2 }, /* Last Value */

   { 0, 1, 0, 251, 12, 26, TA_MAType_SMA, TA_SUCCESS,      0, -3.6393, 25,  252-25 }, /* First Value */
   { 0, 1, 0, 251, 12, 26, TA_MAType_SMA, TA_SUCCESS,      1, -3.9534, 25,  252-25 },
   { 0, 1, 0, 251, 12, 26, TA_MAType_SMA, TA_SUCCESS, 252-26, -0.15281, 25,  252-25 }, /* Last Value */

   { 0, 1, 0,   1, 12, 26, TA_MAType_SMA, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 0, 1, 1,   1, 12, 26, TA_MAType_SMA, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 0, 1, 25,  25, 12, 26, TA_MAType_SMA, TA_SUCCESS,   0, -3.6393,   25,  1 }, /* First/Last Value */
   { 0, 1, 250, 251, 12, 26, TA_MAType_SMA, TA_SUCCESS,   1, -0.15281, 250,  2 }, /* Last  Value */

   /*****************************/
   /*    PPO TEST - EXPONENTIAL */
   /*****************************/
   /* Whole-history values only -- see the APO EMA note above. */
   { 1, 1, 0, 251, 26, 12, TA_MAType_EMA, TA_SUCCESS,      0, -4.5159, 25,  252-25 }, /* First Value */
   { 0, 1, 0, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS,      1, -4.4214, 25,  252-25 },
   { 0, 1, 0, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS, 252-26, 0.83645, 25,  252-25 }, /* Last Value */

   { 0, 1, 0,   1, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 1, 1,   1, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 1, 25,  25, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,  -3.6393,   25,  1 }, /* Just enough to calculate first. */
   { 0, 1, 26,  26, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0,  -3.9534,   26,  1 }, /* Just enough to calculate second. */
   { 0, 1, 250, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS,   1, -0.07175,  250,  2 }, /* Last  Value */
   { 0, 1, 251, 251, 12, 26, TA_MAType_EMA, TA_SUCCESS,   0, -0.15281,  251,  1 }, /* Last  Value */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_po( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   for( i=0; i < NB_TEST; i++ )
   {

      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "TA_APO/TA_PPO Failed Bad Parameter for Test #%d (%d,%d)\n",
                 i, tableTest[i].expectedNbElement, history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "TA_APO/TA_PPO Failed Test #%d (Code=%d)\n", i, retValue );
         return retValue;
      }
   }

   /* Issue #120: PPO and APO default optInMAType to EMA (Gerald Appel's
    * original PPO/MACD definition), not SMA. Lock that in.
    */
   retValue = test_default_is_ema( history, "PPO", 1 );
   if( retValue != 0 )
      return retValue;

   retValue = test_default_is_ema( history, "APO", 0 );
   if( retValue != 0 )
      return retValue;

   retValue = test_dead_after_fractional( 0 );
   if( retValue != 0 )
      return retValue;

   retValue = test_dead_after_fractional( 1 );
   if( retValue != 0 )
      return retValue;

   /* #459: APO, PPO and PVO compute their two SMAs in one pass. This proves
    * that path bit-identical to the two-TA_MA path it replaced.
    */
   retValue = test_sma_fusion();
   if( retValue != 0 )
      return retValue;

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
                                     unsigned  int outputNb,
                                     unsigned int *isOutputInteger )
{
   TA_RetCode retCode;
   TA_RangeTestParam *testParam;

   (void)outputNb;
   (void)outputBufferInt;

   *isOutputInteger = 0;

   testParam = (TA_RangeTestParam *)opaqueData;

   if( testParam->test->doPercentage )
   {
      retCode = TA_PPO( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInFastPeriod,
                        testParam->test->optInSlowPeriod,
                        (TA_MAType)testParam->test->optInMethod_2,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );

     *lookback = TA_PPO_Lookback( testParam->test->optInFastPeriod,
                      testParam->test->optInSlowPeriod,
                      (TA_MAType)testParam->test->optInMethod_2 );
   }
   else
   {
      retCode = TA_APO( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInFastPeriod,
                        testParam->test->optInSlowPeriod,
                        (TA_MAType)testParam->test->optInMethod_2,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );


     *lookback = TA_APO_Lookback( testParam->test->optInFastPeriod,
                      testParam->test->optInSlowPeriod,
                      (TA_MAType)testParam->test->optInMethod_2 );
   }

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
   setInputBuffer( 0, history->close, history->nbBars );
   setInputBuffer( 1, history->close, history->nbBars );

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );

   /* Make a simple first call. */
   if( test->doPercentage )
   {
      retCode = TA_PPO( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInFastPeriod,
                        test->optInSlowPeriod,
                        (TA_MAType)test->optInMethod_2,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
   }
   else
   {
      retCode = TA_APO( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInFastPeriod,
                        test->optInSlowPeriod,
                        (TA_MAType)test->optInMethod_2,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
   }

   errNb = checkDataSame( gBuffer[0].in, history->close, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkExpectedValue( gBuffer[0].out0,
                               retCode, test->expectedRetCode,
                               outBegIdx, test->expectedBegIdx,
                               outNbElement, test->expectedNbElement,
                               test->oneOfTheExpectedOutReal,
                               test->oneOfTheExpectedOutRealIndex );
   if( errNb != TA_TEST_PASS )
      return errNb;

   if( server_verify_active() )
   {
      const char *funcName = test->doPercentage ? "PPO" : "APO";
      errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                            retCode, outBegIdx, outNbElement,
                            (const TA_Real*[]){ gBuffer[0].in, NULL },
                            (double[]){ (double)test->optInFastPeriod,
                                        (double)test->optInSlowPeriod,
                                        (double)test->optInMethod_2 }, 3,
                            (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   if( test->doPercentage )
   {
      retCode = TA_PPO( test->startIdx,
                        test->endIdx,
                        gBuffer[1].in,
                        test->optInFastPeriod,
                        test->optInSlowPeriod,
                        (TA_MAType)test->optInMethod_2,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[1].in );
   }
   else
   {
      retCode = TA_APO( test->startIdx,
                        test->endIdx,
                        gBuffer[1].in,
                        test->optInFastPeriod,
                        test->optInSlowPeriod,
                        (TA_MAType)test->optInMethod_2,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[1].in );
   }

   /* The previous call should have the same output
    * as this call.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkExpectedValue( gBuffer[1].in,
                               retCode, test->expectedRetCode,
                               outBegIdx, test->expectedBegIdx,
                               outNbElement, test->expectedNbElement,
                               test->oneOfTheExpectedOutReal,
                               test->oneOfTheExpectedOutRealIndex );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {

      if( test->optInMethod_2 == TA_MAType_EMA )
      {
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_EMA,
                              (void *)&testParam, 1, 0 );
         if( errNb != TA_TEST_PASS )
            return errNb;
      }
      else
      {
         errNb = doRangeTest( rangeTestFunction,
                              TA_TEST_UNST_NONE,
                              (void *)&testParam, 1, 0 );
         if( errNb != TA_TEST_PASS )
            return errNb;
      }
   }

   return TA_TEST_PASS;
}

/* Issue #120 regression: verify that PPO/APO's optInMAType defaults to EMA.
 *
 * Three independent checks (mirrors test_pvo_default_is_ema in test_composite.c):
 *   (a) the ta_abstract declared default value is TA_MAType_EMA;
 *   (b) driving the function through ta_abstract while leaving optInMAType at
 *       its allocator-initialized default produces the SAME output (bit-exact)
 *       as an explicit EMA call — and, thanks to a vacuity guard, NOT the SMA
 *       one; and
 *   (c) calling the guarded C function directly with TA_INTEGER_DEFAULT for
 *       optInMAType (the sentinel-substitution path) also yields EMA.
 * Sabotage-proven: flipping the yaml default back to 0 fails (a)/(b) via the
 * abstract default, and (c) via the C sentinel substitution.
 */
#define PO_OUT_CAP 300   /* > nbBars (252) */
static ErrorNumber test_default_is_ema( const TA_History *history,
                                        const char *funcName,
                                        int doPercentage )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo   *funcInfo;
   TA_ParamHolder      *paramHolder;
   TA_RetCode           rc;
   TA_Integer           emaBeg, emaNb, smaBeg, smaNb, defBeg, defNb, senBeg, senNb;
   static TA_Real       emaOut[PO_OUT_CAP], smaOut[PO_OUT_CAP], defOut[PO_OUT_CAP];
   static TA_Real       senOut[PO_OUT_CAP];
   int                  endIdx = (int)history->nbBars - 1;
   int                  maTypeIdx = -1;
   int                  maTypeFound = 0;
   unsigned int         i;

   /* Deterministic global state: EMA has an unstable period; pin it to 0 so the
    * explicit-EMA and default-MAType calls are directly comparable. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );

   /* (a) Declared default: the MAType optional input defaults to EMA. */
   if( TA_GetFuncHandle( funcName, &handle ) != TA_SUCCESS ||
       TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "%s default Fail: cannot get func handle/info\n", funcName );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   for( i = 0; i < funcInfo->nbOptInput; i++ )
   {
      const TA_OptInputParameterInfo *optInfo;
      TA_GetOptInputParameterInfo( handle, i, &optInfo );
      if( optInfo->paramName && strstr( optInfo->paramName, "MAType" ) )
      {
         maTypeFound = 1;
         maTypeIdx = (int)i;
         if( (int)optInfo->defaultValue != (int)TA_MAType_EMA )
         {
            printf( "%s default Fail: optInMAType default = %d, expected EMA (%d)\n",
                    funcName, (int)optInfo->defaultValue, (int)TA_MAType_EMA );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   if( !maTypeFound )
   {
      printf( "%s default Fail: no MAType optional input found\n", funcName );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   /* (b) below leaves optInMAType unset by NOT calling its setter; that relies
    * on it being optional input index 2 (after fast/slow period). Guard it. */
   if( maTypeIdx != 2 )
   {
      printf( "%s default Fail: optInMAType is opt-input %d, expected 2\n",
              funcName, maTypeIdx );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   /* Explicit EMA and SMA references. They MUST differ, or (b) proves nothing. */
   if( doPercentage )
   {
      if( TA_PPO( 0, endIdx, history->close, 12, 26, TA_MAType_EMA, &emaBeg, &emaNb, emaOut ) != TA_SUCCESS ||
          TA_PPO( 0, endIdx, history->close, 12, 26, TA_MAType_SMA, &smaBeg, &smaNb, smaOut ) != TA_SUCCESS )
      {
         printf( "%s default Fail: explicit TA_PPO call failed\n", funcName );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }
   else
   {
      if( TA_APO( 0, endIdx, history->close, 12, 26, TA_MAType_EMA, &emaBeg, &emaNb, emaOut ) != TA_SUCCESS ||
          TA_APO( 0, endIdx, history->close, 12, 26, TA_MAType_SMA, &smaBeg, &smaNb, smaOut ) != TA_SUCCESS )
      {
         printf( "%s default Fail: explicit TA_APO call failed\n", funcName );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }
   if( emaNb != smaNb ||
       memcmp( emaOut, smaOut, (size_t)emaNb * sizeof(TA_Real) ) == 0 )
   {
      printf( "%s default Fail: EMA and SMA outputs identical — test would be vacuous\n", funcName );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (b) Behavioural: drive the function through ta_abstract setting only the
    * fast+slow periods, leaving optInMAType at its allocator-initialized
    * default; the result must be the EMA one (bit-exact) — hence NOT SMA. */
   if( TA_ParamHolderAlloc( handle, &paramHolder ) != TA_SUCCESS )
   {
      printf( "%s default Fail: TA_ParamHolderAlloc failed\n", funcName );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   if( TA_SetInputParamRealPtr( paramHolder, 0, history->close ) != TA_SUCCESS ||
       TA_SetOptInputParamInteger( paramHolder, 0, 12 ) != TA_SUCCESS ||  /* optInFastPeriod */
       TA_SetOptInputParamInteger( paramHolder, 1, 26 ) != TA_SUCCESS ||  /* optInSlowPeriod */
       /* optInMAType (index maTypeIdx) is deliberately NOT set -> uses the default. */
       TA_SetOutputParamRealPtr( paramHolder, 0, defOut ) != TA_SUCCESS )
   {
      printf( "%s default Fail: abstract param setup failed\n", funcName );
      TA_ParamHolderFree( paramHolder );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   rc = TA_CallFunc( paramHolder, 0, endIdx, &defBeg, &defNb );
   TA_ParamHolderFree( paramHolder );
   if( rc != TA_SUCCESS )
   {
      printf( "%s default Fail: TA_CallFunc (default MAType) rc=%d\n", funcName, (int)rc );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( defBeg != emaBeg || defNb != emaNb ||
       memcmp( defOut, emaOut, (size_t)defNb * sizeof(TA_Real) ) != 0 )
   {
      printf( "%s default Fail: default-MAType output != explicit EMA "
              "(the default is not EMA)\n", funcName );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (c) C-level default sentinel: calling the guarded function directly with
    * TA_INTEGER_DEFAULT for optInMAType substitutes the default (the
    * `if(optInMAType==TA_INTEGER_DEFAULT) optInMAType=<default>` line), which
    * must now be EMA — bit-exact with the explicit EMA reference. */
   if( doPercentage )
      rc = TA_PPO( 0, endIdx, history->close, 12, 26,
                   (TA_MAType)TA_INTEGER_DEFAULT, &senBeg, &senNb, senOut );
   else
      rc = TA_APO( 0, endIdx, history->close, 12, 26,
                   (TA_MAType)TA_INTEGER_DEFAULT, &senBeg, &senNb, senOut );
   if( rc != TA_SUCCESS )
   {
      printf( "%s default Fail: TA_INTEGER_DEFAULT MAType call rc=%d\n", funcName, (int)rc );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( senBeg != emaBeg || senNb != emaNb ||
       memcmp( senOut, emaOut, (size_t)senNb * sizeof(TA_Real) ) != 0 )
   {
      printf( "%s default Fail: TA_INTEGER_DEFAULT-MAType output != explicit EMA "
              "(the C-level default is not EMA)\n", funcName );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}


/* A slow window that goes dead after fractional values reads 0, as ppo.md and
 * pvo.md document, where SMA, WMA, TRIMA and HMA leave rounding residue in the
 * slow MA. Every other bar, and every bar of EMA (a recursive type, really
 * nonzero there), is the plain ratio of the two TA_MA outputs, bit for bit.
 *
 * Anchor DW_EDGE seeds one bar into the zero run, so the first dead bar is
 * known dead only by counting the zero bars before the first output: the warm-up
 * in batch and Open, and the count the handle carries into Update. The witness
 * check proves the slow MA holds residue past TA_IS_ZERO there, at both anchors.
 *
 * The dead run starts early enough to fall inside the first 2*lookback+10 bars
 * that server_verify's ride-along replays through Open+Update in every
 * language; moving it later leaves that replay short of it, silently. */
#define DW_N     60
#define DW_ZS    8
#define DW_ZE    28
#define DW_EDGE  (DW_ZS + 1)
#define DW_FAST  3
#define DW_SLOW  5

typedef struct { TA_PPO_Stream *ppo; TA_PVO_Stream *pvo; } DwStream;

static TA_RetCode dw_batch( int pvo, int s, int e, const TA_Real *in, TA_MAType t,
                            TA_Integer *beg, TA_Integer *nb, TA_Real *out )
{
   return pvo ? TA_PVO( s, e, in, DW_FAST, DW_SLOW, t, beg, nb, out )
              : TA_PPO( s, e, in, DW_FAST, DW_SLOW, t, beg, nb, out );
}

static TA_RetCode dw_open( int pvo, DwStream *h, const TA_Real *in, int len, TA_MAType t,
                           TA_Real *out )
{
   return pvo ? TA_PVO_Open( &h->pvo, in, len, DW_FAST, DW_SLOW, t, out )
              : TA_PPO_Open( &h->ppo, in, len, DW_FAST, DW_SLOW, t, out );
}

static TA_RetCode dw_fill( int pvo, DwStream *h, const TA_Real *in, int len, TA_MAType t,
                           TA_Integer *beg, TA_Integer *nb, TA_Real *out )
{
   return pvo ? TA_PVO_OpenAndFill( &h->pvo, in, len, DW_FAST, DW_SLOW, t, beg, nb, out )
              : TA_PPO_OpenAndFill( &h->ppo, in, len, DW_FAST, DW_SLOW, t, beg, nb, out );
}

static TA_RetCode dw_step( int pvo, DwStream *h, TA_Real v, TA_Real *peek, TA_Real *got )
{
   TA_RetCode rc;
   rc = pvo ? TA_PVO_Peek( h->pvo, v, peek ) : TA_PPO_Peek( h->ppo, v, peek );
   if( rc != TA_SUCCESS )
      return rc;
   return pvo ? TA_PVO_Update( h->pvo, v, got ) : TA_PPO_Update( h->ppo, v, got );
}

static void dw_close( DwStream *h )
{
   if( h->pvo ) TA_PVO_Close( h->pvo );
   if( h->ppo ) TA_PPO_Close( h->ppo );
   h->pvo = NULL;
   h->ppo = NULL;
}

static ErrorNumber test_dead_after_fractional( int pvo )
{
   static const TA_MAType types[5] = { TA_MAType_SMA, TA_MAType_WMA, TA_MAType_TRIMA,
                                       TA_MAType_HMA, TA_MAType_EMA };
   static TA_Real x[DW_N], fast[DW_N], slow[DW_N], want[DW_N], out[DW_N];
   static TA_Real res[3][DW_N];
   const char *name = pvo ? "PVO" : "PPO";
   TA_Integer beg, nb, fBeg, fNb, sBeg, sNb;
   TA_Real got, peek;
   TA_RetCode rc;
   DwStream h = { NULL, NULL };
   const TA_Real *in;
   int k, c, i, lb, s, n, windowed, base[3], anchor[3], len;

   for( i = 0; i < DW_N; i++ )
      x[i] = ( i >= DW_ZS && i < DW_ZE ) ? 0.0 : 1002.69 + 1.3 * (double)( i % 3 );

   /* EMA, the control, seeds the same way on every call only without an
    * unstable period. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );

   for( k = 0; k < 5; k++ )
   {
      windowed = types[k] != TA_MAType_EMA;
      lb = TA_MA_Lookback( DW_SLOW, types[k] );

      /* Case 0 runs from bar 0; cases 1 and 2 both have DW_EDGE as their
       * first output bar, anchored there and sliced there. */
      base[0] = 0;  anchor[0] = 0;
      base[1] = 0;  anchor[1] = DW_EDGE;
      base[2] = DW_EDGE - lb;  anchor[2] = 0;

      for( c = 0; c < 3; c++ )
      {
         in = x + base[c];
         n  = DW_N - base[c];
         s  = anchor[c] < lb ? lb : anchor[c];
         /* The MAs from the anchor PPO/PVO hand them: their running sums, and so
          * the bits, depend on where each one seeds. */
         rc = TA_MA( anchor[c], n - 1, in, DW_FAST, types[k], &fBeg, &fNb, fast );
         if( rc == TA_SUCCESS )
            rc = TA_MA( anchor[c], n - 1, in, DW_SLOW, types[k], &sBeg, &sNb, slow );
         if( rc != TA_SUCCESS || sBeg != s || fBeg > sBeg )
         {
            printf( "%s dead-after-fractional [type %d case %d]: TA_MA retCode %d\n",
                    name, (int)types[k], c, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( windowed && TA_IS_ZERO( slow[DW_ZS + lb - base[c] - s] ) )
         {
            printf( "%s dead-after-fractional [type %d case %d]: the slow MA holds no "
                    "residue at the first dead bar, so this corpus no longer tells the "
                    "dead window apart\n", name, (int)types[k], c );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = s; i < n; i++ )
         {
            if( windowed && i + base[c] - lb >= DW_ZS && i + base[c] < DW_ZE )
               want[i] = 0.0;
            else if( TA_IS_ZERO( slow[i - s] ) )
               want[i] = 0.0;
            else
               want[i] = ( ( fast[i - fBeg] - slow[i - s] ) / slow[i - s] ) * 100.0;
         }

         rc = dw_batch( pvo, anchor[c], n - 1, in, types[k], &beg, &nb, res[c] );
         if( rc != TA_SUCCESS || beg != s || nb != n - s )
         {
            printf( "%s dead-after-fractional Fail [type %d case %d]: retCode %d (%d,%d)\n",
                    name, (int)types[k], c, (int)rc, (int)beg, (int)nb );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = s; i < n; i++ )
         {
            got = res[c][i - s];
            if( memcmp( &got, &want[i], sizeof(double) ) != 0 )
            {
               printf( "%s dead-after-fractional Fail [type %d case %d] at bar %d: %.17g, "
                       "expected %.17g (bars %d..%d have an all-zero slow window)\n",
                       name, (int)types[k], c, i + base[c], got, want[i],
                       DW_ZS + lb, DW_ZE - 1 );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }

      /* Peek and Update on every bar after an Open ending one bar into the zero
       * run, one ending past the first dead bar, and case 2's minimal Open. */
      for( c = 0; c < 3; c++ )
      {
         const TA_Real *r = res[c == 2 ? 2 : 0];
         in  = x + ( c == 2 ? base[2] : 0 );
         n   = DW_N - ( c == 2 ? base[2] : 0 );
         len = c == 0 ? DW_ZS + 1 : c == 1 ? DW_ZS + lb + 3 : lb + 1;
         rc = dw_open( pvo, &h, in, len, types[k], &got );
         if( rc != TA_SUCCESS || memcmp( &got, &r[len - 1 - lb], sizeof(double) ) != 0 )
         {
            printf( "%s dead-after-fractional stream Fail [type %d open %d]: retCode %d "
                    "%.17g, batch %.17g\n", name, (int)types[k], c, (int)rc, got,
                    r[len - 1 - lb] );
            dw_close( &h );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = len; i < n; i++ )
         {
            rc = dw_step( pvo, &h, in[i], &peek, &got );
            if( rc != TA_SUCCESS || memcmp( &peek, &r[i - lb], sizeof(double) ) != 0
                || memcmp( &got, &r[i - lb], sizeof(double) ) != 0 )
            {
               printf( "%s dead-after-fractional stream Fail [type %d open %d] at bar %d: "
                       "retCode %d peek %.17g update %.17g batch %.17g\n", name,
                       (int)types[k], c, i + ( c == 2 ? base[2] : 0 ), (int)rc, peek, got,
                       r[i - lb] );
               dw_close( &h );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         dw_close( &h );
      }

      rc = dw_fill( pvo, &h, x, DW_N, types[k], &beg, &nb, out );
      dw_close( &h );
      if( rc != TA_SUCCESS || beg != lb || nb != DW_N - lb
          || memcmp( out, res[0], (size_t)nb * sizeof(double) ) != 0 )
      {
         printf( "%s dead-after-fractional OpenAndFill Fail [type %d]: retCode %d (%d,%d)\n",
                 name, (int)types[k], (int)rc, (int)beg, (int)nb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      if( server_verify_active() )
      {
         double optIn[3];
         ErrorNumber e;
         int cmpBefore, rideBefore;

         optIn[0] = (double)DW_FAST;
         optIn[1] = (double)DW_SLOW;
         optIn[2] = (double)types[k];
         for( c = 0; c < 3; c += 2 )
         {
            n = DW_N - base[c];
            cmpBefore = server_verify_comparisons();
            rideBefore = server_verify_ride_cases();
            e = server_verify( name, 0, n - 1, n, TA_SUCCESS, lb, n - lb,
                               (const TA_Real*[]){ x + base[c], NULL }, optIn, 3,
                               (const TA_Real*[]){ res[c], NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* Every server that compared the batch must also have replayed it
             * through its stream: case 2's Open is the one that must count the
             * zero bars before its first output. */
            if( server_verify_comparisons() == cmpBefore
                || server_verify_ride_cases() - rideBefore
                   != server_verify_comparisons() - cmpBefore )
            {
               printf( "%s dead-after-fractional [type %d case %d]: %d server(s) compared "
                       "the batch, %d replayed it through the stream\n", name,
                       (int)types[k], c, server_verify_comparisons() - cmpBefore,
                       server_verify_ride_cases() - rideBefore );
               return TA_SV_ROUTED_VACUOUS;
            }
         }
      }
   }

   return TA_TEST_PASS;
}
#undef DW_N
#undef DW_ZS
#undef DW_ZE
#undef DW_EDGE
#undef DW_FAST
#undef DW_SLOW

/* #459: the SMA fast path of APO, PPO and PVO computes the fast and the slow
 * moving average in ONE pass over the input, where the general MA path calls
 * TA_MA twice and allocates the fast MA in full. This leg is that path's
 * oracle: the same output assembled from two TA_SMA calls and the same
 * dead-window rule the general path applies (#454), compared BIT FOR BIT.
 *
 * Exact equality is the point. Each running sum is path-dependent, so a fused
 * sum that reached its first output bar by a shorter route than TA_SMA takes
 * still agrees to about fifteen digits; the tableTest entries above, which
 * match to five, would not notice. The cases sweep the swap (fast > slow), the
 * equal-period degenerate, startIdx below / at / above each lookback, and a run
 * of zeros so PPO and PVO reach both their TA_IS_ZERO arm and their dead window.
 */
#define PO_FUSE_N   600
#define PO_FUSE_CMP 85200

static ErrorNumber test_sma_fusion( void )
{
   static const struct { int fast, slow; } pairs[] = {
      { 2, 3 }, { 3, 2 }, { 12, 26 }, { 26, 12 }, { 5, 5 }, { 2, 100 }, { 99, 100 }, { 2, 2 }
   };
   static const int starts[] = { 0, 1, 25, 26, 99, 100, 300, PO_FUSE_N-1 };
   static double in[PO_FUSE_N], fastBuf[PO_FUSE_N], slowBuf[PO_FUSE_N], got[PO_FUSE_N];
   TA_RetCode rc;
   TA_Integer beg, nb, fastBeg, fastNb, slowBeg, slowNb;
   const char *name;
   double want, slowMA;
   int p, st, which, i, k, fast, slow, offset, swap, nbCmp, slowLookback, zeroRun;

   /* The corpus has to make a running sum ROUND, or this leg proves nothing:
    * 100.0 + 50.0*lcg_sym() is an integer over 2^23, every partial sum of it is
    * exact, and then every accumulation order agrees and a fused sum that
    * reached its first output bar by the wrong route still matches bit for bit.
    * So: a level far above the increments, a stretch whose exponents are
    * decades apart, a run of exact zeros long enough to kill a slow window, and
    * a flat stretch.
    */
   ta_test_ref_lcg_seed( 0x5090u );
   for( i = 0; i < PO_FUSE_N; i++ )
      in[i] = 1.0e8 + 1.0e-4 * ta_test_ref_lcg_sym();
   for( i = 200; i < 260; i++ )
      in[i] = ldexp( 1.0 + 0.5 * ta_test_ref_lcg_half(), (i % 41) - 20 );
   for( i = 120; i < 160; i++ )
      in[i] = 0.0;
   for( i = 300; i < 340; i++ )
      in[i] = 12.5;

   nbCmp = 0;
   for( p = 0; p < (int)(sizeof(pairs)/sizeof(pairs[0])); p++ )
   for( st = 0; st < (int)(sizeof(starts)/sizeof(starts[0])); st++ )
   for( which = 0; which < 3; which++ )
   {
      fast = pairs[p].fast;
      slow = pairs[p].slow;

      switch( which )
      {
      case 0:
         name = "APO";
         rc = TA_APO( starts[st], PO_FUSE_N-1, in, fast, slow, TA_MAType_SMA, &beg, &nb, got );
         break;
      case 1:
         name = "PPO";
         rc = TA_PPO( starts[st], PO_FUSE_N-1, in, fast, slow, TA_MAType_SMA, &beg, &nb, got );
         break;
      default:
         name = "PVO";
         rc = TA_PVO( starts[st], PO_FUSE_N-1, in, fast, slow, TA_MAType_SMA, &beg, &nb, got );
         break;
      }
      if( rc != TA_SUCCESS )
      {
         printf( "%s fusion Fail [f=%d s=%d start=%d]: rc=%d\n",
                 name, fast, slow, starts[st], (int)rc );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      /* The oracle: exactly what the general MA path computes. */
      if( slow < fast )
      {
         swap = slow; slow = fast; fast = swap;
      }
      rc = TA_SMA( starts[st], PO_FUSE_N-1, in, fast, &fastBeg, &fastNb, fastBuf );
      if( rc == TA_SUCCESS )
         rc = TA_SMA( starts[st], PO_FUSE_N-1, in, slow, &slowBeg, &slowNb, slowBuf );
      if( rc != TA_SUCCESS )
      {
         printf( "%s fusion Fail [f=%d s=%d start=%d]: oracle rc=%d\n",
                 name, fast, slow, starts[st], (int)rc );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( beg != slowBeg || nb != slowNb )
      {
         printf( "%s fusion Fail [f=%d s=%d start=%d]: range (%d,%d), expected (%d,%d)\n",
                 name, fast, slow, starts[st], (int)beg, (int)nb, (int)slowBeg, (int)slowNb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      offset = fastNb - slowNb;
      slowLookback = slow - 1;
      zeroRun = 0;
      for( k = slowBeg - slowLookback; k < slowBeg; k++ )
         zeroRun = fabs(in[k]) <= 0.0 ? zeroRun + 1 : 0;

      for( i = 0; i < (int)slowNb; i++ )
      {
         zeroRun = fabs(in[slowBeg + i]) <= 0.0 ? zeroRun + 1 : 0;
         slowMA = slowBuf[i];
         if( which == 0 )
            want = fastBuf[i+offset] - slowMA;
         else if( zeroRun > slowLookback )
         {
            zeroRun = slowLookback;
            want = 0.0;
         }
         else if( !TA_IS_ZERO(slowMA) )
            want = ((fastBuf[i+offset]-slowMA)/slowMA)*100.0;
         else
            want = 0.0;

         nbCmp++;
         if( got[i] != want )
         {
            printf( "%s fusion Fail [f=%d s=%d start=%d i=%d]: %.17g, expected %.17g\n",
                    name, fast, slow, starts[st], i, got[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* Literal: a leg that compared nothing prints nothing either. */
   if( nbCmp != PO_FUSE_CMP )
   {
      printf( "SMA fusion Fail: compared %d times, not the %d this file was written with\n",
              nbCmp, PO_FUSE_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}
