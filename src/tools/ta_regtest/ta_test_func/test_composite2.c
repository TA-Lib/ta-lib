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
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  082026 MF,CC  First version. SMI legs (#238).
 *  090526 KL     COPPOCK legs (#362).
 *
 */

/* Description:
 *
 *   Hand-written tests for TA_SMI (Stochastic Momentum Index).
 *
 *   SMI is a COMPOSITE function in the sense test_composite.c describes: it is
 *   an exact arithmetic composition of shipped primitives (TA_MAX, TA_MIN and
 *   four TA_EMA), fused into one pass because the streaming producer model
 *   carries only one intermediate series and SMI has two. It lives in its own
 *   file rather than in test_composite.c because that file had accreted nine
 *   indicators; every other group in ta_test_func/ is one file per group.
 *
 *   Legs:
 *
 *   (1) DIFFERENTIAL, bit-exact, against a reference built only from shipped
 *       primitives. The reference is ANCHORED: for a requested startIdx it
 *       begins its TA_MAX/TA_MIN at (refStart - lookback) + (period-1), so each
 *       TA_EMA seeds on exactly the bars the fused loop seeds on. TA_EMA
 *       re-seeds at startIdx-lookback, so an unanchored reference (always from
 *       bar 0) is a DIFFERENT function once startIdx exceeds the lookback --
 *       leg (2) proves the grid actually separates the two.
 *
 *       The grid is crossed with an UNSTABLE-PERIOD sweep, and that is the
 *       load-bearing part: SMI's stage boundaries are the callee lookbacks, not
 *       (period-1), and the two coincide exactly when the unstable period is 0.
 *       Measured during development: swapping them keeps every unstable-period-0
 *       cell bit-exact while breaking 152 of 198 bars at unstable period 3. A
 *       default-parameter suite cannot see that class of bug at all.
 *
 *   (2) ANCHORING NON-VACUITY. Asserts at least one grid cell where the
 *       anchored and unanchored references disagree, so leg (1) is evidence.
 *
 *   (3) REGRESSION PINS on the TA_SREF corpus. These are pins, not an external
 *       oracle: they were produced by the in-repo study
 *       docs/studies/ema-seeding/seed_study.py, which reproduces every printed
 *       digit of the arm-A table in #238. Formula correctness is the Tulip
 *       arm's job (ti_smi), not these values'.
 *
 *   (4) FLAT WINDOW. Every bar with high == low makes the denominator zero and
 *       the numerator zero with it, so the guarded 0/0 must emit 0.0 (the CCI/#7
 *       and IMI/#112 convention). Carries an OVER-FIRE CONTROL: a range that is
 *       small but genuine must NOT be swallowed by the guard.
 *
 *   (5) BOUND. |SMI| <= 100 follows from |num| <= den/2 plus positive EMA
 *       weights, and survives in floating point. Checked over the whole grid.
 *
 *   (6) IN-PLACE ALIASING. outSMI aliased onto each of the three inputs.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

/**** Local declarations. ****/
#define SMI_CAP 300   /* > MAX_NB_TEST_ELEMENT and > nbBars */

/* Parameter grid. Includes the two published defaults, the smallest legal
 * periods, a fast>slow pair (SMI does not swap them -- the pipeline order is
 * fixed), and a signal period longer than the corpus so the
 * "nothing to evaluate" path is walked. */
static const struct { int q; int fast; int slow; int sig; } smiGrid[] =
{
   {  13,  2, 25,  9 },   /* Blau's own, and TTR's defaults */
   {  10,  3,  3,  9 },   /* the platform-common set */
   {   5,  3,  3,  4 },
   {  14,  5, 20,  5 },
   {   4,  2,  2,  2 },   /* small window, smallest legal smoothing */
   {   2,  2,  2,  2 },   /* smallest legal everywhere */
   {  25,  2,  3,  2 },   /* fast<slow, both tiny */
   {   3, 20,  2,  4 },   /* fast>slow: no swap, unlike MACD */
   {  13,  2, 25, 300 },  /* signal longer than the corpus */
};
#define NB_SMI_GRID (sizeof(smiGrid)/sizeof(smiGrid[0]))

/* startIdx grid. Values ABOVE the lookback are what separate an anchored
 * reference from an unanchored one; 0 and 1 clamp to the lookback and cannot. */
static const int smiStartGrid[] = { 0, 1, 60, 100, 175, 251 };
#define NB_SMI_START (sizeof(smiStartGrid)/sizeof(smiStartGrid[0]))

/* Unstable-period sweep. 0 must be present (it is the shipped default); the
 * non-zero values are the only ones that separate a lookback-anchored stage
 * boundary from a (period-1) one. */
static const int smiUnstGrid[] = { 0, 1, 3, 7 };
#define NB_SMI_UNST (sizeof(smiUnstGrid)/sizeof(smiUnstGrid[0]))

/* Regression pins on TA_SREF, produced by docs/studies/ema-seeding/seed_study.py.
 * Pins, not an external oracle -- see the header. */
static const struct { int q, fast, slow, sig, bar; double smi, signal; } smiPins[] =
{
   { 13, 2, 25, 9,  45, -10.963685719412787, -26.360480864391562 },
   { 13, 2, 25, 9,  46,  -4.7979771763643688, -22.047980126786122 },
   { 13, 2, 25, 9, 100,  46.476911546185406,  56.201643761261806 },
   { 13, 2, 25, 9, 175,  22.317000100616923,   7.9982559042671388 },
   { 13, 2, 25, 9, 251,  -9.9844363247192067,  -4.6069927028646864 },
   { 10, 3,  3, 9,  21, -60.572848824195567, -25.777927377318456 },
   { 10, 3,  3, 9,  22, -70.088392931382515, -34.64002048813127 },
   { 10, 3,  3, 9, 100,  -8.6056814147885223, 21.814295314186943 },
   { 10, 3,  3, 9, 175,  49.680170743925423,  39.994666934559099 },
   { 10, 3,  3, 9, 251,   4.3484442819111404, -8.0202494511492404 },
};
#define NB_SMI_PINS (sizeof(smiPins)/sizeof(smiPins[0]))

/* The pins come from a double-precision reimplementation of the same recursion,
 * so agreement is at rounding level. The absolute floor covers the pins that
 * sit near zero, where the relative term is meaningless. */
#define SMI_PIN_REL 1e-12
#define SMI_PIN_ABS 1e-12

/* (7) EXTERNAL ORACLE: Tulip Indicators 0.9.2 (git be18abb13e075ba866898dcc7cb
 * 52399603302a6, build 1645649572), via ta_tulip_serve's TA_SMI arm over
 * TA_SREF_high/low/close_daily_ref_0_PRIV. Captured mechanically, not
 * transcribed.
 *
 * This is the ONLY leg that tests the FORMULA. The differential proves the
 * fused loop matches a composition of our own primitives; both sides of it
 * could share a wrong formula. Tulip is an independent C implementation.
 *
 * BIT-EXACT, and it has to be earned: Tulip seeds each EMA with its first
 * sample while TA_SMI seeds with an SMA, so the two only CONVERGE. Measured on
 * this corpus: bit-identical from bar 63 at (10,3,3), bar 71 at (5,3,3), bar 73
 * at (14,3,3) -- and never within 252 bars at slow=25, where the last 30 bars
 * still differ by ~6.7e-06. Every bar below is past its convergence point, so
 * equality is the right assertion; a vector at slow=25 would need a tolerance
 * and would be testing the warm-up, not the formula.
 *
 * ti_smi emits ONE output, so this leg says nothing about outSMISignal. */
static const struct { int q, fast, slow, bar; double smi; } smiTulip[] =
{
   { 10, 3, 3, 100,  -8.6056814147885223 },
   { 10, 3, 3, 150, -28.490706033510481  },
   { 10, 3, 3, 200, -69.3892731732725    },
   { 10, 3, 3, 225,  70.621436034359874  },
   { 10, 3, 3, 251,   4.3484442819111404 },
   {  5, 3, 3, 100,  -3.4608737264479363 },
   {  5, 3, 3, 150, -10.378274307491484  },
   {  5, 3, 3, 200, -47.742936107156417  },
   {  5, 3, 3, 225,  68.466634237842953  },
   {  5, 3, 3, 251, -20.77703629000035   },
};
#define NB_SMI_TULIP (sizeof(smiTulip)/sizeof(smiTulip[0]))

static ErrorNumber test_smi_differential( const TA_History *history );
static ErrorNumber test_smi_anchoring( const TA_History *history );
static ErrorNumber test_smi_pins( const TA_History *history );
static ErrorNumber test_smi_flat_window( void );
static ErrorNumber test_smi_inplace( const TA_History *history );
static ErrorNumber test_smi_tulip_vector( const TA_History *history );

/* ---- COPPOCK (issue #362) ---- */

/* Parameter grid. Includes the published defaults, w == 1 (the WMA stage
 * degenerates to identity), p1 == p2, p1 > p2 (accepted, not swapped -- the
 * formula is symmetric and the lookback keys off the max), the smallest legal
 * periods everywhere, and one lag long enough that only a tail of the corpus
 * produces output. */
static const struct { int w, p1, p2; } coppockGrid[] =
{
   { 10, 11, 14 },   /* the published defaults */
   {  1, 11, 14 },   /* w == 1 */
   { 10, 14, 11 },   /* p1 > p2 */
   {  5,  7,  7 },   /* p1 == p2 */
   {  1,  1,  1 },   /* smallest legal everywhere */
   {  3,  1, 200 },  /* long lag, output only near the tail */
   { 20,  2,  2 },
};
#define NB_COPPOCK_GRID (sizeof(coppockGrid)/sizeof(coppockGrid[0]))

/* startIdx grid: 0/1 clamp to the lookback; the rest move the WMA re-anchor
 * phase, which the reference must reproduce for the memcmp to hold. */
static const int coppockStartGrid[] = { 0, 1, 23, 24, 100, 251 };
#define NB_COPPOCK_START (sizeof(coppockStartGrid)/sizeof(coppockStartGrid[0]))

/* Regression pins on TA_SREF close, defaults (10,11,14): pandas 2.3.3, the
 * textbook composition (ROC sum -> rolling weighted mean, explicit dot over
 * weights 1..10 / 55), captured locally. Spot bars picked with |value| >~ 1;
 * outBegIdx/outNBElement on this corpus are 23/229.
 *
 * rel 1e-11, NOT tighter: TA_WMA carries a running periodSum/periodSub across
 * the range where pandas recomputes a fresh dot per window, and that residue
 * random-walks with call length (measured worst rel ~2e-12 on this corpus).
 * The gap is shipped-TA_WMA behaviour, not COPPOCK's -- do not "fix" a future
 * excursion by loosening this to 1e-6. */
static const struct { int bar; double v; } coppockPins[] =
{
   {  23, -13.478673401598931 },
   {  60, -5.6416423019625759 },
   { 100, 15.80717920778371 },
   { 200, -22.586650343498274 },
   { 251, -4.566857586063839 },
};
#define NB_COPPOCK_PINS (sizeof(coppockPins)/sizeof(coppockPins[0]))
#define COPPOCK_PIN_REL 1e-11
#define COPPOCK_PIN_ABS 1e-11

static ErrorNumber test_coppock_differential( const TA_History *history );
static ErrorNumber test_coppock_pins( const TA_History *history );
static ErrorNumber test_coppock_flat_and_zero_guard( void );
static ErrorNumber test_coppock_inplace( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_composite2( TA_History *history )
{
   ErrorNumber retValue;

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );

   retValue = test_smi_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_smi_anchoring( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_smi_pins( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_smi_flat_window();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_smi_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_smi_tulip_vector( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_coppock_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_coppock_pins( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_coppock_flat_and_zero_guard();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_coppock_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Every leg above restores it, but a future leg might not. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* Build the SMI line and its signal from shipped primitives only.
 *
 * `base` is the first input bar the caller is allowed to consume. Passing
 * (refStart - lookback) anchors the reference exactly where TA_SMI anchors;
 * passing 0 always builds the UNANCHORED variant leg (2) contrasts against.
 */
static ErrorNumber smi_build_reference( const TA_History *history,
                                        int q, int fast, int slow, int sig,
                                        int base, int endIdx,
                                        double *outLine, double *outSignal,
                                        int *refBeg, int *refNb )
{
   static TA_Real hh[SMI_CAP], ll[SMI_CAP];
   static TA_Real num[SMI_CAP], den[SMI_CAP];
   static TA_Real e1[SMI_CAP], e2[SMI_CAP];
   static TA_Real f1[SMI_CAP], f2[SMI_CAP];
   static TA_Real line[SMI_CAP];
   TA_RetCode rc;
   TA_Integer begHH, nbHH, begLL, nbLL;
   TA_Integer begE1, nbE1, begE2, nbE2;
   TA_Integer begF1, nbF1, begF2, nbF2;
   TA_Integer begSg, nbSg;
   int i, lineBar;

   rc = TA_MAX( base + q - 1, endIdx, history->high, q, &begHH, &nbHH, hh );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   rc = TA_MIN( base + q - 1, endIdx, history->low, q, &begLL, &nbLL, ll );
   if( rc != TA_SUCCESS || begLL != begHH || nbLL != nbHH )
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   if( nbHH <= 0 )
   {
      *refBeg = 0; *refNb = 0;
      return TA_TEST_PASS;
   }

   for( i = 0; i < nbHH; i++ )
   {
      num[i] = history->close[begHH + i] - ((hh[i] + ll[i]) * 0.5);
      den[i] = hh[i] - ll[i];
   }

   /* Slow stage, then fast stage, on numerator and denominator separately. */
   rc = TA_EMA( 0, nbHH - 1, num, slow, &begE1, &nbE1, e1 );
   if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
   rc = TA_EMA( 0, nbHH - 1, den, slow, &begF1, &nbF1, f1 );
   if( rc != TA_SUCCESS || begF1 != begE1 ) return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   if( nbE1 <= 0 ) { *refBeg = 0; *refNb = 0; return TA_TEST_PASS; }

   rc = TA_EMA( 0, nbE1 - 1, e1, fast, &begE2, &nbE2, e2 );
   if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
   rc = TA_EMA( 0, nbF1 - 1, f1, fast, &begF2, &nbF2, f2 );
   if( rc != TA_SUCCESS || begF2 != begE2 ) return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   if( nbE2 <= 0 ) { *refBeg = 0; *refNb = 0; return TA_TEST_PASS; }

   for( i = 0; i < nbE2; i++ )
   {
      double halfDen = 0.5 * f2[i];
      if( !TA_IS_ZERO(halfDen) )
         line[i] = (100.0 * e2[i]) / halfDen;
      else
         line[i] = 0.0;
   }
   lineBar = begHH + begE1 + begE2;   /* absolute bar of line[0] */

   rc = TA_EMA( 0, nbE2 - 1, line, sig, &begSg, &nbSg, outSignal );
   if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( nbSg <= 0 ) { *refBeg = 0; *refNb = 0; return TA_TEST_PASS; }

   memcpy( outLine, &line[begSg], (size_t)nbSg * sizeof(double) );
   *refBeg = lineBar + begSg;
   *refNb  = (int)nbSg;
   return TA_TEST_PASS;
}

/* (1) DIFFERENTIAL, plus (5) the |SMI| <= 100 bound. */
static ErrorNumber test_smi_differential( const TA_History *history )
{
   unsigned int g, s, u;
   int i, nbBars, nbChecked = 0;
   TA_RetCode rc;
   TA_Integer beg, nb;
   ErrorNumber e;
   static TA_Real outSMI[SMI_CAP], outSig[SMI_CAP];
   static TA_Real refLine[SMI_CAP], refSig[SMI_CAP];

   nbBars = (int)history->nbBars;

   for( u = 0; u < NB_SMI_UNST; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, (TA_Integer)smiUnstGrid[u] );

      for( s = 0; s < NB_SMI_START; s++ )
      {
         int startIdx = smiStartGrid[s];

         for( g = 0; g < NB_SMI_GRID; g++ )
         {
            int q = smiGrid[g].q, fast = smiGrid[g].fast;
            int slow = smiGrid[g].slow, sig = smiGrid[g].sig;
            int lookback, refStart, refBeg, refNb;

            rc = TA_SMI( startIdx, nbBars - 1,
                         history->high, history->low, history->close,
                         q, fast, slow, sig, &beg, &nb, outSMI, outSig );
            if( rc != TA_SUCCESS )
            {
               printf( "SMI differential Fail [unst %d start %d q %d f %d s %d sig %d]: retCode %d\n",
                       smiUnstGrid[u], startIdx, q, fast, slow, sig, (int)rc );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }

            lookback = TA_SMI_Lookback( q, fast, slow, sig );
            refStart = startIdx < lookback ? lookback : startIdx;
            if( refStart > nbBars - 1 )
            {
               if( nb != 0 )
               {
                  printf( "SMI differential Fail [unst %d start %d q %d f %d s %d sig %d]: "
                          "expected no output, got nb=%d\n",
                          smiUnstGrid[u], startIdx, q, fast, slow, sig, (int)nb );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_BEGIDX;
               }
               continue;
            }

            if( beg != refStart || nb != nbBars - refStart )
            {
               printf( "SMI differential Fail [unst %d start %d q %d f %d s %d sig %d]: "
                       "range (%d,%d), expected (%d,%d)\n",
                       smiUnstGrid[u], startIdx, q, fast, slow, sig,
                       (int)beg, (int)nb, refStart, nbBars - refStart );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }

            e = smi_build_reference( history, q, fast, slow, sig,
                                     refStart - lookback, nbBars - 1,
                                     refLine, refSig, &refBeg, &refNb );
            if( e != TA_TEST_PASS )
            {
               printf( "SMI differential Fail [unst %d start %d q %d f %d s %d sig %d]: "
                       "reference construction failed\n",
                       smiUnstGrid[u], startIdx, q, fast, slow, sig );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return e;
            }

            if( refBeg != beg || refNb != nb )
            {
               printf( "SMI differential Fail [unst %d start %d q %d f %d s %d sig %d]: "
                       "reference range (%d,%d) vs (%d,%d)\n",
                       smiUnstGrid[u], startIdx, q, fast, slow, sig,
                       refBeg, refNb, (int)beg, (int)nb );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }

            for( i = 0; i < nb; i++ )
            {
               if( outSMI[i] != refLine[i] || outSig[i] != refSig[i] )
               {
                  printf( "SMI differential Fail [unst %d start %d q %d f %d s %d sig %d] bar %d: "
                          "smi %.17g vs %.17g, signal %.17g vs %.17g\n",
                          smiUnstGrid[u], startIdx, q, fast, slow, sig, (int)beg + i,
                          outSMI[i], refLine[i], outSig[i], refSig[i] );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               /* (5) the bound, free on every cell already computed. */
               if( !(fabs(outSMI[i]) <= 100.0 + 1e-9) )
               {
                  printf( "SMI bound Fail [unst %d start %d q %d f %d s %d sig %d] bar %d: "
                          "|SMI| = %.17g exceeds 100\n",
                          smiUnstGrid[u], startIdx, q, fast, slow, sig, (int)beg + i,
                          fabs(outSMI[i]) );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               nbChecked++;
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );

   if( nbChecked < 10000 )
   {
      printf( "SMI differential Fail: only %d value(s) compared; the grid has been "
              "reduced to the point where this leg is no longer evidence\n", nbChecked );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (2) ANCHORING NON-VACUITY: the grid must contain a cell where the anchored
 * reference and an unanchored one (always from bar 0) disagree. Without such a
 * cell, leg (1) would pass against a reference that ignores startIdx. */
static ErrorNumber test_smi_anchoring( const TA_History *history )
{
   unsigned int g, s;
   int i, nbBars = (int)history->nbBars;
   int nbCells = 0, nbSeparating = 0;
   static TA_Real aLine[SMI_CAP], aSig[SMI_CAP];
   static TA_Real bLine[SMI_CAP], bSig[SMI_CAP];

   for( s = 0; s < NB_SMI_START; s++ )
   {
      for( g = 0; g < NB_SMI_GRID; g++ )
      {
         int q = smiGrid[g].q, fast = smiGrid[g].fast;
         int slow = smiGrid[g].slow, sig = smiGrid[g].sig;
         int lookback = TA_SMI_Lookback( q, fast, slow, sig );
         int refStart = smiStartGrid[s] < lookback ? lookback : smiStartGrid[s];
         int aBeg, aNb, bBeg, bNb;

         if( refStart > nbBars - 1 )
            continue;
         if( smi_build_reference( history, q, fast, slow, sig, refStart - lookback,
                                  nbBars - 1, aLine, aSig, &aBeg, &aNb ) != TA_TEST_PASS )
            continue;
         if( smi_build_reference( history, q, fast, slow, sig, 0,
                                  nbBars - 1, bLine, bSig, &bBeg, &bNb ) != TA_TEST_PASS )
            continue;
         if( aNb <= 0 || bNb <= 0 )
            continue;

         nbCells++;
         /* Compare on the overlap, at the anchored run's first bar. */
         i = aBeg - bBeg;
         if( i >= 0 && i < bNb && (aLine[0] != bLine[i] || aSig[0] != bSig[i]) )
            nbSeparating++;
      }
   }

   if( nbCells == 0 )
   {
      printf( "SMI anchoring Fail: no cell built a reference at all\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( nbSeparating == 0 )
   {
      printf( "SMI anchoring Fail: no cell separates the anchored reference from the "
              "unanchored one (%d cell(s)); the differential leg would pass against a "
              "reference that ignores startIdx. Restore a startIdx well above the "
              "lookback to smiStartGrid.\n", nbCells );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (3) REGRESSION PINS. */
static ErrorNumber test_smi_pins( const TA_History *history )
{
   unsigned int p;
   TA_RetCode rc;
   TA_Integer beg, nb;
   double err;
   const char *mode;
   static TA_Real outSMI[SMI_CAP], outSig[SMI_CAP];

   for( p = 0; p < NB_SMI_PINS; p++ )
   {
      int idx;
      rc = TA_SMI( 0, (int)history->nbBars - 1,
                   history->high, history->low, history->close,
                   smiPins[p].q, smiPins[p].fast, smiPins[p].slow, smiPins[p].sig,
                   &beg, &nb, outSMI, outSig );
      if( rc != TA_SUCCESS )
      {
         printf( "SMI pin Fail [q %d f %d s %d sig %d]: retCode %d\n",
                 smiPins[p].q, smiPins[p].fast, smiPins[p].slow, smiPins[p].sig, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      idx = smiPins[p].bar - (int)beg;
      if( idx < 0 || idx >= nb )
      {
         printf( "SMI pin Fail [q %d f %d s %d sig %d]: bar %d outside output (%d,%d)\n",
                 smiPins[p].q, smiPins[p].fast, smiPins[p].slow, smiPins[p].sig,
                 smiPins[p].bar, (int)beg, (int)nb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      if( !checkOracleValue( outSMI[idx], smiPins[p].smi,
                             SMI_PIN_REL, SMI_PIN_ABS, &err, &mode ) )
      {
         printf( "SMI pin Fail [q %d f %d s %d sig %d] bar %d outSMI: got %.17g "
                 "expected %.17g (%s=%.3e > rel %.3e / abs %.3e)\n",
                 smiPins[p].q, smiPins[p].fast, smiPins[p].slow, smiPins[p].sig,
                 smiPins[p].bar, outSMI[idx], smiPins[p].smi, mode, err,
                 SMI_PIN_REL, SMI_PIN_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( !checkOracleValue( outSig[idx], smiPins[p].signal,
                             SMI_PIN_REL, SMI_PIN_ABS, &err, &mode ) )
      {
         printf( "SMI pin Fail [q %d f %d s %d sig %d] bar %d outSMISignal: got %.17g "
                 "expected %.17g (%s=%.3e > rel %.3e / abs %.3e)\n",
                 smiPins[p].q, smiPins[p].fast, smiPins[p].slow, smiPins[p].sig,
                 smiPins[p].bar, outSig[idx], smiPins[p].signal, mode, err,
                 SMI_PIN_REL, SMI_PIN_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (4) FLAT WINDOW plus its over-fire control. */
static ErrorNumber test_smi_flat_window( void )
{
   static TA_Real high[64], low[64], close[64];
   static TA_Real outSMI[SMI_CAP], outSig[SMI_CAP];
   TA_RetCode rc;
   TA_Integer beg, nb;
   int i, n = 64;

   /* Flat: every bar has high == low == close, so den and num are both 0. */
   for( i = 0; i < n; i++ )
   {
      high[i] = 100.0;
      low[i]  = 100.0;
      close[i]= 100.0;
   }
   rc = TA_SMI( 0, n - 1, high, low, close, 5, 3, 3, 4, &beg, &nb, outSMI, outSig );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "SMI flat Fail: retCode %d nb %d\n", (int)rc, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nb; i++ )
   {
      if( outSMI[i] != 0.0 || outSig[i] != 0.0 )
      {
         printf( "SMI flat Fail at bar %d: expected 0/0 to give 0.0, got smi %.17g signal %.17g\n",
                 (int)beg + i, outSMI[i], outSig[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* OVER-FIRE CONTROL. A small but genuine range must NOT be swallowed by the
    * zero guard: with the close pinned to the top of the range the SMI is +100,
    * not 0. If this fires, the guard's band is too wide for real data. */
   for( i = 0; i < n; i++ )
   {
      high[i]  = 100.0 + 1e-5;
      low[i]   = 100.0;
      close[i] = 100.0 + 1e-5;
   }
   rc = TA_SMI( 0, n - 1, high, low, close, 5, 3, 3, 4, &beg, &nb, outSMI, outSig );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "SMI flat control Fail: retCode %d nb %d\n", (int)rc, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nb; i++ )
   {
      if( fabs(outSMI[i] - 100.0) > 1e-6 )
      {
         printf( "SMI flat control Fail at bar %d: a genuine 1e-5 range was swallowed "
                 "by the zero guard (expected +100, got %.17g)\n", (int)beg + i, outSMI[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (6) IN-PLACE ALIASING: outSMI on top of each input in turn. */
static ErrorNumber test_smi_inplace( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begRef, nbRef, begAlias, nbAlias;
   static TA_Real refSMI[SMI_CAP], refSig[SMI_CAP];
   static TA_Real work[SMI_CAP], outSig[SMI_CAP];
   int nbBars = (int)history->nbBars;
   int i, which;
   const int q = 13, fast = 2, slow = 25, sig = 9;

   rc = TA_SMI( 0, nbBars - 1, history->high, history->low, history->close,
                q, fast, slow, sig, &begRef, &nbRef, refSMI, refSig );
   if( rc != TA_SUCCESS || nbRef <= 0 )
   {
      printf( "SMI inplace Fail: reference retCode %d nb %d\n", (int)rc, (int)nbRef );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( which = 0; which < 3; which++ )
   {
      const TA_Real *h = history->high;
      const TA_Real *l = history->low;
      const TA_Real *c = history->close;
      const char *tag = which == 0 ? "outSMI==inHigh"
                      : which == 1 ? "outSMI==inLow" : "outSMI==inClose";

      for( i = 0; i < nbBars; i++ )
         work[i] = which == 0 ? history->high[i]
                 : which == 1 ? history->low[i] : history->close[i];
      if( which == 0 )      h = work;
      else if( which == 1 ) l = work;
      else                  c = work;

      rc = TA_SMI( 0, nbBars - 1, h, l, c, q, fast, slow, sig,
                   &begAlias, &nbAlias, work, outSig );
      if( rc != TA_SUCCESS || begAlias != begRef || nbAlias != nbRef )
      {
         printf( "SMI inplace Fail [%s]: rc=%d range (%d,%d) vs (%d,%d)\n",
                 tag, (int)rc, (int)begAlias, (int)nbAlias, (int)begRef, (int)nbRef );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbRef; i++ )
      {
         if( work[i] != refSMI[i] || outSig[i] != refSig[i] )
         {
            printf( "SMI inplace Fail [%s] at bar %d: smi %.17g vs %.17g, "
                    "signal %.17g vs %.17g\n", tag, (int)begRef + i,
                    work[i], refSMI[i], outSig[i], refSig[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   return TA_TEST_PASS;
}

/* (7) EXTERNAL ORACLE, bit-exact past convergence. */
static ErrorNumber test_smi_tulip_vector( const TA_History *history )
{
   unsigned int v;
   TA_RetCode rc;
   TA_Integer beg, nb;
   static TA_Real outSMI[SMI_CAP], outSig[SMI_CAP];

   for( v = 0; v < NB_SMI_TULIP; v++ )
   {
      int idx;
      /* The signal period does not enter the SMI line; 9 keeps the call at the
       * shipped default and simply moves outBegIdx. */
      rc = TA_SMI( 0, (int)history->nbBars - 1,
                   history->high, history->low, history->close,
                   smiTulip[v].q, smiTulip[v].fast, smiTulip[v].slow, 9,
                   &beg, &nb, outSMI, outSig );
      if( rc != TA_SUCCESS )
      {
         printf( "SMI tulip Fail [q %d f %d s %d]: retCode %d\n",
                 smiTulip[v].q, smiTulip[v].fast, smiTulip[v].slow, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      idx = smiTulip[v].bar - (int)beg;
      if( idx < 0 || idx >= nb )
      {
         printf( "SMI tulip Fail [q %d f %d s %d]: bar %d outside output (%d,%d)\n",
                 smiTulip[v].q, smiTulip[v].fast, smiTulip[v].slow,
                 smiTulip[v].bar, (int)beg, (int)nb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      if( outSMI[idx] != smiTulip[v].smi )
      {
         printf( "SMI tulip Fail [q %d f %d s %d] bar %d: got %.17g, Tulip %.17g "
                 "(delta %.3e). These bars are past the seeding transient, so "
                 "equality is expected -- see the vector's comment.\n",
                 smiTulip[v].q, smiTulip[v].fast, smiTulip[v].slow, smiTulip[v].bar,
                 outSMI[idx], smiTulip[v].smi, outSMI[idx] - smiTulip[v].smi );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* ==================== COPPOCK (issue #362) ==================== */

/* Build the reference from shipped primitives only: TA_ROC(p1) + TA_ROC(p2),
 * summed over the aligned overlap, into TA_WMA(w). `startIdx` matters beyond
 * range selection: TA_WMA's 8*w re-anchor counts from its own clamped start,
 * and TA_COPPOCK's fused stage counts from ITS clamped start, so the WMA leg
 * must start at the S-coordinate of the caller's clamped start or the phases
 * -- and the memcmp -- drift apart.
 */
static ErrorNumber coppock_build_reference( const TA_History *history,
                                            int startIdx,
                                            int w, int p1, int p2,
                                            TA_Real *ref, int *refBeg, int *refNb )
{
   static TA_Real r1[SMI_CAP], r2[SMI_CAP], sum[SMI_CAP], wma[SMI_CAP];
   TA_Integer beg1, nb1, beg2, nb2, begW, nbW;
   TA_RetCode rc;
   int i, n, maxP, lookback, clamped, sWma;
   int nbBars = (int)history->nbBars;

   maxP = (p1 > p2) ? p1 : p2;
   lookback = TA_COPPOCK_Lookback( w, p1, p2 );
   clamped = (startIdx < lookback) ? lookback : startIdx;

   rc = TA_ROC( 0, nbBars - 1, history->close, p1, &beg1, &nb1, r1 );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   rc = TA_ROC( 0, nbBars - 1, history->close, p2, &beg2, &nb2, r2 );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   /* sum[k] is bar maxP+k -- the first bar where both ROCs exist. */
   n = nbBars - maxP;
   for( i = 0; i < n; i++ )
      sum[i] = r1[maxP + i - (int)beg1] + r2[maxP + i - (int)beg2];

   sWma = clamped - maxP;   /* >= w-1 because clamped >= maxP + w - 1 */
   rc = TA_WMA( sWma, n - 1, sum, w, &begW, &nbW, wma );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   *refBeg = maxP + (int)begW;
   *refNb  = (int)nbW;
   for( i = 0; i < (int)nbW; i++ )
      ref[i] = wma[i];
   return TA_TEST_PASS;
}

/* (1) COMPOSITE DIFFERENTIAL, bit-exact. Proves the fusion, not the formula:
 * both sides could share a wrong formula, which is what the pins are for. */
static ErrorNumber test_coppock_differential( const TA_History *history )
{
   unsigned int g, s;
   int i, nbBars, nbChecked = 0;
   TA_RetCode rc;
   TA_Integer beg, nb;
   ErrorNumber e;
   int refBeg, refNb, lookback, refStart;
   static TA_Real out[SMI_CAP], ref[SMI_CAP];

   nbBars = (int)history->nbBars;

   for( s = 0; s < NB_COPPOCK_START; s++ )
   {
      int startIdx = coppockStartGrid[s];

      for( g = 0; g < NB_COPPOCK_GRID; g++ )
      {
         int w = coppockGrid[g].w, p1 = coppockGrid[g].p1, p2 = coppockGrid[g].p2;

         rc = TA_COPPOCK( startIdx, nbBars - 1, history->close,
                          w, p1, p2, &beg, &nb, out );
         if( rc != TA_SUCCESS )
         {
            printf( "COPPOCK differential Fail [start %d w %d p1 %d p2 %d]: retCode %d\n",
                    startIdx, w, p1, p2, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         lookback = TA_COPPOCK_Lookback( w, p1, p2 );
         refStart = (startIdx < lookback) ? lookback : startIdx;
         if( refStart > nbBars - 1 )
         {
            if( nb != 0 )
            {
               printf( "COPPOCK differential Fail [start %d w %d p1 %d p2 %d]: "
                       "expected no output, got nb=%d\n", startIdx, w, p1, p2, (int)nb );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            continue;
         }

         if( (int)beg != refStart || (int)nb != nbBars - refStart )
         {
            printf( "COPPOCK differential Fail [start %d w %d p1 %d p2 %d]: "
                    "range (%d,%d), expected (%d,%d)\n",
                    startIdx, w, p1, p2, (int)beg, (int)nb, refStart, nbBars - refStart );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         e = coppock_build_reference( history, startIdx, w, p1, p2, ref, &refBeg, &refNb );
         if( e != TA_TEST_PASS )
         {
            printf( "COPPOCK differential Fail [start %d w %d p1 %d p2 %d]: "
                    "reference construction failed\n", startIdx, w, p1, p2 );
            return e;
         }
         if( refBeg != (int)beg || refNb != (int)nb )
         {
            printf( "COPPOCK differential Fail [start %d w %d p1 %d p2 %d]: "
                    "reference range (%d,%d) vs (%d,%d)\n",
                    startIdx, w, p1, p2, refBeg, refNb, (int)beg, (int)nb );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         /* Bit-exact, element by element (memcmp semantics with a usable
          * diagnostic on the first divergence). */
         for( i = 0; i < (int)nb; i++ )
         {
            if( memcmp( &out[i], &ref[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "COPPOCK differential Fail [start %d w %d p1 %d p2 %d] bar %d: "
                       "fused %.17g != composed %.17g\n",
                       startIdx, w, p1, p2, (int)beg + i, out[i], ref[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            nbChecked++;
         }
      }
   }

   if( nbChecked < 5000 )
   {
      printf( "COPPOCK differential Fail: only %d value(s) compared; the grid has "
              "been reduced to the point where this leg is no longer evidence\n", nbChecked );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (2) EXTERNAL PINS: the formula leg. See the table's comment for provenance
 * and for why the tolerance is 1e-11 and must not be loosened. */
static ErrorNumber test_coppock_pins( const TA_History *history )
{
   unsigned int p;
   TA_RetCode rc;
   TA_Integer beg, nb;
   double err;
   const char *mode;
   static TA_Real out[SMI_CAP];

   rc = TA_COPPOCK( 0, (int)history->nbBars - 1, history->close,
                    10, 11, 14, &beg, &nb, out );
   if( rc != TA_SUCCESS )
   {
      printf( "COPPOCK pins Fail: retCode %d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != 23 || nb != 229 )
   {
      printf( "COPPOCK pins Fail: range (%d,%d), the pandas capture had (23,229)\n",
              (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( p = 0; p < NB_COPPOCK_PINS; p++ )
   {
      int idx = coppockPins[p].bar - (int)beg;
      if( !checkOracleValue( out[idx], coppockPins[p].v,
                             COPPOCK_PIN_REL, COPPOCK_PIN_ABS, &err, &mode ) )
      {
         printf( "COPPOCK pins Fail bar %d: got %.17g expected %.17g "
                 "(%s=%.3e > rel %.3e / abs %.3e)\n",
                 coppockPins[p].bar, out[idx], coppockPins[p].v, mode, err,
                 COPPOCK_PIN_REL, COPPOCK_PIN_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (3) FLAT INPUT => exactly 0.0 everywhere (every ROC is 0), and a ZERO in
 * the input => every output finite AND equal to the composed reference.
 *
 * The flat half proves nothing about the guard: a flat series never divides by
 * zero, so it takes the non-guard arm and its exact zeros are an accident of
 * the data. Finiteness alone does not pin the guard either -- any finite
 * neutral passes it. What pins it is the memcmp below against
 * TA_ROC + TA_ROC -> TA_WMA, which carries TA_ROC's own guard: change
 * COPPOCK's neutral away from 0.0 and that comparison is what fails. */
static ErrorNumber test_coppock_flat_and_zero_guard( void )
{
   static TA_Real in[64], out[64];
   TA_RetCode rc;
   TA_Integer beg, nb;
   int i, sawNonZero;

   for( i = 0; i < 64; i++ )
      in[i] = 100.0;
   rc = TA_COPPOCK( 0, 63, in, 10, 11, 14, &beg, &nb, out );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "COPPOCK flat Fail: retCode %d nb %d\n", (int)rc, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < (int)nb; i++ )
   {
      if( out[i] != 0.0 )
      {
         printf( "COPPOCK flat Fail bar %d: %.17g != 0.0 exactly\n", (int)beg + i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* One zero mid-series: both ROC denominators cross it. */
   for( i = 0; i < 64; i++ )
      in[i] = 100.0 + (double)(i % 7);
   in[30] = 0.0;
   rc = TA_COPPOCK( 0, 63, in, 10, 11, 14, &beg, &nb, out );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "COPPOCK zero-guard Fail: retCode %d nb %d\n", (int)rc, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   sawNonZero = 0;
   for( i = 0; i < (int)nb; i++ )
   {
      if( !(out[i] > -1e15 && out[i] < 1e15) )   /* catches inf and NaN */
      {
         printf( "COPPOCK zero-guard Fail bar %d: %.17g is not finite\n",
                 (int)beg + i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( out[i] != 0.0 )
         sawNonZero = 1;
   }
   {
      /* TA_ROC's guard is the arbiter of what a zero denominator contributes. */
      static TA_Real gr1[64], gr2[64], gsum[64], gwma[64];
      TA_Integer gb1, gn1, gb2, gn2, gbW, gnW;
      int gi, gn, gmaxP, glb, gsWma;

      gmaxP = 14;
      glb   = TA_COPPOCK_Lookback( 10, 11, 14 );
      if( TA_ROC( 0, 63, in, 11, &gb1, &gn1, gr1 ) != TA_SUCCESS ||
          TA_ROC( 0, 63, in, 14, &gb2, &gn2, gr2 ) != TA_SUCCESS )
      {
         printf( "COPPOCK zero-guard Fail: reference TA_ROC failed\n" );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      gn = 64 - gmaxP;
      for( gi = 0; gi < gn; gi++ )
         gsum[gi] = gr1[gmaxP + gi - (int)gb1] + gr2[gmaxP + gi - (int)gb2];
      gsWma = glb - gmaxP;
      if( TA_WMA( gsWma, gn - 1, gsum, 10, &gbW, &gnW, gwma ) != TA_SUCCESS )
      {
         printf( "COPPOCK zero-guard Fail: reference TA_WMA failed\n" );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( (int)nb != (int)gnW )
      {
         printf( "COPPOCK zero-guard Fail: nb %d != reference %d\n",
                 (int)nb, (int)gnW );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( gi = 0; gi < (int)gnW; gi++ )
      {
         if( memcmp( &out[gi], &gwma[gi], sizeof(TA_Real) ) != 0 )
         {
            printf( "COPPOCK zero-guard Fail bar %d: %.17g != composed %.17g "
                    "(the ROC zero-guard neutral is not TA_ROC's)\n",
                    (int)beg + gi, out[gi], gwma[gi] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   if( !sawNonZero )
   {
      printf( "COPPOCK zero-guard Fail: every output is 0.0 -- the leg is vacuous\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (4) IN-PLACE: outReal == inReal. The fused loop's lowest read at iteration
 * k is index k and its write is out[k], read before write -- assert it
 * instead of trusting the comment. */
static ErrorNumber test_coppock_inplace( const TA_History *history )
{
   static TA_Real buf[SMI_CAP], out[SMI_CAP];
   TA_RetCode rc;
   TA_Integer beg1, nb1, beg2, nb2;
   int i, nbBars = (int)history->nbBars;

   for( i = 0; i < nbBars; i++ )
      buf[i] = history->close[i];

   rc = TA_COPPOCK( 0, nbBars - 1, history->close, 10, 11, 14, &beg1, &nb1, out );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   rc = TA_COPPOCK( 0, nbBars - 1, buf, 10, 11, 14, &beg2, &nb2, buf );
   if( rc != TA_SUCCESS )
   {
      printf( "COPPOCK in-place Fail: retCode %d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg1 != beg2 || nb1 != nb2 )
   {
      printf( "COPPOCK in-place Fail: range (%d,%d) vs (%d,%d)\n",
              (int)beg2, (int)nb2, (int)beg1, (int)nb1 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < (int)nb1; i++ )
   {
      if( memcmp( &buf[i], &out[i], sizeof(TA_Real) ) != 0 )
      {
         printf( "COPPOCK in-place Fail bar %d: aliased %.17g != separate %.17g\n",
                 (int)beg1 + i, buf[i], out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}
