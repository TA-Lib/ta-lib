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
 *  082326 MF   Initial coding (#242).
 *
 */

/* Description:
 *     Numerical-robustness tests for TA_CORREL (issue #242).
 *
 *     The shipped hardcoded expectations in test_per_hl.c pin three values at
 *     seven digits on ordinary daily closes, which says nothing about how the
 *     function behaves once a window is ill-conditioned. These probes referee
 *     CORREL against oracles that share no code with it:
 *
 *       C1  per-window two-pass, over pandas' rolling-corr adversarial arrays
 *       C2  NIST StRD certified value (Norris)
 *       C3  exact identities - r(x, a*x+b) == sign(a), true by construction
 *       C4  metamorphic affine invariance
 *       C5  the range invariant |r| <= 1
 *       C6  the documented degenerate contract (constant window -> 0.0)
 *
 *     v0.6.4 is deliberately NOT an oracle here: it carries the same one-pass
 *     sumX2-(sumX*sumX)/n form, so it is co-wrong on exactly the windows these
 *     probes target. An external certified value and a fresh two-pass are the
 *     only referees that can see this class.
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
static ErrorNumber test_correl_pandas_oracle( void );
static ErrorNumber test_correl_nist_norris( void );
static ErrorNumber test_correl_exact_identity( void );
static ErrorNumber test_correl_affine_invariance( void );
static ErrorNumber test_correl_range_invariant( void );
static ErrorNumber test_correl_degenerate( void );

/**** Local variables definitions.     ****/
static double cr_out[4096];

/**** Global functions definitions.   ****/
ErrorNumber test_func_correl( TA_History *history )
{
   ErrorNumber retValue;

   (void)history;   /* every probe below builds its own adversarial series */

   retValue = test_correl_pandas_oracle();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed CORREL two-pass oracle (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_correl_nist_norris();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed CORREL NIST StRD pin (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_correl_exact_identity();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed CORREL exact identity (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_correl_affine_invariance();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed CORREL affine invariance (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_correl_range_invariant();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed CORREL range invariant (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_correl_degenerate();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed CORREL degenerate contract (#242) (Code=%d)\n", __FILE__, retValue ); return retValue; }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* The trusted oracle, the constant-window helper and the RNG all moved to
 * ta_test_reference.{h,c} (#251). The oracle also stopped accumulating in
 * `long double` -- 64 mantissa bits here, 53 on MSVC -- for a compensated
 * double-double form that carries ~106 bits on every ABI, so the bounds below
 * mean the same thing on every platform this library builds on.
 *
 * It keeps the property that made the old one usable: r is translation
 * invariant, so the deviations are formed as n*x - sum(x) rather than
 * x - mean(x), and a window that is constant in double gives an exact zero
 * instead of a rounding residue. A referee has to be better conditioned than
 * the thing it judges.
 */

/* Referee a whole series against the per-window oracle. */
static ErrorNumber cr_check_vs_twopass( const char *label, const double *x, const double *y,
                                        int n, int period, double rtol )
{
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int k;

   if( n > 4096 ) return TA_TEST_PASS;   /* buffer guard */
   rc = TA_CORREL( 0, n-1, x, y, period, &begIdx, &nbElement, cr_out );
   if( rc != TA_SUCCESS )
   {
      printf( "CORREL #242 oracle[%s]: rc=%d\n", label, (int)rc );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( k = 0; k < (int)nbElement; k++ )
   {
      int s = (int)begIdx + k - ( period - 1 );
      double ref, d;

      if( cr_out[k] != cr_out[k] )
      {
         printf( "CORREL #242 oracle[%s]: NaN period=%d bar=%d\n",
                 label, period, (int)begIdx + k );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      ref = ta_test_ref_corr( x, y, s, period );
      d = fabs( ref ) > 1.0e-12 ? fabs( cr_out[k] - ref ) / fabs( ref )
                                : fabs( cr_out[k] - ref );
      if( d > rtol )
      {
         printf( "CORREL #242 oracle[%s]: period=%d bar=%d val=%.17g ref=%.17g (rel %.3g > %.3g)\n",
                 label, period, (int)begIdx + k, cr_out[k], ref, d, rtol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* The same sweep, refereed against BAKED constants instead (#251).
 *
 * The values come from scripts/gen_test_reference.py, which computes Sxx, Syy
 * and Sxy in exact rational arithmetic -- every input is a double, so every sum
 * and product is exact -- and takes ONE correctly-rounded square root of the
 * exact r^2. Nothing in this binary contributed to them, which is the point: a
 * runtime oracle shares the binary's fate and can be co-wrong with what it
 * judges (#228), and `long double` is 53 bits on MSVC, so a pin measured here
 * used to be weaker there. */
static ErrorNumber cr_check_vs_golden( const char *label, const double *x, const double *y,
                                       int n, int period, const double *golden,
                                       int nbGolden, double rtol )
{
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int k;

   if( n > 4096 )
   {
      /* NOT a silent pass: this function's whole body is the comparison, so
       * returning PASS here would skip the corpus assertion and every value. */
      printf( "CORREL #251 golden[%s]: series of %d exceeds cr_out\n", label, n );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( n - period + 1 != nbGolden )
   {
      printf( "CORREL #251 golden[%s]: %d windows but %d baked values -- this test and "
              "scripts/gen_test_reference.py disagree about the corpus\n",
              label, n - period + 1, nbGolden );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   rc = TA_CORREL( 0, n-1, x, y, period, &begIdx, &nbElement, cr_out );
   if( rc != TA_SUCCESS || (int)nbElement != nbGolden )
   {
      printf( "CORREL #251 golden[%s]: rc=%d nb=%d (expected SUCCESS,%d)\n",
              label, (int)rc, (int)nbElement, nbGolden );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( k = 0; k < nbGolden; k++ )
   {
      double ref = golden[k], d;

      if( cr_out[k] != cr_out[k] )
      {
         printf( "CORREL #251 golden[%s]: NaN period=%d bar=%d\n",
                 label, period, (int)begIdx + k );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( !( cr_out[k] >= -1.0 && cr_out[k] <= 1.0 ) )
      {
         printf( "CORREL #251 golden[%s]: period=%d bar=%d r=%.17g outside [-1,1]\n",
                 label, period, (int)begIdx + k, cr_out[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      d = fabs( ref ) > 1.0e-12 ? fabs( cr_out[k] - ref ) / fabs( ref )
                                : fabs( cr_out[k] - ref );
      if( d > rtol )
      {
         printf( "CORREL #251 golden[%s]: period=%d bar=%d val=%.17g golden=%.17g "
                 "(rel %.3g > %.3g)\n",
                 label, period, (int)begIdx + k, cr_out[k], ref, d, rtol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (C1) pandas' rolling-corr adversarial arrays (GH#65739), refereed by the
 * per-window two-pass. These are the bivariate counterpart of the GH#47721 /
 * #52407 / #42064 arrays already used for VAR/STDDEV in test_stddev.c, and they
 * were written against the same three defects: a large value transiting the
 * window, an offset shared by the whole series, and a sum driven negative by
 * cancellation.
 *
 * The magnitudes are rescaled where pandas exceeds this library's declared
 * input domain: their extreme-range case uses 1e308, but TA_REAL_MAX is 3e37,
 * and at that bound n*x*x cannot overflow. Borrow the shape, not the literal
 * value -- the domains differ.
 *
 * The two loose tolerances mirror pandas' own choice for the same arrays: a
 * window that still holds 1e12 is intrinsically ill-conditioned, so 1e-5 is the
 * bound the incremental update is designed to hold, not float64 noise. */
static ErrorNumber test_correl_pandas_oracle( void )
{
   ErrorNumber e;
   int i, k, w;

   /* outlier_exit: a 3.8e12 value transits a window of 9. The y offset covers
    * the case where the other operand is dominated by a shared offset; the swap
    * covers the value landing in either operand, and judges both orders against
    * the SAME table, so the implementation's own symmetry r(x,y) == r(y,x) is
    * pinned rather than assumed. */
   {
      static const double offs[2] = { 0.0, 1.0e13 };
      static const double *const gold[2] = { ta_test_ref_golden_corr_outlier_off0,
                                             ta_test_ref_golden_corr_outlier_off1e13 };
      static double vy[TA_TEST_REF_PD_OUTLIER_N];
      for( k = 0; k < 2; k++ )
      {
         for( i = 0; i < TA_TEST_REF_PD_OUTLIER_N; i++ )
            vy[i] = offs[k] + ta_test_ref_pd_outlier_y[i];
         for( w = 0; w < 2; w++ )
         {
            e = w ? cr_check_vs_golden( "outlier_exit swapped", vy, ta_test_ref_pd_outlier_x,
                                        TA_TEST_REF_PD_OUTLIER_N, 9, gold[k],
                                        TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF0_N, 1.0e-12 )
                  : cr_check_vs_golden( "outlier_exit", ta_test_ref_pd_outlier_x, vy,
                                        TA_TEST_REF_PD_OUTLIER_N, 9, gold[k],
                                        TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF0_N, 1.0e-12 );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }

   /* shared_offset: an offset carried by the whole series leaves almost no
    * significant digits in the deviations. This is issue #242's mechanism --
    * "prices, or epoch timestamps around 1.7e18". */
   {
      static const double offs[2] = { 1.0e10, 1.0e14 };
      static const double *const gold[2] = { ta_test_ref_golden_corr_shared_1e10,
                                             ta_test_ref_golden_corr_shared_1e14 };
      static double sx[TA_TEST_REF_PD_SHARED_N], sy[TA_TEST_REF_PD_SHARED_N];
      for( k = 0; k < 2; k++ )
      {
         for( i = 0; i < TA_TEST_REF_PD_SHARED_N; i++ )
         {
            sx[i] = ta_test_ref_pd_shared_x[i] + offs[k];
            sy[i] = ta_test_ref_pd_shared_y[i] + offs[k];
         }
         e = cr_check_vs_golden( "shared_offset", sx, sy, TA_TEST_REF_PD_SHARED_N, 5,
                                 gold[k], TA_TEST_REF_GOLDEN_CORR_SHARED_1E10_N, 1.0e-12 );
         if( e != TA_TEST_PASS ) return e;
      }
   }

   /* outlier_exit_no_nan: cancellation drove a sum of squares negative, so the
    * divide took the square root of a negative number. extreme_range is the
    * dynamic-range case, rescaled to TA_REAL_MAX as described above. */
   e = cr_check_vs_golden( "outlier_exit_no_nan", ta_test_ref_pd_nonan_x,
                           ta_test_ref_pd_nonan_y, TA_TEST_REF_PD_NONAN_N, 3,
                           ta_test_ref_golden_corr_nonan,
                           TA_TEST_REF_GOLDEN_CORR_NONAN_N, 1.0e-5 );
   if( e != TA_TEST_PASS ) return e;
   e = cr_check_vs_golden( "extreme_range@TA_REAL_MAX", ta_test_ref_pd_extreme_x,
                           ta_test_ref_pd_extreme_y, TA_TEST_REF_PD_EXTREME_N, 5,
                           ta_test_ref_golden_corr_extreme,
                           TA_TEST_REF_GOLDEN_CORR_EXTREME_N, 1.0e-5 );
   if( e != TA_TEST_PASS ) return e;

   return TA_TEST_PASS;
}

/* (C2) NIST StRD certified value. Norris is the only Statistical Reference
 * Dataset in the linear-least-squares collection with a single predictor, which
 * is what makes its certified R-Squared a certified Pearson r: with B1 > 0,
 * r = +sqrt(R2). The value below is transcribed from Norris.dat and is certified
 * to 15 digits by NIST, independent of every implementation.
 *
 *   R-Squared  0.999993745883712   ->  r = 0.99999687293696671  */
static ErrorNumber test_correl_nist_norris( void )
{
   const double *nx = ta_test_ref_norris_x;
   const double *ny = ta_test_ref_norris_y;
   const double certified = TA_TEST_REF_NORRIS_R;   /* +sqrt(0.999993745883712) */
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   double d;

   /* Second, sharper pin on the same call: the certified value is the fit of
    * the exact DECIMALS, and what this library is handed is those decimals
    * rounded to double. TA_TEST_REF_GOLDEN_NORRIS_R is the exact answer for the
    * input the function actually sees, so it admits no representation slack --
    * and the two agreeing to 1e-15 is what says the transcription is right. */
   rc = TA_CORREL( 0, 35, nx, ny, 36, &begIdx, &nbElement, cr_out );
   if( rc != TA_SUCCESS || nbElement != 1 )
   {
      printf( "CORREL #242 NIST Norris: rc=%d nb=%d\n", (int)rc, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   d = fabs( cr_out[0] - certified );
   if( d > 1.0e-13 )
   {
      printf( "CORREL #242 NIST Norris: r=%.17g certified=%.17g (|diff| %.3g)\n",
              cr_out[0], certified, d );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   d = fabs( cr_out[0] - TA_TEST_REF_GOLDEN_NORRIS_R );
   if( d > 1.0e-15 )
   {
      printf( "CORREL #242/#251 NIST Norris: r=%.17g exact-for-these-doubles=%.17g "
              "(|diff| %.3g)\n", cr_out[0], TA_TEST_REF_GOLDEN_NORRIS_R, d );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* Shared body for (C3): r(x, a*x+b) is sign(a) on every window where x varies.
 * No oracle is involved -- the expected value is exact and representable, which
 * makes this the strongest pin available for a correlation. */
static ErrorNumber cr_identity( const char *label, const double *x, int n, int period,
                                double a, double b )
{
   static double y[4096];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   double want = ( a > 0.0 ) ? 1.0 : -1.0;
   int i, k;

   if( n > 4096 ) return TA_TEST_PASS;
   for( i = 0; i < n; i++ ) y[i] = a * x[i] + b;
   rc = TA_CORREL( 0, n-1, x, y, period, &begIdx, &nbElement, cr_out );
   if( rc != TA_SUCCESS )
   {
      printf( "CORREL #242 identity[%s]: rc=%d\n", label, (int)rc );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( k = 0; k < (int)nbElement; k++ )
   {
      int s = (int)begIdx + k - ( period - 1 );
      if( ta_test_ref_window_is_constant( x, s, period ) ) continue;
      if( fabs( cr_out[k] - want ) > 1.0e-12 )
      {
         printf( "CORREL #242 identity[%s]: period=%d bar=%d val=%.17g want %g\n",
                 label, period, (int)begIdx + k, cr_out[k], want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (C3) Exact identities on data chosen to be hostile.
 *
 * NumAcc4 is NIST's univariate cancellation stressor (a large offset carrying a
 * tiny spread); most of its 30-bar windows are exactly constant, and those are
 * skipped because r is genuinely undefined there.
 *
 * The second family is issue #242's own reproduction: y = 2x + tick, at the tick
 * sizes where the shipped function returns 0 for a perfect correlation. */
static ErrorNumber test_correl_exact_identity( void )
{
   static double x[4096];
   static const double ticks[4] = { 1.0e-5, 1.0e-6, 1.0e-8, 1.0e-9 };
   static const double levels[3] = { 0.0, 100.0, 20000.0 };
   ErrorNumber e;
   int i, k, L;

   /* NIST NumAcc4: 9999999 + { 1.1 x500, 1.3 x500, 1.2 x1 }. */
   for( i = 0; i < 500; i++ )  x[i] = 9999999.0 + 1.1;
   for( i = 500; i < 1000; i++ ) x[i] = 9999999.0 + 1.3;
   x[1000] = 9999999.0 + 1.2;
   e = cr_identity( "NumAcc4 a=+2", x, 1001, 30,  2.0, 1.0e-6 );
   if( e != TA_TEST_PASS ) return e;
   e = cr_identity( "NumAcc4 a=-3", x, 1001, 30, -3.0, 5.0 );
   if( e != TA_TEST_PASS ) return e;

   /* Issue #242's ladder. */
   for( L = 0; L < 3; L++ )
      for( k = 0; k < 4; k++ )
      {
         char label[160];
         snprintf( label, sizeof label, "#242 level=%g tick=%.0e", levels[L], ticks[k] );
         for( i = 0; i < 60; i++ ) x[i] = levels[L] + ta_test_ref_ticks60[i] * ticks[k];
         e = cr_identity( label, x, 60, 30, 2.0, ticks[k] );
         if( e != TA_TEST_PASS ) return e;
      }

   return TA_TEST_PASS;
}

/* (C4) Metamorphic affine invariance: r(a*x+b, c*y+d) == sign(a*c) * r(x,y).
 *
 * The comparison is against the per-window oracle re-run on the TRANSFORMED
 * series, not against the untransformed result. Once a shift is large enough to
 * quantise the input -- ulp(1e10 + 100) is 1.9e-6 against a spread of 4, i.e.
 * 4.8e-7 relative -- the transformed series is genuinely different data and the
 * law cannot hold tighter than that. Judging it against the oracle keeps the
 * probe honest about what the transform destroyed. */
static ErrorNumber test_correl_affine_invariance( void )
{
   static double x[300], y[300], xx[300], yy[300];
   static const double as[4] = {  1.0,   1.0,   1.0e-6, -1.0 };
   static const double bs[4] = {  2.0e4, 1.0e10, 0.0,    0.0 };
   static const double cs[4] = {  1.0,   1.0,   1.0e-6,  1.0 };
   static const double ds[4] = {  2.0e4, 1.0e10, 0.0,    0.0 };
   ErrorNumber e;
   int i, k;

   ta_test_ref_xorshift_seed( 2463534242u );
   for( i = 0; i < 300; i++ )
   {
      x[i] = 100.0 + ta_test_ref_xorshift_unit() * 4.0;
      y[i] =  50.0 + ta_test_ref_xorshift_unit() * 3.0;
   }

   for( k = 0; k < 4; k++ )
   {
      char label[160];
      snprintf( label, sizeof label, "affine a=%g b=%g c=%g d=%g", as[k], bs[k], cs[k], ds[k] );
      for( i = 0; i < 300; i++ )
      {
         xx[i] = as[k] * x[i] + bs[k];
         yy[i] = cs[k] * y[i] + ds[k];
      }
      e = cr_check_vs_twopass( label, xx, yy, 300, 30, 1.0e-9 );
      if( e != TA_TEST_PASS ) return e;
   }
   return TA_TEST_PASS;
}

/* (C5) The range invariant. A correlation coefficient cannot leave [-1, 1];
 * a value outside it is a defect no tolerance argument can excuse. */
static ErrorNumber test_correl_range_invariant( void )
{
   static const double levels[3] = { 100.0, 20000.0, 1.0e6 };
   static const double ticks[4] = { 1.0e-2, 1.0e-4, 1.0e-6, 1.0e-8 };
   static double x[60], y[60];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int i, k, L;

   for( L = 0; L < 3; L++ )
      for( k = 0; k < 4; k++ )
      {
         for( i = 0; i < 60; i++ )
         {
            x[i] = levels[L] + ta_test_ref_ticks60[i] * ticks[k];
            y[i] = 2.0 * x[i] + ticks[k];
         }
         rc = TA_CORREL( 0, 59, x, y, 30, &begIdx, &nbElement, cr_out );
         if( rc != TA_SUCCESS )
         {
            printf( "CORREL #242 range: rc=%d\n", (int)rc );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < (int)nbElement; i++ )
            if( !( cr_out[i] >= -1.0 && cr_out[i] <= 1.0 ) )
            {
               printf( "CORREL #242 range: level=%g tick=%.0e bar=%d r=%.17g outside [-1,1]\n",
                       levels[L], ticks[k], (int)begIdx + i, cr_out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
      }
   return TA_TEST_PASS;
}

/* (C6) The documented degenerate contract: when a window makes r undefined the
 * output is 0.0, "rather than an error or NaN" (correl.md). This is a real API
 * promise and differs from every general-purpose library, which returns NaN --
 * so it needs its own pin, and it must survive the fix. */
static ErrorNumber test_correl_degenerate( void )
{
   static double x[60], y[60];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int i, k;

   for( k = 0; k < 2; k++ )
   {
      for( i = 0; i < 60; i++ )
      {
         x[i] = 42.0;
         y[i] = k ? (double)( i % 7 ) : 7.0;
      }
      rc = TA_CORREL( 0, 59, x, y, 30, &begIdx, &nbElement, cr_out );
      if( rc != TA_SUCCESS )
      {
         printf( "CORREL #242 degenerate[%s]: rc=%d\n", k ? "const/varying" : "const/const", (int)rc );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < (int)nbElement; i++ )
         if( cr_out[i] != 0.0 )
         {
            printf( "CORREL #242 degenerate[%s]: bar=%d val=%.17g want exact 0\n",
                    k ? "const/varying" : "const/const", (int)begIdx + i, cr_out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }
   return TA_TEST_PASS;
}
