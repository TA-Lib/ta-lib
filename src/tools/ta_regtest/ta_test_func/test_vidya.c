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
 *  092926 MF,CC  First version (issue #474).
 */

/* Description:
 *
 *   Test TA_VIDYA (Variable Index Dynamic Average).
 *
 *   Legs:
 *     1. Frozen goldens on the issue's 1200-bar series: a 60-digit reference
 *        on the exact binary inputs.
 *     2. Composite, bit-exact: the warm-up re-derived from the window's first
 *        price with the CMO of the changes seen so far, then the recursion over
 *        TA_CMOU's own output, entered unst bars before startIdx, over periods,
 *        unstable periods and start indexes.
 *     3. Lookback and the unstable-period id.
 *     4. A flat window holds the line, bit-constant, with no NaN.
 *     5. A strictly rising ramp is a plain alpha-EMA from the window's first
 *        price; period 1 is the input.
 *     6. TA_MAType_VIDYA through MA, MAVP and BBANDS.
 *     7. The startIdx/endIdx range sweep.
 *     8. External oracles, captured by ta-lib-oracles capture_474_vidya.py on
 *        the same series (all three libraries take one period, so n = m):
 *        trading-signals 8.3.0 VIDYA (seeded on x[0]) and pandas-ta-classic
 *        0.6.52 vidya (seeded on an SMA) in the tail, from bar 800, where the
 *        seed has decayed; LEAN 2.5.18090 (Wilder CMO, seeded on x[P-1]) for
 *        its first bar only; and ta4j 0.22.6 CMOIndicator, the CMO VIDYA is
 *        driven by.
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
#define VIDYA_NB        1200
#define VIDYA_GOLD_REL  1e-14

#define VIDYA_COMPOSITE_CMP 357183
#define VIDYA_FLAT_CMP      1921
#define VIDYA_RAMP_CMP      3140
#define VIDYA_MATYPE_CMP    31751

/* The tail comparison allows the unfused k*x + (1-k)*prev step the two
 * libraries use; measured 6.5e-16. */
#define VIDYA_TAIL_REL  1e-13
#define VIDYA_CMO_ABS   1e-12

typedef struct { int n, m, bar; double want; } VidyaGolden;

/* A 60-digit reference on the exact binary inputs. Bars 9 to 100 carry the
 * seed: seeding on x[m-1] moves them by 2.9e-2 to 1.8e-7, an SMA seed by
 * 4.2e-3 to 2.6e-8, and a plain alpha-EMA warm-up by 2.5e-5 to 1.5e-10. */
static const VidyaGolden vidyaGolden[] =
{
   { 12, 9,    9, 106.58376159480673 },
   { 12, 9,   10, 107.02109985002824 },
   { 12, 9,   11, 107.40628451938244 },
   { 12, 9,   20, 109.16885340451152 },
   { 12, 9,   50, 109.12175801464875 },
   { 12, 9,  100, 120.18602253125532 },
   { 12, 9,  251, 132.9441644591319  },
   { 12, 9,  500, 181.7499950535472  },
   { 12, 9,  800, 223.84149738801602 },
   { 12, 9, 1000, 243.8869455368122  },
   { 12, 9, 1199, 284.6901162441343  },
   {  9, 9,    9, 107.52016675465694 },
   {  9, 9,  800, 225.63456692955305 },
   {  9, 9, 1000, 242.94580784253432 },
   {  9, 9, 1199, 286.0357344900665  },
   { 14, 14,  800, 222.6660965877049 },
   { 14, 14, 1000, 244.69966753954046 },
   { 14, 14, 1199, 283.522049111793  },
};
#define NB_VIDYA_GOLDEN ((int)(sizeof(vidyaGolden)/sizeof(vidyaGolden[0])))

/* trading-signals and pandas-ta-classic print the same doubles here. */
static const VidyaGolden vidyaTail[] =
{
   {  9,  9,  800, 225.6345669295531  },
   {  9,  9, 1000, 242.94580784253432 },
   {  9,  9, 1199, 286.03573449006655 },
   { 14, 14,  800, 222.66609658770494 },
   { 14, 14, 1000, 244.69966753954046 },
   { 14, 14, 1199, 283.52204911179308 },
};

/* LEAN's first bar. Its seed is x[P-1], so the value is not ours. */
static const int vidyaLeanP[] = { 2, 9, 12, 14, 30 };

/* ta4j CMOIndicator(close, P): TA_CMOU on the same series. */
static const VidyaGolden vidyaTa4jCmo[] =
{
   {  2, 0,    2, 100.0 },
   {  2, 0,   20, -100.0 },
   {  9, 0,    9, 97.776888526814616 },
   {  9, 0,   20, -44.9026232221835 },
   {  9, 0,  500, -7.5821339027343182 },
   {  9, 0, 1000, -57.719235586560643 },
   { 14, 0,   14, 97.647136663678197 },
   { 14, 0,   20, -38.67367149684469 },
   { 14, 0,  500, 39.826960469609844 },
   { 14, 0, 1000, -82.115896622435429 },
};

static const int vidyaGridN[]     = { 2, 3, 12, 30, 200 };
static const int vidyaGridM[]     = { 2, 3, 9, 14, 200 };
static const int vidyaGridUnst[]  = { 0, 7 };
static const int vidyaGridStart[] = { 0, 1, 9, 17, 214, 250, 777, 1199 };
#define NB_OF(a) ((int)(sizeof(a)/sizeof((a)[0])))

static int g_vidyaGoldCmp;
static int g_vidyaCompositeCmp;
static int g_vidyaFlatCmp;
static int g_vidyaRampCmp;
static int g_vidyaMaTypeCmp;
static int g_vidyaOracleCmp;

static void        vidyaSynth( double *c );
static ErrorNumber vidyaCall( const char *tag, const double *x, int nb, int n, int m,
                              double *out, int *begIdx, int *nbElement );
static ErrorNumber test_vidya_goldens( void );
static ErrorNumber test_vidya_composite( void );
static ErrorNumber test_vidya_lookback( void );
static ErrorNumber test_vidya_flat( void );
static ErrorNumber test_vidya_ramp( void );
static ErrorNumber test_vidya_matype( void );
static ErrorNumber test_vidya_range( const TA_Real *in );
static ErrorNumber test_vidya_oracles( void );

/**** Global functions definitions. ****/
ErrorNumber test_func_vidya( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   g_vidyaGoldCmp = g_vidyaCompositeCmp = g_vidyaFlatCmp = 0;
   g_vidyaRampCmp = g_vidyaMaTypeCmp = g_vidyaOracleCmp = 0;

   err = test_vidya_goldens();
   if( err == TA_TEST_PASS )
      err = test_vidya_oracles();
   if( err == TA_TEST_PASS )
      err = test_vidya_composite();
   if( err == TA_TEST_PASS )
      err = test_vidya_lookback();
   if( err == TA_TEST_PASS )
      err = test_vidya_flat();
   if( err == TA_TEST_PASS )
      err = test_vidya_ramp();
   if( err == TA_TEST_PASS )
      err = test_vidya_matype();

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS )
      err = test_vidya_range( history->close );

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* Literal counts: every input is synthesized, so each leg is deterministic. */
   if( err == TA_TEST_PASS
       && ( g_vidyaCompositeCmp != VIDYA_COMPOSITE_CMP || g_vidyaFlatCmp != VIDYA_FLAT_CMP
            || g_vidyaRampCmp != VIDYA_RAMP_CMP || g_vidyaMaTypeCmp != VIDYA_MATYPE_CMP ) )
   {
      printf( "VIDYA Fail: coverage counters (composite %d, flat %d, ramp %d, matype %d) "
              "are not what this file was written with (%d, %d, %d, %d)\n",
              g_vidyaCompositeCmp, g_vidyaFlatCmp, g_vidyaRampCmp, g_vidyaMaTypeCmp,
              VIDYA_COMPOSITE_CMP, VIDYA_FLAT_CMP, VIDYA_RAMP_CMP, VIDYA_MATYPE_CMP );
      return TA_VIDYA_VACUOUS;
   }

   return err;
}

