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
 *  110726 MF,CC  First version (issue #572).
 *
 */

/* Description:
 *     Test TA_SORTINO, the rolling Sortino ratio over simple returns.
 *
 *     Legs:
 *       bacon      Bacon (2008)'s printed downside deviation, 0.0255 at a MAR
 *                  of 0.005 over his 24 monthly returns: the full-n divisor
 *                  matches it, n-1 and the below-MAR count miss it
 *       oracle     frozen empyrical-reloaded and quantstats rows on the
 *                  252-bar close series and on the Bacon series
 *       shapes     every window of hand-built series against a two-pass
 *                  recomputation, and every window with no return below the
 *                  MAR against the ruled plain quotient (+Inf, NaN)
 *       identity   annualization is a post-scale by sqrt(A), bit for bit
 *       alias      outReal == inReal, bitwise
 *       contract   lookback, the parameter ranges
 *       range      doRangeTestEx
 *
 *     WHAT EACH LEG CAN SEE, measured by mutating the generated C; every leg
 *     named goes red first.
 *
 *       n-1 divisor, MAR left out of the mean     bacon
 *       A for sqrt(A)                             shapes
 *       no zero-base guard                        shapes (zero price)
 *       no absolute-return trigger                shapes (crash then tiny)
 *       no downside trigger                       shapes (crash then rise)
 *       mean, not the count, in the no-downside   shapes (returns all equal
 *       quotient; a return at the MAR as below    to the MAR)
 *       lookback n-1                              contract
 *
 *     No leg sees the periodic rebuild dropped: it bounds rounding carried
 *     over windows that never trip a trigger, below 1e-13 on these series.
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
#define SORTINO_CAP 3200

#define SORTINO_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, inArr, period, mar, ann, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_value_comparisons();                         \
         ErrorNumber svErr_ = server_verify(                                     \
            "SORTINO", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),              \
            (const TA_Real*[]){ (inArr), NULL },                                 \
            (double[]){ (double)(period), (mar), (ann) }, 3,                     \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         if( server_verify_value_comparisons() == svCmp_ )                       \
         {                                                                       \
            printf( "SORTINO [period %d]: server_verify compared no server "     \
                    "despite live pipes\n", (int)(period) );                     \
            return TA_SORTINO_VACUOUS;                                           \
         }                                                                       \
      }                                                                          \
   } while(0)

/* The ratio crosses zero, where a relative distance is unbounded. */
#define SORTINO_DIST(got, want) (fabs( (got) - (want) ) / fmax( 1.0, fabs( want ) ))

/* Against the two-pass recomputation below: the running sums carry the
 * rounding of every return that passed through the window since the last
 * rebuild, and the reference its own rounding. Measured worst between the two
 * 3.6e-14, crash then tiny at period 20. */
#define SORTINO_REF_TOL 1e-13

/* Comparisons each leg makes, pinned exactly so a leg that stops comparing
 * fails. */
#define SORTINO_COV_REF    30177
#define SORTINO_COV_INF    12929
#define SORTINO_COV_NAN    1057
#define SORTINO_COV_ZERO   371
#define SORTINO_COV_ANN    3960
#define SORTINO_COV_ALIAS  2448
#define SORTINO_COV_ORACLE 99

static int g_sortinoRefCmp;
static int g_sortinoFlatCmp[2];
static int g_sortinoZeroBaseCmp;
static int g_sortinoAnnCmp;
static int g_sortinoAliasCmp;
static int g_sortinoOracleCmp;

static ErrorNumber test_sortino_bacon( void );
static ErrorNumber test_sortino_shapes( const TA_History *history );
static ErrorNumber test_sortino_identity( const TA_History *history );
static ErrorNumber test_sortino_aliasing( const TA_History *history );
static ErrorNumber test_sortino_contract( const TA_History *history );
static ErrorNumber test_sortino_range( const TA_History *history );
static ErrorNumber test_sortino_oracle( const TA_History *history );

ErrorNumber test_func_sortino( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_sortinoRefCmp = g_sortinoZeroBaseCmp = 0;
   g_sortinoFlatCmp[0] = g_sortinoFlatCmp[1] = 0;
   g_sortinoAnnCmp = g_sortinoAliasCmp = g_sortinoOracleCmp = 0;

#define SORTINO_RUN(call) do { err = (call); if( err != TA_TEST_PASS ) return err; } while(0)
   SORTINO_RUN( test_sortino_bacon() );
   SORTINO_RUN( test_sortino_shapes( history ) );
   SORTINO_RUN( test_sortino_identity( history ) );
   SORTINO_RUN( test_sortino_aliasing( history ) );
   SORTINO_RUN( test_sortino_contract( history ) );
   SORTINO_RUN( test_sortino_range( history ) );
   SORTINO_RUN( test_sortino_oracle( history ) );
#undef SORTINO_RUN

   if( history->nbBars == 252 )
   {
      static const struct { const char *leg; int want; const int *got; } cov[] = {
         { "two-pass",            SORTINO_COV_REF,    &g_sortinoRefCmp      },
         { "no downside, +Inf",   SORTINO_COV_INF,    &g_sortinoFlatCmp[0]  },
         { "no downside, NaN",    SORTINO_COV_NAN,    &g_sortinoFlatCmp[1]  },
         { "zero base price",     SORTINO_COV_ZERO,   &g_sortinoZeroBaseCmp },
         { "annualization",       SORTINO_COV_ANN,    &g_sortinoAnnCmp      },
         { "alias",               SORTINO_COV_ALIAS,  &g_sortinoAliasCmp    },
         { "oracle",              SORTINO_COV_ORACLE, &g_sortinoOracleCmp   },
      };
      unsigned int c;
      int bad = 0;

      for( c = 0; c < sizeof(cov)/sizeof(cov[0]); c++ )
      {
         if( *cov[c].got != cov[c].want )
         {
            printf( "SORTINO Fail: the %s leg compared %d times, not the %d this "
                    "file was written with\n", cov[c].leg, *cov[c].got,
                    cov[c].want );
            bad = 1;
         }
      }
      if( bad )
         return TA_SORTINO_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

static double sortino_return( const double *in, int bar )
{
   return in[bar-1] != 0.0 ? (in[bar]-in[bar-1])/in[bar-1] : 0.0;
}

/* The definition, a fresh pass over each window. Writes the window's counts
 * of returns below and above the MAR. */
static double sortino_two_pass( const double *in, int bar, int period, double mar,
                                double ann, int *below, int *above )
{
   double mean = 0.0, down = 0.0, d;
   int j;

   *below = *above = 0;
   for( j = bar-period+1; j <= bar; j++ )
   {
      d = sortino_return( in, j ) - mar;
      mean += d;
      if( d < 0.0 )
      {
         down += d * d;
         (*below)++;
      }
      else if( d > 0.0 )
         (*above)++;
   }
   mean /= (double)period;
   return sqrt( ann ) * (mean / sqrt( down / (double)period ));
}

/* Prices as whole cents, so every price is the 2-decimal value a feed sends. */
static double sortino_cents( long c )
{
   return (double)c / 100.0;
}

static unsigned int sortino_lcg( unsigned int *s )
{
   *s = *s * 1664525u + 1013904223u;
   return *s >> 8;
}

enum { SHAPE_STEPS, SHAPE_RISE, SHAPE_CRASH_FLAT, SHAPE_CRASH_TINY, SHAPE_CRASH_RISE,
       SHAPE_ZERO, SHAPE_WALK, SHAPE_COUNT };
static const char *const sortinoShapeName[SHAPE_COUNT] =
   { "doubling/halving", "rise", "crash then flat", "crash then tiny",
     "crash then rise", "zero price", "long walk" };

/* Builds a series of SORTINO_CAP bars or fewer; returns its length. */
static int sortino_build( int shape, double *x )
{
   unsigned int s = 12345u;
   long c = 10000;
   int i, n = 0;

   switch( shape )
   {
   case SHAPE_STEPS:
      /* Doubling, halving, flat, then alternating prices: every return is
       * exactly 1, -0.5 or 0, so a MAR of one of them makes a window of
       * returns all equal to it. */
      x[n++] = 1.0;
      for( i = 0; i < 60; i++, n++ ) x[n] = x[n-1] * 2.0;
      for( i = 0; i < 60; i++, n++ ) x[n] = x[n-1] * 0.5;
      for( i = 0; i < 40; i++, n++ ) x[n] = x[n-1];
      for( i = 0; i < 40; i++, n++ ) x[n] = (i & 1) ? x[n-1] * 2.0 : x[n-1] * 0.5;
      break;
   case SHAPE_RISE:
      /* A walk with dips, then a run that only rises or holds, then a dip:
       * windows with no downside and windows whose last shortfall left. */
      for( i = 0; i < 300; i++ )
      {
         if( i < 100 || i >= 250 )
            c += (long)(sortino_lcg( &s ) % 41) - 20;
         else
            c += (long)(sortino_lcg( &s ) % 3);
         x[n++] = sortino_cents( c );
      }
      break;
   case SHAPE_CRASH_FLAT:
   case SHAPE_CRASH_TINY:
      /* One large shortfall in a walk, then flat or near-flat prices: once it
       * leaves, the downside sum left by subtraction is rounding. */
      for( i = 0; i < 400; i++ )
      {
         if( i == 60 )
            c = c / 40;
         else if( i == 61 )
            c = c * 40;
         else if( i < 60 || i > 300 )
            c += (long)(sortino_lcg( &s ) % 41) - 20;
         if( shape == SHAPE_CRASH_TINY && i > 61 && i <= 300 )
            x[n++] = sortino_cents( c ) - (double)(i % 7) * 1e-9;
         else
            x[n++] = sortino_cents( c );
      }
      break;
   case SHAPE_CRASH_RISE:
      /* A halving, then a 2% rise per bar with a one-cent dip every seventh
       * bar: once the halving leaves, the downside sum is a few dips while the
       * returns stay large. */
      c = 100000;
      for( i = 0; i < 300; i++ )
      {
         if( i == 60 )
            c = c / 2;
         else if( i > 60 && i % 7 == 0 )
            c -= 1;
         else if( i > 60 )
            c += c / 50;
         else
            c += (long)(sortino_lcg( &s ) % 41) - 20;
         x[n++] = sortino_cents( c );
      }
      break;
   case SHAPE_ZERO:
      for( i = 0; i < 200; i++ )
      {
         c += (long)(sortino_lcg( &s ) % 41) - 20;
         x[n++] = (i == 50 || i == 120 || i == 121) ? 0.0 : sortino_cents( c );
      }
      break;
   case SHAPE_WALK:
      for( i = 0; i < SORTINO_CAP; i++ )
      {
         c += (long)(sortino_lcg( &s ) % 201) - 100;
         if( c < 100 )
            c = 100;
         x[n++] = sortino_cents( c );
      }
      break;
   }
   return n;
}

/* Every window of one call: a window with no return below the MAR is the
 * plain quotient, every other one the two-pass value. */
static ErrorNumber sortino_check_series( const char *tag, const double *in, int nbBars,
                                         int period, double mar, double ann, int route )
{
   static TA_Real out[SORTINO_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   double want;
   int i, bar, j, below, above;

   retCode = TA_SORTINO( 0, nbBars-1, in, period, mar, ann, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != period || nbElement != nbBars-period )
   {
      printf( "SORTINO %s Fail [N=%d]: rc=%d (%d,%d)\n", tag, period,
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   if( route )
      SORTINO_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                             in, period, mar, ann, out );

   for( i = 0; i < nbElement; i++ )
   {
      bar = begIdx + i;

      for( j = bar-period; j <= bar-1; j++ )
         if( in[j] == 0.0 )
         {
            g_sortinoZeroBaseCmp++;
            break;
         }

      want = sortino_two_pass( in, bar, period, mar, ann, &below, &above );
      if( below == 0 )
      {
         want = (above > 0 && ann != 0.0) ? INFINITY : NAN;
         g_sortinoFlatCmp[isnan( want ) ? 1 : 0]++;
         if( isnan( want ) ? !isnan( out[i] ) : out[i] != want )
         {
            printf( "SORTINO %s Fail [N=%d MAR=%g A=%g] bar %d: %.17g on a window "
                    "with no return below the MAR (%d above), expected %g\n", tag,
                    period, mar, ann, bar, out[i], above, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         continue;
      }

      g_sortinoRefCmp++;
      if( !(SORTINO_DIST( out[i], want ) <= SORTINO_REF_TOL) )
      {
         printf( "SORTINO %s Fail [N=%d MAR=%g A=%g] bar %d: %.17g, two-pass %.17g, "
                 "distance %.3e over %.0e\n", tag, period, mar, ann, bar, out[i],
                 want, SORTINO_DIST( out[i], want ), SORTINO_REF_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* The MAR is chosen per vector so that some window of the doubling/halving
 * series has every return equal to it (1, -0.5, 0), and a negative one so
 * that rising windows have no downside. */
static ErrorNumber test_sortino_shapes( const TA_History *history )
{
   static double x[SORTINO_CAP];
   static const struct { int period; double mar; double ann; } vec[] = {
      {  2,  0.0,    252.0 },
      {  5,  1.0,      1.0 },
      { 20, -0.5,     52.0 },
      { 20,  0.0001, 252.0 },
      { 37, -0.0001,  12.0 },
      { 20,  0.0,      0.0 },
      { 49, -0.5,      1.0 },
      { 20,  0.0,    252.0 },
      {  8, -0.002,  252.0 },
   };
   ErrorNumber err;
   int shape, v, nbBars;

   for( shape = 0; shape < SHAPE_COUNT; shape++ )
   {
      nbBars = sortino_build( shape, x );
      for( v = 0; v < (int)(sizeof(vec)/sizeof(vec[0])); v++ )
      {
         err = sortino_check_series( sortinoShapeName[shape], x, nbBars, vec[v].period,
                                     vec[v].mar, vec[v].ann, 1 );
         if( err != TA_TEST_PASS )
            return err;
      }
   }

   err = sortino_check_series( "close", history->close, (int)history->nbBars,
                               20, 0.0, 252.0, 0 );
   if( err != TA_TEST_PASS )
      return err;
   return sortino_check_series( "close", history->close, (int)history->nbBars,
                                63, 0.0002, 12.0, 1 );
}

/* Bacon, Practical Portfolio Performance Measurement and Attribution, 2nd
 * ed. (2008): the 24 monthly portfolio returns of his worked example, as
 * PerformanceAnalytics ships them (portfolio_bacon), and the downside
 * deviation he prints for them at a MAR of 0.005, 0.0255. The prices are 100
 * compounded by those returns (ta-lib-oracles capture_572_sortino.py), so
 * their returns are his to rounding. */
static const double sortinoBaconReturns[24] = {
    0.003, 0.026, 0.011, -0.010, 0.015, 0.025, 0.016, 0.067,
   -0.014, 0.040, -0.005, 0.081, 0.040, -0.037, -0.061, 0.017,
   -0.049, -0.022, 0.070, 0.058, -0.065, 0.024, -0.005, -0.009 };
static const double sortinoBaconPrices[25] = {
   100, 100.29999999999998, 102.90779999999998, 104.03978579999998,
   102.99938794199997, 104.54437876112996, 107.1579882301582, 108.87251604184073,
   116.16697461664405, 114.54063697201104, 119.12226245089148, 118.52665113863702,
   128.12730988086662, 133.25240227610129, 128.32206339188554, 120.49441752498052,
   122.54282262290518, 116.53822431438282, 113.97438337946639, 121.95259021602904,
   129.02584044855874, 120.63916081940243, 123.53450067906809, 122.91682817567275,
   121.8105767220917 };
#define SORTINO_BACON_DD      0.0255
#define SORTINO_BACON_DD_TOL  0.00005   /* half a unit of his last digit */

static ErrorNumber test_sortino_bacon( void )
{
   static TA_Real out[25];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   double mean = 0.0, down = 0.0, d, dd, alt[2];
   int j, nbBelow = 0;

   for( j = 0; j < 24; j++ )
   {
      d = sortinoBaconReturns[j] - 0.005;
      mean += d;
      if( d < 0.0 )
      {
         down += d * d;
         nbBelow++;
      }
   }
   mean /= 24.0;

   /* Non-vacuity: the other two divisors in published use miss the printed
    * value, so matching it decides the divisor. */
   alt[0] = sqrt( down / 23.0 );
   alt[1] = sqrt( down / (double)nbBelow );
   for( j = 0; j < 2; j++ )
      if( fabs( alt[j] - SORTINO_BACON_DD ) <= SORTINO_BACON_DD_TOL )
      {
         printf( "SORTINO bacon Fail: divisor %d also gives %.6f\n", j, alt[j] );
         return TA_SORTINO_VACUOUS;
      }

   retCode = TA_SORTINO( 0, 24, sortinoBaconPrices, 24, 0.005, 1.0,
                         &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 24 || nbElement != 1 )
   {
      printf( "SORTINO bacon Fail: rc=%d (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   dd = mean / out[0];
   if( !(fabs( dd - SORTINO_BACON_DD ) <= SORTINO_BACON_DD_TOL) )
   {
      printf( "SORTINO bacon Fail: ratio %.17g implies a downside deviation of "
              "%.6f, Bacon prints %.4f\n", out[0], dd, SORTINO_BACON_DD );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* sqrt(A) multiplies the per-bar ratio last, so every annualization is that
 * ratio times sqrt(A) bit for bit. */
static ErrorNumber test_sortino_identity( const TA_History *history )
{
   static TA_Real raw[SORTINO_CAP], out[SORTINO_CAP];
   static const double anns[] = { 2.0, 12.0, 52.0, 252.0, 365.25, 1638.0 };
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, a, i;
   double scaled;

   for( n = 2; n <= 62; n += 30 )
   {
      retCode = TA_SORTINO( 0, nbBars-1, history->close, n, 0.0001, 1.0,
                            &begIdx, &nbElement, raw );
      if( retCode != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      for( a = 0; a < (int)(sizeof(anns)/sizeof(anns[0])); a++ )
      {
         retCode = TA_SORTINO( 0, nbBars-1, history->close, n, 0.0001, anns[a],
                               &begIdx2, &nbElement2, out );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         for( i = 0; i < nbElement; i++ )
         {
            scaled = sqrt( anns[a] ) * raw[i];
            g_sortinoAnnCmp++;
            if( memcmp( &scaled, &out[i], sizeof(double) ) != 0
                || (isfinite( raw[i] ) && raw[i] != 0.0 && out[i] == raw[i]) )
            {
               printf( "SORTINO annualization Fail [N=%d A=%g] bar %d: %.17g, "
                       "sqrt(A) times the A=1 value %.17g is %.17g\n", n, anns[a],
                       begIdx+i, out[i], raw[i], scaled );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* outReal == inReal, bitwise: the trailing price of a window sits in the slot
 * the output of its bar is written to. */
static ErrorNumber test_sortino_aliasing( const TA_History *history )
{
   static double x[SORTINO_CAP], clean[SORTINO_CAP], alias[SORTINO_CAP];
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
         nbBars = sortino_build( SHAPE_CRASH_FLAT, x );
         src = x;
      }

      for( n = 2; n <= 38; n += 12 )
      {
         retCode = TA_SORTINO( 0, nbBars-1, src, n, 0.0001, 252.0,
                               &begIdx, &nbElement, clean );
         if( retCode != TA_SUCCESS )
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         for( i = 0; i < nbBars; i++ )
            alias[i] = src[i];
         retCode = TA_SORTINO( 0, nbBars-1, alias, n, 0.0001, 252.0,
                               &begIdx2, &nbElement2, alias );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "SORTINO alias Fail [N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                    n, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_sortinoAliasCmp++;
            if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
            {
               printf( "SORTINO alias Fail [N=%d] bar %d: separate %.17g, in-place "
                       "%.17g\n", n, begIdx+i, clean[i], alias[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_sortino_contract( const TA_History *history )
{
   static double out[SORTINO_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n;

   for( n = -1; n <= 1; n++ )
   {
      retCode = TA_SORTINO( 0, nbBars-1, history->close, n, 0.0, 252.0,
                            &begIdx, &nbElement, out );
      if( retCode != TA_BAD_PARAM || TA_SORTINO_Lookback( n, 0.0, 252.0 ) != -1 )
      {
         printf( "SORTINO contract Fail: period %d gave rc=%d, expected "
                 "TA_BAD_PARAM\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }
   if( TA_SORTINO_Lookback( 100001, 0.0, 252.0 ) != -1
       || TA_SORTINO_Lookback( 100000, 0.0, 252.0 ) != 100000 )
   {
      printf( "SORTINO contract Fail: lookback at the period's upper bound\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   retCode = TA_SORTINO( 0, nbBars-1, history->close, 20, 0.0, -1.0,
                         &begIdx, &nbElement, out );
   if( retCode != TA_BAD_PARAM )
   {
      printf( "SORTINO contract Fail: annualization -1 gave rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( n = 2; n <= 50; n += 16 )
   {
      if( TA_SORTINO_Lookback( n, -0.01, 1.0 ) != n )
      {
         printf( "SORTINO contract Fail: lookback(%d) = %d\n", n,
                 TA_SORTINO_Lookback( n, -0.01, 1.0 ) );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      retCode = TA_SORTINO( 0, nbBars-1, history->close, n, -0.01, 1.0,
                            &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n || nbElement != nbBars-n )
      {
         printf( "SORTINO contract Fail [N=%d]: rc=%d (%d,%d)\n", n, (int)retCode,
                 begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }

   return TA_TEST_PASS;
}

/* TA_STABLE_EPSILON: the rebuild phase is seeded from each call's own
 * startIdx. */
typedef struct { int period; double mar; double ann; const TA_Real *in; } SortinoRangeParam;

static TA_RetCode sortinoRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                            TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                            TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                            TA_Integer *lookback, void *opaqueData,
                                            unsigned int outputNb, unsigned int *isOutputInteger )
{
   SortinoRangeParam *p = (SortinoRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_SORTINO_Lookback( p->period, p->mar, p->ann );
   return TA_SORTINO( startIdx, endIdx, p->in, p->period, p->mar, p->ann,
                      outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_sortino_range( const TA_History *history )
{
   SortinoRangeParam param;

   param.period = 20;
   param.mar    = -0.0001;
   param.ann    = 252.0;
   param.in     = history->close;

   return doRangeTestEx( sortinoRangeTestFunction, TA_STABLE_EPSILON,
                         TA_TEST_UNST_NONE, (void *)&param, 1, 0 );
}

/* Frozen rows of ta-lib-oracles capture_572_sortino.py (oracles 1ba1b1f). Sources:
 * empyrical-reloaded 0.5.12 roll_sortino_ratio, and quantstats 0.0.81
 * rolling_sortino handed the excess returns. The capture checks both against
 * the ratio in exact rational arithmetic; this file holds TA_SORTINO to all
 * three. */
#define SORTINO_ORACLE_TOL 1e-12
#define SORTINO_NAN  NAN
#define SORTINO_PINF INFINITY

typedef struct { int period; double mar; double ann; int bar;
                 double exact; double empyrical; double quantstats; } SortinoGolden;

static const SortinoGolden sortinoCloseGoldens[] =
{
   /* n=20 MAR=0 A=252: outBegIdx 20, outNBElement 232 */
   {  20,       0.0, 252.0,  20,     -0.88136079669064571,     -0.88136079669064571,     -0.88136079669064571 },
   {  20,       0.0, 252.0,  21,      -2.3697558870186577,      -2.3697558870186577,      -2.3697558870186577 },
   {  20,       0.0, 252.0, 100,       9.1639932390730703,       9.1639932390730685,       9.1639932390730685 },
   {  20,       0.0, 252.0, 130,       40.940344025156918,       40.940344025156918,       40.940344025156925 },
   {  20,       0.0, 252.0, 182,     0.053938477914803121,     0.053938477914802156,     0.053938477914803593 },
   {  20,       0.0, 252.0, 185,      0.39058626495103521,      0.39058626495103665,      0.39058626495103566 },
   {  20,       0.0, 252.0, 196,      -7.9603045212329855,      -7.9603045212329855,      -7.9603045212329846 },
   {  20,       0.0, 252.0, 200,      -6.6859593646546367,      -6.6859593646546367,      -6.6859593646546367 },
   {  20,       0.0, 252.0, 245,       1.2503856190495752,       1.2503856190495752,       1.2503856190495768 },
   {  20,       0.0, 252.0, 251,       1.6201187554045071,        1.620118755404508,       1.6201187554045082 },
   /* n=20 MAR=0 A=1: outBegIdx 20, outNBElement 232 */
   {  20,       0.0,   1.0,  20,    -0.055520511508695428,    -0.055520511508695428,    -0.055520511508695428 },
   {  20,       0.0,   1.0,  21,     -0.14928058916625345,     -0.14928058916625345,     -0.14928058916625345 },
   {  20,       0.0,   1.0,  95,      0.86514189225669047,      0.86514189225669091,      0.86514189225669047 },
   {  20,       0.0,   1.0, 100,      0.57727731254439574,      0.57727731254439563,      0.57727731254439563 },
   {  20,       0.0,   1.0, 130,       2.5789992590474831,       2.5789992590474835,       2.5789992590474835 },
   {  20,       0.0,   1.0, 182,    0.0033978047299980671,    0.0033978047299980069,    0.0033978047299980966 },
   {  20,       0.0,   1.0, 196,     -0.50145205056013242,     -0.50145205056013242,     -0.50145205056013231 },
   {  20,       0.0,   1.0, 200,     -0.42117585130379959,     -0.42117585130379953,     -0.42117585130379953 },
   {  20,       0.0,   1.0, 235,       1.3760693028421025,       1.3760693028421029,       1.3760693028421029 },
   {  20,       0.0,   1.0, 251,      0.10205788859980494,        0.102057888599805,      0.10205788859980501 },
   /* n=20 MAR=0.0005 A=252: outBegIdx 20, outNBElement 232 */
   {  20,    0.0005, 252.0,  20,      -1.2035880845123688,       -1.203588084512369,      -1.2035880845123692 },
   {  20,    0.0005, 252.0,  21,      -2.6750092210611918,      -2.6750092210611918,      -2.6750092210611922 },
   {  20,    0.0005, 252.0, 100,       8.2568441798975947,       8.2568441798975929,       8.2568441798975964 },
   {  20,    0.0005, 252.0, 130,       36.366478693509869,       36.366478693509862,       36.366478693509869 },
   {  20,    0.0005, 252.0, 138,     0.059791292150690699,     0.059791292150690838,     0.059791292150690456 },
   {  20,    0.0005, 252.0, 176,       1.3235474506837159,       1.3235474506837144,        1.323547450683717 },
   {  20,    0.0005, 252.0, 196,       -8.177783397420189,       -8.177783397420189,      -8.1777833974201872 },
   {  20,    0.0005, 252.0, 200,      -6.9658853108100081,      -6.9658853108100072,      -6.9658853108100089 },
   {  20,    0.0005, 252.0, 245,      0.70622145333974284,      0.70622145333974262,      0.70622145333974451 },
   {  20,    0.0005, 252.0, 251,       1.0534985233165199,       1.0534985233165202,       1.0534985233165213 },
   /* n=20 MAR=-0.0002 A=52: outBegIdx 20, outNBElement 232 */
   {  20,   -0.0002,  52.0,  20,     -0.34096503655134724,     -0.34096503655134713,     -0.34096503655134719 },
   {  20,   -0.0002,  52.0,  21,      -1.0201642409770355,      -1.0201642409770353,      -1.0201642409770353 },
   {  20,   -0.0002,  52.0,  35,      -1.1484420735899294,      -1.1484420735899306,      -1.1484420735899294 },
   {  20,   -0.0002,  52.0, 100,       4.3335914596969447,       4.3335914596969483,       4.3335914596969456 },
   {  20,   -0.0002,  52.0, 130,       19.501633637595155,       19.501633637595155,       19.501633637595152 },
   {  20,   -0.0002,  52.0, 184,    0.0058958852105495804,    0.0058958852105487902,    0.0058958852105496047 },
   {  20,   -0.0002,  52.0, 189,      -1.4329217271604235,      -1.4329217271604238,      -1.4329217271604229 },
   {  20,   -0.0002,  52.0, 196,      -3.5755061377258297,      -3.5755061377258293,      -3.5755061377258297 },
   {  20,   -0.0002,  52.0, 200,      -2.9849836836540242,      -2.9849836836540242,      -2.9849836836540238 },
   {  20,   -0.0002,  52.0, 251,      0.84144688090043507,      0.84144688090043485,      0.84144688090043473 },
   /* n=2 MAR=0 A=252: outBegIdx 2, outNBElement 250 */
   {   2,       0.0, 252.0,   2,       76.408916632220269,       76.408916632220254,       76.408916632220269 },
   {   2,       0.0, 252.0,   3,        7.228800975722983,        7.228800975722983,        7.228800975722983 },
   {   2,       0.0, 252.0,  10,             SORTINO_PINF,             SORTINO_PINF,             SORTINO_PINF },
   {   2,       0.0, 252.0,  92,      -13.737526592805828,      -13.737526592805823,      -13.737526592805825 },
   {   2,       0.0, 252.0, 100,       31.259247234250349,       31.259247234250349,       31.259247234250353 },
   {   2,       0.0, 252.0, 121,     0.051289037286066881,     0.051289037286066874,     0.051289037286049749 },
   {   2,       0.0, 252.0, 192,      -15.873077206649612,       -15.87307720664961,      -15.873077206649612 },
   {   2,       0.0, 252.0, 200,      -9.6641113843837587,      -9.6641113843837569,      -9.6641113843837587 },
   {   2,       0.0, 252.0, 201,     0.012588753824659848,     0.012588753824659847,     0.012588753824659848 },
   {   2,       0.0, 252.0, 204,       475.45528233540159,       475.45528233540148,       475.45528233540153 },
   {   2,       0.0, 252.0, 251,      -13.860561313162702,        -13.8605613131627,      -13.860561313162702 },
   /* n=5 MAR=-0.01 A=1: outBegIdx 5, outNBElement 247 */
   {   5,     -0.01,   1.0,   5,       9.8595057198236589,       9.8595057198236606,       9.8595057198236606 },
   {   5,     -0.01,   1.0,   6,      0.91452304762294145,      0.91452304762294157,      0.91452304762294157 },
   {   5,     -0.01,   1.0,  23,     -0.77789028414239059,     -0.77789028414239059,     -0.77789028414239048 },
   {   5,     -0.01,   1.0,  42,             SORTINO_PINF,             SORTINO_PINF,             SORTINO_PINF },
   {   5,     -0.01,   1.0,  99,       1.2778462405525925,       1.2778462405525921,       1.2778462405525919 },
   {   5,     -0.01,   1.0, 100,       1.0735359555108577,       1.0735359555108572,       1.0735359555108572 },
   {   5,     -0.01,   1.0, 181,    -0.010033854842909388,    -0.010033854842909308,    -0.010033854842909258 },
   {   5,     -0.01,   1.0, 200,      0.31698572468741726,      0.31698572468741737,      0.31698572468741743 },
   {   5,     -0.01,   1.0, 233,       84.701445088124686,         84.7014450881247,         84.7014450881247 },
   {   5,     -0.01,   1.0, 251,             SORTINO_PINF,             SORTINO_PINF,             SORTINO_PINF },
   /* n=63 MAR=0 A=252: outBegIdx 63, outNBElement 189 */
   {  63,       0.0, 252.0,  63,      0.27364838449455059,      0.27364838449455081,      0.27364838449455042 },
   {  63,       0.0, 252.0,  64,     0.030170400126111795,      0.03017040012611184,     0.030170400126111635 },
   {  63,       0.0, 252.0, 100,       6.5651844314204171,       6.5651844314204144,       6.5651844314204171 },
   {  63,       0.0, 252.0, 135,       12.214840626913245,       12.214840626913244,       12.214840626913245 },
   {  63,       0.0, 252.0, 153,    -0.015865457801401493,    -0.015865457801401136,    -0.015865457801401511 },
   {  63,       0.0, 252.0, 196,      -3.5179017169877507,      -3.5179017169877493,      -3.5179017169877507 },
   {  63,       0.0, 252.0, 200,      -2.6239569090794026,      -2.6239569090794026,      -2.6239569090794026 },
   {  63,       0.0, 252.0, 242,      -1.4000839030900123,      -1.4000839030900112,      -1.4000839030900125 },
   {  63,       0.0, 252.0, 245,     -0.94559545107048959,     -0.94559545107048948,     -0.94559545107049026 },
   {  63,       0.0, 252.0, 251,     -0.61598378977267731,     -0.61598378977267731,     -0.61598378977267754 },
   /* n=126 MAR=0 A=252: outBegIdx 126, outNBElement 126 */
   { 126,       0.0, 252.0, 126,       3.3431697653407921,       3.3431697653407917,       3.3431697653407917 },
   { 126,       0.0, 252.0, 127,       3.1577152868215603,       3.1577152868215599,       3.1577152868215608 },
   { 126,       0.0, 252.0, 179,        4.726827850694467,        4.726827850694467,       4.7268278506944679 },
   { 126,       0.0, 252.0, 181,       4.1362874939652929,       4.1362874939652921,       4.1362874939652947 },
   { 126,       0.0, 252.0, 200,        2.280108850100834,       2.2801088501008357,       2.2801088501008344 },
   { 126,       0.0, 252.0, 201,        1.118713783065368,       1.1187137830653704,        1.118713783065368 },
   { 126,       0.0, 252.0, 216,      -1.2445744544847064,      -1.2445744544847062,      -1.2445744544847062 },
   { 126,       0.0, 252.0, 238,    -0.067545546720979163,    -0.067545546720979163,    -0.067545546720979066 },
   { 126,       0.0, 252.0, 251,      -1.0183087716044883,      -1.0183087716044876,      -1.0183087716044883 },
   /* n=126 MAR=0.0002 A=12: outBegIdx 126, outNBElement 126 */
   { 126,    0.0002,  12.0, 126,       0.6793611163107981,      0.67936111631079832,       0.6793611163107981 },
   { 126,    0.0002,  12.0, 127,      0.63916827567365819,      0.63916827567365853,      0.63916827567365819 },
   { 126,    0.0002,  12.0, 142,      0.76959860670450064,      0.76959860670450142,      0.76959860670450064 },
   { 126,    0.0002,  12.0, 175,      0.82351351637425285,      0.82351351637425285,      0.82351351637425307 },
   { 126,    0.0002,  12.0, 179,      0.96935195154442966,      0.96935195154442988,      0.96935195154442955 },
   { 126,    0.0002,  12.0, 200,      0.44587297245689139,      0.44587297245689139,      0.44587297245689139 },
   { 126,    0.0002,  12.0, 216,     -0.30493802309672863,     -0.30493802309672857,     -0.30493802309672863 },
   { 126,    0.0002,  12.0, 224,   -0.0082671262485690556,   -0.0082671262485691163,   -0.0082671262485690192 },
   { 126,    0.0002,  12.0, 251,     -0.25565395308671407,     -0.25565395308671396,     -0.25565395308671396 },
   /* n=251 MAR=0 A=252: outBegIdx 251, outNBElement 1 */
   { 251,       0.0, 252.0, 251,      0.89759326931571404,      0.89759326931571481,      0.89759326931571415 },
};
static const SortinoGolden sortinoBaconGoldens[] =
{
   /* n=24 MAR=0.005 A=1: outBegIdx 24, outNBElement 1 */
   {  24,     0.005,   1.0,  24,      0.15663707566008581,      0.15663707566008586,      0.15663707566008581 },
   /* n=24 MAR=0.005 A=12: outBegIdx 24, outNBElement 1 */
   {  24,     0.005,  12.0,  24,      0.54260674678455789,      0.54260674678455811,      0.54260674678455789 },
   /* n=24 MAR=0 A=1: outBegIdx 24, outNBElement 1 */
   {  24,       0.0,   1.0,  24,      0.39237202879506444,      0.39237202879506433,      0.39237202879506444 },
   /* n=12 MAR=0.005 A=1: outBegIdx 12, outNBElement 13 */
   {  12,     0.005,   1.0,  12,       2.1429865385362996,       2.1429865385362996,       2.1429865385362996 },
   {  12,     0.005,   1.0,  13,       2.5570269422289469,       2.5570269422289469,       2.5570269422289469 },
   {  12,     0.005,   1.0,  14,       0.9856280155484658,      0.98562801554846602,       0.9856280155484658 },
   {  12,     0.005,   1.0,  16,      0.44125040472245375,      0.44125040472245391,      0.44125040472245369 },
   {  12,     0.005,   1.0,  21,     0.016629940345795905,     0.016629940345795898,     0.016629940345795898 },
   {  12,     0.005,   1.0,  24,      -0.2336491794154387,     -0.23364917941543864,      -0.2336491794154387 },
};

static int sortino_golden_match( double got, double want )
{
   if( isnan( want ) )
      return isnan( got );
   if( isinf( want ) )
      return got == want;
   return SORTINO_DIST( got, want ) <= SORTINO_ORACLE_TOL;
}

static ErrorNumber sortino_oracle_table( const char *tag, const double *in, int nbBars,
                                         const SortinoGolden *rows, int nbRows, int route )
{
   static TA_Real out[SORTINO_CAP];
   TA_Integer begIdx = 0, nbElement = 0;
   TA_RetCode retCode;
   int k, haveCall = 0;
   const SortinoGolden *prev = NULL;
   double got;

   for( k = 0; k < nbRows; k++ )
   {
      const SortinoGolden *g = &rows[k];

      if( !haveCall || g->period != prev->period || g->mar != prev->mar || g->ann != prev->ann )
      {
         retCode = TA_SORTINO( 0, nbBars-1, in, g->period, g->mar, g->ann,
                               &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != g->period || nbElement != nbBars-begIdx )
         {
            printf( "SORTINO %s oracle Fail [N=%d]: rc=%d (%d,%d)\n", tag, g->period,
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         if( route )
            SORTINO_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                                   in, g->period, g->mar, g->ann, out );
         haveCall = 1;
         prev = g;
      }

      if( g->bar < begIdx || g->bar - begIdx >= nbElement )
      {
         printf( "SORTINO %s oracle Fail [N=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", tag, g->period, g->bar, begIdx, begIdx + nbElement - 1 );
         return TA_SORTINO_VACUOUS;
      }

      got = out[g->bar - begIdx];
      g_sortinoOracleCmp++;
      if( !sortino_golden_match( got, g->empyrical )
          || !sortino_golden_match( got, g->exact )
          || !sortino_golden_match( got, g->quantstats ) )
      {
         printf( "SORTINO %s oracle Fail [N=%d MAR=%g A=%g] at bar %d: got %.17g, "
                 "empyrical %.17g, quantstats %.17g, exact %.17g (over %.0e)\n",
                 tag, g->period, g->mar, g->ann, g->bar, got, g->empyrical,
                 g->quantstats, g->exact, SORTINO_ORACLE_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

static ErrorNumber test_sortino_oracle( const TA_History *history )
{
   ErrorNumber err;

   err = sortino_oracle_table( "bacon", sortinoBaconPrices, 25, sortinoBaconGoldens,
                               (int)(sizeof(sortinoBaconGoldens)/sizeof(SortinoGolden)), 1 );
   if( err != TA_TEST_PASS || history->nbBars != 252 )
      return err;
   return sortino_oracle_table( "close", history->close, 252, sortinoCloseGoldens,
                                (int)(sizeof(sortinoCloseGoldens)/sizeof(SortinoGolden)), 0 );
}
