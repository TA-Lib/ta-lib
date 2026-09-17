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
 *     Test TA_KURTOSIS, sample-adjusted Fisher excess kurtosis (G2).
 *
 *     WHAT EACH LEG CAN SEE, measured by mutating the generator input and
 *     regenerating rather than assumed:
 *
 *       coefA denominator (n-1) -> n     leg 1 RED (144 combinations)
 *       residue correction dropped        leg 1 RED (128)
 *       rebuild period n/4 -> 32n         leg 1 RED (36)
 *       degenerate window returns 0       leg 4 RED (571 bars)
 *
 *     Leg 1 carries three separate defects because the progression identity
 *     constrains the two coefficients against each other AND the shift's
 *     staleness at once. That is also why it is worth more than a golden: a
 *     golden only says "not what it was", this says "not the estimator".
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
#define KURT_CAP 1100

/* The legs below diff their own corpora against the language servers
 * bit-for-bit (issue #427). Without it every vector in this file is checked
 * against in-process C alone -- the arithmetic-progression identity, the
 * 60-digit goldens and the near-degenerate set are written specifically to
 * break this function, and a port that used the integer coefficient forms
 * (which overflow) or a different reseed period would satisfy every assertion
 * here while disagreeing with C.
 *
 * MEASURED that this compares rather than merely runs -- see the commit that
 * added it: handing the servers a different period than C used fails with
 * "SV FAIL [KURTOSIS] ... BITWISE mismatch vs in-process C".
 *
 * The period<4 leg is deliberately NOT wrapped: it asserts TA_BAD_PARAM, and
 * server_verify() skips reject cases by design, so the floor below would fire
 * on a leg that is working exactly as intended.
 */
#define KURT_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, inArr, period, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_comparisons();                               \
         ErrorNumber svErr_ = server_verify(                                     \
            "KURTOSIS", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),             \
            (const TA_Real*[]){ (inArr), NULL },                                 \
            (double[]){ (double)(period) }, 1,                                   \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         /* "Returned PASS" and "compared nothing" are otherwise the same        \
          * observation. Every site below is a success case.  */                 \
         if( server_verify_comparisons() == svCmp_ )                             \
         {                                                                       \
            printf( "KURTOSIS oracle [period %d]: server_verify compared no "    \
                    "server despite live pipes\n", (int)(period) );              \
            return TA_KURTOSIS_VACUOUS;                                          \
         }                                                                       \
      }                                                                          \
   } while(0)

/* The exact-arithmetic identity leg 1 rests on.
 *
 * For n values in arithmetic progression the discrete-uniform population
 * kurtosis is g2 = -6(n^2+1)/(5(n^2-1)). Substituting it into
 * G2 = [(n-1)/((n-2)(n-3))] * [(n+1)g2 + 6] gives a numerator of
 * -6(n-2)(n-3) over a denominator of 5(n-2)(n-3): the n-dependence cancels
 * exactly and every progression, at every length and every spacing, has
 * excess kurtosis -6/5. Nothing external is consulted, and a wrong
 * coefficient moves the answer by O(1/n) -- far outside the tolerance.
 */
#define KURT_PROGRESSION (-1.2)

