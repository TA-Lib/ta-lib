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
 *  100626 KL,CC  First version (proposal PSO, #473).
 */

/* Description:
 *
 *   Test TA_PSO (Premier Stochastic Oscillator, #473).
 *
 *   --codegen, --xlang-hash and server_verify compare every language against
 *   this library, so none of them can catch a wrong formula. Two things do.
 *
 *   The COMPOSITE leg is the primary one: TA_PSO must land on the same BITS
 *   as TA_STOCHF followed by two TA_EMA calls, entered 2*TA_EMA_Lookback(m)
 *   bars early. That offset is the point -- a gate indexing STOCHF at [k]
 *   instead of [k + 2*LE] fails a correct implementation -- and so is the
 *   affine-before-smoothing order, which moves the output by at most 6.0e-16
 *   and is therefore invisible to any golden at a sane tolerance. It also
 *   pins the per-pass seeding and the way the EMA unstable period flows
 *   through both passes.
 *
 *   The GOLDEN leg holds the formula itself, from a 60-digit evaluation over
 *   the committed corpus. It carries no transcendental in its inputs, so it
 *   is the same on every platform.
 *
 *   The SERIES leg re-reads the goldens published in #473, which were
 *   measured against LEAN on a different host. Its series is built from sin
 *   and cos, so its last bits are the platform's libm: MEASURED, perturbing
 *   every sin/cos call by one ulp moves those rows by up to 1.887e-15 and the
 *   whole series by up to 6.564e-15, which is why this leg's tolerance is
 *   1e-13 and not the 2e-15 the proposal quotes. It stays non-vacuous by a
 *   wide margin: the nearest wrong reading of the author (EMA period 4) moves
 *   those same rows by at least 2.9e-03.
 *
 *   Every comparison count is pinned, so a leg that stops comparing fails.
 *
 *   SERVER_VERIFY: the golden leg, once per (period, EMA period) pair.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define PSO_NB_BAR 252

/* The #473 series is 1200 bars. */
#define PSO_NB_SERIES 1200

/* Pinned comparison counts. A leg that stops reaching its comparator still
 * returns TA_TEST_PASS; these are what makes that a failure. */

/* 4 (n,m) sets x 8 rows. */
#define PSO_GOLDEN_CMP      32
/* The #473 table: 9 rows at 8/5 and 5 at 14/3. */
#define PSO_SERIES_CMP      14
/* 4 (n,m) sets x 3 unstable periods. Per combination, with
 * LT = PSO_Lookback: (252-LT) bars from startIdx = LT, (251-LT) from LT+1,
 * 152 from 100, and (252-LT) more for the startIdx = 0 call, which must
 * reproduce the startIdx = LT call bit for bit. That is 907 - 3*LT, and the
 * twelve LT values sum to 286. */
#define PSO_COMPOSITE_CMP 10026
/* 45 bars of exact zero inside the flat run, then the 185 outputs of the
 * machine-flat run compared against the flat run's. */
#define PSO_FLAT_CMP       230
/* 185 outputs finite, plus the assertion that one of them saturated. */
#define PSO_SPIKE_CMP      186
/* 4 (n,m) sets x 4 unstable periods, each checking _Lookback and the call's
 * own begIdx. */
#define PSO_LOOKBACK_CMP    32
/* outReal aliased onto each of the three price inputs. */
#define PSO_ALIAS_CMP        3

static int g_psoGoldenCmp;
static int g_psoSeriesCmp;
static int g_psoCompositeCmp;
static int g_psoFlatCmp;
static int g_psoSpikeCmp;
static int g_psoLookbackCmp;
static int g_psoAliasCmp;

typedef struct
{
   int    n;
   int    m;
   int    bar;
   double value;
} PsoGolden;

/* From a 60-digit evaluation of the #473 formula over the committed corpus,
 * rounded once to 17 significant digits. The first three rows of each set sit
 * on the seed, where a first-value EMA seeding differs by up to 1.9e-02; by
 * bar 100 that difference is gone, so those rows hold the recursion instead.
 * The 5/1 set is the minimum-period edge, where ema.c takes its copy path.
 */
static const PsoGolden psoGolden[] =
{
   {  8,  5,  15,  0.063364421472105206 },
   {  8,  5,  16, -0.19370106718164909  },
   {  8,  5,  17, -0.42647193235461028  },
   {  8,  5,  20, -0.68529056230584173  },
   {  8,  5,  30, -0.27392454089115698  },
   {  8,  5, 100,  0.003161819967686887 },
   {  8,  5, 180, -0.31943047251263085  },
   {  8,  5, 251, -0.045586914352121213 },
   { 14,  3,  17, -0.84091793065566722  },
   { 14,  3,  18, -0.8043574269742183   },
   { 14,  3,  19, -0.80977056564958261  },
   { 14,  3,  22, -0.95059152354547694  },
   { 14,  3,  32, -0.25828422983097082  },
   { 14,  3, 100,  0.30117427032228844  },
   { 14,  3, 180, -0.42387883044369062  },
   { 14,  3, 251, -0.57568451718029234  },
   {  5,  1,   4,  0.19096002056749753  },
   {  5,  1,   5,  0.62880465970872501  },
   {  5,  1,   6, -0.95574557616939049  },
   {  5,  1,   9,  0.22110741148108934  },
   {  5,  1,  19, -0.86583986446674643  },
   {  5,  1, 100,  0.76940749599236602  },
   {  5,  1, 180, -0.76252585390189009  },
   {  5,  1, 251, -0.75594430889008513  },
   { 21, 10,  38, -0.7784303535704421   },
   { 21, 10,  39, -0.75711731868822574  },
   { 21, 10,  40, -0.74470085812887965  },
   { 21, 10,  43, -0.5292646261771774   },
   { 21, 10,  53,  0.45084772063654244  },
   { 21, 10, 100,  0.90490814015651078  },
   { 21, 10, 180,  0.59267962294732479  },
   { 21, 10, 251, -0.079118107272664578 }
};

#define PSO_NB_GOLDEN ((int)(sizeof(psoGolden)/sizeof(psoGolden[0])))

/* The 60-digit rows reproduce to 4.4e-16 in doubles. */
#define PSO_GOLDEN_TOL 2e-15

/* The #473 table, measured on another host against LEAN. See the libm note in
 * the file header for why this tolerance is not PSO_GOLDEN_TOL. */
static const PsoGolden psoSeriesGolden[] =
{
   {  8,  5,   15,  0.7860056010793284   },
   {  8,  5,   16,  0.7545350155678846   },
   {  8,  5,   17,  0.7130885256351905   },
   {  8,  5,   20,  0.06837549597448934  },
   {  8,  5,   30, -0.9453322796731729   },
   {  8,  5,  100,  0.9135980917982862   },
   {  8,  5,  500,  0.5575334648320532   },
   {  8,  5, 1000, -0.8764431919973211   },
   {  8,  5, 1199,  0.9384492846783932   },
   { 14,  3,   17,  0.9268795731866296   },
   { 14,  3,   18,  0.8176893036685873   },
   { 14,  3,   20, -0.21223560364816868  },
   { 14,  3,  100,  0.9693521162738513   },
   { 14,  3, 1199,  0.9732440037628487   }
};

#define PSO_NB_SERIES_GOLDEN ((int)(sizeof(psoSeriesGolden)/sizeof(psoSeriesGolden[0])))
#define PSO_SERIES_TOL 1e-13

static ErrorNumber test_pso_golden   ( const TA_History *history );
static ErrorNumber test_pso_series   ( void );
static ErrorNumber test_pso_composite( const TA_History *history );
static ErrorNumber test_pso_flat     ( const TA_History *history );
static ErrorNumber test_pso_spike    ( const TA_History *history );
static ErrorNumber test_pso_lookback ( const TA_History *history );
static ErrorNumber test_pso_aliasing ( const TA_History *history );
static ErrorNumber test_pso_range    ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_pso( TA_History *history )
{
   ErrorNumber retValue;

   g_psoGoldenCmp = 0;
   g_psoSeriesCmp = 0;
   g_psoCompositeCmp = 0;
   g_psoFlatCmp = 0;
   g_psoSpikeCmp = 0;
   g_psoLookbackCmp = 0;
   g_psoAliasCmp = 0;

   if( history->nbBars != PSO_NB_BAR )
   {
      printf( "Fail: TA_PSO expects the %d-bar corpus, got %d\n",
              PSO_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   retValue = test_pso_golden( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_pso_series();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_pso_composite( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_pso_flat( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_pso_spike( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_pso_lookback( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_pso_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_pso_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   if( g_psoGoldenCmp    != PSO_GOLDEN_CMP
    || g_psoSeriesCmp    != PSO_SERIES_CMP
    || g_psoCompositeCmp != PSO_COMPOSITE_CMP
    || g_psoFlatCmp      != PSO_FLAT_CMP
    || g_psoSpikeCmp     != PSO_SPIKE_CMP
    || g_psoLookbackCmp  != PSO_LOOKBACK_CMP
    || g_psoAliasCmp     != PSO_ALIAS_CMP )
   {
      printf( "Fail: TA_PSO comparison counts (golden %d, series %d, "
              "composite %d, flat %d, spike %d, lookback %d, alias %d) are "
              "not what this file asserts (%d, %d, %d, %d, %d, %d, %d)\n",
              g_psoGoldenCmp, g_psoSeriesCmp, g_psoCompositeCmp,
              g_psoFlatCmp, g_psoSpikeCmp, g_psoLookbackCmp, g_psoAliasCmp,
              PSO_GOLDEN_CMP, PSO_SERIES_CMP, PSO_COMPOSITE_CMP,
              PSO_FLAT_CMP, PSO_SPIKE_CMP, PSO_LOOKBACK_CMP,
              PSO_ALIAS_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* (1) GOLDEN: the formula, from 60 digits over the committed corpus. */
static ErrorNumber test_pso_golden( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   static TA_Real out[PSO_NB_BAR];
   int k, lastN = -1, lastM = -1;

   for( k = 0; k < PSO_NB_GOLDEN; k++ )
   {
      const PsoGolden *g = &psoGolden[k];
      double got, err;

      rc = TA_PSO( 0, (int)history->nbBars - 1,
                   history->high, history->low, history->close,
                   g->n, g->m, &begIdx, &nbElement, out );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_PSO golden rc=%d (n=%d, m=%d)\n",
                 (int)rc, g->n, g->m );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != (g->n - 1) + 2*(g->m - 1) || g->bar < begIdx )
      {
         printf( "Fail: TA_PSO golden range: begIdx=%d (want %d), bar %d\n",
                 (int)begIdx, (g->n - 1) + 2*(g->m - 1), g->bar );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      got = out[g->bar - begIdx];
      err = fabs( got - g->value );
      if( !( err <= PSO_GOLDEN_TOL ) )
      {
         printf( "Fail: TA_PSO golden bar %d (n=%d, m=%d): %.17g, expected "
                 "%.17g (abs %.3g, tol %.1e)\n",
                 g->bar, g->n, g->m, got, g->value, err, PSO_GOLDEN_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_psoGoldenCmp++;

      /* One language-server verdict per parameter pair. */
      if( (g->n != lastN || g->m != lastM) && server_verify_active() )
      {
         const double opt[2] = { (double)g->n, (double)g->m };
         ErrorNumber e;

         e = server_verify( "PSO", 0, (int)history->nbBars - 1,
                            (int)history->nbBars,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ history->high, history->low,
                                                history->close, NULL },
                            opt, 2,
                            (const TA_Real*[]){ out, NULL }, NULL );
         if( e != TA_TEST_PASS ) return e;
         lastN = g->n;
         lastM = g->m;
      }
   }

   return TA_TEST_PASS;
}

/* The #473 series, rebuilt here. Every price is a plain double expression over
 * sin and cos, so this is the only part of this file that depends on the
 * platform's libm. */
static void psoBuildSeries( double *h, double *l, double *c )
{
   int i;

   for( i = 0; i < PSO_NB_SERIES; i++ )
   {
      c[i] = 100.0 + 10.0*sin(i/7.0) + 0.15*i + 2.0*sin(i/2.3);
      h[i] = c[i] + 1.0 + 0.5*fabs(sin(i/3.0));
      l[i] = c[i] - 1.0 - 0.5*fabs(cos(i/5.0));
   }
}

/* (2) SERIES: the goldens published in #473, measured on another host. */
static ErrorNumber test_pso_series( void )
{
   static double h[PSO_NB_SERIES], l[PSO_NB_SERIES], c[PSO_NB_SERIES];
   static TA_Real out[PSO_NB_SERIES];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k;

   psoBuildSeries( h, l, c );

   for( k = 0; k < PSO_NB_SERIES_GOLDEN; k++ )
   {
      const PsoGolden *g = &psoSeriesGolden[k];
      double got, err;

      rc = TA_PSO( 0, PSO_NB_SERIES - 1, h, l, c, g->n, g->m,
                   &begIdx, &nbElement, out );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_PSO series rc=%d (n=%d, m=%d)\n",
                 (int)rc, g->n, g->m );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != (g->n - 1) + 2*(g->m - 1) || g->bar < begIdx )
      {
         printf( "Fail: TA_PSO series range: begIdx=%d (want %d), bar %d\n",
                 (int)begIdx, (g->n - 1) + 2*(g->m - 1), g->bar );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      got = out[g->bar - begIdx];
      err = fabs( got - g->value );
      if( !( err <= PSO_SERIES_TOL ) )
      {
         printf( "Fail: TA_PSO #473 series bar %d (n=%d, m=%d): %.17g, "
                 "expected %.17g (abs %.3g, tol %.1e)\n",
                 g->bar, g->n, g->m, got, g->value, err, PSO_SERIES_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_psoSeriesCmp++;
   }

   return TA_TEST_PASS;
}

/* (3) COMPOSITE, bit-exact: TA_STOCHF then TA_EMA then TA_EMA, entered
 * 2*TA_EMA_Lookback(m) bars early.
 *
 * On this corpus no Fast-K window is flat, so the K = 50 substitution of Q4
 * never fires here; test_pso_flat covers it with a window that is flat.
 */
static ErrorNumber test_pso_composite( const TA_History *history )
{
   static const int nSet[] = {  8, 14,  5, 21 };
   static const int mSet[] = {  5,  3,  1, 10 };
   static const int uSet[] = {  0,  1,  7 };
   static TA_Real fastK[PSO_NB_BAR], fastD[PSO_NB_BAR];
   static TA_Real nsk[PSO_NB_BAR], e1[PSO_NB_BAR], e2[PSO_NB_BAR];
   static TA_Real mine[PSO_NB_BAR], mine0[PSO_NB_BAR];
   TA_RetCode rc;
   TA_Integer bK, nK, b1, n1, b2, n2, bP, nP, bP0, nP0;
   int s, u, j, k, nbStart;
   int n, m, LE, LT, a, startIdx;
   int startSet[3];

   for( s = 0; s < 4; s++ )
   {
      n = nSet[s];
      m = mSet[s];

      for( u = 0; u < 3; u++ )
      {
         if( TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, uSet[u] ) != TA_SUCCESS )
         {
            printf( "Fail: TA_PSO composite could not set the EMA unstable period\n" );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         LE = TA_EMA_Lookback( m );
         LT = TA_PSO_Lookback( n, m );
         if( LT != (n - 1) + 2*LE )
         {
            printf( "Fail: TA_PSO_Lookback(%d,%d)=%d, expected %d\n",
                    n, m, LT, (n - 1) + 2*LE );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         startSet[0] = LT;
         startSet[1] = LT + 1;
         startSet[2] = 100;

         /* The startIdx = 0 call must reproduce the startIdx = LT call. */
         rc = TA_PSO( 0, PSO_NB_BAR - 1, history->high, history->low,
                      history->close, n, m, &bP0, &nP0, mine0 );
         if( rc != TA_SUCCESS ) goto badrc;

         for( nbStart = 0; nbStart < 3; nbStart++ )
         {
            startIdx = startSet[nbStart];
            a = startIdx - 2*LE;

            rc = TA_PSO( startIdx, PSO_NB_BAR - 1, history->high,
                         history->low, history->close, n, m, &bP, &nP, mine );
            if( rc != TA_SUCCESS ) goto badrc;
            if( bP != startIdx )
            {
               printf( "Fail: TA_PSO composite begIdx=%d, expected %d\n",
                       (int)bP, startIdx );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_PARAM;
            }

            rc = TA_STOCHF( a, PSO_NB_BAR - 1, history->high, history->low,
                            history->close, n, 1, TA_MAType_SMA,
                            &bK, &nK, fastK, fastD );
            if( rc != TA_SUCCESS ) goto badrc;
            if( bK != a )
            {
               printf( "Fail: TA_PSO composite STOCHF begIdx=%d, expected %d\n",
                       (int)bK, a );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_PARAM;
            }

            for( j = 0; j < (int)nK; j++ )
               nsk[j] = 0.1 * (fastK[j] - 50.0);

            rc = TA_EMA( 0, (int)nK - 1, nsk, m, &b1, &n1, e1 );
            if( rc != TA_SUCCESS ) goto badrc;
            if( b1 != LE )
            {
               printf( "Fail: TA_PSO composite EMA#1 begIdx=%d, expected %d\n",
                       (int)b1, LE );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_PARAM;
            }

            rc = TA_EMA( 0, (int)n1 - 1, e1, m, &b2, &n2, e2 );
            if( rc != TA_SUCCESS ) goto badrc;
            if( b2 != LE || (int)n2 != (int)nP )
            {
               printf( "Fail: TA_PSO composite EMA#2 begIdx=%d (want %d), "
                       "%d values (want %d)\n",
                       (int)b2, LE, (int)n2, (int)nP );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_PARAM;
            }

            for( k = 0; k < (int)nP; k++ )
            {
               double want = tanh( 0.5 * e2[k] );
               if( mine[k] != want )
               {
                  printf( "Fail: TA_PSO composite n=%d m=%d unst=%d start=%d "
                          "bar %d: %.17g, chain %.17g\n",
                          n, m, uSet[u], startIdx, startIdx + k,
                          mine[k], want );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               g_psoCompositeCmp++;
            }

            /* Only the startIdx = LT call lines up with the startIdx = 0 one. */
            if( startIdx == LT )
            {
               if( bP0 != LT || (int)nP0 != (int)nP )
               {
                  printf( "Fail: TA_PSO startIdx=0 answered begIdx=%d/%d values, "
                          "expected %d/%d\n",
                          (int)bP0, (int)nP0, LT, (int)nP );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_PARAM;
               }
               for( k = 0; k < (int)nP; k++ )
               {
                  if( mine0[k] != mine[k] )
                  {
                     printf( "Fail: TA_PSO startIdx=0 bar %d: %.17g, "
                             "startIdx=%d gives %.17g\n",
                             LT + k, mine0[k], LT, mine[k] );
                     TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                     return TA_TESTUTIL_TFRR_BAD_CALCULATION;
                  }
                  g_psoCompositeCmp++;
               }
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;

badrc:
   printf( "Fail: TA_PSO composite sub-call rc=%d\n", (int)rc );
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TESTUTIL_TFRR_BAD_RETCODE;
}

/* The flat-window runs. Built from the committed corpus rather than from a
 * closed form, so nothing here depends on the platform's libm.
 *
 * flat: bars 60 to 119 have high == low == close == 130.7.
 * mflat: the same, with bars 80 to 98 carrying a high and a close one ulp
 * above that low -- a window whose range is 2.8e-14, which an exact
 * `range == 0` test divides into [0,100] noise and TA_IS_ZERO_SCALED calls
 * flat. LEAN, which tests exactly, reads 1.97 away from TA-Lib there.
 */
#define PSO_FLAT_NB   200
#define PSO_FLAT_FROM  60
#define PSO_FLAT_TO   119

static void psoBuildFlat( const TA_History *history,
                          double *h, double *l, double *c, int machine )
{
   int i;
   double flat = 130.7;
   double up = nextafter( 130.7, 1e308 );

   for( i = 0; i < PSO_FLAT_NB; i++ )
   {
      h[i] = history->high[i];
      l[i] = history->low[i];
      c[i] = history->close[i];
   }
   for( i = PSO_FLAT_FROM; i <= PSO_FLAT_TO; i++ )
   {
      h[i] = flat;
      l[i] = flat;
      c[i] = flat;
   }
   if( machine )
   {
      for( i = 80; i <= 98; i++ )
      {
         h[i] = up;
         c[i] = up;
      }
   }
}

/* (4) FLAT: the Q4 ruling, and the machine-flat window. */
static ErrorNumber test_pso_flat( const TA_History *history )
{
   static double h[PSO_FLAT_NB], l[PSO_FLAT_NB], c[PSO_FLAT_NB];
   static double mh[PSO_FLAT_NB], ml[PSO_FLAT_NB], mc[PSO_FLAT_NB];
   static TA_Real out[PSO_FLAT_NB], mout[PSO_FLAT_NB];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, mBegIdx, mNbElement;
   int k, LT;

   psoBuildFlat( history, h, l, c, 0 );
   psoBuildFlat( history, mh, ml, mc, 1 );

   LT = TA_PSO_Lookback( 8, 5 );

   /* Deep inside the flat run every Fast-K window is flat, so every reading
    * the warm-up sees is the midpoint 50, every normalised value is exactly
    * zero, and both passes seed and run on zeros: the output is EXACTLY zero,
    * not nearly zero. Under the Fast-K convention of 0 it would be
    * -tanh(2.5) = -0.9866 instead, a near-extreme oversold reading. A
    * tolerance here would accept both.
    */
   rc = TA_PSO( PSO_FLAT_FROM + LT, PSO_FLAT_TO, h, l, c, 8, 5,
                &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_PSO flat run rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begIdx != PSO_FLAT_FROM + LT
    || (int)nbElement != PSO_FLAT_TO - (PSO_FLAT_FROM + LT) + 1 )
   {
      printf( "Fail: TA_PSO flat run answered begIdx=%d / %d values\n",
              (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   for( k = 0; k < (int)nbElement; k++ )
   {
      if( out[k] != 0.0 )
      {
         printf( "Fail: TA_PSO flat bar %d: %.17g, expected exactly 0\n",
                 (int)begIdx + k, out[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_psoFlatCmp++;
   }

   /* The machine-flat run must read the same as the flat one, bit for bit:
    * only TA-Lib's own guard arbitrates a window whose range is real but
    * sub-epsilon. */
   rc = TA_PSO( 0, PSO_FLAT_NB - 1, h, l, c, 8, 5,
                &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS ) goto badrc;
   rc = TA_PSO( 0, PSO_FLAT_NB - 1, mh, ml, mc, 8, 5,
                &mBegIdx, &mNbElement, mout );
   if( rc != TA_SUCCESS ) goto badrc;
   if( begIdx != mBegIdx || nbElement != mNbElement )
   {
      printf( "Fail: TA_PSO machine-flat run answered a different range\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   for( k = 0; k < (int)nbElement; k++ )
   {
      if( out[k] != mout[k] )
      {
         printf( "Fail: TA_PSO machine-flat bar %d: %.17g, flat run %.17g\n",
                 (int)begIdx + k, mout[k], out[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_psoFlatCmp++;
   }

   return TA_TEST_PASS;

badrc:
   printf( "Fail: TA_PSO flat run rc=%d\n", (int)rc );
   return TA_TESTUTIL_TFRR_BAD_RETCODE;
}

/* (5) SPIKE: a bar whose close lies far outside its own high/low range.
 *
 * TA-Lib does not validate OHLC, so SS is not bounded by [-5,5] and the
 * published (e^SS - 1)/(e^SS + 1) is inf/inf once SS passes 709.78 -- a NaN
 * out of a successful call, which #112 forbids. tanh(SS/2) saturates
 * at exactly 1.0 instead. This leg is why the tail is spelled tanh.
 */
static ErrorNumber test_pso_spike( const TA_History *history )
{
   static double h[PSO_FLAT_NB], l[PSO_FLAT_NB], c[PSO_FLAT_NB];
   static TA_Real out[PSO_FLAT_NB];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, sawSaturated = 0;

   for( k = 0; k < PSO_FLAT_NB; k++ )
   {
      h[k] = history->high[k];
      l[k] = history->low[k];
      c[k] = history->close[k];
   }
   c[150] = h[150] + 5000.0;

   rc = TA_PSO( 0, PSO_FLAT_NB - 1, h, l, c, 8, 5,
                &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_PSO spike rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( k = 0; k < (int)nbElement; k++ )
   {
      if( !isfinite( out[k] ) )
      {
         printf( "Fail: TA_PSO spike bar %d is %.17g\n",
                 (int)begIdx + k, out[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( out[k] == 1.0 )
         sawSaturated++;
      g_psoSpikeCmp++;
   }

   if( sawSaturated == 0 )
   {
      printf( "Fail: TA_PSO spike never saturated, so this leg proves "
              "nothing about the tail\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_psoSpikeCmp++;

   return TA_TEST_PASS;
}

/* (6) LOOKBACK: the published arithmetic, and what the call actually does. */
static ErrorNumber test_pso_lookback( const TA_History *history )
{
   static const int nSet[] = {  8, 14,  5, 21 };
   static const int mSet[] = {  5,  3,  1, 10 };
   static const int uSet[] = {  0,  1,  3,  7 };
   static TA_Real out[PSO_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int s, u, want, got;

   for( u = 0; u < 4; u++ )
   {
      if( TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, uSet[u] ) != TA_SUCCESS )
      {
         printf( "Fail: TA_PSO lookback could not set the EMA unstable period\n" );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( s = 0; s < 4; s++ )
      {
         want = (nSet[s] - 1) + 2*TA_EMA_Lookback( mSet[s] );
         got = TA_PSO_Lookback( nSet[s], mSet[s] );
         if( got != want )
         {
            printf( "Fail: TA_PSO_Lookback(%d,%d) = %d at unstable %d, "
                    "expected %d\n",
                    nSet[s], mSet[s], got, uSet[u], want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_psoLookbackCmp++;

         rc = TA_PSO( 0, (int)history->nbBars - 1, history->high,
                      history->low, history->close, nSet[s], mSet[s],
                      &begIdx, &nbElement, out );
         if( rc != TA_SUCCESS || begIdx != want )
         {
            printf( "Fail: TA_PSO(%d,%d) at unstable %d answered rc=%d "
                    "begIdx=%d, expected %d\n",
                    nSet[s], mSet[s], uSet[u], (int)rc, (int)begIdx, want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
         g_psoLookbackCmp++;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;
}

/* (7) outReal may be any of the three inputs. */
static ErrorNumber test_pso_aliasing( const TA_History *history )
{
   const TA_Real *src[3];
   static const char * const name[3] = { "inHigh", "inLow", "inClose" };
   static TA_Real ref[PSO_NB_BAR], work[PSO_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int which, i, nb;

   nb = (int)history->nbBars;

   rc = TA_PSO( 0, nb-1, history->high, history->low, history->close,
                8, 5, &begIdx, &nbElement, ref );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_PSO aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   src[0] = history->high;
   src[1] = history->low;
   src[2] = history->close;

   for( which = 0; which < 3; which++ )
   {
      for( i = 0; i < nb; i++ )
         work[i] = src[which][i];

      rc = TA_PSO( 0, nb-1,
                   which == 0 ? work : history->high,
                   which == 1 ? work : history->low,
                   which == 2 ? work : history->close,
                   8, 5, &begIdx2, &nbElement2, work );
      if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "Fail: TA_PSO aliased onto %s answered rc=%d\n",
                 name[which], (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < (int)nbElement; i++ )
      {
         if( work[i] != ref[i] )
         {
            printf( "Fail: TA_PSO aliased onto %s differs at bar %d: "
                    "%.17g vs %.17g\n",
                    name[which], (int)begIdx + i, work[i], ref[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      g_psoAliasCmp++;
   }

   return TA_TEST_PASS;
}

static TA_RetCode psoRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_PSO_Lookback( 8, 5 );
   return TA_PSO( startIdx, endIdx, h->high, h->low, h->close,
                  8, 5, outBegIdx, outNbElement, outputBuffer );
}

/* (8) Range sweep. PSO is recursive through both EMA passes, so the value
 * depends on how far back the recursion started and the EMA unstable period
 * is what bounds the residual. */
static ErrorNumber test_pso_range( const TA_History *history )
{
   return doRangeTestEx( psoRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_EMA,
                         (void *)history, 1, 0 );
}
