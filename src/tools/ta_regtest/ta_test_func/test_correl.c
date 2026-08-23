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

/* ============================================================================
 * Trusted oracle: a fresh two-pass Pearson r over one window, accumulated in
 * long double (~19 digits on x86; degrades gracefully to double on ABIs where
 * long double == double -- the probes' tolerances do not depend on the extra
 * digits, they only widen the margin).
 *
 * The window is recentred on its own first element before anything is
 * accumulated. That is pandas' technique for this exact oracle (GH#65739):
 * r is translation-invariant, so recentring changes no true value, but without
 * it the ORACLE ITSELF loses ~1e-8 on the shared-offset arrays and can no
 * longer referee anything. A referee has to be better conditioned than the
 * thing it judges.
 *
 * Returns 0.0 for a window where either series is constant, matching the
 * documented CORREL contract for an undefined correlation.
 * ==========================================================================*/
static double cr_twopass_r( const double *x, const double *y, int s, int period )
{
   long double ox = (long double)x[s], oy = (long double)y[s];
   long double mx = 0.0L, my = 0.0L, sxx = 0.0L, syy = 0.0L, sxy = 0.0L;
   long double dx, dy, r;
   int j;

   for( j = 0; j < period; j++ )
   {
      mx += (long double)x[s+j] - ox;
      my += (long double)y[s+j] - oy;
   }
   mx /= (long double)period;
   my /= (long double)period;

   for( j = 0; j < period; j++ )
   {
      dx = ( (long double)x[s+j] - ox ) - mx;
      dy = ( (long double)y[s+j] - oy ) - my;
      sxx += dx * dx;
      syy += dy * dy;
      sxy += dx * dy;
   }

   if( sxx <= 0.0L || syy <= 0.0L ) return 0.0;
   r = sxy / ( sqrtl( sxx ) * sqrtl( syy ) );
   if( r >  1.0L ) r =  1.0L;
   if( r < -1.0L ) r = -1.0L;
   return (double)r;
}

/* Is every value of the window identical? Then r is undefined there and the
 * documented answer is 0.0 -- such windows are not evidence either way. */
