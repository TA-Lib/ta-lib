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
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  082326 MF,CC Initial coding (#251): the datasets, oracles and RNG that
 *               test_stddev.c, test_correl.c and test_beta.c each carried a
 *               private copy of, plus the double-double accumulation that
 *               replaces their `long double`.
 *
 */

/* Description:
 *     Shared numerical-reference battery. See ta_test_reference.h for what is
 *     here and why the accumulation is compensated rather than `long double`.
 *
 *     THE DATASET DEFINITIONS BELOW ARE PARSED BY scripts/gen_test_reference.py
 *     to compute the baked goldens in ta_test_reference_golden.{h,c}. Keep them
 *     as one `const double NAME[COUNT] = { ... };` statement each, with plain
 *     decimal literals -- the parser is deliberately narrow and will refuse
 *     anything it does not recognise rather than guess.
 */

/**** Headers ****/
#include <math.h>
#include <stddef.h>

#include "ta_test_reference.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/

/* Compensated double-double. `hi` is the correctly-rounded double of the pair,
 * `lo` the residue, |lo| <= 0.5 ulp(hi) after every operation below. */
typedef struct { double hi, lo; } ta_dd;

/**** Local functions declarations.    ****/
/* None -- the double-double primitives are defined before first use. */

/**** Local variables definitions.     ****/

static unsigned int ta_test_ref_lcg_state      = 0u;
static unsigned int ta_test_ref_xorshift_state = 2463534242u;

/* ===========================================================================
 * DATASETS  (parsed by scripts/gen_test_reference.py -- see the note above)
 * ========================================================================= */

/* --- BEGIN GENERATOR-PARSED DATASETS --- */

const double ta_test_ref_norris_x[TA_TEST_REF_NORRIS_N] = {
   0.2, 337.4, 118.2, 884.6, 10.1, 226.5, 666.3, 996.3, 448.6, 777.0, 558.2, 0.4,
   0.6, 775.5, 666.9, 338.0, 447.5, 11.6, 556.0, 228.1, 995.8, 887.6, 120.2, 0.3,
   0.3, 556.8, 339.1, 887.2, 999.0, 779.0, 11.1, 118.3, 229.2, 669.1, 448.9, 0.5 };

const double ta_test_ref_norris_y[TA_TEST_REF_NORRIS_N] = {
   0.1, 338.8, 118.1, 888.0, 9.2, 228.1, 668.5, 998.5, 449.1, 778.9, 559.2, 0.3,
   0.1, 778.1, 668.8, 339.3, 448.9, 10.8, 557.7, 228.3, 998.0, 888.8, 119.6, 0.3,
   0.6, 557.6, 339.3, 888.0, 998.5, 778.9, 10.2, 117.6, 228.9, 668.4, 449.2, 0.2 };

const double ta_test_ref_wilkinson_x[TA_TEST_REF_WILKINSON_N] = {
   1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };

const double ta_test_ref_wilkinson_round[TA_TEST_REF_WILKINSON_N] = {
   0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5 };

const double ta_test_ref_wilkinson_big[TA_TEST_REF_WILKINSON_N] = {
   99999991.0, 99999992.0, 99999993.0, 99999994.0, 99999995.0,
   99999996.0, 99999997.0, 99999998.0, 99999999.0 };

const double ta_test_ref_wilkinson_little[TA_TEST_REF_WILKINSON_N] = {
   0.99999991, 0.99999992, 0.99999993, 0.99999994, 0.99999995,
   0.99999996, 0.99999997, 0.99999998, 0.99999999 };

const double ta_test_ref_wilkinson_huge[TA_TEST_REF_WILKINSON_N] = {
   1e12, 2e12, 3e12, 4e12, 5e12, 6e12, 7e12, 8e12, 9e12 };

const double ta_test_ref_wilkinson_tiny[TA_TEST_REF_WILKINSON_N] = {
   1e-12, 2e-12, 3e-12, 4e-12, 5e-12, 6e-12, 7e-12, 8e-12, 9e-12 };

const double ta_test_ref_wilkinson_zero[TA_TEST_REF_WILKINSON_N] = {
   0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

const double ta_test_ref_pd_outlier_x[TA_TEST_REF_PD_OUTLIER_N] = {
   3.0, 3.0, 7.0, 9.0, 3.0, 3.0, 3.8e12, 3.0, 8.0,
   2.0, 2.0, 8.0, 6.0, 7.0, 7.0, 3.0, 8.0, 4.0 };

const double ta_test_ref_pd_outlier_y[TA_TEST_REF_PD_OUTLIER_N] = {
   6.0, 3.0, 3.0, 5.0, 9.0, 1.0, 2.0, 9.0, 2.0,
   1.0, 4.0, 6.0, 6.0, 9.0, 7.0, 9.0, 3.0, 5.0 };

const double ta_test_ref_pd_shared_x[TA_TEST_REF_PD_SHARED_N] = {
   1.0, 2.0, 4.0, 7.0, 3.0, 5.0, 9.0, 2.0, 6.0, 8.0 };

const double ta_test_ref_pd_shared_y[TA_TEST_REF_PD_SHARED_N] = {
   2.0, 1.0, 5.0, 3.0, 8.0, 4.0, 7.0, 9.0, 1.0, 6.0 };

const double ta_test_ref_pd_nonan_x[TA_TEST_REF_PD_NONAN_N] = {
   1.0, 2.0, 1.0e12, 1.0e7, 6.0, 5.0, 8.0, 7.0 };

const double ta_test_ref_pd_nonan_y[TA_TEST_REF_PD_NONAN_N] = {
   2.0, 1.0, 3.0, -1.0e12, 4.0, 7.0, 6.0, 9.0 };

const double ta_test_ref_pd_extreme_x[TA_TEST_REF_PD_EXTREME_N] = {
   3.0e37, 1.0, 2.0, 3.0, -3.0e37, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };

const double ta_test_ref_pd_extreme_y[TA_TEST_REF_PD_EXTREME_N] = {
   0.2, 0.1, 0.5, 0.3, 0.8, 0.4, 0.9, 0.6, 0.7, 0.2, 0.5 };

const double ta_test_ref_pd_var47721[TA_TEST_REF_PD_VAR47721_N] = {
   1.0, -1.0, 0.0, 1.0, 3.0, 2.0, -2.0, 10000000000.0,
   1.0, 2.0, 0.0, -2.0, 1.0, 3.0, 0.0, 1.0 };

const double ta_test_ref_pd_var52407[TA_TEST_REF_PD_VAR52407_N] = {
   0.0, 0.0, 3.16188252e-18, 2.95781651e-16, 2.23153542e-51,
   0.0, 0.0, 5.39943432e-48, 1.38206260e-73, 0.0 };

const double ta_test_ref_ladder[TA_TEST_REF_LADDER_N] = {
   99999999.97, 100000000.03, 100000000.02, 100000000.01,
   100000000.0, 99999999.99, 99999999.98, 99999999.97,
   100000000.03, 100000000.02, 100000000.01, 100000000.0,
   120000000.0,
   99999999.98, 99999999.97, 100000000.03, 100000000.02,
   100000000.01, 100000000.0, 99999999.99, 99999999.98,
   99999999.97, 100000000.03, 100000000.02,
   10000010.0, 9999980.0, 10000000.0, 10000020.0, 9999990.0,
   10000010.0, 9999980.0, 10000000.0, 10000020.0,
   10000000.0, 10000000.0, 10000000.0, 10000000.0,
   10000000.0, 10000000.0, 10000000.0 };

/* --- END GENERATOR-PARSED DATASETS --- */

/* Not part of the generator-parsed block: it is an int array, and no golden is
 * derived from it -- the tests that use it multiply it by a tick size first. */
const int ta_test_ref_ticks60[TA_TEST_REF_TICKS60_N] = {
   197, 200, 199, 197, 198, 196, 194, 192, 195, 193, 195, 193,
   192, 194, 197, 194, 192, 189, 186, 183, 185, 186, 185, 182,
   180, 182, 180, 179, 180, 180, 182, 182, 185, 188, 188, 189,
   191, 193, 190, 192, 195, 193, 191, 190, 188, 185, 182, 182,
   182, 179, 179, 179, 176, 173, 172, 170, 167, 166, 169, 170 };

const double *const ta_test_ref_wilkinson_series[TA_TEST_REF_WILKINSON_NB_SERIES] = {
   ta_test_ref_wilkinson_x,      ta_test_ref_wilkinson_round,
   ta_test_ref_wilkinson_big,    ta_test_ref_wilkinson_little,
   ta_test_ref_wilkinson_huge,   ta_test_ref_wilkinson_tiny,
   ta_test_ref_wilkinson_zero };

const char *const ta_test_ref_wilkinson_names[TA_TEST_REF_WILKINSON_NB_SERIES] = {
   "X", "ROUND", "BIG", "LITTLE", "HUGE", "TINY", "ZERO" };

/**** Global functions definitions.   ****/

/* ===========================================================================
 * Double-double primitives.
 *
 * two_sum and quick_two_sum are exact for any IEEE-754 double addition.
 * two_prod is exact because it uses the C99 fma() FUNCTION -- a single
 * correctly-rounded operation, not something the optimiser is free to
 * synthesise or to withhold. That is the whole reason this file does not use
 * Dekker's splitting: splitting's error term is itself an a*b-c expression and
 * would be at the mercy of -ffp-contract.
 * ========================================================================= */

/* Requires |a| >= |b|. */
static ta_dd dd_quick_two_sum( double a, double b )
{
   ta_dd r;
   r.hi = a + b;
   r.lo = b - ( r.hi - a );
   return r;
}

static ta_dd dd_two_sum( double a, double b )
{
   ta_dd r;
   double bb;
   r.hi = a + b;
   bb   = r.hi - a;
   r.lo = ( a - ( r.hi - bb ) ) + ( b - bb );
   return r;
}

static ta_dd dd_two_prod( double a, double b )
{
   ta_dd r;
   r.hi = a * b;
   r.lo = fma( a, b, -r.hi );
   return r;
}

static ta_dd dd_zero( void )
{
   ta_dd r;
   r.hi = 0.0;
   r.lo = 0.0;
   return r;
}

static ta_dd dd_of( double a )
{
   ta_dd r;
   r.hi = a;
   r.lo = 0.0;
   return r;
}

static ta_dd dd_add( ta_dd a, ta_dd b )
{
   ta_dd s = dd_two_sum( a.hi, b.hi );
   ta_dd t = dd_two_sum( a.lo, b.lo );
   s.lo += t.hi;
   s = dd_quick_two_sum( s.hi, s.lo );
   s.lo += t.lo;
   return dd_quick_two_sum( s.hi, s.lo );
}

static ta_dd dd_add_d( ta_dd a, double b )
{
   ta_dd s = dd_two_sum( a.hi, b );
   s.lo += a.lo;
   return dd_quick_two_sum( s.hi, s.lo );
}

static ta_dd dd_neg( ta_dd a )
{
   ta_dd r;
   r.hi = -a.hi;
   r.lo = -a.lo;
   return r;
}

static ta_dd dd_sub( ta_dd a, ta_dd b )
{
   return dd_add( a, dd_neg( b ) );
}

static ta_dd dd_mul( ta_dd a, ta_dd b )
{
   ta_dd p = dd_two_prod( a.hi, b.hi );
   p.lo += a.hi * b.lo + a.lo * b.hi;
   return dd_quick_two_sum( p.hi, p.lo );
}

static ta_dd dd_mul_d( ta_dd a, double b )
{
   ta_dd p = dd_two_prod( a.hi, b );
   p.lo += a.lo * b;
   return dd_quick_two_sum( p.hi, p.lo );
}

static ta_dd dd_div_d( ta_dd a, double b )
{
   double q1, q2;
   ta_dd r;
   q1 = a.hi / b;
   r  = dd_sub( a, dd_two_prod( q1, b ) );
   q2 = ( r.hi + r.lo ) / b;
   return dd_quick_two_sum( q1, q2 );
}

static ta_dd dd_div( ta_dd a, ta_dd b )
{
   double q1, q2, q3;
   ta_dd r;
   q1 = a.hi / b.hi;
   r  = dd_sub( a, dd_mul_d( b, q1 ) );
   q2 = r.hi / b.hi;
   r  = dd_sub( r, dd_mul_d( b, q2 ) );
   q3 = r.hi / b.hi;
   r  = dd_quick_two_sum( q1, q2 );
   return dd_add_d( r, q3 );
}

/* One Newton step from the double square root, which is already accurate to
 * ~1e-16, so the step lands at full double-double accuracy. */
static ta_dd dd_sqrt( ta_dd a )
{
   double s;
   ta_dd t;
   if( a.hi <= 0.0 ) return dd_zero();
   s = sqrt( a.hi );
   t = dd_add_d( dd_div_d( a, s ), s );
   t.hi *= 0.5;
   t.lo *= 0.5;
   return t;
}

/* ===========================================================================
 * Oracles.
 *
 * All of them use the SCALED-DEVIATION form: instead of mean = Sum/n followed
 * by (x - mean), they accumulate Sum in double-double and use
 *
 *     d_i = n*x_i - Sum        (n*x_i is EXACT, via two_prod)
 *
 * which is n times the deviation. Every scale factor cancels in a correlation
 * and in a slope, and a variance divides by n^3 once at the end. This buys two
 * things: no division sits inside the accumulation loop, and a window of
 * bit-identical values gives d_i EXACTLY zero -- the oracle cannot invent a
 * rounding residue where TA-Lib's contract promises a bit-zero. (With
 * `long double` the same property held only by the accident that (n*x)/n is
 * correctly rounded; the constant-window check below states it outright rather
 * than relying on either.)
 * ========================================================================= */

int ta_test_ref_window_is_constant( const double *v, int s, int period )
{
   int j;
   for( j = 1; j < period; j++ ) if( v[s+j] != v[s] ) return 0;
   return 1;
}

double ta_test_ref_var( const double *x, int s, int period, double *outMean )
{
   ta_dd sum, acc, d;
   double n = (double)period;
   int j;

   if( period <= 0 )
   {
      if( outMean ) *outMean = 0.0;
      return 0.0;
   }

   sum = dd_zero();
   for( j = 0; j < period; j++ ) sum = dd_add_d( sum, x[s+j] );
   if( outMean ) *outMean = dd_div_d( sum, n ).hi;

   if( ta_test_ref_window_is_constant( x, s, period ) ) return 0.0;

   acc = dd_zero();
   for( j = 0; j < period; j++ )
   {
      d   = dd_sub( dd_two_prod( x[s+j], n ), sum );
      acc = dd_add( acc, dd_mul( d, d ) );
   }
   return dd_div_d( acc, n * n * n ).hi;
}

double ta_test_ref_stddev( const double *x, int s, int period, double *outMean )
{
   ta_dd sum, acc, d;
   double n = (double)period;
   int j;

   if( period <= 0 )
   {
      if( outMean ) *outMean = 0.0;
      return 0.0;
   }

   sum = dd_zero();
   for( j = 0; j < period; j++ ) sum = dd_add_d( sum, x[s+j] );
   if( outMean ) *outMean = dd_div_d( sum, n ).hi;

   if( ta_test_ref_window_is_constant( x, s, period ) ) return 0.0;

   acc = dd_zero();
   for( j = 0; j < period; j++ )
   {
      d   = dd_sub( dd_two_prod( x[s+j], n ), sum );
      acc = dd_add( acc, dd_mul( d, d ) );
   }
   return dd_sqrt( dd_div_d( acc, n * n * n ) ).hi;
}

double ta_test_ref_corr( const double *x, const double *y, int s, int period )
{
   ta_dd sx, sy, sxx, syy, sxy, dx, dy;
   double n = (double)period, r;
   int j;

   if( period <= 0 ) return 0.0;
   if( ta_test_ref_window_is_constant( x, s, period ) ) return 0.0;
   if( ta_test_ref_window_is_constant( y, s, period ) ) return 0.0;

   sx = dd_zero();
   sy = dd_zero();
   for( j = 0; j < period; j++ )
   {
      sx = dd_add_d( sx, x[s+j] );
      sy = dd_add_d( sy, y[s+j] );
   }

   sxx = dd_zero();
   syy = dd_zero();
   sxy = dd_zero();
   for( j = 0; j < period; j++ )
   {
      dx  = dd_sub( dd_two_prod( x[s+j], n ), sx );
      dy  = dd_sub( dd_two_prod( y[s+j], n ), sy );
      sxx = dd_add( sxx, dd_mul( dx, dx ) );
      syy = dd_add( syy, dd_mul( dy, dy ) );
      sxy = dd_add( sxy, dd_mul( dx, dy ) );
   }

   if( sxx.hi <= 0.0 || syy.hi <= 0.0 ) return 0.0;

   /* sqrt(sxx)*sqrt(syy), not sqrt(sxx*syy). The product form overflows once
    * sxx*syy passes ~1.8e308, and NO IN-DOMAIN INPUT REACHES THAT: at the
    * declared bound of 3e37 and the largest period this library accepts, sxx
    * tops out near 9e89, so the product has ~128 decades of headroom. Two roots
    * are kept anyway because they cost one extra sqrt in a test oracle and
    * remove the question -- not because the extreme-range dataset is close to
    * the edge. It is not. */
   r = dd_div( sxy, dd_mul( dd_sqrt( sxx ), dd_sqrt( syy ) ) ).hi;
   if( r >  1.0 ) r =  1.0;
   if( r < -1.0 ) r = -1.0;
   return r;
}

double ta_test_ref_slope( const double *x, const double *y, int s, int period,
                          double *outKappa )
{
   ta_dd sx, sy, sxx, sxy, dx, dy;
   double n = (double)period, peak = 0.0, a;
   int j;

   if( outKappa ) *outKappa = 0.0;
   if( period <= 0 ) return 0.0;

   sx = dd_zero();
   sy = dd_zero();
   for( j = 0; j < period; j++ )
   {
      sx = dd_add_d( sx, x[s+j] );
      sy = dd_add_d( sy, y[s+j] );
      a  = fabs( x[s+j] );
      if( a > peak ) peak = a;
   }

   if( ta_test_ref_window_is_constant( x, s, period ) ) return 0.0;

   sxx = dd_zero();
   sxy = dd_zero();
   for( j = 0; j < period; j++ )
   {
      dx  = dd_sub( dd_two_prod( x[s+j], n ), sx );
      dy  = dd_sub( dd_two_prod( y[s+j], n ), sy );
      sxx = dd_add( sxx, dd_mul( dx, dx ) );
      sxy = dd_add( sxy, dd_mul( dx, dy ) );
   }

   if( sxx.hi <= 0.0 ) return 0.0;

   /* rms deviation is sqrt(sxx / n^3): sxx carries the n^2 the scaled form put
    * on every deviation, and the population divisor is the third n. */
   if( outKappa )
      *outKappa = peak / dd_sqrt( dd_div_d( sxx, n * n * n ) ).hi;

   return dd_div( sxy, sxx ).hi;
}

void ta_test_ref_linreg( const double *y, int s, int period,
                         double *outSlope, double *outIntercept,
                         double *outFit, double *outForecast )
{
   ta_dd sy, sxx, sxy, dx, dy, slope, ybar, half;
   double n = (double)period;
   double sumX;
   int j;

   if( outSlope )     *outSlope     = 0.0;
   if( outIntercept ) *outIntercept = 0.0;
   if( outFit )       *outFit       = 0.0;
   if( outForecast )  *outForecast  = 0.0;
   if( period <= 0 ) return;

   /* x = 0 .. period-1, so SumX = n(n-1)/2 -- an exact integer for any period
    * this library accepts, and n*x_j is an exact integer too. Every deviation
    * dx below is therefore exact, and all the conditioning lives on the y
    * side, where it belongs. */
   sumX = n * ( n - 1.0 ) * 0.5;

   sy = dd_zero();
   for( j = 0; j < period; j++ ) sy = dd_add_d( sy, y[s+j] );
   ybar = dd_div_d( sy, n );

   if( period == 1 )
   {
      /* One point: the fit is the point, the slope is undefined and TA-Lib's
       * divisor is zero. Nothing here is a claim about what the library does
       * at period 1 -- callers pin that separately. */
      if( outIntercept ) *outIntercept = y[s];
      if( outFit )       *outFit       = y[s];
      if( outForecast )  *outForecast  = y[s];
      return;
   }

   sxx = dd_zero();
   sxy = dd_zero();
   for( j = 0; j < period; j++ )
   {
      dx  = dd_add_d( dd_of( n * (double)j ), -sumX );
      dy  = dd_sub( dd_two_prod( y[s+j], n ), sy );
      sxx = dd_add( sxx, dd_mul( dx, dx ) );
      sxy = dd_add( sxy, dd_mul( dx, dy ) );
   }

   slope = dd_div( sxy, sxx );
   half  = dd_mul_d( slope, ( n - 1.0 ) * 0.5 );

   if( outSlope )     *outSlope     = slope.hi;
   if( outIntercept ) *outIntercept = dd_sub( ybar, half ).hi;
   if( outFit )       *outFit       = dd_add( ybar, half ).hi;
   if( outForecast )  *outForecast  = dd_add( ybar, dd_mul_d( slope, ( n + 1.0 ) * 0.5 ) ).hi;

}

/* ===========================================================================
 * One random-number generator.
 * ========================================================================= */

void ta_test_ref_lcg_seed( unsigned int seed )
{
   ta_test_ref_lcg_state = seed;
}

static unsigned int ta_test_ref_lcg_next( void )
{
   ta_test_ref_lcg_state = ta_test_ref_lcg_state * 1103515245u + 12345u;
   return ( ta_test_ref_lcg_state >> 8 ) & 0xffffffu;
}

double ta_test_ref_lcg_sym( void )
{
   return ( (double)ta_test_ref_lcg_next() / 8388608.0 ) - 1.0;
}

double ta_test_ref_lcg_half( void )
{
   return ( (double)ta_test_ref_lcg_next() / 16777216.0 ) - 0.5;
}

void ta_test_ref_xorshift_seed( unsigned int seed )
{
   ta_test_ref_xorshift_state = seed;
}

double ta_test_ref_xorshift_unit( void )
{
   unsigned int s = ta_test_ref_xorshift_state;
   s ^= s << 13;
   s ^= s >> 17;
   s ^= s << 5;
   ta_test_ref_xorshift_state = s;
   return (double)( ( s >> 8 ) & 0xffffu ) / 65535.0;
}

/* ===========================================================================
 * NIST StRD univariate stressors, built rather than transcribed.
 * ========================================================================= */

int ta_test_ref_numacc( int which, double *buf )
{
   static const double bases[4] = { 1.0e7, 0.0, 999999.0, 9999999.0 };
   double base;
   int idx;

   if( which < 1 || which > 4 ) return 0;
   base = bases[which-1];

   if( which == 1 )
   {
      buf[0] = base + 1.0;
      buf[1] = base + 2.0;
      buf[2] = base + 3.0;
      return 3;
   }

   /* Emitted as base+1.2 first, then 500 alternating pairs of base+1.1 and
    * base+1.3 -- NIST describes the same multiset in a different order, which
    * changes nothing here: order matters only for an autocorrelation, which
    * NIST does not certify, and the variance is a property of the multiset. */
   idx = 0;
   buf[idx++] = base + 1.2;
   while( idx < 1001 ) { buf[idx++] = base + 1.1; buf[idx++] = base + 1.3; }
   return 1001;
}
