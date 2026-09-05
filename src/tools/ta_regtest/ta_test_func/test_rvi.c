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
 *  090526 MF,CC  First version (issue #366).
 */

/* Description:
 *
 *   Test TA_RVI (Relative Volatility Index).
 *
 *   Legs:
 *     1. DIFFERENTIAL, bit-exact: TA_RVI is TA_STDDEV routed by the sign of the
 *        close-to-close change into two TA_RMA legs, combined. Only the compare
 *        and the select are hand-written -- TA-Lib ships no vector sign gate --
 *        so every arithmetic operation in the reference comes from a shipped
 *        function. Bit-exact is not an aspiration here, it is the assertion,
 *        and four anchoring details are what make it hold: TA_STDDEV is called
 *        at TA_RVI's OWN startIdx - (optInTimePeriod-1) - unstable rather than
 *        at 0, TA_FUNC_UNST_RMA is set to TA_FUNC_UNST_RVI's value for the
 *        duration, the smoothing coefficients are spelled beta-first, and the
 *        zero-total combine is copied rather than approximated. This is the
 *        only leg that sees the coefficient SPELLING and the reseed pacing;
 *        every external oracle stays green when either is sabotaged.
 *     2. EXTERNAL GOLDEN, and the only leg that checks the FORMULA:
 *        trading-signals 8.3.0 (TypeScript), which implements this exact form
 *        -- separate stddev and smoothing periods, Wilder smoothing seeded
 *        with a simple average, ties feeding neither leg, and 50 on a zero
 *        total. Four parameter tuples, chosen so that the two tolerance terms
 *        are each driven by a different one.
 *     3. SECOND ORACLE, converged tail: pandas-ta-classic 0.6.52 (Python),
 *        `ta.rvi(close, length=L, mamode='rma')`. A different codebase and
 *        language witnessing the smoother, the up/down gate and the tie rule.
 *        It passes ONE length to both windows and seeds its Wilder average off
 *        a single sample, so only the tail is comparable -- see rviPandas.
 *     4. The TIE RULE, exactly and without decay. At optInTimePeriod 1 the
 *        smoothing has no memory, so every bar reads a literal: the whole
 *        series takes only {0, 50, 100}, and the corpus's single tie bar reads
 *        exactly 50. Ties to the down bucket would make it 0, ties to the up
 *        bucket 100. At the default period the same flip decays as (13/14)^k
 *        and a tail spot-check would barely see it.
 *     5. Flat input: every window has zero deviation, so both legs are exactly
 *        zero on every bar. Exactly 50.0 and never NaN (#112). Plus the two
 *        shape edges: a range that is exactly one bar long, and one shorter
 *        than the lookback.
 *     6. In-place aliasing (outReal == inReal), bitwise. TIGHT rather than
 *        routine: at optInTimePeriod 1 with no unstable period the deviation
 *        window's trailing read lands on exactly the slot being written, so
 *        the leg has zero slack and ASan cannot see a violation of it.
 *     7. The startIdx/endIdx range sweep, in the CONVERGING class.
 *
 *   Cross-language value coverage comes from server_verify in legs 2 and 3 plus
 *   the --xlang-hash sweep; the frozen ta_ref_serve predates this function, so
 *   the --codegen value comparison cannot run for it (same situation as RMA).
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** External variables declarations. ****/
extern double gDataClose[];

/**** Local declarations. ****/
#define RVI_CAP    1100
#define RVI_GD_NB  1000

/* Leg 2. trading-signals differs from TA_RVI in three deliberate FP spellings:
 * a two-pass standard deviation against var.c's shifted running sums, a
 * `(x-prev)*(1/n) + prev` smoothing step against the beta-first fused one, and
 * `(100*U)/T` against `100*(U/T)`. Both terms are load-bearing and each is set
 * by a different tuple. The relative term is driven by (2,2), where a two-bar
 * deviation window is the worst case for the shifted sums: measured 1.8e-12
 * relative there against 4.9e-15 at (14,10), so 1e-12 relative would be red on
 * a correct implementation. The absolute term is driven by (1,10), where the
 * golden is exactly 0 on 123 of its 243 bars and relative error is undefined:
 * measured 1.4e-14 absolute. 55x and 70x of headroom. */
#define RVI_TS_REL 1e-10
#define RVI_TS_ABS 1e-12

/* Leg 3. The pandas arm is compared only on its converged tail, so what is left
 * is FP noise: measured worst 1.5e-12 absolute at L=3 and 1.6e-12 at L=5, over
 * the bars frozen below. Both terms at 1e-9 leave ~650x. */
#define RVI_PANDAS_REL 1e-9
#define RVI_PANDAS_ABS 1e-9

typedef struct { int period; int sdPeriod; int bar; double want; } RviGolden;

/* Goldens captured by ta-lib-oracles/trading_signals_serve/capture.mjs (arm
 * `RVI`) on the 252-bar TA_SREF close series (TA_SREF_close_daily_ref_0_PRIV,
 * byte-equal to that harness's corpus.json), at %.17g, which round-trips to the
 * same double. `bar` is the ABSOLUTE bar index; the output index is bar -
 * begIdx.
 *
 * ORACLE: trading-signals 8.3.0 (TypeScript, MIT), driven 2026-09-05.
 * `dist/volatility/RVI/RelativeVolatilityIndex.js` keeps `stddevInterval`
 * closes, takes their population standard deviation, routes it with strict `>`
 * and `<` so a tie feeds neither leg, smooths both with `WSMA` (SMA-seeded
 * Wilder), and returns 50 when the total is exactly 0. Independent of TA-Lib:
 * pure TypeScript with no binding to it, and its `getRequiredInputs()` of
 * `stddevInterval + interval - 1` is an external confirmation of
 * TA_RVI_Lookback rather than a restatement of it.
 *
 * The four tuples are the coverage, not a sample: (14,10) is the default pair,
 * (30,30) equalises the two windows, (1,10) removes the smoothing's memory, and
 * (2,2) is the narrowest legal deviation window. Each tuple's first three
 * output bars are frozen (the seed is where a mis-anchored lookback shows), its
 * lowest-valued bar (where a relative tolerance is least forgiving), the bar of
 * this file's own worst measured deviation, the corpus's tie bar 101 and the
 * bar after it, and a spread through to the last. */
static const RviGolden rviTradingSignals[] =
{
   /* (14, 10): outBegIdx 22, outNBElement 230 */
   {  14,  10,  22,        52.713741724623155 },
   {  14,  10,  23,        48.646868138730177 },
   {  14,  10,  24,        52.943714669251996 },
   {  14,  10,  54,          34.1994860627427 },
   {  14,  10, 101,        42.101305423280287 },
   {  14,  10, 102,        39.928455950674561 },
   {  14,  10, 130,         68.13691208280666 },
   {  14,  10, 160,        50.053288311904204 },
   {  14,  10, 190,        49.300724220669601 },
   {  14,  10, 220,        45.928639431289859 },
   {  14,  10, 250,        52.432431467108586 },
   {  14,  10, 251,        51.398767922313233 },
   /* (30, 30): outBegIdx 58, outNBElement 194 */
   {  30,  30,  58,        36.379835705045764 },
   {  30,  30,  59,        38.059784265579033 },
   {  30,  30,  60,        37.053397432665136 },
   {  30,  30,  72,        32.424047694596986 },
   {  30,  30, 101,        41.587536115372323 },
   {  30,  30, 102,        39.676762506365129 },
   {  30,  30, 130,        56.652992556396228 },
   {  30,  30, 160,         48.36874198381436 },
   {  30,  30, 185,        47.839941573494478 },
   {  30,  30, 190,        49.957137141115396 },
   {  30,  30, 220,        43.514379819420725 },
   {  30,  30, 250,        50.809464235335028 },
   {  30,  30, 251,        49.804185637015948 },
   /* (1, 10): outBegIdx 9, outNBElement 243. Bar 88 is where the oracle's own
    * `(x-prev)*(1/n) + prev` step fails to be exact at n == 1 and ours is; it
    * is the 1.4e-14 the absolute tolerance is sized on. */
   {   1,  10,   9,                       100 },
   {   1,  10,  10,                       100 },
   {   1,  10,  11,                       100 },
   {   1,  10,  13,                         0 },
   {   1,  10,  88,        99.999999999999986 },
   {   1,  10, 101,                        50 },
   {   1,  10, 102,                         0 },
   {   1,  10, 130,                       100 },
   {   1,  10, 160,                       100 },
   {   1,  10, 190,                       100 },
   {   1,  10, 220,                       100 },
   {   1,  10, 250,                         0 },
   {   1,  10, 251,                         0 },
   /* (2, 2): outBegIdx 2, outNBElement 250 */
   {   2,   2,   2,        88.282290279627205 },
   {   2,   2,   3,        91.530317613089537 },
   {   2,   2,   4,        45.480631276901029 },
   {   2,   2,  72,       0.30538230402443489 },
   {   2,   2, 101,        55.634165686585654 },
   {   2,   2, 102,        16.585015730059386 },
   {   2,   2, 130,        97.189062734496503 },
   {   2,   2, 134,        20.717297589393151 },
   {   2,   2, 160,        67.249400393161054 },
   {   2,   2, 190,        75.624932269351575 },
   {   2,   2, 220,        46.818030002878082 },
   {   2,   2, 250,         22.90510761095139 },
   {   2,   2, 251,        8.0177050796979081 },
};
#define NB_RVI_TS ((int)(sizeof(rviTradingSignals)/sizeof(RviGolden)))

typedef struct { int length; int bar; double want; } RviPandasGolden;

/* Goldens captured by ta-lib-oracles/pandas_serve/capture_366_rvi.py against
 * pandas-ta-classic 0.6.52 / pandas 3.0.3 / numpy 2.5.1 (CPython 3.12), on the
 * same 252-bar close series, at %.17g.
 *
 * `pandas_ta_classic/volatility/rvi.py` with `mamode='rma'`. Its
 * `unsigned_differences` (utils/_core.py) zeroes BOTH legs on a tie -- note
 * that its own docstring says otherwise, so the code is the witness, not the
 * documentation.
 *
 * TAIL-ONLY, for two reasons that are properties of that library, not of this
 * comparison. It passes ONE `length` to both the deviation window and the
 * smoother, so it can be compared only where optInStdDevPeriod equals
 * optInTimePeriod; and its `rma` seeds off a single sample at bar length-1
 * where TA_RVI seeds off the simple average of `length` deviations. The Wilder
 * transient decays as ((L-1)/L)^k -- measured 6.8 index points apart at bar 0
 * and 5.2e-12 from bar 2(L-1)+168 on, at L=3 -- so the first frozen bar of each
 * length is that offset. Recorded so the offset reads as measured rather than
 * arbitrary. Only small L converges inside 252 bars: at L=14 the two are still
 * 3.8e-5 apart at bar 168. */
static const RviPandasGolden rviPandas[] =
{
   {   3, 172,        75.421660723963626 },
   {   3, 173,        84.936263915453551 },
   {   3, 200,        52.117030320814166 },
   {   3, 225,        74.114242807197684 },
   {   3, 250,         46.65444771377809 },
   {   3, 251,        32.442813669720266 },
   {   5, 176,        60.417472204571034 },
   {   5, 177,        49.096182142915353 },
   {   5, 200,        53.021808029532259 },
   {   5, 225,        55.763774660716422 },
   {   5, 250,        52.627149198260405 },
   {   5, 251,         44.68079106522589 },
};
#define NB_RVI_PANDAS ((int)(sizeof(rviPandas)/sizeof(RviPandasGolden)))

/* The differential grid. optInStdDevPeriod starts at 2 because that is the
 * parameter's floor, and it is a floor rather than a preference: the up/down
 * gate reads one bar before the deviation window's newest close, which sits
 * inside the window only while the window is at least two bars wide. */
static const int rviSdPeriods[]   = { 2, 3, 10, 14, 30 };
static const int rviTimePeriods[] = { 1, 2, 14, 30 };
#define NB_RVI_SD   ((int)(sizeof(rviSdPeriods)/sizeof(int)))
#define NB_RVI_TIME ((int)(sizeof(rviTimePeriods)/sizeof(int)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_rviDiffCmp;
static int g_rviTsCmp;
static int g_rviPandasCmp;
static int g_rviTieCmp;
static int g_rviEdgeCmp;
static int g_rviAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_rvi_differential( const char *tag, const TA_Real *in, int nbBars );
static ErrorNumber test_rvi_oracle( const TA_History *history );
static ErrorNumber test_rvi_pandas( const TA_History *history );
static ErrorNumber test_rvi_tie( const TA_History *history );
static ErrorNumber test_rvi_edges( void );
static ErrorNumber test_rvi_aliasing( const char *tag, const TA_Real *in, int nbBars );
static ErrorNumber test_rvi_range( const TA_Real *in );

/**** Global functions definitions. ****/
ErrorNumber test_func_rvi( TA_History *history )
{
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_rviDiffCmp = g_rviTsCmp = g_rviPandasCmp = 0;
   g_rviTieCmp = g_rviEdgeCmp = g_rviAliasCmp = 0;

   err = test_rvi_differential( "TA_SREF close", history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvi_differential( "gData close", gDataClose, RVI_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvi_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvi_pandas( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvi_tie( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvi_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvi_aliasing( "TA_SREF close", history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvi_aliasing( "gData close", gDataClose, RVI_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvi_range( history->close );
   if( err != TA_TEST_PASS )
      return err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_rviDiffCmp != 214154 || g_rviTsCmp != NB_RVI_TS
            || g_rviPandasCmp != NB_RVI_PANDAS || g_rviTieCmp != 243
            || g_rviEdgeCmp != 106592 || g_rviAliasCmp != 24178 ) )
   {
      printf( "RVI Fail: coverage counters (diff %d, trading-signals %d, "
              "pandas %d, tie %d, edges %d, alias %d) are not what this file "
              "was written with (214154, %d, %d, 243, 106592, 24178)\n",
              g_rviDiffCmp, g_rviTsCmp, g_rviPandasCmp, g_rviTieCmp,
              g_rviEdgeCmp, g_rviAliasCmp, NB_RVI_TS, NB_RVI_PANDAS );
      return TA_RVI_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) TA_RVI against a compose over shipped primitives, BIT-EXACT.
 *
 * The reference rebuilds the whole function from TA_STDDEV and two TA_RMA legs;
 * the only hand-written arithmetic is the combine, which is copied character
 * for character from the body under test because `100*(U/T)` and `(100*U)/T`
 * are different doubles.
 *
 * The anchoring is what makes this bit-exact rather than merely close, and
 * every one of the four details below moves the result at the 1e-13 level if
 * dropped: TA_STDDEV runs from the RVI call's own start minus the smoothing
 * lookback (var.c anchors both its shift and its reseed counter at the call's
 * own start, so a call from 0 reaches the same window by a different sequence
 * of adds); TA_FUNC_UNST_RMA is moved to TA_FUNC_UNST_RVI's value so the two
 * legs seed on the same bar; the gate is a compare and a select, which adds no
 * arithmetic; and the combine is the copy above.
 */
static ErrorNumber test_rvi_differential( const char *tag, const TA_Real *in, int nbBars )
{
   static TA_Real sigma[RVI_CAP], up[RVI_CAP], dn[RVI_CAP];
   static TA_Real refUp[RVI_CAP], refDn[RVI_CAP], out[RVI_CAP];
   TA_Integer begSd, nbSd, begU, nbU, begD, nbD, begIdx, nbElement;
   TA_RetCode retCode;
   int a, b, k, unst, anchor, startIdx, sdStart, lookbackTotal;
   int sdPeriod, period;
   ErrorNumber err = TA_TEST_PASS;

   for( a = 0; a < NB_RVI_SD; a++ )
   for( b = 0; b < NB_RVI_TIME; b++ )
   for( unst = 0; unst <= 4; unst += 2 )
   for( anchor = 0; anchor <= 40; anchor += 20 )
   {
      sdPeriod = rviSdPeriods[a];
      period   = rviTimePeriods[b];

      TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, (unsigned int)unst );
      lookbackTotal = TA_RVI_Lookback( period, sdPeriod );
      startIdx = anchor < lookbackTotal ? lookbackTotal : anchor;
      if( startIdx > nbBars-1 )
         continue;
      sdStart = startIdx - (period-1) - unst;

      retCode = TA_STDDEV( sdStart, nbBars-1, in, sdPeriod, 1.0,
                           &begSd, &nbSd, sigma );
      if( retCode != TA_SUCCESS || begSd != sdStart )
      {
         printf( "RVI differential [%s N=%d SD=%d u=%d]: TA_STDDEV rc=%d beg %d "
                 "expected %d\n", tag, period, sdPeriod, unst, (int)retCode,
                 begSd, sdStart );
         err = TA_TESTUTIL_TFRR_BAD_RETCODE;
         goto done;
      }

      /* The only hand-written step: TA-Lib ships no vector sign gate. A
       * compare and a select add no arithmetic, so the reference stays free of
       * novel numerics. */
      for( k = 0; k < nbSd; k++ )
      {
         int bar = begSd + k;
         up[k] = in[bar] > in[bar-1] ? sigma[k] : 0.0;
         dn[k] = in[bar] < in[bar-1] ? sigma[k] : 0.0;
      }

      TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, (unsigned int)unst );
      retCode = TA_RMA( 0, nbSd-1, up, period, &begU, &nbU, refUp );
      if( retCode == TA_SUCCESS )
         retCode = TA_RMA( 0, nbSd-1, dn, period, &begD, &nbD, refDn );
      TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
      if( retCode != TA_SUCCESS || begU != (period-1)+unst || nbD != nbU )
      {
         printf( "RVI differential [%s N=%d SD=%d u=%d]: TA_RMA rc=%d beg %d "
                 "expected %d\n", tag, period, sdPeriod, unst, (int)retCode,
                 begU, (period-1)+unst );
         err = TA_TESTUTIL_TFRR_BAD_RETCODE;
         goto done;
      }

      retCode = TA_RVI( startIdx, nbBars-1, in, period, sdPeriod,
                        &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != startIdx || nbElement != nbU )
      {
         printf( "RVI differential Fail [%s N=%d SD=%d u=%d s=%d]: rc=%d (%d,%d) "
                 "expected (%d,%d)\n", tag, period, sdPeriod, unst, startIdx,
                 (int)retCode, begIdx, nbElement, startIdx, nbU );
         err = TA_TESTUTIL_TFRR_BAD_BEGIDX;
         goto done;
      }

      for( k = 0; k < nbElement; k++ )
      {
         double total = refUp[k] + refDn[k];
         double want  = total == 0.0 ? 50.0 : 100.0*(refUp[k]/total);

         g_rviDiffCmp++;
         if( memcmp( &want, &out[k], sizeof(double) ) != 0 )
         {
            printf( "RVI differential Fail [%s N=%d SD=%d u=%d s=%d] at bar %d: "
                    "got %.17g expected %.17g -- the compose over TA_STDDEV and "
                    "TA_RMA must be BIT-identical\n",
                    tag, period, sdPeriod, unst, startIdx, begIdx+k, out[k], want );
            err = TA_TESTUTIL_TFRR_BAD_CALCULATION;
            goto done;
         }
      }
   }

done:
   TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
   return err;
}

/* (2) The frozen trading-signals goldens, plus the cross-language replay. */
static ErrorNumber test_rvi_oracle( const TA_History *history )
{
   static TA_Real out[RVI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastPeriod = -1, lastSd = -1;

   if( nbBars != 252 )
   {
      printf( "RVI oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_RVI_TS; k++ )
   {
      double got, err;
      const char *mode;

      if( rviTradingSignals[k].period != lastPeriod
          || rviTradingSignals[k].sdPeriod != lastSd )
      {
         lastPeriod = rviTradingSignals[k].period;
         lastSd     = rviTradingSignals[k].sdPeriod;
         retCode = TA_RVI( 0, nbBars-1, history->close, lastPeriod, lastSd,
                           &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS
             || begIdx != TA_RVI_Lookback( lastPeriod, lastSd )
             || nbElement != nbBars - begIdx )
         {
            printf( "RVI oracle Fail [N=%d SD=%d]: rc=%d (%d,%d) expected "
                    "(%d,%d)\n", lastPeriod, lastSd, (int)retCode, begIdx,
                    nbElement, TA_RVI_Lookback( lastPeriod, lastSd ),
                    nbBars - TA_RVI_Lookback( lastPeriod, lastSd ) );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[2];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastPeriod;
            optIn[1] = (double)lastSd;
            e = server_verify( "RVI", 0, nbBars-1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->close, NULL },
                               optIn, 2,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* "No failure reported" and "nothing was compared" are the same
             * observation without this. */
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "RVI oracle [N=%d SD=%d]: compared no server despite "
                       "live pipes\n", lastPeriod, lastSd );
               return TA_RVI_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( rviTradingSignals[k].bar < begIdx
          || rviTradingSignals[k].bar - begIdx >= nbElement )
      {
         printf( "RVI oracle Fail [N=%d SD=%d]: golden bar %d is outside the "
                 "output [%d..%d]\n", lastPeriod, lastSd,
                 rviTradingSignals[k].bar, begIdx, begIdx + nbElement - 1 );
         return TA_RVI_VACUOUS;
      }

      got = out[rviTradingSignals[k].bar - begIdx];
      g_rviTsCmp++;
      if( !checkOracleValue( got, rviTradingSignals[k].want,
                             RVI_TS_REL, RVI_TS_ABS, &err, &mode ) )
      {
         printf( "RVI oracle Fail [N=%d SD=%d] at bar %d: got %.17g expected "
                 "%.17g (%s err %.3g, tol rel %g abs %g)\n",
                 lastPeriod, lastSd, rviTradingSignals[k].bar, got,
                 rviTradingSignals[k].want, mode, err,
                 RVI_TS_REL, RVI_TS_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) The second oracle, on its converged tail. */
static ErrorNumber test_rvi_pandas( const TA_History *history )
{
   static TA_Real out[RVI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastLength = -1;

   if( nbBars != 252 )
      return TA_TEST_PASS;

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_RVI_PANDAS; k++ )
   {
      double got, err;
      const char *mode;

      if( rviPandas[k].length != lastLength )
      {
         lastLength = rviPandas[k].length;
         retCode = TA_RVI( 0, nbBars-1, history->close, lastLength, lastLength,
                           &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS
             || begIdx != TA_RVI_Lookback( lastLength, lastLength ) )
         {
            printf( "RVI pandas Fail [L=%d]: rc=%d (%d,%d)\n", lastLength,
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[2];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastLength;
            optIn[1] = (double)lastLength;
            e = server_verify( "RVI", 0, nbBars-1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->close, NULL },
                               optIn, 2,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "RVI pandas [L=%d]: compared no server despite live "
                       "pipes\n", lastLength );
               return TA_RVI_VACUOUS;
            }
         }
      }

      if( rviPandas[k].bar < begIdx
          || rviPandas[k].bar - begIdx >= nbElement )
      {
         printf( "RVI pandas Fail [L=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", lastLength, rviPandas[k].bar, begIdx,
                 begIdx + nbElement - 1 );
         return TA_RVI_VACUOUS;
      }

      got = out[rviPandas[k].bar - begIdx];
      g_rviPandasCmp++;
      if( !checkOracleValue( got, rviPandas[k].want,
                             RVI_PANDAS_REL, RVI_PANDAS_ABS, &err, &mode ) )
      {
         printf( "RVI pandas Fail [L=%d] at bar %d: got %.17g expected %.17g "
                 "(%s err %.3g, tol rel %g abs %g)\n",
                 lastLength, rviPandas[k].bar, got, rviPandas[k].want, mode,
                 err, RVI_PANDAS_REL, RVI_PANDAS_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4) The tie rule, read as literals.
 *
 * At optInTimePeriod 1 the pair of smoothing coefficients is exactly (1, 0), so
 * each bar's output is decided by that bar alone: an up bar is exactly 100, a
 * down bar exactly 0, and a tie leaves both legs at zero and reads the neutral
 * centre. The corpus has exactly one tie, at bar 101. Routing it to the down
 * bucket would print 0 there and shift the two counts by one; routing it to the
 * up bucket, 100. No tolerance is involved in any of it.
 */
static ErrorNumber test_rvi_tie( const TA_History *history )
{
   static TA_Real out[RVI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int i, nbZero = 0, nbHundred = 0, nbFifty = 0;

   if( nbBars != 252 )
      return TA_TEST_PASS;

   retCode = TA_RVI( 0, nbBars-1, history->close, 1, 10, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 9 || nbElement != 243 )
   {
      printf( "RVI tie Fail: rc=%d (%d,%d) expected (9,243)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbElement; i++ )
   {
      g_rviTieCmp++;
      if( out[i] == 0.0 )
         nbZero++;
      else if( out[i] == 100.0 )
         nbHundred++;
      else if( out[i] == 50.0 )
         nbFifty++;
      else
      {
         printf( "RVI tie Fail at bar %d: %.17g is not one of {0, 50, 100}. "
                 "With no smoothing memory every bar is decided by its own "
                 "sign, so any other value means the gate or the seed carries "
                 "state it should not\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   if( nbZero != 123 || nbHundred != 119 || nbFifty != 1
       || out[101-begIdx] != 50.0 )
   {
      printf( "RVI tie Fail: %d down / %d up / %d neutral bars and bar 101 = "
              "%.17g; expected 123 / 119 / 1 and exactly 50.0. Ties to the "
              "down bucket give 0.0 at bar 101, ties to the up bucket 100.0\n",
              nbZero, nbHundred, nbFifty, out[101-begIdx] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (5) Flat input and the two shape edges.
 *
 * A flat window has zero deviation, so both smoothed legs are exactly zero on
 * every bar and the guarded combine is the only thing between the call and a
 * NaN from 0/0 (#112). Non-vacuous by construction: without the guard every
 * value below is NaN, and NaN fails the equality.
 */
static ErrorNumber test_rvi_edges( void )
{
   static TA_Real in[300], out[RVI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int period, sdPeriod, i, lookbackTotal;

   for( i = 0; i < 300; i++ )
      in[i] = 42.0;

   for( sdPeriod = 2; sdPeriod <= 20; sdPeriod++ )
   for( period = 1; period <= 20; period++ )
   {
      lookbackTotal = TA_RVI_Lookback( period, sdPeriod );
      retCode = TA_RVI( 0, 299, in, period, sdPeriod, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != lookbackTotal
          || nbElement != 300 - lookbackTotal )
      {
         printf( "RVI flat Fail [N=%d SD=%d]: rc=%d (%d,%d)\n",
                 period, sdPeriod, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_rviEdgeCmp++;
         if( isnan( out[i] ) || out[i] != 50.0 )
         {
            printf( "RVI flat Fail [N=%d SD=%d] out %d: %.17g, expected exactly "
                    "50.0 (NaN => the zero-total guard is missing)\n",
                    period, sdPeriod, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* One bar of output, requested at the first index that has one. */
   lookbackTotal = TA_RVI_Lookback( 14, 10 );
   retCode = TA_RVI( lookbackTotal, lookbackTotal, in, 14, 10,
                     &begIdx, &nbElement, out );
   g_rviEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != lookbackTotal || nbElement != 1 )
   {
      printf( "RVI edge Fail: single-bar range gave rc=%d (%d,%d), expected "
              "(%d,1)\n", (int)retCode, begIdx, nbElement, lookbackTotal );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* A range shorter than the lookback produces nothing, successfully. */
   retCode = TA_RVI( 0, lookbackTotal-1, in, 14, 10, &begIdx, &nbElement, out );
   g_rviEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "RVI edge Fail: short range gave rc=%d (%d,%d), expected "
              "(0,0)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   return TA_TEST_PASS;
}

/* (6) In-place aliasing, outReal == inReal, bitwise.
 *
 * Tight rather than routine. Each bar reads the deviation window's trailing
 * close at optInStdDevPeriod-1 bars back, the reseed rescan from the same slot,
 * and the previous close for the gate; the earliest of those lands on exactly
 * the slot being written when optInTimePeriod is 1 and no unstable period is
 * set. Zero slack, and nothing but this leg observes it.
 */
static ErrorNumber test_rvi_aliasing( const char *tag, const TA_Real *in, int nbBars )
{
   static TA_Real clean[RVI_CAP], alias[RVI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int a, b, i, sdPeriod, period;

   for( a = 0; a < NB_RVI_SD; a++ )
   for( b = 0; b < NB_RVI_TIME; b++ )
   {
      sdPeriod = rviSdPeriods[a];
      period   = rviTimePeriods[b];

      retCode = TA_RVI( 0, nbBars-1, in, period, sdPeriod,
                        &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "RVI alias Fail [%s N=%d SD=%d]: rc=%d\n",
                 tag, period, sdPeriod, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = in[i];
      retCode = TA_RVI( 0, nbBars-1, alias, period, sdPeriod,
                        &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "RVI alias Fail [%s N=%d SD=%d]: rc=%d shape (%d,%d) vs "
                 "(%d,%d)\n", tag, period, sdPeriod, (int)retCode,
                 begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_rviAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "RVI alias Fail [%s N=%d SD=%d] out %d: separate %.17g, "
                    "in-place %.17g -- a store landed under a read the same bar "
                    "still needed\n",
                    tag, period, sdPeriod, i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) The startIdx/endIdx range sweep. TA_STABLE_CONVERGING: the two smoothed
 * legs are IIR recurrences seeded at startIdx - lookback, so an earlier start
 * moves a value by a residual the unstable period bounds. */
typedef struct { int period; int sdPeriod; const TA_Real *in; } RviRangeParam;

static TA_RetCode rviRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   RviRangeParam *p = (RviRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_RVI_Lookback( p->period, p->sdPeriod );
   return TA_RVI( startIdx, endIdx, p->in, p->period, p->sdPeriod,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_rvi_range( const TA_Real *in )
{
   RviRangeParam param;

   param.period   = 14;
   param.sdPeriod = 10;
   param.in       = in;

   return doRangeTestEx( rviRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_RVI,
                         (void *)&param, 1, 0 );
}
