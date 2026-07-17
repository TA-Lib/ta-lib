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
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  071626 MF,CC  First version. CMOU (Chande Momentum Oscillator, Unsmoothed).
 */

/* Description:
 *
 *   Regression tests for CMOU, the unsmoothed Chande Momentum Oscillator
 *   (issue #124). CMOU is Chande's original definition,
 *   100*(Su-Sd)/(Su+Sd) over plain moving-window sums of up/down moves, with
 *   NO Wilder smoothing and NO unstable period -- distinct from the shipped,
 *   Wilder-smoothed CMO (which is left unchanged).
 *
 *   Coverage:
 *     (1) External-oracle formula check on the standard 252-bar close series.
 *         The golden values are from an independent from-definition reference
 *         that is byte-for-byte the algorithm pandas-ta-classic uses for
 *         cmo(talib=False) (the rolling-sum, non-TA-Lib CMO that TradingView /
 *         QuantConnect also compute). This proves the RIGHT formula. The same
 *         call is verified bit-for-bit against every language server and in an
 *         in-place (outReal == inReal) buffer.
 *     (2) Deterministic edge windows: an all-up window (-> +100), an all-down
 *         window (-> -100), an exactly-flat window (Su+Sd == 0 -> 0.0, a
 *         non-vacuous test of the divide-by-zero guard -- without it the result
 *         is NaN), and a small mixed window checked against a hand-computed
 *         value.
 *     (3) The "no unstable period" invariant: CMOU carries no unstable-period
 *         abstract flag and ignores the global unstable-period state entirely.
 *     (4) The generic startIdx/endIdx range sweep (self-coherency + lookback).
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define OUT_CAP 300   /* > MAX_NB_TEST_ELEMENT and > nbBars */

/* External-oracle golden values for CMOU(close, 14) on the standard 252-bar
 * close series (TA_SREF_close_daily_ref_0_PRIV). outBegIdx = 14, nb = 238.
 *
 * Confirmed by TWO independent third-party implementations of the unsmoothed
 * CMO, each driven through its ta-lib-oracles JSON-RPC server on this exact
 * series (2026-07-16):
 *   1. Tulip Indicators v0.9.2 (pinned be18abb) `ti_cmo` -- LGPL C library by
 *      Lewis Van Winkle (ta-lib-oracles/tulip_serve). Tulip's own benchmark
 *      notes "talib uses EMA, we use SMA": its ti_cmo is the rolling-sum form,
 *      i.e. exactly CMOU (100*(Su-Sd)/(Su+Sd) over the period, no smoothing).
 *   2. pandas-ta-classic 0.6.52 `cmo(length=14, talib=False)` -- Python library
 *      (ta-lib-oracles/pandas_serve). This is the non-TA-Lib CMO that TradingView
 *      (ta.cmo) and QuantConnect also compute.
 * The two oracles are BIT-IDENTICAL to each other, and TA_CMOU reproduces both
 * BIT-FOR-BIT across all 238 outputs (maxabs 0) on the reference platform --
 * CMOU scales-then-divides, (100*(Su-Sd))/(Su+Sd), the same order Tulip and
 * pandas-ta use (see cmou.c). Tolerance 1e-12 (not exact) only guards against
 * cross-platform FP rounding of the running sums; it stays far tighter than any
 * formula error (the Wilder-smoothed CMO diverges from these by whole percent).
 * Golden captured via ta-lib-oracles {tulip_serve, pandas_serve}.
 *
 * idx is the OUTPUT-array index (0 == global bar 14). */
static const struct { int idx; double value; } cmouOracle[] =
{
   {   0,  -1.7053206002728516 },
   {   1,  -7.189072609633359  },
   {  56,  21.50968603874415   },
   { 113,  57.38636363636365   },
   { 170, -25.802752293577985  },
   { 237,  -8.07719799857037   },
};
#define NB_CMOU_ORACLE (sizeof(cmouOracle)/sizeof(cmouOracle[0]))
#define CMOU_ORACLE_BEG 14
#define CMOU_ORACLE_NB  238
#define CMOU_ORACLE_TOL 1e-12

