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
 *  092926 MF,CC  First version (issue #469).
 */

/* Description:
 *
 *   Test TA_CHOP (raw high-low box) and TA_CHOPTR (true high-low box).
 *
 *   Legs, each run on both functions:
 *     1. DIFFERENTIAL against a compose over TA_TRANGE / TA_SUM / TA_MAX /
 *        TA_MIN, every period 2..100. Not bit-exact: TA_SUM runs a total
 *        where the bodies re-sum the window.
 *     2. The PUBLISHED vector, Futures, October 1993, p. 53.
 *     3. EXTERNAL goldens: Skender 3.0.0's SPY input and a 1200-bar
 *        synthetic, against an exact-rational reference. The gap rows are
 *        where the two boxes differ, so they are what tells the two
 *        functions apart.
 *     4. Exact edges: each half of the degenerate guard, no epsilon band,
 *        the association, no clamp in either direction.
 *     5. Lookback. 6. In-place aliasing. 7. startIdx/endIdx range sweep, in
 *        the exact class.
 *
 *   Leg 2, 3 and 4 inputs are bars the --codegen sweep never sends, so they
 *   are routed through server_verify.
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
#define CHOP_CAP    1300
#define CHOP_SYN_NB 1200
#define CHOP_STAIR_NB 600

/* Leg 1. CHOP is exactly 0 on a staircase, where a relative-only bound
 * fails on rounding noise; hence the absolute companion. */
#define CHOP_DIFF_REL 1e-12
#define CHOP_DIFF_ABS 1e-12

/* Legs 3 and 4 (off-contract row). */
#define CHOP_GOLD_REL 1e-14
#define CHOP_GOLD_ABS 1e-13

/* The exact lower bound is 0 on contract; rounding leaves it by ~1e-14. */
#define CHOP_LOW_SLACK  1e-13
#define CHOP_HIGH_SLACK 1e-12

typedef TA_RetCode (*ChopFunc)( int, int, const double[], const double[],
                                const double[], int, int*, int*, double[] );

typedef struct
{
   const char *name;
   ChopFunc    fn;
   int       (*lookback)( int );
   int         trueBox;
} ChopVariant;

static const ChopVariant chopVariants[2] =
{
   { "CHOP",   TA_CHOP,   TA_CHOP_Lookback,   0 },
   { "CHOPTR", TA_CHOPTR, TA_CHOPTR_Lookback, 1 },
};

/* Leg 2. Gibbons Burke, "Measuring market choppiness with chaos", Futures,
 * October 1993, p. 53: IMM D-mark Sept. 1993 futures ("Source: Technical
 * Tools") and its printed CI14, 2 decimals. Every printed row has raw
 * extrema equal to true extrema, so both functions answer the same numbers
 * and the table cannot tell the boxes apart; leg 3 does. `exact` is the
 * exact-rational value on these doubles. */
static const double chopPubHigh[24] =
   { 57.76, 57.48, 57.87, 57.96, 57.98, 57.94, 58.35, 58.55, 58.73, 58.36, 58.00, 57.73,
     57.83, 57.97, 57.98, 57.31, 58.42, 58.61, 58.31, 58.25, 58.89, 58.90, 58.60, 58.24 };
static const double chopPubLow[24] =
   { 57.54, 57.27, 57.53, 57.44, 57.52, 57.33, 58.06, 58.05, 58.42, 58.08, 57.76, 57.60,
     57.58, 57.70, 57.09, 57.03, 57.48, 58.12, 58.13, 57.94, 58.12, 58.56, 58.01, 57.80 };
static const double chopPubClose[24] =
   { 57.67, 57.39, 57.77, 57.85, 57.70, 57.90, 58.25, 58.51, 58.57, 58.33, 57.87, 57.68,
     57.60, 57.93, 57.14, 57.20, 58.34, 58.35, 58.26, 58.21, 58.79, 58.73, 58.05, 57.88 };
static const double chopPubPrinted[10] =
   { 52.59, 50.53, 54.64, 54.48, 53.19, 51.57, 49.95, 48.86, 51.08, 50.82 };
static const double chopPubExact[10] =
   { 52.58762570583259, 50.527582890361494, 54.64310700976775, 54.48467199914937,
     53.19276329522038, 51.5705565719565, 49.948334091088554, 48.86261833849981,
     51.08424815280752, 50.82018911786761 };

/* Leg 3a. Skender Stock.Indicators 3.0.0, tests/indicators/_testdata/quotes/
 * default.csv (SPY 2017-2018), only the bars the goldens below read. */
typedef struct { int bar; double high, low, close; } ChopBar;
static const ChopBar chopSkender[] =
{
   /* bars 0..27 */
   {   0, 213.35, 211.52, 212.8 }, /* 2017-01-03 */
   {   1, 214.22, 213.15, 214.06 }, /* 2017-01-04 */
   {   2, 214.06, 213.02, 213.89 }, /* 2017-01-05 */
   {   3, 215.17, 213.42, 214.66 }, /* 2017-01-06 */
   {   4, 214.53, 213.91, 213.95 }, /* 2017-01-09 */
   {   5, 214.89, 213.52, 213.95 }, /* 2017-01-10 */
   {   6, 214.55, 213.13, 214.55 }, /* 2017-01-11 */
   {   7, 214.22, 212.53, 214.02 }, /* 2017-01-12 */
   {   8, 214.84, 214.17, 214.51 }, /* 2017-01-13 */
   {   9, 214.25, 213.33, 213.75 }, /* 2017-01-17 */
   {  10, 214.27, 213.42, 214.22 }, /* 2017-01-18 */
   {  11, 214.46, 212.96, 213.43 }, /* 2017-01-19 */
   {  12, 214.75, 213.49, 214.21 }, /* 2017-01-20 */
   {  13, 214.28, 212.83, 213.66 }, /* 2017-01-23 */
   {  14, 215.48, 213.77, 215.03 }, /* 2017-01-24 */
   {  15, 216.89, 215.89, 216.89 }, /* 2017-01-25 */
   {  16, 217.02, 216.36, 216.66 }, /* 2017-01-26 */
   {  17, 216.91, 216.12, 216.32 }, /* 2017-01-27 */
   {  18, 215.59, 213.9, 214.98 }, /* 2017-01-30 */
   {  19, 215.03, 213.82, 214.96 }, /* 2017-01-31 */
   {  20, 215.96, 214.4, 215.05 }, /* 2017-02-01 */
   {  21, 215.5, 214.29, 215.19 }, /* 2017-02-02 */
   {  22, 216.87, 215.84, 216.67 }, /* 2017-02-03 */
   {  23, 216.66, 215.92, 216.28 }, /* 2017-02-06 */
   {  24, 216.97, 216.09, 216.29 }, /* 2017-02-07 */
   {  25, 216.72, 215.7, 216.58 }, /* 2017-02-08 */
   {  26, 218.19, 216.84, 217.86 }, /* 2017-02-09 */
   {  27, 218.97, 217.88, 218.72 }, /* 2017-02-10 */
   /* bars 86..100 */
   {  86, 227.65, 226.94, 227.41 }, /* 2017-05-08 */
   {  87, 227.91, 226.82, 227.2 }, /* 2017-05-09 */
   {  88, 227.61, 226.92, 227.61 }, /* 2017-05-10 */
   {  89, 227.32, 225.95, 227.14 }, /* 2017-05-11 */
   {  90, 227.19, 226.47, 226.76 }, /* 2017-05-12 */
   {  91, 228.15, 227.21, 228.01 }, /* 2017-05-15 */
   {  92, 228.36, 227.38, 227.8 }, /* 2017-05-16 */
   {  93, 226.44, 223.7, 223.76 }, /* 2017-05-17 */
   {  94, 225.59, 223.39, 224.66 }, /* 2017-05-18 */
   {  95, 226.86, 225.14, 226.12 }, /* 2017-05-19 */
   {  96, 227.45, 226.61, 227.27 }, /* 2017-05-22 */
   {  97, 227.96, 227.26, 227.78 }, /* 2017-05-23 */
   {  98, 228.42, 227.66, 228.31 }, /* 2017-05-24 */
   {  99, 229.7, 228.64, 229.4 }, /* 2017-05-25 */
   { 100, 229.53, 229.1, 229.35 }, /* 2017-05-26 */
   /* bars 227..250 */
   { 227, 249.86, 249.14, 249.36 }, /* 2017-11-27 */
   { 228, 251.92, 249.77, 251.89 }, /* 2017-11-28 */
   { 229, 252.62, 251.25, 251.74 }, /* 2017-11-29 */
   { 230, 254.94, 252.66, 253.94 }, /* 2017-11-30 */
   { 231, 254.23, 249.87, 253.41 }, /* 2017-12-01 */
   { 232, 255.65, 253.05, 253.11 }, /* 2017-12-04 */
   { 233, 254.07, 252.05, 252.2 }, /* 2017-12-05 */
   { 234, 252.71, 251.74, 252.24 }, /* 2017-12-06 */
   { 235, 253.38, 251.96, 253.04 }, /* 2017-12-07 */
   { 236, 254.43, 253, 254.42 }, /* 2017-12-08 */
   { 237, 255.25, 254.39, 255.19 }, /* 2017-12-11 */
   { 238, 256.15, 255.22, 255.64 }, /* 2017-12-12 */
   { 239, 256.38, 255.51, 255.61 }, /* 2017-12-13 */
   { 240, 256.06, 254.51, 254.56 }, /* 2017-12-14 */
   { 241, 257.19, 255.6, 256.68 }, /* 2017-12-15 */
   { 242, 258.7, 258.1, 258.31 }, /* 2017-12-18 */
   { 243, 258.63, 257.24, 257.32 }, /* 2017-12-19 */
   { 244, 258.44, 256.86, 257.18 }, /* 2017-12-20 */
   { 245, 258.49, 257.44, 257.71 }, /* 2017-12-21 */
   { 246, 257.77, 257.06, 257.65 }, /* 2017-12-22 */
   { 247, 257.58, 257.04, 257.34 }, /* 2017-12-26 */
   { 248, 257.86, 257.16, 257.46 }, /* 2017-12-27 */
   { 249, 258.04, 257.59, 257.99 }, /* 2017-12-28 */
   { 250, 258.65, 256.81, 257.02 }, /* 2017-12-29 */
   /* bars 483..501 */
   { 483, 273.59, 270.77, 272.52 }, /* 2018-12-03 */
   { 484, 272.08, 263.35, 263.69 }, /* 2018-12-04 */
   { 485, 263.41, 256.07, 263.29 }, /* 2018-12-06 */
   { 486, 264.63, 256.25, 257.17 }, /* 2018-12-07 */
   { 487, 258.72, 252.34, 257.66 }, /* 2018-12-10 */
   { 488, 261.37, 256.11, 257.72 }, /* 2018-12-11 */
   { 489, 262.47, 258.93, 259.01 }, /* 2018-12-12 */
   { 490, 260.99, 257.71, 258.93 }, /* 2018-12-13 */
   { 491, 257.62, 253.54, 254.15 }, /* 2018-12-14 */
   { 492, 254.32, 247.37, 249.16 }, /* 2018-12-17 */
   { 493, 251.69, 247.13, 248.89 }, /* 2018-12-18 */
   { 494, 253.1, 243.3, 245.16 }, /* 2018-12-19 */
   { 495, 245.51, 238.71, 241.17 }, /* 2018-12-20 */
   { 496, 245.07, 235.52, 236.23 }, /* 2018-12-21 */
   { 497, 236.36, 229.92, 229.99 }, /* 2018-12-24 */
   { 498, 241.61, 229.42, 241.61 }, /* 2018-12-26 */
   { 499, 243.68, 234.52, 243.46 }, /* 2018-12-27 */
   { 500, 246.73, 241.87, 243.15 }, /* 2018-12-28 */
   { 501, 245.54, 242.87, 245.28 }, /* 2018-12-31 */
};
#define NB_CHOP_SKENDER ((int)(sizeof(chopSkender)/sizeof(ChopBar)))

/* want[0] is CHOP (raw box), want[1] CHOPTR (true box), both the exact-
 * rational value rounded once. Skender's chop.standard.json computes the
 * true box and agrees with want[1] to <= 3 ulp. */
typedef struct { int bar; double want[2]; } ChopGolden;

static const ChopGolden chopSkenderGold[] =
{
   {  14, { 69.99669727537056,  69.99669727537056  } },
   {  15, { 56.074214139237874, 56.074214139237874 } },
   {  27, { 48.25310513371683,  47.45989729273672  } },
   { 100, { 43.02417433799121,  43.02417433799121  } },
   { 241, { 48.662180087036774, 46.62420038042452  } },
   { 250, { 53.33453739702018,  53.33453739702018  } },
   { 497, { 30.515720874698143, 30.122309601448865 } },
   { 501, { 38.65260907128509,  38.65260907128509  } },
};
#define NB_CHOP_SKENDER_GOLD ((int)(sizeof(chopSkenderGold)/sizeof(ChopGolden)))

/* Leg 3b. The issue's 1200-bar synthetic (chopBuildSynthetic), n = 14. The
 * raw-box column matches ta4j 0.22.6 and pandas-ta-classic 0.6.52, the true-
 * box column trading-signals 8.3.0 and LEAN on true bars, each to <= 6 ulp.
 * Their arms are in ta-lib-oracles b16642f. */
static const ChopGolden chopSynGold[] =
{
   {   14, { 46.777333236499395, 43.81179212997314  } },
   {   15, { 53.320776005622555, 50.34525800988452  } },
   {   50, { 24.41891896967171,  24.41891896967171  } },
   {  100, { 40.1855490876114,   36.82992839622377  } },
   {  250, { 34.61927575398301,  34.083484363106905 } },
   {  500, { 39.11660458444966,  39.11660458444966  } },
   {  800, { 26.376634374504057, 26.376634374504057 } },
   { 1000, { 35.80159835584787,  35.80159835584787  } },
   { 1199, { 36.9231984900254,   33.67828050077823  } },
};
#define NB_CHOP_SYN_GOLD ((int)(sizeof(chopSynGold)/sizeof(ChopGolden)))

/* A golden row whose two columns differ by at least this is a gap row. */
#define CHOP_GAP_MIN 0.39

static TA_Real g_synH[CHOP_CAP], g_synL[CHOP_CAP], g_synC[CHOP_CAP];
static TA_Real g_stH[CHOP_CAP], g_stL[CHOP_CAP], g_stC[CHOP_CAP];

static int g_chopDiffCmp;
static int g_chopPubCmp;
static int g_chopGoldCmp;
static int g_chopGapRows;
static int g_chopEdgeCmp;
static int g_chopAliasCmp;
static int g_chopRangeCmp;

/**** Local functions declarations. ****/
static ErrorNumber chop_run( const ChopVariant *v, int startIdx, int endIdx,
                             const TA_Real *h, const TA_Real *l, const TA_Real *c,
                             int period, int nbBars, int route,
                             TA_Integer *begIdx, TA_Integer *nbElement, TA_Real *out );
static ErrorNumber test_chop_differential( const ChopVariant *v, const char *tag,
                                           const TA_Real *h, const TA_Real *l,
                                           const TA_Real *c, int nbBars, int onContract );
static ErrorNumber test_chop_published( const ChopVariant *v );
static ErrorNumber test_chop_goldens( const ChopVariant *v );
static ErrorNumber test_chop_edges( const ChopVariant *v );
static ErrorNumber test_chop_lookback( const ChopVariant *v );
static ErrorNumber test_chop_aliasing( const ChopVariant *v, const char *tag,
                                       const TA_Real *h, const TA_Real *l,
                                       const TA_Real *c, int nbBars );
static ErrorNumber test_chop_range( const ChopVariant *v, const TA_Real *h,
                                    const TA_Real *l, const TA_Real *c, int nbBars );

static void chopBuildSynthetic( void )
{
   int i;
   for( i = 0; i < CHOP_SYN_NB; i++ )
   {
      double x = (double)i;
      g_synC[i] = 100.0 + 10.0*sin(x/7.0) + 0.15*x + 2.0*sin(x/2.3);
      g_synH[i] = g_synC[i] + 1.0 + 0.5*fabs(sin(x/3.0));
      g_synL[i] = g_synC[i] - 1.0 - 0.5*fabs(cos(x/5.0));
   }
}

/* A jittered geometric staircase: each bar spans exactly from the previous
 * close up to its own, so raw and true boxes coincide and the exact value is
 * 0 everywhere. */
static void chopBuildStaircase( void )
{
   int i;
   g_stC[0] = 100.0;
   g_stH[0] = 100.0;
   g_stL[0] = 99.9;
   for( i = 1; i < CHOP_STAIR_NB; i++ )
   {
      g_stC[i] = g_stC[i-1] * ( 1.001 + 0.0005 * (double)( ( i * 37 ) % 11 ) / 11.0 );
      g_stH[i] = g_stC[i];
      g_stL[i] = g_stC[i-1];
   }
}

/**** Global functions definitions. ****/
ErrorNumber test_func_chop( TA_History *history )
{
   ErrorNumber err;
   int nbBars = (int)history->nbBars;
   int k;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_chopDiffCmp = g_chopPubCmp = g_chopGoldCmp = g_chopGapRows = 0;
   g_chopEdgeCmp = g_chopAliasCmp = g_chopRangeCmp = 0;

   chopBuildSynthetic();
   chopBuildStaircase();

   for( k = 0; k < 2; k++ )
   {
      const ChopVariant *v = &chopVariants[k];

      err = test_chop_differential( v, "TA_SREF", history->high, history->low,
                                    history->close, nbBars, 1 );
      if( err != TA_TEST_PASS ) return err;
      err = test_chop_differential( v, "synthetic", g_synH, g_synL, g_synC,
                                    CHOP_SYN_NB, 1 );
      if( err != TA_TEST_PASS ) return err;
      err = test_chop_differential( v, "staircase", g_stH, g_stL, g_stC,
                                    CHOP_STAIR_NB, 1 );
      if( err != TA_TEST_PASS ) return err;

      err = test_chop_published( v );
      if( err != TA_TEST_PASS ) return err;
      err = test_chop_goldens( v );
      if( err != TA_TEST_PASS ) return err;
      err = test_chop_edges( v );
      if( err != TA_TEST_PASS ) return err;
      err = test_chop_lookback( v );
      if( err != TA_TEST_PASS ) return err;

      err = test_chop_aliasing( v, "TA_SREF", history->high, history->low,
                                history->close, nbBars );
      if( err != TA_TEST_PASS ) return err;
      err = test_chop_aliasing( v, "synthetic", g_synH, g_synL, g_synC, 300 );
      if( err != TA_TEST_PASS ) return err;

      err = test_chop_range( v, history->high, history->low, history->close, nbBars );
      if( err != TA_TEST_PASS ) return err;
      /* TA_SREF's window sums are exact, so a running total would pass there. */
      err = test_chop_range( v, g_synH, g_synL, g_synC, 400 );
      if( err != TA_TEST_PASS ) return err;
   }

   if( g_chopDiffCmp == 0 || g_chopPubCmp != 2*20 || g_chopGapRows < 2*3
       || g_chopGoldCmp != 2*(NB_CHOP_SKENDER_GOLD + NB_CHOP_SYN_GOLD)
       || g_chopEdgeCmp == 0 || g_chopAliasCmp == 0 || g_chopRangeCmp == 0 )
   {
      printf( "CHOP Fail: a leg compared nothing (diff %d, published %d, goldens %d, "
              "gap rows %d, edges %d, alias %d, range %d)\n",
              g_chopDiffCmp, g_chopPubCmp, g_chopGoldCmp, g_chopGapRows,
              g_chopEdgeCmp, g_chopAliasCmp, g_chopRangeCmp );
      return TA_CHOP_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* One call, replayed on every language server when `route` is set. */
static ErrorNumber chop_run( const ChopVariant *v, int startIdx, int endIdx,
                             const TA_Real *h, const TA_Real *l, const TA_Real *c,
                             int period, int nbBars, int route,
                             TA_Integer *begIdx, TA_Integer *nbElement, TA_Real *out )
{
   TA_RetCode retCode;

   retCode = v->fn( startIdx, endIdx, h, l, c, period, begIdx, nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "%s Fail [N=%d]: rc=%d\n", v->name, period, (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   if( route && server_verify_active() )
   {
      double optIn[1];
      ErrorNumber e;
      int before = server_verify_value_comparisons();

      optIn[0] = (double)period;
      e = server_verify( v->name, startIdx, endIdx, nbBars,
                         retCode, *begIdx, *nbElement,
                         (const TA_Real*[]){ h, l, c, NULL },
                         optIn, 1,
                         (const TA_Real*[]){ out, NULL }, NULL );
      if( e != TA_TEST_PASS )
         return e;
      if( server_verify_value_comparisons() == before )
      {
         printf( "%s [N=%d]: compared no server despite live pipes\n", v->name, period );
         return TA_SV_ROUTED_VACUOUS;
      }
   }

   return TA_TEST_PASS;
}

/* (1) Reference: TR from TA_TRANGE (CHOP) or hand-built true highs/lows
 * (CHOPTR: TA-Lib ships no element-wise max of two series), window sums from
 * TA_SUM, the box from TA_MAX/TA_MIN. Every TR-derived series starts at bar
 * 1, so its window ending at bar t starts at output index t-period; the raw
 * H/L window starts at t-period+1. */
static ErrorNumber test_chop_differential( const ChopVariant *v, const char *tag,
                                           const TA_Real *h, const TA_Real *l,
                                           const TA_Real *c, int nbBars, int onContract )
{
   static TA_Real tr[CHOP_CAP], th[CHOP_CAP], tl[CHOP_CAP];
   static TA_Real sum[CHOP_CAP], hi[CHOP_CAP], lo[CHOP_CAP], out[CHOP_CAP];
   TA_Integer b1, n1, b2, n2, b3, n3, begIdx, nbElement;
   int period, i, t, boxOff;
   ErrorNumber err;

   if( TA_TRANGE( 0, nbBars-1, h, l, c, &b1, &n1, tr ) != TA_SUCCESS || b1 != 1 )
   {
      printf( "%s differential [%s]: TA_TRANGE failed\n", v->name, tag );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 1; i < nbBars; i++ )
   {
      th[i-1] = ( c[i-1] > h[i] ) ? c[i-1] : h[i];
      tl[i-1] = ( c[i-1] < l[i] ) ? c[i-1] : l[i];
   }

   for( period = 2; period <= 100; period++ )
   {
      if( TA_SUM( 0, nbBars-2, tr, period, &b1, &n1, sum ) != TA_SUCCESS ||
          ( v->trueBox
            ? ( TA_MAX( 0, nbBars-2, th, period, &b2, &n2, hi ) != TA_SUCCESS ||
                TA_MIN( 0, nbBars-2, tl, period, &b3, &n3, lo ) != TA_SUCCESS )
            : ( TA_MAX( 0, nbBars-1, h, period, &b2, &n2, hi ) != TA_SUCCESS ||
                TA_MIN( 0, nbBars-1, l, period, &b3, &n3, lo ) != TA_SUCCESS ) ) )
      {
         printf( "%s differential [%s N=%d]: reference compose failed\n",
                 v->name, tag, period );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      boxOff = v->trueBox ? period : period - 1;

      err = chop_run( v, 0, nbBars-1, h, l, c, period, nbBars, 0,
                      &begIdx, &nbElement, out );
      if( err != TA_TEST_PASS )
         return err;
      if( begIdx != period || nbElement != nbBars - period )
      {
         printf( "%s differential Fail [%s N=%d]: (%d,%d) expected (%d,%d)\n",
                 v->name, tag, period, begIdx, nbElement, period, nbBars - period );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( t = period; t < nbBars; t++ )
      {
         double s    = sum[t-period];
         double r    = hi[t-boxOff] - lo[t-boxOff];
         double want = ( r <= 0.0 || s <= 0.0 )
                       ? 100.0 : 100.0 * ( log10( s / r ) / log10( (double)period ) );
         double got  = out[t-begIdx];
         double e;
         const char *mode;

         g_chopDiffCmp++;
         if( !checkOracleValue( got, want, CHOP_DIFF_REL, CHOP_DIFF_ABS, &e, &mode ) )
         {
            printf( "%s differential Fail [%s N=%d] at bar %d: got %.17g expected "
                    "%.17g (%s err %.3g)\n", v->name, tag, period, t, got, want,
                    mode, e );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( onContract && ( !( got >= -CHOP_LOW_SLACK )
                             || ( v->trueBox && got > 100.0 + CHOP_HIGH_SLACK ) ) )
         {
            printf( "%s bound Fail [%s N=%d] at bar %d: %.17g is outside its "
                    "on-contract range\n", v->name, tag, period, t, got );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) */
static ErrorNumber test_chop_published( const ChopVariant *v )
{
   static TA_Real out[CHOP_CAP];
   TA_Integer begIdx, nbElement;
   ErrorNumber err;
   int i;

   err = chop_run( v, 0, 23, chopPubHigh, chopPubLow, chopPubClose, 14, 24, 1,
                   &begIdx, &nbElement, out );
   if( err != TA_TEST_PASS )
      return err;
   if( begIdx != 14 || nbElement != 10 )
   {
      printf( "%s published Fail: (%d,%d) expected (14,10)\n", v->name, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < 10; i++ )
   {
      double e;
      const char *mode;

      g_chopPubCmp += 2;
      if( fabs( out[i] - chopPubPrinted[i] ) > 0.005
          || !checkOracleValue( out[i], chopPubExact[i], CHOP_GOLD_REL, CHOP_GOLD_ABS,
                                &e, &mode ) )
      {
         printf( "%s published Fail at bar %d: got %.17g, printed %.2f, exact %.17g\n",
                 v->name, 14 + i, out[i], chopPubPrinted[i], chopPubExact[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber chop_check_golden( const ChopVariant *v, const char *tag,
                                      const ChopGolden *g, double got )
{
   int k = v->trueBox;
   double e;
   const char *mode;

   g_chopGoldCmp++;
   if( fabs( g->want[0] - g->want[1] ) >= CHOP_GAP_MIN )
      g_chopGapRows++;
   if( !checkOracleValue( got, g->want[k], CHOP_GOLD_REL, CHOP_GOLD_ABS, &e, &mode ) )
   {
      printf( "%s golden Fail [%s] at bar %d: got %.17g expected %.17g (%s err %.3g)%s\n",
              v->name, tag, g->bar, got, g->want[k], mode, e,
              fabs( got - g->want[1-k] ) < 1e-9 ? " -- that is the other box" : "" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (3) Each Skender golden is computed from the 15 bars ending at it, so each
 * contiguous run of chopSkender is fed as its own series. */
static ErrorNumber test_chop_goldens( const ChopVariant *v )
{
   static TA_Real h[CHOP_CAP], l[CHOP_CAP], c[CHOP_CAP], out[CHOP_CAP];
   TA_Integer begIdx, nbElement;
   ErrorNumber err;
   int start, end, i, g;

   for( start = 0; start < NB_CHOP_SKENDER; start = end )
   {
      for( end = start + 1;
           end < NB_CHOP_SKENDER && chopSkender[end].bar == chopSkender[end-1].bar + 1;
           end++ ) {}
      for( i = start; i < end; i++ )
      {
         h[i-start] = chopSkender[i].high;
         l[i-start] = chopSkender[i].low;
         c[i-start] = chopSkender[i].close;
      }

      err = chop_run( v, 0, end-start-1, h, l, c, 14, end-start, 1,
                      &begIdx, &nbElement, out );
      if( err != TA_TEST_PASS )
         return err;

      for( g = 0; g < NB_CHOP_SKENDER_GOLD; g++ )
      {
         int rel = chopSkenderGold[g].bar - chopSkender[start].bar;
         if( rel < 0 || rel >= end - start )
            continue;
         if( rel < begIdx || rel - begIdx >= nbElement )
         {
            printf( "%s golden Fail [Skender]: bar %d has no output\n",
                    v->name, chopSkenderGold[g].bar );
            return TA_CHOP_VACUOUS;
         }
         err = chop_check_golden( v, "Skender 3.0.0 SPY", &chopSkenderGold[g],
                                  out[rel-begIdx] );
         if( err != TA_TEST_PASS )
            return err;
      }
   }

   err = chop_run( v, 0, CHOP_SYN_NB-1, g_synH, g_synL, g_synC, 14, CHOP_SYN_NB, 1,
                   &begIdx, &nbElement, out );
   if( err != TA_TEST_PASS )
      return err;
   for( g = 0; g < NB_CHOP_SYN_GOLD; g++ )
   {
      err = chop_check_golden( v, "synthetic", &chopSynGold[g],
                               out[chopSynGold[g].bar - begIdx] );
      if( err != TA_TEST_PASS )
         return err;
   }

   return TA_TEST_PASS;
}

/* Every output from `firstBar` to `lastBar` must equal `want` exactly. */
static ErrorNumber chop_expect_exact( const ChopVariant *v, const char *tag,
                                      const TA_Real *out, int begIdx, int nbElement,
                                      int firstBar, int lastBar, double want )
{
   int t;
   if( firstBar < begIdx || lastBar - begIdx >= nbElement )
   {
      printf( "%s edge Fail [%s]: bars %d..%d outside the output\n",
              v->name, tag, firstBar, lastBar );
      return TA_CHOP_VACUOUS;
   }
   for( t = firstBar; t <= lastBar; t++ )
   {
      g_chopEdgeCmp++;
      if( out[t-begIdx] != want )
      {
         printf( "%s edge Fail [%s] at bar %d: got %.17g expected exactly %.17g\n",
                 v->name, tag, t, out[t-begIdx], want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (4) */
static ErrorNumber test_chop_edges( const ChopVariant *v )
{
   static TA_Real h[128], l[128], c[128], out[128];
   TA_Integer begIdx, nbElement;
   ErrorNumber err;
   double maxOut, e;
   const char *mode;
   int i, period;

   /* All bars identical: 0/0 on every window. */
   for( i = 0; i < 40; i++ )
      h[i] = l[i] = c[i] = 100.0;
   err = chop_run( v, 0, 39, h, l, c, 14, 40, 1, &begIdx, &nbElement, out );
   if( err == TA_TEST_PASS )
      err = chop_expect_exact( v, "flat", out, begIdx, nbElement, 14, 39, 100.0 );
   if( err != TA_TEST_PASS ) return err;

   /* A gap into a flat window. From bar 18 the raw box is empty while the sum
    * still holds the gap (S = 20, R = 0): the raw-box guard answers 100, the
    * true box still spans the gap and answers 0. Bar 19 on is 0/0. */
   for( i = 0; i < 35; i++ )
      h[i] = l[i] = c[i] = ( i < 5 ) ? 100.0 : 120.0;
   err = chop_run( v, 0, 34, h, l, c, 14, 35, 1, &begIdx, &nbElement, out );
   if( err == TA_TEST_PASS )
      err = chop_expect_exact( v, "gap into flat", out, begIdx, nbElement,
                               14, v->trueBox ? 18 : 17, 0.0 );
   if( err == TA_TEST_PASS )
      err = chop_expect_exact( v, "gap into flat", out, begIdx, nbElement,
                               v->trueBox ? 19 : 18, 34, 100.0 );
   if( err != TA_TEST_PASS ) return err;

   /* The same gap into a 0.01 box: the raw box is not bounded by 100. */
   for( i = 5; i < 35; i++ )
   {
      h[i] = 120.01;
      l[i] = 120.0;
      c[i] = 120.005;
   }
   err = chop_run( v, 0, 34, h, l, c, 14, 35, 1, &begIdx, &nbElement, out );
   if( err != TA_TEST_PASS ) return err;
   maxOut = out[0];
   for( i = 1; i < nbElement; i++ )
      if( out[i] > maxOut )
         maxOut = out[i];
   g_chopEdgeCmp++;
   if( v->trueBox ? ( maxOut > 100.0 ) : ( maxOut < 288.2 || maxOut > 288.4 ) )
   {
      printf( "%s edge Fail [gap into narrow box]: max %.17g, expected %s\n",
              v->name, maxOut, v->trueBox ? "<= 100" : "about 288.28 (no clamp)" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* A staircase whose bars span exactly the close-to-close move: ratio 1. */
   for( i = 0; i < 60; i++ )
   {
      c[i] = h[i] = 100.0 + 0.25 * (double)i;
      l[i] = ( i == 0 ) ? 99.75 : c[i-1];
   }
   for( period = 2; period <= 40; period++ )
   {
      err = chop_run( v, 0, 59, h, l, c, period, 60, period == 14,
                      &begIdx, &nbElement, out );
      if( err == TA_TEST_PASS )
         err = chop_expect_exact( v, "staircase", out, begIdx, nbElement,
                                  period, 59, 0.0 );
      if( err != TA_TEST_PASS ) return err;
   }

   /* A constant box: S/R is exactly the period, and 100*(log10(q)/log10(n))
    * must return exactly 100 where (100*log10(q))/log10(n) does not (n=61). */
   for( i = 0; i < 128; i++ )
   {
      c[i] = 100.0;
      h[i] = 100.0 * ( 1.0 + 1e-3 );
      l[i] = 100.0 * ( 1.0 - 1e-3 );
   }
   err = chop_run( v, 0, 127, h, l, c, 61, 128, 1, &begIdx, &nbElement, out );
   if( err == TA_TEST_PASS )
      err = chop_expect_exact( v, "constant box", out, begIdx, nbElement, 61, 127, 100.0 );
   if( err != TA_TEST_PASS ) return err;

   /* The published bars scaled by 2^-60: the scaling is exact, so every value
    * is bit-identical to the unscaled one. An epsilon band on either half of
    * the guard answers 100 here. */
   {
      static TA_Real ref[16];
      TA_Integer begRef, nbRef;
      double scale = ldexp( 1.0, -60 );
      if( v->fn( 0, 23, chopPubHigh, chopPubLow, chopPubClose, 14,
                 &begRef, &nbRef, ref ) != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      for( i = 0; i < 24; i++ )
      {
         h[i] = chopPubHigh[i] * scale;
         l[i] = chopPubLow[i] * scale;
         c[i] = chopPubClose[i] * scale;
      }
      err = chop_run( v, 0, 23, h, l, c, 14, 24, 1, &begIdx, &nbElement, out );
      if( err != TA_TEST_PASS ) return err;
      for( i = 0; i < nbRef; i++ )
      {
         g_chopEdgeCmp++;
         if( begIdx != begRef || nbElement != nbRef
             || memcmp( &out[i], &ref[i], sizeof(TA_Real) ) != 0 )
         {
            printf( "%s edge Fail [tiny prices] out %d: %.17g, unscaled %.17g\n",
                    v->name, i, out[i], ref[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* Off contract, a close outside its own bar: negative, not clamped. */
   h[0] = l[0] = c[0] = 100.0;
   h[1] = l[1] = 100.0; c[1] = 150.0;
   for( i = 2; i < 20; i++ )
   {
      h[i] = 151.0;
      l[i] = 150.0;
      c[i] = 150.5;
   }
   err = chop_run( v, 0, 19, h, l, c, 14, 20, 1, &begIdx, &nbElement, out );
   if( err != TA_TEST_PASS ) return err;
   g_chopEdgeCmp++;
   if( begIdx != 14 || !checkOracleValue( out[0], -51.79411071990855,
                                          CHOP_GOLD_REL, CHOP_GOLD_ABS, &e, &mode ) )
   {
      printf( "%s edge Fail [close outside bar]: begIdx %d, bar 14 %.17g expected "
              "-51.79411071990855\n", v->name, begIdx, out[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   err = chop_expect_exact( v, "close outside bar", out, begIdx, nbElement, 15, 19, 100.0 );
   if( err != TA_TEST_PASS ) return err;

   /* Off contract, S == 0 < R: log10(0) without the sum half of the guard. */
   h[1] = l[1] = 100.0; c[1] = 200.0;
   for( i = 2; i < 20; i++ )
      h[i] = l[i] = c[i] = 200.0;
   err = chop_run( v, 0, 19, h, l, c, 14, 20, 1, &begIdx, &nbElement, out );
   if( err == TA_TEST_PASS )
      err = chop_expect_exact( v, "zero sum, open box", out, begIdx, nbElement,
                               14, 19, 100.0 );
   return err;
}

/* (5) */
static ErrorNumber test_chop_lookback( const ChopVariant *v )
{
   static const int periods[] = { 2, 3, 14, 99999, 100000 };
   TA_Real out[4];
   TA_Integer begIdx, nbElement;
   unsigned int k;

   for( k = 0; k < sizeof(periods)/sizeof(periods[0]); k++ )
   {
      if( v->lookback( periods[k] ) != periods[k] )
      {
         printf( "%s lookback Fail: %d for period %d\n", v->name,
                 v->lookback( periods[k] ), periods[k] );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }
   if( v->lookback( 1 ) != -1 || v->lookback( 100001 ) != -1 )
   {
      printf( "%s lookback Fail: an out-of-range period was accepted\n", v->name );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* endIdx == lookback: exactly one output, at the lookback. One bar less:
    * none. */
   if( v->fn( 0, 2, chopPubHigh, chopPubLow, chopPubClose, 2,
              &begIdx, &nbElement, out ) != TA_SUCCESS
       || begIdx != 2 || nbElement != 1
       || v->fn( 0, 1, chopPubHigh, chopPubLow, chopPubClose, 2,
                 &begIdx, &nbElement, out ) != TA_SUCCESS
       || nbElement != 0 )
   {
      printf( "%s lookback Fail: the first output is not at bar 2 for period 2\n",
              v->name );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   return TA_TEST_PASS;
}

/* (6) The body re-reads the window every bar, reaching back to the close
 * before it, so a store landing under a later read shows up here. */
static ErrorNumber test_chop_aliasing( const ChopVariant *v, const char *tag,
                                       const TA_Real *h, const TA_Real *l,
                                       const TA_Real *c, int nbBars )
{
   static TA_Real ref[CHOP_CAP], buf[3][CHOP_CAP];
   TA_Integer begIdx, nbElement, begA, nbA;
   int period, which, i;

   for( period = 2; period <= 100 && period < nbBars; period++ )
   {
      if( v->fn( 0, nbBars-1, h, l, c, period, &begIdx, &nbElement, ref ) != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      for( which = 0; which < 3; which++ )
      {
         memcpy( buf[0], h, sizeof(TA_Real) * nbBars );
         memcpy( buf[1], l, sizeof(TA_Real) * nbBars );
         memcpy( buf[2], c, sizeof(TA_Real) * nbBars );

         if( v->fn( 0, nbBars-1, buf[0], buf[1], buf[2], period,
                    &begA, &nbA, buf[which] ) != TA_SUCCESS
             || begA != begIdx || nbA != nbElement )
         {
            printf( "%s alias Fail [%s N=%d input %d]: shape\n", v->name, tag,
                    period, which );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_chopAliasCmp++;
            if( memcmp( &buf[which][i], &ref[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "%s alias Fail [%s N=%d input %d] out %d: %.17g vs %.17g\n",
                       v->name, tag, period, which, i, buf[which][i], ref[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) Every value is recomputed from its own window, so no range may move
 * one by a single bit. */
static ErrorNumber test_chop_range( const ChopVariant *v, const TA_Real *h,
                                    const TA_Real *l, const TA_Real *c, int nbBars )
{
   static const int periods[] = { 2, 14, 61 };
   static TA_Real ref[CHOP_CAP], out[CHOP_CAP];
   TA_Integer begRef, nbRef, begIdx, nbElement;
   unsigned int k;
   int s, w, t;

   for( k = 0; k < sizeof(periods)/sizeof(periods[0]); k++ )
   {
      int period = periods[k];
      if( v->fn( 0, nbBars-1, h, l, c, period, &begRef, &nbRef, ref ) != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      for( s = 0; s < nbBars; s++ )
      {
         static const int widths[] = { 0, 1, 7, 1000000 };
         for( w = 0; w < 4; w++ )
         {
            int e = s + widths[w];
            if( e > nbBars - 1 )
               e = nbBars - 1;
            int first = ( s < period ) ? period : s;
            if( v->fn( s, e, h, l, c, period, &begIdx, &nbElement, out ) != TA_SUCCESS
                || begIdx != ( e >= first ? first : 0 )
                || nbElement != ( e >= first ? e - first + 1 : 0 ) )
            {
               printf( "%s range Fail [N=%d] [%d..%d]: (%d,%d)\n", v->name, period,
                       s, e, begIdx, nbElement );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            for( t = 0; t < nbElement; t++ )
            {
               g_chopRangeCmp++;
               if( memcmp( &out[t], &ref[begIdx + t - begRef], sizeof(TA_Real) ) != 0 )
               {
                  printf( "%s range Fail [N=%d] [%d..%d] at bar %d\n", v->name,
                          period, s, e, begIdx + t );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   return TA_TEST_PASS;
}
