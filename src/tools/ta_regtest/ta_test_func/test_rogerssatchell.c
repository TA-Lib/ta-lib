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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100526 KL,CC  First version (proposal ROGERSSATCHELL, #483).
 *  100526 MF,CC  Whole-bar guard on every price and tier, drift-only and
 *                negative-sum windows, rebuild bits, server_verify (#483).
 */

/* Description:
 *
 *   Test TA_ROGERSSATCHELL (Rogers-Satchell volatility, #483).
 *
 *   --codegen, --xlang-hash and server_verify compare every language against
 *   this library, so none of them can catch a wrong formula. The formula is
 *   held by goldens evaluated at 60 digits from eq. (2) of Rogers and
 *   Satchell, The Annals of Applied Probability 1(4):504-512 (1991), and by
 *   analytic properties a wrong formula cannot hold at once.
 *
 *   The golden tolerance is not a whole-series bound: at n <= 2 the error
 *   floor follows C/(C-L), so a whole-series check there needs 1e-12.
 *
 *   Every comparison count is pinned, so a leg that stops comparing fails.
 *
 *   SERVER_VERIFY: the guard leg's bad-price calls and the rebuild leg's
 *   negative-sum call. No sweep sends a price at or below zero or a window
 *   that sums below zero.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define RS_NB_BAR 252

/* Pinned comparison counts. A leg that stops reaching its comparator still
 * returns TA_TEST_PASS; these are what makes that a failure. */
#define RS_GOLDEN_CMP    14
#define RS_SCALE_CMP   1456
#define RS_DRIFT_CMP    255
#define RS_LOOKBACK_CMP  22
#define RS_ALIAS_CMP      4
#define RS_COMPOSITE_CMP 960
/* 3 flat prices x 5 periods: the all-flat exact-zero checks are
 * sum(51-n) = 46+42+37+31+1 = 157 each, and the fresh-sum checks are
 * sum(300-n+1) = 296+292+287+281+251 = 1407 each, so 3*(157+1407) =
 * 4692 per cut point; five cut points make 23460, and shock-then-calm at
 * n = 9 and 20 adds 292+281 = 573. */
#define RS_REBUILD_CMP 24033
/* Periods 5, 9, 14, 20 over 300 bars: 296+292+287+281. */
#define RS_STREAM_CMP  1156
/* Two bad-price passes of 5 pinned bars plus bar 60, and the inconsistent
 * bar: 2*(5+1)+1. */
#define RS_GUARD_CMP     13
/* 4 prices x 2 bad values x 2 bars, 243 outputs each. */
#define RS_GUARD_BAR_CMP 3888
/* 4 prices x 2 bad values, bars 9 to 60. */
#define RS_GUARD_STREAM_CMP 416
/* 3 flat prices x periods 5, 9, 14, 20: sum(51-n) = 46+42+37+31. */
#define RS_DRIFT_RUN_CMP 468
/* Periods 5, 9, 14, 20 over 300 bars: 296+292+287+281. */
#define RS_NEGATIVE_CMP 1156
/* The forced rebuild at n = 5 and the collapse rebuild at n = 9 and 20. */
#define RS_REBUILD_BIT_CMP 3

static int g_rsGoldenCmp;
static int g_rsScaleCmp;
static int g_rsDriftCmp;
static int g_rsLookbackCmp;
static int g_rsAliasCmp;
static int g_rsCompositeCmp;
static int g_rsRebuildCmp;
static int g_rsStreamCmp;
static int g_rsGuardCmp;
static int g_rsGuardBarCmp;
static int g_rsGuardStreamCmp;
static int g_rsDriftRunCmp;
static int g_rsNegativeCmp;
static int g_rsRebuildBitCmp;

typedef struct
{
   int    period;
   double annualization;
   int    bar;
   double value;
} RsGolden;

/* From a 60-digit evaluation of eq. (2) over the committed corpus, rounded
 * once to 17 significant digits. Bar 26 is one of the three drift-only bars,
 * so its single-bar value is exactly zero rather than a small number. */
static const RsGolden rsGolden[] =
{
   {  10,   1.0,   9, 0.017975900777358125 },
   {  10,   1.0,  10, 0.018397071908132254 },
   {  10,   1.0,  50, 0.016447603146939250 },
   {  10,   1.0, 100, 0.019839725374094775 },
   {  10,   1.0, 200, 0.022608525797841318 },
   {  10,   1.0, 251, 0.013464874272606921 },
   {  10, 252.0,   9, 0.285358578295573540 },
   {  10, 252.0, 251, 0.213748252560417820 },
   {  20,   1.0,  19, 0.021118612248610193 },
   {  20,   1.0, 251, 0.022956763453686233 },
   {   1,   1.0,   0, 0.017612307025905667 },
   {   1,   1.0,  26, 0.0                  },
   {   1,   1.0, 251, 0.017892431307494070 },
   { 252,   1.0, 251, 0.020359182379840250 }
};

#define RS_NB_GOLDEN ((int)(sizeof(rsGolden)/sizeof(rsGolden[0])))

/* The 60-digit rows reproduce to 3e-15. */
#define RS_GOLDEN_TOL 1e-13

/* The corpus bars whose open is their low and whose close is their high --
 * all drift, no dispersion. Verified below to be the WHOLE set, not a sample. */
static const int rsDriftBar[] = { 26, 90, 214 };
#define RS_NB_DRIFT ((int)(sizeof(rsDriftBar)/sizeof(rsDriftBar[0])))

static ErrorNumber test_rs_golden  ( const TA_History *history );
static ErrorNumber test_rs_scale   ( const TA_History *history );
static ErrorNumber test_rs_drift   ( const TA_History *history );
static ErrorNumber test_rs_lookback( const TA_History *history );
static ErrorNumber test_rs_aliasing( const TA_History *history );
static ErrorNumber test_rs_range   ( const TA_History *history );
static ErrorNumber test_rs_composite( const TA_History *history );
static ErrorNumber test_rs_rebuild  ( void );
static ErrorNumber test_rs_stream   ( void );
static ErrorNumber test_rs_guard    ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_rogerssatchell( TA_History *history )
{
   ErrorNumber retValue;

   g_rsGoldenCmp = 0;
   g_rsScaleCmp = 0;
   g_rsDriftCmp = 0;
   g_rsLookbackCmp = 0;
   g_rsAliasCmp = 0;
   g_rsCompositeCmp = 0;
   g_rsRebuildCmp = 0;
   g_rsStreamCmp = 0;
   g_rsGuardCmp = 0;
   g_rsGuardBarCmp = 0;
   g_rsGuardStreamCmp = 0;
   g_rsDriftRunCmp = 0;
   g_rsNegativeCmp = 0;
   g_rsRebuildBitCmp = 0;

   if( history->nbBars != RS_NB_BAR )
   {
      printf( "Fail: TA_ROGERSSATCHELL expects the %d-bar corpus, got %d\n",
              RS_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   retValue = test_rs_golden( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_scale( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_drift( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_lookback( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_composite( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_rebuild();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_stream();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_rs_guard( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   if( g_rsGoldenCmp   != RS_GOLDEN_CMP
    || g_rsScaleCmp    != RS_SCALE_CMP
    || g_rsDriftCmp    != RS_DRIFT_CMP
    || g_rsLookbackCmp != RS_LOOKBACK_CMP
    || g_rsAliasCmp    != RS_ALIAS_CMP
    || g_rsCompositeCmp != RS_COMPOSITE_CMP
    || g_rsRebuildCmp  != RS_REBUILD_CMP
    || g_rsStreamCmp   != RS_STREAM_CMP
    || g_rsGuardCmp    != RS_GUARD_CMP
    || g_rsGuardBarCmp != RS_GUARD_BAR_CMP
    || g_rsGuardStreamCmp != RS_GUARD_STREAM_CMP
    || g_rsDriftRunCmp != RS_DRIFT_RUN_CMP
    || g_rsNegativeCmp != RS_NEGATIVE_CMP
    || g_rsRebuildBitCmp != RS_REBUILD_BIT_CMP )
   {
      printf( "Fail: TA_ROGERSSATCHELL comparison counts (golden %d, scale %d, "
              "drift %d, lookback %d, alias %d, composite %d, rebuild %d, "
              "stream %d, guard %d, guard bar %d, guard stream %d, "
              "drift run %d, negative %d, rebuild bits %d) are not what this "
              "file asserts (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
              "%d, %d)\n",
              g_rsGoldenCmp, g_rsScaleCmp, g_rsDriftCmp, g_rsLookbackCmp,
              g_rsAliasCmp, g_rsCompositeCmp, g_rsRebuildCmp, g_rsStreamCmp,
              g_rsGuardCmp, g_rsGuardBarCmp, g_rsGuardStreamCmp,
              g_rsDriftRunCmp, g_rsNegativeCmp, g_rsRebuildBitCmp,
              RS_GOLDEN_CMP, RS_SCALE_CMP, RS_DRIFT_CMP, RS_LOOKBACK_CMP,
              RS_ALIAS_CMP, RS_COMPOSITE_CMP, RS_REBUILD_CMP, RS_STREAM_CMP,
              RS_GUARD_CMP, RS_GUARD_BAR_CMP, RS_GUARD_STREAM_CMP,
              RS_DRIFT_RUN_CMP, RS_NEGATIVE_CMP, RS_REBUILD_BIT_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.    ****/

/* (1) Frozen goldens from the primary's own equation. */
static ErrorNumber test_rs_golden( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   static TA_Real out[RS_NB_BAR];
   int k;

   for( k = 0; k < RS_NB_GOLDEN; k++ )
   {
      const RsGolden *g = &rsGolden[k];
      double got, err;

      rc = TA_ROGERSSATCHELL( 0, (int)history->nbBars - 1,
                              history->open, history->high,
                              history->low, history->close,
                              g->period, g->annualization,
                              &begIdx, &nbElement, out );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_ROGERSSATCHELL golden rc=%d (n=%d, A=%g)\n",
                 (int)rc, g->period, g->annualization );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != g->period - 1 || g->bar < begIdx )
      {
         printf( "Fail: TA_ROGERSSATCHELL golden range: begIdx=%d (want %d), "
                 "bar %d\n", (int)begIdx, g->period - 1, g->bar );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      got = out[g->bar - begIdx];

      /* The drift-only row is an EXACT zero, not a small number: a tolerance
       * there would accept any implementation that merely gets close. */
      if( g->value == 0.0 )
      {
         if( got != 0.0 )
         {
            printf( "Fail: TA_ROGERSSATCHELL golden bar %d (n=%d): %.17g, "
                    "expected exactly 0\n", g->bar, g->period, got );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      else
      {
         err = fabs( got - g->value ) / fabs( g->value );
         if( !( err <= RS_GOLDEN_TOL ) )
         {
            printf( "Fail: TA_ROGERSSATCHELL golden bar %d (n=%d, A=%g): "
                    "%.17g, expected %.17g (rel %.3g, tol %.1e)\n",
                    g->bar, g->period, g->annualization, got, g->value,
                    err, RS_GOLDEN_TOL );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      g_rsGoldenCmp++;
   }

   return TA_TEST_PASS;
}

/* (2) The annualisation is a factor of exactly sqrt(A), and A = 0 is 0. */
static ErrorNumber test_rs_scale( const TA_History *history )
{
   static const int periods[] = { 1, 10, 20 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   static TA_Real base[RS_NB_BAR], scaled[RS_NB_BAR];
   double sqrtA;
   int p, i;

   sqrtA = sqrt( 252.0 );

   for( p = 0; p < (int)(sizeof(periods)/sizeof(periods[0])); p++ )
   {
      int n = periods[p];

      rc = TA_ROGERSSATCHELL( 0, (int)history->nbBars - 1,
                              history->open, history->high,
                              history->low, history->close,
                              n, 1.0, &begIdx, &nbElement, base );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_ROGERSSATCHELL scale A=1 rc=%d (n=%d)\n", (int)rc, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      rc = TA_ROGERSSATCHELL( 0, (int)history->nbBars - 1,
                              history->open, history->high,
                              history->low, history->close,
                              n, 252.0, &begIdx2, &nbElement2, scaled );
      if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "Fail: TA_ROGERSSATCHELL scale A=252 rc=%d, range %d/%d vs "
                 "%d/%d (n=%d)\n", (int)rc, (int)begIdx2, (int)nbElement2,
                 (int)begIdx, (int)nbElement, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         double want = sqrtA * base[i];
         /* Bitwise: the factor multiplies the finished per-bar value. A
          * folded inside the root lands on different bits. */
         if( memcmp( &want, &scaled[i], sizeof(double) ) != 0 )
         {
            printf( "Fail: TA_ROGERSSATCHELL scale n=%d bar %d: %.17g, "
                    "expected sqrt(252)*%.17g = %.17g\n",
                    n, (int)begIdx + i, scaled[i], base[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsScaleCmp++;
      }

      rc = TA_ROGERSSATCHELL( 0, (int)history->nbBars - 1,
                              history->open, history->high,
                              history->low, history->close,
                              n, 0.0, &begIdx2, &nbElement2, scaled );
      if( rc != TA_SUCCESS || nbElement2 != nbElement )
      {
         printf( "Fail: TA_ROGERSSATCHELL scale A=0 rc=%d (n=%d)\n", (int)rc, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < nbElement2; i++ )
      {
         if( scaled[i] != 0.0 )
         {
            printf( "Fail: TA_ROGERSSATCHELL A=0 n=%d bar %d: %.17g, expected 0\n",
                    n, (int)begIdx2 + i, scaled[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsScaleCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (3) Unbiased whatever the drift: a bar that opens at its low and closes at
 * its high reads exactly zero. */
static ErrorNumber test_rs_drift( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   static TA_Real out[RS_NB_BAR];
   int i, k, nbFound;

   rc = TA_ROGERSSATCHELL( 0, (int)history->nbBars - 1,
                           history->open, history->high,
                           history->low, history->close,
                           1, 1.0, &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS || begIdx != 0 || nbElement != (TA_Integer)history->nbBars )
   {
      printf( "Fail: TA_ROGERSSATCHELL drift rc=%d range %d/%d\n",
              (int)rc, (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* The named bars read exactly zero ... */
   for( k = 0; k < RS_NB_DRIFT; k++ )
   {
      int bar = rsDriftBar[k];
      if( !( history->open[bar] == history->low[bar]
          && history->close[bar] == history->high[bar] ) )
      {
         printf( "Fail: corpus bar %d is not the open-at-low, close-at-high "
                 "shape this leg rests on\n", bar );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
      if( out[bar] != 0.0 )
      {
         printf( "Fail: TA_ROGERSSATCHELL drift bar %d: %.17g, expected "
                 "exactly 0\n", bar, out[bar] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_rsDriftCmp++;
   }

   /* ... and they are the ONLY bars that do. Without this half the leg would
    * pass an implementation that returns zero everywhere. */
   nbFound = 0;
   for( i = 0; i < nbElement; i++ )
   {
      if( out[i] == 0.0 ) nbFound++;
      g_rsDriftCmp++;
   }
   if( nbFound != RS_NB_DRIFT )
   {
      printf( "Fail: TA_ROGERSSATCHELL drift: %d corpus bars read exactly 0 "
              "at n=1, expected %d\n", nbFound, RS_NB_DRIFT );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (4) The lookback is the window's, and the annualisation cannot move it. */
static ErrorNumber test_rs_lookback( const TA_History *history )
{
   static const int periods[] = { 1, 2, 10, 100000 };
   static const double scales[] = { 0.0, 1.0, 252.0, 3e37 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   static TA_Real out[RS_NB_BAR];
   int p, s;

   for( p = 0; p < (int)(sizeof(periods)/sizeof(periods[0])); p++ )
   {
      for( s = 0; s < (int)(sizeof(scales)/sizeof(scales[0])); s++ )
      {
         int want = periods[p] - 1;
         int got = TA_ROGERSSATCHELL_Lookback( periods[p], scales[s] );
         if( got != want )
         {
            printf( "Fail: TA_ROGERSSATCHELL_Lookback(%d, %g) = %d, expected %d\n",
                    periods[p], scales[s], got, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsLookbackCmp++;
      }
   }

   /* An out-of-range annualisation is refused, by both entry points. NaN
    * included: the generated test is !(x >= min && x <= max), so a NaN fails
    * it rather than slipping through a pair of ordered comparisons. */
   {
      double bad[3];
      int b;

      bad[0] = -1e-300;
      bad[1] = nextafter( 3e37, 1e308 );
      bad[2] = (double)NAN;

      for( b = 0; b < 3; b++ )
      {
         if( TA_ROGERSSATCHELL_Lookback( 10, bad[b] ) != -1 )
         {
            printf( "Fail: TA_ROGERSSATCHELL_Lookback(10, %g) did not answer -1\n",
                    bad[b] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsLookbackCmp++;

         rc = TA_ROGERSSATCHELL( 0, (int)history->nbBars - 1,
                                 history->open, history->high,
                                 history->low, history->close,
                                 10, bad[b], &begIdx, &nbElement, out );
         if( rc != TA_BAD_PARAM )
         {
            printf( "Fail: TA_ROGERSSATCHELL with annualisation %g answered "
                    "rc=%d, expected TA_BAD_PARAM\n", bad[b], (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         g_rsLookbackCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (5) outReal may be any of the four inputs. */
static ErrorNumber test_rs_aliasing( const TA_History *history )
{
   const TA_Real *src[4];
   static TA_Real ref[RS_NB_BAR], work[RS_NB_BAR];
   static const char * const name[4] = { "inOpen", "inHigh", "inLow", "inClose" };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int which, i, nb;

   nb = (int)history->nbBars;

   rc = TA_ROGERSSATCHELL( 0, nb-1, history->open, history->high,
                           history->low, history->close,
                           10, 252.0, &begIdx, &nbElement, ref );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_ROGERSSATCHELL aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   src[0] = history->open;
   src[1] = history->high;
   src[2] = history->low;
   src[3] = history->close;

   for( which = 0; which < 4; which++ )
   {
      const TA_Real *in[4];

      for( i = 0; i < nb; i++ )
         work[i] = src[which][i];
      for( i = 0; i < 4; i++ )
         in[i] = ( i == which ) ? work : src[i];

      if( TA_ROGERSSATCHELL( 0, nb-1, in[0], in[1], in[2], in[3],
                             10, 252.0, &begIdx2, &nbElement2, work ) != TA_SUCCESS
          || begIdx2 != begIdx || nbElement2 != nbElement
          || memcmp( ref, work, (size_t)nbElement * sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_ROGERSSATCHELL aliasing over %s\n", name[which] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_rsAliasCmp++;
   }

   return TA_TEST_PASS;
}

/* (6) A sub-range equals the same slice of a full run. */
static TA_RetCode rsRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                       TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                       TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                       TA_Integer *lookback, void *opaqueData,
                                       unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_ROGERSSATCHELL_Lookback( 10, 252.0 );
   return TA_ROGERSSATCHELL( startIdx, endIdx, h->open, h->high, h->low, h->close,
                             10, 252.0, outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_rs_range( const TA_History *history )
{
   return doRangeTestEx( rsRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)history, 1, 0 );
}

/* (7) COMPOSITE, bit-exact. The same estimator built out of shipped
 * primitives -- TA_DIV, TA_LN, TA_MULT, TA_ADD for the term, TA_SUM for the
 * window -- must land on the SAME BITS as the fused function.
 *
 * This is the leg that pins the term's evaluation order, and it is the reason
 * the two products are separate statements in the source. Written as one
 * expression the generator fuses the first product into the add, which moves
 * most of the outputs by an ulp: a tolerance would absorb that, these bits do
 * not.
 *
 * Periods 8 and up only. The window's forced rebuild fires every 32n bars,
 * which cannot happen inside 252 bars once n >= 8, and the collapse trigger
 * stays silent on this corpus -- every bar is consistent and no window
 * collapses. At n = 7 the forced rebuild does fire and the two spellings part
 * company, correctly: that is the running sum being rebuilt, not a defect.
 */
static ErrorNumber test_rs_composite( const TA_History *history )
{
   static const int periods[] = { 8, 10, 14, 20 };
   static TA_Real hc[RS_NB_BAR], ho[RS_NB_BAR], lc[RS_NB_BAR], lo[RS_NB_BAR];
   static TA_Real p1[RS_NB_BAR], p2[RS_NB_BAR], term[RS_NB_BAR];
   static TA_Real sum[RS_NB_BAR], fused[RS_NB_BAR];
   TA_RetCode rc;
   TA_Integer beg, nb, begF, nbF;
   int nbBar, p, i;
   double sqrtA;

   nbBar = (int)history->nbBars;
   sqrtA = sqrt( 252.0 );

   /* The four ratios, then their logs, in place. */
   if( TA_DIV( 0, nbBar-1, history->high, history->close, &beg, &nb, hc ) != TA_SUCCESS
    || TA_DIV( 0, nbBar-1, history->high, history->open,  &beg, &nb, ho ) != TA_SUCCESS
    || TA_DIV( 0, nbBar-1, history->low,  history->close, &beg, &nb, lc ) != TA_SUCCESS
    || TA_DIV( 0, nbBar-1, history->low,  history->open,  &beg, &nb, lo ) != TA_SUCCESS
    || TA_LN ( 0, nbBar-1, hc, &beg, &nb, hc ) != TA_SUCCESS
    || TA_LN ( 0, nbBar-1, ho, &beg, &nb, ho ) != TA_SUCCESS
    || TA_LN ( 0, nbBar-1, lc, &beg, &nb, lc ) != TA_SUCCESS
    || TA_LN ( 0, nbBar-1, lo, &beg, &nb, lo ) != TA_SUCCESS
    || TA_MULT( 0, nbBar-1, hc, ho, &beg, &nb, p1 ) != TA_SUCCESS
    || TA_MULT( 0, nbBar-1, lc, lo, &beg, &nb, p2 ) != TA_SUCCESS
    || TA_ADD ( 0, nbBar-1, p1, p2, &beg, &nb, term ) != TA_SUCCESS )
   {
      printf( "Fail: TA_ROGERSSATCHELL composite: building the term failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != 0 || nb != nbBar )
   {
      printf( "Fail: TA_ROGERSSATCHELL composite term range %d/%d, want 0/%d\n",
              (int)beg, (int)nb, nbBar );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   for( p = 0; p < (int)(sizeof(periods)/sizeof(periods[0])); p++ )
   {
      int n = periods[p];

      rc = TA_SUM( 0, nbBar-1, term, n, &beg, &nb, sum );
      if( rc != TA_SUCCESS || beg != n - 1 )
      {
         printf( "Fail: TA_ROGERSSATCHELL composite TA_SUM rc=%d beg=%d (n=%d)\n",
                 (int)rc, (int)beg, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      rc = TA_ROGERSSATCHELL( 0, nbBar-1, history->open, history->high,
                              history->low, history->close,
                              n, 252.0, &begF, &nbF, fused );
      if( rc != TA_SUCCESS || begF != beg || nbF != nb )
      {
         printf( "Fail: TA_ROGERSSATCHELL composite fused rc=%d range %d/%d vs "
                 "%d/%d (n=%d)\n", (int)rc, (int)begF, (int)nbF,
                 (int)beg, (int)nb, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nb; i++ )
      {
         double want = sqrtA * sqrt( sum[i] / (double)n );

         if( memcmp( &want, &fused[i], sizeof(double) ) != 0 )
         {
            printf( "Fail: TA_ROGERSSATCHELL composite n=%d bar %d: fused "
                    "%.17g, composition %.17g\n",
                    n, (int)beg + i, fused[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsCompositeCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* ---- Synthetic series for the running-sum legs -------------------------- */

#define RS_SYN_N      300
#define RS_FLAT_LEN    50   /* bars; real bars resume right after */

/* Which sign the residue of a missing rebuild takes depends on the terms the
 * window carried when the flat run began, and a negative one is hidden on an
 * all-flat window by the S <= 0 rule. The legs sweep the cut point so they do
 * not rest on one draw of it. */
static int g_rsFlatFrom, g_rsFlatTo;

static TA_Real g_synO[RS_SYN_N], g_synH[RS_SYN_N];
static TA_Real g_synL[RS_SYN_N], g_synC[RS_SYN_N];
static TA_Real g_synRef[RS_SYN_N];

/* A trending-with-cycle series whose bars flatFrom to flatFrom+RS_FLAT_LEN-1
 * are replaced: shape 0 leaves them flat at `flat`, shape 1 makes each open at
 * its low and close at its high, shape 2 is flat with one bar whose high sits
 * strictly inside its open and close. Real bars resume after the run.
 */
static void rsBuildSeriesShape( double flat, int flatFrom, int shape )
{
   int i;
   double p, o;

   g_rsFlatFrom = flatFrom;
   g_rsFlatTo = flatFrom + RS_FLAT_LEN - 1;

   for( i = 0; i < RS_SYN_N; i++ )
   {
      if( i >= g_rsFlatFrom && i <= g_rsFlatTo )
      {
         g_synO[i] = g_synH[i] = g_synL[i] = g_synC[i] = flat;
         if( shape == 1 )
         {
            /* A different rise on each bar, so the window is not one bar
             * repeated. */
            g_synH[i] = flat * ( 1.0 + 0.001 * (double)( 1 + i % 7 ) );
            g_synC[i] = g_synH[i];
         }
         else if( shape == 2 && i == g_rsFlatFrom + RS_FLAT_LEN / 2 )
         {
            g_synO[i] = flat;
            g_synH[i] = flat * 1.005;
            g_synL[i] = flat * 0.9999;
            g_synC[i] = flat * 1.01;
         }
         continue;
      }
      p = 100.0 + 10.0 * sin( (double)i / 7.0 ) + 0.15 * (double)i;
      o = ( i == 0 ) ? p : g_synC[i-1];
      g_synC[i] = p;
      g_synO[i] = o;
      g_synH[i] = ( o > p ? o : p ) + 0.5;
      g_synL[i] = ( o < p ? o : p ) - 0.5;
   }
}

static void rsBuildSeries( double flat, int flatFrom )
{
   rsBuildSeriesShape( flat, flatFrom, 0 );
}

/* One bar of 20% range among bars whose range is 1e-6 of price: the window
 * sum is dominated by the shock while it is inside, and is pure rounding
 * residue once it leaves. */
static void rsBuildShockSeries( void )
{
   int i;

   for( i = 0; i < RS_SYN_N; i++ )
   {
      double p = 100.0 + 0.01 * (double)i;
      double w = ( i == 150 ) ? 0.20 : 1e-6;

      g_synO[i] = p;
      g_synC[i] = p * ( 1.0 + w * 0.25 );
      g_synH[i] = p * ( 1.0 + w );
      g_synL[i] = p * ( 1.0 - w );
   }
}

/* The reference: the same terms, but the window summed FRESH on every bar.
 * Fills g_synRef[i] for i >= n-1. */
static void rsFreshReference( int n, double annualization )
{
   int i, j;
   double sqrtA = sqrt( annualization );

   for( i = 0; i < RS_SYN_N; i++ )
      g_synRef[i] = 0.0;

   for( i = n - 1; i < RS_SYN_N; i++ )
   {
      double s = 0.0;

      for( j = i - n + 1; j <= i; j++ )
      {
         double o = g_synO[j], h = g_synH[j], l = g_synL[j], c = g_synC[j];
         double t;

         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 )
         {
            double q1 = log( h / c ) * log( h / o );
            double q2 = log( l / c ) * log( l / o );
            t = q1 + q2;
         }
         else
            t = 0.0;
         s += t;
      }

      g_synRef[i] = ( s > 0.0 ) ? sqrtA * sqrt( s / (double)n ) : 0.0;
   }
}

/* (8) The running sum is rebuilt, and that is what keeps it honest. */
static ErrorNumber test_rs_rebuild( void )
{
   static const int    periods[] = { 5, 9, 14, 20, 50 };
   static const double flats[]   = { 100.0, 87.5, 250.25 };
   static const int    cuts[]    = { 170, 185, 200, 215, 230 };
   static TA_Real out[RS_SYN_N];
   TA_RetCode rc;
   TA_Integer beg, nb;
   int p, f, cu, i;

   for( cu = 0; cu < (int)(sizeof(cuts)/sizeof(cuts[0])); cu++ )
   for( f = 0; f < (int)(sizeof(flats)/sizeof(flats[0])); f++ )
   {
      rsBuildSeries( flats[f], cuts[cu] );

      for( p = 0; p < (int)(sizeof(periods)/sizeof(periods[0])); p++ )
      {
         int n = periods[p];

         rsFreshReference( n, 1.0 );

         rc = TA_ROGERSSATCHELL( 0, RS_SYN_N-1, g_synO, g_synH, g_synL, g_synC,
                                 n, 1.0, &beg, &nb, out );
         if( rc != TA_SUCCESS || beg != n - 1 )
         {
            printf( "Fail: TA_ROGERSSATCHELL rebuild rc=%d beg=%d (n=%d, flat %g)\n",
                    (int)rc, (int)beg, n, flats[f] );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nb; i++ )
         {
            int bar = (int)beg + i;
            double want = g_synRef[bar];
            double err;

            /* Exactly zero, not small: a tolerance here would accept the
             * residue the rebuild exists to remove. */
            if( bar >= g_rsFlatFrom + n - 1 && bar <= g_rsFlatTo )
            {
               if( out[i] != 0.0 )
               {
                  printf( "Fail: TA_ROGERSSATCHELL all-flat window n=%d bar %d "
                          "(flat %g, cut %d): %.17g, expected exactly 0\n",
                          n, bar, flats[f], cuts[cu], out[i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               g_rsRebuildCmp++;
            }

            /* Every bar is within 1e-12 of a fresh sum of the same terms. */
            err = ( want != 0.0 ) ? fabs( out[i] - want ) / fabs( want )
                                  : fabs( out[i] - want );
            if( !( err <= 1e-12 ) )
            {
               printf( "Fail: TA_ROGERSSATCHELL rebuild n=%d bar %d (flat %g): "
                       "%.17g, fresh-sum reference %.17g (err %.3g, cut %d)\n",
                       n, bar, flats[f], out[i], want, err, cuts[cu] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_rsRebuildCmp++;
         }
      }
   }

   /* Shock then calm: the window carries a 20% bar for n bars, then nothing
    * but 1e-6 bars. A running sum that is never rebuilt is orders of
    * magnitude outside this tolerance once the shock has left. */
   rsBuildShockSeries();
   for( p = 0; p < 2; p++ )
   {
      int n = ( p == 0 ) ? 9 : 20;

      rsFreshReference( n, 1.0 );

      rc = TA_ROGERSSATCHELL( 0, RS_SYN_N-1, g_synO, g_synH, g_synL, g_synC,
                              n, 1.0, &beg, &nb, out );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_ROGERSSATCHELL shock rc=%d (n=%d)\n", (int)rc, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < nb; i++ )
      {
         int bar = (int)beg + i;
         double want = g_synRef[bar];
         double err = ( want != 0.0 ) ? fabs( out[i] - want ) / fabs( want )
                                      : fabs( out[i] - want );
         if( !( err <= 1e-12 ) )
         {
            printf( "Fail: TA_ROGERSSATCHELL shock n=%d bar %d: %.17g, "
                    "fresh-sum reference %.17g (err %.3g)\n",
                    n, bar, out[i], want, err );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsRebuildCmp++;
      }

      /* The first window without the shock collapses and is rebuilt, so
       * its output is the fresh sum taken oldest first, bit for bit. */
      if( memcmp( &out[150 + n - (int)beg], &g_synRef[150 + n],
                  sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_ROGERSSATCHELL shock n=%d bar %d: %.17g, the "
                 "rebuilt sum gives %.17g\n", n, 150 + n,
                 out[150 + n - (int)beg], g_synRef[150 + n] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_rsRebuildBitCmp++;
   }

   /* With no collapse before it, output 32n is the first forced rebuild. */
   rsBuildSeries( 100.0, 200 );
   rsFreshReference( 5, 1.0 );
   rc = TA_ROGERSSATCHELL( 0, RS_SYN_N-1, g_synO, g_synH, g_synL, g_synC,
                           5, 1.0, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 4 )
   {
      printf( "Fail: TA_ROGERSSATCHELL forced rebuild rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( memcmp( &out[32*5 - 1], &g_synRef[4 + 32*5 - 1], sizeof(double) ) != 0 )
   {
      printf( "Fail: TA_ROGERSSATCHELL forced rebuild n=5 bar %d: %.17g, the "
              "rebuilt sum gives %.17g\n", 4 + 32*5 - 1, out[32*5 - 1],
              g_synRef[4 + 32*5 - 1] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_rsRebuildBitCmp++;

   /* A bar that opens at its low and closes at its high has a term of
    * exactly 0.0, so a window of them reads 0.0 at any period. */
   for( f = 0; f < (int)(sizeof(flats)/sizeof(flats[0])); f++ )
   {
      rsBuildSeriesShape( flats[f], 200, 1 );
      for( p = 0; p < 4; p++ )
      {
         int n = periods[p];

         rc = TA_ROGERSSATCHELL( 0, RS_SYN_N-1, g_synO, g_synH, g_synL, g_synC,
                                 n, 1.0, &beg, &nb, out );
         if( rc != TA_SUCCESS || beg != n - 1 )
         {
            printf( "Fail: TA_ROGERSSATCHELL drift run rc=%d (n=%d)\n", (int)rc, n );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         for( i = g_rsFlatFrom + n - 1; i <= g_rsFlatTo; i++ )
         {
            if( out[i - (int)beg] != 0.0 )
            {
               printf( "Fail: TA_ROGERSSATCHELL drift-only window n=%d bar %d "
                       "(price %g): %.17g, expected exactly 0\n",
                       n, i, flats[f], out[i - (int)beg] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_rsDriftRunCmp++;
         }
      }
   }

   /* A window whose sum is below zero answers 0.0 at n > 1, and the windows
    * around it match a fresh sum. */
   rsBuildSeriesShape( 100.0, 200, 2 );
   for( p = 0; p < 4; p++ )
   {
      int n = periods[p];

      rsFreshReference( n, 1.0 );

      rc = TA_ROGERSSATCHELL( 0, RS_SYN_N-1, g_synO, g_synH, g_synL, g_synC,
                              n, 1.0, &beg, &nb, out );
      if( rc != TA_SUCCESS || beg != n - 1 )
      {
         printf( "Fail: TA_ROGERSSATCHELL negative sum rc=%d (n=%d)\n", (int)rc, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( n == 9 && server_verify_active() )
      {
         const double opt[2] = { 9.0, 1.0 };
         ErrorNumber e = server_verify( "ROGERSSATCHELL", 0, RS_SYN_N-1,
                            RS_SYN_N, rc, beg, nb,
                            (const TA_Real*[]){ g_synO, g_synH, g_synL,
                                                g_synC, NULL },
                            opt, 2,
                            (const TA_Real*[]){ out, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
      if( out[g_rsFlatFrom + RS_FLAT_LEN / 2 - (int)beg] != 0.0 )
      {
         printf( "Fail: TA_ROGERSSATCHELL negative sum n=%d: %.17g, expected "
                 "exactly 0\n", n, out[g_rsFlatFrom + RS_FLAT_LEN / 2 - (int)beg] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nb; i++ )
      {
         int bar = (int)beg + i;
         double want = g_synRef[bar];
         double err = ( want != 0.0 ) ? fabs( out[i] - want ) / fabs( want )
                                      : fabs( out[i] - want );
         if( !( err <= 1e-12 ) )
         {
            printf( "Fail: TA_ROGERSSATCHELL negative sum n=%d bar %d: %.17g, "
                    "fresh-sum reference %.17g (err %.3g)\n",
                    n, bar, out[i], want, err );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsNegativeCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (9) The stream tier carries the same rebuild. A missing one there is
 * invisible to every batch leg above. */
static ErrorNumber test_rs_stream( void )
{
   static const int periods[] = { 5, 9, 14, 20 };
   static TA_Real batch[RS_SYN_N];
   TA_ROGERSSATCHELL_Stream *s;
   TA_RetCode rc;
   TA_Integer beg, nb;
   int p, bar;

   rsBuildSeries( 100.0, 200 );

   for( p = 0; p < (int)(sizeof(periods)/sizeof(periods[0])); p++ )
   {
      int n = periods[p];
      double got;

      rc = TA_ROGERSSATCHELL( 0, RS_SYN_N-1, g_synO, g_synH, g_synL, g_synC,
                              n, 252.0, &beg, &nb, batch );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_ROGERSSATCHELL stream batch rc=%d (n=%d)\n", (int)rc, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      s = NULL;
      rc = TA_ROGERSSATCHELL_Open( &s, g_synO, g_synH, g_synL, g_synC,
                                   n, n, 252.0, &got );
      if( rc != TA_SUCCESS || s == NULL )
      {
         printf( "Fail: TA_ROGERSSATCHELL_Open rc=%d (n=%d)\n", (int)rc, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( bar = n - 1; bar < RS_SYN_N; bar++ )
      {
         double want = batch[bar - (int)beg];

         if( memcmp( &got, &want, sizeof(double) ) != 0 )
         {
            printf( "Fail: TA_ROGERSSATCHELL stream n=%d bar %d: %.17g, batch "
                    "%.17g\n", n, bar, got, want );
            TA_ROGERSSATCHELL_Close( s );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsStreamCmp++;

         if( bar + 1 >= RS_SYN_N ) break;
         rc = TA_ROGERSSATCHELL_Update( s, g_synO[bar+1], g_synH[bar+1],
                                        g_synL[bar+1], g_synC[bar+1], &got );
         if( rc != TA_SUCCESS )
         {
            printf( "Fail: TA_ROGERSSATCHELL_Update rc=%d (n=%d, bar %d)\n",
                    (int)rc, n, bar+1 );
            TA_ROGERSSATCHELL_Close( s );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
      }

      TA_ROGERSSATCHELL_Close( s );
   }

   return TA_TEST_PASS;
}

/* ---- Guard differentials ------------------------------------------------ */

/* The corpus with one bad price at bar 50, n = 10, from the same 60-digit
 * evaluation as the goldens -- with the guard's own rule applied there, since
 * the rule is TA-Lib's and not the paper's. The values are the SAME whether
 * the low is set to zero or to its own negation: both are "not a price", and
 * the whole bar contributes 0.0 either way.
 *
 * Zeroing only the products that touch the bad price reads 0.016439 at bar 50,
 * and dropping the bar from the window reads 0 there and 0.0183565 at bar 51:
 * both are outside the tolerance, so the leg tells the three readings apart.
 */
static const int    rsGuardBar[]   = { 49, 50, 51, 59, 60 };
static const double rsGuardValue[] =
{
   0.017077499399512471,
   0.016347065982245768,
   0.016064887412207542,
   0.018290377457680515,
   0.019040160632441901
};
#define RS_NB_GUARD ((int)(sizeof(rsGuardBar)/sizeof(rsGuardBar[0])))

/* (10) A price at or below zero zeroes its bar's term and the bar still
 * counts; a bar whose high or low sits inside its open and close can drive
 * the window sum negative, and that answers 0 rather than reaching the root. */
static ErrorNumber test_rs_guard( const TA_History *history )
{
   static TA_Real o[RS_NB_BAR], h[RS_NB_BAR], l[RS_NB_BAR], c[RS_NB_BAR];
   static TA_Real out[RS_NB_BAR], clean[RS_NB_BAR];
   TA_RetCode rc;
   TA_Integer beg, nb, begC, nbC;
   int pass, i, k;

   rc = TA_ROGERSSATCHELL( 0, RS_NB_BAR-1, history->open, history->high,
                           history->low, history->close,
                           10, 1.0, &begC, &nbC, clean );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_ROGERSSATCHELL guard: the clean baseline failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( pass = 0; pass < 2; pass++ )
   {
      const char *what = ( pass == 0 ) ? "low[50] = 0" : "low[50] = -low[50]";

      for( i = 0; i < RS_NB_BAR; i++ )
      {
         o[i] = history->open[i];
         h[i] = history->high[i];
         l[i] = history->low[i];
         c[i] = history->close[i];
      }
      l[50] = ( pass == 0 ) ? 0.0 : -l[50];

      rc = TA_ROGERSSATCHELL( 0, RS_NB_BAR-1, o, h, l, c, 10, 1.0,
                              &beg, &nb, out );
      if( rc != TA_SUCCESS || beg != begC || nb != nbC )
      {
         printf( "Fail: TA_ROGERSSATCHELL guard %s: rc=%d range %d/%d\n",
                 what, (int)rc, (int)beg, (int)nb );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( server_verify_active() )
      {
         const double opt[2] = { 10.0, 1.0 };
         ErrorNumber e = server_verify( "ROGERSSATCHELL", 0, RS_NB_BAR-1,
                            RS_NB_BAR, rc, beg, nb,
                            (const TA_Real*[]){ o, h, l, c, NULL },
                            opt, 2,
                            (const TA_Real*[]){ out, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }

      for( k = 0; k < RS_NB_GUARD; k++ )
      {
         int bar = rsGuardBar[k];
         double got = out[bar - (int)beg];
         double err = fabs( got - rsGuardValue[k] ) / fabs( rsGuardValue[k] );

         if( !( err <= RS_GOLDEN_TOL ) )
         {
            printf( "Fail: TA_ROGERSSATCHELL guard %s bar %d: %.17g, expected "
                    "%.17g (rel %.3g)\n", what, bar, got, rsGuardValue[k], err );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsGuardCmp++;
      }

      /* Bar 60's window is [51, 60]: the bad bar has left it. Not bitwise,
       * the running sum reached this bar through a different history. */
      {
         int bar = 60;
         double want = clean[bar - (int)begC];
         double err = fabs( out[bar - (int)beg] - want ) / fabs( want );
         if( !( err <= RS_GOLDEN_TOL ) )
         {
            printf( "Fail: TA_ROGERSSATCHELL guard %s bar 60: %.17g, the clean "
                    "run gives %.17g; the bad bar has left this window\n",
                    what, out[bar - (int)beg], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsGuardCmp++;
      }
   }

   /* The guard is the whole bar: whichever price is at or below zero, the
    * term is 0.0, which is bit for bit the term of a flat bar. Bar 3 is read
    * by the warm-up, bar 50 by the main loop and, streamed, by Update. */
   for( pass = 0; pass < 16; pass++ )
   {
      int which = pass % 4;
      int bar = ( pass & 4 ) ? 50 : 3;
      double bad;
      TA_Real *price;

      for( i = 0; i < RS_NB_BAR; i++ )
      {
         o[i] = history->open[i];
         h[i] = history->high[i];
         l[i] = history->low[i];
         c[i] = history->close[i];
      }
      o[bar] = h[bar] = l[bar] = c[bar] = history->close[bar];
      rc = TA_ROGERSSATCHELL( 0, RS_NB_BAR-1, o, h, l, c, 10, 1.0,
                              &begC, &nbC, clean );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_ROGERSSATCHELL guard flat bar %d: rc=%d\n", bar, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      o[bar] = history->open[bar];
      h[bar] = history->high[bar];
      l[bar] = history->low[bar];
      c[bar] = history->close[bar];
      price = ( which == 0 ) ? o : ( which == 1 ) ? h : ( which == 2 ) ? l : c;
      bad = ( pass & 8 ) ? -price[bar] : 0.0;
      price[bar] = bad;

      rc = TA_ROGERSSATCHELL( 0, RS_NB_BAR-1, o, h, l, c, 10, 1.0,
                              &beg, &nb, out );
      if( rc != TA_SUCCESS || beg != begC || nb != nbC )
      {
         printf( "Fail: TA_ROGERSSATCHELL guard price %d = %g at bar %d: rc=%d "
                 "range %d/%d\n", which, bad, bar, (int)rc, (int)beg, (int)nb );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nb; i++ )
      {
         if( memcmp( &out[i], &clean[i], sizeof(double) ) != 0 )
         {
            printf( "Fail: TA_ROGERSSATCHELL guard price %d = %g at bar %d: "
                    "bar %d reads %.17g, a flat bar there gives %.17g\n",
                    which, bad, bar, (int)beg + i, out[i], clean[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_rsGuardBarCmp++;
      }

      if( bar == 50 )
      {
         TA_ROGERSSATCHELL_Stream *s = NULL;
         double got;

         rc = TA_ROGERSSATCHELL_Open( &s, o, h, l, c, 10, 10, 1.0, &got );
         if( rc != TA_SUCCESS || s == NULL )
         {
            printf( "Fail: TA_ROGERSSATCHELL_Open guard price %d = %g: rc=%d\n",
                    which, bad, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         for( i = 9; i <= 60; i++ )
         {
            if( memcmp( &got, &out[i - (int)beg], sizeof(double) ) != 0 )
            {
               printf( "Fail: TA_ROGERSSATCHELL guard stream price %d = %g: "
                       "bar %d reads %.17g, batch %.17g\n",
                       which, bad, i, got, out[i - (int)beg] );
               TA_ROGERSSATCHELL_Close( s );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_rsGuardStreamCmp++;
            rc = TA_ROGERSSATCHELL_Update( s, o[i+1], h[i+1], l[i+1], c[i+1], &got );
            if( rc != TA_SUCCESS )
            {
               printf( "Fail: TA_ROGERSSATCHELL_Update guard price %d = %g "
                       "bar %d: rc=%d\n", which, bad, i + 1, (int)rc );
               TA_ROGERSSATCHELL_Close( s );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
         }
         TA_ROGERSSATCHELL_Close( s );
      }
   }

   /* A bar whose high sits strictly inside its open and close gives a
    * NEGATIVE term (-2.3747e-5 at 60 digits here). At
    * n = 1 that is the whole window, so the sum is negative and the output is
    * 0.0 rather than a root of a negative number. */
   for( i = 0; i < RS_NB_BAR; i++ )
   {
      o[i] = history->open[i];
      h[i] = history->high[i];
      l[i] = history->low[i];
      c[i] = history->close[i];
   }
   o[50] = 100.0;
   h[50] = 100.5;
   l[50] =  99.99;
   c[50] = 101.0;

   rc = TA_ROGERSSATCHELL( 0, RS_NB_BAR-1, o, h, l, c, 1, 1.0, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 0 )
   {
      printf( "Fail: TA_ROGERSSATCHELL guard inconsistent bar: rc=%d beg=%d\n",
              (int)rc, (int)beg );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( out[50] != 0.0 )
   {
      printf( "Fail: TA_ROGERSSATCHELL inconsistent bar at n=1: %.17g, expected "
              "exactly 0\n", out[50] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_rsGuardCmp++;

   return TA_TEST_PASS;
}
