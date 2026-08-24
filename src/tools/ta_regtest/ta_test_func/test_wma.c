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
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  082426 MF,CC Initial coding (#255).
 *
 */

/* Description:
 *     Numerical-reference tests for TA_WMA.
 *
 *     TA_WMA carries periodSum and periodSub as running totals across bars, and
 *     `periodSum -= periodSub` is the same weight-shifting identity the
 *     LINEARREG family uses -- which is why WMA had the #254 defect and TA_SMA,
 *     whose output lives at its own sum's scale, does not. Before #255 the
 *     residue grew without bound in the length of the call: 1.41e-08 at 200000
 *     bars against a 1e-10 absolute tier, over the tier from ~10000 bars on
 *     ordinary closes and from ~1000 with one large print.
 *
 *     What test_ma.c already had for WMA is hand-written expected values at 2-6
 *     significant digits on 252 bars. That cannot see a 1e-8 drift, and it is
 *     the same gap #251 closed for the LINEARREG family.
 *
 *     THE ORACLE IS INDEPENDENT, TWICE OVER. scripts/gen_test_reference.py
 *     evaluates TA_WMA in EXACT RATIONAL arithmetic over the committed
 *     datasets -- every input is a double, so every weighted product and every
 *     sum is exact -- and bakes the result. ta_test_ref_wma() computes the same
 *     thing at run time in compensated double-double. test_reference.c requires
 *     the second to reproduce the first, so a leg here that compares the
 *     shipped function against either is anchored to arithmetic that shares no
 *     code with TA-Lib and cannot be co-wrong with it.
 *
 *     MEASURED RESOLUTION: this suite detects a uniform relative perturbation
 *     of TA_WMA's output at 1e-12, and is blind at 1e-14 (mutation sweep on the
 *     generated C, mutation verified applied before each run). W1's Wilkinson
 *     leg is what fires. That is the same tier ta_regtest/CLAUDE.md records for
 *     STDDEV/VAR and BETA.
 *
 *     THE WEIGHTS ARE OLDEST=1, NEWEST=period. Taken from the shipped body, not
 *     from the literature, where the reverse convention is equally common and
 *     would produce a plausible wrong answer rather than an obviously wrong one.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_test_reference.h"

/**** Local declarations. ****/

#define WMA_EPS      2.2204460492503131e-16
#define WMA_LONG_N   8000

/* (W2) Worst last-quarter / first-quarter range disagreement. A bounded
 * residue gives ~1; the pre-#255 recurrence measured 20.9 / 2.6 / 117.0 / 14.1
 * at periods 2/5/14/30. Post-fix worst is 1.6, so 5.0 sits ~3x above what the
 * fix produces and ~4x below what its absence produces. */
#define WMA_GROWTH_BOUND   5.0

/* (W3) TA_STABLE_EPSILON is TA_REAL_EQ(a,b,1e-10) -- an ABSOLUTE band -- and
 * this leg asserts that contract directly on a corpus carrying a large print.
 * A ratio against the clean run was the first shape tried and is WRONG for the
 * shipped design: without an outlier trigger a print's residue legitimately
 * persists until the next periodic re-anchor, so the ratio is ~9 while the
 * absolute figure is 1.4e-11, i.e. 7x INSIDE the contract. The contract is the
 * claim; the ratio was measuring a property WMA does not promise. */
#define WMA_CONTAM_TIER    1.0e-10

/* (W1) The shipped function against the baked exact-rational goldens, as a
 * multiple of eps scaled by the window's own magnitude.
 *
 * Measured worst is 20.1, bisected: the leg fails at 20.0 and passes at 21.0,
 * bound by the LADDER at period 14, window 24 -- 1e7-scale values, where the
 * recurrence's residue plus the reseed lands ~20 eps of the window magnitude
 * away from the exact rational answer. 60 is ~3x that, the convention
 * ta_test_legacy.c and test_linearreg.c both use.
 *
 * The first draft of this comment said "measured worst 4.8" and set 30. Both
 * numbers were written rather than measured: the real worst is 4x higher, and
 * 30 was 1.5x margin rather than the 6x claimed. 1.5x is not enough for a bound
 * read on MSVC and AArch64 as well as here -- the same shape of portability hole
 * #251 closed when it replaced `long double` in the oracles. */
#define WMA_GOLDEN_C       60.0

static ErrorNumber test_wma_goldens( void );
static ErrorNumber test_wma_growth( void );
static ErrorNumber test_wma_contamination( void );
static int wma_range_worst( const double *y, int n, int period, int fromBar,
                            double *outFirst, double *outLast );

static double wma_long[WMA_LONG_N];
static double wma_spiked[WMA_LONG_N];
static double wma_out[WMA_LONG_N];

/**** Global functions definitions. ****/
ErrorNumber test_func_wma( TA_History *history )
{
   ErrorNumber retValue;

   (void)history;   /* every leg below builds its own series */

   retValue = test_wma_goldens();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed WMA goldens (#255) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_wma_growth();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed WMA drift growth (#255) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_wma_contamination();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed WMA print contamination (#255) (Code=%d)\n", __FILE__, retValue ); return retValue; }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* The magnitude a window's answer lives on: WMA is a convex combination of the
 * window, so |sum of |y| * weight| / divider bounds it, and that is what the
 * rounding is proportional to. */
static double wma_scale( const double *y, int s, int period )
{
   double acc = 0.0, divider = (double)period * ( period + 1 ) / 2.0;
   int k;
   for( k = 0; k < period; k++ ) acc += fabs( y[s+k] ) * (double)( k + 1 );
   return acc / divider;
}

/* (W1) The shipped function against EXACT RATIONAL goldens, over the two
 * committed corpora the LINEARREG family is pinned on -- Wilkinson's nasty.dat
 * for conditioning and the sliding-sum ladder for a large print followed by a
 * level shift.
 *
 * Goldens, not the runtime oracle, and on COMMITTED data rather than a
 * generated series: a table cannot drift when the ABI changes, and a series
 * baked into the repository is the only thing a golden can honestly pin. */
static ErrorNumber test_wma_goldens( void )
{
   TA_Integer b, nb;
   TA_RetCode rc;
   int i, k, t;
   static const double *const gLadder[TA_TEST_REF_GOLDEN_LADDER_PERIODS_N] = {
      ta_test_ref_golden_ladder_p2_wma,  ta_test_ref_golden_ladder_p5_wma,
      ta_test_ref_golden_ladder_p14_wma, ta_test_ref_golden_ladder_p30_wma };
   static const double *const gWilk[TA_TEST_REF_WILKINSON_NB_SERIES] = {
      ta_test_ref_golden_wilkinson_wma_x,     ta_test_ref_golden_wilkinson_wma_round,
      ta_test_ref_golden_wilkinson_wma_big,   ta_test_ref_golden_wilkinson_wma_little,
      ta_test_ref_golden_wilkinson_wma_huge,  ta_test_ref_golden_wilkinson_wma_tiny,
      ta_test_ref_golden_wilkinson_wma_zero };

   /* Wilkinson, period 9 over 9 bars: exactly one window per series. */
   for( i = 0; i < TA_TEST_REF_WILKINSON_NB_SERIES; i++ )
   {
      const double *y = ta_test_ref_wilkinson_series[i];
      double tol;
      rc = TA_WMA( 0, TA_TEST_REF_WILKINSON_N-1, y, 9, &b, &nb, wma_out );
      if( rc != TA_SUCCESS || nb != 1 )
      {
         printf( "WMA #255 [wilkinson %s]: rc=%d nb=%d (wanted SUCCESS,1)\n",
                 ta_test_ref_wilkinson_names[i], (int)rc, (int)nb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      tol = WMA_GOLDEN_C * WMA_EPS * wma_scale( y, 0, 9 );
      if( fabs( wma_out[0] - gWilk[i][0] ) > tol )
      {
         printf( "WMA #255 [wilkinson %s]: got %.17g want %.17g (|diff| %.3g > %.3g)\n",
                 ta_test_ref_wilkinson_names[i], wma_out[0], gWilk[i][0],
                 fabs( wma_out[0] - gWilk[i][0] ), tol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* The ladder, every window at each pinned period. A FULL-RANGE call, so the
    * recurrence -- not the priming scan -- produces every bar after the first,
    * which is what makes this leg see the residue at all. */
   for( t = 0; t < TA_TEST_REF_GOLDEN_LADDER_PERIODS_N; t++ )
   {
      const int period = ta_test_ref_golden_ladder_periods[t];
      rc = TA_WMA( 0, TA_TEST_REF_LADDER_N-1, ta_test_ref_ladder, period,
                   &b, &nb, wma_out );
      if( rc != TA_SUCCESS || (int)nb != ta_test_ref_golden_ladder_counts[t] )
      {
         printf( "WMA #255 [ladder p=%d]: rc=%d nb=%d (wanted SUCCESS,%d)\n",
                 period, (int)rc, (int)nb, ta_test_ref_golden_ladder_counts[t] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( k = 0; k < (int)nb; k++ )
      {
         double tol = WMA_GOLDEN_C * WMA_EPS
                    * wma_scale( ta_test_ref_ladder, k, period );
         if( fabs( wma_out[k] - gLadder[t][k] ) > tol )
         {
            printf( "WMA #255 [ladder p=%d]: window %d got %.17g want %.17g "
                    "(|diff| %.3g > %.3g)\n", period, k, wma_out[k], gLadder[t][k],
                    fabs( wma_out[k] - gLadder[t][k] ), tol );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   return TA_TEST_PASS;
}

/* Geometric random walk, same shape/seed/step as test_linearreg.c's corpora so
 * the two files' numbers are directly comparable. */
static void wma_corpus( double *dst, int n )
{
   int i;
   ta_test_ref_lcg_seed( 0xBEEFu );
   dst[0] = 100.0;
   for( i = 1; i < n; i++ )
      dst[i] = dst[i-1] * ( 1.0 + 0.015 * ta_test_ref_lcg_sym() );
}

/* Worst |full-range - single-window| over y, split first/last quarter. When
 * fromBar >= 0 only bars whose window sits entirely after it are scored, all
 * into *outLast.
 *
 * The single-window call (startIdx == endIdx == bar) produces its value from
 * the priming scan alone -- no recurrence, no re-anchor, no history -- so this
 * is the same function answering about the same bar with nothing accumulated,
 * and needs no oracle to be non-vacuous. */
static int wma_range_worst( const double *y, int n, int period, int fromBar,
                            double *outFirst, double *outLast )
{
   TA_Integer b, nb, b2, nb2;
   TA_RetCode rc;
   double one[8];
   int k, nbOut;

   *outFirst = 0.0;
   *outLast  = 0.0;
   rc = TA_WMA( 0, n-1, y, period, &b, &nb, wma_out );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "WMA #255: full-range call rc=%d nb=%d (period=%d)\n",
              (int)rc, (int)nb, period );
      return 1;
   }
   nbOut = (int)nb;
   for( k = 0; k < nbOut; k++ )
   {
      const int bar = (int)b + k;
      double d;
      if( fromBar >= 0 && bar - period + 1 <= fromBar ) continue;
      rc = TA_WMA( bar, bar, y, period, &b2, &nb2, one );
      if( rc != TA_SUCCESS || nb2 != 1 || (int)b2 != bar )
      {
         printf( "WMA #255: single-window call rc=%d beg=%d nb=%d (wanted SUCCESS,%d,1)\n",
                 (int)rc, (int)b2, (int)nb2, bar );
         return 1;
      }
      d = fabs( wma_out[k] - one[0] );
      if( fromBar >= 0 )                { if( d > *outLast  ) *outLast  = d; }
      else if( k < nbOut / 4 )          { if( d > *outFirst ) *outFirst = d; }
      else if( k >= 3 * ( nbOut / 4 ) ) { if( d > *outLast  ) *outLast  = d; }
   }
   return 0;
}

/* (W2) #255 -- the periodic re-anchor.
 *
 * The claim is not "the error is small" but "it does not grow with how far into
 * the call a bar sits". Ratios, not magnitudes, so nothing platform-dependent
 * is pinned and no oracle is needed.
 *
 * SABOTAGE-PROVEN: removing the re-anchor fails this at 117.0 (period 14). */
static ErrorNumber test_wma_growth( void )
{
   static const int periods[4] = { 2, 5, 14, 30 };
   int p;

   wma_corpus( wma_long, WMA_LONG_N );
   for( p = 0; p < 4; p++ )
   {
      double first, last;
      if( wma_range_worst( wma_long, WMA_LONG_N, periods[p], -1, &first, &last ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      /* Compared as a product: with first == 0 this passes only if last is 0
       * too, instead of dividing by zero or reporting a tidy 0.0 ratio for
       * infinite growth. */
      if( last > WMA_GROWTH_BOUND * first )
      {
         printf( "WMA #255 [drift growth]: period=%d over %d bars -- worst range "
                 "disagreement grows down the call: first quarter %.3g, last quarter "
                 "%.3g (ratio %.1f > %.1f). periodSum/periodSub are not being "
                 "re-anchored.\n", periods[p], WMA_LONG_N, first, last,
                 first > 0.0 ? last / first : 0.0, WMA_GROWTH_BOUND );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (W3) #255 -- a large print must not contaminate later bars.
 *
 * The residue's size was set by the largest value the totals had ever held, so
 * one print inflated every later bar permanently. Bars whose window is entirely
 * past the print have no legitimate reason to know it happened.
 *
 * WHY THIS ASSERTS THE TIER AND NOT A RATIO, unlike test_linearreg.c's L11:
 * WMA carries no outlier trigger, so a print's residue legitimately survives
 * until the next periodic re-anchor and the spiked/clean RATIO stays around 9.
 * The absolute figure is 1.4e-11 -- 7x inside the band -- because WMA's weights
 * are bounded by `period` and its divider is period*(period+1)/2, which dilutes
 * the residue far more than a slope's small divisor does. Adding the trigger
 * bought 1.4e-11 -> ~7e-12 and cost 1.17x on TA_WMA and 1.65x on TA_HMA, whose
 * three fused stages each pay it. Not worth it here; the LINEARREG family keeps
 * its trigger because interval-only FAILS its tier outright, at 2.38e-10.
 *
 * SABOTAGE-PROVEN: removing the re-anchor fails this at 24.1 (period 5). */
static ErrorNumber test_wma_contamination( void )
{
   static const int periods[4] = { 2, 5, 14, 30 };
   enum { SPIKE_BAR = 60 };
   int p, i;

   wma_corpus( wma_long, WMA_LONG_N );
   for( i = 0; i < WMA_LONG_N; i++ ) wma_spiked[i] = wma_long[i];
   wma_spiked[SPIKE_BAR] *= 1000.0;

   for( p = 0; p < 4; p++ )
   {
      double unused, clean, spiked;
      if( wma_range_worst( wma_long, WMA_LONG_N, periods[p], SPIKE_BAR, &unused, &clean ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      if( wma_range_worst( wma_spiked, WMA_LONG_N, periods[p], SPIKE_BAR, &unused, &spiked ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      if( spiked > WMA_CONTAM_TIER )
      {
         printf( "WMA #255 [print contamination]: period=%d -- with a 1000x print at "
                 "bar %d the worst range disagreement over bars past it is %.3g, "
                 "outside the %.1g TA_STABLE_EPSILON band (the same bars run %.3g "
                 "clean). The residue a large print leaves is not being bounded.\n",
                 periods[p], (int)SPIKE_BAR, spiked, WMA_CONTAM_TIER, clean );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}
