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
 *  090426 MF,CC  First version (issue #369).
 */

/* Description:
 *
 *   Test TA_PERCENTRANK (Percent Rank).
 *
 *   Legs:
 *     1. EXTERNAL ORACLE at ZERO tolerance: ta4j 0.22.6's
 *        PercentRankIndicator, replayed on the frozen corpus. Every value of
 *        this function is (k/N)*100 for an integer k, so a correct build is
 *        bit-identical to it on every platform and the frozen rows need no
 *        tolerance at all. That is what makes the leg gate the OPERATION
 *        ORDER: `100.0*k/N` is a different double from `(k/N)*100.0` on 99 of
 *        the 883 corpus values, and the rows below include such bars.
 *     2. The count-to-value map, k by k, on windows built to hold exactly k
 *        values below the current one. This is the leg that fails on integer
 *        division -- `k/N` truncates to 0 or 1, which both of the obvious
 *        hand-written series (monotone up, monotone down) would still accept.
 *     3. The whole-corpus identity against an independent brute-force count,
 *        bitwise, over three corpora and every period from 2 to 40. Pins the
 *        anchor and the window bounds at scale; one corpus is quantised to
 *        seven levels so most windows carry ties.
 *     4. Exact-arithmetic edges. Constant input is the discriminator for the
 *        TIE RULE and is asserted, not assumed: strict `<` answers exactly
 *        0.0 where TradingView's `<=` answers 100.0. Signed zeros are the
 *        cheap probe of the same rule without a constant series.
 *     5. In-place aliasing (outReal == inReal), bitwise. Safe by exactly one
 *        slot -- the store lands on the input index the window's FIRST read
 *        consumed -- so this is the leg that catches a store hoisted above
 *        the count.
 *     6. Shape edges, then the startIdx/endIdx range sweep in the EXACT class:
 *        every bar is recounted from its own window, so no range may move a
 *        value at all.
 *
 *   Cross-language value coverage comes from server_verify in leg 1 plus the
 *   --xlang-hash sweep; the frozen ta_ref_serve predates this function, so the
 *   --codegen value comparison cannot run for it (same situation as VHF).
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
#define PR_CAP     3100
#define PR_GD_NB   1000
#define PR_TIE_NB  2000
#define PR_MAX_PER 40

typedef struct { int period; int bar; double want; } PercentRankGolden;

/* Goldens captured by ta-lib-oracles/capture_369_percentrank.py on the 252-bar
 * TA_SREF close series (TA_SREF_close_daily_ref_0_PRIV), at %.17g, which
 * round-trips to the same double. `bar` is the ABSOLUTE bar index; the output
 * index is bar - begIdx.
 *
 * ORACLE: ta4j 0.22.6 (org.ta4j:ta4j-core, Maven Central), class
 * org.ta4j.core.indicators.helpers.PercentRankIndicator over a
 * ClosePriceIndicator, driven on this exact series (2026-09-04) through
 * ta-lib-oracles/ta4j_serve on Java 21.0.12 with DoubleNumFactory -- ta4j's own
 * default is arbitrary-precision DecimalNum, which would be testing BigDecimal
 * rather than the formula. Its window is [index-period, index), strictly
 * less-than, divided by the scanned count and only then scaled to a percentage.
 *
 * TOLERANCE: ZERO -- compared with ==. Justified rather than hopeful: every
 * value is a single IEEE-754 division of two small integers followed by a
 * single multiplication, both correctly rounded on any conforming platform, so
 * there is no libm call and nothing to drift. A relative tolerance here would
 * silently accept `100.0*k/N`, which is the one spelling mistake this function
 * invites.
 *
 * ta4j emits one output EARLIER than TA-Lib does (its unstable count is
 * period-1) and divides that first bar by period-1, its window being short by
 * one; the capture requests startIdx == period, which drops exactly that bar.
 *
 * No second wireable arm exists: Tulip Indicators 0.9.2 and pandas-ta-classic
 * ship no rank or quantile indicator at all, and the pine_serve arm of the same
 * name is TradingView's less-than-OR-EQUAL function -- a different indicator,
 * never a golden source for this one. trading-signals 8.3.0 transcribes the
 * same strict rule but only inside ConnorsRSI.update(), which exports nothing
 * to drive. */
static const PercentRankGolden percentRankOracle[] =
{
   {   2,   2,                     50 },
   {   2,   3,                    100 },
   {   2,   4,                      0 },
   {   2,  20,                      0 },
   {   2,  64,                    100 },
   {   2,  70,                      0 },
   {   2, 100,                     50 },
   {   2, 127,                    100 },
   {   2, 150,                      0 },
   {   2, 189,                     50 },
   {   2, 251,                      0 },
   {   3,   3,                    100 },
   {   3,   4,                      0 },
   {   3,   5,     66.666666666666657 },
   {   3,  20,                      0 },
   {   3,  65,                    100 },
   {   3,  70,                      0 },
   {   3,  79,     33.333333333333329 },
   {   3, 100,     66.666666666666657 },
   {   3, 127,                    100 },
   {   3, 144,     33.333333333333329 },
   {   3, 150,     33.333333333333329 },
   {   3, 189,     33.333333333333329 },
   {   3, 197,     33.333333333333329 },
   {   3, 251,                      0 },
   {  20,  20,                      0 },
   {  20,  21,                      0 },
   {  20,  43,                    100 },
   {  20,  58,     55.000000000000007 },
   {  20,  70,                     45 },
   {  20,  78,                    100 },
   {  20,  98,     55.000000000000007 },
   {  20, 100,                     65 },
   {  20, 105,     55.000000000000007 },
   {  20, 136,                     35 },
   {  20, 150,                     10 },
   {  20, 157,     55.000000000000007 },
   {  20, 194,                      5 },
   {  20, 251,                     10 },
   { 100, 100,                     93 },
   { 100, 101,                     92 },
   { 100, 116,                    100 },
   { 100, 138,                     83 },
   { 100, 150,                     67 },
   { 100, 176,                     85 },
   { 100, 186,     28.000000000000004 },
   { 100, 195,                      0 },
   { 100, 214,                      5 },
   { 100, 231,     28.000000000000004 },
   { 100, 251,                     35 },
};
#define NB_PR_ORACLE ((int)(sizeof(percentRankOracle)/sizeof(PercentRankGolden)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing.
 * g_prMidCmp counts values strictly between 0 and 100: under integer division
 * that count is zero and everything else here still passes. */
static int g_prOracleCmp;
static int g_prCountMapCmp;
static int g_prIdentityCmp;
static int g_prMidCmp;
static int g_prEdgeCmp;
static int g_prAliasCmp;
static int g_prShapeCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_pr_oracle( const TA_History *history );
static ErrorNumber test_pr_count_map( void );
static ErrorNumber test_pr_identity( const char *tag, const TA_Real *in,
                                     int nbBars, int maxPeriod );
static ErrorNumber test_pr_edges( void );
static ErrorNumber test_pr_aliasing( const char *tag, const TA_Real *in,
                                     int nbBars, int maxPeriod );
static ErrorNumber test_pr_shape( const TA_Real *in, int nbBars );
static ErrorNumber test_pr_range( const TA_Real *in );

/* The brute-force reference the identity leg compares against: how many of the
 * period values ending one bar before `today` are strictly below it. */
static int prCountBelow( const TA_Real *in, int today, int period )
{
   int j, count = 0;

   for( j = today - period; j < today; j++ )
   {
      if( in[j] < in[today] )
         count++;
   }
   return count;
}

/* Seven distinct levels, so most windows carry several ties: the corpus where
 * a `<=` reading of the rule diverges on nearly every bar. Every value is a
 * small exact integer, so the identity leg stays a bitwise equality. */
static void prBuildTied( double *dest, int nb )
{
   int i;

   for( i = 0; i < nb; i++ )
      dest[i] = (double)( ( i * 37 ) % 7 );
}

/**** Global functions definitions. ****/
ErrorNumber test_func_percentrank( TA_History *history )
{
   static TA_Real tied[PR_TIE_NB];
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   /* PERCENTRANK has no unstable period; a leftover global setting must not
    * reach it, and the range sweep below asserts the same from the other side. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_prOracleCmp = g_prCountMapCmp = g_prIdentityCmp = g_prMidCmp = 0;
   g_prEdgeCmp = g_prAliasCmp = g_prShapeCmp = 0;

   prBuildTied( tied, PR_TIE_NB );

   err = test_pr_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_count_map();
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_identity( "TA_SREF close", history->close, nbBars, PR_MAX_PER );
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_identity( "gData close", gDataClose, PR_GD_NB, PR_MAX_PER );
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_identity( "seven-level ties", tied, PR_TIE_NB, PR_MAX_PER );
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_aliasing( "TA_SREF close", history->close, nbBars, PR_MAX_PER );
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_aliasing( "seven-level ties", tied, PR_TIE_NB, PR_MAX_PER );
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_shape( history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_pr_range( history->close );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_prOracleCmp != NB_PR_ORACLE || g_prCountMapCmp != 137
            || g_prIdentityCmp != 124371 || g_prMidCmp != 96419
            || g_prEdgeCmp != 12324 || g_prAliasCmp != 86190
            || g_prShapeCmp != 6 ) )
   {
      printf( "PERCENTRANK Fail: coverage counters (oracle %d, count-map %d, "
              "identity %d, mid-range %d, edges %d, alias %d, shape %d) are not "
              "what this file was written with (%d, 137, 124371, 96419, 12324, "
              "86190, 6)\n",
              g_prOracleCmp, g_prCountMapCmp, g_prIdentityCmp, g_prMidCmp,
              g_prEdgeCmp, g_prAliasCmp, g_prShapeCmp, NB_PR_ORACLE );
      return TA_PERCENTRANK_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) The frozen ta4j goldens at zero tolerance, plus the cross-language
 * replay of each of those calls. */
static ErrorNumber test_pr_oracle( const TA_History *history )
{
   static TA_Real out[PR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastPeriod = -1;

   if( nbBars != 252 )
   {
      printf( "PERCENTRANK oracle skip: goldens were captured on the 252-bar "
              "corpus, got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;
   for( k = 0; k < NB_PR_ORACLE; k++ )
   {
      double got;

      if( percentRankOracle[k].period != lastPeriod )
      {
         lastPeriod = percentRankOracle[k].period;
         retCode = TA_PERCENTRANK( 0, nbBars-1, history->close, lastPeriod,
                                   &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod
             || nbElement != nbBars - lastPeriod )
         {
            printf( "PERCENTRANK oracle Fail [N=%d]: rc=%d (%d,%d) expected "
                    "(%d,%d)\n", lastPeriod, (int)retCode, begIdx, nbElement,
                    lastPeriod, nbBars - lastPeriod );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[1];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastPeriod;
            e = server_verify( "PERCENTRANK", 0, nbBars-1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->close, NULL },
                               optIn, 1,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* "No failure reported" and "nothing was compared" are the same
             * observation without this. */
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "PERCENTRANK oracle [N=%d]: compared no server despite "
                       "live pipes\n", lastPeriod );
               return TA_PERCENTRANK_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( percentRankOracle[k].bar < begIdx
          || percentRankOracle[k].bar - begIdx >= nbElement )
      {
         printf( "PERCENTRANK oracle Fail [N=%d]: golden bar %d is outside the "
                 "output [%d..%d]\n", percentRankOracle[k].period,
                 percentRankOracle[k].bar, begIdx, begIdx + nbElement - 1 );
         return TA_PERCENTRANK_VACUOUS;
      }

      got = out[percentRankOracle[k].bar - begIdx];
      g_prOracleCmp++;
      if( got != percentRankOracle[k].want )
      {
         printf( "PERCENTRANK oracle Fail [N=%d] at bar %d: got %.17g expected "
                 "exactly %.17g (ta4j 0.22.6, tolerance 0; a near miss here is "
                 "the 100.0*count/N spelling)\n",
                 percentRankOracle[k].period, percentRankOracle[k].bar,
                 got, percentRankOracle[k].want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (2) The count-to-value map, one call per (N, k).
 *
 * The window holds k copies of 1.0 then N-k copies of 3.0, and the bar being
 * ranked is 2.0, so exactly k of the previous N values are below it whatever N
 * and k are. Every intermediate is an exact binary fraction, so the expected
 * value is an equality.
 *
 * Integer division survives every monotone series -- k == N gives 100.0 and
 * k == 0 gives 0.0 either way -- and dies here at the first k in between.
 */
static ErrorNumber test_pr_count_map( void )
{
   static const int periods[] = { 2, 3, 7, 20, 100 };
   static TA_Real in[128], out[PR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int p, k, i;

   for( p = 0; p < (int)(sizeof(periods)/sizeof(int)); p++ )
   {
      int period = periods[p];

      for( k = 0; k <= period; k++ )
      {
         double want = ( (double)k / (double)period ) * 100.0;

         for( i = 0; i < k; i++ )
            in[i] = 1.0;
         for( i = k; i < period; i++ )
            in[i] = 3.0;
         in[period] = 2.0;

         retCode = TA_PERCENTRANK( 0, period, in, period,
                                   &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != period || nbElement != 1 )
         {
            printf( "PERCENTRANK count-map Fail [N=%d k=%d]: rc=%d (%d,%d) "
                    "expected (%d,1)\n", period, k, (int)retCode,
                    begIdx, nbElement, period );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         g_prCountMapCmp++;
         if( out[0] != want )
         {
            printf( "PERCENTRANK count-map Fail [N=%d k=%d]: got %.17g, "
                    "expected exactly %.17g. %d of the previous %d values are "
                    "below; a result of 0 or 100 here means the count was "
                    "divided as an integer\n",
                    period, k, out[0], want, k, period );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) The whole-corpus identity against an independent brute-force count.
 *
 * Bitwise: the count is an integer and the scaling is two correctly-rounded
 * operations, so there is nothing for a tolerance to absorb. What this pins
 * that the goldens cannot is the ANCHOR and the window bounds on every bar of
 * every period -- an off-by-one in either direction moves whole values, not
 * ulps.
 */
static ErrorNumber test_pr_identity( const char *tag, const TA_Real *in,
                                     int nbBars, int maxPeriod )
{
   static TA_Real out[PR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int period, t;

   for( period = 2; period <= maxPeriod; period++ )
   {
      retCode = TA_PERCENTRANK( 0, nbBars-1, in, period,
                                &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period
          || nbElement != nbBars - period )
      {
         printf( "PERCENTRANK identity Fail [%s N=%d]: rc=%d (%d,%d) expected "
                 "(%d,%d)\n", tag, period, (int)retCode, begIdx, nbElement,
                 period, nbBars - period );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( t = period; t < nbBars; t++ )
      {
         int count = prCountBelow( in, t, period );
         double want = ( (double)count / (double)period ) * 100.0;
         double got  = out[t-begIdx];

         g_prIdentityCmp++;
         if( got > 0.0 && got < 100.0 )
            g_prMidCmp++;
         if( got != want )
         {
            printf( "PERCENTRANK identity Fail [%s N=%d] at bar %d: got %.17g "
                    "expected exactly %.17g (%d of the previous %d values are "
                    "strictly below)\n",
                    tag, period, t, got, want, count, period );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( !( got >= 0.0 ) || got > 100.0 )
         {
            printf( "PERCENTRANK bound Fail [%s N=%d] at bar %d: %.17g is "
                    "outside [0,100]\n", tag, period, t, got );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) Exact-arithmetic edges.
 *
 *   - CONSTANT input: every output is exactly 0.0. This is the tie rule, and
 *     it is the only cheap case that separates it -- TradingView's `<=`
 *     reading answers exactly 100.0 on the same series.
 *   - Strictly increasing: exactly 100.0 everywhere. Strictly decreasing:
 *     exactly 0.0. Neither discriminates on its own (see leg 2), but together
 *     they pin the direction of the comparison.
 *   - SIGNED ZEROS: -0.0 < 0.0 and 0.0 < -0.0 are both false, so a window
 *     mixing the two must behave exactly like the constant case. Probes the
 *     tie rule without needing a flat series.
 */
static ErrorNumber test_pr_edges( void )
{
   static TA_Real in[128], out[PR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int period, i;

   for( i = 0; i < 100; i++ )
      in[i] = 42.0;
   for( period = 2; period <= PR_MAX_PER; period++ )
   {
      retCode = TA_PERCENTRANK( 0, 99, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period || nbElement != 100 - period )
      {
         printf( "PERCENTRANK constant Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_prEdgeCmp++;
         if( out[i] != 0.0 )
         {
            printf( "PERCENTRANK constant Fail [N=%d] out %d: %.17g, expected "
                    "exactly 0.0. Nothing in a flat window is STRICTLY below "
                    "the current value; 100.0 here means the comparison became "
                    "<= (the TradingView reading, a different function)\n",
                    period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* +0.0 and -0.0 alternating: equal under <, so the constant answer holds. */
   for( i = 0; i < 100; i++ )
      in[i] = ( i & 1 ) ? -0.0 : 0.0;
   if( !signbit( in[1] ) || signbit( in[0] ) )
   {
      printf( "PERCENTRANK signed-zero Fail: the input lost its -0.0, so the "
              "case below would prove nothing\n" );
      return TA_PERCENTRANK_VACUOUS;
   }
   for( period = 2; period <= PR_MAX_PER; period++ )
   {
      retCode = TA_PERCENTRANK( 0, 99, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period || nbElement != 100 - period )
      {
         printf( "PERCENTRANK signed-zero Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_prEdgeCmp++;
         if( out[i] != 0.0 )
         {
            printf( "PERCENTRANK signed-zero Fail [N=%d] out %d: %.17g, "
                    "expected exactly 0.0 -- -0.0 and +0.0 compare equal\n",
                    period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* Monotone, in exact halves, both directions. */
   for( i = 0; i < 100; i++ )
      in[i] = 64.0 + 0.5 * (double)i;
   for( period = 2; period <= PR_MAX_PER; period++ )
   {
      retCode = TA_PERCENTRANK( 0, 99, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period || nbElement != 100 - period )
      {
         printf( "PERCENTRANK rising Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_prEdgeCmp++;
         if( out[i] != 100.0 )
         {
            printf( "PERCENTRANK rising Fail [N=%d] out %d: %.17g, expected "
                    "exactly 100.0\n", period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   for( i = 0; i < 100; i++ )
      in[i] = 64.0 - 0.5 * (double)i;
   for( period = 2; period <= PR_MAX_PER; period++ )
   {
      retCode = TA_PERCENTRANK( 0, 99, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period || nbElement != 100 - period )
      {
         printf( "PERCENTRANK falling Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_prEdgeCmp++;
         if( out[i] != 0.0 )
         {
            printf( "PERCENTRANK falling Fail [N=%d] out %d: %.17g, expected "
                    "exactly 0.0\n", period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, outReal == inReal, bitwise.
 *
 * With startIdx at the lookback the first store lands on input index
 * today-optInTimePeriod, which the window's FIRST read has just consumed. That
 * one slot of clearance is the whole invariant and it holds only because the
 * count precedes the store.
 */
static ErrorNumber test_pr_aliasing( const char *tag, const TA_Real *in,
                                     int nbBars, int maxPeriod )
{
   static TA_Real clean[PR_CAP], alias[PR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int period, i;

   for( period = 2; period <= maxPeriod; period++ )
   {
      retCode = TA_PERCENTRANK( 0, nbBars-1, in, period,
                                &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "PERCENTRANK alias Fail [%s N=%d]: rc=%d\n",
                 tag, period, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = in[i];
      retCode = TA_PERCENTRANK( 0, nbBars-1, alias, period,
                                &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "PERCENTRANK alias Fail [%s N=%d]: rc=%d shape (%d,%d) vs "
                 "(%d,%d)\n", tag, period, (int)retCode,
                 begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_prAliasCmp++;
         if( clean[i] != alias[i] )
         {
            printf( "PERCENTRANK alias Fail [%s N=%d] out %d: separate %.17g, "
                    "in-place %.17g -- a store landed under a read the window "
                    "still needed\n", tag, period, i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6a) Shape edges: a range shorter than the lookback, and a range holding
 * exactly one output. */
static ErrorNumber test_pr_shape( const TA_Real *in, int nbBars )
{
   static TA_Real out[PR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   const int period = 20;

   (void)nbBars;

   begIdx = -1; nbElement = -1;
   retCode = TA_PERCENTRANK( 0, period-1, in, period,
                             &begIdx, &nbElement, out );
   g_prShapeCmp++;
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "PERCENTRANK shape Fail (short range): rc=%d (%d,%d) expected "
              "TA_SUCCESS (0,0)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   begIdx = -1; nbElement = -1;
   retCode = TA_PERCENTRANK( period, period, in, period,
                             &begIdx, &nbElement, out );
   g_prShapeCmp++;
   if( retCode != TA_SUCCESS || begIdx != period || nbElement != 1 )
   {
      printf( "PERCENTRANK shape Fail (single bar): rc=%d (%d,%d) expected "
              "(%d,1)\n", (int)retCode, begIdx, nbElement, period );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   g_prShapeCmp++;
   if( out[0] != ( (double)prCountBelow( in, period, period )
                   / (double)period ) * 100.0 )
   {
      printf( "PERCENTRANK shape Fail (single bar): %.17g does not match the "
              "same bar of a full-range call\n", out[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* The lookback tier must agree with what the batch tier anchors on. */
   g_prShapeCmp++;
   if( TA_PERCENTRANK_Lookback( period ) != period )
   {
      printf( "PERCENTRANK shape Fail: lookback %d, expected %d -- the current "
              "bar is excluded from its own window, so it is N and not N-1\n",
              TA_PERCENTRANK_Lookback( period ), period );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   g_prShapeCmp++;
   if( TA_PERCENTRANK_Lookback( 1 ) != -1 || TA_PERCENTRANK_Lookback( 0 ) != -1 )
   {
      printf( "PERCENTRANK shape Fail: a period below the declared minimum was "
              "given a lookback\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   begIdx = -1; nbElement = -1;
   retCode = TA_PERCENTRANK( 0, 251, in, 1, &begIdx, &nbElement, out );
   g_prShapeCmp++;
   if( retCode != TA_BAD_PARAM )
   {
      printf( "PERCENTRANK shape Fail: period 1 returned rc=%d, expected "
              "TA_BAD_PARAM\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   return TA_TEST_PASS;
}

/* (6b) The startIdx/endIdx range sweep. TA_STABLE_EXACT: every bar is
 * recounted from its own window with no carried state, so no range may move a
 * value by even one ulp. No unstable-period id (matching its abstract
 * metadata, which doRangeTestEx cross-checks against the stability class). */
typedef struct { int period; const TA_Real *in; } PercentRankRangeParam;

static TA_RetCode percentRankRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                                TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                                TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                                TA_Integer *lookback, void *opaqueData,
                                                unsigned int outputNb, unsigned int *isOutputInteger )
{
   PercentRankRangeParam *p = (PercentRankRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_PERCENTRANK_Lookback( p->period );
   return TA_PERCENTRANK( startIdx, endIdx, p->in, p->period,
                          outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_pr_range( const TA_Real *in )
{
   PercentRankRangeParam param;

   param.period = 20;
   param.in     = in;

   return doRangeTestEx( percentRankRangeTestFunction,
                         TA_STABLE_EXACT, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
