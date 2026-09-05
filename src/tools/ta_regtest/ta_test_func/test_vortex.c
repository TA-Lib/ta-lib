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
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #349).
 */

/* Description:
 *
 *   Hand-written tests for TA_VORTEX (Vortex Indicator).
 *
 *   (1) DIFFERENTIAL, bit-exact: the shipped fused loop against a test-only
 *       reference built from TA_TRANGE + TA_SUM for the denominator and a
 *       hand abs-diff + TA_SUM for the two numerators, over a period and
 *       startIdx grid. This is the composite-category pattern, but VORTEX is
 *       not a pure composition of shipped functions (the numerators need the
 *       abs-diff helper), so it lives here rather than in test_composite.
 *       It proves the fusion, NOT the formula.
 *
 *   (2) FORMULA: frozen goldens at the defaults, rel 1e-12, triple-sourced
 *       on the issue (pandas-ta-classic 0.6.52 / from-scratch numpy
 *       transcription of StockCharts / talib-0.6.8-primitives composition,
 *       all three bit-identical to each other on this corpus). rel 1e-12,
 *       NOT bitwise: pandas' rolling().sum() is Kahan-compensated and drifts
 *       ~1.5e-15 relative from a naive running sum at n=100, so a bitwise
 *       pin buys a nightly red on a different period or corpus.
 *
 *   (3) ALL-FLAT WINDOW: H==L==C sums the true range to exactly zero; with
 *       no guard the ratios are NaN, so asserting exactly 0.0 is the guard's
 *       own non-vacuity (the CMOU trick), and satisfies #112.
 *
 *   (4) IN-PLACE ALIASING, each of the 2 outputs onto each of the 3 inputs:
 *       the fused loop's trailing subtraction re-reads bars trailingIdx-1
 *       and trailingIdx (== output slots outIdx and outIdx+1), so the
 *       outputs must be written LAST. No generic gate exercises
 *       input==output, and all four backends inherit the same order from
 *       one input .c -- --xlang-hash would agree bitwise on the wrong
 *       answer. Only this leg can see it.
 *
 *   (5) EDGES: startIdx == endIdx, startIdx < lookback clamping, and n=1
 *       (single-bar window; the ratio is still well-defined).
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

#define VORTEX_CAP 300

static const int vortexPeriodGrid[] = { 1, 2, 14, 21, 30, 100 };
#define NB_VORTEX_PERIOD (sizeof(vortexPeriodGrid)/sizeof(vortexPeriodGrid[0]))

static const int vortexStartGrid[] = { 0, 1, 14, 15, 100, 251 };
#define NB_VORTEX_START (sizeof(vortexStartGrid)/sizeof(vortexStartGrid[0]))

/* Frozen goldens on TA_SREF_{high,low,close}_daily_ref_0_PRIV, n=14,
 * outBegIdx=14, outNBElement=238 -- from issue #349, triple-sourced. */
static const struct { int bar; double plus; double minus; } vortexPins[] =
{
   {  14, 0.91516119373190941, 0.90527996806068456 },
   {  20, 0.90702681542376196, 0.96036406341749847 },
   {  50, 0.99915659263424228, 0.81993252741073930 },
   { 125, 1.19342208300704770, 0.63997650743931100 },
   { 200, 0.72637931034482750, 1.22413793103448270 },
   { 251, 0.93942403177755707, 1.02333664349553130 },
};
#define NB_VORTEX_PINS (sizeof(vortexPins)/sizeof(vortexPins[0]))
#define VORTEX_PIN_REL 1e-12
#define VORTEX_PIN_ABS 1e-12

/* Reference: TA_TRANGE + hand abs-diffs, each through TA_SUM(n), aligned so
 * ref index 0 is output bar `refStart`. Terms exist from bar 1; TA_SUM over
 * them yields its first output at term index n-1 == bar n. */
