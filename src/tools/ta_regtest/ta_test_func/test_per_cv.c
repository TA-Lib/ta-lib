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
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  071726 MF,CC  First version. Close+volume indicators (NVI/PVI, #126).
 */

/* Description:
 *
 *     Test functions whose inputs are (close, volume) with one real output
 *     and no optional parameter: NVI and PVI (issue #126).
 *
 *     Both are cumulative volume indices seeded at 1000 and updated by the
 *     bar's percentage price change only when volume fell (NVI) / rose (PVI)
 *     versus the previous bar. Like OBV/AD, their absolute level is anchored at
 *     startIdx and is therefore path-dependent across ranges -> the range sweep
 *     uses TA_DO_NOT_COMPARE.
 *
 *     FORMULA-CORRECTNESS is anchored by the hard-coded expected values below.
 *     These are golden numbers produced by an INDEPENDENT implementation:
 *     Tulip Indicators 0.9.2 (ti_nvi / ti_pvi), which were additionally verified
 *     to agree bit-for-bit with a from-scratch pure-Python textbook reference on
 *     this same 252-bar series. This proves the formula is right; the generic
 *     cross-language gate (--codegen / --xlang-hash) separately proves every
 *     backend reproduces it bit-for-bit.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

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
typedef enum {
TA_NVI_TEST,
TA_PVI_TEST,
} TA_TestId;

typedef struct
{
   TA_Integer doRangeTestFlag;

   TA_TestId  theFunction;

   TA_Integer startIdx;
   TA_Integer endIdx;

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
   const TA_Real *volume;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

/**** Local variables definitions.     ****/

/* Golden values: Tulip Indicators 0.9.2 (ti_nvi / ti_pvi), confirmed bit-for-bit
 * against an independent pure-Python textbook reference on the standard 252-bar
 * close/volume series. Spot indices sample the seed (0), an early carry-forward
 * bar, and points across the full history including the last bar. */
static TA_Test tableTest[] =
{
   /*************/
   /* NVI TEST  */
   /*************/
   { 1, TA_NVI_TEST, 0, 251, TA_SUCCESS,   0, 1000.0,              0, 252 }, /* First Value (seed) */
   { 0, TA_NVI_TEST, 0, 251, TA_SUCCESS,   2,  995.359384063703,  0, 252 },
   { 0, TA_NVI_TEST, 0, 251, TA_SUCCESS,  50,  937.3246329630261, 0, 252 },
   { 0, TA_NVI_TEST, 0, 251, TA_SUCCESS, 125, 1038.1401959467728, 0, 252 },
   { 0, TA_NVI_TEST, 0, 251, TA_SUCCESS, 200, 1173.0691300345422, 0, 252 },
   { 0, TA_NVI_TEST, 0, 251, TA_SUCCESS, 251, 1248.340285103247,  0, 252 }, /* Last Value */

   /*************/
   /* PVI TEST  */
   /*************/
   { 1, TA_PVI_TEST, 0, 251, TA_SUCCESS,   0, 1000.0,              0, 252 }, /* First Value (seed) */
   { 0, TA_PVI_TEST, 0, 251, TA_SUCCESS,   1, 1036.2295081967213, 0, 252 },
   { 0, TA_PVI_TEST, 0, 251, TA_SUCCESS,  50, 1038.0666743015763, 0, 252 },
   { 0, TA_PVI_TEST, 0, 251, TA_SUCCESS, 125, 1392.254337997847,  0, 252 },
   { 0, TA_PVI_TEST, 0, 251, TA_SUCCESS, 200,  997.9892510505222, 0, 252 },
   { 0, TA_PVI_TEST, 0, 251, TA_SUCCESS, 251,  944.3796037773723, 0, 252 }, /* Last Value */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_per_cv( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   for( i=0; i < NB_TEST; i++ )
   {
      /* Re-initialize all the unstable period to zero. */
      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "Failed Bad Parameter for Test #%d (%d,%d)\n",
                 i, tableTest[i].expectedNbElement, history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "Failed Test #%d (Code=%d)\n", i, retValue );
         return retValue;
      }
   }

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* All test succeed. */
   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/
static TA_RetCode callFunc( TA_TestId    theFunction,
                            TA_Integer   startIdx,
                            TA_Integer   endIdx,
                            const TA_Real inClose[],
                            const TA_Real inVolume[],
                            TA_Integer  *outBegIdx,
                            TA_Integer  *outNbElement,
                            TA_Real     *outReal )
{
   switch( theFunction )
   {
   case TA_NVI_TEST:
      return TA_NVI( startIdx, endIdx, inClose, inVolume,
                     outBegIdx, outNbElement, outReal );
   case TA_PVI_TEST:
      return TA_PVI( startIdx, endIdx, inClose, inVolume,
                     outBegIdx, outNbElement, outReal );
   default:
      return TA_INTERNAL_ERROR(180);
   }
}

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

   retCode = callFunc( testParam->test->theFunction,
                       startIdx, endIdx,
                       testParam->close, testParam->volume,
                       outBegIdx, outNbElement, outputBuffer );

   switch( testParam->test->theFunction )
   {
   case TA_NVI_TEST: *lookback = TA_NVI_Lookback(); break;
   case TA_PVI_TEST: *lookback = TA_PVI_Lookback(); break;
   default:          *lookback = 0;                 break;
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
   const char *funcName;

   /* Set to NAN all the elements of the gBuffers. */
   clearAllBuffers();

   /* Build the input: [0]=close, [1]=volume. */
   setInputBuffer( 0, history->close,  history->nbBars );
   setInputBuffer( 1, history->volume, history->nbBars );

   /* Make a simple first call. */
   retCode = callFunc( test->theFunction,
                       test->startIdx, test->endIdx,
                       gBuffer[0].in, gBuffer[1].in,
                       &outBegIdx, &outNbElement, gBuffer[0].out0 );

   /* Check that the inputs were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->close, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->volume, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   if( server_verify_active() )
   {
      funcName = (test->theFunction == TA_NVI_TEST) ? "NVI" : "PVI";
      errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                            retCode, outBegIdx, outNbElement,
                            (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, NULL },
                            NULL, 0,
                            (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   /* Make another call where the output overwrites the close input buffer. */
   retCode = callFunc( test->theFunction,
                       test->startIdx, test->endIdx,
                       gBuffer[0].in, gBuffer[1].in,
                       &outBegIdx, &outNbElement, gBuffer[0].in );

   /* Volume input must be preserved. */
   errNb = checkDataSame( gBuffer[1].in, history->volume, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call should have produced the same output. */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[0].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].in, 0 );
   setInputBuffer( 0, history->close, history->nbBars );

   /* Make another call where the output overwrites the volume input buffer. */
   retCode = callFunc( test->theFunction,
                       test->startIdx, test->endIdx,
                       gBuffer[0].in, gBuffer[1].in,
                       &outBegIdx, &outNbElement, gBuffer[1].in );

   /* Close input must be preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->close, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call should have produced the same output. */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[1].in, 0 );
   setInputBuffer( 1, history->volume, history->nbBars );

   if( test->doRangeTestFlag )
   {
      /* Systematic startIdx/endIdx sweep. NVI/PVI are cumulative indices anchored
       * at startIdx (like OBV/AD), so their absolute level is legitimately
       * range-dependent -> compare coherency only (TA_DO_NOT_COMPARE). */
      testParam.test   = test;
      testParam.close  = history->close;
      testParam.volume = history->volume;

      errNb = doRangeTest( rangeTestFunction,
                           TA_FUNC_UNST_NONE,
                           (void *)&testParam, 1,
                           TA_DO_NOT_COMPARE );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}