/* kurtBenign: 40 bars */
static const double kurtBenign[] = {
   100.0, 100.25, 100.5, 100.125,
   99.75, 99.5, 99.875, 100.375,
   101.0, 101.5, 101.25, 100.75,
   100.5, 100.25, 100.0, 99.5,
   99.0, 98.75, 99.25, 100.0,
   112.0, 100.5, 100.25, 100.0,
   99.875, 99.75, 99.625, 99.5,
   99.5, 99.5, 99.5, 99.5,
   99.5, 99.75, 100.25, 101.0,
   102.0, 103.5, 105.5, 108.0,
};
/* period 10: 31 values; scipy 1.15.3 differs from the 60-digit reference at bar 9 by 8.22e-14 relative */
static const double kurtBenign_g2_10[] = {
   0.6482205399083414, -0.8816072588500443, -1.107521252571008,
   -1.107521252571008, -1.0168714998943365, -0.7775047258979206,
   -0.7775047258979206, -0.22568255461919157, -1.0187074829931972,
   -1.2335393711367912, -0.8475573064056762, 9.256930592297806,
   9.341830620802492, 9.405005759562547, 9.446429672329058,
   9.459034555889948, 9.464346012906676, 9.56317970094897,
   9.760662166066723, 9.809810251124867, 9.788599860715935,
   0.027177349374474004, 0.4779993558054533, 0.2571428571428571,
   -0.3936640054357058, 5.241042805274473, 4.525870285221228,
   2.662284493719168, 1.9120786353315609, 1.2672326730673418,
   0.6783972260217559,
};
/* period 20: 21 values; scipy 1.15.3 differs from the 60-digit reference at bar 19 by 1.50e-14 relative */
static const double kurtBenign_g2_20[] = {
   -0.15125214370233347, 16.997367238867056, 16.956117509081388,
   16.997367238867056, 16.996536660159897, 17.014937702450045,
   17.079555019736922, 17.03231950337341, 16.975516979193134,
   17.19475491858422, 17.87812665102518, 18.457808808413514,
   18.746412883040506, 18.937243337549692, 19.044354102785295,
   18.97082202167872, 18.460266518744998, 17.234158037516167,
   14.45765249177646, 9.615968694353265, 4.676997147652225,
};

/* kurtNearDegenerate: 20 bars */
static const double kurtNearDegenerate[] = {
   1048576.0, 1048576.0000000002, 1048576.0, 1048576.0000000005,
   1048576.0000000002, 1048576.0000000007, 1048576.0, 1048576.0000000002,
   1048576.0000000005, 1048576.0, 1048576.0000000002, 1048576.0000000002,
   1048576.000000001, 1048576.0, 1048576.0000000005, 1048576.0000000002,
   1048576.0, 1048576.0000000007, 1048576.0000000002, 1048576.0000000005,
};
/* period 8: 13 values; scipy 1.15.3 differs from the 60-digit reference at bar 7 by 3.75e+00 relative */
static const double kurtNearDegenerate_g2_8[] = {
   0.35, -0.448, -0.9886927196984725,
   -0.448, 0.8404628099173553, -0.22857142857142856,
   2.5706475633895747, 1.6517673130193906, 1.6517673130193906,
   2.5706475633895747, -0.22857142857142856, -0.22857142857142856,
   -0.5644996347699051,
};

/* Tolerances, from measurement rather than habit. The worst relative deviation
 * of this implementation from the 60-digit reference is 1.14e-14 on the benign
 * corpus and 1.59e-15 on the near-degenerate one, so 1e-12 leaves roughly two
 * orders of headroom on both and still fails anything that changes the
 * estimator. For scale: scipy 1.15.3 misses the near-degenerate window by 375%
 * (1.6625 against 0.35) and warns while doing it, which is what makes that leg
 * worth having rather than a restatement of the benign one.
 */
