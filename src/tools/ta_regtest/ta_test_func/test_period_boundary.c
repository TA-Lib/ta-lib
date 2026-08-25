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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY        Description
 *  -------------------------------------------------------------------
 *  070226 MF,CC     First version. Period=1 / minimum-period boundary
 *                   cases for GitHub issues #48 and #59 (SourceForge
 *                   bug 84).
 *  070726 MF,CC     Widen the abstract sweep to the full parameter grid
 *                   (all parameter types, min..max+1) with a coherence /
 *                   clean-BAD_PARAM / finite-output contract (#94).
 *  072426 MF,CC     Route the sweep's empty-output (period>input) cases through
 *                   server_verify, extending that contract cross-language (#142).
 *  081226 KL,MF,CC  Drive the identity sweep off TA_FUNC_FLG_PERIOD1_IDENTITY
 *                   instead of a hand-list of non-MAType averages (#184).
 */

/* Description:
 *
 * Boundary tests for the smallest allowed period values, with an
 * emphasis on period=1 ("no smoothing"):
 *
 *  - Lookback contract: TA_INTEGER_DEFAULT maps to the default
 *    period, out-of-range params return -1, and period=1 lookbacks
 *    are coherent (the SF bug-84 case: TA_MACD_Lookback(2,7,1)==6).
 *  - Identity: SMA/EMA/WMA/DEMA/TEMA/TRIMA/KAMA/T3/HMA/MAVP and
 *    MA(period=1, every MAType) must return the input unchanged.
 *  - The same rule, enumerated instead of hand-listed: every function
 *    named by the TA_MAType metadata, plus every function declaring
 *    TA_FUNC_FLG_PERIOD1_IDENTITY, must be a bit-exact copy at period 1.
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
 *    lists at every enumerated value plus one past the highest; real
 *    ranges at min / default / max. Each in-range value must be coherent
 *    (outBegIdx >= lookback, coverage to the last bar, or an empty
 *    result) or a clean TA_BAD_PARAM; the default must always compute;
 *    out-of-range integers must be rejected; the lookback tier and the
 *    call tier must agree about whether the parameters are usable at
 *    all; and every output is scanned for NaN/Inf/subnormal garbage.
 *    Reads the metadata, so it adapts automatically. Set
 *    PB_SWEEP_LIST_ALL=1 to enumerate every failing case in one run
 *    instead of aborting on the first.
 *
 * When --codegen is active every successful hand-written call is
 * also verified against the language servers (C, Rust, Java, C#)
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
/* Server-verify scratch bounds: a Price input expands to at most OHLCV+OI (6)
 * pointers, and MACDEXT carries the most optional params (6). Sized with slack;
 * pbBuildServerInputs guards the input bound, the opt loop the param bound. */
#define PB_MAX_INPUT  8
#define PB_MAX_OPT    8
/* MA's shipped optInMAType choice list -- ta_codegen generates it from
 * enums.yaml, the same human-maintained file the enumerators come from, so it
 * is how that source of truth reaches a C test. A loop bounded by an enumerator
 * name (`<= TA_MAType_DISABLED`) silently stops covering whatever is appended
 * after it and no gate notices. The list carries its own count and the members'
 * real VALUES, so iterating it needs no cap here and survives a non-contiguous
 * list too. NULL if the metadata is unreachable (a failure, never a skip). */
static const TA_IntegerList *pbMaTypeList( void )
{
   const TA_FuncHandle *handle;
   const TA_OptInputParameterInfo *optInfo;

   if( TA_GetFuncHandle( "MA", &handle ) != TA_SUCCESS ) return NULL;
   if( TA_GetOptInputParameterInfo( handle, 1, &optInfo ) != TA_SUCCESS ) return NULL;
   if( optInfo->type != TA_OptInput_IntegerList || !optInfo->dataSet ) return NULL;
   if( strcmp( optInfo->paramName, "optInMAType" ) != 0 ) return NULL;
   return (const TA_IntegerList *)optInfo->dataSet;
}

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
   int nbServerEmpty;   /* empty-output cases cross-checked vs the servers (#142) */
   int nbLookbackParity;/* cases where lookback and call were compared (in-process C) */
   int nbLookbackParityServer; /* cases where the SAME check ran against the servers (#256) */
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
static ErrorNumber testEveryMovingAverageIdentity( const TA_History *history );
static ErrorNumber testMacdFamilySignalOne( const TA_History *history );
static ErrorNumber testMacdSignalOneHostile( void );
static ErrorNumber testPeriodOnePins( const TA_History *history );
static ErrorNumber testMinBoundarySweep( const TA_History *history );
static ErrorNumber testLinearRegRampOverflowProbe( void );
static int pbBuildServerInputs( const TA_FuncInfo *funcInfo,
                                const TA_History *history,
                                const TA_Real *inputs[], int maxInputs );
static void pbBuildSweptOptVector( const TA_FuncHandle *handle,
                                   unsigned int nbOptInput,
                                   unsigned int paramNb, int isReal,
                                   int ivalue, TA_Real dvalue,
                                   double svOpt[PB_MAX_OPT] );

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

   errNb = testEveryMovingAverageIdentity( history );
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

   errNb = testLinearRegRampOverflowProbe();
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
   const TA_IntegerList *maTypes;
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
   PB_CHECK_INT( "TA_HMA_Lookback(1)",   TA_HMA_Lookback( 1 ),   0 );
   PB_CHECK_INT( "TA_EMA_Lookback(2)",   TA_EMA_Lookback( 2 ),   1 );

   /* The SourceForge bug-84 report, verbatim. */
   PB_CHECK_INT( "TA_MACD_Lookback(2,7,1)", TA_MACD_Lookback( 2, 7, 1 ), 6 );
   PB_CHECK_INT( "TA_MACD_Lookback(2,7,2)", TA_MACD_Lookback( 2, 7, 2 ), 7 );
   PB_CHECK_INT( "TA_MACD_Lookback(12,26,1)", TA_MACD_Lookback( 12, 26, 1 ), 25 );
   PB_CHECK_INT( "TA_MACDFIX_Lookback(1)", TA_MACDFIX_Lookback( 1 ), 25 );

   PB_CHECK_INT( "TA_TRIX_Lookback(1)", TA_TRIX_Lookback( 1 ), 1 );
   PB_CHECK_INT( "TA_ULTOSC_Lookback(1,1,1)", TA_ULTOSC_Lookback( 1, 1, 1 ), 1 );

   maTypes = pbMaTypeList();
   if( !maTypes )
   {
      printf( "\nFail: cannot read MA's optInMAType choice list\n" );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   for( i = 0; i < (int)maTypes->nbElement; i++ )
   {
      PB_CHECK_INT( "TA_MA_Lookback(1,maType)",
                    TA_MA_Lookback( 1, (TA_MAType)maTypes->data[i].value ), 0 );
   }
   /* TA_MAType_DISABLED ignores the period: lookback is 0 at any period. */
   PB_CHECK_INT( "TA_MA_Lookback(30,DISABLED)",  TA_MA_Lookback( 30,  TA_MAType_DISABLED ), 0 );
   PB_CHECK_INT( "TA_MA_Lookback(100,DISABLED)", TA_MA_Lookback( 100, TA_MAType_DISABLED ), 0 );
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

static ErrorNumber testIdentityAtPeriodOne( const TA_History *history )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx, outNbElement;
   TA_Integer endIdx = (TA_Integer)(history->nbBars - 1);
   int i;
   const TA_IntegerList *maTypes;

   clearAllBuffers();
   setInputBuffer( 0, history->close, history->nbBars );

   /* The direct moving averages (SMA/EMA/WMA/DEMA/TEMA/TRIMA/KAMA/T3/HMA) are
    * NOT called here: testEveryMovingAverageIdentity below drives every one of
    * them off the TA_MAType metadata, on two input series, with the same
    * shape/value/server checks — a superset of what a hand list can state, and
    * one that a new MAType cannot slip past. What follows is the part that
    * sweep does not reach.
    */

   /* MA(period=1) for every MAType: the documented "just copy" path (includes
    * TA_MAType_DISABLED, whose copy is period-independent, and TA_MAType_DEFAULT,
    * which the prologue has already resolved to SMA by this point). */
   maTypes = pbMaTypeList();
   if( !maTypes )
   {
      printf( "\nFail: cannot read MA's optInMAType choice list\n" );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   for( i = 0; i < (int)maTypes->nbElement; i++ )
   {
      char label[64];
      snprintf( label, sizeof(label), "MA(1,maType=%d)", maTypes->data[i].value );
      retCode = TA_MA( 0, endIdx, gBuffer[0].in, 1, (TA_MAType)maTypes->data[i].value,
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

   /* TA_MAType_DISABLED (#93): identity copy that IGNORES the period. Unlike
    * the period==1 path above, it must copy for ANY period, with lookback 0 and
    * outBegIdx 0. Swept across several periods and bitwise-checked cross-language.
    */
   {
      static const int disabledPeriods[] = { 1, 2, 5, 30, 100 };
      unsigned int p;
      for( p = 0; p < sizeof(disabledPeriods)/sizeof(disabledPeriods[0]); p++ )
      {
         int period = disabledPeriods[p];
         char label[64];
         snprintf( label, sizeof(label), "MA(%d,DISABLED)", period );

         retCode = TA_MA( 0, endIdx, gBuffer[0].in, period, TA_MAType_DISABLED,
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
                                   (const double[]){ (double)period, (double)TA_MAType_DISABLED }, 2,
                                   (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL );
            if( errNb != TA_TEST_PASS )
            {
               printf( "Fail: %s: server verification\n", label );
               return errNb;
            }
         }
      }
   }

   /* DISABLED propagates through a delegating function: BBANDS with a DISABLED
    * middle band is the raw price (identity MA), so the bands are price +/-
    * nbDev*stddev. The standard deviation still needs its period-1 warmup, so
    * outBegIdx = period-1 while the MA lookback is 0 (the accepted
    * MAMA-large-period pattern, issue #99); the #99 realignment then pairs each
    * middle-band price with the standard deviation ending at the same bar.
    */
   {
      int period = 5;
      retCode = TA_BBANDS( 0, endIdx, gBuffer[0].in, period, 2.0, 2.0, TA_MAType_DISABLED,
                           &outBegIdx, &outNbElement,
                           gBuffer[0].out0, gBuffer[0].out1, gBuffer[0].out2 );
      errNb = pbCheckCallShape( "BBANDS(5,DISABLED)", retCode, outBegIdx, period-1, outNbElement, endIdx );
      if( errNb != TA_TEST_PASS ) return errNb;
      /* Middle band == raw price at the aligned bars. */
      errNb = pbCheckSameSeries( "BBANDS(5,DISABLED) middle", gBuffer[0].out1,
                                 &history->close[outBegIdx], outNbElement );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* doRangeTest varies the unstable period and leaves it set. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   return TA_TEST_PASS;
}

/****************************************************************/
/* Sub-test: EVERY moving average copies its input at period 1  */
/****************************************************************/

/* The contract: a moving average of period 1 performs no smoothing, so its
 * lookback is 0 and its output is a BIT-EXACT copy of its input.
 *
 * The list is the TA_MAType enumeration itself, read from the abstract
 * metadata — MA's optInMAType list pairs each enum value with the NAME of the
 * function it dispatches to — so a MAType added later is held to this without
 * anyone remembering to edit a table here. That is the whole point of driving
 * it off the enum: testIdentityAtPeriodOne above hand-lists its calls, and a
 * hand list cannot notice a new arrival.
 *
 * Two kinds of entry cannot be driven to a period of 1, and both must be named
 * here with the reason. An exemption the table does NOT list is a failure, not
 * a skip: without that, renaming a MAType label to something that resolves to
 * no function (or a function losing its period parameter) drops it from the
 * gate silently, and the accounting below still balances because every entry
 * increments exactly one counter either way.
 *
 * The moving averages that are NOT MAType-selectable used to be a hand-list here
 * (it held one name, VWMA). They now DECLARE themselves: `period1_identity` in a
 * function's YAML surfaces as TA_FUNC_FLG_PERIOD1_IDENTITY, and the second walk
 * below sweeps every function in the library carrying it. That is issue #184's
 * point — VWMA's (P*V)/V shipped precisely because the promise lived in prose and
 * the sweep's membership lived in a list nobody edited. MAVP is still absent, and
 * now for a reason the metadata states: it carries no such flag, its period being
 * a per-bar input array (pinned by its own case above).
 *
 * A flag word is a weaker membership test than a list in one specific way: a
 * function that LOSES the bit leaves the sweep instead of failing it. Three
 * different gates close that, and it is worth knowing which covers what before
 * trusting the count below:
 *
 *  - a bit lost in the C table only (a backend bug) fails test_abstract.c, which
 *    compares C's flags against the Rust/Java/C# registries -- an independent
 *    derivation, which is why that comparison is worth anything;
 *  - a bit lost on an ENUM member fails right here: every member reaching the
 *    check is required to carry it. A bit lost on one of the others fails the
 *    PB_MIN_FLAGGED floor, which is why that floor is a literal;
 *  - a `period1_identity` deleted from a function's YAML fails at generate time
 *    (ta_codegen/generator/tests/period1_suite.rs), which is also where the rule
 *    for who must DECLARE it lives. That gate covers the enum members and every
 *    function carrying a recognisable identity arm, which today is all of them.
 *
 * Every count is printed and asserted, so the sweep cannot pass by checking
 * nothing.
 */

/* Names collected from a TA_ForEachFunc walk. 168 functions today; the bound is
 * checked, and an overflow is a failure rather than a silent truncation. */
#define PB_MAX_FLAGGED 64
/* The flagged set at #184: the 9 MAType members that have a period, plus MA and
 * VWMA. A floor, so the set can grow freely and only shrinks deliberately. */
#define PB_MIN_FLAGGED 11

typedef struct
{
   const char *names[PB_MAX_FLAGGED];
   int nb;
   int overflow;
} PBFlaggedList;

static void pbCollectPeriod1Flagged( const TA_FuncInfo *funcInfo, void *opaqueData )
{
   PBFlaggedList *list = (PBFlaggedList *)opaqueData;

   if( !(funcInfo->flags & TA_FUNC_FLG_PERIOD1_IDENTITY) )
      return;
   if( list->nb >= PB_MAX_FLAGGED )
   {
      list->overflow++;
      return;
   }
   list->names[list->nb++] = funcInfo->name;
}

static const struct { const char *name; const char *why; } PB_MA_EXEMPT[] =
{
   { "DISABLED", "dispatch-only sentinel: no function of that name (the "
                 "MA(period=1) loop covers it)" },
   { "DEFAULT",  "resolved before dispatch: the prologue substitutes this "
                 "parameter's declared default, whose own row runs the check" },
   { "MAMA",     "no integer-range parameter: its two parameters are the real "
                 "fast/slow limits" },
};

/* The reason this name is allowed to escape the sweep, or NULL if it is not. */
static const char *pbMaExemptReason( const char *name )
{
   unsigned int i;
   for( i = 0; i < sizeof(PB_MA_EXEMPT)/sizeof(PB_MA_EXEMPT[0]); i++ )
      if( strcmp( PB_MA_EXEMPT[i].name, name ) == 0 )
         return PB_MA_EXEMPT[i].why;
   return NULL;
}

/* Call `name` with its first integer-range parameter set to 1, every other
 * parameter left at its default, and require lookback 0 plus a bit-exact copy
 * of the close series over the whole output range. Increments *checked when the
 * call ran, *exempt when the metadata says there is nothing to set to 1.
 * `mustExist` makes an unresolvable name a failure instead of an exemption. */
static ErrorNumber pbCheckMaIdentityByName( const char *name,
                                            const TA_History *history,
                                            int mustExist,
                                            int *checked,
                                            int *exempt )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo *funcInfo;
   const TA_InputParameterInfo *inputInfo;
   const TA_OptInputParameterInfo *optInfo;
   TA_ParamHolder *paramHolder;
   TA_RetCode retCode;
   TA_Integer outBegIdx = -1, outNbElement = -1;
   int lookback = -1;
   int endIdx = (TA_Integer)history->nbBars - 1;
   int periodParam = -1;
   unsigned int i;
   double optParams[PB_MAX_OPT];
   const TA_Real *serverInputs[PB_MAX_INPUT];
   char label[128];

   if( TA_GetFuncHandle( name, &handle ) != TA_SUCCESS )
   {
      if( mustExist || !pbMaExemptReason( name ) )
      {
         printf( "\nFail: period-1 MA identity: no function named '%s', and it "
                 "is not a documented exemption\n", name );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      (*exempt)++;
      return TA_TEST_PASS;
   }
   if( TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "\nFail: period-1 MA identity: TA_GetFuncInfo(%s)\n", name );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* The parameter to drive: the first integer RANGE (a choice list is a type
    * selector, not a period). No such parameter means nothing to set to 1. */
   for( i = 0; i < funcInfo->nbOptInput && i < PB_MAX_OPT; i++ )
   {
      TA_GetOptInputParameterInfo( handle, i, &optInfo );
      optParams[i] = optInfo->defaultValue;
      if( periodParam < 0 && optInfo->type == TA_OptInput_IntegerRange )
         periodParam = (int)i;
   }
   if( funcInfo->nbOptInput > PB_MAX_OPT )
   {
      printf( "\nFail: period-1 MA identity: %s has %u optional params (max %d)\n",
              name, funcInfo->nbOptInput, PB_MAX_OPT );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   if( periodParam < 0 )
   {
      if( !pbMaExemptReason( name ) )
      {
         printf( "\nFail: period-1 MA identity: %s has no integer-range "
                 "parameter to set to 1, and it is not a documented exemption\n",
                 name );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      (*exempt)++;
      return TA_TEST_PASS;
   }
   optParams[periodParam] = 1.0;

   /* Anything reaching the check must say so in its own metadata. For a name off
    * the MAType list that is the enum-implies-flag rule (a moving average that
    * does not copy its input at a period of 1 is not a moving average); for a
    * name off the flag walk it is true by construction, and asserting it anyway
    * is what makes a vanished flag word fail loudly instead of shrinking the
    * sweep. */
   if( !(funcInfo->flags & TA_FUNC_FLG_PERIOD1_IDENTITY) )
   {
      printf( "\nFail: period-1 MA identity: %s does not carry "
              "TA_FUNC_FLG_PERIOD1_IDENTITY, so nothing holds it to the copy\n"
              "      (add `period1_identity` to its ta_codegen/input YAML flags)\n",
              name );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* One real output: a moving average produces one series. Anything else is
    * a new shape someone must classify, not something to compare blindly. */
   if( funcInfo->nbOutput != 1 )
   {
      printf( "\nFail: period-1 MA identity: %s has %u outputs, expected 1\n",
              name, funcInfo->nbOutput );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* pbSweepOutReal is PB_DATA_SIZE wide and shared with the sweep below,
    * which runs after this sub-test — so its own bound check cannot protect
    * this one. */
   if( (int)history->nbBars > PB_DATA_SIZE )
   {
      printf( "\nFail: period-1 MA identity: history has %u bars, buffer holds %d\n",
              history->nbBars, PB_DATA_SIZE );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   snprintf( label, sizeof(label), "%s period=1 identity", name );

   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   PB_CHECK_RC( label, retCode, TA_SUCCESS );

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
         TA_SetInputParamRealPtr( paramHolder, i, history->close );
         break;
      case TA_Input_Integer:
         break;   /* no integer-input function today */
      }
   }
   TA_SetOutputParamRealPtr( paramHolder, 0, &pbSweepOutReal[0][0] );

   retCode = TA_SetOptInputParamInteger( paramHolder, (unsigned int)periodParam, 1 );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFail: %s: a period of 1 was rejected by set-param [%d]\n", label, retCode );
      TA_ParamHolderFree( paramHolder );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   retCode = TA_CallFunc( paramHolder, 0, endIdx, &outBegIdx, &outNbElement );
   if( retCode == TA_SUCCESS && TA_GetLookback( paramHolder, &lookback ) != TA_SUCCESS )
      lookback = -1;
   TA_ParamHolderFree( paramHolder );

   if( retCode != TA_SUCCESS )
   {
      printf( "\nFail: %s: retCode %d (a period of 1 must be accepted)\n", label, (int)retCode );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   if( lookback != 0 )
   {
      printf( "\nFail: %s: lookback %d, expected 0 (no smoothing, no warm-up)\n",
              label, lookback );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   {
      ErrorNumber errNb = pbCheckCallShape( label, retCode, outBegIdx, 0, outNbElement, endIdx );
      if( errNb != TA_TEST_PASS )
         return errNb;
      errNb = pbCheckSameSeries( label, &pbSweepOutReal[0][0],
                                 &history->close[outBegIdx], outNbElement );
      if( errNb != TA_TEST_PASS )
         return errNb;

      /* Cross-language: the rule is the library's, not C's. */
      if( server_verify_active() )
      {
         int nbIn = pbBuildServerInputs( funcInfo, history, serverInputs, PB_MAX_INPUT );
         if( nbIn < 0 )
         {
            printf( "\nFail: %s: too many inputs for server_verify\n", label );
            return TA_REGTEST_OPTIMIZATION_REF_ERROR;
         }
         errNb = server_verify( name, 0, endIdx, history->nbBars,
                                retCode, outBegIdx, outNbElement,
                                serverInputs, optParams, (int)funcInfo->nbOptInput,
                                (const TA_Real*[]){ &pbSweepOutReal[0][0], NULL }, NULL );
         if( errNb != TA_TEST_PASS )
         {
            printf( "Fail: %s: server verification\n", label );
            return errNb;
         }
      }
   }

   (*checked)++;
   return TA_TEST_PASS;
}

/* An input the naive formulas cannot round-trip.
 *
 * The reference series does not discriminate: VWMA's (P*V)/V happens to be
 * bit-exact on all 252 of its bars, so a gate run only there would pass while
 * the contract was broken (it did, before this series existed). Two-decimal
 * prices are NOT dyadic, so P already spends a full mantissa; multiplying by a
 * six-digit integer volume overflows it and the division cannot come back.
 * Ordinary equity data, in other words, not a pathological one: the reference
 * series is the lucky case, not this.
 *
 * The hostility is ASSERTED here, not asserted-by-comment: tweak a constant so
 * every bar round-trips and this fails, instead of quietly turning the sweep
 * below into a second copy of the reference run. That is the whole value of
 * the series -- it is the only leg that can see a VWMA period-1 regression.
 *
 * OHLC is kept valid (low <= open,close <= high) so a price-bundle input is fed
 * something coherent.
 */
#define PB_HOSTILE_MIN_DIVERGENT 8

static ErrorNumber pbFillRoundTripHostileHistory( TA_History *h )
{
   static TA_Real open[PB_DATA_SIZE], high[PB_DATA_SIZE], low[PB_DATA_SIZE];
   static TA_Real close[PB_DATA_SIZE], volume[PB_DATA_SIZE];
   int i, nbDivergent = 0;

   for( i = 0; i < PB_DATA_SIZE; i++ )
   {
      close[i]  = (TA_Real)(10000 + (i * 7919) % 20000) / 100.0;
      open[i]   = (TA_Real)(10000 + (i * 5417) % 20000) / 100.0;
      high[i]   = (close[i] > open[i] ? close[i] : open[i]) + 0.25;
      low[i]    = (close[i] < open[i] ? close[i] : open[i]) - 0.25;
      volume[i] = (TA_Real)(100003 + (i * 104729) % 899993);

      /* The naive period-1 form, spelled out: a bar where it does NOT return
       * the price is a bar on which the gate below has something to say. */
      if( (close[i] * volume[i]) / volume[i] != close[i] )
         nbDivergent++;
   }

   if( nbDivergent < PB_HOSTILE_MIN_DIVERGENT )
   {
      printf( "\nFail: the round-trip-hostile series is not hostile: (P*V)/V "
              "returns P on %d of %d bars (need at least %d divergent)\n",
              PB_DATA_SIZE - nbDivergent, PB_DATA_SIZE, PB_HOSTILE_MIN_DIVERGENT );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   h->nbBars = PB_DATA_SIZE;
   h->open = open;   h->high = high;  h->low = low;
   h->close = close; h->volume = volume;
   h->openInterest = NULL;
   return TA_TEST_PASS;
}

/* An input the EMA-family recursion cannot round-trip.
 *
 * At a period of 1 the EMA factor k = 2/(1+1) is exactly 1.0, so the recursion
 * reduces to (x - prev) + prev. That returns x only while (x - prev) is exactly
 * representable -- guaranteed by Sterbenz's lemma while prev/2 <= x <= 2*prev,
 * and easily lost outside it. Two-decimal prices are not dyadic, so they already
 * spend a full mantissa and a single 3x move is enough; the round-trip-hostile
 * series above cannot show this because its bar-to-bar moves are small.
 *
 * Like that series, the hostility is ASSERTED, not claimed: soften the moves (or
 * pick dyadic values, which is exactly how this went unnoticed) and this fails
 * rather than quietly becoming a third benign sweep.
 */
#define PB_STERBENZ_MIN_DIVERGENT 32

static ErrorNumber pbFillSterbenzHostileHistory( TA_History *h )
{
   static TA_Real open[PB_DATA_SIZE], high[PB_DATA_SIZE], low[PB_DATA_SIZE];
   static TA_Real close[PB_DATA_SIZE], volume[PB_DATA_SIZE];
   int i, nbDivergent = 0;
   TA_Real prev;

   for( i = 0; i < PB_DATA_SIZE; i++ )
   {
      /* Alternating ~3x, two-decimal, never dyadic. */
      close[i]  = (i & 1) ? 41.37 : 124.11;
      open[i]   = (i & 1) ? 41.53 : 123.67;
      high[i]   = (close[i] > open[i] ? close[i] : open[i]) + 0.11;
      low[i]    = (close[i] < open[i] ? close[i] : open[i]) - 0.11;
      volume[i] = (TA_Real)(100003 + (i * 104729) % 899993);
   }

   /* The naive period-1 EMA step, spelled out: a bar where it does not give
    * back the input is a bar on which the sweep has something to say. */
   prev = close[0];
   for( i = 1; i < PB_DATA_SIZE; i++ )
   {
      prev = ((close[i] - prev) * 1.0) + prev;
      if( prev != close[i] )
         nbDivergent++;
   }

   if( nbDivergent < PB_STERBENZ_MIN_DIVERGENT )
   {
      printf( "\nFail: the Sterbenz-hostile series is not hostile: the period-1 "
              "EMA step returns its input on all but %d of %d bars (need at "
              "least %d divergent)\n",
              nbDivergent, PB_DATA_SIZE - 1, PB_STERBENZ_MIN_DIVERGENT );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   h->nbBars = PB_DATA_SIZE;
   h->open = open;   h->high = high;  h->low = low;
   h->close = close; h->volume = volume;
   h->openInterest = NULL;
   return TA_TEST_PASS;
}

static ErrorNumber pbSweepMaIdentity( const TA_History *history, const char *what )
{
   const TA_FuncHandle *maHandle;
   const TA_FuncInfo *maInfo;
   const TA_OptInputParameterInfo *optInfo;
   const TA_IntegerList *maTypeList = NULL;
   PBFlaggedList flagged;
   ErrorNumber errNb;
   unsigned int i;
   int checked = 0, exempt = 0, extra = 0;

   flagged.nb = 0;
   flagged.overflow = 0;

   /* The enumeration, straight from the metadata TA_MA publishes. */
   PB_CHECK_RC( "TA_GetFuncHandle(MA)", TA_GetFuncHandle( "MA", &maHandle ), TA_SUCCESS );
   PB_CHECK_RC( "TA_GetFuncInfo(MA)", TA_GetFuncInfo( maHandle, &maInfo ), TA_SUCCESS );
   for( i = 0; i < maInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( maHandle, i, &optInfo );
      if( optInfo->type == TA_OptInput_IntegerList )
      {
         maTypeList = (const TA_IntegerList *)optInfo->dataSet;
         break;
      }
   }
   if( !maTypeList || maTypeList->nbElement == 0 )
   {
      printf( "\nFail: period-1 MA identity: MA publishes no MAType list\n" );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   for( i = 0; i < maTypeList->nbElement; i++ )
   {
      errNb = pbCheckMaIdentityByName( maTypeList->data[i].string, history,
                                       0 /*mustExist*/, &checked, &exempt );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   /* Everything that DECLARES the contract, enumerated from the library rather
    * than listed here. The ones already covered above are skipped by name: the
    * enum publishes labels, the walk publishes functions, and the flagged
    * moving averages are in both. */
   PB_CHECK_RC( "TA_ForEachFunc", TA_ForEachFunc( pbCollectPeriod1Flagged, &flagged ),
                TA_SUCCESS );
   if( flagged.overflow )
   {
      printf( "\nFail: period-1 MA identity: %d function(s) carry the flag, buffer holds %d\n",
              flagged.nb + flagged.overflow, PB_MAX_FLAGGED );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   for( i = 0; i < (unsigned int)flagged.nb; i++ )
   {
      int before = checked;
      unsigned int j;
      int inEnum = 0;

      for( j = 0; j < maTypeList->nbElement && !inEnum; j++ )
         if( strcmp( maTypeList->data[j].string, flagged.names[i] ) == 0 )
            inEnum = 1;
      if( inEnum )
         continue;

      errNb = pbCheckMaIdentityByName( flagged.names[i], history,
                                       1 /*mustExist*/, &checked, &exempt );
      if( errNb != TA_TEST_PASS )
         return errNb;
      if( checked == before )
      {
         /* The flag promises the copy at a period of 1, so a function carrying
          * it and having no period to set is a mis-declaration, not a skip. */
         printf( "\nFail: period-1 MA identity: '%s' carries "
                 "TA_FUNC_FLG_PERIOD1_IDENTITY but has no period to set to 1\n",
                 flagged.names[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      extra++;
   }

   /* Non-vacuity: every enum entry must have been either checked or exempted,
    * and the checked set must be non-empty. */
   if( checked - extra + exempt != (int)maTypeList->nbElement || checked <= extra )
   {
      printf( "\nFail: period-1 MA identity: %d checked (%d outside the enum), "
              "%d exempt, %u MAType value(s) — the accounting does not add up\n",
              checked, extra, exempt, maTypeList->nbElement );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   /* ...and the flag walk must still find what it found when it was written. A
    * LITERAL floor, not one derived from `checked`: a derived floor moves with
    * the set it is meant to protect, so the two functions the walk contributes
    * beyond the enum could both lose the bit and the accounting above would still
    * balance. Removing a member is legitimate (RSI and CMO were, their range
    * forbidding a period of 1) -- it just has to be a deliberate edit here. */
   if( flagged.nb < PB_MIN_FLAGGED )
   {
      printf( "\nFail: period-1 MA identity: %d function(s) carry "
              "TA_FUNC_FLG_PERIOD1_IDENTITY, expected at least %d — the flagged "
              "set has shrunk, so this sweep now covers less than it did\n",
              flagged.nb, PB_MIN_FLAGGED );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   printf( "\n  Period-1 identity (%s): %d moving average(s) copy their input "
           "bit-exactly (%u MAType value(s), %d exempt, %d flagged outside the enum)",
           what, checked, maTypeList->nbElement, exempt, extra );

   return TA_TEST_PASS;
}

/* The period-1 copy on the STREAMING surfaces, on hostile data.
 *
 * pbSweepMaIdentity above reaches only the double batch entry point. The
 * streaming surfaces are covered by stream_verify, which compares stream
 * against batch on its own seed-generated shapes -- and those are benign at
 * period 1, so an arm deleted from the step leaves batch(copy) and
 * stream(recursion) numerically equal and the gate green. Sabotage-proven:
 * removing the arm from TA_EMA_StepImpl alone passes ta_regtest and
 * 15908 stream_verify legs; removing it from TA_S_EMA instead is caught (by
 * the VARIANT gate), so only the streaming half needs this.
 *
 * EMA and DEMA only, and deliberately: they are the two whose period-1 value
 * comes from an EMA recursion rather than a window, i.e. the two the arm
 * actually rescues. The other arms are copies either way.
 */
static ErrorNumber pbCheckStreamCopy( const char *label, TA_RetCode rc,
                                      double got, double want )
{
   if( rc != TA_SUCCESS )
   {
      printf( "\nFail: %s: retCode %d\n", label, (int)rc );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   if( got != want )
   {
      printf( "\nFail: %s: got %.17g, expected %.17g\n", label, got, want );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   return TA_TEST_PASS;
}

static ErrorNumber testStreamIdentityAtPeriodOne( const TA_History *hostile )
{
   static TA_Real fill[PB_DATA_SIZE];
   const TA_Real *in = hostile->close;
   int n = (int)hostile->nbBars;
   int warm = 8;                 /* bars consumed by Open; the rest via Update */
   TA_EMA_Stream *es = NULL;
   TA_DEMA_Stream *ds = NULL;
   TA_Integer beg, nb;
   TA_RetCode rc;
   ErrorNumber errNb;
   double v;
   int i;

   /* --- EMA: Open, then Peek/Update bar by bar --- */
   rc = TA_EMA_Open( &es, in, warm, 1, &v );
   errNb = pbCheckStreamCopy( "EMA(1) stream Open", rc, v, in[warm-1] );
   if( errNb != TA_TEST_PASS ) { TA_EMA_Close( es ); return errNb; }
   for( i = warm; i < n; i++ )
   {
      rc = TA_EMA_Peek( es, in[i], &v );
      errNb = pbCheckStreamCopy( "EMA(1) stream Peek", rc, v, in[i] );
      if( errNb != TA_TEST_PASS ) { TA_EMA_Close( es ); return errNb; }
      rc = TA_EMA_Update( es, in[i], &v );
      errNb = pbCheckStreamCopy( "EMA(1) stream Update", rc, v, in[i] );
      if( errNb != TA_TEST_PASS ) { TA_EMA_Close( es ); return errNb; }
   }
   TA_EMA_Close( es );

   /* --- EMA: OpenAndFill over the whole history --- */
   rc = TA_EMA_OpenAndFill( &es, in, n, 1, &beg, &nb, fill );
   if( rc != TA_SUCCESS || beg != 0 || nb != n )
   {
      printf( "\nFail: EMA(1) OpenAndFill: rc=%d beg=%d nb=%d expected 0/0/%d\n",
              (int)rc, (int)beg, (int)nb, n );
      TA_EMA_Close( es );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   TA_EMA_Close( es );
   errNb = pbCheckSameSeries( "EMA(1) OpenAndFill", fill, in, nb );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* --- DEMA: same three surfaces --- */
   rc = TA_DEMA_Open( &ds, in, warm, 1, &v );
   errNb = pbCheckStreamCopy( "DEMA(1) stream Open", rc, v, in[warm-1] );
   if( errNb != TA_TEST_PASS ) { TA_DEMA_Close( ds ); return errNb; }
   for( i = warm; i < n; i++ )
   {
      rc = TA_DEMA_Update( ds, in[i], &v );
      errNb = pbCheckStreamCopy( "DEMA(1) stream Update", rc, v, in[i] );
      if( errNb != TA_TEST_PASS ) { TA_DEMA_Close( ds ); return errNb; }
   }
   TA_DEMA_Close( ds );

   rc = TA_DEMA_OpenAndFill( &ds, in, n, 1, &beg, &nb, fill );
   if( rc != TA_SUCCESS || beg != 0 || nb != n )
   {
      printf( "\nFail: DEMA(1) OpenAndFill: rc=%d beg=%d nb=%d expected 0/0/%d\n",
              (int)rc, (int)beg, (int)nb, n );
      TA_DEMA_Close( ds );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   TA_DEMA_Close( ds );
   errNb = pbCheckSameSeries( "DEMA(1) OpenAndFill", fill, in, nb );
   if( errNb != TA_TEST_PASS ) return errNb;

   printf( "\n  Period-1 streaming copy: EMA and DEMA, Open/Peek/Update/OpenAndFill "
           "over %d bars of the Sterbenz-hostile series", n );
   return TA_TEST_PASS;
}

static ErrorNumber testEveryMovingAverageIdentity( const TA_History *history )
{
   TA_History hostile;
   ErrorNumber errNb;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   errNb = pbSweepMaIdentity( history, "reference series" );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = pbFillRoundTripHostileHistory( &hostile );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = pbSweepMaIdentity( &hostile, "round-trip-hostile series" );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = pbFillSterbenzHostileHistory( &hostile );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = pbSweepMaIdentity( &hostile, "Sterbenz-hostile series" );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = testStreamIdentityAtPeriodOne( &hostile );
   if( errNb != TA_TEST_PASS )
      return errNb;

   printf( "\n" );
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
   int m;
   const TA_IntegerList *maTypes;

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

   /* MACDEXT with signalPeriod=1 for every signal MAType (incl. DISABLED and DEFAULT). */
   maTypes = pbMaTypeList();
   if( !maTypes )
   {
      printf( "\nFail: cannot read MA's optInMAType choice list\n" );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   for( m = 0; m < (int)maTypes->nbElement; m++ )
   {
      char label[64];
      int maType = maTypes->data[m].value;
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

   errNb = testMacdSignalOneHostile();
   if( errNb != TA_TEST_PASS )
      return errNb;

   return TA_TEST_PASS;
}

/* The signal=1 contract on inputs the signal recursion cannot round-trip.
 *
 * "signal == MACD line, histogram == 0" is asserted bit-exactly above, but the
 * reference close series can never break it: at a signal period of 1 the EMA
 * factor is exactly 1.0, so the signal step is (x - prev) + prev, which returns
 * x only while consecutive MACD-LINE values stay within a factor of two of each
 * other. The MACD line is a difference of two EMAs decaying toward zero inside
 * a flat run, so it leaves that window on inputs a price series never does --
 * which is why this defect outlived the sub-test above.
 *
 * A GRID, not one series, and deliberately so: the divergence is delicate (a
 * few bars out of 226) and each function has its own MACD line -- MACDFIX's
 * fixed 0.15/0.075 factors put its line somewhere else entirely, so the series
 * that breaks MACD leaves MACDFIX untouched. That is not hypothetical; the
 * first version of this leg hardcoded one series and its own self-check caught
 * it being vacuous for MACDFIX. Sweeping run lengths x levels means no single
 * shape going benign can disarm the gate.
 *
 * Hostility is asserted from the RETURNED MACD line, replaying the naive step
 * over it, accumulated per function across the whole grid. The fix does not
 * touch the MACD line, so the assertion keeps its meaning afterwards.
 */
#define MH_N        252
#define MH_MIN_NAIVE  1   /* per function, summed over the grid */

static const int    pbMacdRuns[]   = { 24, 30, 36, 39, 45 };
static const TA_Real pbMacdLo[]    = { 470.3574516572389, 0.87 };
static const TA_Real pbMacdHi[]    = { 6515.901813836415, 913.71 };

/* Assert the contract, and return how many bars the naive step would have
 * lost (the caller accumulates it as the non-vacuity evidence). */
static ErrorNumber pbCheckMacdSignalIsLine( const char *label,
                                            const TA_Real *macd,
                                            const TA_Real *signal,
                                            const TA_Real *hist,
                                            int nb, int *nbNaive )
{
   int i;
   TA_Real prev;

   prev = macd[0];
   for( i = 1; i < nb; i++ )
   {
      prev = ((macd[i] - prev) * 1.0) + prev;
      if( prev != macd[i] )
         (*nbNaive)++;
   }

   for( i = 0; i < nb; i++ )
   {
      if( signal[i] != macd[i] )
      {
         printf( "\nFail: %s: [%d] signal %.17g != MACD line %.17g\n",
                 label, i, signal[i], macd[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      if( hist[i] != 0.0 )
      {
         printf( "\nFail: %s: [%d] hist %.17g, expected 0\n", label, i, hist[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }
   return TA_TEST_PASS;
}

static ErrorNumber testMacdSignalOneHostile( void )
{
   static TA_Real in[MH_N];
   static TA_Real outM[MH_N], outS[MH_N], outH[MH_N];
   /* MACDEXT is the control: it reaches the same contract through ma()'s
    * period-1 copy, not an EMA recursion (its all-EMA fast path requires
    * signal >= 2), so it must already hold everywhere on this grid. */
   static const char * const name[3] = { "MACD(12,26,1)", "MACDFIX(1)",
                                         "MACDEXT(sig=1,EMA) [control]" };
   int nbNaive[3] = { 0, 0, 0 };
   TA_Integer beg, nb;
   TA_RetCode rc;
   ErrorNumber errNb;
   unsigned int r, k;
   int i, w;
   char label[96];

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   for( r = 0; r < sizeof(pbMacdRuns)/sizeof(pbMacdRuns[0]); r++ )
   {
      for( k = 0; k < sizeof(pbMacdLo)/sizeof(pbMacdLo[0]); k++ )
      {
         for( i = 0; i < MH_N; i++ )
            in[i] = ((i / pbMacdRuns[r]) % 2) ? pbMacdLo[k] : pbMacdHi[k];

         for( w = 0; w < 3; w++ )
         {
            switch( w )
            {
            case 0:
               rc = TA_MACD( 0, MH_N-1, in, 12, 26, 1, &beg, &nb, outM, outS, outH );
               break;
            case 1:
               rc = TA_MACDFIX( 0, MH_N-1, in, 1, &beg, &nb, outM, outS, outH );
               break;
            default:
               rc = TA_MACDEXT( 0, MH_N-1, in, 12, TA_MAType_EMA, 26, TA_MAType_EMA,
                                1, TA_MAType_EMA, &beg, &nb, outM, outS, outH );
               break;
            }
            snprintf( label, sizeof(label), "%s run=%d lvl=%u",
                      name[w], pbMacdRuns[r], k );
            PB_CHECK_RC( label, rc, TA_SUCCESS );
            errNb = pbCheckMacdSignalIsLine( label, outM, outS, outH, nb, &nbNaive[w] );
            if( errNb != TA_TEST_PASS )
               return errNb;
         }
      }
   }

   for( w = 0; w < 3; w++ )
   {
      if( nbNaive[w] < MH_MIN_NAIVE )
      {
         printf( "\nFail: %s: the signal=1 grid no longer discriminates -- the "
                 "naive step round-trips its MACD line on every bar of all %u "
                 "shapes (need at least %d)\n", name[w],
                 (unsigned)(sizeof(pbMacdRuns)/sizeof(pbMacdRuns[0]) *
                            sizeof(pbMacdLo)/sizeof(pbMacdLo[0])), MH_MIN_NAIVE );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   printf( "\n  MACD signal=1: contract held on %u shapes x 3 functions; the "
           "naive step would have lost %d/%d/%d bars\n",
           (unsigned)(sizeof(pbMacdRuns)/sizeof(pbMacdRuns[0]) *
                      sizeof(pbMacdLo)/sizeof(pbMacdLo[0])),
           nbNaive[0], nbNaive[1], nbNaive[2] );

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

/* Build a full optional-parameter vector: every param at its ta_abstract default
 * except `paramNb`, forced to the value the case under test set on paramHolder.
 * Shared by the empty-output server_verify leg and the lookback-parity leg
 * below -- both need the SAME vector paramHolder was actually set to. */
static void pbBuildSweptOptVector( const TA_FuncHandle *handle,
                                   unsigned int nbOptInput,
                                   unsigned int paramNb, int isReal,
                                   int ivalue, TA_Real dvalue,
                                   double svOpt[PB_MAX_OPT] )
{
   unsigned int j;
   for( j = 0; j < nbOptInput; j++ )
   {
      const TA_OptInputParameterInfo *oi;
      TA_GetOptInputParameterInfo( handle, j, &oi );
      svOpt[j] = oi->defaultValue;
   }
   svOpt[paramNb] = isReal ? dvalue : (double)ivalue;
}

/* Build the NULL-terminated inputs[] array server_verify expects, mirroring the
 * paramHolder wiring below: a Price input expands to its used OHLCV+OI
 * components in that fixed order (identical to server_verify's PRICE_COMPONENTS),
 * and each Real input is the close series (MAVP's periods array too — matching
 * the TA_SetInputParamRealPtr calls). Returns the pointer count (excluding the
 * NULL terminator), or -1 if it would exceed maxInputs. */
static int pbBuildServerInputs( const TA_FuncInfo *funcInfo,
                                const TA_History *history,
                                const TA_Real *inputs[], int maxInputs )
{
   const TA_FuncHandle *handle = funcInfo->handle;
   const TA_InputParameterInfo *inputInfo;
   const struct { unsigned int flag; const TA_Real *data; } price[] = {
      { TA_IN_PRICE_OPEN,         history->open },
      { TA_IN_PRICE_HIGH,         history->high },
      { TA_IN_PRICE_LOW,          history->low },
      { TA_IN_PRICE_CLOSE,        history->close },
      { TA_IN_PRICE_VOLUME,       history->volume },
      { TA_IN_PRICE_OPENINTEREST, history->openInterest },
   };
   unsigned int i;
   int n = 0, c;

   for( i = 0; i < funcInfo->nbInput; i++ )
   {
      TA_GetInputParameterInfo( handle, i, &inputInfo );
      switch( inputInfo->type )
      {
      case TA_Input_Price:
         for( c = 0; c < 6; c++ )
            if( inputInfo->flags & price[c].flag )
            {
               if( n >= maxInputs - 1 ) return -1;
               inputs[n++] = price[c].data;
            }
         break;
      case TA_Input_Real:
      case TA_Input_Integer:   /* no integer-input function today; close is fine */
         if( n >= maxInputs - 1 ) return -1;
         inputs[n++] = history->close;
         break;
      }
   }
   inputs[n] = NULL;
   return n;
}

/* Run one sweep case: optional parameter `paramNb` set to `ivalue` (integer
 * params) or `dvalue` (when isReal), every other parameter left at its
 * default. `expect` is PB_EXPECT_STRICT (must succeed, coherent+finite),
 * PB_EXPECT_LENIENT (coherent OR a clean TA_BAD_PARAM), or PB_EXPECT_REJECT
 * (must return TA_BAD_PARAM). A successful call is coherent when the output
 * either fully spans lookback..last-bar (lookback <= endIdx) or is empty (the
 * period consumes the whole range); a crash, an out-of-bounds access, a
 * garbage/non-finite value, or any other retCode is always a failure.
 * Independent of `expect`, TA_GetLookback must report a negative value exactly
 * when the call returns TA_BAD_PARAM -- the two tiers have to agree about
 * whether the parameters are usable, whatever that answer is.
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

   /* The lookback tier and the call tier must agree about whether the bound
    * parameters are usable at all. TA_GetLookback reports its verdict in the
    * value (-1 = rejected), TA_CallFunc in its return code, and a caller sizes
    * its buffers from the first before trusting the second -- so a lookback that
    * answers a plausible number for parameters the call will reject is a lie a
    * wrapper cannot detect.
    *
    * Asserted for EVERY swept case, not just the out-of-range ones: the
    * interesting failures are values each tier considers in range on its own.
    * For a generated range check the two cannot disagree (one emitter, two
    * failure literals) -- what this reaches is the hand-written decisions in
    * ta_codegen/input: a switch default, or a cross-parameter constraint the
    * lookback never re-checks. */
   if( TA_GetLookback( paramHolder, &lookback ) != TA_SUCCESS )
   {
      printf( "\nFail: %s: TA_GetLookback failed\n", label );
      pbFail( ctx );
      TA_ParamHolderFree( paramHolder );
      return;
   }
   retCode = TA_CallFunc( paramHolder, 0, endIdx, &outBegIdx, &outNbElement );

   ctx->nbLookbackParity++;
   if( ( lookback < 0 ) != ( retCode == TA_BAD_PARAM ) )
   {
      printf( "\nFail: %s: lookback/call disagree -- lookback=%d, call retCode=%d.\n"
              "      One tier accepts these parameters and the other rejects them;\n"
              "      a caller sizes its buffers from the lookback.\n",
              label, lookback, retCode );
      pbFail( ctx );
      TA_ParamHolderFree( paramHolder );
      return;
   }

   /* Same rule, one server at a time (issue #256): does THIS server's own
    * lookback tier agree with THIS server's own batch tier for the SAME
    * vector? Independent of what C or any other server says -- server_verify()
    * deliberately skips reject cases across languages (parameter validation is
    * implementation-specific), but a language disagreeing with ITSELF is
    * always a bug. Runs for every swept case, matching the in-process check
    * above -- including PB_EXPECT_REJECT, which server_verify() never reaches
    * (its early call-tier rejection returns before server_verify would fire). */
   if( server_verify_active() && funcInfo->nbOptInput <= PB_MAX_OPT )
   {
      const TA_Real *svInputs[PB_MAX_INPUT];
      double         svOpt[PB_MAX_OPT];
      ErrorNumber    svErr;

      if( pbBuildServerInputs( funcInfo, history, svInputs, PB_MAX_INPUT ) < 0 )
      {
         printf( "\nFail: %s: too many inputs for lookback-parity server verify\n", label );
         pbFail( ctx );
         TA_ParamHolderFree( paramHolder );
         return;
      }
      pbBuildSweptOptVector( handle, funcInfo->nbOptInput, paramNb, isReal,
                             ivalue, dvalue, svOpt );

      svErr = server_verify_lookback_parity( funcInfo->name, 0, endIdx,
                                             (int)history->nbBars, svInputs,
                                             svOpt, (int)funcInfo->nbOptInput );
      if( svErr != TA_TEST_PASS )
      {
         printf( "Fail: %s: lookback-parity server verification\n", label );
         pbFail( ctx );
         TA_ParamHolderFree( paramHolder );
         return;
      }
      ctx->nbLookbackParityServer++;
   }

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

      /* #142 deferred: extend the empty-output (period > input-length) contract
       * to the language servers. The generic --codegen / --xlang-hash sweeps keep
       * every lookback < nbBars (compute_large_int clamps to nbBars-5), so this
       * is the one boundary they never cross-check. Each server must likewise
       * return TA_SUCCESS with a zero-length output at the same outBegIdx. */
      if( server_verify_active() && funcInfo->nbOptInput <= PB_MAX_OPT )
      {
         const TA_Real     *svInputs[PB_MAX_INPUT];
         const TA_Real     *svOutReal[PB_MAX_OUTPUT + 1];
         const TA_Integer  *svOutInt[PB_MAX_OUTPUT + 1];
         double             svOpt[PB_MAX_OPT];
         const TA_OutputParameterInfo *oinfo;
         unsigned int j;
         int nReal = 0, nInt = 0;
         ErrorNumber svErr;

         if( pbBuildServerInputs( funcInfo, history, svInputs, PB_MAX_INPUT ) < 0 )
         {
            printf( "\nFail: %s: too many inputs for server verify\n", label );
            pbFail( ctx );
            TA_ParamHolderFree( paramHolder );
            return;
         }

         /* Full optional-parameter vector: every parameter at the default the
          * paramHolder used, the swept one at its probed value. */
         pbBuildSweptOptVector( handle, funcInfo->nbOptInput, paramNb, isReal,
                                ivalue, dvalue, svOpt );

         for( j = 0; j < funcInfo->nbOutput && j < PB_MAX_OUTPUT; j++ )
         {
            TA_GetOutputParameterInfo( handle, j, &oinfo );
            if( oinfo->type == TA_Output_Integer )
               svOutInt[nInt++] = &pbSweepOutInt[j][0];
            else
               svOutReal[nReal++] = &pbSweepOutReal[j][0];
         }
         svOutReal[nReal] = NULL;
         svOutInt[nInt]   = NULL;

         svErr = server_verify( funcInfo->name, 0, endIdx, (int)history->nbBars,
                                retCode, outBegIdx, outNbElement,
                                svInputs, svOpt, (int)funcInfo->nbOptInput,
                                nReal ? svOutReal : NULL,
                                nInt  ? svOutInt  : NULL );
         if( svErr != TA_TEST_PASS )
         {
            printf( "Fail: %s: server verification (empty-output contract)\n", label );
            pbFail( ctx );
            TA_ParamHolderFree( paramHolder );
            return;
         }
         ctx->nbServerEmpty++;
      }
   }

   TA_ParamHolderFree( paramHolder );
}

/* Boundary grid for one function: every optional parameter is swept across
 * min / min+1 / default-1 / default / default+1 / a large "past the data"
 * period (integer ranges), across every enumerated value plus one past the
 * highest (integer/real lists), and across min / default / max (real ranges —
 * the library does not range-check reals, so those exercise the finite scan,
 * not BAD_PARAM). The
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
         int maxValue = 0;
         for( e = 0; e < list->nbElement; e++ )
         {
            if( list->data[e].value > maxValue ) maxValue = list->data[e].value;
            pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 0,
                            list->data[e].value, 0.0, PB_EXPECT_STRICT );
            if( ctx->errNb != TA_TEST_PASS ) return;
         }
         /* One value past the end of the list. A choice list declares no
          * `range:`, so until the prologue learned to derive one from the
          * members nothing rejected this before each body's own dispatch --
          * which is how a lookback came to answer 0 for a call that rejects.
          * Now both tiers reject it from the one emitter, and this is what
          * pins that. Derived from the list rather than spelled here, so an
          * appended member cannot turn it into a valid value. */
         pbSweepRunCase( ctx, funcInfo, paramNb, optInfo, 0,
                         maxValue + 1, 0.0, PB_EXPECT_REJECT );
         if( ctx->errNb != TA_TEST_PASS ) return;
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
   ctx.nbLookbackParity = 0;
   ctx.nbLookbackParityServer = 0;
   ctx.nbServerEmpty = 0;
   g_pbListAll = ( getenv( "PB_SWEEP_LIST_ALL" ) != NULL );

   TA_ForEachFunc( pbSweepOneFunction, &ctx );

   if( g_pbListAll )
      printf( "\n  boundary sweep: %d cases, %d failure(s)\n",
              ctx.nbParamTested, ctx.nbFail );

   if( ctx.errNb != TA_TEST_PASS )
      return ctx.errNb;

   printf( "\n  Lookback/call parity: %d case(s) compared over %d swept parameter value(s)\n",
           ctx.nbLookbackParity, ctx.nbParamTested );

   if( ctx.nbParamTested == 0 )
   {
      printf( "\nFail: boundary sweep tested no parameter (enumeration broken?)\n" );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* Every swept case is compared, so this is an equality rather than a
    * non-zero floor: a case that silently stopped being compared would look
    * exactly like a pass. */
   if( ctx.nbLookbackParity != ctx.nbParamTested )
   {
      printf( "\nFail: boundary sweep compared %d lookback(s) for %d swept value(s)\n",
              ctx.nbLookbackParity, ctx.nbParamTested );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* Same-server lookback/batch parity (#256), against every active language
    * server. Every swept case must reach it, same equality as the in-process
    * check above -- a case that silently stopped being compared would look
    * exactly like a pass. */
   if( server_verify_active() )
   {
      if( ctx.nbLookbackParityServer != ctx.nbParamTested )
      {
         printf( "\nFail: boundary sweep server-compared %d lookback(s) for %d swept "
                 "value(s)\n", ctx.nbLookbackParityServer, ctx.nbParamTested );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      printf( "  boundary sweep: %d lookback/batch case(s) compared per server (#256)\n",
              ctx.nbLookbackParityServer );
   }

   /* The period > input-length empty-output contract must be exercised against
    * the servers non-vacuously when they are up (#142 deferred follow-up). */
   if( server_verify_active() )
   {
      if( ctx.nbServerEmpty == 0 )
      {
         printf( "\nFail: boundary sweep cross-checked no empty-output case vs the "
                 "servers (period>input contract vacuous)\n" );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      printf( "  boundary sweep: %d empty-output case(s) cross-checked vs servers\n",
              ctx.nbServerEmpty );
   }

   return TA_TEST_PASS;
}

/* #142 regression: the linear-regression family computed
 *   SumXSqr = optInTimePeriod*(optInTimePeriod-1)*(2*optInTimePeriod-1)/6
 * in int32, overflowing (signed-integer-overflow UB) at period >= 1025. This
 * probe runs the whole family at that supra-threshold period on a tiny (~16 KB)
 * linear ramp -- a case no prior test reached (reference data tops out at 2760
 * bars, always at small periods; period=max+1 was only reject-tested, never
 * computed).
 *
 * Where the teeth are: the --sanitize (UBSan) build the nightly runs. The int32
 * product traps there (-fno-sanitize-recover aborts) while the widened double
 * form is clean, so a revert to the int expression fails the sanitized run on
 * these very calls. In a plain -O3 release build the value check below is
 * effectively VACUOUS -- the optimizer, entitled to assume no signed overflow,
 * folds the polynomial to the correct value either way -- so a perfect ramp's
 * closed-form output (least-squares fit is exact) is only a cheap coherence
 * guard here. The source-form regression guard lives in the generator's
 * backend_suite.rs (test_period_scaled_arithmetic_is_double_not_int32).
 */
#define PB_LR_PROBE_PERIOD 1025
#define PB_LR_PROBE_N      1030   /* a few outputs past the lookback */

/* Which closed-form value each LR-family output must equal at global bar
 * `today`, for a ramp value[i] = base + step*i (window position x in
 * 0..period-1, x=0 the oldest bar; b = intercept at x=0, m = slope). */
enum { PB_LR_REG = 0, PB_LR_SLOPE, PB_LR_INTERCEPT, PB_LR_TSF, PB_LR_ANGLE };

static ErrorNumber pbLrExpect( const char *label, const TA_Real *out,
                               TA_Integer begIdx, TA_Integer nbElement,
                               double base, double step, int period, int kind )
{
   TA_Integer i;
   TA_Integer expectedNb = PB_LR_PROBE_N - ( period - 1 );

   if( nbElement != expectedNb )
   {
      printf( "\nFail: %s: outNBElement %d, expected %d (period=%d, n=%d)\n",
              label, (int)nbElement, (int)expectedNb, period, PB_LR_PROBE_N );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   for( i = 0; i < nbElement; i++ )
   {
      TA_Integer today = begIdx + i;
      double want;
      switch( kind )
      {
      case PB_LR_REG:       want = base + step * (double)today;                    break;
      case PB_LR_SLOPE:     want = step;                                           break;
      case PB_LR_INTERCEPT: want = base + step * (double)( today - period + 1 );   break;
      case PB_LR_TSF:       want = base + step * (double)( today + 1 );            break;
      default:              want = atan( step ) * ( 180.0 / 3.14159265358979323846 ); break;
      }
      if( fabs( out[i] - want ) > 1e-6 * ( fabs( want ) + 1.0 ) )
      {
         printf( "\nFail: %s [%d]: got %.17g, expected %.17g (#142 int32 overflow at period %d)\n",
                 label, (int)today, out[i], want, period );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }
   return TA_TEST_PASS;
}

static ErrorNumber testLinearRegRampOverflowProbe( void )
{
   static TA_Real ramp[PB_LR_PROBE_N];
   static TA_Real out[PB_LR_PROBE_N];
   const double base = 100.0;
   const double step = 0.25;
   const int period = PB_LR_PROBE_PERIOD;
   TA_Integer i, begIdx, nbElement;
   TA_RetCode rc;
   ErrorNumber errNb;

   for( i = 0; i < PB_LR_PROBE_N; i++ )
      ramp[i] = base + step * (double)i;

   #define PB_LR_RUN( FN, LABEL, KIND )                                              \
      do {                                                                           \
         rc = FN( 0, PB_LR_PROBE_N - 1, ramp, period, &begIdx, &nbElement, out );    \
         if( rc != TA_SUCCESS )                                                      \
         {                                                                           \
            printf( "\nFail: %s: retCode %d (period=%d)\n", LABEL, (int)rc, period );\
            return TA_REGTEST_OPTIMIZATION_REF_ERROR;                                \
         }                                                                           \
         errNb = pbLrExpect( LABEL, out, begIdx, nbElement, base, step, period, KIND );\
         if( errNb != TA_TEST_PASS )                                                 \
            return errNb;                                                            \
      } while( 0 )

   PB_LR_RUN( TA_LINEARREG,           "LINEARREG@1025",           PB_LR_REG );
   PB_LR_RUN( TA_LINEARREG_SLOPE,     "LINEARREG_SLOPE@1025",     PB_LR_SLOPE );
   PB_LR_RUN( TA_LINEARREG_INTERCEPT, "LINEARREG_INTERCEPT@1025", PB_LR_INTERCEPT );
   PB_LR_RUN( TA_TSF,                 "TSF@1025",                 PB_LR_TSF );
   PB_LR_RUN( TA_LINEARREG_ANGLE,     "LINEARREG_ANGLE@1025",     PB_LR_ANGLE );

   #undef PB_LR_RUN

   return TA_TEST_PASS;
}
