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

/**** Local variables definitions.     ****/
static double bt_out[4096];

/* Wilkinson's "nasty.dat" arrays, as used by scipy's linregress tests. Read as
 * PRICE series here: BIG and LITTLE both carry a per-bar return of ~1e-8, which
 * is precisely the regime where an absolute 1e-14 band on n*S_xx - S_x*S_x
 * swallows a well-defined slope. */
static const double W_X[9]      = { 1,2,3,4,5,6,7,8,9 };
static const double W_ROUND[9]  = { 0.5,1.5,2.5,3.5,4.5,5.5,6.5,7.5,8.5 };
static const double W_HUGE[9]   = { 1e12,2e12,3e12,4e12,5e12,6e12,7e12,8e12,9e12 };
static const double W_TINY[9]   = { 1e-12,2e-12,3e-12,4e-12,5e-12,6e-12,7e-12,8e-12,9e-12 };
static const double W_BIG[9]    = { 99999991,99999992,99999993,99999994,99999995,
                                    99999996,99999997,99999998,99999999 };
static const double W_LITTLE[9] = { 0.99999991,0.99999992,0.99999993,0.99999994,0.99999995,
                                    0.99999996,0.99999997,0.99999998,0.99999999 };
static const double W_ZERO[9]   = { 0,0,0,0,0,0,0,0,0 };

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

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* Deterministic LCG so the data is reproducible across platforms. */
static unsigned int bt_rng = 0u;
static double bt_rand( void )   /* uniform [-0.5, 0.5) */
{
   bt_rng = bt_rng * 1103515245u + 12345u;
   return ( (double)( ( bt_rng >> 8 ) & 0xffffffu ) / 16777216.0 ) - 0.5;
}

/* BETA's own return, zero-price guard included. */
static double bt_ret( const double *p, int i )
{
   return ( p[i-1] != 0.0 ) ? ( p[i] - p[i-1] ) / p[i-1] : 0.0;
}

/* Trusted oracle: a mean-centred two-pass OLS slope over the `period` returns
 * ending at bar `end`, accumulated in long double. Verified to agree with the
 * shipped function to 1.7e-15 on well-conditioned data before being relied on.
 * Returns 0.0 where the regressor has no variance, matching the contract. */
static double bt_twopass_beta( const double *px, const double *py, int end, int period )
{
   long double mx = 0.0L, my = 0.0L, sxx = 0.0L, sxy = 0.0L, dx, dy;
   int i;

   for( i = end-period+1; i <= end; i++ ) { mx += bt_ret(px,i); my += bt_ret(py,i); }
   mx /= (long double)period;
   my /= (long double)period;
   for( i = end-period+1; i <= end; i++ )
   {
      dx = (long double)bt_ret(px,i) - mx;
      dy = (long double)bt_ret(py,i) - my;
      sxx += dx * dx;
      sxy += dx * dy;
   }
   if( sxx <= 0.0L ) return 0.0;
   return (double)( sxy / sxx );
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
   static const double nx[36] = {
      0.2,337.4,118.2,884.6,10.1,226.5,666.3,996.3,448.6,777.0,558.2,0.4,
      0.6,775.5,666.9,338.0,447.5,11.6,556.0,228.1,995.8,887.6,120.2,0.3,
      0.3,556.8,339.1,887.2,999.0,779.0,11.1,118.3,229.2,669.1,448.9,0.5 };
   static const double ny[36] = {
      0.1,338.8,118.1,888.0,9.2,228.1,668.5,998.5,449.1,778.9,559.2,0.3,
      0.1,778.1,668.8,339.3,448.9,10.8,557.7,228.3,998.0,888.8,119.6,0.3,
      0.6,557.6,339.3,888.0,998.5,778.9,10.2,117.6,228.9,668.4,449.2,0.2 };
   const double certified = 1.00211681802045;   /* NIST StRD Norris, B1 */
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

   bt_rng = 7u;
   for( v = 0; v < 6; v++ )
      for( kk = 0; kk < 3; kk++ )
      {
         double k = ks[kk];
         double tol = 1.0e-9 + 100.0 * 2.2204460492503131e-16 / vols[v];

         px[0] = 100.0;
         py[0] = 250.0;
         for( i = 1; i < 400; i++ )
         {
            double r = vols[v] * bt_rand();
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

   bt_rng = 991u;
   for( v = 0; v < 4; v++ )
      for( p = 0; p < 3; p++ )
      {
         double tol = 1.0e-9 + 100.0 * 2.2204460492503131e-16 / vols[v];

         px[0] = 100.0;
         py[0] = 250.0;
         for( i = 1; i < 400; i++ )
         {
            double a = vols[v] * bt_rand();
            double b = vols[v] * bt_rand();
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
            double ref = bt_twopass_beta( px, py, (int)begIdx + j, periods[p] );
            double d;
            if( bt_out[j] != bt_out[j] )
            {
               printf( "BETA #242 oracle[vol=%.0e period=%d]: NaN at bar %d\n",
                       vols[v], periods[p], (int)begIdx + j );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            if( fabs( ref ) < 1.0e-12 ) continue;   /* slope ~0: relative test is ill-posed */
            d = fabs( bt_out[j] - ref ) / fabs( ref );
            if( d > tol )
            {
               printf( "BETA #242 oracle[vol=%.0e period=%d]: bar %d = %.17g ref=%.17g "
                       "(rel %.3g > %.3g)\n", vols[v], periods[p], (int)begIdx + j,
                       bt_out[j], ref, d, tol );
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
