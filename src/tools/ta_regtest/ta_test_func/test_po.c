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
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   First version.
 *  020605 MF   Add regression test with inverted slow/fast period.
 *  020805 AA   Fix one of the TA_PPO call (wrong buffer was pass).
 */

/* Description:
 *     Regression test of APO(Absolute Price Oscillator).
 *     Regression test of PPO (Percentage Price Oscillator).
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
   TA_Integer doRangeTestFlag;

   TA_Integer doPercentage;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer optInFastPeriod; /* From 1 to 200 */
   TA_Integer optInSlowPeriod; /* From 1 to 200 */
   TA_Integer optInMethod_2;
   TA_Integer compatibility;

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

/**** Local variables definitions.     ****/
static TA_Test tableTest[] =
{
   /**********************************/
   /*    APO TEST - SIMPLE - CLASSIC */
   /**********************************/
   { 1, 0, 0, 251, 26, 12, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,      0, -3.3124, 25,  252-25 }, /* First Value */
   { 1, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,      0, -3.3124, 25,  252-25 }, /* First Value */
   { 0, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,      1, -3.5876, 25,  252-25 },
   { 0, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 252-26, -0.1667, 25,  252-25 }, /* Last Value */

   { 0, 0, 0,   1, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 1,   1, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 25,  25, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  -3.3124,   25,  1 }, /* First/Last Value */
   { 0, 0, 250, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,  -0.1667,  250,  2 }, /* Last  Value */

   /************************************/
   /*    APO TEST - SIMPLE - METASTOCK */
   /************************************/
   { 0, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,      0, -3.3124, 25,  252-25 }, /* First Value */
   { 0, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,      1, -3.5876, 25,  252-25 },
   { 0, 0, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 252-26, -0.1667, 25,  252-25 }, /* Last Value */

   { 0, 0, 0,   1, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 1,   1, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 25,  25, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  -3.3124,   25,  1 }, /* First/Last Value */
   { 0, 0, 250, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  -0.1667,  250,  2 }, /* Last  Value */


   /***************************************/
   /*    APO TEST - EXPONENTIAL - CLASSIC */
   /***************************************/
   /* !!! To be done. */

   /*****************************************/
   /*    APO TEST - EXPONENTIAL - METASTOCK */
   /*****************************************/
   { 1, 0, 0, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,      0, -2.4193, 25,  252-25 }, /* First Value */
   { 0, 0, 0, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,      1, -2.4367, 25,  252-25 },
   { 0, 0, 0, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 252-26, 0.90401, 25,  252-25 }, /* Last Value */

   { 0, 0, 0,   1, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 1,   1, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 0, 25,  25, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  -2.4193,   25,  1 },
   { 0, 0, 250, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  0.90401,  250,  2 }, /* Last  Value */

   { 0, 0, 251, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  0.90401,  251,  1 },  /* Last  Value */
   { 0, 0, 25,  25, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  -2.4193,   25,  1 },  /* Just enough to calculate first. */
   { 0, 0, 26,  26, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  -2.4367,   26,  1 },  /* Just enough to calculate second. */

   /**********************************/
   /*    PPO TEST - SIMPLE - CLASSIC */
   /**********************************/
   { 1, 1, 0, 251, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  1.10264, 2,  252-2 }, /* First Value */
   { 0, 1, 0, 251, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1, -0.02813, 2,  252-2 },
   { 0, 1, 0, 251, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 249, -0.21191, 2,  252-2 }, /* Last Value */

   { 0, 1, 0,   1, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 0, 1, 1,   1, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 0, 1, 2,   2, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  1.10264,   2,  1 }, /* First/Last Value */
   { 0, 1, 250, 251, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1, -0.21191, 250,  2 }, /* Last  Value */

   /************************************/
   /*    PPO TEST - SIMPLE - METASTOCK */
   /************************************/
   { 0, 1, 0, 251, 3, 2, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  1.10264, 2,  252-2 }, /* First Value */
   { 0, 1, 0, 251, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1, -0.02813, 2,  252-2 },
   { 0, 1, 0, 251, 3, 2, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 249, -0.21191, 2,  252-2 }, /* Last Value */

   { 0, 1, 0,   1, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 1, 1, 1,   1, 3, 2, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 1, 1, 2,   2, 2, 3, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  1.10264,   2,  1 }, /* First/Last Value */
   { 0, 1, 250, 251, 3, 2, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1, -0.21191, 250,  2 }, /* Last  Value */

   { 0, 1, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,      0, -3.6393, 25,  252-25 }, /* First Value */
   { 0, 1, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,      1, -3.9534, 25,  252-25 },
   { 0, 1, 0, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 252-26, -0.15281, 25,  252-25 }, /* Last Value */

   { 0, 1, 0,   1, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 0, 1, 1,   1, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,   0,  0 }, /* Out of range value */
   { 0, 1, 25,  25, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0, -3.6393,   25,  1 }, /* First/Last Value */
   { 0, 1, 250, 251, 12, 26, TA_MAType_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1, -0.15281, 250,  2 }, /* Last  Value */

   /***************************************/
   /*    PPO TEST - EXPONENTIAL - CLASSIC */
   /***************************************/
   /* !!! To be done. */

   /*****************************************/
   /*    PPO TEST - EXPONENTIAL - METASTOCK */
   /*****************************************/
   { 1, 1, 0, 251, 26, 12, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,      0, -2.7083, 25,  252-25 }, /* First Value */
   { 0, 1, 0, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,      1, -2.7390, 25,  252-25 },
   { 0, 1, 0, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 252-26, 0.83644, 25,  252-25 }, /* Last Value */

   { 0, 1, 0,   1, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 1, 1,   1, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,        0,    0,  0 }, /* Out of range value */
   { 0, 1, 25,  25, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,   -2.7083,   25,  1 },
   { 0, 1, 250, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,   0.83644,  250,  2 }, /* Last  Value */

   { 0, 1, 251, 251, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  0.83644,  251,  1 },  /* Last  Value */
   { 0, 1, 25,  25, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  -2.7083,   25,  1 },  /* Just enough to calculate first. */
   { 0, 1, 26,  26, 12, 26, TA_MAType_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  -2.7390,   26,  1 },  /* Just enough to calculate second. */
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

   TA_SetCompatibility( (TA_Compatibility)test->compatibility );

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
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
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
                              TA_FUNC_UNST_NONE,
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
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );
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