#define KURT_GOLDEN_TOL 1e-12

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_kurtProgCmp;
static int g_kurtProgSkip;
static int g_kurtScaleCmp;
static int g_kurtGoldenCmp;
static int g_kurtNearCmp;
static int g_kurtNanCmp;
static int g_kurtAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_kurt_progression( void );
static ErrorNumber test_kurt_scale_invariance( const TA_History *history );
static ErrorNumber test_kurt_golden( void );
static ErrorNumber test_kurt_degenerate( void );
static ErrorNumber test_kurt_aliasing( const TA_History *history );
static ErrorNumber test_kurt_contract( const TA_History *history );
static ErrorNumber test_kurt_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_kurtosis( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_kurtProgCmp = g_kurtProgSkip = g_kurtScaleCmp = g_kurtGoldenCmp = 0;
   g_kurtNearCmp = g_kurtNanCmp = g_kurtAliasCmp = 0;

   err = test_kurt_progression();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_scale_invariance( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_golden();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_degenerate();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_contract( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_range( history );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: every leg above is deterministic and
    * independent of the corpus, so these hold on any history. */
   if( g_kurtProgCmp != 81936 || g_kurtProgSkip != 36
       || g_kurtGoldenCmp != 52 || g_kurtNearCmp != 13
       || g_kurtNanCmp != 571 || g_kurtScaleCmp == 0 || g_kurtAliasCmp == 0 )
   {
      printf( "KURTOSIS Fail: coverage counters (progression %d/%d skipped, "
              "scale %d, golden %d, near-degenerate %d, NaN %d, alias %d) are "
              "not what this file was written with (81936/36, >0, 52, 13, 571, "
              ">0)\n",
              g_kurtProgCmp, g_kurtProgSkip, g_kurtScaleCmp, g_kurtGoldenCmp,
              g_kurtNearCmp, g_kurtNanCmp, g_kurtAliasCmp );
      return TA_KURTOSIS_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) Every arithmetic progression has excess kurtosis exactly -6/5.
 *
 * THE PREMISE IS CHECKED PER SERIES. At base 3.1e10 the ulp is 3.8e-6, so a
 * requested spacing of 1e-4 lands on two different actual differences
 * (9.918e-05 and 1.030e-04) and the series is not a progression at all;
 * asserting -1.2 on it would be testing the spacing's representability. Such a
 * combination is skipped and counted rather than silently expected to hold.
 */
static ErrorNumber test_kurt_progression( void )
{
   static double in[600], out[600];
   static const double spacings[] = { 1.0, 0.25, -3.0, 1e-4, 7.5 };
   static const double bases[]    = { 0.0, 100.0, -50.0, 31498938283.0 };
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int s, b, n, i, uneven;
   double d0, di;

   for( s = 0; s < 5; s++ )
   for( b = 0; b < 4; b++ )
   for( n = 4; n <= 60; n += 7 )
   {
      for( i = 0; i < 600; i++ )
         in[i] = bases[b] + spacings[s] * (double)i;

      uneven = 0;
      d0 = in[1] - in[0];
      for( i = 2; i < 600; i++ )
      {
         di = in[i] - in[i-1];
         if( memcmp( &d0, &di, sizeof(double) ) != 0 ) { uneven = 1; break; }
      }
      if( uneven )
      {
         g_kurtProgSkip++;
         continue;
      }

      retCode = TA_KURTOSIS( 0, 599, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || nbElement != 600-(n-1) )
      {
         printf( "KURTOSIS progression Fail [base=%g spacing=%g N=%d]: rc=%d "
                 "(%d,%d)\n", bases[b], spacings[s], n, (int)retCode,
                 begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      KURT_SERVER_VERIFY( 0, 599, 600, retCode, begIdx, nbElement, in, n, out );

      for( i = 0; i < nbElement; i++ )
      {
         g_kurtProgCmp++;
         if( !(fabs(out[i] - KURT_PROGRESSION) <= 1.2e-9) )
         {
            printf( "KURTOSIS progression Fail [base=%g spacing=%g N=%d] bar %d: "
                    "%.17g, expected %.17g. Every progression has this value at "
                    "every n -- the n-dependence cancels -- so a miss is the "
                    "estimator, not the corpus\n",
                    bases[b], spacings[s], n, begIdx+i, out[i], KURT_PROGRESSION );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) Scaling by a power of two is BITWISE invisible.
 *
 * G2 is a standardised moment, so it is scale-free in exact arithmetic. In
 * floating point that only survives if the scaling is exact: a power of two
 * shifts the exponent and leaves every mantissa alone, so each deviation, each
 * running sum and the final quotient are the same numbers scaled by the same
 * power, and the k^4 in the numerator cancels the k^4 in the denominator
 * exactly. Any other multiplier rounds and the equality is approximate.
 *
 * THE EXPONENT RANGE IS LOAD-BEARING. This leg exists to catch an absolute
 * threshold anywhere in the body -- the #243 mistake, where a fixed epsilon is
 * a cliff at a price level rather than a noise floor. At 2^-8 a corpus around
 * 100 still has a second moment near 1e-1, so a 1e-14 floor is nowhere near it
 * and the leg passes over the defect. MEASURED: with such a floor inserted,
 * 2^-8 is green and 2^-40 is red. The span has to reach the floor to see it.
 */
static ErrorNumber test_kurt_scale_invariance( const TA_History *history )
{
   static double scaled[KURT_CAP], base[KURT_CAP], out[KURT_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i, e;
   double k;

   if( nbBars > KURT_CAP )
      nbBars = KURT_CAP;

   for( n = 4; n <= 40; n += 9 )
   {
      retCode = TA_KURTOSIS( 0, nbBars-1, history->close, n,
                             &begIdx, &nbElement, base );
      if( retCode != TA_SUCCESS )
      {
         printf( "KURTOSIS scale Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          history->close, n, base );

      for( e = -40; e <= 40; e += 20 )
      {
         k = ldexp( 1.0, e );
         for( i = 0; i < nbBars; i++ )
            scaled[i] = history->close[i] * k;

         retCode = TA_KURTOSIS( 0, nbBars-1, scaled, n,
                                &begIdx2, &nbElement2, out );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "KURTOSIS scale Fail [N=%d k=2^%d]: rc=%d (%d,%d) vs "
                    "(%d,%d)\n", n, e, (int)retCode, begIdx2, nbElement2,
                    begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_kurtScaleCmp++;
            if( memcmp( &base[i], &out[i], sizeof(double) ) != 0 )
            {
               printf( "KURTOSIS scale Fail [N=%d k=2^%d] bar %d: %.17g against "
                       "%.17g -- a power of two must be bitwise invisible to a "
                       "standardised moment\n",
                       n, e, begIdx+i, out[i], base[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) Frozen goldens, computed in 60-digit decimal over the exact binary value
 * of each input (never over its shortest round-trip decimal, which sits ~3e-15
 * away and is the size of what is being measured).
 */
static ErrorNumber kurt_vs_golden( const char *tag, const double *x, int nbBars,
                                   int period, const double *golden, int nbGolden,
                                   int *counter )
{
   static double out[KURT_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;
   double err;

   retCode = TA_KURTOSIS( 0, nbBars-1, x, period, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != nbGolden
       || begIdx != period-1 )
   {
      printf( "KURTOSIS golden[%s] Fail: rc=%d (%d,%d), expected (%d,%d)\n",
              tag, (int)retCode, begIdx, nbElement, period-1, nbGolden );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                       x, period, out );

   for( i = 0; i < nbGolden; i++ )
   {
      (*counter)++;
      err = fabs( out[i] - golden[i] );
      if( golden[i] != 0.0 )
         err /= fabs( golden[i] );
      if( !(err <= KURT_GOLDEN_TOL) )
      {
         printf( "KURTOSIS golden[%s] Fail at bar %d: %.17g against %.17g, "
                 "relative %.3e over %.3e\n",
                 tag, begIdx+i, out[i], golden[i], err, KURT_GOLDEN_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_kurt_golden( void )
{
   ErrorNumber err;

   err = kurt_vs_golden( "benign p=10", kurtBenign, 40, 10,
                         kurtBenign_g2_10, 31, &g_kurtGoldenCmp );
   if( err != TA_TEST_PASS )
      return err;

   err = kurt_vs_golden( "benign p=20", kurtBenign, 40, 20,
                         kurtBenign_g2_20, 21, &g_kurtGoldenCmp );
   if( err != TA_TEST_PASS )
      return err;

   /* The near-degenerate window: every value is base + k*ulp at base 2^20, so
    * the corpus is exact and the reference is not measuring its own error.
    * NON-VACUOUS, and by a wide margin -- scipy 1.15.3 answers 1.6624999999999996
    * on the first window where the reference says 0.34999999999999998, a 375%
    * miss, and raises RuntimeWarning: Precision loss occurred while doing it.
    * This implementation lands 1.59e-15 from the reference there.
    */
   return kurt_vs_golden( "near-degenerate p=8", kurtNearDegenerate, 20, 8,
                          kurtNearDegenerate_g2_8, 13, &g_kurtNearCmp );
}

/* (4) A window with no spread returns NaN, not a number.
 *
 * Non-vacuous by construction: the division is unguarded, so without the
 * rebuild anchoring the shift at the window's single value this would be a
 * finite garbage quotient rather than 0/0, and a finite value fails isnan().
 */
static ErrorNumber test_kurt_degenerate( void )
{
   static double in[600], out[600];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   for( i = 0; i < 600; i++ )
      in[i] = 42.0;

   retCode = TA_KURTOSIS( 0, 599, in, 30, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != 571 )
   {
      printf( "KURTOSIS degenerate Fail: rc=%d (%d,%d), expected (29,571)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   KURT_SERVER_VERIFY( 0, 599, 600, retCode, begIdx, nbElement, in, 30, out );

   for( i = 0; i < nbElement; i++ )
   {
      g_kurtNanCmp++;
      if( !isnan( out[i] ) )
      {
         printf( "KURTOSIS degenerate Fail at bar %d: %.17g, expected NaN. A "
                 "point mass has no excess kurtosis and no defensible neutral "
                 "-- 0 asserts normality, -1.2 asserts uniformity\n",
                 begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, outReal == inReal, bitwise. The rebuild re-reads the
 * window after this bar's store, so the store must not land under a read the
 * next bar still needs.
 */
static ErrorNumber test_kurt_aliasing( const TA_History *history )
{
   static double clean[KURT_CAP], alias[KURT_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > KURT_CAP )
      nbBars = KURT_CAP;

   for( n = 4; n <= 40; n += 9 )
   {
      retCode = TA_KURTOSIS( 0, nbBars-1, history->close, n,
                             &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "KURTOSIS alias Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      /* The non-aliased call only: in-place behaviour is a C-side memory
       * property, not something the servers are asked to reproduce. */
      KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          history->close, n, clean );

      for( i = 0; i < nbBars; i++ )
         alias[i] = history->close[i];
      retCode = TA_KURTOSIS( 0, nbBars-1, alias, n,
                             &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "KURTOSIS alias Fail [N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 n, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_kurtAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "KURTOSIS alias Fail [N=%d] bar %d: separate %.17g, "
                    "in-place %.17g\n", n, begIdx+i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) The (n-2)(n-3) denominators are why the range starts at 4, and the
 * contract is what enforces it -- there is no runtime branch for a short
 * window, so this leg is the only thing standing between a caller and a
 * division by zero.
 */
static ErrorNumber test_kurt_contract( const TA_History *history )
{
   static double out[KURT_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, lookback;

   if( nbBars > KURT_CAP )
      nbBars = KURT_CAP;

   for( n = -2; n <= 3; n++ )
   {
      retCode = TA_KURTOSIS( 0, nbBars-1, history->close, n,
                             &begIdx, &nbElement, out );
      if( retCode != TA_BAD_PARAM )
      {
         printf( "KURTOSIS contract Fail: period %d gave rc=%d, expected "
                 "TA_BAD_PARAM -- (n-2)(n-3) is zero or negative below 4\n",
                 n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }

   for( n = 4; n <= 40; n += 9 )
   {
      lookback = TA_KURTOSIS_Lookback( n );
      if( lookback != n-1 )
      {
         printf( "KURTOSIS contract Fail: lookback(%d) = %d, expected %d\n",
                 n, lookback, n-1 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      retCode = TA_KURTOSIS( 0, nbBars-1, history->close, n,
                             &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n-1 || nbElement != nbBars-(n-1) )
      {
         printf( "KURTOSIS contract Fail [N=%d]: rc=%d (%d,%d), expected "
                 "(%d,%d)\n", n, (int)retCode, begIdx, nbElement,
                 n-1, nbBars-(n-1) );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          history->close, n, out );
   }

   return TA_TEST_PASS;
}

/* (7) The startIdx/endIdx range sweep. TA_STABLE_EPSILON, not EXACT: the window
 * is finite, but it is carried in a running accumulator whose shift -- and
 * whose rebuild phase -- are both seeded from the call's own startIdx. Two
 * calls covering the same bar from different starts therefore rebuild on
 * different bars and land a few ulp apart, which is exactly what this class
 * describes. No unstable period: the rebuild re-derives the shift from the
 * window itself and carries nothing in from before startIdx.
 */
typedef struct { int period; const TA_Real *in; } KurtRangeParam;

static TA_RetCode kurtRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   KurtRangeParam *p = (KurtRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_KURTOSIS_Lookback( p->period );
   return TA_KURTOSIS( startIdx, endIdx, p->in, p->period,
                       outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_kurt_range( const TA_History *history )
{
   KurtRangeParam param;

   param.period = 30;
   param.in     = history->close;

   return doRangeTestEx( kurtRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