/**** Local functions definitions. ****/
static void vidyaSynth( double *c )
{
   int i;

   for( i = 0; i < VIDYA_NB; i++ )
      c[i] = 100.0 + 10.0*sin(i/7.0) + 0.15*i + 2.0*sin(i/2.3);
}

/* Whole-series call at unstable period 0, checked for its range and routed to
 * the language servers. */
static ErrorNumber vidyaCall( const char *tag, const double *x, int nb, int n, int m,
                              double *out, int *begIdx, int *nbElement )
{
   TA_RetCode retCode;
   double optIn[2];
   ErrorNumber e;
   int cmpBefore;

   retCode = TA_VIDYA( 0, nb-1, x, n, m, begIdx, nbElement, out );
   if( retCode != TA_SUCCESS || *begIdx != m || *nbElement != nb-m )
   {
      printf( "VIDYA %s Fail [%d,%d]: rc=%d (%d,%d) expected (%d,%d)\n", tag, n, m,
              (int)retCode, *begIdx, *nbElement, m, nb-m );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   if( !server_verify_active() )
      return TA_TEST_PASS;

   optIn[0] = (double)n;
   optIn[1] = (double)m;
   cmpBefore = server_verify_comparisons();
   e = server_verify( "VIDYA", 0, nb-1, nb, retCode, *begIdx, *nbElement,
                      (const TA_Real*[]){ x, NULL }, optIn, 2,
                      (const TA_Real*[]){ out, NULL }, NULL );
   if( e != TA_TEST_PASS )
      return e;
   if( server_verify_comparisons() == cmpBefore )
   {
      printf( "VIDYA %s [%d,%d]: compared no server despite live pipes\n", tag, n, m );
      return TA_SV_ROUTED_VACUOUS;
   }
   return TA_TEST_PASS;
}

/* (1) */
static ErrorNumber test_vidya_goldens( void )
{
   static double c[VIDYA_NB], out[VIDYA_NB];
   TA_Integer begIdx = 0, nbElement = 0;
   ErrorNumber e;
   int k, n = -1, m = -1;
   double err;
   const char *mode;

   vidyaSynth( c );

   for( k = 0; k < NB_VIDYA_GOLDEN; k++ )
   {
      const VidyaGolden *g = &vidyaGolden[k];
      int idx;

      if( g->n != n || g->m != m )
      {
         n = g->n;
         m = g->m;
         e = vidyaCall( "golden", c, VIDYA_NB, n, m, out, &begIdx, &nbElement );
         if( e != TA_TEST_PASS )
            return e;
      }

      idx = g->bar - begIdx;
      if( idx < 0 || idx >= nbElement )
      {
         printf( "VIDYA golden Fail [%d,%d]: bar %d outside the output\n", n, m, g->bar );
         return TA_VIDYA_VACUOUS;
      }
      if( !checkOracleValue( out[idx], g->want, VIDYA_GOLD_REL, 0.0, &err, &mode ) )
      {
         printf( "VIDYA golden Fail [%d,%d] at bar %d: got %.17g expected %.17g "
                 "(%s %.3e)\n", n, m, g->bar, out[idx], g->want, mode, err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_vidyaGoldCmp++;
   }

   return g_vidyaGoldCmp == NB_VIDYA_GOLDEN ? TA_TEST_PASS : TA_VIDYA_VACUOUS;
}

static ErrorNumber vidyaCheckOne( const char *tag, int n, int m, int bar, double got,
                                  double want, double rel, double abs )
{
   double err;
   const char *mode;

   if( !checkOracleValue( got, want, rel, abs, &err, &mode ) )
   {
      printf( "VIDYA %s Fail [%d,%d] at bar %d: got %.17g expected %.17g (%s %.3e)\n",
              tag, n, m, bar, got, want, mode, err );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_vidyaOracleCmp++;
   return TA_TEST_PASS;
}

/* (8) */
static ErrorNumber test_vidya_oracles( void )
{
   static double c[VIDYA_NB], out[VIDYA_NB];
   TA_Integer begIdx = 0, nbElement = 0;
   ErrorNumber e;
   int k;

   vidyaSynth( c );

   for( k = 0; k < NB_OF(vidyaTail); k++ )
   {
      const VidyaGolden *g = &vidyaTail[k];
      e = vidyaCall( "tail", c, VIDYA_NB, g->n, g->m, out, &begIdx, &nbElement );
      if( e == TA_TEST_PASS )
         e = vidyaCheckOne( "trading-signals/pandas tail", g->n, g->m, g->bar,
                            out[g->bar - begIdx], g->want, VIDYA_TAIL_REL, 0.0 );
      if( e != TA_TEST_PASS )
         return e;
   }

   for( k = 0; k < NB_OF(vidyaLeanP); k++ )
   {
      int p = vidyaLeanP[k];
      if( TA_VIDYA_Lookback( p, p ) != p )
      {
         printf( "VIDYA LEAN Fail [%d,%d]: lookback %d, LEAN's %d\n", p, p,
                 TA_VIDYA_Lookback( p, p ), p );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      g_vidyaOracleCmp++;
   }

   for( k = 0; k < NB_OF(vidyaTa4jCmo); k++ )
   {
      const VidyaGolden *g = &vidyaTa4jCmo[k];
      if( TA_CMOU( 0, VIDYA_NB-1, c, g->n, &begIdx, &nbElement, out ) != TA_SUCCESS
          || begIdx != g->n )
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      e = vidyaCheckOne( "ta4j CMO", g->n, g->n, g->bar, out[g->bar - begIdx], g->want,
                         0.0, VIDYA_CMO_ABS );
      if( e != TA_TEST_PASS )
         return e;
   }

   return g_vidyaOracleCmp == NB_OF(vidyaTail) + NB_OF(vidyaLeanP) + NB_OF(vidyaTa4jCmo)
          ? TA_TEST_PASS : TA_VIDYA_VACUOUS;
}

/* (2) From the first full window on, VIDYA is the composite of TA_CMOU, bit
 * for bit; before it, the warm-up runs from the window's first price with the
 * CMO of the changes seen so far. The step is fused, as TA_EMA's is. The
 * callee is entered unst bars before the first emitted bar, so the reference
 * is compared from ref + unst: dropping that offset, or seeding a bar off,
 * reads red. */
static ErrorNumber vidyaComposite( const char *tag, const double *c, int nb,
                                   int n, int m, int unst, int startIdx )
{
   static double out[VIDYA_NB], cm[VIDYA_NB], ref[VIDYA_NB];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, cmBeg, cmNb;
   int first = startIdx < m + unst ? m + unst : startIdx;
   int a = first - unst;
   int j;
   double v, su = 0.0, sd = 0.0, d, t;

   rc = TA_VIDYA( startIdx, nb-1, c, n, m, &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS || begIdx != first || nbElement != nb - first )
   {
      printf( "VIDYA %s Fail [%d,%d unst %d start %d]: rc=%d (%d,%d) expected (%d,%d)\n",
              tag, n, m, unst, startIdx, (int)rc, begIdx, nbElement, first, nb - first );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   rc = TA_CMOU( a, nb-1, c, m, &cmBeg, &cmNb, cm );
   if( rc != TA_SUCCESS || cmBeg != a || cmNb != nb - a )
   {
      printf( "VIDYA %s Fail: TA_CMOU(%d) answered (%d,%d)\n", tag, a, cmBeg, cmNb );
      return TA_VIDYA_VACUOUS;
   }

   v = c[a-m];
   for( j = 1; j < m; j++ )
   {
      d = c[a-m+j] - c[a-m+j-1];
      if( d > 0.0 )
         su += d;
      else if( d < 0.0 )
         sd -= d;
      t = su + sd;
      v = fma( c[a-m+j] - v,
               t > 0.0 ? (2.0/((double)(n+1))) * (fabs((100.0*(su-sd))/t) / 100.0) : 0.0,
               v );
   }
   for( j = 0; j < cmNb; j++ )
   {
      double k = (2.0/((double)(n+1))) * (fabs(cm[j]) / 100.0);
      v = fma( c[a+j] - v, k, v );
      ref[j] = v;
   }

   if( memcmp( out, ref + unst, (size_t)nbElement * sizeof(double) ) != 0 )
   {
      for( j = 0; j < nbElement; j++ )
         if( out[j] != ref[j+unst] )
            break;
      printf( "VIDYA %s Fail [%d,%d unst %d start %d] at bar %d: %.17g != %.17g "
              "(must be BIT-exact)\n", tag, n, m, unst, startIdx, first + j, out[j],
              ref[j+unst] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_vidyaCompositeCmp += nbElement;
   return TA_TEST_PASS;
}

static ErrorNumber test_vidya_composite( void )
{
   static double c[VIDYA_NB];
   ErrorNumber e;
   int in, im, iu, is;

   vidyaSynth( c );

   for( iu = 0; iu < NB_OF(vidyaGridUnst); iu++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, (unsigned int)vidyaGridUnst[iu] );
      for( in = 0; in < NB_OF(vidyaGridN); in++ )
      for( im = 0; im < NB_OF(vidyaGridM); im++ )
      for( is = 0; is < NB_OF(vidyaGridStart); is++ )
      {
         e = vidyaComposite( "composite", c, VIDYA_NB, vidyaGridN[in], vidyaGridM[im],
                             vidyaGridUnst[iu], vidyaGridStart[is] );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, 0 );
   return TA_TEST_PASS;
}

/* (3) The lookback is the CMO period plus VIDYA's own unstable period: the
 * EMA length does not enter it, and no other id moves it. */
static ErrorNumber test_vidya_lookback( void )
{
   static const unsigned int unsts[] = { 0, 1, 7, 50 };
   static const TA_FuncUnstId others[] = { TA_FUNC_UNST_EMA, TA_FUNC_UNST_CMO,
                                           TA_FUNC_UNST_KAMA };
   static double c[VIDYA_NB], out[VIDYA_NB];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, j;

   vidyaSynth( c );

   for( k = 0; k < NB_OF(unsts); k++ )
   {
      int u = (int)unsts[k];
      TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, unsts[k] );
      if( TA_VIDYA_Lookback( 12, 9 ) != 9 + u || TA_VIDYA_Lookback( 2, 9 ) != 9 + u
          || TA_VIDYA_Lookback( 200, 9 ) != 9 + u || TA_VIDYA_Lookback( 12, 2 ) != 2 + u )
      {
         printf( "VIDYA lookback Fail [unst %d]: %d\n", u, TA_VIDYA_Lookback( 12, 9 ) );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      rc = TA_VIDYA( 9 + u, VIDYA_NB-1, c, 12, 9, &begIdx, &nbElement, out );
      if( rc != TA_SUCCESS || begIdx != 9 + u )
      {
         printf( "VIDYA lookback Fail [unst %d]: outBegIdx %d\n", u, begIdx );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      rc = TA_VIDYA( 0, 8 + u, c, 12, 9, &begIdx, &nbElement, out );
      if( rc != TA_SUCCESS || nbElement != 0 )
      {
         printf( "VIDYA lookback Fail [unst %d]: output before the lookback\n", u );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }
   TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, 0 );

   for( j = 0; j < NB_OF(others); j++ )
   {
      TA_SetUnstablePeriod( others[j], 17 );
      if( TA_VIDYA_Lookback( 12, 9 ) != 9 )
      {
         printf( "VIDYA lookback Fail: unstable id %d moves VIDYA's lookback\n",
                 (int)others[j] );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      TA_SetUnstablePeriod( others[j], 0 );
   }

   if( TA_VIDYA_Lookback( 0, 9 ) != -1 || TA_VIDYA_Lookback( 12, 1 ) != -1
       || TA_VIDYA_Lookback( 100001, 9 ) != -1 || TA_VIDYA_Lookback( 12, 100001 ) != -1 )
   {
      printf( "VIDYA lookback Fail: an out-of-range period is not rejected\n" );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   return TA_TEST_PASS;
}

/* (4) Once the CMO window is flat the CMO is exactly 0, so k is 0 and the line
 * holds. startIdx is swept across the prefix/flat boundary because the seed
 * moves with it. The second prefix mixes magnitudes so that the running sums
 * keep a rounding residue when the window empties: only the exact reset of a
 * flat window holds the line there, and the composite pins that reset. */
static ErrorNumber test_vidya_flat( void )
{
   static double x[70], out[70];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i, startIdx, pre, u;
   ErrorNumber e;

   for( pre = 0; pre < 2; pre++ )
   {
      for( i = 0; i < 70; i++ )
      {
         if( i >= 30 )
            x[i] = 130.7;
         else if( pre == 0 )
            x[i] = 100.0 + sin(i/3.0);
         else
            x[i] = (i%3 == 0 ? 5000.0 : (i%3 == 1 ? 0.37 : 100.0)) + i/7.0;
      }

      e = vidyaCall( "flat", x, 70, 9, 9, out, &begIdx, &nbElement );
      if( e != TA_TEST_PASS )
         return e;

      /* An unstable period of 12 puts the first flat window inside the skipped
       * bars, where the reset has its own copy. */
      for( u = 0; u <= 12; u += 6 )
      {
         TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, (unsigned int)u );
         for( startIdx = 9; startIdx <= 45; startIdx++ )
         {
            e = vidyaComposite( "flat composite", x, 70, 12, 9, u, startIdx );
            if( e != TA_TEST_PASS )
               return e;
         }
      }
      TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, 0 );

      for( startIdx = 9; startIdx <= 45; startIdx++ )
      {
         double held;

         rc = TA_VIDYA( startIdx, 69, x, 9, 9, &begIdx, &nbElement, out );
         if( rc != TA_SUCCESS || begIdx != startIdx )
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         for( i = 0; i < nbElement; i++ )
            if( !isfinite( out[i] ) )
            {
               printf( "VIDYA flat Fail [start %d]: non-finite at bar %d\n", startIdx, begIdx+i );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         /* Bars 30..38 are the last to carry a change; bar 39 is the first flat
          * window, so from bar 38 on the line no longer moves. */
         if( startIdx > 38 )
            continue;
         held = out[38 - begIdx];
         for( i = 39 - begIdx; i < nbElement; i++ )
         {
            if( out[i] != held )
            {
               printf( "VIDYA flat Fail [start %d] at bar %d: %.17g != %.17g\n",
                       startIdx, begIdx+i, out[i], held );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_vidyaFlatCmp++;
         }
      }
   }

   /* Wholly flat: the output is the price. */
   for( i = 0; i < 70; i++ )
      x[i] = 42.17;
   e = vidyaCall( "constant", x, 70, 12, 9, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   for( i = 0; i < nbElement; i++ )
   {
      if( out[i] != 42.17 )
      {
         printf( "VIDYA constant Fail at bar %d: %.17g\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_vidyaFlatCmp++;
   }

   return TA_TEST_PASS;
}

/* (5) On a strictly rising series |CMO| is 100 over every window, the partial
 * ones included, so k is alpha and VIDYA is a plain alpha-EMA seeded on x[0]
 * (not TA_EMA, which seeds on an SMA). The ramp's steps are integers so that
 * 100*Su/Su is exactly 100. Period 1 is the identity copy. */
static ErrorNumber test_vidya_ramp( void )
{
   static const int ns[] = { 2, 12, 30 }, ms[] = { 2, 9, 40 };
   static double x[300], out[300];
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   int i, a, b;

   for( i = 0; i < 300; i++ )
      x[i] = 1000.0 + (double)(3*i + (i*i) % 3);

   for( a = 0; a < NB_OF(ns); a++ )
   for( b = 0; b < NB_OF(ms); b++ )
   {
      double alpha = 2.0 / ((double)(ns[a] + 1)), v;

      e = vidyaCall( "ramp", x, 300, ns[a], ms[b], out, &begIdx, &nbElement );
      if( e != TA_TEST_PASS )
         return e;
      v = x[0];
      for( i = 1; i < ms[b]; i++ )
         v = fma( x[i] - v, alpha, v );
      for( i = 0; i < nbElement; i++ )
      {
         v = fma( x[begIdx + i] - v, alpha, v );
         if( out[i] != v )
         {
            printf( "VIDYA ramp Fail [%d,%d] at bar %d: %.17g != %.17g\n",
                    ns[a], ms[b], begIdx+i, out[i], v );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_vidyaRampCmp++;
      }
   }

   for( b = 0; b < 2; b++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, b ? 7u : 0u );
      if( TA_VIDYA_Lookback( 1, 9 ) != (b ? 7 : 0)
          || TA_VIDYA( 0, 299, x, 1, 9, &begIdx, &nbElement, out ) != TA_SUCCESS
          || begIdx != (b ? 7 : 0) || nbElement != 300 - begIdx
          || memcmp( out, x + begIdx, (size_t)nbElement * sizeof(double) ) != 0 )
      {
         printf( "VIDYA period 1 Fail [unst %d]: not the input copy\n", b ? 7 : 0 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_vidyaRampCmp += nbElement;
   }
   TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, 0 );
   return TA_TEST_PASS;
}

/* (6) TA_MAType_VIDYA: MA's one period is the EMA length n, and the CMO period
 * is (3n+2)/4. The lookback is compared on its own: ma() forwards to vidya(),
 * which clamps startIdx to its own lookback, so a wrong ma_lookback arm leaves
 * every value right and only the caller's buffer sizing wrong. */
static ErrorNumber test_vidya_matype( void )
{
   static const struct { int n, m; } ratio[] = {
      { 2, 2 }, { 5, 4 }, { 9, 7 }, { 10, 8 }, { 12, 9 }, { 14, 11 }, { 20, 15 },
      { 26, 20 }, { 30, 23 }, { 50, 38 }, { 200, 150 }
   };
   static const unsigned int unsts[] = { 0, 5 };
   static double c[VIDYA_NB], outMA[VIDYA_NB], outV[VIDYA_NB], per[VIDYA_NB];
   static double up[VIDYA_NB], mid[VIDYA_NB], lo[VIDYA_NB];
   static double alt[2][VIDYA_NB];
   TA_RetCode rcM, rcV;
   TA_Integer begM, nbM, begV, nbV, begA[2], nbA[2];
   int k, u, i;

   vidyaSynth( c );

   for( u = 0; u < NB_OF(unsts); u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, unsts[u] );

      for( k = 0; k < NB_OF(ratio); k++ )
      {
         int n = ratio[k].n, m = ratio[k].m;

         if( TA_MA_Lookback( n, TA_MAType_VIDYA ) != TA_VIDYA_Lookback( n, m )
             || TA_MA_Lookback( n, TA_MAType_VIDYA ) != m + (int)unsts[u] )
         {
            printf( "VIDYA matype Fail [n %d unst %u]: MA_Lookback %d, VIDYA_Lookback(%d,%d) %d\n",
                    n, unsts[u], TA_MA_Lookback( n, TA_MAType_VIDYA ), n, m,
                    TA_VIDYA_Lookback( n, m ) );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         g_vidyaMaTypeCmp++;

         rcM = TA_MA( 0, VIDYA_NB-1, c, n, TA_MAType_VIDYA, &begM, &nbM, outMA );
         rcV = TA_VIDYA( 0, VIDYA_NB-1, c, n, m, &begV, &nbV, outV );
         if( rcM != TA_SUCCESS || rcV != TA_SUCCESS || begM != begV || nbM != nbV
             || memcmp( outMA, outV, (size_t)nbM * sizeof(double) ) != 0 )
         {
            printf( "VIDYA matype Fail [n %d unst %u]: MA rc=%d (%d,%d) != VIDYA rc=%d (%d,%d) "
                    "or values differ\n", n, unsts[u], (int)rcM, begM, nbM, (int)rcV, begV, nbV );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_vidyaMaTypeCmp += nbM;
      }

      /* MA's period-1 copy answers before the dispatch, so unlike TA_VIDYA(1, m)
       * it does not wait out VIDYA's unstable period. */
      rcM = TA_MA( 0, VIDYA_NB-1, c, 1, TA_MAType_VIDYA, &begM, &nbM, outMA );
      if( rcM != TA_SUCCESS || begM != 0 || nbM != VIDYA_NB
          || memcmp( outMA, c, sizeof(c) ) != 0 )
      {
         printf( "VIDYA matype Fail: MA(1, VIDYA) is not the identity copy\n" );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_vidyaMaTypeCmp += nbM;

      /* MAVP picks, bar by bar, the MA of that bar's period, each anchored at
       * MAVP's own first bar. */
      for( i = 0; i < VIDYA_NB; i++ )
         per[i] = (i % 3) ? 5.0 : 12.0;
      rcM = TA_MAVP( 0, VIDYA_NB-1, c, per, 5, 12, TA_MAType_VIDYA, &begM, &nbM, outMA );
      rcV  = TA_MA( begM, VIDYA_NB-1, c, 5, TA_MAType_VIDYA, &begA[0], &nbA[0], alt[0] );
      rcV |= TA_MA( begM, VIDYA_NB-1, c, 12, TA_MAType_VIDYA, &begA[1], &nbA[1], alt[1] );
      if( rcM != TA_SUCCESS || rcV != TA_SUCCESS
          || begM != TA_MA_Lookback( 12, TA_MAType_VIDYA ) || nbM != VIDYA_NB - begM )
      {
         printf( "VIDYA matype Fail: MAVP rc=%d (%d,%d)\n", (int)rcM, begM, nbM );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbM; i++ )
      {
         int bar = begM + i, w = (bar % 3) ? 0 : 1;
         if( outMA[i] != alt[w][bar - begA[w]] )
         {
            printf( "VIDYA matype Fail: MAVP at bar %d is %.17g, MA(%d) %.17g\n",
                    bar, outMA[i], w ? 12 : 5, alt[w][bar - begA[w]] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_vidyaMaTypeCmp++;
      }
   }
   TA_SetUnstablePeriod( TA_FUNC_UNST_VIDYA, 0 );

   /* BBANDS' middle band is MA(close, 20, VIDYA). */
   rcM = TA_BBANDS( 0, VIDYA_NB-1, c, 20, 2.0, 2.0, TA_MAType_VIDYA,
                    &begM, &nbM, up, mid, lo );
   rcV = TA_VIDYA( 0, VIDYA_NB-1, c, 20, 15, &begV, &nbV, outV );
   if( rcM != TA_SUCCESS || rcV != TA_SUCCESS || nbM <= 0 || begM < begV )
   {
      printf( "VIDYA matype Fail: BBANDS rc=%d (%d,%d)\n", (int)rcM, begM, nbM );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbM; i++ )
   {
      if( mid[i] != outV[begM - begV + i] )
      {
         printf( "VIDYA matype Fail: BBANDS middle at bar %d %.17g != %.17g\n",
                 begM + i, mid[i], outV[begM - begV + i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_vidyaMaTypeCmp++;
   }

   return TA_TEST_PASS;
}

/* (7) An IIR recursion: TA_STABLE_CONVERGING with its own id. */
typedef struct { int n, m; const TA_Real *in; } VidyaRangeParam;

static TA_RetCode vidyaRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                          TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                          TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                          TA_Integer *lookback, void *opaqueData,
                                          unsigned int outputNb, unsigned int *isOutputInteger )
{
   VidyaRangeParam *p = (VidyaRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_VIDYA_Lookback( p->n, p->m );
   return TA_VIDYA( startIdx, endIdx, p->in, p->n, p->m,
                    outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_vidya_range( const TA_Real *in )
{
   VidyaRangeParam param;

   param.n  = 12;
   param.m  = 9;
   param.in = in;

   return doRangeTestEx( vidyaRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_VIDYA,
                         (void *)&param, 1, 0 );
}
