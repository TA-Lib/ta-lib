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
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091126 KL     First version (proposal-drafts issue #68).
 *  092026 MF,CC  Freeze the trading-signals and pandas-ta-classic goldens
 *                (issue #416).
 *
 */

/* Description:
 *     Test TA_RVIR, the 1995 refined Relative Volatility Index: TA_RVI over the
 *     highs and again over the lows, averaged.
 *
 *     WHAT EACH LEG CAN AND CANNOT SEE. The function is implemented by calling
 *     TA_RVI twice, so a leg written against TA_RVI is structurally true and
 *     cannot fail for an arithmetic reason -- it pins the API contract for a
 *     later rewrite, nothing more. Leg 1 carries the arithmetic: it rebuilds
 *     both legs from TA_STDDEV and TA_RMA, so it holds even if TA_RVI and
 *     TA_RVIR are both wrong in the same way. Legs 2 and 3 are the only ones
 *     that check the FORMULA against code TA-Lib did not write.
 *
 *     Leg 1 anchors its reference at the outBegIdx this function reports, so a
 *     lookback that drifts by one bar shifts both sides together and is
 *     invisible to it. Legs 2 and 3 index ABSOLUTE bars, which is what catches
 *     that class.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define RVIR_CAP 1100

/* Leg 2. Each trading-signals leg carries TA_RVI's three FP spellings apart
 * from ours (see test_rvi.c), and the average adds the two legs' residuals.
 * Relative term: measured worst 3.7e-12 over the whole (2,2) series, the
 * narrowest deviation window, so 27x. Absolute term: (1,10) has 88 bars where
 * the golden is exactly 0 and relative error is undefined; the deviation is
 * exactly 0 on all of them. */
#define RVIR_TS_REL 1e-10
#define RVIR_TS_ABS 1e-12

/* Leg 3. Converged tail only, so what is left is FP noise: measured worst
 * 1.5e-13 relative over the frozen bars, where the relative term governs. */
#define RVIR_PANDAS_REL 1e-9
#define RVIR_PANDAS_ABS 1e-9

typedef struct { int period; int sdPeriod; int bar; double want; } RvirGolden;

/* Captured by ta-lib-oracles/trading_signals_serve/capture_416_rvir.mjs on the
 * 252-bar TA_SREF high and low series, at %.17g. `bar` is the ABSOLUTE bar index.
 *
 * ORACLE: trading-signals 8.3.0 (TypeScript, MIT), driven 2026-09-13. The
 * library ships only the close form, so the arm is COMPOSED: one
 * `RelativeVolatilityIndex` fed the highs, one fed the lows, averaged in the
 * harness. Each leg is the arm test_rvi.c froze TA_RVI against; what this
 * table adds is the leg assignment, the average, and the n1 != n2 default pair,
 * which pandas cannot reach.
 *
 * Tuples as test_rvi.c: (14,10) the defaults, (30,30) equal windows, (1,10) no
 * smoothing memory, (2,2) the narrowest deviation window. Per tuple: the first
 * three output bars, the lowest bar, the bar of the worst absolute deviation, a
 * spread to the last bar, and the six tie bars (highs 66, 88, 250; lows 81,
 * 176, 191). At (1,10) a tie reads a literal: 75 or 25 under ties-feed-neither,
 * where ties-to-down would print 50 or 0. */
static const RvirGolden rvirTradingSignals[] =
{
   /* (14, 10): outBegIdx 22, outNBElement 230 */
   {  14,  10,  22,        43.834741790037512 },
   {  14,  10,  23,         40.77374430876857 },
   {  14,  10,  24,        37.279696373902446 },
   {  14,  10,  66,        66.903540746366005 },
   {  14,  10,  81,        55.038451097956369 },
   {  14,  10,  88,        66.475488651760941 },
   {  14,  10, 130,        69.532254289972755 },
   {  14,  10, 160,        54.453504657419614 },
   {  14,  10, 176,         59.71553016588912 },
   {  14,  10, 190,        43.207283551951889 },
   {  14,  10, 191,        42.114446624474176 },
   {  14,  10, 197,        30.103611556830717 },
   {  14,  10, 220,        51.853545388720846 },
   {  14,  10, 250,        53.521391948420103 },
   {  14,  10, 251,        52.847645335991672 },
   /* (30, 30): outBegIdx 58, outNBElement 194 */
   {  30,  30,  58,        56.068615861509272 },
   {  30,  30,  59,        57.233831712375022 },
   {  30,  30,  60,        57.051528886620581 },
   {  30,  30,  66,        59.832630648346139 },
   {  30,  30,  81,        54.010003772515546 },
   {  30,  30,  88,        65.395682542795669 },
   {  30,  30, 130,        60.748543399744833 },
   {  30,  30, 160,        54.001348179556217 },
   {  30,  30, 173,        58.338042240109715 },
   {  30,  30, 176,        55.622472925677386 },
   {  30,  30, 190,        47.894782786787374 },
   {  30,  30, 191,        47.132821157584125 },
   {  30,  30, 197,        39.408095166996944 },
   {  30,  30, 220,        51.103847013845694 },
   {  30,  30, 250,        53.365991863635259 },
   {  30,  30, 251,        52.179727694709939 },
   /* (1, 10): outBegIdx 9, outNBElement 243. Bar 116 is the oracle's
    * `(100*U)/T` rounding one ulp past 100 where ours is exact. */
   {   1,  10,   9,                       100 },
   {   1,  10,  10,                       100 },
   {   1,  10,  11,                       100 },
   {   1,  10,  12,                         0 },
   {   1,  10,  66,                        75 },
   {   1,  10,  81,                        75 },
   {   1,  10,  88,                        75 },
   {   1,  10, 116,        100.00000000000001 },
   {   1,  10, 130,                       100 },
   {   1,  10, 160,                       100 },
   {   1,  10, 176,                        75 },
   {   1,  10, 190,                       100 },
   {   1,  10, 191,                        25 },
   {   1,  10, 220,                         0 },
   {   1,  10, 250,                        25 },
   {   1,  10, 251,                         0 },
   /* (2, 2): outBegIdx 2, outNBElement 250 */
   {   2,   2,   2,                       100 },
   {   2,   2,   3,        79.706723891273185 },
   {   2,   2,   4,        59.331323234255507 },
   {   2,   2,  66,        86.857488948631499 },
   {   2,   2,  73,       0.44422411013126006 },
   {   2,   2,  81,        61.165537915375296 },
   {   2,   2,  88,         94.11601868471412 },
   {   2,   2, 130,        98.553548758222831 },
   {   2,   2, 160,        47.176306319817677 },
   {   2,   2, 176,        49.879101818014568 },
   {   2,   2, 190,         72.85692467201622 },
   {   2,   2, 191,        65.931783243236012 },
   {   2,   2, 209,        81.241982015637376 },
   {   2,   2, 220,        24.230690810514364 },
   {   2,   2, 250,         47.03563025544014 },
   {   2,   2, 251,        7.3249335697861042 },
};
#define NB_RVIR_TS ((int)(sizeof(rvirTradingSignals)/sizeof(RvirGolden)))

typedef struct { int length; int bar; double want; } RvirPandasGolden;

/* Captured by ta-lib-oracles/pandas_serve/capture_416_rvir.py against
 * pandas-ta-classic 0.6.52 / pandas 3.0.3 / numpy 2.5.1 (CPython 3.12):
 * `ta.rvi(close, high=, low=, length=L, mamode='rma', refined=True)`, the
 * library's NATIVE refined form, so unlike leg 2 the average is its code too.
 *
 * TAIL-ONLY for test_rvi.c's two reasons: one `length` feeds both windows, and
 * its `rma` seeds off a single sample. The first frozen bar is outBegIdx + 168,
 * where the transient has decayed to FP noise; the same arm is 13.2 index
 * points away at outBegIdx itself (L=3).
 *
 * {3, 5} is where the noise still fits RVIR_PANDAS_REL: the same tail is
 * 4.1e-12 at L=10 and 5.8e-9 at L=14, so a longer window needs its own
 * tolerance, not this table. */
static const RvirPandasGolden rvirPandas[] =
{
   {   3, 172,        93.863274259268735 },
   {   3, 173,        96.039919686896539 },
   {   3, 200,        37.333838623496639 },
   {   3, 225,        96.825942774942291 },
   {   3, 250,        39.532209962368356 },
   {   3, 251,        18.128116997522341 },
   {   5, 176,        66.188035983194098 },
   {   5, 177,        52.774106499207271 },
   {   5, 200,        38.588496220743195 },
   {   5, 225,        90.713792267953266 },
   {   5, 250,        48.129912462543267 },
   {   5, 251,        40.335119357911168 },
};
#define NB_RVIR_PANDAS ((int)(sizeof(rvirPandas)/sizeof(RvirPandasGolden)))
/* The legs below diff their own corpora against the language servers
 * bit-for-bit (issue #427). Without it a vector reaches in-process C and
 * nothing else: the RMA differential, the two-leg composite identity and the
 * degenerate high==low case are written specifically to pin this function's
 * shape, and a Rust/Java/C# port that averaged the legs in the wrong order or
 * mis-seeded the RMA would satisfy all of them while disagreeing with C.
 *
 * MEASURED that this compares rather than merely runs -- see the commit that
 * added it: feeding the servers a different optInTimePeriod than C used fails
 * with "SV FAIL [RVIR] ... BITWISE mismatch vs in-process C".
 */
#define RVIR_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, hi, lo, per, sdPer, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_value_comparisons();                         \
         ErrorNumber svErr_ = server_verify(                                     \
            "RVIR", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),                 \
            (const TA_Real*[]){ (hi), (lo), NULL },                              \
            (double[]){ (double)(per), (double)(sdPer) }, 2,                     \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         /* "Returned PASS" and "compared nothing" are the same observation      \
          * without this floor: server_verify() skips reject cases by design.    \
          * Every call site below is a success case, so the skip path is         \
          * unreachable and the count must advance.  */                          \
         if( server_verify_value_comparisons() == svCmp_ )                       \
         {                                                                       \
            printf( "RVIR [N=%d SD=%d]: server_verify compared no "              \
                    "server despite live pipes\n", (int)(per), (int)(sdPer) );   \
            return TA_RVIR_VACUOUS;                                              \
         }                                                                       \
      }                                                                          \
   } while(0)

