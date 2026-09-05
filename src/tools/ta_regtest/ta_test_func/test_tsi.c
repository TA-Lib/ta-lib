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
 *  090426 MF,CC  First version (issue #360).
 */

/* Description:
 *
 *   Test TA_TSI (True Strength Index).
 *
 *   Legs:
 *     1. DIFFERENTIAL, bit-exact, against a reference built only from shipped
 *        primitives: TA_MOM(1) -> |.| -> TA_EMA(first) -> TA_EMA(second), the
 *        same chain on the signed and the absolute stream, then the divide.
 *        The reference is ANCHORED: for a requested startIdx it begins its
 *        TA_MOM at (refStart - lookback) + 1, so each TA_EMA seeds on exactly
 *        the bars the fused loop seeds on. TA_EMA re-seeds at
 *        startIdx-lookback, so an unanchored reference (always from bar 0) is
 *        a DIFFERENT function once startIdx exceeds the lookback -- leg 2
 *        proves the grid separates the two.
 *
 *        The grid is crossed with an UNSTABLE-PERIOD sweep, and that is the
 *        load-bearing part: the second stage starts at ema_lookback(first),
 *        not (first-1), and the two coincide exactly when the unstable period
 *        is 0 -- which is where every default gate runs. This leg proves the
 *        FUSION, not the formula; legs 3 and 4 are what test the formula.
 *     2. ANCHORING NON-VACUITY. Asserts at least one grid cell where the
 *        anchored and unanchored references disagree.
 *     3. EXTERNAL ORACLE, frozen rows from pandas-ta-classic (see tsiOracle),
 *        replayed on every language server by server_verify.
 *     4. SECOND EXTERNAL ORACLE, trading-signals 8.3.0 -- an implementation
 *        that shares no code and no seeding with either TA-Lib or pandas-ta.
 *        Tail-convergent, so only its converged rows are frozen.
 *     5. EXACT-ARITHMETIC EDGES. A strictly monotone series whose every change
 *        is exactly 0.5 makes the numerator and the denominator bit-identical
 *        chains, so TSI is exactly +100 (or -100 descending) on every bar of a
 *        whole period grid: the sign convention and the tightness of the |TSI|
 *        <= 100 bound in one assertion. An all-flat series drives the
 *        denominator to exactly zero and must read 0.0, never NaN, and carries
 *        an OVER-FIRE CONTROL -- a genuine 1e-5 move must NOT be swallowed.
 *     6. IN-PLACE ALIASING (outReal == inReal), bitwise, over the whole period
 *        grid. The steady loop overwrites outReal[t] while the next bar still
 *        needs inReal[t] as its previous close, so this is the leg that fails
 *        if prevClose stops being carried in a scalar.
 *     7. The startIdx/endIdx range sweep, in the CONVERGING class against
 *        TA_FUNC_UNST_EMA, which TSI consumes twice through ema_lookback.
 *
 *   Cross-language value coverage comes from server_verify in legs 3 and 4
 *   plus the --xlang-hash sweep; the frozen ta_ref_serve predates this
 *   function, so the --codegen value comparison cannot run for it.
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
#define TSI_CAP 300   /* > MAX_NB_TEST_ELEMENT and > nbBars */

/* Leg 3. ABSOLUTE, not relative. The oracle and TA_TSI agree to 2.842e-14 over
 * the frozen rows (worst case, measured 2026-09-04 against the live arm), but
 * one of them sits at a zero crossing where the RELATIVE error is 2.9e-13 --
 * within 4x of failing a 1e-12 relative check while saying nothing about
 * correctness. 1e-12 absolute keeps ~35x margin on the measured gap. */
#define TSI_ORACLE_ABS 1e-12

/* Leg 4. Same absolute form, and the same measured order: 2.132e-14 worst over
 * the rows below. The arm is first-value-seeded, so only rows past its seeding
 * transient are frozen -- see tsiTradingSignals. */
#define TSI_TAIL_ABS 1e-12

typedef struct { int first; int second; int bar; double want; } TsiGolden;

/* Captured on the 252-bar TA_SREF close series
 * (TA_SREF_close_daily_ref_0_PRIV) at %.17g, which round-trips to the same
 * double, by driving pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1,
 * CPython 3.12.3) live over the lossless hex-of-IEEE-bits JSON-RPC transport
 * on 2026-09-04. `bar` is the ABSOLUTE bar index; the output index is
 * bar - begIdx.
 *
 * pandas-ta-classic's momentum/tsi.py is `close.diff() -> ema(slow) ->
 * ema(fast)`, the same chain on `abs(diff)`, then `scalar * num / den`, and
 * its overlap/ema.py deliberately reproduces TA-Lib's SMA-seeded EMA. That
 * last point is why the agreement below is at rounding level rather than a
 * warm-up divergence, and it is also this arm's limit: it corroborates the
 * COMPOSITION, never the seeding. Leg 4's arm is the one that shares nothing.
 *
 * The (13,25) rows are not a duplicate of the (25,13) ones: they are the same
 * two periods applied in the opposite order, and at bar 37 the two differ by
 * 8.0 TSI points. Both orderings return lookback 37, outBegIdx 37, 215
 * elements and TA_SUCCESS, so a swapped parameter is silent in every other
 * gate this repo has -- these rows are what makes it loud.
 *
 * Each tuple's first two output bars are included (that is where seeding error
 * is largest and where it has converged away by bar ~150), plus a spread, plus
 * the bar closest to zero, where an absolute tolerance is least forgiving. */
static const TsiGolden tsiOracle[] =
{
   {  25,  13,  37,    -0.41461229824595841 },
   {  25,  13,  38,      -1.672713855773069 },
   {  25,  13,  90,      44.541914832022627 },
   {  25,  13, 144,      2.4310887126742262 },
   {  25,  13, 183,    0.031799884765040587 },
   {  25,  13, 198,     -28.613219734438267 },
   {  25,  13, 251,      7.3214614559034272 },
   {  13,  25,  37,     -8.4110519229397998 },
   {  13,  25,  38,      -9.362121509652944 },
   {  13,  25,  90,      44.422722547226734 },
   {  13,  25, 144,      2.4291882491146488 },
   {  13,  25, 183,      0.0317280296696862 },
   {  13,  25, 198,     -28.613241033713955 },
   {  13,  25, 251,      7.3214610929914201 },
   {   5,   3,   7,      5.6674369979091166 },
   {   5,   3,   8,     -27.889130683770507 },
   {   5,   3,  68,     -22.227345909791396 },
   {   5,   3, 129,       79.02722194354665 },
   {   5,   3, 190,     -4.6779110594926996 },
   {   5,   3, 219,     0.68859950461539343 },
   {   5,   3, 251,     -33.993598268321328 },
};
#define NB_TSI_ORACLE ((int)(sizeof(tsiOracle)/sizeof(TsiGolden)))

typedef struct { int bar; double want; } TsiTail;

/* Leg 4. trading-signals 8.3.0 (TypeScript, MIT) `ts.TSI`, captured on the
 * same series through ta-lib-oracles/trading_signals_serve on 2026-09-04.
 *
 * Independent of TA-Lib and of pandas-ta, and it is what settles the parameter
 * ORDER from outside this repo: momentum/TSI/TSI.js takes
 * {longPeriod = 25, shortPeriod = 13} and applies the LONG one first, exactly
 * as optInFirstPeriod is applied first here. Its getRequiredInputs() is
 * long + short, and its first value lands at bar long+short-1 -- TA_TSI's
 * lookback, confirmed at every tuple captured. Its zero guard is
 * `doubleSmoothedAbs === 0 ? 0 : ...`, the same exact test, not an epsilon band.
 *
 * TAIL-CONVERGENT: its trend/EMA/EMA.js seeds from the first raw sample
 * instead of an average, so it agrees only once that transient decays. At
 * (5,3) the gap falls under 1e-12 from bar 76 and the rows below (bar 100 and
 * later) agree to 2.132e-14; at (25,13) it is still 2.8e-07 at bar 251, so no
 * row is frozen there and a value comparison at the defaults would be testing
 * the warm-up, not the formula. */
static const TsiTail tsiTradingSignals[] =
{
   { 100,      7.6249903255253262 },
   { 150,     -2.3492620104287201 },
   { 200,      -40.48871219233019 },
   { 240,     -43.177714244761709 },
   { 251,     -33.993598268321328 },
};
#define NB_TSI_TAIL ((int)(sizeof(tsiTradingSignals)/sizeof(TsiTail)))
#define TSI_TAIL_FIRST 5
#define TSI_TAIL_SECOND 3

/* Parameter grid for legs 1, 2, 5 and 6. Carries the published defaults, the
 * same pair in the opposite order (TSI does not sort them), the smallest legal
 * periods, both asymmetric extremes, and a tuple whose lookback exceeds the
 * corpus so the "nothing to evaluate" path is walked. */
static const struct { int first; int second; } tsiGrid[] =
{
   {  25,  13 },
   {  13,  25 },
   {   5,   3 },
   {   2,   2 },
   { 100,   5 },
   {   2, 100 },
   { 200, 100 },
};
#define NB_TSI_GRID ((int)(sizeof(tsiGrid)/sizeof(tsiGrid[0])))

/* startIdx grid. Values ABOVE the lookback are what separate an anchored
 * reference from an unanchored one; 0 and 1 clamp to the lookback and cannot. */
static const int tsiStartGrid[] = { 0, 1, 60, 100, 175, 251 };
#define NB_TSI_START ((int)(sizeof(tsiStartGrid)/sizeof(tsiStartGrid[0])))

/* Unstable-period sweep. 0 must be present (it is the shipped default); the
 * non-zero values are the only ones that separate a lookback-anchored stage
 * boundary from a (period-1) one. */
static const int tsiUnstGrid[] = { 0, 1, 3, 7 };
#define NB_TSI_UNST ((int)(sizeof(tsiUnstGrid)/sizeof(tsiUnstGrid[0])))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_tsiDiffCmp;
static int g_tsiOracleCmp;
static int g_tsiTailCmp;
static int g_tsiEdgeCmp;
static int g_tsiAliasCmp;
static int g_tsiAnchorCells;

/**** Local functions declarations. ****/
static ErrorNumber tsi_build_reference( const TA_Real *in, int first, int second,
                                        int base, int endIdx,
                                        double *ref, int *refBeg, int *refNb );
static ErrorNumber test_tsi_differential( const TA_History *history );
static ErrorNumber test_tsi_anchoring( const TA_History *history );
static ErrorNumber test_tsi_oracle( const TA_History *history );
static ErrorNumber test_tsi_tail_oracle( const TA_History *history );
static ErrorNumber test_tsi_edges( void );
static ErrorNumber test_tsi_aliasing( const TA_History *history );
static ErrorNumber test_tsi_range( const TA_Real *in );

/**** Global functions definitions. ****/
ErrorNumber test_func_tsi( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );

   g_tsiDiffCmp = g_tsiOracleCmp = g_tsiTailCmp = 0;
   g_tsiEdgeCmp = g_tsiAliasCmp = g_tsiAnchorCells = 0;

   err = test_tsi_differential( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_tsi_anchoring( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_tsi_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_tsi_tail_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_tsi_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_tsi_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_tsi_range( history->close );
   if( err != TA_TEST_PASS )
      return err;

   /* Every leg above restores it, but a future leg might not. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( (int)history->nbBars == 252
       && ( g_tsiDiffCmp != 19200 || g_tsiOracleCmp != NB_TSI_ORACLE
            || g_tsiTailCmp != NB_TSI_TAIL || g_tsiEdgeCmp != 58784
            || g_tsiAliasCmp != 1223 || g_tsiAnchorCells != 20 ) )
   {
      printf( "TSI Fail: coverage counters (diff %d, oracle %d, tail %d, edges %d, "
              "alias %d, separating anchor cells %d) are not what this file was "
              "written with (19200, %d, %d, 58784, 1223, 20)\n",
              g_tsiDiffCmp, g_tsiOracleCmp, g_tsiTailCmp, g_tsiEdgeCmp,
              g_tsiAliasCmp, g_tsiAnchorCells, NB_TSI_ORACLE, NB_TSI_TAIL );
      return TA_TSI_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* Build TSI from shipped primitives only.
 *
 * `base` is the first input bar the caller is allowed to consume. Passing
 * (refStart - lookback) anchors the reference exactly where TA_TSI anchors;
 * passing 0 builds the UNANCHORED variant leg 2 contrasts against.
 */
static ErrorNumber tsi_build_reference( const TA_Real *in, int first, int second,
                                        int base, int endIdx,
                                        double *ref, int *refBeg, int *refNb )
{
   static TA_Real mom[TSI_CAP], absMom[TSI_CAP];
   static TA_Real e1[TSI_CAP], e2[TSI_CAP], f1[TSI_CAP], f2[TSI_CAP];
   TA_Integer begMom, nbMom, begE1, nbE1, begE2, nbE2, begF1, nbF1, begF2, nbF2;
   int i;

   *refBeg = 0;
   *refNb  = 0;

   if( TA_MOM( base+1, endIdx, in, 1, &begMom, &nbMom, mom ) != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( nbMom <= 0 )
      return TA_TEST_PASS;
   for( i = 0; i < nbMom; i++ )
      absMom[i] = fabs( mom[i] );

   if( TA_EMA( 0, nbMom-1, mom, first, &begE1, &nbE1, e1 ) != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( TA_EMA( 0, nbMom-1, absMom, first, &begF1, &nbF1, f1 ) != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( begF1 != begE1 || nbF1 != nbE1 )
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   if( nbE1 <= 0 )
      return TA_TEST_PASS;

   if( TA_EMA( 0, nbE1-1, e1, second, &begE2, &nbE2, e2 ) != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( TA_EMA( 0, nbF1-1, f1, second, &begF2, &nbF2, f2 ) != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( begF2 != begE2 || nbF2 != nbE2 )
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   if( nbE2 <= 0 )
      return TA_TEST_PASS;

   for( i = 0; i < nbE2; i++ )
      ref[i] = ( f2[i] > 0.0 ) ? (100.0 * e2[i]) / f2[i] : 0.0;

   *refBeg = begMom + begE1 + begE2;
   *refNb  = (int)nbE2;
   return TA_TEST_PASS;
}

/* (1) DIFFERENTIAL, plus the |TSI| <= 100 bound on every value it computes. */
static ErrorNumber test_tsi_differential( const TA_History *history )
{
   static TA_Real out[TSI_CAP], ref[TSI_CAP];
   TA_RetCode rc;
   TA_Integer beg, nb;
   ErrorNumber err;
   int u, s, g, i, nbBars = (int)history->nbBars;

   for( u = 0; u < NB_TSI_UNST; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, (TA_Integer)tsiUnstGrid[u] );

      for( s = 0; s < NB_TSI_START; s++ )
      {
         for( g = 0; g < NB_TSI_GRID; g++ )
         {
            int first = tsiGrid[g].first, second = tsiGrid[g].second;
            int startIdx = tsiStartGrid[s];
            int lookback, refStart, refBeg, refNb;

            rc = TA_TSI( startIdx, nbBars-1, history->close, first, second,
                         &beg, &nb, out );
            if( rc != TA_SUCCESS )
            {
               printf( "TSI differential Fail [unst %d start %d (%d,%d)]: retCode %d\n",
                       tsiUnstGrid[u], startIdx, first, second, (int)rc );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }

            lookback = TA_TSI_Lookback( first, second );
            refStart = startIdx < lookback ? lookback : startIdx;
            if( refStart > nbBars-1 )
            {
               if( nb != 0 )
               {
                  printf( "TSI differential Fail [unst %d start %d (%d,%d)]: "
                          "expected no output, got nb=%d\n",
                          tsiUnstGrid[u], startIdx, first, second, (int)nb );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_BEGIDX;
               }
               continue;
            }

            if( beg != refStart || nb != nbBars - refStart )
            {
               printf( "TSI differential Fail [unst %d start %d (%d,%d)]: "
                       "range (%d,%d), expected (%d,%d)\n",
                       tsiUnstGrid[u], startIdx, first, second,
                       (int)beg, (int)nb, refStart, nbBars - refStart );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }

            err = tsi_build_reference( history->close, first, second,
                                       refStart - lookback, nbBars-1,
                                       ref, &refBeg, &refNb );
            if( err != TA_TEST_PASS )
            {
               printf( "TSI differential Fail [unst %d start %d (%d,%d)]: "
                       "reference construction failed\n",
                       tsiUnstGrid[u], startIdx, first, second );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return err;
            }
            if( refBeg != beg || refNb != nb )
            {
               printf( "TSI differential Fail [unst %d start %d (%d,%d)]: "
                       "reference range (%d,%d) vs (%d,%d)\n",
                       tsiUnstGrid[u], startIdx, first, second,
                       refBeg, refNb, (int)beg, (int)nb );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }

            for( i = 0; i < nb; i++ )
            {
               g_tsiDiffCmp++;
               if( out[i] != ref[i] )
               {
                  printf( "TSI differential Fail [unst %d start %d (%d,%d)] bar %d: "
                          "%.17g vs %.17g (delta %.3e)\n",
                          tsiUnstGrid[u], startIdx, first, second, (int)beg + i,
                          out[i], ref[i], out[i] - ref[i] );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               /* |EMA(EMA(m))| <= EMA(EMA(|m|)) for non-negative weights, so
                * the ratio cannot leave [-100,100]. Free on every cell already
                * computed. */
               if( !( fabs(out[i]) <= 100.0 ) )
               {
                  printf( "TSI bound Fail [unst %d start %d (%d,%d)] bar %d: "
                          "|TSI| = %.17g exceeds 100\n",
                          tsiUnstGrid[u], startIdx, first, second, (int)beg + i,
                          fabs(out[i]) );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;
}

/* (2) ANCHORING NON-VACUITY: the grid must contain a cell where the anchored
 * reference and an unanchored one (always from bar 0) disagree. Without such a
 * cell, leg 1 would pass against a reference that ignores startIdx. */
static ErrorNumber test_tsi_anchoring( const TA_History *history )
{
   static TA_Real a[TSI_CAP], b[TSI_CAP];
   int s, g, i, nbBars = (int)history->nbBars;
   int nbCells = 0;

   for( s = 0; s < NB_TSI_START; s++ )
   {
      for( g = 0; g < NB_TSI_GRID; g++ )
      {
         int first = tsiGrid[g].first, second = tsiGrid[g].second;
         int lookback = TA_TSI_Lookback( first, second );
         int refStart = tsiStartGrid[s] < lookback ? lookback : tsiStartGrid[s];
         int aBeg, aNb, bBeg, bNb;

         if( refStart > nbBars-1 )
            continue;
         if( tsi_build_reference( history->close, first, second,
                                  refStart - lookback, nbBars-1,
                                  a, &aBeg, &aNb ) != TA_TEST_PASS )
            continue;
         if( tsi_build_reference( history->close, first, second, 0,
                                  nbBars-1, b, &bBeg, &bNb ) != TA_TEST_PASS )
            continue;
         if( aNb <= 0 || bNb <= 0 )
            continue;

         /* Compare on the overlap, at the anchored run's first bar. */
         i = aBeg - bBeg;
         if( i >= 0 && i < bNb && a[0] != b[i] )
            nbCells++;
      }
   }

   g_tsiAnchorCells = nbCells;
   if( nbCells == 0 )
   {
      printf( "TSI anchoring Fail: no cell separates the anchored reference from "
              "the unanchored one; the differential leg would pass against a "
              "reference that ignores startIdx. Restore a startIdx well above "
              "the lookback to tsiStartGrid.\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (3) The frozen pandas-ta-classic goldens, plus the cross-language replay. */
static ErrorNumber test_tsi_oracle( const TA_History *history )
{
   static TA_Real out[TSI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastFirst = -1, lastSecond = -1;

   if( nbBars != 252 )
   {
      printf( "TSI oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;
   retCode = TA_SUCCESS;

   for( k = 0; k < NB_TSI_ORACLE; k++ )
   {
      double got, err;
      const char *mode;

      if( tsiOracle[k].first != lastFirst || tsiOracle[k].second != lastSecond )
      {
         int lookback;

         lastFirst  = tsiOracle[k].first;
         lastSecond = tsiOracle[k].second;
         lookback   = TA_TSI_Lookback( lastFirst, lastSecond );

         retCode = TA_TSI( 0, nbBars-1, history->close, lastFirst, lastSecond,
                           &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lookback
             || nbElement != nbBars - lookback )
         {
            printf( "TSI oracle Fail [(%d,%d)]: rc=%d (%d,%d) expected (%d,%d)\n",
                    lastFirst, lastSecond, (int)retCode, begIdx, nbElement,
                    lookback, nbBars - lookback );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[2];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastFirst;
            optIn[1] = (double)lastSecond;
            e = server_verify( "TSI", 0, nbBars-1, nbBars,
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
               printf( "TSI oracle [(%d,%d)]: compared no server despite live "
                       "pipes\n", lastFirst, lastSecond );
               return TA_TSI_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar under the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( tsiOracle[k].bar < begIdx || tsiOracle[k].bar - begIdx >= nbElement )
      {
         printf( "TSI oracle Fail [(%d,%d)]: golden bar %d is outside the output "
                 "[%d..%d]\n", tsiOracle[k].first, tsiOracle[k].second,
                 tsiOracle[k].bar, begIdx, begIdx + nbElement - 1 );
         return TA_TSI_VACUOUS;
      }

      got = out[tsiOracle[k].bar - begIdx];
      g_tsiOracleCmp++;
      if( !checkOracleValue( got, tsiOracle[k].want, 0.0, TSI_ORACLE_ABS,
                             &err, &mode ) )
      {
         printf( "TSI oracle Fail [(%d,%d)] at bar %d: got %.17g expected %.17g "
                 "(%s err %.3g, tol abs %g)\n",
                 tsiOracle[k].first, tsiOracle[k].second, tsiOracle[k].bar,
                 got, tsiOracle[k].want, mode, err, TSI_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4) The second, fully independent oracle, on its converged tail. */
static ErrorNumber test_tsi_tail_oracle( const TA_History *history )
{
   static TA_Real out[TSI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int lookback = TA_TSI_Lookback( TSI_TAIL_FIRST, TSI_TAIL_SECOND );
   int k;

   if( nbBars != 252 )
      return TA_TEST_PASS;

   retCode = TA_TSI( 0, nbBars-1, history->close, TSI_TAIL_FIRST, TSI_TAIL_SECOND,
                     &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != lookback
       || nbElement != nbBars - lookback )
   {
      printf( "TSI tail oracle Fail: rc=%d (%d,%d) expected (%d,%d)\n",
              (int)retCode, begIdx, nbElement, lookback, nbBars - lookback );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   if( server_verify_active() )
   {
      double optIn[2];
      ErrorNumber e;
      int cmpBefore = server_verify_comparisons();

      optIn[0] = (double)TSI_TAIL_FIRST;
      optIn[1] = (double)TSI_TAIL_SECOND;
      e = server_verify( "TSI", 0, nbBars-1, nbBars,
                         retCode, begIdx, nbElement,
                         (const TA_Real*[]){ history->close, NULL },
                         optIn, 2,
                         (const TA_Real*[]){ out, NULL }, NULL );
      if( e != TA_TEST_PASS )
         return e;
      if( server_verify_comparisons() == cmpBefore )
      {
         printf( "TSI tail oracle: compared no server despite live pipes\n" );
         return TA_TSI_VACUOUS;
      }
   }

   for( k = 0; k < NB_TSI_TAIL; k++ )
   {
      double got, err;
      const char *mode;

      if( tsiTradingSignals[k].bar < begIdx
          || tsiTradingSignals[k].bar - begIdx >= nbElement )
      {
         printf( "TSI tail oracle Fail: golden bar %d is outside the output "
                 "[%d..%d]\n", tsiTradingSignals[k].bar, begIdx,
                 begIdx + nbElement - 1 );
         return TA_TSI_VACUOUS;
      }

      got = out[tsiTradingSignals[k].bar - begIdx];
      g_tsiTailCmp++;
      if( !checkOracleValue( got, tsiTradingSignals[k].want, 0.0, TSI_TAIL_ABS,
                             &err, &mode ) )
      {
         printf( "TSI tail oracle Fail at bar %d: got %.17g expected %.17g "
                 "(%s err %.3g, tol abs %g)\n",
                 tsiTradingSignals[k].bar, got, tsiTradingSignals[k].want,
                 mode, err, TSI_TAIL_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) Exact-arithmetic edges.
 *
 * Every close below is an exact binary fraction and every change is exactly
 * ±0.5, so the numerator chain and the denominator chain hold bit-identical
 * magnitudes at every stage and the assertions are equalities, not tolerances.
 *
 *   - A strictly monotone series reads exactly +100 rising and exactly -100
 *     falling, on every bar and every period pair. That pins the SIGN
 *     convention and the tightness of the |TSI| <= 100 bound at once: the
 *     bound is reached, so leg 1's inequality is not vacuously slack.
 *   - An all-flat series drives both chains to exactly zero. Non-vacuous by
 *     construction -- without the guard the divide yields NaN, and NaN fails
 *     the equality below.
 *   - OVER-FIRE CONTROL: a genuine but tiny 1e-5 monotone move must NOT be
 *     swallowed by that guard. A fixed epsilon band in place of the exact test
 *     would report 0 here instead of ±100 (issue #253).
 */
static ErrorNumber test_tsi_edges( void )
{
   static TA_Real in[128], out[TSI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int first, second, i, sign;

   for( sign = 1; sign >= -1; sign -= 2 )
   {
      for( i = 0; i < 100; i++ )
         in[i] = 64.0 + (double)sign * 0.5 * (double)i;

      for( first = 2; first <= 20; first++ )
      {
         for( second = 2; second <= 20; second++ )
         {
            double want = (double)sign * 100.0;
            int lookback = TA_TSI_Lookback( first, second );

            retCode = TA_TSI( 0, 99, in, first, second, &begIdx, &nbElement, out );
            if( retCode != TA_SUCCESS || begIdx != lookback
                || nbElement != 100 - lookback )
            {
               printf( "TSI monotone Fail [(%d,%d) sign=%d]: rc=%d (%d,%d) "
                       "expected (%d,%d)\n", first, second, sign, (int)retCode,
                       begIdx, nbElement, lookback, 100 - lookback );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            for( i = 0; i < nbElement; i++ )
            {
               g_tsiEdgeCmp++;
               if( out[i] != want )
               {
                  printf( "TSI monotone Fail [(%d,%d) sign=%d] out %d: %.17g, "
                          "expected exactly %.17g -- every change has the same "
                          "sign, so the two smoothing chains carry bit-identical "
                          "magnitudes\n", first, second, sign, i, out[i], want );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   for( i = 0; i < 100; i++ )
      in[i] = 42.0;
   for( first = 2; first <= 20; first++ )
   {
      retCode = TA_TSI( 0, 99, in, first, 3, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || nbElement <= 0 )
      {
         printf( "TSI flat Fail [(%d,3)]: rc=%d nb=%d\n",
                 first, (int)retCode, nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_tsiEdgeCmp++;
         if( isnan( out[i] ) || out[i] != 0.0 )
         {
            printf( "TSI flat Fail [(%d,3)] out %d: %.17g, expected exactly 0.0 "
                    "(NaN => the zero-denominator guard is missing)\n",
                    first, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   for( i = 0; i < 100; i++ )
      in[i] = 100.0 + 1.0e-5 * (double)i;
   retCode = TA_TSI( 0, 99, in, 5, 3, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement <= 0 )
   {
      printf( "TSI flat control Fail: rc=%d nb=%d\n", (int)retCode, nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nbElement; i++ )
   {
      g_tsiEdgeCmp++;
      if( fabs( out[i] - 100.0 ) > 1.0e-9 )
      {
         printf( "TSI flat control Fail out %d: a genuine 1e-5 move was swallowed "
                 "by the zero guard (expected +100, got %.17g)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (6) In-place aliasing, outReal == inReal, bitwise.
 *
 * This pins the calling convention; it does NOT discriminate a scalar carry
 * from an array re-read. TSI's store index trails its lowest read by
 * startIdx >= tsi_lookback >= 3, so the write can never reach a slot a later
 * bar still needs, and dropping the prevClose scalar leaves this leg green.
 * A body whose lookback shrank to 0 is what it would catch.
 */
static ErrorNumber test_tsi_aliasing( const TA_History *history )
{
   static TA_Real clean[TSI_CAP], alias[TSI_CAP];
   TA_RetCode retCode;
   TA_Integer beg, nb, beg2, nb2;
   int nbBars = (int)history->nbBars;
   int g, i;

   for( g = 0; g < NB_TSI_GRID; g++ )
   {
      int first = tsiGrid[g].first, second = tsiGrid[g].second;

      retCode = TA_TSI( 0, nbBars-1, history->close, first, second,
                        &beg, &nb, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "TSI alias Fail [(%d,%d)]: rc=%d\n", first, second, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = history->close[i];
      retCode = TA_TSI( 0, nbBars-1, alias, first, second, &beg2, &nb2, alias );
      if( retCode != TA_SUCCESS || beg2 != beg || nb2 != nb )
      {
         printf( "TSI alias Fail [(%d,%d)]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 first, second, (int)retCode, beg2, nb2, beg, nb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nb; i++ )
      {
         g_tsiAliasCmp++;
         if( clean[i] != alias[i] )
         {
            printf( "TSI alias Fail [(%d,%d)] out %d: separate %.17g, in-place "
                    "%.17g -- a store landed under the previous close the next "
                    "bar still needed\n", first, second, i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) The startIdx/endIdx range sweep. TA_STABLE_CONVERGING against
 * TA_FUNC_UNST_EMA: both stages are the EMA recurrence, seeded as ema.c seeds,
 * so a later startIdx converges toward the same trajectory rather than
 * reproducing it. */
typedef struct { int first; int second; const TA_Real *in; } TsiRangeParam;

static TA_RetCode tsiRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   TsiRangeParam *p = (TsiRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_TSI_Lookback( p->first, p->second );
   return TA_TSI( startIdx, endIdx, p->in, p->first, p->second,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_tsi_range( const TA_Real *in )
{
   TsiRangeParam param;

   param.first  = 25;
   param.second = 13;
   param.in     = in;

   return doRangeTestEx( tsiRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_EMA,
                         (void *)&param, 1, 0 );
}
