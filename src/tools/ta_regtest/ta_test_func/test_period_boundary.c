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
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  070226 MF,CC  First version. Period=1 / minimum-period boundary
 *                cases for GitHub issues #48 and #59 (SourceForge
 *                bug 84).
 *  070726 MF,CC  Widen the abstract sweep to the full parameter grid
 *                (all parameter types, min..max+1) with a coherence /
 *                clean-BAD_PARAM / finite-output contract (#94).
 */

/* Description:
 *
 * Boundary tests for the smallest allowed period values, with an
 * emphasis on period=1 ("no smoothing"):
 *
 *  - Lookback contract: TA_INTEGER_DEFAULT maps to the default
 *    period, out-of-range params return -1, and period=1 lookbacks
 *    are coherent (the SF bug-84 case: TA_MACD_Lookback(2,7,1)==6).
 *  - Identity: SMA/EMA/WMA/DEMA/TEMA/TRIMA/KAMA/T3/MAVP and
 *    MA(period=1, every MAType) must return the input unchanged.
 *  - MACD family with signalPeriod=1: the signal line equals the
 *    MACD line, the histogram is zero, and the output is aligned
 *    and complete (the #59 "repaint" regression pins).
 *  - Behavior pins at period=1 for functions that always allowed it
 *    (ATR, NATR, TRIX, ULTOSC, DI/DM, MOM/ROC*, VAR, BETA, CORREL,
 *    STOCH family). NATR(1) and +DI/-DI(1) intentionally pin the
 *    historical scaling quirks (NATR(1)==TRANGE without
 *    normalization; DI(1)==DM/TR without the x100 of DI(n>=2)) so
 *    any future semantic change is a deliberate test edit.
 *  - An abstract-driven parameter-boundary sweep (#94): every optional
 *    parameter of every function is exercised across its whole range —
 *    integer ranges at min / min+1 / default+-1 / a large "past the
 *    data" period plus the out-of-range min-1 and max+1; integer/real
 *    lists at every enumerated value; real ranges at min / default /
 *    max. Each in-range value must be coherent (outBegIdx >= lookback,
 *    coverage to the last bar, or an empty result) or a clean
 *    TA_BAD_PARAM; the default must always compute; out-of-range
 *    integers must be rejected; and every output is scanned for
 *    NaN/Inf/subnormal garbage. Reads the metadata, so it adapts
 *    automatically. Set PB_SWEEP_LIST_ALL=1 to enumerate every failing
 *    case in one run instead of aborting on the first.
 *
 * When --codegen is active every successful hand-written call is
 * also verified against the language servers (C, Rust, Java, .NET)
 * through server_verify.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

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
#define PB_DATA_SIZE 252   /* Daily reference data size. */

/* Buffers for the abstract-driven sweep (max 3 outputs per function). */
#define PB_MAX_OUTPUT 3

/* An integer max above this is treated as effectively unbounded: we do not
 * probe max+1 (it would overflow a value no caller ever passes) and leave the
 * true-max / integer-overflow surface to the ASan/UBSan nightly job. */
#define PB_SWEEP_MAX_BOUNDED 1000000

/* Per-case expectation for the sweep. #94's memory-safety contract is
 * "succeed-coherent OR clean TA_BAD_PARAM — never a crash/garbage". */
#define PB_EXPECT_REJECT  0   /* out of range: must return TA_BAD_PARAM       */
#define PB_EXPECT_STRICT  1   /* realistic value: must succeed, coherent      */
#define PB_EXPECT_LENIENT 2   /* extreme value: coherent OR clean TA_BAD_PARAM
                               * (an in-range value can still be rejected when
                               *  it violates a cross-parameter constraint,
                               *  e.g. MAVP minPeriod > maxPeriod).            */
static TA_Real    pbSweepOutReal[PB_MAX_OUTPUT][PB_DATA_SIZE];
static TA_Integer pbSweepOutInt[PB_MAX_OUTPUT][PB_DATA_SIZE];

typedef struct
{
   const TA_History *history;
   ErrorNumber errNb;
   int nbParamTested;
   int nbFail;
} PBSweepCtx;

/* Diagnostic: when the environment variable PB_SWEEP_LIST_ALL is set, the
 * sweep reports every failing case (and keeps going) instead of aborting on
 * the first one. Leaves the pass/fail verdict unchanged for CI. */
static int g_pbListAll = 0;

/* Record a sweep failure. In normal mode this aborts the sweep (sets errNb);
 * in list-all mode it only counts, so a single run enumerates every case. */
static void pbFail( PBSweepCtx *ctx )
{
   ctx->nbFail++;
   if( !g_pbListAll )
      ctx->errNb = (ErrorNumber)( TA_REGTEST_OPTIMIZATION_REF_ERROR );
}

typedef struct
{
   int useKama;         /* 1: KAMA range test, else EMA. */
   const TA_Real *in;
} PBIdentityRangeParam;

typedef struct
{
   const TA_Real *in;
   TA_Integer fast;
   TA_Integer slow;
   TA_Integer signal;
} PBMacdRangeParam;

/**** Local functions declarations.    ****/
static ErrorNumber testLookbackContract( void );
static ErrorNumber testIdentityAtPeriodOne( const TA_History *history );
static ErrorNumber testMacdFamilySignalOne( const TA_History *history );
static ErrorNumber testPeriodOnePins( const TA_History *history );
static ErrorNumber testMinBoundarySweep( const TA_History *history );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
ErrorNumber test_func_period_boundary( TA_History *history )
{
   ErrorNumber errNb;

   /* These tests assume pristine global settings. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   errNb = testLookbackContract();
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = testIdentityAtPeriodOne( history );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = testMacdFamilySignalOne( history );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = testPeriodOnePins( history );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = testMinBoundarySweep( history );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* Leave globals as found. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* Strict integer pin with diagnostic output. */
#define PB_CHECK_INT( label, actual, expected ) \
   { \
      if( (int)(actual) != (int)(expected) ) \
      { \
         printf( "\nFail: %s: got %d, expected %d\n", label, (int)(actual), (int)(expected) ); \
         return TA_REGTEST_OPTIMIZATION_REF_ERROR; \
      } \
   }

/* Strict TA_RetCode pin. */
#define PB_CHECK_RC( label, actual, expected ) \
   { \
      if( (actual) != (expected) ) \
      { \
         printf( "\nFail: %s: retCode %d, expected %d\n", label, (int)(actual), (int)(expected) ); \
         return TA_REGTEST_OPTIMIZATION_REF_ERROR; \
      } \
   }

/* Compare two output series bit-exactly (period=1 semantics are exact
 * copies/differences, so no tolerance is appropriate here).
 */
static ErrorNumber pbCheckSameSeries( const char *label,
                                      const TA_Real *actual,
                                      const TA_Real *expected,
                                      int nbElement )
{
   int i;
   for( i = 0; i < nbElement; i++ )
   {
      if( !(actual[i] == expected[i]) )
      {
         printf( "\nFail: %s: [%d] got %.17g, expected %.17g\n",
                 label, i, actual[i], expected[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }
   return TA_TEST_PASS;
}

/* Check a completed call: retCode/outBegIdx/coverage-to-endIdx. */
static ErrorNumber pbCheckCallShape( const char *label,
                                     TA_RetCode retCode,
                                     TA_Integer outBegIdx, TA_Integer expectedBegIdx,
                                     TA_Integer outNbElement, TA_Integer endIdx )
{
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFail: %s: retCode %d\n", label, (int)retCode );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   if( outBegIdx != expectedBegIdx )
   {
      printf( "\nFail: %s: outBegIdx %d, expected %d\n", label, outBegIdx, expectedBegIdx );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   if( outBegIdx + outNbElement - 1 != endIdx )
   {
      printf( "\nFail: %s: last covered bar %d, expected %d (missing tail)\n",
              label, outBegIdx + outNbElement - 1, endIdx );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   return TA_TEST_PASS;
}

/*******************************/
/* Sub-test: lookback contract */
/*******************************/
static ErrorNumber testLookbackContract( void )
{
   int i;
   TA_RetCode retCode;

   /* Period=1 is valid: lookback is 0 plus the unstable-period term
    * where one applies (all unstable periods are 0 here).
    */
   PB_CHECK_INT( "TA_SMA_Lookback(1)",   TA_SMA_Lookback( 1 ),   0 );
   PB_CHECK_INT( "TA_EMA_Lookback(1)",   TA_EMA_Lookback( 1 ),   0 );
   PB_CHECK_INT( "TA_WMA_Lookback(1)",   TA_WMA_Lookback( 1 ),   0 );
   PB_CHECK_INT( "TA_DEMA_Lookback(1)",  TA_DEMA_Lookback( 1 ),  0 );
   PB_CHECK_INT( "TA_TEMA_Lookback(1)",  TA_TEMA_Lookback( 1 ),  0 );
   PB_CHECK_INT( "TA_TRIMA_Lookback(1)", TA_TRIMA_Lookback( 1 ), 0 );
   PB_CHECK_INT( "TA_KAMA_Lookback(1)",  TA_KAMA_Lookback( 1 ),  0 );
   PB_CHECK_INT( "TA_T3_Lookback(1)",    TA_T3_Lookback( 1, 0.7 ), 0 );
   PB_CHECK_INT( "TA_EMA_Lookback(2)",   TA_EMA_Lookback( 2 ),   1 );

   /* The SourceForge bug-84 report, verbatim. */
   PB_CHECK_INT( "TA_MACD_Lookback(2,7,1)", TA_MACD_Lookback( 2, 7, 1 ), 6 );
   PB_CHECK_INT( "TA_MACD_Lookback(2,7,2)", TA_MACD_Lookback( 2, 7, 2 ), 7 );
   PB_CHECK_INT( "TA_MACD_Lookback(12,26,1)", TA_MACD_Lookback( 12, 26, 1 ), 25 );
   PB_CHECK_INT( "TA_MACDFIX_Lookback(1)", TA_MACDFIX_Lookback( 1 ), 25 );

   PB_CHECK_INT( "TA_TRIX_Lookback(1)", TA_TRIX_Lookback( 1 ), 1 );
   PB_CHECK_INT( "TA_ULTOSC_Lookback(1,1,1)", TA_ULTOSC_Lookback( 1, 1, 1 ), 1 );

   for( i = 0; i <= (int)TA_MAType_T3; i++ )
   {
      PB_CHECK_INT( "TA_MA_Lookback(1,maType)", TA_MA_Lookback( 1, (TA_MAType)i ), 0 );
   }
   PB_CHECK_INT( "TA_MAVP_Lookback(1,2,SMA)", TA_MAVP_Lookback( 1, 2, TA_MAType_SMA ), 1 );

   /* TA_INTEGER_DEFAULT maps to the documented default period. */
   PB_CHECK_INT( "TA_SMA_Lookback(TA_INTEGER_DEFAULT)",
                 TA_SMA_Lookback( TA_INTEGER_DEFAULT ), 29 );
   PB_CHECK_INT( "TA_EMA_Lookback(TA_INTEGER_DEFAULT)",
                 TA_EMA_Lookback( TA_INTEGER_DEFAULT ), 29 );

   /* Out-of-range params return -1 (the classic contract). */
   PB_CHECK_INT( "TA_SMA_Lookback(0)",  TA_SMA_Lookback( 0 ),  -1 );
   PB_CHECK_INT( "TA_EMA_Lookback(0)",  TA_EMA_Lookback( 0 ),  -1 );
   PB_CHECK_INT( "TA_DEMA_Lookback(0)", TA_DEMA_Lookback( 0 ), -1 );
   PB_CHECK_INT( "TA_T3_Lookback(0)",   TA_T3_Lookback( 0, 0.7 ), -1 );
   PB_CHECK_INT( "TA_KAMA_Lookback(0)", TA_KAMA_Lookback( 0 ), -1 );
   PB_CHECK_INT( "TA_EMA_Lookback(100001)", TA_EMA_Lookback( 100001 ), -1 );
   PB_CHECK_INT( "TA_MACD_Lookback(2,7,0)", TA_MACD_Lookback( 2, 7, 0 ), -1 );

   /* The abstract layer must agree with the direct lookback. */
   {
      const TA_FuncHandle *handle;
      TA_ParamHolder *paramHolder;
      int lookback = -42;

      retCode = TA_GetFuncHandle( "MACD", &handle );
      PB_CHECK_RC( "TA_GetFuncHandle(MACD)", retCode, TA_SUCCESS );
      retCode = TA_ParamHolderAlloc( handle, &paramHolder );
      PB_CHECK_RC( "TA_ParamHolderAlloc(MACD)", retCode, TA_SUCCESS );
      TA_SetOptInputParamInteger( paramHolder, 0, 12 );
      TA_SetOptInputParamInteger( paramHolder, 1, 26 );
      TA_SetOptInputParamInteger( paramHolder, 2, 1 );
      retCode = TA_GetLookback( paramHolder, &lookback );
      TA_ParamHolderFree( paramHolder );
      PB_CHECK_RC( "TA_GetLookback(MACD sig=1)", retCode, TA_SUCCESS );
      PB_CHECK_INT( "TA_GetLookback(MACD sig=1)", lookback, TA_MACD_Lookback( 12, 26, 1 ) );
   }

   return TA_TEST_PASS;
}

/*****************************************/
/* Sub-test: identity at period=1        */
/*****************************************/

/* Range-test callback shared by the EMA(1)/KAMA(1) doRangeTest calls. */
static TA_RetCode pbIdentityRangeFunction( TA_Integer    startIdx,
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
   PBIdentityRangeParam *testParam;

   (void)outputNb;
   (void)outputBufferInt;

   *isOutputInteger = 0;

   testParam = (PBIdentityRangeParam *)opaqueData;

   if( testParam->useKama )
   {
      retCode = TA_KAMA( startIdx, endIdx, testParam->in, 1,
                         outBegIdx, outNbElement, outputBuffer );
      *lookback = TA_KAMA_Lookback( 1 );
   }
   else
   {
      retCode = TA_EMA( startIdx, endIdx, testParam->in, 1,
                        outBegIdx, outNbElement, outputBuffer );
      *lookback = TA_EMA_Lookback( 1 );
   }

   return retCode;
}

/* Call one single-input function at period=1 and require exact identity. */
static ErrorNumber pbCheckIdentityCall( const char *funcName,
                                        const TA_History *history,
                                        TA_RetCode retCode,
                                        TA_Integer outBegIdx,
                                        TA_Integer outNbElement,
                                        TA_Integer expectedBegIdx,
                                        const TA_Real *out,
                                        const double optParams[],
                                        int nbOptParams )
{
   ErrorNumber errNb;
   char label[128];

   snprintf( label, sizeof(label), "%s period=1 identity", funcName );

   errNb = pbCheckCallShape( label, retCode, outBegIdx, expectedBegIdx,
                             outNbElement, (TA_Integer)(history->nbBars - 1) );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = pbCheckSameSeries( label, out, &history->close[outBegIdx], outNbElement );
   if( errNb != TA_TEST_PASS )
      return errNb;

   if( server_verify_active() )
   {
      errNb = server_verify( funcName, 0, history->nbBars - 1, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ history->close, NULL },
                             optParams, nbOptParams,
                             (const TA_Real*[]){ out, NULL }, NULL );
      if( errNb != TA_TEST_PASS )
      {
         printf( "Fail: %s: server verification\n", label );
         return errNb;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber testIdentityAtPeriodOne( const TA_History *history )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx, outNbElement;
   TA_Integer endIdx = (TA_Integer)(history->nbBars - 1);
   int i;

   clearAllBuffers();
   setInputBuffer( 0, history->close, history->nbBars );

   /* The 8 direct moving averages. */
   retCode = TA_SMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckIdentityCall( "SMA", history, retCode, outBegIdx, outNbElement, 0,
                                gBuffer[0].out0, (const double[]){ 1 }, 1 );
   if( errNb != TA_TEST_PASS ) return errNb;

   retCode = TA_EMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckIdentityCall( "EMA", history, retCode, outBegIdx, outNbElement, 0,
                                gBuffer[0].out0, (const double[]){ 1 }, 1 );
   if( errNb != TA_TEST_PASS ) return errNb;

   retCode = TA_WMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckIdentityCall( "WMA", history, retCode, outBegIdx, outNbElement, 0,
                                gBuffer[0].out0, (const double[]){ 1 }, 1 );
   if( errNb != TA_TEST_PASS ) return errNb;

   retCode = TA_DEMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckIdentityCall( "DEMA", history, retCode, outBegIdx, outNbElement, 0,
                                gBuffer[0].out0, (const double[]){ 1 }, 1 );
   if( errNb != TA_TEST_PASS ) return errNb;

   retCode = TA_TEMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckIdentityCall( "TEMA", history, retCode, outBegIdx, outNbElement, 0,
                                gBuffer[0].out0, (const double[]){ 1 }, 1 );
   if( errNb != TA_TEST_PASS ) return errNb;

   retCode = TA_TRIMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckIdentityCall( "TRIMA", history, retCode, outBegIdx, outNbElement, 0,
                                gBuffer[0].out0, (const double[]){ 1 }, 1 );
   if( errNb != TA_TEST_PASS ) return errNb;

   retCode = TA_KAMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckIdentityCall( "KAMA", history, retCode, outBegIdx, outNbElement, 0,
                                gBuffer[0].out0, (const double[]){ 1 }, 1 );
   if( errNb != TA_TEST_PASS ) return errNb;

   retCode = TA_T3( 0, endIdx, gBuffer[0].in, 1, 0.7, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckIdentityCall( "T3", history, retCode, outBegIdx, outNbElement, 0,
                                gBuffer[0].out0, (const double[]){ 1, 0.7 }, 2 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* MA(period=1) for every MAType: the documented "just copy" path. */
   for( i = 0; i <= (int)TA_MAType_T3; i++ )
   {
      char label[64];
      snprintf( label, sizeof(label), "MA(1,maType=%d)", i );
      retCode = TA_MA( 0, endIdx, gBuffer[0].in, 1, (TA_MAType)i,
                       &outBegIdx, &outNbElement, gBuffer[0].out0 );
      errNb = pbCheckCallShape( label, retCode, outBegIdx, 0, outNbElement, endIdx );
      if( errNb != TA_TEST_PASS ) return errNb;
      errNb = pbCheckSameSeries( label, gBuffer[0].out0, history->close, outNbElement );
      if( errNb != TA_TEST_PASS ) return errNb;

      if( server_verify_active() )
      {
         errNb = server_verify( "MA", 0, endIdx, history->nbBars,
                                retCode, outBegIdx, outNbElement,
                                (const TA_Real*[]){ gBuffer[0].in, NULL },
                                (const double[]){ 1, (double)i }, 2,
                                (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL );
         if( errNb != TA_TEST_PASS )
         {
            printf( "Fail: %s: server verification\n", label );
            return errNb;
         }
      }
   }

   /* MAVP with all variable periods = 1 (minPeriod=1). Lookback is
    * driven by maxPeriod, so output starts at bar 1.
    */
   {
      static TA_Real periodsOne[PB_DATA_SIZE];
      for( i = 0; i < (int)history->nbBars; i++ )
         periodsOne[i] = 1.0;

      retCode = TA_MAVP( 0, endIdx, gBuffer[0].in, periodsOne, 1, 2, TA_MAType_SMA,
                         &outBegIdx, &outNbElement, gBuffer[0].out0 );
      errNb = pbCheckCallShape( "MAVP(periods=1,min=1,max=2)", retCode, outBegIdx,
                                TA_MAVP_Lookback( 1, 2, TA_MAType_SMA ), outNbElement, endIdx );
      if( errNb != TA_TEST_PASS ) return errNb;
      errNb = pbCheckSameSeries( "MAVP(periods=1,min=1,max=2)", gBuffer[0].out0,
                                 &history->close[outBegIdx], outNbElement );
      if( errNb != TA_TEST_PASS ) return errNb;

      if( server_verify_active() )
      {
         errNb = server_verify( "MAVP", 0, endIdx, history->nbBars,
                                retCode, outBegIdx, outNbElement,
                                (const TA_Real*[]){ gBuffer[0].in, periodsOne, NULL },
                                (const double[]){ 1, 2, (double)TA_MAType_SMA }, 3,
                                (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL );
         if( errNb != TA_TEST_PASS )
         {
            printf( "Fail: MAVP(periods=1): server verification\n" );
            return errNb;
         }
      }
   }

   /* Identity holds under a non-zero unstable period: output starts
    * later but the values are still exact copies.
    */
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 3 );
   retCode = TA_EMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   errNb = pbCheckCallShape( "EMA(1) unstable=3", retCode, outBegIdx, 3, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   errNb = pbCheckSameSeries( "EMA(1) unstable=3", gBuffer[0].out0,
                              &history->close[outBegIdx], outNbElement );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Identity holds under Metastock compatibility (different seeding). */
   TA_SetCompatibility( TA_COMPATIBILITY_METASTOCK );
   retCode = TA_EMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );
   errNb = pbCheckCallShape( "EMA(1) metastock", retCode, outBegIdx, 0, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   errNb = pbCheckSameSeries( "EMA(1) metastock", gBuffer[0].out0,
                              history->close, outNbElement );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* In-place call: output over the input buffer must give the same result. */
   retCode = TA_EMA( 0, endIdx, gBuffer[0].in, 1, &outBegIdx, &outNbElement, gBuffer[0].in );
   errNb = pbCheckCallShape( "EMA(1) in-place", retCode, outBegIdx, 0, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   errNb = pbCheckSameSeries( "EMA(1) in-place", gBuffer[0].in, history->close, outNbElement );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Exhaustive startIdx/endIdx range sweep for EMA(1) and KAMA(1). */
   {
      PBIdentityRangeParam testParam;

      testParam.in = history->close;
      testParam.useKama = 0;
      errNb = doRangeTest( pbIdentityRangeFunction, TA_FUNC_UNST_EMA,
                           (void *)&testParam, 1, 0 );
      if( errNb != TA_TEST_PASS )
      {
         printf( "Fail: EMA(1) range test\n" );
         return errNb;
      }

      testParam.useKama = 1;
      errNb = doRangeTest( pbIdentityRangeFunction, TA_FUNC_UNST_KAMA,
                           (void *)&testParam, 1, 0 );
      if( errNb != TA_TEST_PASS )
      {
         printf( "Fail: KAMA(1) range test\n" );
         return errNb;
      }
   }

   /* doRangeTest varies the unstable period and leaves it set. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   return TA_TEST_PASS;
}

/*********************************************/
/* Sub-test: MACD family with signalPeriod=1 */
/*********************************************/

static TA_RetCode pbMacdRangeFunction( TA_Integer    startIdx,
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
   PBMacdRangeParam *testParam;
   TA_Real *dummyBuffer1, *dummyBuffer2;

   (void)outputBufferInt;

   *isOutputInteger = 0;

   testParam = (PBMacdRangeParam *)opaqueData;

   /* doRangeTest exercises one output at a time. */
   dummyBuffer1 = &gBuffer[3].out0[20];
   dummyBuffer2 = &gBuffer[3].out1[20];

   switch( outputNb )
   {
   case 0:
      retCode = TA_MACD( startIdx, endIdx, testParam->in,
                         testParam->fast, testParam->slow, testParam->signal,
                         outBegIdx, outNbElement,
                         outputBuffer, dummyBuffer1, dummyBuffer2 );
      break;
   case 1:
      retCode = TA_MACD( startIdx, endIdx, testParam->in,
                         testParam->fast, testParam->slow, testParam->signal,
                         outBegIdx, outNbElement,
                         dummyBuffer1, outputBuffer, dummyBuffer2 );
      break;
   default:
      retCode = TA_MACD( startIdx, endIdx, testParam->in,
                         testParam->fast, testParam->slow, testParam->signal,
                         outBegIdx, outNbElement,
                         dummyBuffer1, dummyBuffer2, outputBuffer );
      break;
   }

   *lookback = TA_MACD_Lookback( testParam->fast, testParam->slow, testParam->signal );

   return retCode;
}

/* Shared checks for one MACD-family call with signalPeriod=1:
 * shape, signal==macd (exact), hist==0 (exact), server verification.
 */
static ErrorNumber pbCheckMacdSignalOne( const char *label,
                                         const char *funcName,
                                         const TA_History *history,
                                         TA_RetCode retCode,
                                         TA_Integer outBegIdx,
                                         TA_Integer outNbElement,
                                         TA_Integer expectedBegIdx,
                                         const double optParams[],
                                         int nbOptParams )
{
   ErrorNumber errNb;
   int i;

   errNb = pbCheckCallShape( label, retCode, outBegIdx, expectedBegIdx,
                             outNbElement, (TA_Integer)(history->nbBars - 1) );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = pbCheckSameSeries( label, gBuffer[0].out1, gBuffer[0].out0, outNbElement );
   if( errNb != TA_TEST_PASS )
   {
      printf( "  (signal line != MACD line at signalPeriod=1)\n" );
      return errNb;
   }

   for( i = 0; i < outNbElement; i++ )
   {
      if( gBuffer[0].out2[i] != 0.0 )
      {
         printf( "\nFail: %s: hist[%d]=%.17g, expected 0\n", label, i, gBuffer[0].out2[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   if( server_verify_active() )
   {
      errNb = server_verify( funcName, 0, history->nbBars - 1, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ gBuffer[0].in, NULL },
                             optParams, nbOptParams,
                             (const TA_Real*[]){ gBuffer[0].out0, gBuffer[0].out1,
                                                 gBuffer[0].out2, NULL }, NULL );
      if( errNb != TA_TEST_PASS )
      {
         printf( "Fail: %s: server verification\n", label );
         return errNb;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber testMacdFamilySignalOne( const TA_History *history )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx, outNbElement;
   TA_Integer endIdx = (TA_Integer)(history->nbBars - 1);
   int maType;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   clearAllBuffers();
   setInputBuffer( 0, history->close, history->nbBars );

   /* MACD(12,26,1): the issue #48/#59 case. */
   retCode = TA_MACD( 0, endIdx, gBuffer[0].in, 12, 26, 1,
                      &outBegIdx, &outNbElement,
                      gBuffer[0].out0, gBuffer[0].out1, gBuffer[0].out2 );
   errNb = pbCheckMacdSignalOne( "MACD(12,26,1)", "MACD", history,
                                 retCode, outBegIdx, outNbElement,
                                 TA_MACD_Lookback( 12, 26, 1 ),
                                 (const double[]){ 12, 26, 1 }, 3 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* The MACD line must be the same series whether the signal period
    * is 1 or 2 (on the overlapping range): no shift, no rescale.
    */
   {
      TA_Integer outBegIdx2, outNbElement2;
      int offset;

      retCode = TA_MACD( 0, endIdx, gBuffer[0].in, 12, 26, 2,
                         &outBegIdx2, &outNbElement2,
                         gBuffer[1].out0, gBuffer[1].out1, gBuffer[1].out2 );
      PB_CHECK_RC( "MACD(12,26,2)", retCode, TA_SUCCESS );
      PB_CHECK_INT( "MACD(12,26,2) outBegIdx", outBegIdx2, TA_MACD_Lookback( 12, 26, 2 ) );

      offset = outBegIdx2 - outBegIdx;
      errNb = pbCheckSameSeries( "MACD line sig=1 vs sig=2 alignment",
                                 &gBuffer[0].out0[offset], gBuffer[1].out0, outNbElement2 );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* MACDFIX(1). */
   retCode = TA_MACDFIX( 0, endIdx, gBuffer[0].in, 1,
                         &outBegIdx, &outNbElement,
                         gBuffer[0].out0, gBuffer[0].out1, gBuffer[0].out2 );
   errNb = pbCheckMacdSignalOne( "MACDFIX(1)", "MACDFIX", history,
                                 retCode, outBegIdx, outNbElement,
                                 TA_MACDFIX_Lookback( 1 ),
                                 (const double[]){ 1 }, 1 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* MACDEXT with signalPeriod=1 for every signal MAType. */
   for( maType = 0; maType <= (int)TA_MAType_T3; maType++ )
   {
      char label[64];
      snprintf( label, sizeof(label), "MACDEXT(12,26,sig=1,maType=%d)", maType );

      retCode = TA_MACDEXT( 0, endIdx, gBuffer[0].in,
                            12, TA_MAType_SMA,
                            26, TA_MAType_SMA,
                            1, (TA_MAType)maType,
                            &outBegIdx, &outNbElement,
                            gBuffer[0].out0, gBuffer[0].out1, gBuffer[0].out2 );
      errNb = pbCheckMacdSignalOne( label, "MACDEXT", history,
                                    retCode, outBegIdx, outNbElement,
                                    TA_MACDEXT_Lookback( 12, TA_MAType_SMA,
                                                         26, TA_MAType_SMA,
                                                         1, (TA_MAType)maType ),
                                    (const double[]){ 12, (double)TA_MAType_SMA,
                                                      26, (double)TA_MAType_SMA,
                                                      1, (double)maType }, 6 );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* signal==macd still holds with a non-zero EMA unstable period... */
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 3 );
   retCode = TA_MACD( 0, endIdx, gBuffer[0].in, 12, 26, 1,
                      &outBegIdx, &outNbElement,
                      gBuffer[0].out0, gBuffer[0].out1, gBuffer[0].out2 );
   errNb = pbCheckMacdSignalOne( "MACD(12,26,1) unstable=3", "MACD", history,
                                 retCode, outBegIdx, outNbElement,
                                 TA_MACD_Lookback( 12, 26, 1 ),
                                 (const double[]){ 12, 26, 1 }, 3 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* ...and under Metastock compatibility. */
   TA_SetCompatibility( TA_COMPATIBILITY_METASTOCK );
   retCode = TA_MACD( 0, endIdx, gBuffer[0].in, 12, 26, 1,
                      &outBegIdx, &outNbElement,
                      gBuffer[0].out0, gBuffer[0].out1, gBuffer[0].out2 );
   errNb = pbCheckMacdSignalOne( "MACD(12,26,1) metastock", "MACD", history,
                                 retCode, outBegIdx, outNbElement,
                                 TA_MACD_Lookback( 12, 26, 1 ),
                                 (const double[]){ 12, 26, 1 }, 3 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Exhaustive startIdx/endIdx sweep of all 3 outputs at sig=1. */
   {
      PBMacdRangeParam testParam;

      testParam.in = history->close;
      testParam.fast = 12;
      testParam.slow = 26;
      testParam.signal = 1;
      errNb = doRangeTest( pbMacdRangeFunction, TA_FUNC_UNST_EMA,
                           (void *)&testParam, 3, 0 );
      if( errNb != TA_TEST_PASS )
      {
         printf( "Fail: MACD(12,26,1) range test\n" );
         return errNb;
      }
   }

   /* doRangeTest varies the unstable period and leaves it set. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   return TA_TEST_PASS;
}

/**********************************************************/
/* Sub-test: pins for functions that always allowed p=1   */
/**********************************************************/
static ErrorNumber testPeriodOnePins( const TA_History *history )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx, outNbElement;
   TA_Integer begIdx2, nbElement2;
   TA_Integer endIdx = (TA_Integer)(history->nbBars - 1);
   int i;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   clearAllBuffers();
   setInputBuffer( 0, history->high,  history->nbBars );
   setInputBuffer( 1, history->low,   history->nbBars );
   setInputBuffer( 2, history->close, history->nbBars );

   /* TRANGE is the reference series for the ATR/NATR/DI pins below. */
   retCode = TA_TRANGE( 0, endIdx, gBuffer[0].in, gBuffer[1].in, gBuffer[2].in,
                        &begIdx2, &nbElement2, gBuffer[3].out0 );
   PB_CHECK_RC( "TRANGE", retCode, TA_SUCCESS );
   PB_CHECK_INT( "TRANGE outBegIdx", begIdx2, 1 );

   /* ATR(1) == TRANGE. */
   retCode = TA_ATR( 0, endIdx, gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, 1,
                     &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "ATR(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   errNb = pbCheckSameSeries( "ATR(1)==TRANGE", gBuffer[0].out0, gBuffer[3].out0, outNbElement );
   if( errNb != TA_TEST_PASS ) return errNb;
   if( server_verify_active() )
   {
      errNb = server_verify( "ATR", 0, endIdx, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, NULL },
                             (const double[]){ 1 }, 1,
                             (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* NATR(1) == TRANGE as well: the historical no-smoothing shortcut
    * skips the 100*TR/close normalization. This is a deliberate pin
    * of long-released behavior (see GitHub #94 discussion) — if NATR
    * period=1 semantics are ever changed, this row must be edited
    * consciously.
    */
   retCode = TA_NATR( 0, endIdx, gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, 1,
                      &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "NATR(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   errNb = pbCheckSameSeries( "NATR(1)==TRANGE (historical quirk)", gBuffer[0].out0,
                              gBuffer[3].out0, outNbElement );
   if( errNb != TA_TEST_PASS ) return errNb;
   if( server_verify_active() )
   {
      errNb = server_verify( "NATR", 0, endIdx, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, NULL },
                             (const double[]){ 1 }, 1,
                             (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* +DM(1) / -DM(1): the raw directional movements. Also feed the
    * DI(1) pins: DI(1) == DM(1)/TRANGE with no x100 scaling — the
    * second historical quirk pinned on purpose (DI(n>=2) is 0-100).
    */
   retCode = TA_PLUS_DM( 0, endIdx, gBuffer[0].in, gBuffer[1].in, 1,
                         &outBegIdx, &outNbElement, gBuffer[1].out0 );
   errNb = pbCheckCallShape( "PLUS_DM(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   if( server_verify_active() )
   {
      errNb = server_verify( "PLUS_DM", 0, endIdx, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, NULL },
                             (const double[]){ 1 }, 1,
                             (const TA_Real*[]){ gBuffer[1].out0, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   retCode = TA_PLUS_DI( 0, endIdx, gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, 1,
                         &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "PLUS_DI(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < outNbElement; i++ )
   {
      TA_Real expected = (gBuffer[3].out0[i] == 0.0) ? 0.0
                         : gBuffer[1].out0[i] / gBuffer[3].out0[i];
      if( fabs( gBuffer[0].out0[i] - expected ) > 1e-9 )
      {
         printf( "\nFail: PLUS_DI(1)==+DM/TR (historical quirk): [%d] got %.17g, expected %.17g\n",
                 i, gBuffer[0].out0[i], expected );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }
   if( server_verify_active() )
   {
      errNb = server_verify( "PLUS_DI", 0, endIdx, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, NULL },
                             (const double[]){ 1 }, 1,
                             (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   retCode = TA_MINUS_DM( 0, endIdx, gBuffer[0].in, gBuffer[1].in, 1,
                          &outBegIdx, &outNbElement, gBuffer[1].out0 );
   errNb = pbCheckCallShape( "MINUS_DM(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;

   retCode = TA_MINUS_DI( 0, endIdx, gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, 1,
                          &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "MINUS_DI(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < outNbElement; i++ )
   {
      TA_Real expected = (gBuffer[3].out0[i] == 0.0) ? 0.0
                         : gBuffer[1].out0[i] / gBuffer[3].out0[i];
      if( fabs( gBuffer[0].out0[i] - expected ) > 1e-9 )
      {
         printf( "\nFail: MINUS_DI(1)==-DM/TR (historical quirk): [%d] got %.17g, expected %.17g\n",
                 i, gBuffer[0].out0[i], expected );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   /* ULTOSC(1,1,1): one-bar buying pressure over true range, x100. */
   retCode = TA_ULTOSC( 0, endIdx, gBuffer[0].in, gBuffer[1].in, gBuffer[2].in,
                        1, 1, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "ULTOSC(1,1,1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < outNbElement; i++ )
   {
      int bar = outBegIdx + i;
      TA_Real trueLow  = history->low[bar]  < history->close[bar-1] ? history->low[bar]  : history->close[bar-1];
      TA_Real trueHigh = history->high[bar] > history->close[bar-1] ? history->high[bar] : history->close[bar-1];
      TA_Real tr = trueHigh - trueLow;
      TA_Real expected = (tr == 0.0) ? 0.0 : 100.0 * (history->close[bar] - trueLow) / tr;
      if( fabs( gBuffer[0].out0[i] - expected ) > 1e-9 )
      {
         printf( "\nFail: ULTOSC(1,1,1): [%d] got %.17g, expected %.17g\n",
                 i, gBuffer[0].out0[i], expected );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }
   if( server_verify_active() )
   {
      errNb = server_verify( "ULTOSC", 0, endIdx, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, NULL },
                             (const double[]){ 1, 1, 1 }, 3,
                             (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* STOCH family: a K/D smoothing period of 1 must be transparent.
    * STOCHF(5,1) fastK is the raw %K; STOCH(5,1,1) slowK and slowD
    * both equal it, for SMA and EMA smoothing types alike.
    */
   retCode = TA_STOCHF( 0, endIdx, gBuffer[0].in, gBuffer[1].in, gBuffer[2].in,
                        5, 1, TA_MAType_SMA,
                        &outBegIdx, &outNbElement, gBuffer[2].out0, gBuffer[2].out1 );
   errNb = pbCheckCallShape( "STOCHF(5,1,SMA)", retCode, outBegIdx, 4, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < outNbElement; i++ )
   {
      int bar = outBegIdx + i;
      TA_Real hh = history->high[bar], ll = history->low[bar];
      TA_Real expected;
      int j;
      for( j = bar - 4; j < bar; j++ )
      {
         if( history->high[j] > hh ) hh = history->high[j];
         if( history->low[j]  < ll ) ll = history->low[j];
      }
      expected = (hh - ll == 0.0) ? 0.0 : 100.0 * (history->close[bar] - ll) / (hh - ll);
      if( fabs( gBuffer[2].out0[i] - expected ) > 1e-9 )
      {
         printf( "\nFail: STOCHF(5,1) fastK==raw %%K: [%d] got %.17g, expected %.17g\n",
                 i, gBuffer[2].out0[i], expected );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }
   errNb = pbCheckSameSeries( "STOCHF(5,1) fastD==fastK", gBuffer[2].out1,
                              gBuffer[2].out0, outNbElement );
   if( errNb != TA_TEST_PASS ) return errNb;
   if( server_verify_active() )
   {
      errNb = server_verify( "STOCHF", 0, endIdx, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, NULL },
                             (const double[]){ 5, 1, (double)TA_MAType_SMA }, 3,
                             (const TA_Real*[]){ gBuffer[2].out0, gBuffer[2].out1, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   for( i = 0; i < 2; i++ )
   {
      TA_MAType smoothing = (i == 0) ? TA_MAType_SMA : TA_MAType_EMA;
      char label[64];
      snprintf( label, sizeof(label), "STOCH(5,1,1,%s)", (i == 0) ? "SMA" : "EMA" );

      retCode = TA_STOCH( 0, endIdx, gBuffer[0].in, gBuffer[1].in, gBuffer[2].in,
                          5, 1, smoothing, 1, smoothing,
                          &outBegIdx, &outNbElement, gBuffer[0].out0, gBuffer[0].out1 );
      errNb = pbCheckCallShape( label, retCode, outBegIdx, 4, outNbElement, endIdx );
      if( errNb != TA_TEST_PASS ) return errNb;
      errNb = pbCheckSameSeries( label, gBuffer[0].out0, gBuffer[2].out0, outNbElement );
      if( errNb != TA_TEST_PASS ) return errNb;
      errNb = pbCheckSameSeries( label, gBuffer[0].out1, gBuffer[2].out0, outNbElement );
      if( errNb != TA_TEST_PASS ) return errNb;

      if( server_verify_active() )
      {
         errNb = server_verify( "STOCH", 0, endIdx, history->nbBars,
                                retCode, outBegIdx, outNbElement,
                                (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in, gBuffer[2].in, NULL },
                                (const double[]){ 5, 1, (double)smoothing, 1, (double)smoothing }, 5,
                                (const TA_Real*[]){ gBuffer[0].out0, gBuffer[0].out1, NULL }, NULL );
         if( errNb != TA_TEST_PASS ) return errNb;
      }
   }

   /* STOCHRSI(14,1,1): a 1-bar %K window means high==low==RSI, which
    * the %K zero-divide guard maps to 0 on every bar.
    */
   retCode = TA_STOCHRSI( 0, endIdx, gBuffer[2].in, 14, 1, 1, TA_MAType_SMA,
                          &outBegIdx, &outNbElement, gBuffer[0].out0, gBuffer[0].out1 );
   errNb = pbCheckCallShape( "STOCHRSI(14,1,1)", retCode, outBegIdx, 14, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < outNbElement; i++ )
   {
      if( gBuffer[0].out0[i] != 0.0 || gBuffer[0].out1[i] != 0.0 )
      {
         printf( "\nFail: STOCHRSI(14,1,1): [%d] fastK=%.17g fastD=%.17g, expected 0\n",
                 i, gBuffer[0].out0[i], gBuffer[0].out1[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }
   if( server_verify_active() )
   {
      errNb = server_verify( "STOCHRSI", 0, endIdx, history->nbBars,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ gBuffer[2].in, NULL },
                             (const double[]){ 14, 1, 1, (double)TA_MAType_SMA }, 4,
                             (const TA_Real*[]){ gBuffer[0].out0, gBuffer[0].out1, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* MOM/ROC family at period=1: one-bar difference and ratios. */
   retCode = TA_MOM( 0, endIdx, gBuffer[2].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "MOM(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < outNbElement; i++ )
   {
      TA_Real expected = history->close[i+1] - history->close[i];
      if( fabs( gBuffer[0].out0[i] - expected ) > 1e-9 )
      {
         printf( "\nFail: MOM(1): [%d] got %.17g, expected %.17g\n",
                 i, gBuffer[0].out0[i], expected );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   retCode = TA_ROC( 0, endIdx, gBuffer[2].in, 1, &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "ROC(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* TRIX(1): three identity EMAs followed by a 1-day rate-of-change,
    * so TRIX(1) == ROC(1) exactly.
    */
   retCode = TA_TRIX( 0, endIdx, gBuffer[2].in, 1, &begIdx2, &nbElement2, gBuffer[1].out0 );
   errNb = pbCheckCallShape( "TRIX(1)", retCode, begIdx2, 1, nbElement2, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < nbElement2; i++ )
   {
      if( fabs( gBuffer[1].out0[i] - gBuffer[0].out0[i] ) > 1e-9 )
      {
         printf( "\nFail: TRIX(1)==ROC(1): [%d] got %.17g, expected %.17g\n",
                 i, gBuffer[1].out0[i], gBuffer[0].out0[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }
   if( server_verify_active() )
   {
      errNb = server_verify( "TRIX", 0, endIdx, history->nbBars,
                             retCode, begIdx2, nbElement2,
                             (const TA_Real*[]){ gBuffer[2].in, NULL },
                             (const double[]){ 1 }, 1,
                             (const TA_Real*[]){ gBuffer[1].out0, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* Degenerate-window statistics: exact zeros on every bar. */
   retCode = TA_VAR( 0, endIdx, gBuffer[2].in, 1, 1.0,
                     &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "VAR(1)", retCode, outBegIdx, 0, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   /* Mathematically 0 (x^2 - x*x), but NOT bit-exact everywhere: under FMA
    * contraction (ARM64) the fused x*x is subtracted from the rounded x^2,
    * leaving ~1e-13 residue. Same for BETA/CORREL below.
    */
   for( i = 0; i < outNbElement; i++ )
   {
      if( fabs( gBuffer[0].out0[i] ) > 1e-9 )
      {
         printf( "\nFail: VAR(1): [%d] got %.17g, expected ~0\n", i, gBuffer[0].out0[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   retCode = TA_CORREL( 0, endIdx, gBuffer[0].in, gBuffer[1].in, 1,
                        &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "CORREL(1)", retCode, outBegIdx, 0, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < outNbElement; i++ )
   {
      if( fabs( gBuffer[0].out0[i] ) > 1e-9 )
      {
         printf( "\nFail: CORREL(1): [%d] got %.17g, expected ~0\n", i, gBuffer[0].out0[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   retCode = TA_BETA( 0, endIdx, gBuffer[0].in, gBuffer[1].in, 1,
                      &outBegIdx, &outNbElement, gBuffer[0].out0 );
   errNb = pbCheckCallShape( "BETA(1)", retCode, outBegIdx, 1, outNbElement, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;
   for( i = 0; i < outNbElement; i++ )
   {
      if( fabs( gBuffer[0].out0[i] ) > 1e-9 )
      {
         printf( "\nFail: BETA(1): [%d] got %.17g, expected ~0\n", i, gBuffer[0].out0[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   return TA_TEST_PASS;
}

/**********************************************/
/* Sub-test: abstract-driven min-param sweep  */
/**********************************************/

/* Scan every real output element in [0,nb) for NaN/Inf/subnormal garbage —
 * the "no denormal-garbage values" contract from issue #94. A subnormal is a
 * valid finite double in the abstract, but no TA indicator produces one on the
 * reference data; treating it as a failure is a deliberate uninitialized-memory
 * heuristic — it is how the MAVP inverted-window garbage (6.5e-310) was caught,
 * and ASan/UBSan do not flag uninitialized reads. Integer outputs are not
 * scanned (any bit pattern is a legal candlestick/index code).
 */
static ErrorNumber pbScanOutputsFinite( const char *label,
                                        const TA_FuncHandle *handle,
                                        const TA_FuncInfo *funcInfo,
                                        int nb )
{
   const TA_OutputParameterInfo *outputInfo;
   unsigned int o;
   int j;

   for( o = 0; o < funcInfo->nbOutput && o < PB_MAX_OUTPUT; o++ )
   {
      TA_GetOutputParameterInfo( handle, o, &outputInfo );
      if( outputInfo->type != TA_Output_Real )
         continue;
      for( j = 0; j < nb; j++ )
      {
         TA_Real v = pbSweepOutReal[o][j];
         if( !isfinite( v ) || fpclassify( v ) == FP_SUBNORMAL )
         {
            printf( "\nFail: %s: out[%u][%d] = %.17g is not finite/normal\n",
                    label, o, j, v );
            return TA_REGTEST_OPTIMIZATION_REF_ERROR;
         }
      }
   }
   return TA_TEST_PASS;
}

/* Run one sweep case: optional parameter `paramNb` set to `ivalue` (integer
 * params) or `dvalue` (when isReal), every other parameter left at its
 * default. `expect` is PB_EXPECT_STRICT (must succeed, coherent+finite),
 * PB_EXPECT_LENIENT (coherent OR a clean TA_BAD_PARAM), or PB_EXPECT_REJECT
 * (must return TA_BAD_PARAM). A successful call is coherent when the output
 * either fully spans lookback..last-bar (lookback <= endIdx) or is empty (the
 * period consumes the whole range); a crash, an out-of-bounds access, a
 * garbage/non-finite value, or any other retCode is always a failure.
 * On the first failure sets ctx->errNb and returns.
 */
static void pbSweepRunCase( PBSweepCtx *ctx,
                            const TA_FuncInfo *funcInfo,
                            unsigned int paramNb,
                            const TA_OptInputParameterInfo *optInfo,
                            int isReal, int ivalue, TA_Real dvalue,
                            int expect )
{
   const TA_History *history = ctx->history;
   const TA_FuncHandle *handle = funcInfo->handle;
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outputInfo;
   TA_ParamHolder *paramHolder;
   TA_RetCode retCode;
   unsigned int i;
   int endIdx = (int)history->nbBars - 1;
   int outBegIdx = -1, outNbElement = -1, lookback = -1;
   char label[160];

   {
      const char *tag = ( expect == PB_EXPECT_REJECT )  ? " (expect BAD_PARAM)" :
                        ( expect == PB_EXPECT_LENIENT ) ? " (lenient)" : "";
      if( isReal )
         snprintf( label, sizeof(label), "sweep %s.%s=%.9g%s",
                   funcInfo->name, optInfo->paramName, dvalue, tag );
      else
         snprintf( label, sizeof(label), "sweep %s.%s=%d%s",
                   funcInfo->name, optInfo->paramName, ivalue, tag );
   }

   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFail: %s: TA_ParamHolderAlloc [%d]\n", label, retCode );
      pbFail( ctx );
      return;
   }
   ctx->nbParamTested++;

   for( i = 0; i < funcInfo->nbInput; i++ )
   {
      TA_GetInputParameterInfo( handle, i, &inputInfo );
      switch( inputInfo->type )
      {
      case TA_Input_Price:
         TA_SetInputParamPricePtr( paramHolder, i,
            inputInfo->flags & TA_IN_PRICE_OPEN   ? history->open   : NULL,
            inputInfo->flags & TA_IN_PRICE_HIGH   ? history->high   : NULL,
            inputInfo->flags & TA_IN_PRICE_LOW    ? history->low    : NULL,
            inputInfo->flags & TA_IN_PRICE_CLOSE  ? history->close  : NULL,
            inputInfo->flags & TA_IN_PRICE_VOLUME ? history->volume : NULL,
            NULL );
         break;
      case TA_Input_Real:
         /* Second real input of MAVP is the periods array: close prices are
          * clamped into [minPeriod,maxPeriod], exactly the boundary behavior
          * we want exercised. */
         TA_SetInputParamRealPtr( paramHolder, i, history->close );
         break;
      case TA_Input_Integer:
         /* No function currently uses an integer input array. */
         break;
      }
   }

   for( i = 0; i < funcInfo->nbOutput && i < PB_MAX_OUTPUT; i++ )
   {
      TA_GetOutputParameterInfo( handle, i, &outputInfo );
      if( outputInfo->type == TA_Output_Real )
         TA_SetOutputParamRealPtr( paramHolder, i, &pbSweepOutReal[i][0] );
      else
         TA_SetOutputParamIntegerPtr( paramHolder, i, &pbSweepOutInt[i][0] );
   }

   if( isReal )
      retCode = TA_SetOptInputParamReal( paramHolder, paramNb, dvalue );
   else
      retCode = TA_SetOptInputParamInteger( paramHolder, paramNb, ivalue );
   if( retCode != TA_SUCCESS )
   {
      /* A set-time rejection is a clean rejection: fine for an out-of-range
       * (or cross-constrained) value, a failure only for a strictly-valid one. */
      if( expect == PB_EXPECT_STRICT )
      {
         printf( "\nFail: %s: set-param rejected a valid value [%d]\n", label, retCode );
         pbFail( ctx );
      }
      else if( retCode != TA_BAD_PARAM )
      {
         printf( "\nFail: %s: set-param retCode %d, expected TA_BAD_PARAM\n", label, retCode );
         pbFail( ctx );
      }
      TA_ParamHolderFree( paramHolder );
      return;
   }

   retCode = TA_CallFunc( paramHolder, 0, endIdx, &outBegIdx, &outNbElement );

   if( expect == PB_EXPECT_REJECT )
   {
      if( retCode != TA_BAD_PARAM )
      {
         printf( "\nFail: %s: retCode %d, expected TA_BAD_PARAM\n", label, retCode );
         pbFail( ctx );
      }
      TA_ParamHolderFree( paramHolder );
      return;
   }

   /* A lenient (extreme) value may be cleanly rejected by a cross-parameter
    * constraint — that is an acceptable outcome, just not a coherent result. */
   if( expect == PB_EXPECT_LENIENT && retCode == TA_BAD_PARAM )
   {
      TA_ParamHolderFree( paramHolder );
      return;
   }

   /* Otherwise the call must succeed with a coherent, finite result. */
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFail: %s: retCode %d, expected TA_SUCCESS\n", label, retCode );
      pbFail( ctx );
      TA_ParamHolderFree( paramHolder );
      return;
   }
   if( TA_GetLookback( paramHolder, &lookback ) != TA_SUCCESS )
   {
      printf( "\nFail: %s: TA_GetLookback failed\n", label );
      pbFail( ctx );
      TA_ParamHolderFree( paramHolder );
      return;
   }

   if( lookback <= endIdx )
   {
      /* Enough data: output must start at or after the lookback (a later start
       * is valid — e.g. the #99 BBANDS/MAMA middle-band realign) and reach the
       * last bar with a positive count. */
      if( outBegIdx < lookback || outNbElement <= 0 ||
          outBegIdx + outNbElement - 1 != endIdx )
      {
         printf( "\nFail: %s: incoherent begIdx %d nb %d lookback %d (last bar %d)\n",
                 label, outBegIdx, outNbElement, lookback, endIdx );
         pbFail( ctx );
         TA_ParamHolderFree( paramHolder );
         return;
      }
      if( pbScanOutputsFinite( label, handle, funcInfo, outNbElement ) != TA_TEST_PASS )
      {
         pbFail( ctx );
         TA_ParamHolderFree( paramHolder );
         return;
      }
   }
   else
   {
      /* The period consumes the whole range: the coherent result is empty. */
      if( outNbElement != 0 )
      {
         printf( "\nFail: %s: lookback %d > last bar %d but nb %d != 0\n",
                 label, lookback, endIdx, outNbElement );
         pbFail( ctx );
         TA_ParamHolderFree( paramHolder );
         return;
      }
   }

   TA_ParamHolderFree( paramHolder );
}

/* Boundary grid for one function: every optional parameter is swept across
 * min / min+1 / default-1 / default / default+1 / a large "past the data"
 * period (integer ranges), across every enumerated value (integer/real
 * lists), and across min / default / max (real ranges — the library does not
 * range-check reals, so those exercise the finite scan, not BAD_PARAM). The
 * out-of-range integers min-1 and (bounded) max+1 must be cleanly rejected.
 * Every other parameter is held at its default. #94's true-max / integer-
 * overflow surface is deliberately delegated to the ASan/UBSan nightly job
 * (a value no caller passes, and unsafe to force here in a plain build).
 */
static void pbSweepOneFunction( const TA_FuncInfo *funcInfo, void *opaque )
{
   PBSweepCtx *ctx = (PBSweepCtx *)opaque;
   const TA_FuncHandle *handle = funcInfo->handle;
   const TA_OptInputParameterInfo *optInfo;
   unsigned int paramNb;
   int endIdx = (int)ctx->history->nbBars - 1;
   /* MAVP is the only indicator with an integer inter-parameter ordering
    * constraint (minPeriod <= maxPeriod): sweeping one period past the other's
    * default legitimately returns TA_BAD_PARAM, so its non-default in-range
    * values are lenient. Every OTHER function must compute at every realistic
    * value (min / min+1 / default+-1) — so a regression that wrongly rejects a
    * documented minimum period is caught, not silently accepted. */
   int crossConstrained = ( strcmp( funcInfo->name, "MAVP" ) == 0 );

   if( ctx->errNb != TA_TEST_PASS )
      return;   /* Already failed: skip the rest quietly. */

   for( paramNb = 0; paramNb < funcInfo->nbOptInput; paramNb++ )
   {
      TA_GetOptInputParameterInfo( handle, paramNb, &optInfo );

      switch( optInfo->type )
      {
      case TA_OptInput_IntegerRange:
      {
         const TA_IntegerRange *r = (const TA_IntegerRange *)optInfo->dataSet;
         int lo = r->min, hi = r->max, def = (int)optInfo->defaultValue;
         /* A large period near/past the data boundary — overflow-safe. */
         int big = (hi < endIdx + 2) ? hi : endIdx + 2;
         int wanted[5];
         int cand[5];
         int nWanted = 0, nCand = 0, k, m, dup;

         wanted[nWanted++] = lo;
         wanted[nWanted++] = lo + 1;
         wanted[nWanted++] = def - 1;
         wanted[nWanted++] = def;
         wanted[nWanted++] = def + 1;
         for( m = 0; m < nWanted; m++ )
         {
            int v = wanted[m];
            if( v < lo || v > hi ) continue;
            dup = 0;
            for( k = 0; k < nCand; k++ ) if( cand[k] == v ) { dup = 1; break; }
            if( !dup ) cand[nCand++] = v;
         }
         /* Realistic values must compute coherently. Only a cross-constrained
          * function (MAVP) is allowed to reject a non-default boundary; the
          * default itself must always succeed, for every function. */
         for( k = 0; k < nCand; k++ )
         {
            int expect = ( !crossConstrained || cand[k] == def )
                             ? PB_EXPECT_STRICT : PB_EXPECT_LENIENT;
            pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 0, cand[k], 0.0, expect );
            if( ctx->errNb != TA_TEST_PASS ) return;
         }
         /* A large "past the data" period: coherent (empty) OR a clean
          * cross-parameter rejection (e.g. minPeriod pushed above maxPeriod). */
         dup = 0;
         for( k = 0; k < nCand; k++ ) if( cand[k] == big ) { dup = 1; break; }
         if( !dup && big >= lo && big <= hi )
         {
            pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 0, big, 0.0,
                            PB_EXPECT_LENIENT );
            if( ctx->errNb != TA_TEST_PASS ) return;
         }
         /* min-1 is always below the floor: a clean rejection. */
         if( lo > INT_MIN )
         {
            pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 0, lo - 1, 0.0,
                            PB_EXPECT_REJECT );
            if( ctx->errNb != TA_TEST_PASS ) return;
         }
         /* max+1 above a *bounded* ceiling: a clean rejection. */
         if( hi > 0 && hi <= PB_SWEEP_MAX_BOUNDED )
         {
            pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 0, hi + 1, 0.0,
                            PB_EXPECT_REJECT );
            if( ctx->errNb != TA_TEST_PASS ) return;
         }
         break;
      }
      case TA_OptInput_IntegerList:
      {
         const TA_IntegerList *list = (const TA_IntegerList *)optInfo->dataSet;
         unsigned int e;
         for( e = 0; e < list->nbElement; e++ )
         {
            pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 0,
                            list->data[e].value, 0.0, PB_EXPECT_STRICT );
            if( ctx->errNb != TA_TEST_PASS ) return;
         }
         break;
      }
      case TA_OptInput_RealRange:
      {
         const TA_RealRange *r = (const TA_RealRange *)optInfo->dataSet;
         TA_Real wanted[3];
         TA_Real cand[3];
         int nWanted = 0, nCand = 0, k, m, dup;

         wanted[nWanted++] = r->min;
         wanted[nWanted++] = optInfo->defaultValue;
         wanted[nWanted++] = r->max;
         for( m = 0; m < nWanted; m++ )
         {
            TA_Real v = wanted[m];
            /* Only realistic magnitudes: the abstract ranges are often
             * effectively unbounded (~+/-3e37), and forcing those risks a
             * legitimate Inf that is not a memory-safety signal. */
            if( !isfinite( v ) || fabs( v ) > 1e6 ) continue;
            dup = 0;
            for( k = 0; k < nCand; k++ ) if( cand[k] == v ) { dup = 1; break; }
            if( !dup ) cand[nCand++] = v;
         }
         for( k = 0; k < nCand; k++ )
         {
            pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 1, 0, cand[k],
                            PB_EXPECT_STRICT );
            if( ctx->errNb != TA_TEST_PASS ) return;
         }
         break;
      }
      case TA_OptInput_RealList:
      {
         const TA_RealList *list = (const TA_RealList *)optInfo->dataSet;
         unsigned int e;
         for( e = 0; e < list->nbElement; e++ )
         {
            pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 1, 0,
                            list->data[e].value, PB_EXPECT_STRICT );
            if( ctx->errNb != TA_TEST_PASS ) return;
         }
         break;
      }
      }
   }
}

static ErrorNumber testMinBoundarySweep( const TA_History *history )
{
   PBSweepCtx ctx;

   /* The per-output scratch buffers are sized to PB_DATA_SIZE; a lookback-0
    * function writes up to nbBars elements. Guard the coupling explicitly so a
    * larger reference dataset fails loudly here instead of corrupting memory. */
   if( (int)history->nbBars > PB_DATA_SIZE )
   {
      printf( "\nFail: boundary sweep buffers (%d) smaller than data (%d bars)\n",
              PB_DATA_SIZE, (int)history->nbBars );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   ctx.history = history;
   ctx.errNb = TA_TEST_PASS;
   ctx.nbParamTested = 0;
   ctx.nbFail = 0;
   g_pbListAll = ( getenv( "PB_SWEEP_LIST_ALL" ) != NULL );

   TA_ForEachFunc( pbSweepOneFunction, &ctx );

   if( g_pbListAll )
      printf( "\n  boundary sweep: %d cases, %d failure(s)\n",
              ctx.nbParamTested, ctx.nbFail );

   if( ctx.errNb != TA_TEST_PASS )
      return ctx.errNb;

   if( ctx.nbParamTested == 0 )
   {
      printf( "\nFail: boundary sweep tested no parameter (enumeration broken?)\n" );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   return TA_TEST_PASS;
}
