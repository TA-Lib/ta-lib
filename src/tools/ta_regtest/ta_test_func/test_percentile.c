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
 *  090426 MF,CC  First version (issue #368).
 *  092226 MF,CC  Legs across the scan/binary threshold and the float tiers (issue #435).
 *  092326 MF,CC  Leg 10 across every size threshold, on trend corpora (issue #435).
 */

/* Description:
 *
 *   Test TA_PERCENTILE (rolling percentile, nearest-rank method).
 *
 *   Legs:
 *     1. EXTERNAL ORACLES: two independent nearest-rank implementations, in two
 *        languages, agreeing BIT-FOR-BIT with each other and with TA_PERCENTILE
 *        over every value of the frozen corpus. Frozen at tolerance ZERO -- the
 *        output is an input element copied verbatim, so there is no arithmetic
 *        to round and a tolerance could only hide a wrong rank. See
 *        percentileOracle below.
 *     2. The PUBLISHED worked example, whose whole point is that it is checkable
 *        by hand: {15,20,35,40,50} at eight percentages.
 *     3. FREE in-tree formula differential, no new reference code: P=0 must be
 *        bit-equal to TA_MIN and P=100 to TA_MAX at the same period, with the
 *        same anchor and count. Two already-xlang-hash-verified shipped
 *        functions pinning both clamp arms -- and unlike leg 4 this catches a
 *        wrong rank formula at the boundaries.
 *     4. INTERNAL differential against a naive per-bar re-sort, bitwise, over a
 *        grid of periods x percentages on four corpora. Its subject is the
 *        optimization: that the incrementally maintained pair of buffers equals
 *        a fresh sort of the whole window. The rank rule is transcribed here a
 *        second time rather than shared, so a respelling on one side does fire
 *        -- but a wrong rule written into both would not, which is what leg 5
 *        is for. The reference sorts with an explicitly STABLE insertion sort,
 *        never qsort: the C standard does not require qsort to be stable, so
 *        the +0.0/-0.0 tie order would otherwise be implementation-defined.
 *     5. The EXACT-RANK class, asserted against integer arithmetic rather than
 *        against any float: at (n=100,P=7), (50,14), (25,28) and their kin,
 *        ceil(P/100.0*n) is one order statistic HIGHER than ceil(P*n/100.0)
 *        because P/100 is inexact in binary64. This is the leg that fails if
 *        someone "simplifies" the rank expression, and it asserts the two
 *        spellings really do disagree there before trusting the comparison.
 *     6. Edges: a constant window, a +-0.0 window compared BITWISE, the two
 *        smallest periods, startIdx == endIdx == lookback, a range too short
 *        to produce anything, and the top of the period range.
 *     7. In-place aliasing (outReal == inReal), bitwise, over two corpora and
 *        every period. Safe only because both scratch buffers hold copies and
 *        inReal is never re-read below the current bar; ASan cannot see the
 *        failure this would be.
 *     8. The startIdx/endIdx range sweep, in the EXACT class: every bar's window
 *        is rebuilt from its own bars, so no range may move a value at all.
 *     9. A POSITIVE assertion that optInPercentile carries TA_OPTIN_IS_PERCENT.
 *        test_abstract.c only compares C against each server, so a flag dropped
 *        on both sides is invisible there: 0 == 0 passes. PERCENTILE is the
 *        flag's first user, so this is the only check that it is set at all.
 *    10. Windows of one retained value, either side of every other size a body
 *        switches strategy at (32, 64, 100, 256, 300 and 2048), and of 113,
 *        where a merge-sorted first window ends in a run of one, BITWISE
 *        against a fresh sort of each window keyed on (value, age), over a
 *        tick-grid walk, a series of -0.0, 0.0 and +-1, a rising and a falling
 *        trend made of strict runs, noise and flats, a strict fall and a
 *        strict rise through six zeros of alternating sign, trends of random
 *        length and slope, and a first window of zeros of both signs: the
 *        order inside a run of equal values reaches the output only as the
 *        sign of a zero, and the trends are what reach the monotone and ring
 *        paths. Routed to the language servers, also from a later anchor and
 *        over a range too short to output, through the C stream tier, also
 *        opened on a later bar, and through every single-precision tier,
 *        which is a separate body.
 *
 *   Cross-language value coverage comes from server_verify in legs 1, 2 and
 *   10, --codegen and --xlang-hash. All three compare against this library,
 *   so none can catch a wrong formula.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** External variables declarations. ****/
extern double gDataClose[];

/**** Local declarations. ****/
#define PCTL_CAP     2100
#define PCTL_GD_NB   1000
#define PCTL_SYN_NB  2000
#define PCTL_BIG_NB  1150   /* enough for one period-1000 block, cheap to re-sort */
#define PCTL_WIDE_NB (2049+400)
#define PCTL_WIDE_CMP        267792
#define PCTL_WIDE_STREAM_CMP 264096

typedef struct { int period; double pct; int bar; double want; } PercentileGolden;

/* Goldens captured by ta-lib-oracles/capture_368_percentile.py on the 252-bar
 * TA_SREF close series (TA_SREF_close_daily_ref_0_PRIV), at %.17g, which
 * round-trips to the same double. `bar` is the ABSOLUTE bar index; the output
 * index is bar - begIdx.
 *
 * TWO independent implementations, in two languages, each driven on this exact
 * series (2026-09-04) and each agreeing BIT-FOR-BIT with the other over every
 * value of every configuration below (the capture script refuses to print a
 * table when any count is non-zero):
 *   1. numpy 2.5.1 `np.percentile(..., method='inverted_cdf')` over a
 *      sliding_window_view -- Hyndman & Fan type 1, the textbook nearest rank.
 *   2. PyneCore 6.8.14 `ta.percentile_nearest_rank` -- a Pine runtime, i.e. the
 *      semantics the original request named. It maintains its window
 *      incrementally under a tolerant (1e-10) tie order, where numpy sorts each
 *      window from scratch exactly; on this corpus no two closes are within
 *      1e-10 without being equal, so the two are directly comparable.
 * A third arm checks the rank INDEX at one slice only: pandas-ta-classic 0.6.52
 * `ta.quantile(q=0.5)` is the linear-interpolation family, which coincides with
 * nearest rank exactly when the length is odd -- measured bit-exact at
 * n = 3, 5, 21, 31.
 *
 * numpy is NOT an oracle at an exact rank: it takes the rank from n*(P/100)
 * where the rule is ceil(P*n/100), so it runs one order statistic high wherever
 * P*n/100 is an integer. Only twelve values of P ever diverge and no row here
 * uses one; leg 5 covers that class against integer arithmetic instead.
 */
static const PercentileGolden percentileOracle[] =
{
   {    2,  50.00,   1,                   91.5 },
   {    2,  50.00,   2,                 94.375 },
   {    2,  50.00,  25,                 81.375 },
   {    2,  50.00,  84,                    106 },
   {    2,  50.00, 126,                    131 },
   {    2,  50.00, 131,                 137.81 },
   {    2,  50.00, 168,                 125.87 },
   {    2,  50.00, 251,                 107.87 },
   {    3,  50.00,   2,                 94.375 },
   {    3,  50.00,   3,     94.814999999999998 },
   {    3,  50.00,  25,                     83 },
   {    3,  50.00,  85,                    106 },
   {    3,  50.00, 127,                 132.25 },
   {    3,  50.00, 131,                 137.81 },
   {    3,  50.00, 168,                 125.87 },
   {    3,  50.00, 251,                 108.75 },
   {    5,  25.00,   4,     93.780000000000001 },
   {    5,  25.00,   5,                 94.375 },
   {    5,  25.00,  25,                     83 },
   {    5,  25.00,  86,                    106 },
   {    5,  25.00, 128,                    131 },
   {    5,  25.00, 132,                 137.25 },
   {    5,  25.00, 169,                 124.56 },
   {    5,  25.00, 251,                 108.75 },
   {   20,  45.00,  19,     92.469999999999999 },
   {   20,  45.00,  20,     92.469999999999999 },
   {   20,  45.00,  40,                 84.875 },
   {   20,  45.00,  96,                106.125 },
   {   20,  45.00, 135,                    131 },
   {   20,  45.00, 174,                 124.44 },
   {   20,  45.00, 251,                  109.2 },
   {   20,  60.00,  19,     92.814999999999998 },
   {   20,  60.00,  20,     92.814999999999998 },
   {   20,  60.00,  41,     85.814999999999998 },
   {   20,  60.00,  96,                  110.5 },
   {   20,  60.00, 135,                 132.81 },
   {   20,  60.00, 174,                 127.25 },
   {   20,  60.00, 251,                 109.75 },
   {   21,  50.00,  20,     92.530000000000001 },
   {   21,  50.00,  21,     92.530000000000001 },
   {   21,  50.00,  41,                   85.5 },
   {   21,  50.00,  97,                109.315 },
   {   21,  50.00, 134,                    131 },
   {   21,  50.00, 136,                    131 },
   {   21,  50.00, 174,                 124.56 },
   {   21,  50.00, 251,                 109.25 },
   {   30,  50.00,  29,     90.314999999999998 },
   {   30,  50.00,  30,                 89.875 },
   {   30,  50.00,  57,                 86.375 },
   {   30,  50.00, 103,                  110.5 },
   {   30,  50.00, 140,                 124.81 },
   {   30,  50.00, 142,                 128.25 },
   {   30,  50.00, 177,                    124 },
   {   30,  50.00, 251,                 108.75 },
   {   30,   0.00,  29,                 81.375 },
   {   30,   0.00,  30,                 81.375 },
   {   30,   0.00, 103,     85.939999999999998 },
   {   30,   0.00, 140,                  115.5 },
   {   30,   0.00, 145,                 122.25 },
   {   30,   0.00, 177,                 119.31 },
   {   30,   0.00, 251,                     98 },
   {   30, 100.00,  29,                   98.5 },
   {   30, 100.00,  30,                   98.5 },
   {   30, 100.00,  48,     91.439999999999998 },
   {   30, 100.00, 103,                    123 },
   {   30, 100.00, 131,                 137.88 },
   {   30, 100.00, 140,                 137.88 },
   {   30, 100.00, 177,                    135 },
   {   30, 100.00, 251,                 118.28 },
   {  100,  75.00,  99,                  97.25 },
   {  100,  75.00, 100,                   98.5 },
   {  100,  75.00, 150,                 123.19 },
   {  100,  75.00, 175,                 127.25 },
   {  100,  75.00, 179,                 128.38 },
   {  100,  75.00, 201,                 128.38 },
   {  100,  75.00, 251,                 123.19 },
};
#define NB_PCTL_ORACLE ((int)(sizeof(percentileOracle)/sizeof(PercentileGolden)))

/* Leg 2. Wikipedia's own nearest-rank worked example, reproduced by running
 * numpy 2.5.1 `inverted_cdf` on it (2026-09-04). Every value is one of the five
 * inputs, which is the property that makes the table checkable by eye. */
static const double pctlBookIn[5]  = { 15.0, 20.0, 35.0, 40.0, 50.0 };
static const double pctlBookP[8]   = { 5.0, 25.0, 30.0, 40.0, 50.0, 75.0, 95.0, 100.0 };
static const double pctlBookOut[8] = { 15.0, 20.0, 20.0, 20.0, 35.0, 40.0, 50.0, 50.0 };

/* Leg 4's grid. The percentages deliberately include the twelve where numpy's
 * inverted_cdf and the exact rank part company, since a P=60-only grid would
 * never exercise the rank expression where it is fragile. */
static const double pctlGridP[] =
{
   0.0, 1.0, 7.0, 14.0, 17.0, 25.0, 27.0, 28.0, 34.0, 45.0, 50.0,
   54.0, 55.0, 56.0, 60.0, 67.0, 68.0, 75.0, 81.0, 99.0, 100.0
};
#define NB_PCTL_P ((int)(sizeof(pctlGridP)/sizeof(double)))

static const int pctlGridN[] = { 2, 3, 5, 20, 30, 100 };
#define NB_PCTL_N ((int)(sizeof(pctlGridN)/sizeof(int)))

static const int pctlBigN[] = { 1000 };

/* Leg 5. Integer (n, P) pairs where P*n/100 is an exact integer -- the class
 * where ceil(P/100.0*n) is one order statistic too high. */
typedef struct { int period; int pct; } PercentileExactRank;
static const PercentileExactRank pctlExactRank[] =
{
   { 100,  7 }, { 100, 14 }, { 100, 28 }, { 100, 55 },
   { 100, 56 }, {  50, 14 }, {  50, 28 }, {  25, 28 },
   {  75, 68 }, { 200,  7 },
};
#define NB_PCTL_EXACT ((int)(sizeof(pctlExactRank)/sizeof(PercentileExactRank)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_pctlOracleCmp;
static int g_pctlBookCmp;
static int g_pctlExtremaCmp;
static int g_pctlDiffCmp;
static int g_pctlRankCmp;
static int g_pctlEdgeCmp;
static int g_pctlAliasCmp;
static int g_pctlFlagCmp;
static int g_pctlWideCmp;
static int g_pctlWideStreamCmp;
static int g_pctlWideFloatCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_percentile_oracle( const TA_History *history );
static ErrorNumber test_percentile_published( void );
static ErrorNumber test_percentile_extrema( const char *tag, const TA_Real *in,
                                            int nbBars, int maxPeriod );
static ErrorNumber test_percentile_differential( const char *tag, const TA_Real *in,
                                                 int nbBars, const int *periods,
                                                 int nbPeriods );
static ErrorNumber test_percentile_exact_rank( const TA_Real *in, int nbBars );
static ErrorNumber test_percentile_edges( void );
static ErrorNumber test_percentile_aliasing( const char *tag, const TA_Real *in,
                                             int nbBars, int maxPeriod );
static ErrorNumber test_percentile_range( const TA_Real *in );
static ErrorNumber test_percentile_percent_flag( void );
static ErrorNumber test_percentile_wide( void );

/* The reference sort: STABLE insertion, never qsort. Stability is what makes
 * the +-0.0 tie order a property of the input rather than of the C library. */
static void pctlSortWindow( const TA_Real *in, int t, int period, double *dest )
{
   int i, j;
   double v;

   for( i = 0; i < period; i++ )
   {
      v = in[t-period+1+i];
      j = i;
      while( j > 0 && dest[j-1] > v )
      {
         dest[j] = dest[j-1];
         j--;
      }
      dest[j] = v;
   }
}

/* The rank rule, transcribed independently of the implementation; leg 5 is
 * where the rule itself is held to arithmetic with no float in it. */
static int pctlRankFp( int period, double pct )
{
   int k = (int)ceil( pct * (double)period / 100.0 );

   if( k < 1 ) k = 1;
   if( k > period ) k = period;
   return k;
}

/* The same rank in exact integer arithmetic -- no binary64 anywhere. */
static int pctlRankInt( int period, int pct )
{
   int k = (pct*period + 99) / 100;

   if( k < 1 ) k = 1;
   if( k > period ) k = period;
   return k;
}

/* The spelling leg 5 exists to forbid. */
static int pctlRankWrong( int period, double pct )
{
   int k = (int)ceil( pct / 100.0 * (double)period );

   if( k < 1 ) k = 1;
   if( k > period ) k = period;
   return k;
}

/* A staircase with long flats: every window holds runs of equal values, which
 * is the only regime where the insert-after-the-equal-run rule is observable.
 * Built from an exact rule so nothing is transported. */
static void pctlBuildStairs( double *dest, int nb )
{
   int i;

   for( i = 0; i < nb; i++ )
      dest[i] = 40.0 + (double)( ( i / 7 ) % 11 ) * 0.5;
}

/**** Global functions definitions. ****/
ErrorNumber test_func_percentile( TA_History *history )
{
   static TA_Real stairs[PCTL_SYN_NB];
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   /* PERCENTILE has no unstable period; a leftover global setting must not
    * reach it, and the range sweep below asserts the same from the other side. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_pctlOracleCmp = g_pctlBookCmp = g_pctlExtremaCmp = 0;
   g_pctlDiffCmp = g_pctlRankCmp = g_pctlEdgeCmp = 0;
   g_pctlAliasCmp = g_pctlFlagCmp = 0;
   g_pctlWideCmp = g_pctlWideStreamCmp = g_pctlWideFloatCmp = 0;

   pctlBuildStairs( stairs, PCTL_SYN_NB );

   err = test_percentile_percent_flag();
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_published();
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_extrema( "TA_SREF close", history->close, nbBars, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_extrema( "gData close", gDataClose, PCTL_GD_NB, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_differential( "TA_SREF close", history->close, nbBars,
                                       pctlGridN, NB_PCTL_N );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_differential( "gData close", gDataClose, PCTL_GD_NB,
                                       pctlGridN, NB_PCTL_N );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_differential( "staircase", stairs, PCTL_SYN_NB,
                                       pctlGridN, NB_PCTL_N );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_differential( "gData close, long window", gDataClose,
                                       PCTL_BIG_NB, pctlBigN, 1 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_exact_rank( gDataClose, PCTL_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_aliasing( "TA_SREF close", history->close, nbBars, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_aliasing( "staircase", stairs, PCTL_SYN_NB, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_range( history->close );
   if( err != TA_TEST_PASS )
      return err;

   err = test_percentile_wide();
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_pctlOracleCmp != NB_PCTL_ORACLE || g_pctlBookCmp != 8
            || g_pctlExtremaCmp != 140656 || g_pctlDiffCmp != 403221
            || g_pctlRankCmp != 9110 || g_pctlEdgeCmp != 48406
            || g_pctlAliasCmp != 129328 || g_pctlFlagCmp != 1
            || g_pctlWideCmp != PCTL_WIDE_CMP
            || g_pctlWideStreamCmp != PCTL_WIDE_STREAM_CMP
            || g_pctlWideFloatCmp != PCTL_WIDE_CMP ) )
   {
      printf( "PERCENTILE Fail: coverage counters (oracle %d, published %d, "
              "extrema %d, differential %d, rank %d, edges %d, alias %d, "
              "flag %d, wide %d, wide stream %d, wide float %d) are not what "
              "this file was written with (%d, 8, 140656, 403221, 9110, 48406, "
              "129328, 1, %d, %d, %d)\n",
              g_pctlOracleCmp, g_pctlBookCmp, g_pctlExtremaCmp, g_pctlDiffCmp,
              g_pctlRankCmp, g_pctlEdgeCmp, g_pctlAliasCmp, g_pctlFlagCmp,
              g_pctlWideCmp, g_pctlWideStreamCmp, g_pctlWideFloatCmp,
              NB_PCTL_ORACLE, PCTL_WIDE_CMP, PCTL_WIDE_STREAM_CMP, PCTL_WIDE_CMP );
      return TA_PERCENTILE_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (9) TA_OPTIN_IS_PERCENT on optInPercentile.
 *
 * PERCENTILE is the flag's first function-level user in the corpus, and the
 * only other check on it (test_abstract.c) compares C against each server: a
 * flag dropped in the metadata generator is dropped on both sides and 0 == 0
 * passes. An unrecognised YAML flag string is silently ignored, so this is the
 * assertion that the key spelled in percentile.yaml is one the generator knows.
 */
static ErrorNumber test_percentile_percent_flag( void )
{
   const TA_FuncHandle *handle;
   const TA_OptInputParameterInfo *info;
   TA_RetCode retCode;

   retCode = TA_GetFuncHandle( "PERCENTILE", &handle );
   if( retCode != TA_SUCCESS )
   {
      printf( "PERCENTILE metadata Fail: TA_GetFuncHandle rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   retCode = TA_GetOptInputParameterInfo( handle, 1, &info );
   if( retCode != TA_SUCCESS || info == NULL )
   {
      printf( "PERCENTILE metadata Fail: TA_GetOptInputParameterInfo rc=%d\n",
              (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   if( strcmp( info->paramName, "optInPercentile" ) != 0 )
   {
      printf( "PERCENTILE metadata Fail: opt-input 1 is \"%s\", expected "
              "\"optInPercentile\"\n", info->paramName );
      return TA_PERCENTILE_VACUOUS;
   }

   g_pctlFlagCmp++;
   if( ( info->flags & TA_OPTIN_IS_PERCENT ) == 0 )
   {
      printf( "PERCENTILE metadata Fail: optInPercentile flags=0x%08x, "
              "TA_OPTIN_IS_PERCENT (0x%08x) is not set -- an unknown flag key "
              "in percentile.yaml is dropped without a word\n",
              (unsigned)info->flags, (unsigned)TA_OPTIN_IS_PERCENT );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (1) The frozen two-oracle goldens, plus the cross-language replay. */
static ErrorNumber test_percentile_oracle( const TA_History *history )
{
   static TA_Real out[PCTL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastPeriod = -1;
   double lastPct = -1.0;

   if( nbBars != 252 )
   {
      printf( "PERCENTILE oracle skip: goldens were captured on the 252-bar "
              "corpus, got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_PCTL_ORACLE; k++ )
   {
      int period = percentileOracle[k].period;
      double pct = percentileOracle[k].pct;

      if( period != lastPeriod || pct != lastPct )
      {
         lastPeriod = period;
         lastPct    = pct;
         retCode = TA_PERCENTILE( 0, nbBars-1, history->close, period, pct,
                                  &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != period-1
             || nbElement != nbBars - period + 1 )
         {
            printf( "PERCENTILE oracle Fail [N=%d P=%g]: rc=%d (%d,%d) "
                    "expected (%d,%d)\n", period, pct, (int)retCode, begIdx,
                    nbElement, period-1, nbBars - period + 1 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[2];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)period;
            optIn[1] = pct;
            e = server_verify( "PERCENTILE", 0, nbBars-1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->close, NULL },
                               optIn, 2,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* "No failure reported" and "nothing was compared" are the same
             * observation without this. */
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "PERCENTILE oracle [N=%d P=%g]: compared no server "
                       "despite live pipes\n", period, pct );
               return TA_PERCENTILE_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( percentileOracle[k].bar < begIdx
          || percentileOracle[k].bar - begIdx >= nbElement )
      {
         printf( "PERCENTILE oracle Fail [N=%d P=%g]: golden bar %d is outside "
                 "the output [%d..%d]\n", period, pct, percentileOracle[k].bar,
                 begIdx, begIdx + nbElement - 1 );
         return TA_PERCENTILE_VACUOUS;
      }

      g_pctlOracleCmp++;
      /* Tolerance ZERO: nearest rank returns an input element verbatim. */
      if( out[percentileOracle[k].bar - begIdx] != percentileOracle[k].want )
      {
         printf( "PERCENTILE oracle Fail [N=%d P=%g] at bar %d: got %.17g "
                 "expected %.17g (tol 0 -- the output is a copy of an input)\n",
                 period, pct, percentileOracle[k].bar,
                 out[percentileOracle[k].bar - begIdx],
                 percentileOracle[k].want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (2) The published worked example, exactly. */
static ErrorNumber test_percentile_published( void )
{
   static TA_Real out[PCTL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i;

   for( i = 0; i < 8; i++ )
   {
      retCode = TA_PERCENTILE( 0, 4, pctlBookIn, 5, pctlBookP[i],
                               &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 4 || nbElement != 1 )
      {
         printf( "PERCENTILE published Fail [P=%g]: rc=%d (%d,%d) expected "
                 "(4,1)\n", pctlBookP[i], (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      if( server_verify_active() )
      {
         double optIn[2];
         ErrorNumber e;
         int cmpBefore = server_verify_comparisons();

         optIn[0] = 5.0;
         optIn[1] = pctlBookP[i];
         e = server_verify( "PERCENTILE", 0, 4, 5,
                            retCode, begIdx, nbElement,
                            (const TA_Real*[]){ pctlBookIn, NULL },
                            optIn, 2,
                            (const TA_Real*[]){ out, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
         if( server_verify_comparisons() == cmpBefore )
         {
            printf( "PERCENTILE published [P=%g]: compared no server despite "
                    "live pipes\n", pctlBookP[i] );
            return TA_PERCENTILE_VACUOUS;
         }
      }

      g_pctlBookCmp++;
      if( out[0] != pctlBookOut[i] )
      {
         printf( "PERCENTILE published Fail [P=%g]: got %.17g expected %.17g\n",
                 pctlBookP[i], out[0], pctlBookOut[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) The clamp arms against TA_MIN and TA_MAX, bitwise.
 *
 * Both are already held bit-exact across every backend by --xlang-hash, and
 * both take their lookback from the same optInTimePeriod-1, so the whole shape
 * of the call -- anchor, count and every value -- has to line up. Unlike leg 4
 * this reference shares no code with PERCENTILE, so it does catch a wrong rank
 * formula, at the two percentages where the answer is knowable independently.
 */
static ErrorNumber test_percentile_extrema( const char *tag, const TA_Real *in,
                                            int nbBars, int maxPeriod )
{
   static TA_Real out[PCTL_CAP], ref[PCTL_CAP];
   TA_Integer begIdx, nbElement, begRef, nbRef;
   TA_RetCode retCode;
   int period, i, hi;

   for( period = 2; period <= maxPeriod; period++ )
   {
      for( hi = 0; hi <= 1; hi++ )
      {
         retCode = hi ? TA_MAX( 0, nbBars-1, in, period, &begRef, &nbRef, ref )
                      : TA_MIN( 0, nbBars-1, in, period, &begRef, &nbRef, ref );
         if( retCode != TA_SUCCESS )
         {
            printf( "PERCENTILE extrema [%s N=%d]: reference rc=%d\n",
                    tag, period, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         retCode = TA_PERCENTILE( 0, nbBars-1, in, period, hi ? 100.0 : 0.0,
                                  &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != begRef || nbElement != nbRef )
         {
            printf( "PERCENTILE extrema Fail [%s N=%d P=%d]: rc=%d (%d,%d) vs "
                    "TA_%s (%d,%d)\n", tag, period, hi ? 100 : 0, (int)retCode,
                    begIdx, nbElement, hi ? "MAX" : "MIN", begRef, nbRef );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbElement; i++ )
         {
            g_pctlExtremaCmp++;
            if( out[i] != ref[i] )
            {
               printf( "PERCENTILE extrema Fail [%s N=%d P=%d] out %d: %.17g "
                       "vs TA_%s %.17g -- the rank clamp must land on the "
                       "first/last order statistic\n",
                       tag, period, hi ? 100 : 0, i, out[i],
                       hi ? "MAX" : "MIN", ref[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) The incremental two-buffer window against a naive per-bar re-sort.
 *
 * Bitwise: both sides pick an element out of the window, so anything but
 * equality is a wrong pick. The window is sorted ONCE per bar and reused across
 * every percentage, which is what keeps the long-period corpus affordable.
 */
static ErrorNumber test_percentile_differential( const char *tag, const TA_Real *in,
                                                 int nbBars, const int *periods,
                                                 int nbPeriods )
{
   static TA_Real out[NB_PCTL_P][PCTL_CAP];
   static TA_Real win[PCTL_CAP];
   TA_Integer begIdx[NB_PCTL_P], nbElement[NB_PCTL_P];
   TA_RetCode retCode;
   int rank[NB_PCTL_P];
   int p, q, t, period;

   for( q = 0; q < nbPeriods; q++ )
   {
      period = periods[q];

      for( p = 0; p < NB_PCTL_P; p++ )
      {
         retCode = TA_PERCENTILE( 0, nbBars-1, in, period, pctlGridP[p],
                                  &begIdx[p], &nbElement[p], out[p] );
         if( retCode != TA_SUCCESS || begIdx[p] != period-1
             || nbElement[p] != nbBars - period + 1 )
         {
            printf( "PERCENTILE differential Fail [%s N=%d P=%g]: rc=%d (%d,%d) "
                    "expected (%d,%d)\n", tag, period, pctlGridP[p], (int)retCode,
                    begIdx[p], nbElement[p], period-1, nbBars - period + 1 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         rank[p] = pctlRankFp( period, pctlGridP[p] );
      }

      for( t = period-1; t < nbBars; t++ )
      {
         pctlSortWindow( in, t, period, win );

         for( p = 0; p < NB_PCTL_P; p++ )
         {
            double got = out[p][t-(period-1)];

            g_pctlDiffCmp++;
            if( got != win[rank[p]-1] )
            {
               printf( "PERCENTILE differential Fail [%s N=%d P=%g] at bar %d: "
                       "incremental %.17g, fresh re-sort %.17g (rank %d)\n",
                       tag, period, pctlGridP[p], t, got, win[rank[p]-1],
                       rank[p] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) The exact-rank class, against integer arithmetic.
 *
 * At every pair below P*n/100 is an exact integer, and that is precisely where
 * ceil(P/100.0*n) overshoots by one: P/100 is inexact in binary64 and the
 * product lands just above the integer. The leg asserts the two spellings
 * really do disagree at each pair before comparing, so it cannot pass by
 * choosing a pair where the question does not arise, and it counts the bars
 * where the two ranks select DIFFERENT values -- without one, a window whose
 * two neighbouring order statistics are equal would satisfy either spelling.
 */
static ErrorNumber test_percentile_exact_rank( const TA_Real *in, int nbBars )
{
   static TA_Real out[PCTL_CAP], win[PCTL_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int k, t, discriminating;

   for( k = 0; k < NB_PCTL_EXACT; k++ )
   {
      int period = pctlExactRank[k].period;
      int pct    = pctlExactRank[k].pct;
      int want   = pctlRankInt( period, pct );

      if( pctlRankWrong( period, (double)pct ) == want )
      {
         printf( "PERCENTILE rank Fail [N=%d P=%d]: the two spellings agree "
                 "here, so this pair proves nothing -- pick one where "
                 "P*n/100 is an exact integer\n", period, pct );
         return TA_PERCENTILE_VACUOUS;
      }

      retCode = TA_PERCENTILE( 0, nbBars-1, in, period, (double)pct,
                               &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period-1
          || nbElement != nbBars - period + 1 )
      {
         printf( "PERCENTILE rank Fail [N=%d P=%d]: rc=%d (%d,%d)\n",
                 period, pct, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      discriminating = 0;
      for( t = period-1; t < nbBars; t++ )
      {
         pctlSortWindow( in, t, period, win );

         if( win[want-1] != win[pctlRankWrong( period, (double)pct )-1] )
            discriminating++;

         g_pctlRankCmp++;
         if( out[t-(period-1)] != win[want-1] )
         {
            printf( "PERCENTILE rank Fail [N=%d P=%d] at bar %d: got %.17g, "
                    "exact integer rank %d gives %.17g -- keep the multiply "
                    "left of the divide\n", period, pct, t,
                    out[t-(period-1)], want, win[want-1] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      if( discriminating == 0 )
      {
         printf( "PERCENTILE rank Fail [N=%d P=%d]: no bar in the corpus tells "
                 "the two rank spellings apart, so this pair compared "
                 "nothing\n", period, pct );
         return TA_PERCENTILE_VACUOUS;
      }
   }

   return TA_TEST_PASS;
}

/* (6) Edges.
 *
 * The +-0.0 window is the one that needs a BITWISE comparison: -0.0 == +0.0, so
 * a value compare would accept either sign. Both the reference and the shipped
 * body order equal values by age, so which zero comes out is determined by the
 * input, not by the sort -- which is also why the reference may not be qsort.
 */
static ErrorNumber test_percentile_edges( void )
{
   static TA_Real in[128], out[PCTL_CAP], win[PCTL_CAP];
   static const TA_Real tiny[5] = { 5.0, 1.0, 4.0, 2.0, 3.0 };
   static const TA_Real tinyMin2[4] = { 1.0, 1.0, 2.0, 2.0 };
   static const TA_Real tinyMax2[4] = { 5.0, 4.0, 4.0, 3.0 };
   static const TA_Real tinyMid3[3] = { 4.0, 2.0, 3.0 };
   static const TA_Real tinyMax3[3] = { 5.0, 4.0, 4.0 };
   static const TA_Real tinyMin3[3] = { 1.0, 1.0, 2.0 };
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int period, i, p;

   /* A constant window: every order statistic is the constant, so every
    * percentage must return it exactly. */
   for( i = 0; i < 100; i++ )
      in[i] = 42.5;
   for( p = 0; p < NB_PCTL_P; p++ )
   {
      for( period = 2; period <= 20; period++ )
      {
         retCode = TA_PERCENTILE( 0, 99, in, period, pctlGridP[p],
                                  &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != period-1
             || nbElement != 101 - period )
         {
            printf( "PERCENTILE flat Fail [N=%d P=%g]: rc=%d (%d,%d)\n",
                    period, pctlGridP[p], (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_pctlEdgeCmp++;
            if( out[i] != 42.5 )
            {
               printf( "PERCENTILE flat Fail [N=%d P=%g] out %d: %.17g, "
                       "expected exactly 42.5\n",
                       period, pctlGridP[p], i, out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   /* A window mixing +0.0 and -0.0, compared BITWISE. */
   for( i = 0; i < 60; i++ )
      in[i] = ( ( i % 3 ) == 0 ) ? -0.0 : 0.0;
   for( period = 2; period <= 12; period++ )
   {
      for( p = 0; p < NB_PCTL_P; p++ )
      {
         int rank = pctlRankFp( period, pctlGridP[p] );

         retCode = TA_PERCENTILE( 0, 59, in, period, pctlGridP[p],
                                  &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != period-1
             || nbElement != 61 - period )
         {
            printf( "PERCENTILE signed-zero Fail [N=%d P=%g]: rc=%d (%d,%d)\n",
                    period, pctlGridP[p], (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nbElement; i++ )
         {
            double want;

            pctlSortWindow( in, i + period-1, period, win );
            want = win[rank-1];
            g_pctlEdgeCmp++;
            if( memcmp( &out[i], &want, sizeof(double) ) != 0 )
            {
               printf( "PERCENTILE signed-zero Fail [N=%d P=%g] out %d: "
                       "%.17g (sign %d), expected %.17g (sign %d) -- equal "
                       "values must leave the window in age order\n",
                       period, pctlGridP[p], i, out[i], (int)signbit(out[i]),
                       want, (int)signbit(want) );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   /* The two smallest periods, hand-checkable. */
   retCode = TA_PERCENTILE( 0, 4, tiny, 2, 50.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != 4 )
   {
      printf( "PERCENTILE tiny Fail [N=2 P=50]: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < 4; i++ )
   {
      g_pctlEdgeCmp++;
      if( out[i] != tinyMin2[i] )
      {
         printf( "PERCENTILE tiny Fail [N=2 P=50] out %d: %.17g expected "
                 "%.17g\n", i, out[i], tinyMin2[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   retCode = TA_PERCENTILE( 0, 4, tiny, 2, 100.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != 4 )
   {
      printf( "PERCENTILE tiny Fail [N=2 P=100]: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < 4; i++ )
   {
      g_pctlEdgeCmp++;
      if( out[i] != tinyMax2[i] )
      {
         printf( "PERCENTILE tiny Fail [N=2 P=100] out %d: %.17g expected "
                 "%.17g\n", i, out[i], tinyMax2[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   retCode = TA_PERCENTILE( 0, 4, tiny, 3, 50.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 2 || nbElement != 3 )
   {
      printf( "PERCENTILE tiny Fail [N=3 P=50]: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < 3; i++ )
   {
      g_pctlEdgeCmp++;
      if( out[i] != tinyMid3[i] )
      {
         printf( "PERCENTILE tiny Fail [N=3 P=50] out %d: %.17g expected "
                 "%.17g\n", i, out[i], tinyMid3[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   retCode = TA_PERCENTILE( 0, 4, tiny, 3, 100.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 2 || nbElement != 3 )
   {
      printf( "PERCENTILE tiny Fail [N=3 P=100]: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < 3; i++ )
   {
      g_pctlEdgeCmp++;
      if( out[i] != tinyMax3[i] )
      {
         printf( "PERCENTILE tiny Fail [N=3 P=100] out %d: %.17g expected "
                 "%.17g\n", i, out[i], tinyMax3[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* P=1 at period 3 rounds the rank UP to 1, not down to 0. */
   retCode = TA_PERCENTILE( 0, 4, tiny, 3, 1.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 2 || nbElement != 3 )
   {
      printf( "PERCENTILE tiny Fail [N=3 P=1]: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < 3; i++ )
   {
      g_pctlEdgeCmp++;
      if( out[i] != tinyMin3[i] )
      {
         printf( "PERCENTILE tiny Fail [N=3 P=1] out %d: %.17g expected "
                 "%.17g\n", i, out[i], tinyMin3[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* A single-bar request exactly at the lookback. */
   retCode = TA_PERCENTILE( 2, 2, tiny, 3, 50.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 2 || nbElement != 1 )
   {
      printf( "PERCENTILE anchor Fail: rc=%d (%d,%d) expected (2,1)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   g_pctlEdgeCmp++;
   if( out[0] != tinyMid3[0] )
   {
      printf( "PERCENTILE anchor Fail: %.17g expected %.17g\n",
              out[0], tinyMid3[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* A range shorter than the lookback produces nothing, successfully. */
   begIdx = -1; nbElement = -1;
   retCode = TA_PERCENTILE( 0, 1, tiny, 5, 50.0, &begIdx, &nbElement, out );
   g_pctlEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "PERCENTILE short Fail: rc=%d (%d,%d) expected TA_SUCCESS "
              "(0,0)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   g_pctlEdgeCmp++;
   if( TA_PERCENTILE_Lookback( 10000, 50.0 ) != 9999
       || TA_PERCENTILE_Lookback( 10001, 50.0 ) != -1 )
   {
      printf( "PERCENTILE edges Fail: lookback %d at 10000 and %d at 10001, "
              "expected 9999 and -1\n", TA_PERCENTILE_Lookback( 10000, 50.0 ),
              TA_PERCENTILE_Lookback( 10001, 50.0 ) );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   g_pctlEdgeCmp++;
   retCode = TA_PERCENTILE( 0, 99, in, 10001, 50.0, &begIdx, &nbElement, out );
   if( retCode != TA_BAD_PARAM )
   {
      printf( "PERCENTILE edges Fail: period 10001 returned rc=%d, expected "
              "TA_BAD_PARAM\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   g_pctlEdgeCmp++;
   retCode = TA_PERCENTILE( 0, 99, in, 10000, 50.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "PERCENTILE edges Fail: period 10000 returned rc=%d, expected "
              "TA_SUCCESS\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   return TA_TEST_PASS;
}

/* (7) In-place aliasing, outReal == inReal, bitwise.
 *
 * The window is held as copies in the two scratch buffers and inReal is never
 * re-read below the bar being written, which is the whole invariant. A store
 * landing under a later read would be invisible to ASan.
 */
static ErrorNumber test_percentile_aliasing( const char *tag, const TA_Real *in,
                                             int nbBars, int maxPeriod )
{
   static TA_Real clean[PCTL_CAP], alias[PCTL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int period, i;
   double pct;

   for( period = 2; period <= maxPeriod; period++ )
   {
      pct = pctlGridP[period % NB_PCTL_P];

      retCode = TA_PERCENTILE( 0, nbBars-1, in, period, pct,
                               &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "PERCENTILE alias Fail [%s N=%d]: rc=%d\n",
                 tag, period, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = in[i];
      retCode = TA_PERCENTILE( 0, nbBars-1, alias, period, pct,
                               &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "PERCENTILE alias Fail [%s N=%d]: rc=%d shape (%d,%d) vs "
                 "(%d,%d)\n", tag, period, (int)retCode, begIdx2, nbElement2,
                 begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_pctlAliasCmp++;
         if( clean[i] != alias[i] )
         {
            printf( "PERCENTILE alias Fail [%s N=%d P=%g] out %d: separate "
                    "%.17g, in-place %.17g -- a store landed under a read the "
                    "window still needed\n",
                    tag, period, pct, i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (8) The startIdx/endIdx range sweep. TA_STABLE_EXACT: each bar's window is
 * rebuilt from its own bars with no value carried across, so no range may move
 * a value by even one ulp. No unstable-period id, matching its abstract
 * metadata, which doRangeTestEx cross-checks against the stability class. */
typedef struct { int period; double pct; const TA_Real *in; } PctlRangeParam;

static TA_RetCode pctlRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   PctlRangeParam *p = (PctlRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_PERCENTILE_Lookback( p->period, p->pct );
   return TA_PERCENTILE( startIdx, endIdx, p->in, p->period, p->pct,
                         outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_percentile_range( const TA_Real *in )
{
   PctlRangeParam param;

   param.period = 28;
   param.pct    = 45.0;
   param.in     = in;

   return doRangeTestEx( pctlRangeTestFunction,
                         TA_STABLE_EXACT, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}

/* (10) Across the size thresholds. */
static const int pctlWideN[] = { 2, 32, 33, 64, 65, 100, 101, 114, 256, 257,
                                 300, 301, 2048, 2049 };
static const double pctlWideP[] = { 0.0, 7.0, 50.0, 90.0, 100.0 };
#define NB_PCTL_WIDE_N ((int)(sizeof(pctlWideN)/sizeof(int)))
#define NB_PCTL_WIDE_P ((int)(sizeof(pctlWideP)/sizeof(double)))
#define NB_PCTL_WIDE_CORPORA 8

static void pctlBuildWide( int which, int n, double *x )
{
   static const double pm[4] = { 0.0, -0.0, 1.0, -1.0 };
   unsigned int seed = 377u;
   double v = 100.0, slope = 0.0;
   int i, k, leg, left = 0;

   for( i = 0; i < PCTL_WIDE_NB; i++ )
   {
      seed = (seed * 1103515245u + 12345u) & 0x7fffffffu;
      if( which == 0 )
      {
         v += 0.25 * (double)((int)((seed >> 8) % 5u) - 2);
         x[i] = v;
      }
      else if( which == 1 )
         x[i] = pm[(seed >> 8) & 3u];
      else if( which < 4 )
      {
         /* Cycles of 150 bars: 70 strictly up, 40 of noise, 25 strictly down,
          * 15 flat. Corpus 3 is corpus 2 negated, a falling trend. */
         leg = i % 150;
         if( leg < 70 )
            v += 0.25;
         else if( leg < 110 )
            v += 0.25 * (double)((int)((seed >> 8) % 3u) - 1);
         else if( leg < 135 )
            v -= 0.25;
         x[i] = ( which == 2 ) ? v : -v;
      }
      else if( which < 6 )
      {
         /* A strict fall that bottoms out, 30 bars after the first window, in
          * six zeros of alternating sign, then keeps falling: a new value tied
          * with the window's minimum mid-trend. Corpus 5 is its mirror. */
         k = i - (n+30);
         if( k < 0 )
            x[i] = 0.25 * (double)(-k);
         else if( k < 6 )
            x[i] = ( k & 1 ) ? -0.0 : 0.0;
         else
            x[i] = -0.25 * (double)(k-5);
         if( which == 5 )
            x[i] = -x[i];
      }
      else if( which == 6 )
      {
         /* A first window of zeros whose sign flips every two bars, with a -1
          * at bar 19 and at its end, then corpus 1: a merge-sorted first window
          * meets equal and opposite-signed zeros at its run boundaries. */
         if( i < n-1 )
            x[i] = ( i == 19 || i == n-2 ) ? -1.0 : ( ( i & 2 ) ? -0.0 : 0.0 );
         else
            x[i] = pm[(seed >> 8) & 3u];
      }
      else
      {
         /* Trends of random length and slope under tick noise. These constants
          * are what wraps the ring's search and head at 300 retained values:
          * re-measure coverage before changing one. */
         if( left == 0 )
         {
            left = 2 + (int)((seed >> 8) % (unsigned)n);
            slope = 0.25 * (double)((int)((seed >> 6) % 3u) - 1);
         }
         left--;
         v += slope + 0.25 * (double)((int)((seed >> 12) % 3u) - 1);
         x[i] = v;
      }
   }
}

/* Ties broken by index make the order total, so qsort's instability cannot
 * matter, and a tie by index is a tie by age: the order the body keeps.
 */
static const TA_Real *g_pctlKeyIn;
static int pctlKeyCmp( const void *a, const void *b )
{
   int i = *(const int *)a, j = *(const int *)b;

   if( g_pctlKeyIn[i] < g_pctlKeyIn[j] )
      return -1;
   if( g_pctlKeyIn[i] > g_pctlKeyIn[j] )
      return 1;
   return (i > j) - (i < j);
}

static void pctlSortWindowKeyed( const TA_Real *in, int t, int period, double *dest )
{
   static int idx[PCTL_WIDE_NB];
   int i;

   for( i = 0; i < period; i++ )
      idx[i] = t-period+1+i;
   g_pctlKeyIn = in;
   qsort( idx, (size_t)period, sizeof(int), pctlKeyCmp );
   for( i = 0; i < period; i++ )
      dest[i] = in[idx[i]];
}

/* One leg-10 call replayed on the servers, through their single-precision tier
 * when isFloat is set. */
static ErrorNumber pctlWideRoute( const TA_Real *x, int startIdx, int endIdx,
                                  int nbBars, int n, double pct, int isFloat,
                                  TA_RetCode retCode, TA_Integer begIdx,
                                  TA_Integer nbElement, const TA_Real *out )
{
   double optIn[2];
   ErrorNumber e;
   int cmpBefore;

   if( !( isFloat ? server_verify_float_active() : server_verify_active() ) )
      return TA_TEST_PASS;

   optIn[0] = (double)n;
   optIn[1] = pct;
   cmpBefore = server_verify_comparisons();
   server_verify_set_float( isFloat );
   e = server_verify( "PERCENTILE", startIdx, endIdx, nbBars,
                      retCode, begIdx, nbElement,
                      (const TA_Real*[]){ x, NULL }, optIn, 2,
                      (const TA_Real*[]){ out, NULL }, NULL );
   server_verify_set_float( 0 );
   if( e != TA_TEST_PASS )
      return e;
   if( server_verify_comparisons() == cmpBefore )
   {
      printf( "PERCENTILE wide%s [N=%d P=%g range %d..%d]: compared no server "
              "despite live pipes\n", isFloat ? " float" : "", n, pct,
              startIdx, endIdx );
      return TA_PERCENTILE_VACUOUS;
   }
   return TA_TEST_PASS;
}

static ErrorNumber test_percentile_wide( void )
{
   static TA_Real x[PCTL_WIDE_NB], out[NB_PCTL_WIDE_P][PCTL_WIDE_NB];
   static TA_Real win[PCTL_WIDE_NB], outS[PCTL_WIDE_NB];
   static float xf[PCTL_WIDE_NB];
   TA_PERCENTILE_Stream *s;
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   ErrorNumber e;
   int which, q, p, t, k, n, nbBars, rank, isFloat, startIdx, endIdx;
   int *cmp;
   double peeked, updated, first;

   for( which = 0; which < NB_PCTL_WIDE_CORPORA; which++ )
   {
      for( q = 0; q < NB_PCTL_WIDE_N; q++ )
      {
         n = pctlWideN[q];
         nbBars = n + 400;
         pctlBuildWide( which, n, x );
         for( t = 0; t < nbBars; t++ )
            xf[t] = (float)x[t];

         for( p = 0; p < NB_PCTL_WIDE_P; p++ )
         {
            retCode = TA_PERCENTILE( 0, nbBars-1, x, n, pctlWideP[p],
                                     &begIdx, &nbElement, out[p] );
            if( retCode != TA_SUCCESS || begIdx != n-1
                || nbElement != nbBars - n + 1 )
            {
               printf( "PERCENTILE wide Fail [corpus %d N=%d P=%g]: rc=%d "
                       "(%d,%d)\n", which, n, pctlWideP[p], (int)retCode,
                       begIdx, nbElement );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            e = pctlWideRoute( x, 0, nbBars-1, nbBars, n, pctlWideP[p], 0,
                               retCode, begIdx, nbElement, out[p] );
            if( e != TA_TEST_PASS )
               return e;
         }

         for( t = n-1; t < nbBars; t++ )
         {
            pctlSortWindowKeyed( x, t, n, win );
            for( p = 0; p < NB_PCTL_WIDE_P; p++ )
            {
               rank = pctlRankFp( n, pctlWideP[p] );
               g_pctlWideCmp++;
               if( memcmp( &out[p][t-(n-1)], &win[rank-1], sizeof(double) ) != 0 )
               {
                  printf( "PERCENTILE wide Fail [corpus %d N=%d P=%g] bar %d: "
                          "%.17g, fresh re-sort %.17g\n", which, n,
                          pctlWideP[p], t, out[p][t-(n-1)], win[rank-1] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }

         /* Every corpus is float-exact, so TA_S_PERCENTILE must return the
          * same doubles. */
         for( p = 0; p < NB_PCTL_WIDE_P; p++ )
         {
            retCode = TA_S_PERCENTILE( 0, nbBars-1, xf, n, pctlWideP[p],
                                       &begIdx, &nbElement, outS );
            if( retCode != TA_SUCCESS || begIdx != n-1
                || nbElement != nbBars - n + 1 )
            {
               printf( "PERCENTILE wide float Fail [corpus %d N=%d P=%g]: rc=%d "
                       "(%d,%d)\n", which, n, pctlWideP[p], (int)retCode,
                       begIdx, nbElement );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            e = pctlWideRoute( x, 0, nbBars-1, nbBars, n, pctlWideP[p], 1,
                               retCode, begIdx, nbElement, outS );
            if( e != TA_TEST_PASS )
               return e;
            for( t = 0; t < nbElement; t++ )
            {
               g_pctlWideFloatCmp++;
               if( memcmp( &outS[t], &out[p][t], sizeof(double) ) != 0 )
               {
                  printf( "PERCENTILE wide float Fail [corpus %d N=%d P=%g] bar "
                          "%d: TA_S_ %.17g, TA_ %.17g\n", which, n,
                          pctlWideP[p], t + n-1, outS[t], out[p][t] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }

         /* A later anchor primes from a window of its own and must land on the
          * same values; a range one bar short of the lookback outputs none. */
         for( k = 0; k < 4; k++ )
         {
            isFloat = k & 1;
            cmp = isFloat ? &g_pctlWideFloatCmp : &g_pctlWideCmp;
            startIdx = ( k < 2 ) ? n+16 : 0;
            endIdx = ( k < 2 ) ? nbBars-1 : n-2;
            retCode = isFloat
               ? TA_S_PERCENTILE( startIdx, endIdx, xf, n, pctlWideP[2], &begIdx,
                                  &nbElement, outS )
               : TA_PERCENTILE( startIdx, endIdx, x, n, pctlWideP[2], &begIdx,
                                &nbElement, outS );
            (*cmp)++;
            if( retCode != TA_SUCCESS || begIdx != ( k < 2 ? n+16 : 0 )
                || nbElement != ( k < 2 ? nbBars-n-16 : 0 ) )
            {
               printf( "PERCENTILE wide range Fail [corpus %d N=%d %d..%d%s]: "
                       "rc=%d (%d,%d)\n", which, n, startIdx, endIdx,
                       isFloat ? " float" : "", (int)retCode, begIdx, nbElement );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            for( t = 0; t < nbElement; t++ )
            {
               (*cmp)++;
               if( memcmp( &outS[t], &out[2][t+17], sizeof(double) ) != 0 )
               {
                  printf( "PERCENTILE wide range Fail [corpus %d N=%d%s] bar %d: "
                          "%.17g from anchor %d, %.17g from 0\n", which, n,
                          isFloat ? " float" : "", t + n+16, outS[t], n+16,
                          out[2][t+17] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
            e = pctlWideRoute( x, startIdx, endIdx, nbBars, n, pctlWideP[2],
                               isFloat, retCode, begIdx, nbElement, outS );
            if( e != TA_TEST_PASS )
               return e;
         }

         /* The stream tier: OpenAndFill over every bar, then Open over a
          * warm-up with Peek and Update every later bar, each bitwise against
          * the batch value. Open and OpenAndFill run their own copy of the
          * first window and the loop. */
         for( p = 0; p < NB_PCTL_WIDE_P; p += 2 )
         {
            retCode = TA_PERCENTILE_OpenAndFill( &s, x, nbBars, n, pctlWideP[p],
                                                 &begIdx, &nbElement, outS );
            if( retCode != TA_SUCCESS || begIdx != n-1
                || nbElement != nbBars - n + 1 )
            {
               printf( "PERCENTILE wide fill Fail [corpus %d N=%d P=%g]: rc=%d "
                       "(%d,%d)\n", which, n, pctlWideP[p], (int)retCode, begIdx,
                       nbElement );
               if( retCode == TA_SUCCESS )
                  TA_PERCENTILE_Close( s );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
            TA_PERCENTILE_Close( s );
            for( t = 0; t < nbElement; t++ )
            {
               g_pctlWideStreamCmp++;
               if( memcmp( &outS[t], &out[p][t], sizeof(double) ) != 0 )
               {
                  printf( "PERCENTILE wide fill Fail [corpus %d N=%d P=%g] bar %d: "
                          "%.17g, batch %.17g\n", which, n, pctlWideP[p],
                          t + n-1, outS[t], out[p][t] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }

            retCode = TA_PERCENTILE_Open( &s, x, n + 17, n, pctlWideP[p], &first );
            if( retCode != TA_SUCCESS )
            {
               printf( "PERCENTILE wide stream Fail [corpus %d N=%d P=%g]: Open "
                       "rc=%d\n", which, n, pctlWideP[p], (int)retCode );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
            g_pctlWideStreamCmp++;
            if( memcmp( &first, &out[p][17], sizeof(double) ) != 0 )
            {
               printf( "PERCENTILE wide stream Fail [corpus %d N=%d P=%g]: Open "
                       "%.17g, batch %.17g\n", which, n, pctlWideP[p], first,
                       out[p][17] );
               TA_PERCENTILE_Close( s );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            for( t = n + 17; t < nbBars; t++ )
            {
               if( TA_PERCENTILE_Peek( s, x[t], &peeked ) != TA_SUCCESS
                   || TA_PERCENTILE_Update( s, x[t], &updated ) != TA_SUCCESS )
               {
                  printf( "PERCENTILE wide stream Fail [corpus %d N=%d P=%g] "
                          "bar %d: Peek or Update rejected\n", which, n,
                          pctlWideP[p], t );
                  TA_PERCENTILE_Close( s );
                  return TA_TESTUTIL_TFRR_BAD_RETCODE;
               }
               g_pctlWideStreamCmp++;
               if( memcmp( &peeked, &out[p][t-(n-1)], sizeof(double) ) != 0
                   || memcmp( &updated, &out[p][t-(n-1)], sizeof(double) ) != 0 )
               {
                  printf( "PERCENTILE wide stream Fail [corpus %d N=%d P=%g] "
                          "bar %d: peek %.17g, update %.17g, batch %.17g\n",
                          which, n, pctlWideP[p], t, peeked, updated,
                          out[p][t-(n-1)] );
                  TA_PERCENTILE_Close( s );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
            TA_PERCENTILE_Close( s );

            /* Opened 17 bars in, the first window is a later one of its own. */
            retCode = TA_PERCENTILE_Open( &s, x+17, n, n, pctlWideP[p], &first );
            if( retCode != TA_SUCCESS )
            {
               printf( "PERCENTILE wide stream Fail [corpus %d N=%d P=%g]: late "
                       "Open rc=%d\n", which, n, pctlWideP[p], (int)retCode );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
            TA_PERCENTILE_Close( s );
            g_pctlWideStreamCmp++;
            if( memcmp( &first, &out[p][17], sizeof(double) ) != 0 )
            {
               printf( "PERCENTILE wide stream Fail [corpus %d N=%d P=%g]: late "
                       "Open %.17g, batch %.17g\n", which, n, pctlWideP[p], first,
                       out[p][17] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}
