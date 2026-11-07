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
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  110626 MF,CC  First version (issue #571).
 *
 */

/* Description:
 *     Test TA_SHARPE, the rolling Sharpe ratio over simple returns.
 *
 *     Legs:
 *       oracle     frozen empyrical-reloaded and quantstats rows on the
 *                  252-bar close series
 *       shapes     every window of hand-built series against a two-pass
 *                  recomputation, and every window whose returns are all equal
 *                  against the ruled plain quotient (+/-Inf, NaN)
 *       identity   annualization is a post-scale by sqrt(A), bit for bit; the
 *                  risk-free rate moves only the mean
 *       alias      outReal == inReal, bitwise
 *       contract   lookback, the parameter ranges
 *       range      doRangeTestEx
 *
 *     WHAT EACH LEG CAN SEE, measured by mutating the generator input and
 *     regenerating; every leg named goes red first.
 *
 *       population divisor, A for sqrt(A)          shapes
 *       rate subtracted twice                      shapes
 *       x/x[-1] - 1 for (x - x[-1])/x[-1]          shapes (equal returns)
 *       no zero-base guard                         shapes (zero price)
 *       no spread trigger, no re-anchor            shapes (halving at 49)
 *       trigger at 1e-4 or 1e-6 of the peak        shapes (equal returns)
 *       trigger against the current sum            shapes (spike then flat)
 *       output written before the trailing read    alias
 *       mean without the shift                     shapes
 *       lookback n-1                               contract
 *
 *     No leg sees the periodic rebuild dropped: it bounds rounding carried
 *     over windows that never trip the trigger, below 1e-13 on these series.
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
#define SHARPE_CAP 3200

#define SHARPE_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, inArr, period, rf, ann, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_value_comparisons();                         \
         ErrorNumber svErr_ = server_verify(                                     \
            "SHARPE", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),               \
            (const TA_Real*[]){ (inArr), NULL },                                 \
            (double[]){ (double)(period), (rf), (ann) }, 3,                      \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         if( server_verify_value_comparisons() == svCmp_ )                       \
         {                                                                       \
            printf( "SHARPE [period %d]: server_verify compared no server "      \
                    "despite live pipes\n", (int)(period) );                     \
            return TA_SHARPE_VACUOUS;                                            \
         }                                                                       \
      }                                                                          \
   } while(0)

/* The ratio crosses zero, where a relative distance is unbounded. */
#define SHARPE_DIST(got, want) (fabs( (got) - (want) ) / fmax( 1.0, fabs( want ) ))

/* Against the two-pass recomputation below: the running sums carry the
 * rounding of every return that passed through the window since the last
 * rebuild, and the reference its own rounding. Measured worst between the two
 * 2.7e-14, on the long walk. */
#define SHARPE_REF_TOL 1e-13

/* Comparisons each leg makes on the shipped 252-bar corpus, pinned exactly so
 * a leg that stops comparing fails. */
#define SHARPE_COV_REF    36291
#define SHARPE_COV_POS    1205
#define SHARPE_COV_NEG    1177
#define SHARPE_COV_NAN    757
#define SHARPE_COV_ZERO   365
#define SHARPE_COV_ANN    3960
#define SHARPE_COV_RF     660
#define SHARPE_COV_ALIAS  2448
#define SHARPE_COV_ORACLE 83

static int g_sharpeRefCmp;
static int g_sharpeEqualCmp[3];
static int g_sharpeZeroBaseCmp;
static int g_sharpeAnnCmp;
static int g_sharpeRfCmp;
static int g_sharpeAliasCmp;
static int g_sharpeOracleCmp;

static ErrorNumber test_sharpe_shapes( const TA_History *history );
static ErrorNumber test_sharpe_identity( const TA_History *history );
static ErrorNumber test_sharpe_aliasing( const TA_History *history );
static ErrorNumber test_sharpe_contract( const TA_History *history );
static ErrorNumber test_sharpe_range( const TA_History *history );
static ErrorNumber test_sharpe_oracle( const TA_History *history );

ErrorNumber test_func_sharpe( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_sharpeRefCmp = g_sharpeZeroBaseCmp = 0;
   g_sharpeEqualCmp[0] = g_sharpeEqualCmp[1] = g_sharpeEqualCmp[2] = 0;
   g_sharpeAnnCmp = g_sharpeRfCmp = g_sharpeAliasCmp = g_sharpeOracleCmp = 0;

#define SHARPE_RUN(call) do { err = (call); if( err != TA_TEST_PASS ) return err; } while(0)
   SHARPE_RUN( test_sharpe_shapes( history ) );
   SHARPE_RUN( test_sharpe_identity( history ) );
   SHARPE_RUN( test_sharpe_aliasing( history ) );
   SHARPE_RUN( test_sharpe_contract( history ) );
   SHARPE_RUN( test_sharpe_range( history ) );
   SHARPE_RUN( test_sharpe_oracle( history ) );
#undef SHARPE_RUN

   if( history->nbBars == 252 )
   {
      static const struct { const char *leg; int want; const int *got; } cov[] = {
         { "two-pass",            SHARPE_COV_REF,   &g_sharpeRefCmp      },
         { "equal returns, +Inf", SHARPE_COV_POS,   &g_sharpeEqualCmp[0] },
         { "equal returns, -Inf", SHARPE_COV_NEG,   &g_sharpeEqualCmp[1] },
         { "equal returns, NaN",  SHARPE_COV_NAN,   &g_sharpeEqualCmp[2] },
         { "zero base price",     SHARPE_COV_ZERO,  &g_sharpeZeroBaseCmp },
         { "annualization",       SHARPE_COV_ANN,   &g_sharpeAnnCmp      },
         { "risk-free rate",      SHARPE_COV_RF,    &g_sharpeRfCmp       },
         { "alias",               SHARPE_COV_ALIAS, &g_sharpeAliasCmp    },
         { "oracle",              SHARPE_COV_ORACLE,&g_sharpeOracleCmp   },
      };
      unsigned int c;
      int bad = 0;

      for( c = 0; c < sizeof(cov)/sizeof(cov[0]); c++ )
      {
         if( *cov[c].got != cov[c].want )
         {
            printf( "SHARPE Fail: the %s leg compared %d times, not the %d this "
                    "file was written with\n", cov[c].leg, *cov[c].got,
                    cov[c].want );
            bad = 1;
         }
      }
      if( bad )
         return TA_SHARPE_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

static double sharpe_return( const double *in, int bar )
{
   return in[bar-1] != 0.0 ? (in[bar]-in[bar-1])/in[bar-1] : 0.0;
}

/* 1 when every return of the window ending at `bar` is the same double. */
static int sharpe_equal_window( const double *in, int bar, int period )
{
   double first = sharpe_return( in, bar-period+1 );
   int j;

   for( j = bar-period+2; j <= bar; j++ )
      if( sharpe_return( in, j ) != first )
         return 0;
   return 1;
}

/* The definition, two fresh passes over each window, so nothing carries over
 * from one window to the next. The spread is taken from the returns, which
 * is the spread of the excess returns: subtracting a rate of 1 from returns
 * near 1e-11 first would leave the deviations at the rounding of the rate.
 * Deviations are measured from the window's first return: that subtraction
 * is exact for returns within a factor of two of it, where a mean is not. */
static double sharpe_two_pass( const double *in, int bar, int period, double rf, double ann )
{
   double base = sharpe_return( in, bar-period+1 ), mean = 0.0, m2 = 0.0, d;
   int j;

   for( j = bar-period+1; j <= bar; j++ )
      mean += sharpe_return( in, j ) - base;
   mean /= (double)period;
   for( j = bar-period+1; j <= bar; j++ )
   {
      d = (sharpe_return( in, j ) - base) - mean;
      m2 += d * d;
   }
   return sqrt( ann ) * (((base + mean) - rf) / sqrt( m2 / (double)(period-1) ));
}

/* Prices as whole cents, so every price is the 2-decimal value a feed sends. */
static double sharpe_cents( long c )
{
   return (double)c / 100.0;
}

static unsigned int sharpe_lcg( unsigned int *s )
{
   *s = *s * 1664525u + 1013904223u;
   return *s >> 8;
}

enum { SHAPE_STEPS, SHAPE_EQUAL, SHAPE_SPIKE_FLAT, SHAPE_SPIKE_TINY, SHAPE_ZERO,
       SHAPE_WALK, SHAPE_COUNT };
static const char *const sharpeShapeName[SHAPE_COUNT] =
   { "doubling/halving", "equal returns", "spike then flat", "spike then tiny",
     "zero price", "long walk" };

/* The price after x whose return is exactly c, among x*(1+c) and its two
 * neighbours; 0 when none is. One price in a few hits a given return, so a
 * chain of them exists only from chosen starting prices. */
static double sharpe_next_equal( double x, double c )
{
   double y = x + x * c;

   if( (y-x)/x == c )
      return y;
   if( (nextafter( y, INFINITY )-x)/x == c )
      return nextafter( y, INFINITY );
   if( (nextafter( y, -INFINITY )-x)/x == c )
      return nextafter( y, -INFINITY );
   return 0.0;
}

/* Builds a series of SHARPE_CAP bars or fewer; returns its length. */
static int sharpe_build( int shape, double *x )
{
   unsigned int s = 12345u;
   long c = 10000;
   int i, n = 0;

   switch( shape )
   {
   case SHAPE_STEPS:
      /* Doubling, halving, flat, then alternating prices: every return is
       * exactly 1, -0.5 or 0. At period 49 the mean of the halving returns,
       * taken through the rounded 1/49, is not -0.5. */
      x[n++] = 1.0;
      for( i = 0; i < 60; i++, n++ ) x[n] = x[n-1] * 2.0;
      for( i = 0; i < 60; i++, n++ ) x[n] = x[n-1] * 0.5;
      for( i = 0; i < 40; i++, n++ ) x[n] = x[n-1];
      for( i = 0; i < 40; i++, n++ ) x[n] = (i & 1) ? x[n-1] * 2.0 : x[n-1] * 0.5;
      break;
   case SHAPE_EQUAL:
      /* Eleven returns of exactly 0.7 from 273.15, then 39 of exactly -0.6
       * from 100.02: equal returns that are not dyadic. */
      x[n++] = 273.15;
      for( i = 0; i < 11; i++, n++ )
         if( (x[n] = sharpe_next_equal( x[n-1], 0.7 )) == 0.0 )
            return 0;
      x[n++] = 100.02;
      for( i = 0; i < 39; i++, n++ )
         if( (x[n] = sharpe_next_equal( x[n-1], -0.6 )) == 0.0 )
            return 0;
      break;
   case SHAPE_SPIKE_FLAT:
   case SHAPE_SPIKE_TINY:
      for( i = 0; i < 400; i++ )
      {
         if( i == 60 )
            c = c * 40;
         else if( i == 61 )
            c = c / 40;
         else if( i < 60 || i > 300 )
            c += (long)(sharpe_lcg( &s ) % 41) - 20;
         if( shape == SHAPE_SPIKE_TINY && i > 61 && i <= 300 )
            x[n++] = sharpe_cents( c ) + (double)(i % 7) * 1e-9;
         else
            x[n++] = sharpe_cents( c );
      }
      break;
   case SHAPE_ZERO:
      for( i = 0; i < 200; i++ )
      {
         c += (long)(sharpe_lcg( &s ) % 41) - 20;
         x[n++] = (i == 50 || i == 120 || i == 121) ? 0.0 : sharpe_cents( c );
      }
      break;
   case SHAPE_WALK:
      for( i = 0; i < SHARPE_CAP; i++ )
      {
         c += (long)(sharpe_lcg( &s ) % 201) - 100;
         if( c < 100 )
            c = 100;
         x[n++] = sharpe_cents( c );
      }
      break;
   }
   return n;
}

/* Every window of one call: an all-equal window is the plain quotient, every
 * other one the two-pass value. */
static ErrorNumber sharpe_check_series( const char *tag, const double *in, int nbBars,
                                        int period, double rf, double ann, int route )
{
   static TA_Real out[SHARPE_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   double want, mean;
   int i, bar, j;

   retCode = TA_SHARPE( 0, nbBars-1, in, period, rf, ann, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != period || nbElement != nbBars-period )
   {
      printf( "SHARPE %s Fail [N=%d]: rc=%d (%d,%d)\n", tag, period,
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   if( route )
      SHARPE_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                            in, period, rf, ann, out );

   for( i = 0; i < nbElement; i++ )
   {
      bar = begIdx + i;

      for( j = bar-period; j <= bar-1; j++ )
         if( in[j] == 0.0 )
         {
            g_sharpeZeroBaseCmp++;
            break;
         }

      if( sharpe_equal_window( in, bar, period ) )
      {
         mean = sharpe_return( in, bar ) - rf;
         want = mean == 0.0 ? NAN : (mean > 0.0 ? INFINITY : -INFINITY);
         if( ann == 0.0 )
            want = NAN;
         g_sharpeEqualCmp[isnan( want ) ? 2 : (want > 0.0 ? 0 : 1)]++;
         if( isnan( want ) ? !isnan( out[i] ) : out[i] != want )
         {
            printf( "SHARPE %s Fail [N=%d rf=%g A=%g] bar %d: %.17g on a window "
                    "of equal returns %.17g, expected %g\n", tag, period, rf, ann,
                    bar, out[i], sharpe_return( in, bar ), want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         continue;
      }

      want = sharpe_two_pass( in, bar, period, rf, ann );
      g_sharpeRefCmp++;
      if( !(SHARPE_DIST( out[i], want ) <= SHARPE_REF_TOL) )
      {
         printf( "SHARPE %s Fail [N=%d rf=%g A=%g] bar %d: %.17g, two-pass %.17g, "
                 "distance %.3e over %.0e\n", tag, period, rf, ann, bar, out[i],
                 want, SHARPE_DIST( out[i], want ), SHARPE_REF_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* The risk-free rate is chosen per shape so that some all-equal window has an
 * excess of each sign and of exactly zero: the doubling windows return 1, the
 * halving ones -0.5, the flat ones 0, the equal-return chains 0.7 and -0.6. */
static ErrorNumber test_sharpe_shapes( const TA_History *history )
{
   static double x[SHARPE_CAP];
   static const struct { int period; double rf; double ann; } vec[] = {
      {  2,  0.0,    252.0 },
      {  5,  1.0,      1.0 },
      { 20, -0.5,     52.0 },
      { 20,  0.0001, 252.0 },
      { 37, -0.0001,  12.0 },
      { 20,  0.0,      0.0 },
      {  5,  0.7,      1.0 },
      { 49, -0.5,      1.0 },
      { 20, -0.6,    252.0 },
   };
   ErrorNumber err;
   int shape, v, nbBars;

   for( shape = 0; shape < SHAPE_COUNT; shape++ )
   {
      nbBars = sharpe_build( shape, x );
      if( nbBars == 0 )
      {
         printf( "SHARPE Fail: no series of equal returns could be built\n" );
         return TA_SHARPE_VACUOUS;
      }
      for( v = 0; v < (int)(sizeof(vec)/sizeof(vec[0])); v++ )
      {
         err = sharpe_check_series( sharpeShapeName[shape], x, nbBars, vec[v].period,
                                    vec[v].rf, vec[v].ann, 1 );
         if( err != TA_TEST_PASS )
            return err;
      }
   }

   err = sharpe_check_series( "close", history->close, (int)history->nbBars,
                              20, 0.0, 252.0, 0 );
   if( err != TA_TEST_PASS )
      return err;
   return sharpe_check_series( "close", history->close, (int)history->nbBars,
                               63, 0.0002, 12.0, 1 );
}

/* sqrt(A) multiplies the per-bar ratio last, so every annualization is that
 * ratio times sqrt(A) bit for bit. With the variance taken from the returns,
 * the rate moves only the mean: at A = 1, out(0) - out(rf) is rf / sd for
 * every rf, and sd is the same for both rates. */
static ErrorNumber test_sharpe_identity( const TA_History *history )
{
   static TA_Real raw[SHARPE_CAP], out[SHARPE_CAP], outB[SHARPE_CAP];
   static const double anns[] = { 2.0, 12.0, 52.0, 252.0, 365.25, 1638.0 };
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, a, i;
   double scaled, sdA, sdB;

   for( n = 2; n <= 62; n += 30 )
   {
      retCode = TA_SHARPE( 0, nbBars-1, history->close, n, 0.0, 1.0,
                           &begIdx, &nbElement, raw );
      if( retCode != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      for( a = 0; a < (int)(sizeof(anns)/sizeof(anns[0])); a++ )
      {
         retCode = TA_SHARPE( 0, nbBars-1, history->close, n, 0.0, anns[a],
                              &begIdx2, &nbElement2, out );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         for( i = 0; i < nbElement; i++ )
         {
            scaled = sqrt( anns[a] ) * raw[i];
            g_sharpeAnnCmp++;
            if( memcmp( &scaled, &out[i], sizeof(double) ) != 0 || out[i] == raw[i] )
            {
               printf( "SHARPE annualization Fail [N=%d A=%g] bar %d: %.17g, "
                       "sqrt(A) times the A=1 value %.17g is %.17g\n", n, anns[a],
                       begIdx+i, out[i], raw[i], scaled );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }

      retCode = TA_SHARPE( 0, nbBars-1, history->close, n, 0.0001, 1.0,
                           &begIdx2, &nbElement2, out );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      retCode = TA_SHARPE( 0, nbBars-1, history->close, n, -0.0003, 1.0,
                           &begIdx2, &nbElement2, outB );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      for( i = 0; i < nbElement; i++ )
      {
         sdA = 0.0001 / (raw[i] - out[i]);
         sdB = -0.0003 / (raw[i] - outB[i]);
         g_sharpeRfCmp++;
         if( !(sdA > 0.0) || !(fabs( sdA - sdB ) <= 1e-9 * sdA) )
         {
            printf( "SHARPE risk-free Fail [N=%d] bar %d: the rate moved the "
                    "spread, %.17g at 0.0001 and %.17g at -0.0003\n", n,
                    begIdx+i, sdA, sdB );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* outReal == inReal, bitwise: the trailing price of a window sits in the slot
 * the output of its bar is written to. */
static ErrorNumber test_sharpe_aliasing( const TA_History *history )
{
   static double x[SHARPE_CAP], clean[SHARPE_CAP], alias[SHARPE_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   const double *src;
   int nbBars, n, i, pass;

   for( pass = 0; pass < 2; pass++ )
   {
      if( pass == 0 )
      {
         src = history->close;
         nbBars = (int)history->nbBars;
      }
      else
      {
         nbBars = sharpe_build( SHAPE_SPIKE_FLAT, x );
         src = x;
      }

      for( n = 2; n <= 38; n += 12 )
      {
         retCode = TA_SHARPE( 0, nbBars-1, src, n, 0.0001, 252.0,
                              &begIdx, &nbElement, clean );
         if( retCode != TA_SUCCESS )
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         for( i = 0; i < nbBars; i++ )
            alias[i] = src[i];
         retCode = TA_SHARPE( 0, nbBars-1, alias, n, 0.0001, 252.0,
                              &begIdx2, &nbElement2, alias );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "SHARPE alias Fail [N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                    n, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_sharpeAliasCmp++;
            if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
            {
               printf( "SHARPE alias Fail [N=%d] bar %d: separate %.17g, in-place "
                       "%.17g\n", n, begIdx+i, clean[i], alias[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_sharpe_contract( const TA_History *history )
{
   static double out[SHARPE_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n;

   for( n = -1; n <= 1; n++ )
   {
      retCode = TA_SHARPE( 0, nbBars-1, history->close, n, 0.0, 252.0,
                           &begIdx, &nbElement, out );
      if( retCode != TA_BAD_PARAM || TA_SHARPE_Lookback( n, 0.0, 252.0 ) != -1 )
      {
         printf( "SHARPE contract Fail: period %d gave rc=%d, expected "
                 "TA_BAD_PARAM -- the sample spread needs two returns\n",
                 n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }
   if( TA_SHARPE_Lookback( 100001, 0.0, 252.0 ) != -1
       || TA_SHARPE_Lookback( 100000, 0.0, 252.0 ) != 100000 )
   {
      printf( "SHARPE contract Fail: lookback at the period's upper bound\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   retCode = TA_SHARPE( 0, nbBars-1, history->close, 20, 0.0, -1.0,
                        &begIdx, &nbElement, out );
   if( retCode != TA_BAD_PARAM )
   {
      printf( "SHARPE contract Fail: annualization -1 gave rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( n = 2; n <= 50; n += 16 )
   {
      if( TA_SHARPE_Lookback( n, -0.01, 1.0 ) != n )
      {
         printf( "SHARPE contract Fail: lookback(%d) = %d\n", n,
                 TA_SHARPE_Lookback( n, -0.01, 1.0 ) );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      retCode = TA_SHARPE( 0, nbBars-1, history->close, n, -0.01, 1.0,
                           &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n || nbElement != nbBars-n )
      {
         printf( "SHARPE contract Fail [N=%d]: rc=%d (%d,%d)\n", n, (int)retCode,
                 begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }

   return TA_TEST_PASS;
}

/* TA_STABLE_EPSILON: the shift and the rebuild phase are seeded from each
 * call's own startIdx. */
typedef struct { int period; double rf; double ann; const TA_Real *in; } SharpeRangeParam;

static TA_RetCode sharpeRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                           TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                           TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                           TA_Integer *lookback, void *opaqueData,
                                           unsigned int outputNb, unsigned int *isOutputInteger )
{
   SharpeRangeParam *p = (SharpeRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_SHARPE_Lookback( p->period, p->rf, p->ann );
   return TA_SHARPE( startIdx, endIdx, p->in, p->period, p->rf, p->ann,
                     outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_sharpe_range( const TA_History *history )
{
   SharpeRangeParam param;

   param.period = 20;
   param.rf     = 0.0001;
   param.ann    = 252.0;
   param.in     = history->close;

   return doRangeTestEx( sharpeRangeTestFunction, TA_STABLE_EPSILON,
                         TA_TEST_UNST_NONE, (void *)&param, 1, 0 );
}

/* Frozen rows of ta-lib-oracles capture_571_sharpe.py (oracles d172432), run on
 * the returns of the 252-bar close series. Sources: empyrical-reloaded 0.5.12
 * roll_sharpe_ratio, and quantstats 0.0.81 rolling_sharpe where the rate is 0
 * (quantstats de-annualizes a non-zero rate itself). The capture checks every
 * empyrical row against the ratio in exact rational arithmetic; this file
 * holds TA_SHARPE to empyrical, to the exact value, and to quantstats where
 * the row has one.
 */
#define SHARPE_ORACLE_TOL 1e-12
#define NO_ARM NAN

typedef struct { int period; double rf; double ann; int bar;
                 double exact; double empyrical; double quantstats; } SharpeGolden;

static const SharpeGolden sharpeGoldens[] =
{
   /* n=20 rf=0 A=252: outBegIdx 20, outNBElement 232 */
   {  20,       0.0, 252.0,  20,      -0.7133756997172479,     -0.71337569971724801,      -0.7133756997172479 },
   {  20,       0.0, 252.0,  21,      -2.0163825257222237,      -2.0163825257222237,      -2.0163825257222237 },
   {  20,       0.0, 252.0, 100,       3.4999470029448054,       3.4999470029448063,       3.4999470029448045 },
   {  20,       0.0, 252.0, 130,       10.010207199367935,       10.010207199367935,       10.010207199367935 },
   {  20,       0.0, 252.0, 182,     0.035631583258193759,     0.035631583258193128,     0.035631583258194058 },
   {  20,       0.0, 252.0, 185,      0.25393028617061436,      0.25393028617061536,      0.25393028617061453 },
   {  20,       0.0, 252.0, 189,      -2.4647718296335634,      -2.4647718296335634,      -2.4647718296335612 },
   {  20,       0.0, 252.0, 196,      -7.6210991513803918,      -7.6210991513803936,      -7.6210991513803874 },
   {  20,       0.0, 252.0, 200,      -6.3590742612692015,      -6.3590742612691997,      -6.3590742612691988 },
   {  20,       0.0, 252.0, 251,        1.002980829048844,       1.0029808290488447,        1.002980829048844 },
   /* n=20 rf=0 A=1: outBegIdx 20, outNBElement 232 */
   {  20,       0.0,   1.0,  20,    -0.044938445066869719,    -0.044938445066869726,    -0.044938445066869719 },
   {  20,       0.0,   1.0,  21,     -0.12702015978660247,     -0.12702015978660247,     -0.12702015978660247 },
   {  20,       0.0,   1.0, 100,      0.22047593742137628,      0.22047593742137631,      0.22047593742137619 },
   {  20,       0.0,   1.0, 130,      0.63058378147037897,      0.63058378147037897,      0.63058378147037897 },
   {  20,       0.0,   1.0, 135,      0.39135801590169966,      0.39135801590169983,       0.3913580159016995 },
   {  20,       0.0,   1.0, 182,    0.0022445787647779347,    0.0022445787647778948,    0.0022445787647779533 },
   {  20,       0.0,   1.0, 196,     -0.48008412075042645,      -0.4800841207504265,     -0.48008412075042611 },
   {  20,       0.0,   1.0, 200,     -0.40058402533119242,     -0.40058402533119231,     -0.40058402533119225 },
   {  20,       0.0,   1.0, 251,     0.063181853414967362,      0.06318185341496739,     0.063181853414967348 },
   /* n=20 rf=0.0002 A=252: outBegIdx 20, outNBElement 232 */
   {  20,    0.0002, 252.0,  20,     -0.82168901535514138,     -0.82168901535514161,                   NO_ARM },
   {  20,    0.0002, 252.0,  21,      -2.1298840818651761,      -2.1298840818651765,                   NO_ARM },
   {  20,    0.0002, 252.0, 100,       3.3950503370081648,       3.3950503370081657,                   NO_ARM },
   {  20,    0.0002, 252.0, 130,       9.7994203195244562,       9.7994203195244562,                   NO_ARM },
   {  20,    0.0002, 252.0, 140,    -0.052181200349798597,     -0.05218120034979809,                   NO_ARM },
   {  20,    0.0002, 252.0, 185,      0.10506519771206022,      0.10506519771206115,                   NO_ARM },
   {  20,    0.0002, 252.0, 196,      -7.7587529246025886,      -7.7587529246025895,                   NO_ARM },
   {  20,    0.0002, 252.0, 200,      -6.5120081390303017,         -6.5120081390303,                   NO_ARM },
   {  20,    0.0002, 252.0, 251,      0.86732638595402556,      0.86732638595402578,                   NO_ARM },
   /* n=20 rf=-0.0001 A=52: outBegIdx 20, outNBElement 232 */
   {  20,   -0.0001,  52.0,  20,     -0.29945470780348066,     -0.29945470780348055,                   NO_ARM },
   {  20,   -0.0001,  52.0,  21,     -0.89017597353583222,     -0.89017597353583211,                   NO_ARM },
   {  20,   -0.0001,  52.0, 100,       1.6136996046944045,       1.6136996046944043,                   NO_ARM },
   {  20,   -0.0001,  52.0, 130,        4.595079997950827,       4.5950799979508261,                   NO_ARM },
   {  20,   -0.0001,  52.0, 152,      -1.5632968903554063,       -1.563296890355405,                   NO_ARM },
   {  20,   -0.0001,  52.0, 158,    -0.011391246756131131,    -0.011391246756131077,                   NO_ARM },
   {  20,   -0.0001,  52.0, 196,      -3.4306707491130317,      -3.4306707491130317,                   NO_ARM },
   {  20,   -0.0001,  52.0, 200,      -2.8539168628265168,      -2.8539168628265164,                   NO_ARM },
   {  20,   -0.0001,  52.0, 251,      0.48642179840791722,      0.48642179840791749,                   NO_ARM },
   /* n=2 rf=0 A=252: outBegIdx 2, outNBElement 250 */
   {   2,       0.0, 252.0,   2,       8.6758835144375261,       8.6758835144375261,       8.6758835144375261 },
   {   2,       0.0, 252.0,   3,       2.7340471739865473,       2.7340471739865473,       2.7340471739865473 },
   {   2,       0.0, 252.0, 100,       6.5330378246049854,       6.5330378246049863,       6.5330378246049863 },
   {   2,       0.0, 252.0, 115,        0.658598327100407,        0.658598327100407,      0.65859832710039812 },
   {   2,       0.0, 252.0, 190,       1708.7782327946436,       1708.7782327946438,       1708.7782327946438 },
   {   2,       0.0, 252.0, 192,      -836.03195175178553,      -836.03195175178553,      -836.03195175178325 },
   {   2,       0.0, 252.0, 197,       -4.683680971526587,      -4.6836809715265888,      -4.6836809715265879 },
   {   2,       0.0, 252.0, 200,      -8.4843421453064334,      -8.4843421453064334,      -8.4843421453064334 },
   {   2,       0.0, 252.0, 201,    0.0062908493332645421,    0.0062908493332645421,    0.0062908493332645421 },
   {   2,       0.0, 252.0, 251,      -20.105165099870128,      -20.105165099870128,      -20.105165099870128 },
   /* n=5 rf=0.0001 A=1: outBegIdx 5, outNBElement 247 */
   {   5,    0.0001,   1.0,   5,      0.35883140923105245,      0.35883140923105245,                   NO_ARM },
   {   5,    0.0001,   1.0,   6,     -0.36290119447125113,     -0.36290119447125113,                   NO_ARM },
   {   5,    0.0001,   1.0,  23,      -2.2697082225766163,      -2.2697082225766154,                   NO_ARM },
   {   5,    0.0001,   1.0,  59,       1.2553484074513597,       1.2553484074513597,                   NO_ARM },
   {   5,    0.0001,   1.0, 100,   -0.0069940793336305714,   -0.0069940793336305557,                   NO_ARM },
   {   5,    0.0001,   1.0, 163,  -2.6783843770812871e-05,  -2.6783843770796221e-05,                   NO_ARM },
   {   5,    0.0001,   1.0, 200,     -0.21195587945062219,     -0.21195587945062219,                   NO_ARM },
   {   5,    0.0001,   1.0, 251,     -0.19524777782239952,     -0.19524777782239949,                   NO_ARM },
   /* n=63 rf=0 A=252: outBegIdx 63, outNBElement 189 */
   {  63,       0.0, 252.0,  63,       0.1952976365471189,      0.19529763654711904,      0.19529763654711879 },
   {  63,       0.0, 252.0,  64,     0.021802684192169085,      0.02180268419216912,     0.021802684192168974 },
   {  63,       0.0, 252.0, 100,         2.92437227016539,       2.9243722701653896,         2.92437227016539 },
   {  63,       0.0, 252.0, 105,       2.4926696576910485,       2.4926696576910468,       2.4926696576910485 },
   {  63,       0.0, 252.0, 135,       4.5672892970176937,       4.5672892970176928,       4.5672892970176928 },
   {  63,       0.0, 252.0, 153,    -0.010276672522199616,    -0.010276672522199387,    -0.010276672522199631 },
   {  63,       0.0, 252.0, 189,     -0.92644360666650505,     -0.92644360666650538,     -0.92644360666650594 },
   {  63,       0.0, 252.0, 196,      -2.8136282531651902,      -2.8136282531651897,      -2.8136282531651915 },
   {  63,       0.0, 252.0, 200,      -2.0190615883044813,      -2.0190615883044818,      -2.0190615883044822 },
   {  63,       0.0, 252.0, 251,     -0.48536120782995701,     -0.48536120782995718,     -0.48536120782995745 },
   /* n=126 rf=0 A=252: outBegIdx 126, outNBElement 126 */
   { 126,       0.0, 252.0, 126,       1.8872119321497414,       1.8872119321497411,       1.8872119321497418 },
   { 126,       0.0, 252.0, 127,        1.792225212770179,       1.7922252127701792,       1.7922252127701797 },
   { 126,       0.0, 252.0, 179,       2.4349124188209061,       2.4349124188209061,       2.4349124188209057 },
   { 126,       0.0, 252.0, 200,       1.2812739086369234,        1.281273908636924,       1.2812739086369231 },
   { 126,       0.0, 252.0, 201,      0.70553465494742962,      0.70553465494743117,       0.7055346549474294 },
   { 126,       0.0, 252.0, 216,     -0.97593113453635094,     -0.97593113453635083,     -0.97593113453635061 },
   { 126,       0.0, 252.0, 238,    -0.050950222798266442,    -0.050950222798266449,    -0.050950222798266359 },
   { 126,       0.0, 252.0, 250,     -0.70396107388424478,     -0.70396107388424445,     -0.70396107388424423 },
   { 126,       0.0, 252.0, 251,     -0.79071888246315214,     -0.79071888246315192,     -0.79071888246315158 },
   /* n=126 rf=0.0002 A=12: outBegIdx 126, outNBElement 126 */
   { 126,    0.0002,  12.0, 126,      0.38613743413152252,      0.38613743413152257,                   NO_ARM },
   { 126,    0.0002,  12.0, 127,      0.36526982894021121,      0.36526982894021132,                   NO_ARM },
   { 126,    0.0002,  12.0, 143,      0.37856376555268284,      0.37856376555268323,                   NO_ARM },
   { 126,    0.0002,  12.0, 179,      0.50358738996801111,      0.50358738996801122,                   NO_ARM },
   { 126,    0.0002,  12.0, 200,      0.25255738034591829,      0.25255738034591824,                   NO_ARM },
   { 126,    0.0002,  12.0, 216,     -0.24033351123148153,     -0.24033351123148147,                   NO_ARM },
   { 126,    0.0002,  12.0, 224,   -0.0061916613806617055,   -0.0061916613806617515,                   NO_ARM },
   { 126,    0.0002,  12.0, 251,     -0.19951342144240197,     -0.19951342144240194,                   NO_ARM },
   /* n=251 rf=0 A=252: outBegIdx 251, outNBElement 1 */
   { 251,       0.0, 252.0, 251,      0.60358137864970174,      0.60358137864970196,      0.60358137864970185 },
};
#define NB_SHARPE_GOLDEN ((int)(sizeof(sharpeGoldens)/sizeof(SharpeGolden)))

static ErrorNumber test_sharpe_oracle( const TA_History *history )
{
   static TA_Real out[SHARPE_CAP];
   TA_Integer begIdx = 0, nbElement = 0;
   TA_RetCode retCode;
   int k, haveCall = 0;
   const SharpeGolden *prev = NULL;
   double got;

   if( history->nbBars != 252 )
      return TA_TEST_PASS;

   for( k = 0; k < NB_SHARPE_GOLDEN; k++ )
   {
      const SharpeGolden *g = &sharpeGoldens[k];

      if( !haveCall || g->period != prev->period || g->rf != prev->rf || g->ann != prev->ann )
      {
         retCode = TA_SHARPE( 0, 251, history->close, g->period, g->rf, g->ann,
                              &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != g->period || nbElement != 252-begIdx )
         {
            printf( "SHARPE oracle Fail [N=%d]: rc=%d (%d,%d)\n", g->period,
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         haveCall = 1;
         prev = g;
      }

      if( g->bar < begIdx || g->bar - begIdx >= nbElement )
      {
         printf( "SHARPE oracle Fail [N=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", g->period, g->bar, begIdx, begIdx + nbElement - 1 );
         return TA_SHARPE_VACUOUS;
      }

      got = out[g->bar - begIdx];
      g_sharpeOracleCmp++;
      if( !(SHARPE_DIST( got, g->empyrical ) <= SHARPE_ORACLE_TOL)
          || !(SHARPE_DIST( got, g->exact ) <= SHARPE_ORACLE_TOL)
          || (!isnan( g->quantstats )
              && !(SHARPE_DIST( got, g->quantstats ) <= SHARPE_ORACLE_TOL)) )
      {
         printf( "SHARPE oracle Fail [N=%d rf=%g A=%g] at bar %d: got %.17g, "
                 "empyrical %.17g, quantstats %.17g, exact %.17g (over %.0e)\n",
                 g->period, g->rf, g->ann, g->bar, got, g->empyrical,
                 g->quantstats, g->exact, SHARPE_ORACLE_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}