/**** Local functions declarations. ****/
static ErrorNumber test_cmou_oracle( const TA_History *history );
static ErrorNumber test_cmou_edges( void );
static ErrorNumber test_cmou_no_unstable_period( const TA_History *history );
static ErrorNumber test_cmou_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_cmou( TA_History *history )
{
   ErrorNumber retValue;

   /* CMOU has no unstable period; make sure a leftover global setting from an
    * earlier test cannot influence it (it must not). */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_cmou_oracle( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_cmou_edges();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_cmou_no_unstable_period( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_cmou_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) External-oracle formula check + in-place + cross-language bitwise. */
static ErrorNumber test_cmou_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   static TA_Real out[OUT_CAP];
   static TA_Real inplace[OUT_CAP];
   unsigned int k;
   int i;

   retCode = TA_CMOU( 0, (int)history->nbBars - 1, history->close,
                      14, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "CMOU oracle Fail: retCode = %d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begIdx != CMOU_ORACLE_BEG || nbElement != CMOU_ORACLE_NB )
   {
      printf( "CMOU oracle Fail: shape got (%d,%d) expected (%d,%d)\n",
              begIdx, nbElement, CMOU_ORACLE_BEG, CMOU_ORACLE_NB );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( k = 0; k < NB_CMOU_ORACLE; k++ )
   {
      int idx    = cmouOracle[k].idx;
      double want = cmouOracle[k].value;
      double got  = out[idx];
      double rel  = fabs( got - want ) / fabs( want );  /* every golden |value| >> 0 */

      if( isnan( got ) || rel > CMOU_ORACLE_TOL )  /* NaN > tol is false -> guard explicitly */
      {
         printf( "CMOU oracle Fail at out[%d]: got %.17g expected %.17g (rel=%.3e > %.3e)\n",
                 idx, got, want, rel, CMOU_ORACLE_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* In-place (outReal == inReal) must be bit-for-bit identical to the
    * separate-buffer result -- the trailing-value cache exists precisely so the
    * moving window survives the output overwriting the input in place. */
   for( i = 0; i < (int)history->nbBars; i++ )
      inplace[i] = history->close[i];
   retCode = TA_CMOU( 0, (int)history->nbBars - 1, inplace,
                      14, &begIdx2, &nbElement2, inplace );
   if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
   {
      printf( "CMOU in-place Fail: rc=%d shape (%d,%d) vs (%d,%d)\n",
              (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( memcmp( out, inplace, (size_t)nbElement * sizeof(TA_Real) ) != 0 )
   {
      for( i = 0; i < nbElement; i++ )
         if( out[i] != inplace[i] )
         {
            printf( "CMOU in-place Fail: bit mismatch at out[%d] separate=%.17g inplace=%.17g\n",
                    i, out[i], inplace[i] );
            break;
         }
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* Cross-language: CMOU must be bit-identical on every language server. */
   if( server_verify_active() )
   {
      ErrorNumber e = server_verify( "CMOU", 0, (int)history->nbBars - 1, history->nbBars,
                                     retCode, begIdx, nbElement,
                                     (const TA_Real*[]){ history->close, NULL },
                                     (double[]){ 14.0 }, 1,
                                     (const TA_Real*[]){ out, NULL }, NULL );
      if( e != TA_TEST_PASS )
         return e;
   }

   return TA_TEST_PASS;
}

/* (2) Deterministic edge windows on synthetic inputs. */
static ErrorNumber test_cmou_edges( void )
{
   static TA_Real in[32];
   static TA_Real out[OUT_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i;

   /* --- all-up: strictly increasing -> every change > 0 -> Su>0, Sd=0 -> +100 --- */
   for( i = 0; i < 20; i++ )
      in[i] = 100.0 + 1.5 * i;
   retCode = TA_CMOU( 0, 19, in, 5, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != 15 )
   {
      printf( "CMOU all-up Fail: rc=%d shape (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( out[i] != 100.0 )
      {
         printf( "CMOU all-up Fail: out[%d] = %.17g (expected exactly 100.0)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* --- all-down: strictly decreasing -> every change < 0 -> Su=0, Sd>0 -> -100 --- */
   for( i = 0; i < 20; i++ )
      in[i] = 100.0 - 1.5 * i;
   retCode = TA_CMOU( 0, 19, in, 5, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != 15 )
   {
      printf( "CMOU all-down Fail: rc=%d shape (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( out[i] != -100.0 )
      {
         printf( "CMOU all-down Fail: out[%d] = %.17g (expected exactly -100.0)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* --- all-flat: constant -> every change == 0 -> Su+Sd == 0 -> guarded 0.0.
    * Non-vacuous: without the TA_IS_ZERO guard this is 100*(0/0) == NaN, so an
    * exact 0.0 (and not-NaN) here proves the guard fired. --- */
   for( i = 0; i < 20; i++ )
      in[i] = 42.0;
   retCode = TA_CMOU( 0, 19, in, 5, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != 15 )
   {
      printf( "CMOU all-flat Fail: rc=%d shape (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( isnan( out[i] ) || out[i] != 0.0 )
      {
         printf( "CMOU all-flat Fail: out[%d] = %.17g (expected exactly 0.0; NaN => guard missing)\n",
                 i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* --- mixed, hand-computed. prices {10,11,9,12,12,8}, period 3.
    *   changes: +1,-2,+3, 0,-4
    *   bar 3 window (+1,-2,+3): Su=4, Sd=2 -> 100*(4-2)/(4+2)
    *   bar 4 window (-2,+3, 0): Su=3, Sd=2 -> 100*(3-2)/(3+2) = 20
    *   bar 5 window (+3, 0,-4): Su=3, Sd=4 -> 100*(3-4)/(3+4)
    */
   {
      static const TA_Real mixed[6] = { 10.0, 11.0, 9.0, 12.0, 12.0, 8.0 };
      double expect[3];
      expect[0] = 100.0 * ( (4.0 - 2.0) / (4.0 + 2.0) );
      expect[1] = 100.0 * ( (3.0 - 2.0) / (3.0 + 2.0) );
      expect[2] = 100.0 * ( (3.0 - 4.0) / (3.0 + 4.0) );

      retCode = TA_CMOU( 0, 5, mixed, 3, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 3 || nbElement != 3 )
      {
         printf( "CMOU mixed Fail: rc=%d shape (%d,%d) expected (3,3)\n",
                 (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < 3; i++ )
         if( fabs( out[i] - expect[i] ) > 1e-12 )
         {
            printf( "CMOU mixed Fail: out[%d] = %.17g expected %.17g\n", i, out[i], expect[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   return TA_TEST_PASS;
}

/* (3) No unstable period: neither the abstract flag nor the runtime behavior. */
static ErrorNumber test_cmou_no_unstable_period( const TA_History *history )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo   *funcInfo;
   TA_RetCode retCode;
   TA_Integer beg0, nb0, beg5, nb5;
   static TA_Real out0[OUT_CAP], out5[OUT_CAP];
   int endIdx = (int)history->nbBars - 1;

   /* (a) CMOU must not advertise the unstable-period abstract flag. */
   if( TA_GetFuncHandle( "CMOU", &handle ) != TA_SUCCESS ||
       TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "CMOU no-unstable Fail: cannot get func handle/info\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   if( funcInfo->flags & TA_FUNC_FLG_UNST_PER )
   {
      printf( "CMOU no-unstable Fail: CMOU advertises TA_FUNC_FLG_UNST_PER\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (b) The global unstable-period state must not change CMOU output. Compute
    * once with everything at 0, once with everything at 5; results must be
    * bit-identical (a Wilder-smoothed CMO would differ). */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   retCode = TA_CMOU( 0, endIdx, history->close, 14, &beg0, &nb0, out0 );
   if( retCode != TA_SUCCESS ) { printf( "CMOU no-unstable Fail: rc=%d\n", (int)retCode ); return TA_TESTUTIL_TFRR_BAD_RETCODE; }

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 5 );
   retCode = TA_CMOU( 0, endIdx, history->close, 14, &beg5, &nb5, out5 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );   /* restore for later tests */
   if( retCode != TA_SUCCESS ) { printf( "CMOU no-unstable Fail: rc=%d\n", (int)retCode ); return TA_TESTUTIL_TFRR_BAD_RETCODE; }

   if( beg0 != beg5 || nb0 != nb5 ||
       memcmp( out0, out5, (size_t)nb0 * sizeof(TA_Real) ) != 0 )
   {
      printf( "CMOU no-unstable Fail: output changed with the global unstable period\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (4) Generic startIdx/endIdx range sweep (self-coherency + lookback). CMOU is
 * a finite moving-window sum with no unstable period: TA_STABLE_EPSILON, no
 * unstId (matching its abstract metadata; the doRangeTestEx guard rejects a
 * bogus unstId for a non-converging class). */
typedef struct { int period; const TA_Real *close; } CmouRangeParam;

static TA_RetCode cmouRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   CmouRangeParam *p = (CmouRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_CMOU_Lookback( p->period );
   return TA_CMOU( startIdx, endIdx, p->close, p->period,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_cmou_range( const TA_History *history )
{
   CmouRangeParam param;
   param.period = 14;
   param.close  = history->close;

   return doRangeTestEx( cmouRangeTestFunction,
                         TA_STABLE_EPSILON, TA_FUNC_UNST_NONE,
                         (void *)&param, 1, 0 );
}