static ErrorNumber vortex_build_reference( const TA_History *history,
                                           int n, int refStart,
                                           TA_Real *refPlus, TA_Real *refMinus,
                                           int *refNb )
{
   static TA_Real tr[VORTEX_CAP], vmp[VORTEX_CAP], vmm[VORTEX_CAP];
   static TA_Real sTR[VORTEX_CAP], sP[VORTEX_CAP], sM[VORTEX_CAP];
   TA_Integer beg, nb, begS, nbS;
   TA_RetCode rc;
   int i, nbBars = (int)history->nbBars, nTerms, off;

   rc = TA_TRANGE( 0, nbBars - 1, history->high, history->low, history->close,
                   &beg, &nb, tr );
   if( rc != TA_SUCCESS || beg != 1 )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   /* terms at bar i land in slot i-1, matching TRANGE's own alignment. */
   nTerms = nbBars - 1;
   for( i = 1; i < nbBars; i++ )
   {
      vmp[i-1] = fabs( history->high[i] - history->low[i-1] );
      vmm[i-1] = fabs( history->low[i] - history->high[i-1] );
   }

   if( n == 1 )
   {
      /* TA_SUM's optInTimePeriod floor is 2; SUM(x,1) == x, so the terms ARE
       * the sums. Slot i covers bar i+1; the caller indexes bar - refStart
       * below, so shift the terms to start at bar refStart's slot. */
      nbS = (TA_Integer)(nTerms - (refStart - n));
      for( i = 0; i < (int)nbS; i++ )
      {
         sTR[i] = tr[refStart - n + i];
         sP[i]  = vmp[refStart - n + i];
         sM[i]  = vmm[refStart - n + i];
      }
   }
   else
   {
      /* Anchor TA_SUM at the same bar the fused loop primes on: its running
       * sum accumulates rounding differently from a fresh prime, so a
       * reference rolled from term 0 is NOT bit-identical to a fused call
       * whose startIdx exceeded the lookback (measured: 1e-15-class drift at
       * n=100, startIdx=251). With matching anchors both sides prime the
       * same n-1 terms and roll the same number of steps. */
      int sumStart = refStart - 1;   /* term coords: slot 0 must be the window ENDING at term refStart-1 == bar refStart */
      rc = TA_SUM( sumStart, nTerms - 1, tr, n, &begS, &nbS, sTR );
      if( rc != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      rc = TA_SUM( sumStart, nTerms - 1, vmp, n, &begS, &nbS, sP );
      if( rc != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      rc = TA_SUM( sumStart, nTerms - 1, vmm, n, &begS, &nbS, sM );
      if( rc != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* TA_SUM slot j covers term slots [j, j+n-1] wait -- begS = n-1 in term
    * coords, so sum slot k is bars [k+1 .. k+n] ending at bar k+n. Output
    * bar t therefore reads sum slot t - n - (begS - (n-1))... begS == n-1
    * exactly (TA_SUM lookback), so sum slot k ends at bar k + n and output
    * bar t is slot t - n. */
   *refNb = 0;
   for( i = refStart; i < nbBars; i++ )
   {
      off = i - refStart;   /* slot 0 is bar refStart after the anchoring */
      if( off < 0 || off >= (int)nbS )
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      if( TA_IS_ZERO( sTR[off] ) )
      {
         refPlus[*refNb]  = 0.0;
         refMinus[*refNb] = 0.0;
      }
      else
      {
         refPlus[*refNb]  = sP[off] / sTR[off];
         refMinus[*refNb] = sM[off] / sTR[off];
      }
      (*refNb)++;
   }
   return TA_TEST_PASS;
}

ErrorNumber test_func_vortex( TA_History *history )
{
   static TA_Real outP[VORTEX_CAP], outM[VORTEX_CAP];
   static TA_Real refP[VORTEX_CAP], refM[VORTEX_CAP];
   static TA_Real aH[VORTEX_CAP], aL[VORTEX_CAP], aC[VORTEX_CAP], aOther[VORTEX_CAP];
   TA_RetCode rc;
   TA_Integer beg, nb;
   ErrorNumber e;
   double err;
   const char *mode;
   unsigned int p, s, pin;
   int i, o, in, nbBars, refStart, refNb, lookback, nbChecked = 0;

   nbBars = (int)history->nbBars;

   /* (1) Differential over the grid, bit-exact. */
   for( s = 0; s < NB_VORTEX_START; s++ )
   {
      int startIdx = vortexStartGrid[s];
      for( p = 0; p < NB_VORTEX_PERIOD; p++ )
      {
         int n = vortexPeriodGrid[p];

         rc = TA_VORTEX( startIdx, nbBars - 1, history->high, history->low,
                         history->close, n, &beg, &nb, outP, outM );
         if( rc != TA_SUCCESS )
         {
            printf( "VORTEX differential Fail [start %d n %d]: retCode %d\n",
                    startIdx, n, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         lookback = TA_VORTEX_Lookback( n );
         refStart = (startIdx < lookback) ? lookback : startIdx;
         if( refStart > nbBars - 1 )
         {
            if( nb != 0 )
            {
               printf( "VORTEX differential Fail [start %d n %d]: expected no "
                       "output, got nb=%d\n", startIdx, n, (int)nb );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            continue;
         }
         if( (int)beg != refStart || (int)nb != nbBars - refStart )
         {
            printf( "VORTEX differential Fail [start %d n %d]: range (%d,%d), "
                    "expected (%d,%d)\n", startIdx, n, (int)beg, (int)nb,
                    refStart, nbBars - refStart );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         e = vortex_build_reference( history, n, refStart, refP, refM, &refNb );
         if( e != TA_TEST_PASS )
         {
            printf( "VORTEX differential Fail [start %d n %d]: reference "
                    "construction failed\n", startIdx, n );
            return e;
         }
         if( refNb != (int)nb )
         {
            printf( "VORTEX differential Fail [start %d n %d]: reference nb %d "
                    "vs %d\n", startIdx, n, refNb, (int)nb );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < (int)nb; i++ )
         {
            if( memcmp( &outP[i], &refP[i], sizeof(TA_Real) ) != 0 ||
                memcmp( &outM[i], &refM[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "VORTEX differential Fail [start %d n %d] bar %d: fused "
                       "(%.17g,%.17g) != composed (%.17g,%.17g)\n",
                       startIdx, n, (int)beg + i, outP[i], outM[i], refP[i], refM[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            nbChecked++;
         }
      }
   }
   if( nbChecked < 4000 )
   {
      printf( "VORTEX differential Fail: only %d value(s) compared; the grid "
              "has been reduced to the point where this leg is no longer "
              "evidence\n", nbChecked );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (2) Formula: the frozen triple-sourced goldens, n=14. */
   rc = TA_VORTEX( 0, nbBars - 1, history->high, history->low, history->close,
                   14, &beg, &nb, outP, outM );
   if( rc != TA_SUCCESS || beg != 14 || nb != nbBars - 14 )
   {
      printf( "VORTEX pins Fail: retCode %d range (%d,%d), expected (14,%d)\n",
              (int)rc, (int)beg, (int)nb, nbBars - 14 );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( pin = 0; pin < NB_VORTEX_PINS; pin++ )
   {
      int idx = vortexPins[pin].bar - (int)beg;
      if( !checkOracleValue( outP[idx], vortexPins[pin].plus,
                             VORTEX_PIN_REL, VORTEX_PIN_ABS, &err, &mode ) )
      {
         printf( "VORTEX pins Fail bar %d outPlusVI: got %.17g expected %.17g "
                 "(%s=%.3e)\n", vortexPins[pin].bar, outP[idx],
                 vortexPins[pin].plus, mode, err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( !checkOracleValue( outM[idx], vortexPins[pin].minus,
                             VORTEX_PIN_REL, VORTEX_PIN_ABS, &err, &mode ) )
      {
         printf( "VORTEX pins Fail bar %d outMinusVI: got %.17g expected %.17g "
                 "(%s=%.3e)\n", vortexPins[pin].bar, outM[idx],
                 vortexPins[pin].minus, mode, err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* (3) All-flat window: sTR == 0 exactly; unguarded this is NaN, so exact
    * 0.0 is the guard's own non-vacuity. */
   for( i = 0; i < 64; i++ )
   {
      aH[i] = 100.0;
      aL[i] = 100.0;
      aC[i] = 100.0;
   }
   rc = TA_VORTEX( 0, 63, aH, aL, aC, 14, &beg, &nb, outP, outM );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "VORTEX flat Fail: retCode %d nb %d\n", (int)rc, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < (int)nb; i++ )
   {
      if( outP[i] != 0.0 || outM[i] != 0.0 )
      {
         printf( "VORTEX flat Fail bar %d: (%.17g,%.17g) != exact 0.0 -- the "
                 "zero-denominator guard did not fire\n",
                 (int)beg + i, outP[i], outM[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* (3b) ABSORPTION: a large spread swallowing a 1-ULP one leaves the
    * running sTR exactly 0.0 while nullRun is far below n (the #377 review
    * reproducer: flat 100s, bar 20 spread 256, bar 21 spread ~1.4e-14 --
    * absorbed on add, gone entirely once 256 expires). A flat-count-only
    * guard emits NaN/+Inf there; the exact `curTR > 0.0` division gate
    * emits 0.0. Assert every output finite AND the absorbed window's bars
    * exactly 0.0. */
   for( i = 0; i < 40; i++ )
   {
      aH[i] = 100.0;
      aL[i] = 100.0;
      aC[i] = 100.0;
   }
   aH[20] = 356.0;
   aH[21] = 100.00000000000001;
   rc = TA_VORTEX( 0, 39, aH, aL, aC, 14, &beg, &nb, outP, outM );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "VORTEX absorption Fail: retCode %d nb %d\n", (int)rc, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < (int)nb; i++ )
   {
      if( !(outP[i] > -1e300 && outP[i] < 1e300) ||
          !(outM[i] > -1e300 && outM[i] < 1e300) )
      {
         printf( "VORTEX absorption Fail bar %d: (%.17g,%.17g) is not finite -- "
                 "the division gate did not fire on the absorbed-to-zero sum\n",
                 (int)beg + i, outP[i], outM[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   /* Bar 34 is the ONLY bar where the exact division gate and a flat-count
    * proxy disagree: its window [21..34] still holds bar 21's live term, so
    * nullRun < n and a proxy would divide -- by the absorbed exact 0.0,
    * emitting NaN/Inf. The gate answers 0.0. (At bar 35 the flat-run purge
    * fires and both agree, so pinning only 35 could not fail on the defect
    * this leg exists for.) */
   i = 34 - (int)beg;
   if( outP[i] != 0.0 || outM[i] != 0.0 )
   {
      printf( "VORTEX absorption Fail bar 34: (%.17g,%.17g), expected the "
              "denominator gate's exact 0.0\n", outP[i], outM[i] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (3c) HALT-RESUME: a spread bar, a full flat window, then resumption --
    * the #377 second-review reproducer. A zero TRUE RANGE does not zero that
    * bar's vortex terms (they read the PREVIOUS bar's extremes), so a reseed
    * that zeroes the numerator sums poisons both lines permanently; the
    * unreachable symptom is a NEGATIVE -VI (a ratio of fabs sums). Assert
    * non-negativity everywhere and the resumption values against a naive
    * fresh-sum reference at 1e-12. */
   for( i = 0; i < 40; i++ )
   {
      aH[i] = 100.0;
      aL[i] = 100.0;
      aC[i] = 100.0;
   }
   aH[0] = 110.0;
   aL[0] = 95.0;
   /* Resumption at bar 15 exactly: the flat run first reaches n at bar 14
    * (bars 1..14), so a sums purge fires there while bar 1's live vortex
    * terms are still inside the window and about to be retired -- the one
    * phase where an all-sums purge leaves the running numerators negative
    * and the VERY NEXT bar divides by a live TR before any later purge can
    * launder them. One more flat bar and the corruption self-erases. */
   aH[15] = 109.0;
   aL[15] = 100.0;
   aC[15] = 108.0;
   aH[16] = 111.0;
   aL[16] = 103.0;
   aC[16] = 110.0;
   rc = TA_VORTEX( 0, 39, aH, aL, aC, 14, &beg, &nb, outP, outM );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "VORTEX halt-resume Fail: retCode %d nb %d\n", (int)rc, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < (int)nb; i++ )
   {
      int bar = (int)beg + i;
      double nTR = 0.0, nP = 0.0, nM = 0.0, wantP0, wantM0;
      int j;
      if( outP[i] < 0.0 || outM[i] < 0.0 )
      {
         printf( "VORTEX halt-resume Fail bar %d: (%.17g,%.17g) -- a negative "
                 "line is unreachable from fabs sums; the numerator sums were "
                 "poisoned\n", bar, outP[i], outM[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      /* Naive fresh sums per bar: the reference the running form must track. */
      for( j = bar - 14 + 1; j <= bar; j++ )
      {
         double t1 = aH[j] - aL[j];
         double t2 = fabs( aC[j-1] - aH[j] );
         double t3 = fabs( aC[j-1] - aL[j] );
         double tr = t1;
         if( t2 > tr ) tr = t2;
         if( t3 > tr ) tr = t3;
         nTR += tr;
         nP  += fabs( aH[j] - aL[j-1] );
         nM  += fabs( aL[j] - aH[j-1] );
      }
      wantP0 = ( nTR > 0.0 ) ? nP / nTR : 0.0;
      wantM0 = ( nTR > 0.0 ) ? nM / nTR : 0.0;
      if( fabs( outP[i] - wantP0 ) > 1e-12 || fabs( outM[i] - wantM0 ) > 1e-12 )
      {
         printf( "VORTEX halt-resume Fail bar %d: (%.17g,%.17g) vs naive "
                 "(%.17g,%.17g)\n", bar, outP[i], outM[i], wantP0, wantM0 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* (4) In-place aliasing: each output onto each input. The reference run
    * uses separate buffers; each aliased run must be bit-identical. */
   rc = TA_VORTEX( 0, nbBars - 1, history->high, history->low, history->close,
                   14, &beg, &nb, refP, refM );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   for( o = 0; o < 2; o++ )
   {
      for( in = 0; in < 3; in++ )
      {
         TA_Real *outs[2];
         for( i = 0; i < nbBars; i++ )
         {
            aH[i] = history->high[i];
            aL[i] = history->low[i];
            aC[i] = history->close[i];
         }
         outs[o] = (in == 0) ? aH : (in == 1) ? aL : aC;
         outs[1-o] = aOther;
         rc = TA_VORTEX( 0, nbBars - 1, aH, aL, aC, 14, &beg, &nb,
                         outs[0], outs[1] );
         /* outs[0] is always the outPlusVI POSITION and outs[1] outMinusVI;
          * `o` only chooses which of the two got aliased onto an input. */
         if( rc != TA_SUCCESS ||
             memcmp( outs[0], refP, (size_t)nb * sizeof(TA_Real) ) != 0 ||
             memcmp( outs[1], refM, (size_t)nb * sizeof(TA_Real) ) != 0 )
         {
            printf( "VORTEX aliasing Fail [out%d over in%d]: aliased call "
                    "differs from separate-buffer call\n", o, in );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* (5) Edges. */
   rc = TA_VORTEX( 14, 14, history->high, history->low, history->close,
                   14, &beg, &nb, outP, outM );
   if( rc != TA_SUCCESS || beg != 14 || nb != 1 )
   {
      printf( "VORTEX edge Fail: startIdx==endIdx gave (%d,%d)\n",
              (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   if( memcmp( &outP[0], &refP[0], sizeof(TA_Real) ) != 0 )
   {
      printf( "VORTEX edge Fail: single-bar call differs from full-range bar 14\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   rc = TA_VORTEX( 0, nbBars - 1, history->high, history->low, history->close,
                   1, &beg, &nb, outP, outM );
   if( rc != TA_SUCCESS || beg != 1 || nb != nbBars - 1 )
   {
      printf( "VORTEX edge Fail: n=1 gave retCode %d range (%d,%d)\n",
              (int)rc, (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   return TA_TEST_PASS;
}
