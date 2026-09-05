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
 *  090626 KL     First version (issue #361).
 */

/* Description:
 *
 *   Hand-written tests for TA_ERI (Elder Ray Index).
 *
 *   (1) DIFFERENTIAL, bit-exact and non-tautological: TA_EMA(close, n) plus
 *       two subtractions, memcmp'd over a period x startIdx x EMA-unstable-
 *       period grid. The fused loop reproduces ema.c's DEFAULT arm op for op
 *       (seed sum order, warm-up consuming the unstable period), so any
 *       transcription slip fails deterministically.
 *
 *   (2) GOLDEN PINS at the defaults, ABSOLUTE tolerance 1e-12, rows pinned
 *       away from zero crossings. Provenance: a from-scratch transcription of
 *       ema.c's DEFAULT arm on the committed TA_SREF corpus (the NVI/PVI
 *       pattern) -- the issue measured pandas-ta-classic 0.6.52 against the
 *       same transcription at max abs 7.1e-14 on this corpus, so these rows
 *       hold against either. ABSOLUTE, not relative: ERI is a cancelling
 *       difference (high - EMA), and relative error is amplified by
 *       |EMA|/|out| without bound at the zero crossings that carry the
 *       signal (measured 1.9e-10 rel at an out-value of -1.5e-4). Do not
 *       loosen to rel-1e-6; do not tighten to bitwise.
 *
 *   (3) EDGES: all-flat input (both lines exactly 0.0, no NaN); high == low
 *       on every bar (the two lines bitwise identical); n=1 (EMA is the
 *       identity on close, so bull == high - close exactly) on TA_SREF and
 *       again on a series Sterbenz does not cover -- only the second can see
 *       a period-1 EMA left as the bare recursion.
 *
 *   (4) IN-PLACE ALIASING: two outputs over three inputs is the widest
 *       aliasing surface of any recent addition. Each output aliased onto
 *       each input, bit-compared against the separate-buffer call, at n=1 as
 *       well as n=13: the lookback at n=13 holds every store 12 slots behind
 *       its load, so the combinations only bite at the minimum period. #108
 *       already rejects outBullPower == outBearPower.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

#define ERI_CAP 300

static const int eriPeriodGrid[] = { 1, 2, 13, 30, 100 };
#define NB_ERI_PERIOD (sizeof(eriPeriodGrid)/sizeof(eriPeriodGrid[0]))
static const int eriStartGrid[] = { 0, 1, 12, 13, 100, 251 };
#define NB_ERI_START (sizeof(eriStartGrid)/sizeof(eriStartGrid[0]))
static const int eriUnstGrid[] = { 0, 1, 3, 7 };
#define NB_ERI_UNST (sizeof(eriUnstGrid)/sizeof(eriUnstGrid[0]))
/* The aliasing sweep needs the minimum period: at n=13 the lookback puts
 * every store 12 slots behind its load, so no combination can bite. */
static const int eriAliasGrid[] = { 1, 13 };
#define NB_ERI_ALIAS (sizeof(eriAliasGrid)/sizeof(eriAliasGrid[0]))

/* Two closes alternating at a ratio near 8.9, both spending a full mantissa.
 * Sterbenz does not cover them: 32 of the 63 steps of fl(fl(x-prev)+prev)
 * land off the close, which is what makes the n=1 leg below able to fail. */
#define ERI_NS_HI 651.28353856681395
#define ERI_NS_LO 73.36385038087522

/* Golden pins, n=13, unstable 0, outBegIdx=12, outNBElement=240 on TA_SREF.
 * See the header for provenance and for why the tolerance is ABSOLUTE. */
static const struct { int bar; double bull; double bear; } eriPins[] =
{
   {  12, 4.9611538461538487, 1.8661538461538498 },
   {  63, 4.7412968545015843, 2.4612968545015832 },
   { 110, 1.1794771477264163, -2.7605228522735814 },
   { 155, 4.1215715236162112, 0.7515715236162066 },
   { 203, -11.790083766069827, -13.910083766069832 },
   { 251, 0.53947989919237216, -2.3405201008076233 },
};
#define NB_ERI_PINS (sizeof(eriPins)/sizeof(eriPins[0]))
#define ERI_PIN_ABS 1e-12

ErrorNumber test_func_eri( TA_History *history )
{
   static TA_Real outBull[ERI_CAP], outBear[ERI_CAP];
   static TA_Real refBull[ERI_CAP], refBear[ERI_CAP], ema[ERI_CAP];
   static TA_Real aH[ERI_CAP], aL[ERI_CAP], aC[ERI_CAP], aOther[ERI_CAP];
   TA_RetCode rc;
   TA_Integer beg, nb, begE, nbE;
   unsigned int p, s, u, pin;
   int i, o, in, n, startIdx, nbBars, nbChecked = 0;

   nbBars = (int)history->nbBars;

   /* (1) Differential over the grid, bit-exact. */
   for( u = 0; u < NB_ERI_UNST; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, (TA_Integer)eriUnstGrid[u] );
      for( s = 0; s < NB_ERI_START; s++ )
      {
         startIdx = eriStartGrid[s];
         for( p = 0; p < NB_ERI_PERIOD; p++ )
         {
            n = eriPeriodGrid[p];

            rc = TA_ERI( startIdx, nbBars - 1, history->high, history->low,
                         history->close, n, &beg, &nb, outBull, outBear );
            if( rc != TA_SUCCESS )
            {
               printf( "ERI differential Fail [unst %d start %d n %d]: retCode %d\n",
                       eriUnstGrid[u], startIdx, n, (int)rc );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
            rc = TA_EMA( startIdx, nbBars - 1, history->close, n,
                         &begE, &nbE, ema );
            if( rc != TA_SUCCESS )
            {
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
            if( beg != begE || nb != nbE )
            {
               printf( "ERI differential Fail [unst %d start %d n %d]: range "
                       "(%d,%d) vs TA_EMA (%d,%d)\n", eriUnstGrid[u], startIdx,
                       n, (int)beg, (int)nb, (int)begE, (int)nbE );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            for( i = 0; i < (int)nb; i++ )
            {
               refBull[i] = history->high[(int)beg + i] - ema[i];
               refBear[i] = history->low[(int)beg + i] - ema[i];
               if( memcmp( &outBull[i], &refBull[i], sizeof(TA_Real) ) != 0 ||
                   memcmp( &outBear[i], &refBear[i], sizeof(TA_Real) ) != 0 )
               {
                  printf( "ERI differential Fail [unst %d start %d n %d] bar %d: "
                          "fused (%.17g,%.17g) != TA_EMA-composed (%.17g,%.17g)\n",
                          eriUnstGrid[u], startIdx, n, (int)beg + i,
                          outBull[i], outBear[i], refBull[i], refBear[i] );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               /* Free on every cell: bull >= bear, since high >= low. */
               if( outBull[i] < outBear[i] )
               {
                  printf( "ERI invariant Fail bar %d: bull %.17g < bear %.17g\n",
                          (int)beg + i, outBull[i], outBear[i] );
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
      printf( "ERI differential Fail: only %d value(s) compared\n", nbChecked );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (2) Golden pins, absolute tolerance. */
   rc = TA_ERI( 0, nbBars - 1, history->high, history->low, history->close,
                13, &beg, &nb, outBull, outBear );
   if( rc != TA_SUCCESS || beg != 12 || nb != 240 )
   {
      printf( "ERI pins Fail: range (%d,%d), expected (12,240)\n",
              (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( pin = 0; pin < NB_ERI_PINS; pin++ )
   {
      int idx = eriPins[pin].bar - (int)beg;
      if( fabs( outBull[idx] - eriPins[pin].bull ) > ERI_PIN_ABS ||
          fabs( outBear[idx] - eriPins[pin].bear ) > ERI_PIN_ABS )
      {
         printf( "ERI pins Fail bar %d: got (%.17g,%.17g) expected (%.17g,%.17g) "
                 "abs tol %.1e\n", eriPins[pin].bar, outBull[idx], outBear[idx],
                 eriPins[pin].bull, eriPins[pin].bear, ERI_PIN_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* (3) Edges. All-flat: both lines exactly 0.0. */
   for( i = 0; i < 64; i++ )
   {
      aH[i] = 100.0;
      aL[i] = 100.0;
      aC[i] = 100.0;
   }
   rc = TA_ERI( 0, 63, aH, aL, aC, 13, &beg, &nb, outBull, outBear );
   if( rc != TA_SUCCESS || nb <= 0 )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   for( i = 0; i < (int)nb; i++ )
   {
      if( outBull[i] != 0.0 || outBear[i] != 0.0 )
      {
         printf( "ERI flat Fail bar %d: (%.17g,%.17g) != exact 0.0\n",
                 (int)beg + i, outBull[i], outBear[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   /* high == low on every bar: the two lines bitwise identical. */
   for( i = 0; i < 64; i++ )
   {
      aH[i] = 100.0 + (double)( i % 9 );
      aL[i] = aH[i];
      aC[i] = aH[i] - 0.25;
   }
   rc = TA_ERI( 0, 63, aH, aL, aC, 13, &beg, &nb, outBull, outBear );
   if( rc != TA_SUCCESS ||
       memcmp( outBull, outBear, (size_t)nb * sizeof(TA_Real) ) != 0 )
   {
      printf( "ERI high==low Fail: the two lines are not bitwise identical\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   /* n=1: the EMA is the identity on close, so bull == high - close and
    * bear == low - close, bit for bit. */
   rc = TA_ERI( 0, nbBars - 1, history->high, history->low, history->close,
                1, &beg, &nb, outBull, outBear );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   for( i = 0; i < (int)nb; i++ )
   {
      double eb = history->high[(int)beg + i] - history->close[(int)beg + i];
      double es = history->low[(int)beg + i] - history->close[(int)beg + i];
      if( memcmp( &outBull[i], &eb, sizeof(TA_Real) ) != 0 ||
          memcmp( &outBear[i], &es, sizeof(TA_Real) ) != 0 )
      {
         printf( "ERI n=1 Fail bar %d: (%.17g,%.17g) != (high-close, low-close) "
                 "(%.17g,%.17g)\n", (int)beg + i, outBull[i], outBear[i], eb, es );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   /* n=1 again, off the Sterbenz-benign corpus. TA_SREF keeps consecutive
    * closes within a factor of two, where fl(fl(x-prev)+prev) returns x on
    * every bar -- so the leg above stays green against a period-1 EMA written
    * as the bare recursion, which is not the identity. This series is what
    * separates the two. */
   for( i = 0; i < 64; i++ )
   {
      aC[i] = ( i % 2 == 0 ) ? ERI_NS_HI : ERI_NS_LO;
      aH[i] = aC[i] + 1.5;
      aL[i] = aC[i] - 1.5;
   }
   rc = TA_ERI( 0, 63, aH, aL, aC, 1, &beg, &nb, outBull, outBear );
   if( rc != TA_SUCCESS || nb != 64 )
   {
      printf( "ERI n=1 non-Sterbenz Fail: range (%d,%d), expected (0,64)\n",
              (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < (int)nb; i++ )
   {
      double eb = aH[(int)beg + i] - aC[(int)beg + i];
      double es = aL[(int)beg + i] - aC[(int)beg + i];
      if( memcmp( &outBull[i], &eb, sizeof(TA_Real) ) != 0 ||
          memcmp( &outBear[i], &es, sizeof(TA_Real) ) != 0 )
      {
         printf( "ERI n=1 non-Sterbenz Fail bar %d: (%.17g,%.17g) != "
                 "(high-close, low-close) (%.17g,%.17g)\n",
                 (int)beg + i, outBull[i], outBear[i], eb, es );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* (4) In-place aliasing: each output onto each input, at the minimum
    * period as well as the default -- n=1 is the only one where a store and
    * its load index the same bar. */
   for( p = 0; p < NB_ERI_ALIAS; p++ )
   {
      n = eriAliasGrid[p];

      rc = TA_ERI( 0, nbBars - 1, history->high, history->low, history->close,
                   n, &beg, &nb, refBull, refBear );
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
            /* outs[0] is always the outBullPower POSITION, outs[1] outBearPower;
             * `o` only chooses which of the two got aliased onto an input. */
            rc = TA_ERI( 0, nbBars - 1, aH, aL, aC, n, &beg, &nb,
                         outs[0], outs[1] );
            if( rc != TA_SUCCESS ||
                memcmp( outs[0], refBull, (size_t)nb * sizeof(TA_Real) ) != 0 ||
                memcmp( outs[1], refBear, (size_t)nb * sizeof(TA_Real) ) != 0 )
            {
               printf( "ERI aliasing Fail [n %d out%d over in%d]: aliased call "
                       "differs from separate-buffer call\n", n, o, in );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}
