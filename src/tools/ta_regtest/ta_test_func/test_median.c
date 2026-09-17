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
 *  091526 KL     First version (proposal-drafts issue #72).
 *
 */


/* Description:
 *     Test TA_MEDIAN, the rolling median.
 *
 *     THE STRONGEST LEG IS IN-TREE AND FREE. At odd optInTimePeriod the median
 *     IS the nearest-rank 50th percentile, so this must be bitwise equal to the
 *     shipped TA_PERCENTILE(x, n, 50) -- same order statistic out of the same
 *     sorted window, so any difference is a bug and the assertion is memcmp
 *     rather than a tolerance.
 *
 *     That leg alone is satisfied by a body that simply forwards to
 *     TA_PERCENTILE, which is the thing this function exists NOT to be. Leg 2
 *     is what carries that: at even n the two must DIFFER, on data whose two
 *     central values differ.
 *
 *     WHAT EACH LEG CAN SEE, measured by mutating the generator input and
 *     regenerating rather than assumed:
 *
 *       even-n average replaced by the lower central value
 *         (i.e. the body degenerates into TA_PERCENTILE)
 *                          -> leg 2 RED (0 of 7600 differ), leg 4 RED,
 *                             leg 1 GREEN -- which is exactly why leg 2 exists
 *
 *     One mutation deliberately did NOT go red, and the reason is recorded at
 *     the site: flipping the placement scan's `<=` to `<` moves a new value to
 *     the front of a run of equal values instead of the back, and changes no
 *     output at all. MEASURED over 14820 windows on both a 7-distinct-value
 *     series and a random walk. Equal members are interchangeable; that is the
 *     premise the removal relies on, not an accident.
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
#define MEDIAN_CAP 1100

/* The legs below diff their own corpora against the language servers
 * bit-for-bit (issue #427). Without it every vector here is checked against
 * in-process C alone: the odd/even split, the frozen goldens over the
 * cancellation corpus and the invented-value leg are what pin the even-window
 * averaging rule, and a port that took the lower of the two central members
 * instead of their mean would pass the odd legs untouched while disagreeing
 * with C on every even window.
 *
 * MEASURED that this compares rather than merely runs -- see the commit that
 * added it: handing the servers a different period than C used fails with
 * "SV FAIL [MEDIAN] ... BITWISE mismatch vs in-process C".
 */
#define MEDIAN_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, inArr, period, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_comparisons();                               \
         ErrorNumber svErr_ = server_verify(                                     \
            "MEDIAN", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),               \
            (const TA_Real*[]){ (inArr), NULL },                                 \
            (double[]){ (double)(period) }, 1,                                   \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         /* "Returned PASS" and "compared nothing" are otherwise the same        \
          * observation. Every site below is a success case.  */                 \
         if( server_verify_comparisons() == svCmp_ )                             \
         {                                                                       \
            printf( "MEDIAN oracle [period %d]: server_verify compared no "      \
                    "server despite live pipes\n", (int)(period) );              \
            return TA_MEDIAN_VACUOUS;                                            \
         }                                                                       \
      }                                                                          \
   } while(0)

