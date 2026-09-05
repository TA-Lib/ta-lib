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
 *  090426 MF,CC  First version (issue #370).
 */

/* Description:
 *
 *   Test TA_RVOL (Relative Volume).
 *
 *   Legs:
 *     1. DIFFERENTIAL against TA_SMA shifted one bar, BITWISE. RVOL's baseline
 *        excludes the current bar, so it is TA_SMA's own window one bar behind:
 *        RVOL(v,n,s,e)[k] == v[s+k] / SMA(v,n,s-1,e-1)[k], with no tolerance at
 *        all. Bit-exactness is what makes this leg worth having -- it holds
 *        only for the subtract-old-then-add-new accumulator order, and the
 *        reverse order differs in the last ulp, which any tolerance would let
 *        through. Swept over three corpora, every period 1..60 and four start
 *        indices, so it also pins the lookback (a copy of SMA's off-by-one
 *        would move begIdx and fail the shape check first).
 *     2. EXTERNAL ORACLE: trading-signals 8.3.0 `ts.RVOL`, driven on this exact
 *        corpus. See rvolOracle below. Also the cross-language replay.
 *     3. Exact-arithmetic edges, on inputs whose every intermediate is an exact
 *        integer: a lone spike is reported at FULL size on its own bar and
 *        elevates exactly the next optInTimePeriod baselines -- the assertion
 *        that pins WHICH bars the window covers, since a co-terminal window
 *        would dilute the spike into its own baseline. Plus the period-1 form
 *        and a flat series.
 *     4. The zero-baseline contract: a dead window has no ratio, and TA_RVOL
 *        emits a non-finite value there rather than a guarded 0. That is what
 *        `nan_inf_output` declares in rvol.yaml, and this leg is what makes the
 *        declaration honest.
 *     5. In-place aliasing (outReal == inVolume), bitwise, over both corpora
 *        and every period. At startIdx == optInTimePeriod the first output slot
 *        IS the bar about to be subtracted from the running total, so this is
 *        the leg that catches a store placed above the trailing read.
 *     6. The startIdx/endIdx range sweep, in the EPSILON class: the running
 *        accumulator is seeded by a fresh sum whose first term depends on
 *        startIdx, so ranges may differ in the last ulp (the "running-sum MAs"
 *        example in ta_test_priv.h). TA_STABLE_EXACT would be wrong here.
 *
 *   Cross-language value coverage comes from server_verify in leg 2 plus the
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
#define RVOL_CAP     2200
#define RVOL_GD_NB   1000
#define RVOL_SYN_NB  2000
#define RVOL_MAX_PER 60

/* Leg 2. trading-signals re-sums its window fresh where TA_RVOL rolls a running
 * total, so the two are not bit-exact in general -- but on this corpus they
 * measured bit-identical over all 681 values of the three periods below (both
 * summation orders are exact, the volumes being integers far under 2^53). The
 * tolerance is therefore headroom for a platform that evaluates the divide
 * differently, not slack for a measured gap. Relative only: RVOL is a ratio of
 * like quantities and the frozen rows bottom out at 0.21, so an absolute
 * companion would never be the binding term. */
#define RVOL_ORACLE_REL 1e-13

typedef struct { int period; int bar; double want; } RvolGolden;

/* Goldens captured by ta-lib-oracles/capture_370_rvol.py on the 252-bar TA_SREF
 * volume series (TA_SREF_volume_daily_ref_0_PRIV), at %.17g, which round-trips
 * to the same double. `bar` is the ABSOLUTE bar index; the output index is
 * bar - begIdx.
 *
 * ONE independent implementation, executed on this exact series (2026-09-04),
 * never re-derived from the formula:
 *   trading-signals 8.3.0 -- TypeScript, `ts.RVOL` (dist/volume/RVOL/RVOL.js).
 *   It keeps the prior volumes, takes slice(-period) and re-sums that window on
 *   every bar, so its arithmetic is independent of our running total.
 *   getRequiredInputs() = period + 1, matching TA_RVOL_Lookback.
 * No second arm exists: pandas-ta-classic 0.6.52 has no rvol/relative_volume
 * module, Tulip Indicators 0.9.2 has none among its 104 indicators, and ta4j's
 * RelativeVolumeStandardDeviation is a z-score rather than this ratio.
 * It diverges only where this corpus does not go: on a zero baseline it returns
 * null, where TA_RVOL emits the non-finite value leg 4 pins. No window in this
 * corpus is dead.
 * Each period's own lowest and highest bar is included: the lowest is where a
 * relative tolerance is least forgiving, the highest is the spike a wrong
 * window would dilute. */
static const RvolGolden rvolOracle[] =
{
   {   5,   5,       0.80491286557225883 },
   {   5,   6,       0.76496207752737333 },
   {   5,  40,       0.74078679542706727 },
   {   5,  90,        2.5170958734098665 },
   {   5, 140,       0.59316775520544829 },
   {   5, 190,        0.8021552261118724 },
   {   5, 202,        5.9503951661548582 },
   {   5, 227,       0.21479995197688267 },
   {   5, 251,        0.7765369778225043 },
   {  20,  20,       0.92298468851863125 },
   {  20,  21,       0.86943163409078217 },
   {  20,  40,       0.72334551269118974 },
   {  20,  90,        1.7911746133612803 },
   {  20, 140,       0.85190589951195217 },
   {  20, 190,       0.86538364615890162 },
   {  20, 202,        7.0026771534447549 },
   {  20, 227,       0.28144426081893764 },
   {  20, 251,       0.38150919748832579 },
   {  50,  50,       0.61101047224649951 },
   {  50,  51,        1.2830327108870381 },
   {  50,  90,        2.1848488134677853 },
   {  50, 140,       0.88344561537251332 },
   {  50, 190,        1.0009532443887297 },
   {  50, 202,        8.9090144515092167 },
   {  50, 249,       0.24550898313090333 },
   {  50, 251,       0.26871463320382361 },
};
#define NB_RVOL_ORACLE ((int)(sizeof(rvolOracle)/sizeof(RvolGolden)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_rvolDiffCmp;
static int g_rvolOracleCmp;
static int g_rvolEdgeCmp;
static int g_rvolNanCmp;
static int g_rvolAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_rvol_differential( const char *tag, const TA_Real *in,
                                           int nbBars );
static ErrorNumber test_rvol_oracle( const TA_History *history );
static ErrorNumber test_rvol_edges( void );
static ErrorNumber test_rvol_deadwindow( void );
static ErrorNumber test_rvol_aliasing( const char *tag, const TA_Real *in,
                                       int nbBars );
static ErrorNumber test_rvol_range( const TA_Real *in );

/* A synthetic series whose values are NOT exact integers, built from an exact
 * rule so nothing is transported. The price corpora and the volume corpus both
 * carry values a running sum happens to accumulate exactly; here it does not,
 * which is what makes leg 1's bitwise claim a real constraint on the
 * accumulator order rather than a tautology. */
static void rvolBuildFractional( double *dest, int nb )
{
   int i;
   for( i = 0; i < nb; i++ )
      dest[i] = 1.0 + (double)( ( i * 7919 ) % 1000003 ) / 3.0;
}

/**** Global functions definitions. ****/
ErrorNumber test_func_rvol( TA_History *history )
{
   static TA_Real fractional[RVOL_SYN_NB];
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   /* RVOL has no unstable period; a leftover global setting must not reach it,
    * and the range sweep below asserts the same thing from the other side. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_rvolDiffCmp = g_rvolOracleCmp = g_rvolEdgeCmp = 0;
   g_rvolNanCmp = g_rvolAliasCmp = 0;

   rvolBuildFractional( fractional, RVOL_SYN_NB );

   err = test_rvol_differential( "TA_SREF volume", history->volume, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvol_differential( "gData close", gDataClose, RVOL_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvol_differential( "fractional", fractional, RVOL_SYN_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvol_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvol_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvol_deadwindow();
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvol_aliasing( "TA_SREF volume", history->volume, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvol_aliasing( "fractional", fractional, RVOL_SYN_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvol_range( history->volume );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_rvolDiffCmp != 757980 || g_rvolOracleCmp != NB_RVOL_ORACLE
            || g_rvolEdgeCmp != 18419 || g_rvolNanCmp != 90
            || g_rvolAliasCmp != 131460 ) )
   {
      printf( "RVOL Fail: coverage counters (diff %d, oracle %d, edges %d, "
              "dead-window %d, alias %d) are not what this file was written "
              "with (757980, %d, 18419, 90, 131460)\n",
              g_rvolDiffCmp, g_rvolOracleCmp, g_rvolEdgeCmp, g_rvolNanCmp,
              g_rvolAliasCmp, NB_RVOL_ORACLE );
      return TA_RVOL_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) TA_RVOL against TA_SMA shifted one bar, with NO tolerance.
 *
 * The identity is exact, not approximate: TA_RVOL accumulates the same window
 * TA_SMA does, in the same order, one bar behind, so every baseline is the
 * same double TA_SMA divided by the same period. Comparing bitwise is the only
 * way to hold the accumulator order -- add-new-then-subtract-old computes the
 * same window and lands within a few ulp, which every tolerance in this file
 * would accept.
 *
 * The shape check ahead of it is what pins the lookback: TA_RVOL needs one bar
 * more than TA_SMA of the same period, so an off-by-one there moves begIdx.
 */
static ErrorNumber test_rvol_differential( const char *tag, const TA_Real *in,
                                           int nbBars )
{
   static TA_Real outRvol[RVOL_CAP], outSma[RVOL_CAP];
   TA_Integer begR, nbR, begS, nbS;
   TA_RetCode retCode;
   int period, s, k, shift;

   for( period = 1; period <= RVOL_MAX_PER; period++ )
   {
      for( shift = 0; shift <= 3; shift++ )
      {
         /* shift 0 drives both from bar 0 and lets each clamp to its own
          * lookback; the others anchor RVOL explicitly and hand TA_SMA the
          * matching one-bar-earlier range. */
         s = ( shift == 0 ) ? 0 : period + shift - 1;

         retCode = TA_RVOL( s, nbBars-1, in, period, &begR, &nbR, outRvol );
         if( retCode != TA_SUCCESS || begR != ( s < period ? period : s )
             || nbR != nbBars - begR )
         {
            printf( "RVOL differential Fail [%s N=%d s=%d]: rc=%d (%d,%d) "
                    "expected (%d,%d)\n", tag, period, s, (int)retCode,
                    begR, nbR, ( s < period ? period : s ),
                    nbBars - ( s < period ? period : s ) );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         retCode = TA_SMA( s == 0 ? 0 : s-1, nbBars-2, in, period,
                           &begS, &nbS, outSma );
         if( retCode != TA_SUCCESS || nbS != nbR )
         {
            printf( "RVOL differential Fail [%s N=%d s=%d]: reference TA_SMA "
                    "rc=%d gave %d values, RVOL gave %d\n",
                    tag, period, s, (int)retCode, nbS, nbR );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( k = 0; k < nbR; k++ )
         {
            double want = in[begR+k] / outSma[k];

            g_rvolDiffCmp++;
            if( memcmp( &want, &outRvol[k], sizeof(double) ) != 0 )
            {
               printf( "RVOL differential Fail [%s N=%d s=%d] at bar %d: got "
                       "%.17g, TA_SMA one bar back gives %.17g. The identity is "
                       "bit-exact; a last-ulp gap means the accumulator adds "
                       "today's bar before dropping the trailing one\n",
                       tag, period, s, begR+k, outRvol[k], want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) The frozen trading-signals goldens, plus the cross-language replay. */
static ErrorNumber test_rvol_oracle( const TA_History *history )
{
   static TA_Real out[RVOL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastPeriod = -1;

   if( nbBars != 252 )
   {
      printf( "RVOL oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_RVOL_ORACLE; k++ )
   {
      double got, err;
      const char *mode;

      if( rvolOracle[k].period != lastPeriod )
      {
         lastPeriod = rvolOracle[k].period;
         retCode = TA_RVOL( 0, nbBars-1, history->volume, lastPeriod,
                            &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod
             || nbElement != nbBars - lastPeriod )
         {
            printf( "RVOL oracle Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    lastPeriod, (int)retCode, begIdx, nbElement,
                    lastPeriod, nbBars - lastPeriod );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[1];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastPeriod;
            e = server_verify( "RVOL", 0, nbBars-1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->volume, NULL },
                               optIn, 1,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* "No failure reported" and "nothing was compared" are the same
             * observation without this. */
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "RVOL oracle [N=%d]: compared no server despite live "
                       "pipes\n", lastPeriod );
               return TA_RVOL_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( rvolOracle[k].bar < begIdx || rvolOracle[k].bar - begIdx >= nbElement )
      {
         printf( "RVOL oracle Fail [N=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", rvolOracle[k].period, rvolOracle[k].bar,
                 begIdx, begIdx + nbElement - 1 );
         return TA_RVOL_VACUOUS;
      }

      got = out[rvolOracle[k].bar - begIdx];
      g_rvolOracleCmp++;
      if( !checkOracleValue( got, rvolOracle[k].want, RVOL_ORACLE_REL, 0.0,
                             &err, &mode ) )
      {
         printf( "RVOL oracle Fail [N=%d] at bar %d: got %.17g expected %.17g "
                 "(%s err %.3g, tol rel %g)\n",
                 rvolOracle[k].period, rvolOracle[k].bar, got,
                 rvolOracle[k].want, mode, err, RVOL_ORACLE_REL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) Exact-arithmetic edges.
 *
 * Every value below is a small integer, so every window sum and every quotient
 * asserted as an equality is exact.
 *
 *   - A flat series is exactly 1.0 at every bar and every period.
 *   - A lone spike on an otherwise flat series is reported at FULL size on its
 *     own bar, because it is not in its own baseline. This is the assertion
 *     that pins the window POSITION: a co-terminal window would dilute the
 *     spike into the average it is divided by and answer 1024*N/(N+1023).
 *   - The spike then elevates exactly the next N baselines and no more, which
 *     pins the window LENGTH from both ends without recomputing the formula.
 *   - At a period of 1 the baseline is the single previous bar, so the output
 *     is the exact ratio of consecutive values.
 */
static ErrorNumber test_rvol_edges( void )
{
   static TA_Real in[300], out[RVOL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int period, i;
   const int spike = 200;

   for( i = 0; i < 300; i++ )
      in[i] = 1.0;

   for( period = 1; period <= RVOL_MAX_PER; period++ )
   {
      retCode = TA_RVOL( 0, 299, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period || nbElement != 300 - period )
      {
         printf( "RVOL flat Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_rvolEdgeCmp++;
         if( out[i] != 1.0 )
         {
            printf( "RVOL flat Fail [N=%d] out %d: %.17g, expected exactly "
                    "1.0\n", period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   in[spike] = 1024.0;

   for( period = 1; period <= RVOL_MAX_PER; period++ )
   {
      retCode = TA_RVOL( 0, 299, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period )
      {
         printf( "RVOL spike Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      g_rvolEdgeCmp++;
      if( out[spike-begIdx] != 1024.0 )
      {
         printf( "RVOL spike Fail [N=%d]: the spike bar reads %.17g, expected "
                 "exactly 1024.0 -- a bar is not part of its own baseline\n",
                 period, out[spike-begIdx] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 1; i <= period; i++ )
      {
         g_rvolEdgeCmp++;
         if( out[spike+i-begIdx] == 1.0 )
         {
            printf( "RVOL spike Fail [N=%d]: bar %d reads exactly 1.0, so the "
                    "spike %d bars back is outside its baseline -- the window "
                    "is shorter than the period\n", period, spike+i, i );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      g_rvolEdgeCmp++;
      if( out[spike+period+1-begIdx] != 1.0 )
      {
         printf( "RVOL spike Fail [N=%d]: bar %d reads %.17g, expected exactly "
                 "1.0 -- the spike is %d bars back and the window reaches only "
                 "%d\n", period, spike+period+1, out[spike+period+1-begIdx],
                 period+1, period );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   retCode = TA_RVOL( 0, 299, in, 1, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != 299 )
   {
      printf( "RVOL period-1 Fail: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbElement; i++ )
   {
      double want = in[i+1] / in[i];

      g_rvolEdgeCmp++;
      if( out[i] != want )
      {
         printf( "RVOL period-1 Fail out %d: %.17g, expected exactly %.17g "
                 "(the baseline is the single previous bar)\n",
                 i, out[i], want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4) The dead-window contract behind `nan_inf_output`.
 *
 * A window in which nothing traded has no average to divide by, and TA_RVOL
 * says so instead of substituting a guarded 0: +Inf when the current bar did
 * trade, NaN when it did not either. test_abstract.c holds every function
 * WITHOUT that flag to finite output on its zero dataset, so dropping the flag
 * and dropping this leg would have to happen together -- which is what makes
 * the flag a contract rather than an annotation.
 */
static ErrorNumber test_rvol_deadwindow( void )
{
   static TA_Real in[100], out[RVOL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   const int period = 10;
   int i;

   for( i = 0; i < 100; i++ )
      in[i] = ( i >= 50 && i < 70 ) ? 0.0 : 1000.0;

   retCode = TA_RVOL( 0, 99, in, period, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != period || nbElement != 100 - period )
   {
      printf( "RVOL dead-window Fail: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = begIdx; i < 100; i++ )
   {
      double got = out[i-begIdx];
      int deadWindow = ( i-period >= 50 && i-1 < 70 );

      g_rvolNanCmp++;
      if( deadWindow )
      {
         if( isfinite( got ) )
         {
            printf( "RVOL dead-window Fail at bar %d: %.17g is finite, but the "
                    "%d preceding bars all traded nothing -- there is no ratio "
                    "to report and rvol.yaml declares nan_inf_output\n",
                    i, got, period );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( ( in[i] == 0.0 ) != ( isnan( got ) != 0 ) )
         {
            printf( "RVOL dead-window Fail at bar %d: volume %.17g over a dead "
                    "window gave %.17g; expected NaN only when the bar itself "
                    "traded nothing\n", i, in[i], got );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      else if( !isfinite( got ) )
      {
         printf( "RVOL dead-window Fail at bar %d: %.17g, but its baseline "
                 "window has volume\n", i, got );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, outReal == inVolume, bitwise.
 *
 * Anchored at bar 0 so startIdx clamps to optInTimePeriod, which is the one
 * alignment where the first output slot and the first trailing read are the
 * same element. Nothing else in this file reaches that coincidence.
 */
static ErrorNumber test_rvol_aliasing( const char *tag, const TA_Real *in,
                                       int nbBars )
{
   static TA_Real clean[RVOL_CAP], alias[RVOL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int period, i;

   for( period = 1; period <= RVOL_MAX_PER; period++ )
   {
      retCode = TA_RVOL( 0, nbBars-1, in, period, &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "RVOL alias Fail [%s N=%d]: rc=%d\n", tag, period, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = in[i];
      retCode = TA_RVOL( 0, nbBars-1, alias, period, &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "RVOL alias Fail [%s N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 tag, period, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_rvolAliasCmp++;
         if( clean[i] != alias[i] )
         {
            printf( "RVOL alias Fail [%s N=%d] out %d: separate %.17g, in-place "
                    "%.17g -- a store landed on the bar the running total was "
                    "about to subtract\n", tag, period, i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) The startIdx/endIdx range sweep. TA_STABLE_EPSILON: the baseline is a
 * running total seeded by a fresh sum that starts wherever startIdx puts it, so
 * two ranges may land a few ulp apart on the same bar. No unstable-period id
 * (matching its abstract metadata, which doRangeTestEx cross-checks against the
 * stability class). */
typedef struct { int period; const TA_Real *in; } RvolRangeParam;

static TA_RetCode rvolRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   RvolRangeParam *p = (RvolRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_RVOL_Lookback( p->period );
   return TA_RVOL( startIdx, endIdx, p->in, p->period,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_rvol_range( const TA_Real *in )
{
   RvolRangeParam param;

   param.period = 20;
   param.in     = in;

   return doRangeTestEx( rvolRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