static int cr_window_is_constant( const double *v, int s, int period )
{
   int j;
   for( j = 1; j < period; j++ ) if( v[s+j] != v[s] ) return 0;
   return 1;
}

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
      ref = cr_twopass_r( x, y, s, period );
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

   /* GH#65739 outlier_exit: a 3.8e12 value transits a window of 9. offset_y
    * covers the case where the other operand is dominated by a shared offset;
    * the swap covers the value landing in either operand. */
   {
      static const double vx[18] = { 3,3,7,9,3,3,3.8e12,3,8,2,2,8,6,7,7,3,8,4 };
      static const double vyb[18] = { 6,3,3,5,9,1,2,9,2,1,4,6,6,9,7,9,3,5 };
      static const double offs[2] = { 0.0, 1.0e13 };
      static double vy[18];
      for( k = 0; k < 2; k++ )
      {
         for( i = 0; i < 18; i++ ) vy[i] = offs[k] + vyb[i];
         for( w = 0; w < 2; w++ )
         {
            e = w ? cr_check_vs_twopass( "outlier_exit swapped", vy, vx, 18, 9, 1.0e-12 )
                  : cr_check_vs_twopass( "outlier_exit",         vx, vy, 18, 9, 1.0e-12 );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }

   /* GH#65739 shared_offset: an offset carried by the whole series leaves almost
    * no significant digits in the deviations. This is issue #242's mechanism --
    * "prices, or epoch timestamps around 1.7e18". */
   {
      static const double bx[10] = { 1,2,4,7,3,5,9,2,6,8 };
      static const double by[10] = { 2,1,5,3,8,4,7,9,1,6 };
      static const double offs[2] = { 1.0e10, 1.0e14 };
      static double sx[10], sy[10];
      for( k = 0; k < 2; k++ )
      {
         for( i = 0; i < 10; i++ ) { sx[i] = bx[i] + offs[k]; sy[i] = by[i] + offs[k]; }
         e = cr_check_vs_twopass( "shared_offset", sx, sy, 10, 5, 1.0e-12 );
         if( e != TA_TEST_PASS ) return e;
      }
   }

   /* GH#65739 outlier_exit_no_nan: cancellation drove a sum of squares negative,
    * so the divide took the square root of a negative number. */
   {
      static const double x[8] = { 1.0, 2.0, 1.0e12, 1.0e7, 6.0, 5.0, 8.0, 7.0 };
      static const double y[8] = { 2.0, 1.0, 3.0, -1.0e12, 4.0, 7.0, 6.0, 9.0 };
      e = cr_check_vs_twopass( "outlier_exit_no_nan", x, y, 8, 3, 1.0e-5 );
      if( e != TA_TEST_PASS ) return e;
   }

   /* GH#65739 extreme_range, rescaled to TA_REAL_MAX (see the note above). */
   {
      static const double x[11] = { 3.0e37,1.0,2.0,3.0,-3.0e37,4.0,5.0,6.0,7.0,8.0,9.0 };
      static const double y[11] = { 0.2,0.1,0.5,0.3,0.8,0.4,0.9,0.6,0.7,0.2,0.5 };
      e = cr_check_vs_twopass( "extreme_range@TA_REAL_MAX", x, y, 11, 5, 1.0e-5 );
      if( e != TA_TEST_PASS ) return e;
   }

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
   static const double ny[36] = {
      0.1,338.8,118.1,888.0,9.2,228.1,668.5,998.5,449.1,778.9,559.2,0.3,
      0.1,778.1,668.8,339.3,448.9,10.8,557.7,228.3,998.0,888.8,119.6,0.3,
      0.6,557.6,339.3,888.0,998.5,778.9,10.2,117.6,228.9,668.4,449.2,0.2 };
   static const double nx[36] = {
      0.2,337.4,118.2,884.6,10.1,226.5,666.3,996.3,448.6,777.0,558.2,0.4,
      0.6,775.5,666.9,338.0,447.5,11.6,556.0,228.1,995.8,887.6,120.2,0.3,
      0.3,556.8,339.1,887.2,999.0,779.0,11.1,118.3,229.2,669.1,448.9,0.5 };
   const double certified = 0.99999687293696671;   /* +sqrt(0.999993745883712) */
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   double d;

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
      if( cr_window_is_constant( x, s, period ) ) continue;
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
   static const int T[60] = {
      197,200,199,197,198,196,194,192,195,193,195,193,192,194,197,194,192,189,186,183,
      185,186,185,182,180,182,180,179,180,180,182,182,185,188,188,189,191,193,190,192,
      195,193,191,190,188,185,182,182,182,179,179,179,176,173,172,170,167,166,169,170 };
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
         for( i = 0; i < 60; i++ ) x[i] = levels[L] + T[i] * ticks[k];
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
   unsigned int s = 2463534242u;
   ErrorNumber e;
   int i, k;

   for( i = 0; i < 300; i++ )
   {
      s ^= s << 13; s ^= s >> 17; s ^= s << 5;
      x[i] = 100.0 + (double)( ( s >> 8 ) & 0xffffu ) / 65535.0 * 4.0;
      s ^= s << 13; s ^= s >> 17; s ^= s << 5;
      y[i] =  50.0 + (double)( ( s >> 8 ) & 0xffffu ) / 65535.0 * 3.0;
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
   static const int T[60] = {
      197,200,199,197,198,196,194,192,195,193,195,193,192,194,197,194,192,189,186,183,
      185,186,185,182,180,182,180,179,180,180,182,182,185,188,188,189,191,193,190,192,
      195,193,191,190,188,185,182,182,182,179,179,179,176,173,172,170,167,166,169,170 };
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
            x[i] = levels[L] + T[i] * ticks[k];
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