/* ==== generated by scratchpad/median/mkgolden.py (numpy 2.2.6) ==== */
/* medianCorpus: 40 bars, chosen for duplicate runs, one spike, a flat tail */
static const double medianCorpus[] = {
   100.0, 100.25, 100.25, 100.125,
   99.75, 99.5, 99.875, 100.375,
   101.0, 101.5, 101.5, 101.5,
   100.5, 100.25, 100.0, 99.5,
   99.0, 98.75, 99.25, 100.0,
   112.0, 100.5, 100.25, 100.0,
   99.875, 99.75, 99.625, 99.5,
   99.5, 99.5, 99.5, 99.5,
   99.5, 99.75, 100.25, 101.0,
   102.0, 103.5, 105.5, 108.0,
};
/* period 7 (odd): 34 values from numpy.median */
static const double medianGolden_7[] = {
   100.0, 100.125, 100.125,
   100.125, 100.375, 101.0,
   101.0, 101.0, 101.0,
   100.5, 100.25, 100.0,
   99.5, 99.5, 99.5,
   99.5, 100.0, 100.0,
   100.0, 100.0, 100.0,
   99.875, 99.75, 99.625,
   99.5, 99.5, 99.5,
   99.5, 99.5, 99.5,
   99.75, 100.25, 101.0,
   102.0,
};
/* period 8 (even): 33 values from numpy.median */
static const double medianGolden_8[] = {
   100.0625, 100.1875, 100.1875,
   100.25, 100.6875, 100.75,
   100.75, 100.75, 100.75,
   100.375, 100.125, 99.75,
   99.75, 99.75, 99.75,
   99.75, 100.0, 100.0,
   100.0, 100.0, 99.9375,
   99.8125, 99.6875, 99.5625,
   99.5, 99.5, 99.5,
   99.5, 99.5, 99.625,
   100.0, 100.625, 101.5,
};
/* period 20 (even): 21 values from numpy.median */
static const double medianGolden_20[] = {
   100.0625, 100.1875, 100.1875,
   100.1875, 100.125, 100.125,
   100.125, 100.125, 100.0,
   100.0, 99.9375, 99.8125,
   99.6875, 99.5625, 99.5625,
   99.5625, 99.6875, 99.75,
   99.8125, 99.9375, 99.9375,
};

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_medOddCmp;
static int g_medEvenCmp;
static int g_medEvenDiff;
static int g_medGoldenCmp;
static int g_medInventedCmp;
static int g_medExactCmp;
static int g_medAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_median_odd_identity( const TA_History *history );
static ErrorNumber test_median_even_differs( const TA_History *history );
static ErrorNumber test_median_golden( void );
static ErrorNumber test_median_invents_a_level( void );
static ErrorNumber test_median_exact_shapes( void );
static ErrorNumber test_median_aliasing( const TA_History *history );
static ErrorNumber test_median_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_median( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_medOddCmp = g_medEvenCmp = g_medEvenDiff = g_medGoldenCmp = 0;
   g_medInventedCmp = g_medExactCmp = g_medAliasCmp = 0;

   err = test_median_odd_identity( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_median_even_differs( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_median_golden();
   if( err != TA_TEST_PASS )
      return err;

   err = test_median_invents_a_level();
   if( err != TA_TEST_PASS )
      return err;

   err = test_median_exact_shapes();
   if( err != TA_TEST_PASS )
      return err;

   err = test_median_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_median_range( history );
   if( err != TA_TEST_PASS )
      return err;

   /* The golden and shape legs are corpus-independent, so those counts are
    * literal. The identity legs walk the caller's history, so they are floors.
    */
   if( g_medGoldenCmp != 88 || g_medInventedCmp == 0 || g_medExactCmp == 0
       || g_medOddCmp == 0 || g_medEvenCmp == 0 || g_medEvenDiff == 0
       || g_medAliasCmp == 0 )
   {
      printf( "MEDIAN Fail: coverage counters (odd %d, even %d with %d "
              "differing, golden %d, invented %d, exact %d, alias %d) are not "
              "what this file was written with (>0, >0, >0, 88, >0, >0, >0)\n",
              g_medOddCmp, g_medEvenCmp, g_medEvenDiff, g_medGoldenCmp,
              g_medInventedCmp, g_medExactCmp, g_medAliasCmp );
      return TA_MEDIAN_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) Odd period: bitwise identical to the shipped TA_PERCENTILE at P = 50.
 *
 * Both read the same order statistic out of the same sorted window -- the
 * nearest rank ceil(50*n/100) at odd n IS the central ordinal -- so this is
 * asserted with memcmp rather than a tolerance. No oracle server, no capture.
 */
static ErrorNumber test_median_odd_identity( const TA_History *history )
{
   static TA_Real med[MEDIAN_CAP], pct[MEDIAN_CAP];
   TA_Integer begM, nbM, begP, nbP;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > MEDIAN_CAP )
      nbBars = MEDIAN_CAP;

   for( n = 3; n <= 41; n += 2 )
   {
      retCode = TA_MEDIAN( 0, nbBars-1, history->close, n, &begM, &nbM, med );
      if( retCode != TA_SUCCESS )
      {
         printf( "MEDIAN odd Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      MEDIAN_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begM, nbM,
                            history->close, n, med );
      retCode = TA_PERCENTILE( 0, nbBars-1, history->close, n, 50.0,
                               &begP, &nbP, pct );
      if( retCode != TA_SUCCESS || begP != begM || nbP != nbM )
      {
         printf( "MEDIAN odd Fail [N=%d]: PERCENTILE rc=%d (%d,%d) vs (%d,%d)\n",
                 n, (int)retCode, begP, nbP, begM, nbM );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbM; i++ )
      {
         g_medOddCmp++;
         if( memcmp( &med[i], &pct[i], sizeof(double) ) != 0 )
         {
            printf( "MEDIAN odd Fail [N=%d] bar %d: %.17g against "
                    "TA_PERCENTILE's %.17g -- at odd n these are the same "
                    "order statistic and must be the same double\n",
                    n, begM+i, med[i], pct[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) Even period: must NOT match TA_PERCENTILE at P = 50.
 *
 * THE NON-VACUITY LEG FOR (1), and the only one that proves this function has
 * a reason to exist. ceil(50*n/100) at even n is n/2, the LOWER of the two
 * central values; the median is their mean. A body that forwarded to
 * TA_PERCENTILE would pass leg 1 and fail here. MEASURED with exactly that
 * mutation: 0 of 7600 windows differed, against 7322 for the real body.
 */
static ErrorNumber test_median_even_differs( const TA_History *history )
{
   static TA_Real med[MEDIAN_CAP], pct[MEDIAN_CAP];
   TA_Integer begM, nbM, begP, nbP;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > MEDIAN_CAP )
      nbBars = MEDIAN_CAP;

   for( n = 2; n <= 40; n += 2 )
   {
      retCode = TA_MEDIAN( 0, nbBars-1, history->close, n, &begM, &nbM, med );
      if( retCode != TA_SUCCESS )
      {
         printf( "MEDIAN even Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      MEDIAN_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begM, nbM,
                            history->close, n, med );
      retCode = TA_PERCENTILE( 0, nbBars-1, history->close, n, 50.0,
                               &begP, &nbP, pct );
      if( retCode != TA_SUCCESS || begP != begM || nbP != nbM )
      {
         printf( "MEDIAN even Fail [N=%d]: PERCENTILE rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbM; i++ )
      {
         g_medEvenCmp++;
         if( memcmp( &med[i], &pct[i], sizeof(double) ) != 0 )
            g_medEvenDiff++;
      }
   }

   if( g_medEvenDiff == 0 )
   {
      printf( "MEDIAN even Fail: %d windows and not one differs from "
              "TA_PERCENTILE(50). Either this body forwards to it, or the "
              "corpus has no window whose two central values differ -- in "
              "which case leg 1 is proving nothing either\n", g_medEvenCmp );
      return TA_MEDIAN_VACUOUS;
   }

   return TA_TEST_PASS;
}

/* (3) Frozen numpy goldens at both parities. The definition has no variant to
 * pick -- NumPy, R, scipy and Excel agree -- so these are asserted bitwise.
 */
static ErrorNumber med_vs_golden( int period, const double *golden, int nbGolden )
{
   static TA_Real out[MEDIAN_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   retCode = TA_MEDIAN( 0, 39, medianCorpus, period, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != period-1 || nbElement != nbGolden )
   {
      printf( "MEDIAN golden[p=%d] Fail: rc=%d (%d,%d), expected (%d,%d)\n",
              period, (int)retCode, begIdx, nbElement, period-1, nbGolden );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   MEDIAN_SERVER_VERIFY( 0, 39, 40, retCode, begIdx, nbElement,
                         medianCorpus, period, out );

   for( i = 0; i < nbGolden; i++ )
   {
      g_medGoldenCmp++;
      if( memcmp( &out[i], &golden[i], sizeof(double) ) != 0 )
      {
         printf( "MEDIAN golden[p=%d] Fail at bar %d: %.17g against numpy's "
                 "%.17g\n", period, begIdx+i, out[i], golden[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_median_golden( void )
{
   ErrorNumber err;

   err = med_vs_golden( 7, medianGolden_7, 34 );
   if( err != TA_TEST_PASS )
      return err;

   err = med_vs_golden( 8, medianGolden_8, 33 );
   if( err != TA_TEST_PASS )
      return err;

   return med_vs_golden( 20, medianGolden_20, 21 );
}

/* (4) At even period the output can be a level the series never traded at.
 *
 * The documented consequence of averaging the two central values, and the
 * property that separates this function from TA_PERCENTILE, whose whole design
 * point is that it never invents one. Asserted rather than described: the
 * corpus above has 40 distinct-valued bars and the 8-bar median lands on
 * 100.0625 and 100.1875, neither of which appears in the input.
 */
static ErrorNumber test_median_invents_a_level( void )
{
   static TA_Real out[MEDIAN_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i, j, found;

   retCode = TA_MEDIAN( 0, 39, medianCorpus, 8, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "MEDIAN invented Fail: rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   MEDIAN_SERVER_VERIFY( 0, 39, 40, retCode, begIdx, nbElement,
                         medianCorpus, 8, out );

   for( i = 0; i < nbElement; i++ )
   {
      found = 0;
      for( j = 0; j < 40; j++ )
      {
         if( memcmp( &out[i], &medianCorpus[j], sizeof(double) ) == 0 )
         {
            found = 1;
            break;
         }
      }
      if( !found )
         g_medInventedCmp++;
   }

   if( g_medInventedCmp == 0 )
   {
      printf( "MEDIAN invented Fail: every 8-bar median is a value that "
              "occurs in the input. At even n the mean of the two central "
              "values should reach levels the series never traded at -- if "
              "none does, this is reporting an order statistic\n" );
      return TA_MEDIAN_VACUOUS;
   }

   return TA_TEST_PASS;
}

/* (5) Shapes with an exact answer, both parities, no oracle.
 *
 * A strictly monotone window: its median is the mean of its two ends, and with
 * a spacing of 2 that mean is exact in binary.
 * A flat window: the value itself, exactly, at both parities -- the even case
 * exercises (v + v) / 2 and must come back to v.
 */
static ErrorNumber test_median_exact_shapes( void )
{
   static TA_Real in[400], out[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int n, i;
   double want;

   for( i = 0; i < 400; i++ )
      in[i] = 2.0*(double)i + 5.0;

   for( n = 2; n <= 40; n++ )
   {
      retCode = TA_MEDIAN( 0, 399, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "MEDIAN monotone Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      MEDIAN_SERVER_VERIFY( 0, 399, 400, retCode, begIdx, nbElement, in, n, out );
      for( i = 0; i < nbElement; i++ )
      {
         want = ( in[begIdx+i-n+1] + in[begIdx+i] ) / 2.0;
         g_medExactCmp++;
         if( memcmp( &out[i], &want, sizeof(double) ) != 0 )
         {
            printf( "MEDIAN monotone Fail [N=%d] bar %d: %.17g, expected "
                    "%.17g\n", n, begIdx+i, out[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   for( i = 0; i < 400; i++ )
      in[i] = 7.25;

   for( n = 2; n <= 40; n++ )
   {
      retCode = TA_MEDIAN( 0, 399, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "MEDIAN flat Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      MEDIAN_SERVER_VERIFY( 0, 399, 400, retCode, begIdx, nbElement, in, n, out );
      for( i = 0; i < nbElement; i++ )
      {
         g_medExactCmp++;
         if( out[i] != 7.25 )
         {
            printf( "MEDIAN flat Fail [N=%d] bar %d: %.17g, expected exactly "
                    "7.25\n", n, begIdx+i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) In-place aliasing, outReal == inReal, bitwise. The removal path reads the
 * trailing value out of the ring rather than out of inReal, which is what makes
 * this safe; the test is what holds that true.
 */
static ErrorNumber test_median_aliasing( const TA_History *history )
{
   static TA_Real clean[MEDIAN_CAP], alias[MEDIAN_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > MEDIAN_CAP )
      nbBars = MEDIAN_CAP;

   for( n = 2; n <= 40; n += 3 )
   {
      retCode = TA_MEDIAN( 0, nbBars-1, history->close, n,
                           &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "MEDIAN alias Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      /* The non-aliased call only: in-place behaviour is a C-side memory
       * property, not something the servers are asked to reproduce. */
      MEDIAN_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                            history->close, n, clean );

      for( i = 0; i < nbBars; i++ )
         alias[i] = history->close[i];
      retCode = TA_MEDIAN( 0, nbBars-1, alias, n,
                           &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "MEDIAN alias Fail [N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 n, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_medAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "MEDIAN alias Fail [N=%d] bar %d: separate %.17g, "
                    "in-place %.17g\n", n, begIdx+i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) The startIdx/endIdx range sweep. TA_STABLE_EXACT: the window is rebuilt
 * from the input on every call and carries no accumulator, so the same bar
 * reached from two different starts is the same double, not merely close.
 */
typedef struct { int period; const TA_Real *in; } MedianRangeParam;

static TA_RetCode medianRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                           TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                           TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                           TA_Integer *lookback, void *opaqueData,
                                           unsigned int outputNb, unsigned int *isOutputInteger )
{
   MedianRangeParam *p = (MedianRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_MEDIAN_Lookback( p->period );
   return TA_MEDIAN( startIdx, endIdx, p->in, p->period,
                     outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_median_range( const TA_History *history )
{
   MedianRangeParam param;

   param.period = 30;
   param.in     = history->close;

   return doRangeTestEx( medianRangeTestFunction,
                         TA_STABLE_EXACT, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
