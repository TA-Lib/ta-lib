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
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  071726 MF,CC  First version. Close+volume indicators (NVI/PVI, #126).
 *  090426 MF,CC  Add PVT (Price Volume Trend, #364).
 */

/* Description:
 *
 *     Test functions whose inputs are (close, volume) with one real output
 *     and no optional parameter: NVI, PVI (issue #126) and PVT (issue #364).
 *
 *     NVI and PVI are cumulative volume indices seeded at 1000 and updated by
 *     the bar's percentage price change only when volume fell (NVI) / rose
 *     (PVI) versus the previous bar. PVT sums, rather than compounds, the same
 *     percentage change scaled by the bar's volume, from a seed of zero. All
 *     three anchor their level at startIdx, like OBV/AD, and are therefore
 *     path-dependent across ranges -> the range sweep uses TA_DO_NOT_COMPARE.
 *
 *     FORMULA-CORRECTNESS is anchored by the hard-coded expected values below.
 *     These are golden numbers produced by an INDEPENDENT implementation:
 *     Tulip Indicators 0.9.2 (ti_nvi / ti_pvi), which were additionally verified
 *     to agree bit-for-bit with a from-scratch pure-Python textbook reference on
 *     this same 252-bar series. This proves the formula is right; the generic
 *     cross-language gate (--codegen / --xlang-hash) separately proves every
 *     backend reproduces it bit-for-bit.
 *
 *     Tulip has no PVT, so PVT's own goldens come from three other
 *     implementations and carry their own leg below (test_pvt_legs).
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
TA_PVT_TEST,
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
static ErrorNumber test_pvt_legs( const TA_History *history );

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

   /*************/
   /* PVT TEST  */
   /*************/
   /* Provenance: pvtOracle below. CHECK_EXPECTED_VALUE compares with
    * TA_REAL_EQ's 0.01 ABSOLUTE window, which on these magnitudes is 1e-9 to
    * 4e-8 relative -- do not carry over the NVI/PVI habit of reading it as
    * tight, their values are ~1000. */
   { 1, TA_PVT_TEST, 0, 251, TA_SUCCESS,   0,       0.0,             0, 252 }, /* First Value (seed) */
   { 0, TA_PVT_TEST, 0, 251, TA_SUCCESS,   1,  179549.81967213101,   0, 252 },
   { 0, TA_PVT_TEST, 0, 251, TA_SUCCESS,  50, -1088931.9928736195,   0, 252 },
   { 0, TA_PVT_TEST, 0, 251, TA_SUCCESS, 125,  4062669.8867437406,   0, 252 },
   { 0, TA_PVT_TEST, 0, 251, TA_SUCCESS, 200,  1131316.760423244,    0, 252 },
   { 0, TA_PVT_TEST, 0, 251, TA_SUCCESS, 251, -6045790.0001211911,   0, 252 }, /* Last Value */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

#define PVT_CAP      300
#define PVT_EDGE_NB   30

/* Relative only, and headroom rather than a measured gap: every row below is
 * BIT-IDENTICAL to ours here, and PVT touches no libm, so a conforming double
 * with -ffp-contract=off has nowhere to differ. The frozen rows bottom out at
 * 1.6e5, so an absolute companion would never be the binding term. */
#define PVT_ORACLE_REL 1e-12

typedef struct { int bar; double want; } PvtGolden;

/* Captured by ta-lib-oracles/capture_364_pvt.py on the 252-bar TA_SREF
 * close/volume series, at %.17g, which round-trips to the same double. `bar` is
 * the ABSOLUTE bar index; the output index is bar - begIdx.
 *
 * THREE independent implementations, each driven on this exact series
 * (2026-09-04):
 *   1. PyneCore 6.8.14 `ta.pvt` -- Python, a from-scratch runtime for the Pine
 *      language. Its `cum` is a plain running sum, not Pine's compensated
 *      math.sum, and its operand order is ours. Corroborates "this is what Pine
 *      SPECIFIES", never "this is what TradingView computes".
 *   2. trading-signals 8.3.0 `ts.PVT` -- TypeScript. Commutes the multiply
 *      (`volume * ratio`), which IEEE multiplication makes immaterial, and
 *      carries the total forward on the same exact `previousClose === 0` test.
 *   3. pandas-ta-classic 0.6.52 `ta.pvt` (pandas 3.0.3, numpy 2.5.1) -- Python.
 *      NOT in the table: it is x100 AND reassociated ((100*(c-p))/p), so
 *      dividing by 100 cannot recover the bits. It agrees to 1.27e-15 max
 *      relative over all 251 values, which corroborates the FORMULA while (1)
 *      and (2) pin the arithmetic.
 * (1) and (2) agree with each other and with TA_PVT bit-for-bit over all 251
 * values -- 0 disagreements; the capture script refuses to print a table when
 * that count is non-zero.
 *
 * Tulip Indicators 0.9.2 has no PVT/VPT (indicators/ holds nvi.c, obv.c, pvi.c)
 * and ta4j 0.24.1 has none either, so neither could be an arm here.
 *
 * All three oracles leave bar 0 undefined (NaN / null / NaN) where TA_PVT
 * materialises the 0.0 seed that issue #112 requires; the table starts at bar 1
 * and the seed is pinned by tableTest and by the edge leg instead. */
static const PvtGolden pvtOracle[] =
{
   {   1,        179549.81967213101 },
   {   2,        157389.48639153206 },
   {   3,        189090.84798093597 },
   {  10,        224295.51503477019 },
   {  25,       -1795649.4181918642 },
   {  50,       -1088931.9928736195 },
   {  75,        1104218.0731603324 },
   { 100,        3174604.1838120408 },
   { 125,        4062669.8867437406 },
   { 150,        3209185.5786155956 },
   { 175,        3722558.9281922411 },
   { 200,         1131316.760423244 },
   { 225,       -6196402.8455754165 },
   { 250,       -6022562.0460982025 },
   { 251,       -6045790.0001211911 },
};
#define NB_PVT_ORACLE ((int)(sizeof(pvtOracle)/sizeof(PvtGolden)))

/* The PVT leg is silent on success, so a count that reached zero is the only
 * remaining way it could run while comparing nothing. */
static int g_pvtOracleCmp;
static int g_pvtDiffCmp;
static int g_pvtEdgeCmp;

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

   retValue = test_pvt_legs( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

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
   case TA_PVT_TEST:
      return TA_PVT( startIdx, endIdx, inClose, inVolume,
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
   case TA_PVT_TEST: *lookback = TA_PVT_Lookback(); break;
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
      switch( test->theFunction )
      {
      case TA_NVI_TEST: funcName = "NVI"; break;
      case TA_PVI_TEST: funcName = "PVI"; break;
      case TA_PVT_TEST: funcName = "PVT"; break;
      /* Naming the wrong function here does not fail: it silently compares this
       * function's C output against a DIFFERENT one in every other language. */
      default:          return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
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
                           TA_TEST_UNST_NONE,
                           (void *)&testParam, 1,
                           TA_DO_NOT_COMPARE );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

/* PVT (issue #364). Three legs the 6-field tableTest row cannot express.
 *
 *   (a) The frozen goldens above, plus the cross-language replay.
 *   (b) A BIT-EXACT differential against TA_ROCP(close,1). PVT's per-bar term
 *       is exactly that function's output -- same operands, same order, same
 *       exact `!= 0.0` guard -- so accumulating rocp[j]*volume[j] must
 *       reproduce TA_PVT with no tolerance at all. This is what pins the
 *       operation ORDER: writing the term as ((c-p)*v)/p instead would still
 *       satisfy every value comparison above at 1e-12 and fail here.
 *       ROCP's lookback is 1, so its first output is bar 1 and PVT's own seed
 *       bar has no counterpart to compare.
 *   (c) Exact-arithmetic edges no oracle corpus reaches: the
 *       zero-previous-close guard, an all-zero volume, a flat close, and a
 *       single-bar range. Every expectation here is an equality.
 */
static ErrorNumber test_pvt_legs( const TA_History *history )
{
   static TA_Real out[PVT_CAP], rocp[PVT_CAP], scratch[PVT_EDGE_NB];
   static TA_Real zeros[PVT_EDGE_NB], flat[PVT_EDGE_NB];
   static const TA_Real guardClose[4]  = { 0.0, 0.0, 2.0, 4.0 };
   static const TA_Real guardVolume[4] = { 10.0, 20.0, 30.0, 40.0 };
   static const TA_Real guardWant[4]   = { 0.0, 0.0, 0.0, 40.0 };
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begRocp, nbRocp;
   int nbBars = (int)history->nbBars;
   int i, k;
   double acc;

   g_pvtOracleCmp = g_pvtDiffCmp = g_pvtEdgeCmp = 0;

   /* The edge legs below read PVT_EDGE_NB bars of the corpus. */
   if( nbBars < PVT_EDGE_NB || nbBars > PVT_CAP )
   {
      printf( "PVT Fail: corpus is %d bars, outside [%d,%d]\n",
              nbBars, PVT_EDGE_NB, PVT_CAP );
      return TA_PVT_VACUOUS;
   }

   for( i = 0; i < PVT_EDGE_NB; i++ )
   {
      zeros[i] = 0.0;
      flat[i]  = 5.0;
   }

   retCode = TA_PVT( 0, nbBars-1, history->close, history->volume,
                     &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != nbBars )
   {
      printf( "PVT Fail: rc=%d (%d,%d) expected (0,%d)\n",
              (int)retCode, begIdx, nbElement, nbBars );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   if( server_verify_active() )
   {
      ErrorNumber e;
      int cmpBefore = server_verify_comparisons();

      e = server_verify( "PVT", 0, nbBars-1, nbBars,
                         retCode, begIdx, nbElement,
                         (const TA_Real*[]){ history->close, history->volume, NULL },
                         NULL, 0,
                         (const TA_Real*[]){ out, NULL }, NULL );
      if( e != TA_TEST_PASS )
         return e;
      /* "No failure reported" and "nothing was compared" are the same
       * observation without this. */
      if( server_verify_comparisons() == cmpBefore )
      {
         printf( "PVT: compared no server despite live pipes\n" );
         return TA_PVT_VACUOUS;
      }
   }

   /* (a) */
   if( nbBars == 252 )
   {
      for( k = 0; k < NB_PVT_ORACLE; k++ )
      {
         double err;
         const char *mode;

         /* Each row's bar is hand-transcribed from the capture script, and the
          * index below is unchecked arithmetic on it: a bar past the end is a
          * silent out-of-bounds read, not a mismatch. */
         if( pvtOracle[k].bar < begIdx || pvtOracle[k].bar - begIdx >= nbElement )
         {
            printf( "PVT oracle Fail: golden bar %d is outside the output "
                    "[%d..%d]\n", pvtOracle[k].bar, begIdx,
                    begIdx + nbElement - 1 );
            return TA_PVT_VACUOUS;
         }

         g_pvtOracleCmp++;
         if( !checkOracleValue( out[pvtOracle[k].bar - begIdx], pvtOracle[k].want,
                                PVT_ORACLE_REL, 0.0, &err, &mode ) )
         {
            printf( "PVT oracle Fail at bar %d: got %.17g expected %.17g "
                    "(%s err %.3g, tol rel %g)\n",
                    pvtOracle[k].bar, out[pvtOracle[k].bar - begIdx],
                    pvtOracle[k].want, mode, err, PVT_ORACLE_REL );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   else
   {
      printf( "PVT oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
   }

   /* (b) */
   retCode = TA_ROCP( 0, nbBars-1, history->close, 1, &begRocp, &nbRocp, rocp );
   if( retCode != TA_SUCCESS || begRocp != 1 || nbRocp != nbBars-1 )
   {
      printf( "PVT differential Fail: TA_ROCP rc=%d (%d,%d) expected (1,%d)\n",
              (int)retCode, begRocp, nbRocp, nbBars-1 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   acc = 0.0;
   for( i = begRocp; i < nbBars; i++ )
   {
      acc += rocp[i-begRocp] * history->volume[i];
      g_pvtDiffCmp++;
      if( acc != out[i-begIdx] )
      {
         printf( "PVT differential Fail at bar %d: TA_PVT %.17g, the TA_ROCP "
                 "accumulation %.17g -- these must be bit-identical\n",
                 i, out[i-begIdx], acc );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* (c) The zero-previous-close guard: bar 3 is the first with a non-zero
    * previous close, and its term is exactly (4-2)/2 * 40. */
   retCode = TA_PVT( 0, 3, guardClose, guardVolume, &begIdx, &nbElement, scratch );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 4 )
   {
      printf( "PVT edge Fail [zero close]: rc=%d (%d,%d) expected (0,4)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < 4; i++ )
   {
      g_pvtEdgeCmp++;
      if( scratch[i] != guardWant[i] )
      {
         printf( "PVT edge Fail [zero close] at %d: got %.17g expected %.17g\n",
                 i, scratch[i], guardWant[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* An all-zero volume and a flat close each make every term exactly 0.0. */
   for( k = 0; k < 2; k++ )
   {
      const TA_Real *edgeClose, *edgeVolume;

      edgeClose  = ( k == 0 ) ? history->close : flat;
      edgeVolume = ( k == 0 ) ? zeros          : history->volume;

      retCode = TA_PVT( 0, PVT_EDGE_NB-1, edgeClose, edgeVolume,
                        &begIdx, &nbElement, scratch );
      if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != PVT_EDGE_NB )
      {
         printf( "PVT edge Fail [%s]: rc=%d (%d,%d) expected (0,%d)\n",
                 (k == 0) ? "zero volume" : "flat close",
                 (int)retCode, begIdx, nbElement, PVT_EDGE_NB );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < PVT_EDGE_NB; i++ )
      {
         g_pvtEdgeCmp++;
         if( scratch[i] != 0.0 )
         {
            printf( "PVT edge Fail [%s] at %d: got %.17g expected 0\n",
                    (k == 0) ? "zero volume" : "flat close", i, scratch[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* A single-bar range is the seed alone. */
   retCode = TA_PVT( 5, 5, history->close, history->volume,
                     &begIdx, &nbElement, scratch );
   g_pvtEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != 1 || scratch[0] != 0.0 )
   {
      printf( "PVT edge Fail [single bar]: rc=%d (%d,%d) %.17g expected "
              "(5,1) 0\n", (int)retCode, begIdx, nbElement, scratch[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_pvtOracleCmp != NB_PVT_ORACLE || g_pvtDiffCmp != 251
            || g_pvtEdgeCmp != 4 + 2*PVT_EDGE_NB + 1 ) )
   {
      printf( "PVT Fail: coverage counters (oracle %d, differential %d, "
              "edges %d) are not what this file was written with (%d, 251, "
              "%d)\n", g_pvtOracleCmp, g_pvtDiffCmp, g_pvtEdgeCmp,
              NB_PVT_ORACLE, 4 + 2*PVT_EDGE_NB + 1 );
      return TA_PVT_VACUOUS;
   }

   return TA_TEST_PASS;
}