/* Same check, for the two legs that own a `done:` label. Those legs sweep the
 * unstable period and restore it there, so returning straight out of the macro
 * would leave TA_FUNC_UNST_RVI (and RMA) set for every test that runs after
 * this file -- a failure here would corrupt the rest of the run rather than
 * just reporting itself. */
#define RVIR_SERVER_VERIFY_GOTO(sIdx, eIdx, nbBars, rc, beg, nb, hi, lo, per, sdPer, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_value_comparisons();                         \
         ErrorNumber svErr_ = server_verify(                                     \
            "RVIR", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),                 \
            (const TA_Real*[]){ (hi), (lo), NULL },                              \
            (double[]){ (double)(per), (double)(sdPer) }, 2,                     \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
         {                                                                       \
            err = svErr_;                                                        \
            goto done;                                                           \
         }                                                                       \
         if( server_verify_value_comparisons() == svCmp_ )                       \
         {                                                                       \
            printf( "RVIR [N=%d SD=%d]: server_verify compared no "              \
                    "server despite live pipes\n", (int)(per), (int)(sdPer) );   \
            err = TA_RVIR_VACUOUS;                                               \
            goto done;                                                           \
         }                                                                       \
      }                                                                          \
   } while(0)

/* optInStdDevPeriod >= 2: a one-bar deviation window is identically zero.
 * optInTimePeriod == 1 puts the no-memory case into every sweep below. */
