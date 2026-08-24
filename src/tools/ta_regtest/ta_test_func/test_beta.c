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
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  082326 MF   Initial coding (#242 addendum).
 *
 */

/* Description:
 *     Numerical-robustness tests for TA_BETA (#242 addendum).
 *
 *     TA_BETA is a regression SLOPE -- of the security's returns on the index's
 *     returns -- so the references that pin it are the ones used for linear
 *     least squares, not for correlation:
 *
 *       B1  Wilkinson's "nasty.dat" battery (W.IV.B), the classic reliability
 *           quiz for statistical software; scipy tests linregress against the
 *           same arrays
 *       B2  Wilkinson W.IV.D -- a zero-variance input
 *       B3  NIST StRD Norris' certified regression slope B1
 *       B4  the exact scaling identity beta == k
 *       B5  a per-window mean-centred two-pass oracle
 *       B6  the documented degenerate contract
 *
 *     Unlike TA_CORREL (#242), BETA works on RETURNS, which are already
 *     near-zero-mean: the measured cancellation pressure S_x^2/(n*S_xx) stays
 *     between 0.008 and 0.52 on real shapes, never near 1. So the shifted-data
 *     treatment TA_VAR and TA_CORREL need buys nothing here, and these probes
 *     target the defect BETA actually has -- an ABSOLUTE epsilon on a quantity
 *     that scales with the square of the return volatility.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_test_reference.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static ErrorNumber test_beta_wilkinson_self( void );
static ErrorNumber test_beta_wilkinson_zero( void );
static ErrorNumber test_beta_nist_norris( void );
static ErrorNumber test_beta_scaling_identity( void );
static ErrorNumber test_beta_twopass_oracle( void );
static ErrorNumber test_beta_degenerate( void );
static ErrorNumber test_beta_outlier_transit( void );

/**** Local variables definitions.     ****/
static double bt_out[4096];

/* Wilkinson's "nasty.dat" arrays now live in the shared battery (#251), where
 * test_linearreg.c reads them too -- there they are regressed on the bar index,
 * which is Wilkinson's own task IV.B. Read as PRICE series HERE: BIG and LITTLE
 * both carry a per-bar return of ~1e-8, which is precisely the regime where an
 * absolute 1e-14 band on n*S_xx - S_x*S_x swallows a well-defined slope. */
#define W_X      ta_test_ref_wilkinson_x
#define W_ROUND  ta_test_ref_wilkinson_round
#define W_HUGE   ta_test_ref_wilkinson_huge
#define W_TINY   ta_test_ref_wilkinson_tiny
#define W_BIG    ta_test_ref_wilkinson_big
#define W_LITTLE ta_test_ref_wilkinson_little
#define W_ZERO   ta_test_ref_wilkinson_zero

/**** Global functions definitions.   ****/
ErrorNumber test_func_beta( TA_History *history )
{
   ErrorNumber retValue;

   (void)history;   /* every probe below builds its own series */

   retValue = test_beta_wilkinson_self();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed BETA Wilkinson W.IV.B (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_beta_wilkinson_zero();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed BETA Wilkinson W.IV.D (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_beta_nist_norris();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed BETA NIST StRD pin (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_beta_scaling_identity();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed BETA scaling identity (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_beta_twopass_oracle();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed BETA two-pass oracle (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_beta_degenerate();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed BETA degenerate contract (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_beta_outlier_transit();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed BETA outlier transit (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* BETA's own return, zero-price guard included. */
static double bt_ret( const double *p, int i )
{
   return ( p[i-1] != 0.0 ) ? ( p[i] - p[i-1] ) / p[i-1] : 0.0;
}

/* The window of returns TA_BETA actually regresses, materialised so the shared
 * oracle -- which takes two arrays like every other regression -- can referee
 * it (#251). This is the only BETA-specific step: everything downstream is the
 * generic OLS slope, in compensated double-double rather than the `long double`
 * this file used to carry (64 mantissa bits here, 53 on MSVC). */
/* Sized past the longest series in this file (400 bars), so period > N is
 * impossible and the guards below are unreachable by construction. They still
 * poison rather than return 0.0: both callers read 0.0 as "skip this bar", so
 * the original 256 turned a whole leg silently green the moment a corpus grew
 * past it -- fail-open in the one place a test must not. */
#define BT_MAX_PERIOD 512
#define BT_POISON     (-1.0e300)
static void bt_window_returns( const double *px, const double *py, int end, int period,
                               double *rx, double *ry )
{
   int i;
   for( i = 0; i < period; i++ )
   {
      const int bar = end - period + 1 + i;
      rx[i] = bt_ret( px, bar );
      ry[i] = bt_ret( py, bar );
   }
}

/* Trusted oracle: the OLS slope of the security's returns on the index's, over
 * the `period` returns ending at bar `end`. Also reports the window
 * conditioning (max|return| against how much the returns actually vary), so the
 * caller can widen its bound where the data itself cannot resolve the slope.
 * Returns 0.0 where the regressor has no variance, matching the contract. */
static double bt_twopass_beta( const double *px, const double *py, int end, int period,
                               double *outKappa )
{
   static double rx[BT_MAX_PERIOD], ry[BT_MAX_PERIOD];
   if( period > BT_MAX_PERIOD )
   {
      printf( "BETA #251: period %d exceeds BT_MAX_PERIOD %d -- raise it; the "
              "oracle cannot referee this window\n", period, BT_MAX_PERIOD );
      if( outKappa ) *outKappa = 0.0;
      return BT_POISON;   /* not 0.0: the callers read 0.0 as "skip" */
   }
   bt_window_returns( px, py, end, period, rx, ry );
   return ta_test_ref_slope( rx, ry, 0, period, outKappa );
}

/* (B1) Wilkinson W.IV.B: regressing a series on ITSELF must give a slope of
 * exactly 1. scipy asserts the same thing of linregress. The expected value is
 * exact and representable, so this needs no oracle and no tolerance argument. */
static ErrorNumber test_beta_wilkinson_self( void )
{
   static const struct { const char *name; const double *p; } cases[] = {
      { "X",      W_X      }, { "ROUND",  W_ROUND  },
      { "HUGE",   W_HUGE   }, { "TINY",   W_TINY   },
      { "BIG",    W_BIG    }, { "LITTLE", W_LITTLE },
   };
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   unsigned int c;
   int k;

   for( c = 0; c < sizeof(cases)/sizeof(cases[0]); c++ )
   {
      rc = TA_BETA( 0, 8, cases[c].p, cases[c].p, 8, &begIdx, &nbElement, bt_out );
      if( rc != TA_SUCCESS || nbElement < 1 )
      {
         printf( "BETA #242 W.IV.B[%s]: rc=%d nb=%d\n", cases[c].name, (int)rc, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( k = 0; k < (int)nbElement; k++ )
         if( fabs( bt_out[k] - 1.0 ) > 1.0e-12 )
         {
            printf( "BETA #242 W.IV.B[%s]: beta(P,P)=%.17g want exactly 1 "
                    "(per-bar return ~%.2e)\n",
                    cases[c].name, bt_out[k], fabs( bt_ret( cases[c].p, 1 ) ) );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }
   return TA_TEST_PASS;
}

/* (B2) Wilkinson W.IV.D: an input with no variance. The slope is undefined and
 * this library's contract is 0.0 -- never NaN, never an error. */
static ErrorNumber test_beta_wilkinson_zero( void )
{
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int k;

   rc = TA_BETA( 0, 8, W_ZERO, W_X, 8, &begIdx, &nbElement, bt_out );
   if( rc != TA_SUCCESS )
   {
      printf( "BETA #242 W.IV.D: rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( k = 0; k < (int)nbElement; k++ )
      if( bt_out[k] != 0.0 )
      {
         printf( "BETA #242 W.IV.D: bar %d = %.17g want exact 0\n", (int)begIdx + k, bt_out[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   return TA_TEST_PASS;
}

/* (B3) NIST StRD Norris certifies the regression slope B1 to 15 digits. BETA
 * regresses RETURNS, so the data is fed as returns: prices are built with
 * p[i] = p[i-1] * (1 + s*value). A slope is invariant under a COMMON scale on
 * both series -- beta(s*x, s*y) == beta(x, y) -- so s only keeps the prices in
 * a sane range and does not move the certified answer.
 *
 * The tolerance is the price->return round trip, not the algorithm: rebuilding
 * r from consecutive prices costs a few ulp. Measured 4.4e-15 at s=1e-3. */
static ErrorNumber test_beta_nist_norris( void )
{
   const double *nx = ta_test_ref_norris_x;
   const double *ny = ta_test_ref_norris_y;
   const double certified = TA_TEST_REF_NORRIS_B1;   /* NIST StRD Norris, B1 */
   const double s = 1.0e-3;
   static double px[37], py[37];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   double d;
   int i;

   px[0] = 1.0;
   py[0] = 1.0;
   for( i = 0; i < 36; i++ )
   {
      px[i+1] = px[i] * ( 1.0 + s * nx[i] );
      py[i+1] = py[i] * ( 1.0 + s * ny[i] );
   }

   rc = TA_BETA( 0, 36, px, py, 36, &begIdx, &nbElement, bt_out );
   if( rc != TA_SUCCESS || nbElement < 1 )
   {
      printf( "BETA #242 NIST Norris: rc=%d nb=%d\n", (int)rc, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   d = fabs( bt_out[nbElement-1] - certified ) / fabs( certified );
   if( d > 1.0e-12 )
   {
      printf( "BETA #242 NIST Norris: slope=%.17g certified=%.17g (rel %.3g)\n",
              bt_out[nbElement-1], certified, d );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (B4) If the security's returns are exactly k times the index's, the slope is
 * exactly k -- at any volatility. This is the identity the reported defect
 * breaks: below a per-bar return of ~1e-8 the guarded quantity n*S_xx - S_x*S_x
 * falls under the fixed 1e-14 band and the output collapses to 0.
 *
 * The tolerance tracks the INPUT, not the algorithm. Recovering r from
 * consecutive prices is a cancelling subtraction, so the returns themselves
 * carry ~eps/vol of relative error; demanding more than that of the slope would
 * be asking the test to out-resolve its own data. */
static ErrorNumber test_beta_scaling_identity( void )
{
   static const double vols[6] = { 1.0e-2, 1.0e-4, 1.0e-6, 1.0e-7, 1.0e-8, 1.0e-9 };
   static const double ks[3]   = { 2.0, -0.5, 1.0 };
   static double px[400], py[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int v, kk, i, j;

   ta_test_ref_lcg_seed( 7u );
   for( v = 0; v < 6; v++ )
      for( kk = 0; kk < 3; kk++ )
      {
         double k = ks[kk];
         double tol = 1.0e-9 + 100.0 * 2.2204460492503131e-16 / vols[v];

         px[0] = 100.0;
         py[0] = 250.0;
         for( i = 1; i < 400; i++ )
         {
            double r = vols[v] * ta_test_ref_lcg_half();
            px[i] = px[i-1] * ( 1.0 + r );
            py[i] = py[i-1] * ( 1.0 + k * r );
         }

         rc = TA_BETA( 0, 399, px, py, 30, &begIdx, &nbElement, bt_out );
         if( rc != TA_SUCCESS )
         {
            printf( "BETA #242 scaling[vol=%.0e k=%g]: rc=%d\n", vols[v], k, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( j = 0; j < (int)nbElement; j++ )
            if( fabs( bt_out[j] - k ) / fabs( k ) > tol )
            {
               printf( "BETA #242 scaling[vol=%.0e k=%g]: bar %d = %.17g want %g "
                       "(rel %.3g > %.3g)\n", vols[v], k, (int)begIdx + j, bt_out[j], k,
                       fabs( bt_out[j] - k ) / fabs( k ), tol );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
      }
   return TA_TEST_PASS;
}

/* (B5) The referee leg: a fresh per-window two-pass over random data, at
 * volatilities spanning the band where the fixed epsilon bites. Unlike the
 * identities above this pins arbitrary slopes, so it is the leg that would
 * catch a future cancellation defect rather than only a collapsed guard. */
static ErrorNumber test_beta_twopass_oracle( void )
{
   static const double vols[4] = { 1.0e-2, 1.0e-4, 1.0e-6, 1.0e-8 };
   static const int periods[3] = { 2, 14, 60 };
   static double px[400], py[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int v, p, i, j;

   ta_test_ref_lcg_seed( 991u );
   for( v = 0; v < 4; v++ )
      for( p = 0; p < 3; p++ )
      {
         /* The bound is the ALGORITHM'S DESIGN, not a fitted constant. The
          * reseed trigger fires when the denominator drops below 1e-6 of its
          * scale, so between reseeds each extracted quantity can carry ~eps/1e-6
          * of cancellation, and a slope is a ratio of two of them. Measured
          * worst case is 1.2e-9, at period 2 where a 2-bar window turns over
          * long before the shift is re-anchored; periods 14 and 60 come in at
          * 1.7e-15 to 2e-14, four orders inside the bound.
          *
          * The oracle leg is the arbitrary-value referee -- the tight lines are
          * held by the exact identities above (Wilkinson 1, NIST 1e-12, the
          * scaling identity). A defect that broke the shift would show as 1e-3
          * or worse; before this fix these windows returned 0. */
         double tol = 1.0e-8;

         px[0] = 100.0;
         py[0] = 250.0;
         for( i = 1; i < 400; i++ )
         {
            double a = vols[v] * ta_test_ref_lcg_half();
            double b = vols[v] * ta_test_ref_lcg_half();
            px[i] = px[i-1] * ( 1.0 + a );
            py[i] = py[i-1] * ( 1.0 + 0.7*a + 0.3*b );
         }

         rc = TA_BETA( 0, 399, px, py, periods[p], &begIdx, &nbElement, bt_out );
         if( rc != TA_SUCCESS )
         {
            printf( "BETA #242 oracle[vol=%.0e period=%d]: rc=%d\n",
                    vols[v], periods[p], (int)rc );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( j = 0; j < (int)nbElement; j++ )
         {
            double kappa = 0.0;
            double ref = bt_twopass_beta( px, py, (int)begIdx + j, periods[p], &kappa );
            double d, wtol;
            if( bt_out[j] != bt_out[j] )
            {
               printf( "BETA #242 oracle[vol=%.0e period=%d]: NaN at bar %d\n",
                       vols[v], periods[p], (int)begIdx + j );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            if( fabs( ref ) < 1.0e-12 ) continue;   /* slope ~0: relative test is ill-posed */
            wtol = tol + 100.0 * kappa * 2.2204460492503131e-16;
            d = fabs( bt_out[j] - ref ) / fabs( ref );
            if( d > wtol )
            {
               printf( "BETA #242 oracle[vol=%.0e period=%d]: bar %d = %.17g ref=%.17g "
                       "(rel %.3g > %.3g, kappa %.2g)\n", vols[v], periods[p], (int)begIdx + j,
                       bt_out[j], ref, d, wtol, kappa );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   return TA_TEST_PASS;
}

/* (B6) The documented degenerate contract: a window with no variation in the
 * regressor leaves the slope undefined, and the output is 0.0 rather than an
 * error or NaN. */
static ErrorNumber test_beta_degenerate( void )
{
   static double px[60], py[60];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int i, k, c;

   for( c = 0; c < 2; c++ )
   {
      for( i = 0; i < 60; i++ )
      {
         px[i] = 42.0;                          /* flat index: no regressor variance */
         py[i] = c ? ( 17.0 + (double)(i % 5) ) : 17.0;
      }
      rc = TA_BETA( 0, 59, px, py, 30, &begIdx, &nbElement, bt_out );
      if( rc != TA_SUCCESS )
      {
         printf( "BETA #242 degenerate[%s]: rc=%d\n", c ? "flat/varying" : "flat/flat", (int)rc );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( k = 0; k < (int)nbElement; k++ )
         if( bt_out[k] != 0.0 )
         {
            printf( "BETA #242 degenerate[%s]: bar %d = %.17g want exact 0\n",
                    c ? "flat/varying" : "flat/flat", (int)begIdx + k, bt_out[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }
   return TA_TEST_PASS;
}

/* sd(y-returns)/sd(x-returns) over the window: the ceiling on |beta| there, and
 * so the scale an error in it should be measured against. */
static double bt_beta_scale( const double *px, const double *py, int end, int period )
{
   static double rx[BT_MAX_PERIOD], ry[BT_MAX_PERIOD];
   double sx;
   if( period > BT_MAX_PERIOD )
   {
      printf( "BETA #251: period %d exceeds BT_MAX_PERIOD %d\n", period, BT_MAX_PERIOD );
      return BT_POISON;
   }
   bt_window_returns( px, py, end, period, rx, ry );
   sx = ta_test_ref_stddev( rx, 0, period, NULL );
   if( sx <= 0.0 ) return 0.0;
   return ta_test_ref_stddev( ry, 0, period, NULL ) / sx;
}

/* (B7) SYNTHETIC LIMIT CASE -- not a model of any input this library expects.
 *
 * Read this as a boundary probe on the sliding sums, not as a domain test. It
 * drives a single bar 1000x to 1e10x away from its neighbours, which is not a
 * price, not a volume and not a bad print: it is the magnitude at which the
 * accumulators demonstrably break, found by sweeping until they did. Good to
 * pass; NOT a release-blocking property.
 *
 * What was measured on data the library IS given, before this test existed --
 * equity closes (GBM), two names' daily volume across quiet and news days, a 10x
 * fat-finger print corrected on the next bar, and a spread that crosses zero, at
 * periods 5/14/30: every cell already agreed with a long-double two-pass to
 * better than 4.3e-10, with zero bars above 1e-9, WITHOUT the trigger this test
 * pins. So nothing in the realistic domain was ever broken, and nothing here
 * should be read as claiming otherwise. The trigger does tighten those cells
 * (the fat-finger case 2.7e-10 -> 2.3e-14, the spread 4.2e-10 -> 8.2e-12), which
 * is why it is kept -- three lines and at most ~3% -- but that is an improvement,
 * not a repair.
 *
 * The mechanism, for whoever meets it in a stranger regime than equities. The
 * sliding sums cannot un-see an outlier: while a huge return sits in the window
 * S_xx is entirely that one term, and the ordinary ones fall below its ulp and
 * are never really added. When it leaves, the subtraction takes back a term they
 * were never part of, and the residue no longer describes the window. The
 * cancellation trigger cannot see this -- the residue is a consistent OFFSET, so
 * denom/denom_scale stays ~1 -- and only the periodic re-anchor recovers, up to
 * 32*period bars later. Unguarded, a 1e8 tick left 286 of 386 bars wrong, the
 * worst by 0.36 ABSOLUTE, silently and with TA_SUCCESS. That is worth foreclosing
 * even at a magnitude nobody trades at, because TA_BETA takes any inReal -- a
 * ratio, a spread, an open-interest series -- not only prices.
 *
 * The threshold is 1e3 rather than TA_VAR's 1e6 because a return amplifies: a
 * tick multiplying the price by k puts k-1 into the return and (k-1)^2 into
 * S_xx, so the ratio when that term leaves lands an order or two below the
 * value-scale case var.c was tuned on. At 1e6 a 1e5 tick slips through --
 * verified: a flat 2.5e-5 relative error on 285 of 386 bars.
 *
 * Non-vacuity is structural: every rung asserts recovery on the bars AFTER the
 * spike has left the window, over six magnitudes in both directions, so a
 * trigger that stopped firing fails several rungs at once. Confirmed by removing
 * the trigger: the 1e5 rung fails at 4.4e-05 against its 2e-07 bound.
 */
static ErrorNumber test_beta_outlier_transit( void )
{
   enum { N = 400, SPIKE_AT = 100 };
   static double px[N], py[N], out[N];
   static const double spikes[] = { 1.1e2, 2.0e2, 1.0e3, 1.0e5, 1.0e8, 1.0e12,
                                    1.0e-3, 1.0e-9 };
   static const int periods[] = { 5, 14, 30 };
   TA_Integer b, nb;
   TA_RetCode rc;
   unsigned int si;
   int pi, i, k, axis;

   /* BOTH axes. The denominator is x-only, so it is easy to assume the y side
    * needs no cover -- an earlier version of this test swept only px and was
    * green while a py spike failed 12 of these 24 rungs, worst 156x relative.
    * The output reads S_xy and S_y too. Sweeping both is what makes the second
    * trigger disjunct in beta.c a pinned property rather than a measured one. */
   for( axis = 0; axis < 2; axis++ )
   for( si = 0; si < sizeof(spikes)/sizeof(spikes[0]); si++ )
   for( pi = 0; pi < 3; pi++ )
   {
      const int period = periods[pi];
      for( i = 0; i < N; i++ )
      {
         px[i] = 100.0 + (double)( ( i * 37 ) % 11 ) * 0.01;
         py[i] = 200.0 + (double)( ( i * 53 ) % 13 ) * 0.02;
      }
      if( axis == 0 ) px[SPIKE_AT] = spikes[si];
      else            py[SPIKE_AT] = spikes[si];

      rc = TA_BETA( 0, N-1, px, py, period, &b, &nb, out );
      if( rc != TA_SUCCESS )
      {
         printf( "BETA #242 outlier transit: rc=%d axis=%s spike=%g period=%d\n",
                 (int)rc, axis ? "y" : "x", spikes[si], period );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( k = 0; k < (int)nb; k++ )
      {
         const int bar = (int)b + k;
         double kappa = 0.0, ref, tol, d, scale, norm;
         /* Only bars whose window no longer contains the spike, nor the return
          * it induces on the bar after it. Those two ARE the outlier and are
          * legitimately whatever the data says. */
         if( bar <= SPIKE_AT + 1 + period ) continue;
         ref = bt_twopass_beta( px, py, bar, period, &kappa );
         /* Judge the error against the scale a slope LIVES on, not against its
          * own magnitude: |beta| = |r| * sd(y)/sd(x), so sd(y)/sd(x) is its
          * ceiling and a beta that happens to sit near zero (r ~ 0) is not
          * thereby entitled to a tighter absolute bound than one that does not.
          * Output-relative alone would false-red on exactly those bars -- a
          * reference slope of 3.5e-10 fails a 1e-9 relative test on an absolute
          * error of 8e-18. */
         scale = bt_beta_scale( px, py, bar, period );
         norm  = ( fabs( ref ) > scale ) ? fabs( ref ) : scale;
         if( norm == 0.0 ) continue;
         /* 1e-9 everywhere except three named rungs, rather than one loose
          * bound over all 24: a blanket tolerance sized for the worst cell
          * stops pinning the other 21.
          *
          * The exception is period 5 at a spike of 1e5 or more, where the
          * trigger takes the error from 0.36 ABSOLUTE down to 3.7e-08 but not
          * to zero -- ~90 bars stay between 1e-9 and 3.7e-08. That tail is NOT
          * the threshold being too coarse (dropping it to 10 leaves the tail
          * unchanged and adds a failing rung at 1e3), so it is a second, far
          * smaller mechanism, left open deliberately and pinned here at its
          * measured size so it cannot grow unnoticed.
          *
          * 1.2e-7, sized at ~3x the measurement the way this file's other bounds
          * are: the residue is 3.69e-08 worst on the x axis and 2.31e-08 on y.
          * An earlier 2e-7 was 5.4x, and the slack was not free -- a mutation
          * sweep showed it sheltered a real regression. Raising beta.c's y-side
          * trigger from 1e3 to TA_VAR's 1e6 degrades this ladder by up to 265x
          * relative, peaking at 1.65e-07, which passed under 2e-7 and fails
          * under 1.2e-7. Sizing the bound honestly is what made the test catch
          * it; do not widen this without re-measuring. */
         tol = ( period == 5 && spikes[si] >= 1.0e5 ) ? 1.2e-7 : 1.0e-9;
         tol += 100.0 * kappa * 2.2204460492503131e-16;
         d   = fabs( out[k] - ref ) / norm;
         if( d > tol )
         {
            printf( "BETA #242 outlier transit: axis=%s spike=%g period=%d bar=%d "
                    "val=%.17g ref=%.17g (rel %.3g > %.3g, kappa %.2g, scale %.3g)\n",
                    axis ? "y" : "x", spikes[si], period, bar, out[k], ref, d, tol, kappa, scale );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   return TA_TEST_PASS;
}