static const int rvirSdPeriods[]   = { 2, 3, 10, 14, 30 };
static const int rvirTimePeriods[] = { 1, 2, 14, 30 };
#define NB_RVIR_SD   ((int)(sizeof(rvirSdPeriods)/sizeof(int)))
#define NB_RVIR_TIME ((int)(sizeof(rvirTimePeriods)/sizeof(int)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_rvirDiffCmp;
static int g_rvirTsCmp;
static int g_rvirPandasCmp;
static int g_rvirCompCmp;
static int g_rvirDegenCmp;
static int g_rvirTieCmp;
static int g_rvirEdgeCmp;
static int g_rvirAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber rvir_reference_leg( const TA_Real *in, int nbBars, int startIdx,
                                       int period, int sdPeriod, int unst,
                                       TA_Real *ref, int *nbRef, const char *tag );
static ErrorNumber test_rvir_differential( const TA_History *history );
static ErrorNumber test_rvir_oracle( const TA_History *history );
static ErrorNumber test_rvir_pandas( const TA_History *history );
static ErrorNumber test_rvir_composite( const TA_History *history );
static ErrorNumber test_rvir_degenerate( const TA_History *history );
static ErrorNumber test_rvir_tie( const TA_History *history );
static ErrorNumber test_rvir_edges( void );
static ErrorNumber test_rvir_aliasing( const TA_History *history );
static ErrorNumber test_rvir_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_rvir( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_rvirDiffCmp = g_rvirTsCmp = g_rvirPandasCmp = 0;
   g_rvirCompCmp = g_rvirDegenCmp = 0;
   g_rvirTieCmp = g_rvirEdgeCmp = g_rvirAliasCmp = 0;

   err = test_rvir_differential( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_pandas( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_composite( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_degenerate( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_tie( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_range( history );
   if( err != TA_TEST_PASS )
      return err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( history->nbBars == 252 )
   {
      /* One source for each count: a second copy in the message can disagree
       * with the one the gate tests, and only a mutation would show it.  */
      static const struct { const char *leg; int want; const int *got; } cov[] = {
         { "differential",     39757, &g_rvirDiffCmp   },
         { "trading-signals",     63, &g_rvirTsCmp     },
         { "pandas",              12, &g_rvirPandasCmp },
         { "composite",         4609, &g_rvirCompCmp   },
         { "degenerate",       13707, &g_rvirDegenCmp  },
         { "tie",                243, &g_rvirTieCmp    },
         { "edges",           106592, &g_rvirEdgeCmp   },
         { "alias",             9218, &g_rvirAliasCmp  },
      };
      unsigned int c;

      for( c = 0; c < sizeof(cov)/sizeof(cov[0]); c++ )
      {
         if( *cov[c].got != cov[c].want )
         {
            printf( "RVIR Fail: the %s leg compared %d times, not the %d this "
                    "file was written with\n",
                    cov[c].leg, *cov[c].got, cov[c].want );
            return TA_RVIR_VACUOUS;
         }
      }
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* One leg of the reference, rebuilt from TA_STDDEV and two TA_RMA legs exactly
 * as test_rvi.c's differential does, including the four anchoring details that
 * each move the result at the 1e-13 level if dropped. Returns the leg's values
 * from `startIdx` onward.
 */
static ErrorNumber rvir_reference_leg( const TA_Real *in, int nbBars, int startIdx,
                                       int period, int sdPeriod, int unst,
                                       TA_Real *ref, int *nbRef, const char *tag )
{
   static TA_Real sigma[RVIR_CAP], up[RVIR_CAP], dn[RVIR_CAP];
   static TA_Real refUp[RVIR_CAP], refDn[RVIR_CAP];
   TA_Integer begSd, nbSd, begU, nbU, begD, nbD;
   TA_RetCode retCode;
   int k, sdStart;

   sdStart = startIdx - (period-1) - unst;

   retCode = TA_STDDEV( sdStart, nbBars-1, in, sdPeriod, 1.0, &begSd, &nbSd, sigma );
   if( retCode != TA_SUCCESS || begSd != sdStart )
   {
      printf( "RVIR differential [%s N=%d SD=%d u=%d]: TA_STDDEV rc=%d beg %d "
              "expected %d\n", tag, period, sdPeriod, unst, (int)retCode,
              begSd, sdStart );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* The only hand-written step: TA-Lib ships no vector sign gate. A compare
    * and a select add no arithmetic, so the reference stays free of novel
    * numerics. */
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
      printf( "RVIR differential [%s N=%d SD=%d u=%d]: TA_RMA rc=%d beg %d "
              "expected %d\n", tag, period, sdPeriod, unst, (int)retCode,
              begU, (period-1)+unst );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( k = 0; k < nbU; k++ )
   {
      double total = refUp[k] + refDn[k];
      ref[k] = total == 0.0 ? 50.0 : 100.0*(refUp[k]/total);
   }
   *nbRef = nbU;

   return TA_TEST_PASS;
}

/* (1) TA_RVIR against a compose over shipped primitives, BIT-EXACT.
 *
 * Independent of TA_RVI: both legs are rebuilt from TA_STDDEV and TA_RMA, so a
 * shared defect in TA_RVI and TA_RVIR cannot hide here. The average is spelled
 * `0.5*(a+b)` because scaling by one half is exact -- `(a+b)/2` and
 * `0.5*a + 0.5*b` are the same double, which is why the body under test is free
 * to use any of them.
 */
static ErrorNumber test_rvir_differential( const TA_History *history )
{
   static TA_Real refHigh[RVIR_CAP], refLow[RVIR_CAP], out[RVIR_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int a, b, k, unst, anchor, startIdx, lookbackTotal;
   int sdPeriod, period, nbHigh, nbLow;
   ErrorNumber err;

   for( a = 0; a < NB_RVIR_SD; a++ )
   for( b = 0; b < NB_RVIR_TIME; b++ )
   for( unst = 0; unst <= 4; unst += 2 )
   for( anchor = 0; anchor <= 40; anchor += 20 )
   {
      sdPeriod = rvirSdPeriods[a];
      period   = rvirTimePeriods[b];

      TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, (unsigned int)unst );
      lookbackTotal = TA_RVIR_Lookback( period, sdPeriod );
      startIdx = anchor < lookbackTotal ? lookbackTotal : anchor;
      if( startIdx > nbBars-1 )
         continue;

      err = rvir_reference_leg( history->high, nbBars, startIdx, period,
                                sdPeriod, unst, refHigh, &nbHigh, "high" );
      if( err != TA_TEST_PASS )
         goto done;
      err = rvir_reference_leg( history->low, nbBars, startIdx, period,
                                sdPeriod, unst, refLow, &nbLow, "low" );
      if( err != TA_TEST_PASS )
         goto done;

      retCode = TA_RVIR( startIdx, nbBars-1, history->high, history->low,
                         period, sdPeriod, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != startIdx
          || nbElement != nbHigh || nbHigh != nbLow )
      {
         printf( "RVIR differential Fail [N=%d SD=%d u=%d s=%d]: rc=%d (%d,%d) "
                 "expected (%d,%d)\n", period, sdPeriod, unst, startIdx,
                 (int)retCode, begIdx, nbElement, startIdx, nbHigh );
         err = TA_TESTUTIL_TFRR_BAD_BEGIDX;
         goto done;
      }
      RVIR_SERVER_VERIFY_GOTO( startIdx, nbBars-1, nbBars, retCode, begIdx,
                               nbElement, history->high, history->low,
                               period, sdPeriod, out );

      for( k = 0; k < nbElement; k++ )
      {
         double want = 0.5 * ( refHigh[k] + refLow[k] );

         g_rvirDiffCmp++;
         if( memcmp( &want, &out[k], sizeof(double) ) != 0 )
         {
            printf( "RVIR differential Fail [N=%d SD=%d u=%d s=%d] at bar %d: "
                    "got %.17g expected %.17g -- the compose over TA_STDDEV and "
                    "TA_RMA is bit-exact, so any difference is a change in the "
                    "arithmetic or its order\n", period, sdPeriod, unst,
                    startIdx, startIdx+k, out[k], want );
            err = TA_TESTUTIL_TFRR_BAD_CALCULATION;
            goto done;
         }
      }
   }

   err = TA_TEST_PASS;

done:
   TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
   return err;
}

/* Recomputes a golden tuple's whole series and replays it through every live
 * language server. */
static ErrorNumber rvir_golden_series( const TA_History *history, int period,
                                       int sdPeriod, const char *tag,
                                       TA_Real *out, int *begIdx, int *nbElement )
{
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int lookback = TA_RVIR_Lookback( period, sdPeriod );

   retCode = TA_RVIR( 0, nbBars-1, history->high, history->low, period, sdPeriod,
                      begIdx, nbElement, out );
   if( retCode != TA_SUCCESS || *begIdx != lookback
       || *nbElement != nbBars - lookback )
   {
      printf( "RVIR %s Fail [N=%d SD=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
              tag, period, sdPeriod, (int)retCode, *begIdx, *nbElement,
              lookback, nbBars - lookback );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   RVIR_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, *begIdx, *nbElement,
                       history->high, history->low, period, sdPeriod, out );

   return TA_TEST_PASS;
}

/* A golden's bar is hand-transcribed and indexes `out` unchecked: a bar
 * outside the output is a silent out-of-bounds read, not a mismatch. */
static ErrorNumber rvir_check_golden( const char *tag, int period, int sdPeriod,
                                      int bar, double want, const TA_Real *out,
                                      int begIdx, int nbElement,
                                      double relTol, double absTol )
{
   double got, err;
   const char *mode;

   if( bar < begIdx || bar - begIdx >= nbElement )
   {
      printf( "RVIR %s Fail [N=%d SD=%d]: golden bar %d is outside the output "
              "[%d..%d]\n", tag, period, sdPeriod, bar, begIdx,
              begIdx + nbElement - 1 );
      return TA_RVIR_VACUOUS;
   }

   got = out[bar - begIdx];
   if( !checkOracleValue( got, want, relTol, absTol, &err, &mode ) )
   {
      printf( "RVIR %s Fail [N=%d SD=%d] at bar %d: got %.17g expected %.17g "
              "(%s err %.3g, tol rel %g abs %g)\n", tag, period, sdPeriod, bar,
              got, want, mode, err, relTol, absTol );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (2) The frozen trading-signals goldens, plus the cross-language replay. */
static ErrorNumber test_rvir_oracle( const TA_History *history )
{
   static TA_Real out[RVIR_CAP];
   TA_Integer begIdx = 0, nbElement = 0;
   int k, lastPeriod = -1, lastSd = -1;
   ErrorNumber err;

   if( history->nbBars != 252 )
   {
      printf( "RVIR oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", (int)history->nbBars );
      return TA_TEST_PASS;
   }

   for( k = 0; k < NB_RVIR_TS; k++ )
   {
      const RvirGolden *g = &rvirTradingSignals[k];

      if( g->period != lastPeriod || g->sdPeriod != lastSd )
      {
         lastPeriod = g->period;
         lastSd     = g->sdPeriod;
         err = rvir_golden_series( history, lastPeriod, lastSd, "oracle",
                                   out, &begIdx, &nbElement );
         if( err != TA_TEST_PASS )
            return err;
      }

      g_rvirTsCmp++;
      err = rvir_check_golden( "oracle", g->period, g->sdPeriod, g->bar, g->want,
                               out, begIdx, nbElement,
                               RVIR_TS_REL, RVIR_TS_ABS );
      if( err != TA_TEST_PASS )
         return err;
   }

   return TA_TEST_PASS;
}

/* (3) The second oracle, on its converged tail. */
static ErrorNumber test_rvir_pandas( const TA_History *history )
{
   static TA_Real out[RVIR_CAP];
   TA_Integer begIdx = 0, nbElement = 0;
   int k, lastLength = -1;
   ErrorNumber err;

   if( history->nbBars != 252 )
      return TA_TEST_PASS;

   for( k = 0; k < NB_RVIR_PANDAS; k++ )
   {
      const RvirPandasGolden *g = &rvirPandas[k];

      if( g->length != lastLength )
      {
         lastLength = g->length;
         err = rvir_golden_series( history, lastLength, lastLength, "pandas",
                                   out, &begIdx, &nbElement );
         if( err != TA_TEST_PASS )
            return err;
      }

      g_rvirPandasCmp++;
      err = rvir_check_golden( "pandas", g->length, g->length, g->bar, g->want,
                               out, begIdx, nbElement,
                               RVIR_PANDAS_REL, RVIR_PANDAS_ABS );
      if( err != TA_TEST_PASS )
         return err;
   }

   return TA_TEST_PASS;
}

/* (4) The published contract: this function is the average of TA_RVI over the
 * highs and TA_RVI over the lows, bit for bit.
 *
 * STRUCTURALLY TRUE against a body that calls TA_RVI twice. It pins the
 * contract against TA_RVI itself, where leg 1 pins it against TA_STDDEV and
 * TA_RMA.
 */
static ErrorNumber test_rvir_composite( const TA_History *history )
{
   static TA_Real legHigh[RVIR_CAP], legLow[RVIR_CAP], out[RVIR_CAP];
   TA_Integer begIdx, nbElement, begH, nbH, begL, nbL;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int a, b, k, sdPeriod, period;

   for( a = 0; a < NB_RVIR_SD; a++ )
   for( b = 0; b < NB_RVIR_TIME; b++ )
   {
      sdPeriod = rvirSdPeriods[a];
      period   = rvirTimePeriods[b];

      retCode = TA_RVIR( 0, nbBars-1, history->high, history->low,
                         period, sdPeriod, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "RVIR composite Fail [N=%d SD=%d]: rc=%d\n",
                 period, sdPeriod, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      RVIR_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          history->high, history->low, period, sdPeriod, out );

      retCode = TA_RVI( begIdx, nbBars-1, history->high, period, sdPeriod,
                        &begH, &nbH, legHigh );
      if( retCode == TA_SUCCESS )
         retCode = TA_RVI( begIdx, nbBars-1, history->low, period, sdPeriod,
                           &begL, &nbL, legLow );
      if( retCode != TA_SUCCESS || begH != begIdx || begL != begIdx
          || nbH != nbElement || nbL != nbElement )
      {
         printf( "RVIR composite Fail [N=%d SD=%d]: legs rc=%d (%d,%d)/(%d,%d) "
                 "against (%d,%d)\n", period, sdPeriod, (int)retCode,
                 begH, nbH, begL, nbL, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( k = 0; k < nbElement; k++ )
      {
         double want = 0.5 * ( legHigh[k] + legLow[k] );

         g_rvirCompCmp++;
         if( memcmp( &want, &out[k], sizeof(double) ) != 0 )
         {
            printf( "RVIR composite Fail [N=%d SD=%d] at bar %d: got %.17g "
                    "expected %.17g\n", period, sdPeriod, begIdx+k,
                    out[k], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) A series whose high equals its low at every bar: the two legs are the
 * same computation, so this function must return exactly TA_RVI of it.
 *
 * It compares outBegIdx against a function whose lookback is established, so a
 * lookback that drifts by one bar is caught here, as it is by leg 2's first
 * golden bars -- leg 1 anchors its reference at whatever outBegIdx this
 * function reports and follows the drift instead of failing on it.
 */
static ErrorNumber test_rvir_degenerate( const TA_History *history )
{
   static TA_Real out[RVIR_CAP], want[RVIR_CAP];
   TA_Integer begIdx, nbElement, begWant, nbWant;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int a, b, k, unst, sdPeriod, period;
   ErrorNumber err;

   for( a = 0; a < NB_RVIR_SD; a++ )
   for( b = 0; b < NB_RVIR_TIME; b++ )
   for( unst = 0; unst <= 4; unst += 2 )
   {
      sdPeriod = rvirSdPeriods[a];
      period   = rvirTimePeriods[b];

      TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, (unsigned int)unst );

      retCode = TA_RVIR( 0, nbBars-1, history->high, history->high,
                         period, sdPeriod, &begIdx, &nbElement, out );
      if( retCode == TA_SUCCESS )
         retCode = TA_RVI( 0, nbBars-1, history->high, period, sdPeriod,
                           &begWant, &nbWant, want );
      if( retCode != TA_SUCCESS || begIdx != begWant || nbElement != nbWant )
      {
         printf( "RVIR degenerate Fail [N=%d SD=%d u=%d]: rc=%d (%d,%d) against "
                 "TA_RVI (%d,%d) -- with high == low the two legs are the same "
                 "computation, so the warm-up must match too\n",
                 period, sdPeriod, unst, (int)retCode, begIdx, nbElement,
                 begWant, nbWant );
         err = TA_TESTUTIL_TFRR_BAD_BEGIDX;
         goto done;
      }
      RVIR_SERVER_VERIFY_GOTO( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                               history->high, history->high, period, sdPeriod,
                               out );

      for( k = 0; k < nbElement; k++ )
      {
         g_rvirDegenCmp++;
         if( memcmp( &want[k], &out[k], sizeof(double) ) != 0 )
         {
            printf( "RVIR degenerate Fail [N=%d SD=%d u=%d] at bar %d: %.17g "
                    "against TA_RVI's %.17g\n", period, sdPeriod, unst,
                    begIdx+k, out[k], want[k] );
            err = TA_TESTUTIL_TFRR_BAD_CALCULATION;
            goto done;
         }
      }
   }

   err = TA_TEST_PASS;

done:
   TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, 0 );
   return err;
}

/* (6) The tie rule, made observable.
 *
 * At optInTimePeriod == 1 the smoothing has no memory, so each leg is decided
 * by the sign of its own bar alone and takes one of {0, 50, 100}. The average
 * of two such legs takes {0, 25, 50, 75, 100}, and the quarter-points are the
 * bars where one series is flat while the other moves. A tie routed to the
 * down bucket instead of to neither would erase them: both legs would be in
 * {0, 100} and the set would collapse to {0, 50, 100}.
 */
static ErrorNumber test_rvir_tie( const TA_History *history )
{
   static TA_Real out[RVIR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   /* Census of {0, 25, 50, 75, 100} on the 252-bar corpus. The quarter-points
    * are the six single-series tie bars: highs 66, 88, 250 and lows 81, 176,
    * 191, where exactly one leg reads 50. */
   static const int want[5] = { 88, 2, 47, 4, 102 };
   int i, k, count[5] = { 0, 0, 0, 0, 0 };

   if( nbBars != 252 )
      return TA_TEST_PASS;

   retCode = TA_RVIR( 0, nbBars-1, history->high, history->low, 1, 10,
                      &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 9 || nbElement != 243 )
   {
      printf( "RVIR tie Fail: rc=%d (%d,%d) expected (9,243)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   RVIR_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                       history->high, history->low, 1, 10, out );

   for( i = 0; i < nbElement; i++ )
   {
      g_rvirTieCmp++;
      k = out[i] >= 0.0 && out[i] <= 100.0 ? (int)(out[i] / 25.0) : -1;
      if( k < 0 || out[i] != 25.0 * k )
      {
         printf( "RVIR tie Fail at bar %d: %.17g is not one of "
                 "{0, 25, 50, 75, 100}. With no smoothing memory each leg is "
                 "decided by its own bar's sign, so any other value means a leg "
                 "carries state it should not\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      count[k]++;
   }

   for( k = 0; k < 5; k++ )
   {
      if( count[k] != want[k] )
      {
         printf( "RVIR tie Fail: %d bars read %d, expected %d. Ties routed to "
                 "a bucket move bars off 25 and 75\n", count[k], 25*k, want[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (7) Flat input, the two shape edges, and the scratch buffer's extent.
 *
 * A flat window has zero deviation in both legs, so each resolves to 50 through
 * its own zero-total guard and the average is 50 -- without the guard every
 * value below is NaN, and NaN fails the equality.
 *
 * The single-bar range sizes the scratch buffer to exactly one element.
 */
static ErrorNumber test_rvir_edges( void )
{
   static TA_Real high[300], low[300], out[RVIR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int period, sdPeriod, i, lookbackTotal;

   for( i = 0; i < 300; i++ )
   {
      high[i] = 42.0;
      low[i]  = 42.0;
   }

   for( sdPeriod = 2; sdPeriod <= 20; sdPeriod++ )
   for( period = 1; period <= 20; period++ )
   {
      lookbackTotal = TA_RVIR_Lookback( period, sdPeriod );
      retCode = TA_RVIR( 0, 299, high, low, period, sdPeriod,
                         &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != lookbackTotal
          || nbElement != 300 - lookbackTotal )
      {
         printf( "RVIR flat Fail [N=%d SD=%d]: rc=%d (%d,%d)\n",
                 period, sdPeriod, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      RVIR_SERVER_VERIFY( 0, 299, 300, retCode, begIdx, nbElement,
                          high, low, period, sdPeriod, out );
      for( i = 0; i < nbElement; i++ )
      {
         g_rvirEdgeCmp++;
         if( isnan( out[i] ) || out[i] != 50.0 )
         {
            printf( "RVIR flat Fail [N=%d SD=%d] out %d: %.17g, expected exactly "
                    "50.0 (NaN => a leg's zero-total guard is missing)\n",
                    period, sdPeriod, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* One bar of output, requested at the first index that has one: the scratch
    * buffer is exactly one element wide here. */
   lookbackTotal = TA_RVIR_Lookback( 14, 10 );
   retCode = TA_RVIR( lookbackTotal, lookbackTotal, high, low, 14, 10,
                      &begIdx, &nbElement, out );
   g_rvirEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != lookbackTotal || nbElement != 1 )
   {
      printf( "RVIR edge Fail: single-bar range gave rc=%d (%d,%d), expected "
              "(%d,1)\n", (int)retCode, begIdx, nbElement, lookbackTotal );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* A range shorter than the lookback produces nothing, successfully, and
    * allocates nothing on the way. */
   retCode = TA_RVIR( 0, lookbackTotal-1, high, low, 14, 10,
                      &begIdx, &nbElement, out );
   g_rvirEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "RVIR edge Fail: short range gave rc=%d (%d,%d), expected (0,0)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   return TA_TEST_PASS;
}

/* (8) In-place aliasing, outReal over each input in turn, bitwise.
 *
 * Two cases rather than one: the two inputs are consumed by different calls at
 * different points, so aliasing one is not evidence about the other.
 */
static ErrorNumber test_rvir_aliasing( const TA_History *history )
{
   static TA_Real clean[RVIR_CAP], alias[RVIR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int nbBars = (int)history->nbBars;
   int a, b, i, which, sdPeriod, period;
   const TA_Real *aliased;

   for( a = 0; a < NB_RVIR_SD; a++ )
   for( b = 0; b < NB_RVIR_TIME; b++ )
   for( which = 0; which < 2; which++ )
   {
      sdPeriod = rvirSdPeriods[a];
      period   = rvirTimePeriods[b];
      aliased  = which == 0 ? history->high : history->low;

      retCode = TA_RVIR( 0, nbBars-1, history->high, history->low,
                         period, sdPeriod, &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "RVIR alias Fail [N=%d SD=%d %s]: rc=%d\n", period, sdPeriod,
                 which == 0 ? "high" : "low", (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      /* The non-aliased call only: in-place behaviour is a C-side memory
       * property, not something the servers are asked to reproduce. */
      RVIR_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          history->high, history->low, period, sdPeriod, clean );

      for( i = 0; i < nbBars; i++ )
         alias[i] = aliased[i];
      if( which == 0 )
         retCode = TA_RVIR( 0, nbBars-1, alias, history->low, period, sdPeriod,
                            &begIdx2, &nbElement2, alias );
      else
         retCode = TA_RVIR( 0, nbBars-1, history->high, alias, period, sdPeriod,
                            &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "RVIR alias Fail [N=%d SD=%d %s]: rc=%d shape (%d,%d) vs "
                 "(%d,%d)\n", period, sdPeriod, which == 0 ? "high" : "low",
                 (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_rvirAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "RVIR alias Fail [N=%d SD=%d %s] out %d: separate %.17g, "
                    "in-place %.17g -- a store landed under a read the same bar "
                    "still needed\n", period, sdPeriod,
                    which == 0 ? "high" : "low", i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (9) The startIdx/endIdx range sweep. TA_STABLE_CONVERGING for TA_RVI's
 * reason, inherited through both legs: the smoothed legs are IIR recurrences
 * seeded at startIdx - lookback, so an earlier start moves a value by a
 * residual the unstable period bounds. */
typedef struct { int period; int sdPeriod; const TA_Real *high; const TA_Real *low; } RvirRangeParam;

static TA_RetCode rvirRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   RvirRangeParam *p = (RvirRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_RVIR_Lookback( p->period, p->sdPeriod );
   return TA_RVIR( startIdx, endIdx, p->high, p->low, p->period, p->sdPeriod,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_rvir_range( const TA_History *history )
{
   RvirRangeParam param;

   param.period   = 14;
   param.sdPeriod = 10;
   param.high     = history->high;
   param.low      = history->low;

   return doRangeTestEx( rvirRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_RVI,
                         (void *)&param, 1, 0 );
}
