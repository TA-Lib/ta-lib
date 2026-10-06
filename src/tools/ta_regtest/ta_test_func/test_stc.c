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
 *  092926 MF,CC  First version (issue #478).
 */

/* Description:
 *
 *   Test TA_STC (Schaff Trend Cycle).
 *
 *   Legs:
 *     1. Composite, bit-exact: TA_EMA(fast) - TA_EMA(slow) from the line's
 *        first bar (and TA_MACD's line at a zero EMA unstable period), TA_MIN /
 *        TA_MAX over it and over PF, the held fractions and the two 0.5
 *        smoothers, over tuples, both unstable ids and swept start indexes.
 *     2. Lookback and both unstable ids, and the shift identity in the STC id.
 *     3. Frozen goldens on the issue's walk. The tail rows are LEAN 2.5.18090
 *        SchaffTrendCycle captured through the ta-lib-oracles lean_serve arm
 *        TA_STC. The early rows are the issue's re-derivation.
 *     4. A sustained trend reads exactly 100: the hold rule.
 *     5. Flat runs: all-flat, R and F(p), at zero tolerance.
 *     6. Fast/slow swap, and fast == slow.
 *     7. In-place (outReal == inReal) and the startIdx/endIdx range sweep.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define STC_WALK_NB 1200
#define STC_MAX_NB  3000

/* LEAN computes in decimal: worst 4.6e-11 over the six rows. */
#define STC_LEAN_ABS  2e-10
/* The early rows come from an unfused re-derivation; the library fuses each
 * EMA step into fma(), as TA_EMA does, which moves them by up to 1.6e-12 (bar
 * 86). The seed rule they pin moves bar 86 by 5.4e-3. */
#define STC_EARLY_ABS 5e-12

/* TA_EMA against stcRefEma, relative. Worst over the grid: 2.6e-16. */
#define STC_EMA_REL 8e-16

#define STC_COMPOSITE_CMP 2030988
#define STC_EMA_CMP       4288616
#define STC_MACD_CMP      1544001
#define STC_LOOKBACK_CMP  14
#define STC_SHIFT_CMP     46085
#define STC_GOLD_CMP      10
#define STC_TREND_CMP     336
#define STC_FLAT_CMP      7357
#define STC_SWAP_CMP      6866
#define STC_INPLACE_CMP   20484

typedef struct { int bar; double want; } StcGolden;

static const StcGolden stcLean[] =
{
   {  411,  4.700918621972339  },
   {  545, 32.474069289844074 },
   {  570, 17.104962402333424 },
   {  692, 63.56417075360968  },
   { 1065, 70.51638442893523  },
   { 1168, 89.38805758203083  },
};

/* startIdx 73: the smoothers are seeded on the first value, not on an SMA of
 * three (that gives 78.16919818630875 at bar 77). */
static const StcGolden stcEarly[] =
{
   { 73,  1.969848703069687 },
   { 77, 80.34171989997208  },
   { 86, 17.89909687775242  },
};
/* Bar 73 under startIdx 0. */
#define STC_BAR73_FROM0 1.2058547348130355

typedef struct { int fast, slow, cycle; } StcTuple;

/* Cycle 40 exceeds the rings' 30-slot stack prologue. */
static const StcTuple stcTuples[] =
{
   { 23, 50, 10 }, { 2, 3, 2 }, { 12, 26, 40 }, { 5, 13, 3 }, { 2, 26, 10 },
};
static const int stcUnst[] = { 0, 5, TA_UNSTABLE_AUTO_PREC_4, TA_UNSTABLE_AUTO_PREC_8 };
/* Offsets from the lookback, then absolute bars: the seeds are anchored to
 * startIdx, so a gate at the lookback alone cannot see an anchor error. */
static const int stcStartOff[] = { 0, 1, 7 };
static const int stcStartAbs[] = { 150, 333, 800 };
#define NB_OF(a) ((int)(sizeof(a)/sizeof((a)[0])))

static int g_stcCompositeCmp;
static int g_stcEmaCmp;
static int g_stcMacdCmp;
static int g_stcLookbackCmp;
static int g_stcShiftCmp;
static int g_stcGoldCmp;
static int g_stcTrendCmp;
static int g_stcFlatCmp;
static int g_stcSwapCmp;
static int g_stcInplaceCmp;

static void        stcWalk( double *c, int nb );
static void        stcSin( double *c, int nb );
static int         stcTrend( double *c );
static int         stcFlatF( double *c, double p );
static int         stcFlatR( double *c );
static void        stcSetUnst( int k, int u );
static ErrorNumber stcCall( const char *tag, const double *x, int nb,
                            int fast, int slow, int cycle, int route,
                            double *out, int *begIdx, int *nbElement );
static ErrorNumber test_stc_composite( void );
static ErrorNumber test_stc_lookback( void );
static ErrorNumber test_stc_goldens( void );
static ErrorNumber test_stc_trend( void );
static ErrorNumber test_stc_flat( void );
static ErrorNumber test_stc_swap( void );
static ErrorNumber test_stc_inplace( void );
static ErrorNumber test_stc_range( const TA_Real *in );

/**** Global functions definitions. ****/
ErrorNumber test_func_stc( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   g_stcCompositeCmp = g_stcMacdCmp = g_stcLookbackCmp = g_stcShiftCmp = 0;
   g_stcEmaCmp = 0;
   g_stcGoldCmp = g_stcTrendCmp = g_stcFlatCmp = g_stcSwapCmp = g_stcInplaceCmp = 0;

   err = test_stc_goldens();
   if( err == TA_TEST_PASS )
      err = test_stc_composite();
   if( err == TA_TEST_PASS )
      err = test_stc_lookback();
   if( err == TA_TEST_PASS )
      err = test_stc_trend();
   if( err == TA_TEST_PASS )
      err = test_stc_flat();
   if( err == TA_TEST_PASS )
      err = test_stc_swap();
   if( err == TA_TEST_PASS )
      err = test_stc_inplace();

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS )
      err = test_stc_range( history->close );

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* Literal counts: every input is synthesized, so each leg is deterministic. */
   if( err == TA_TEST_PASS
       && ( g_stcCompositeCmp != STC_COMPOSITE_CMP || g_stcMacdCmp != STC_MACD_CMP
            || g_stcEmaCmp != STC_EMA_CMP
            || g_stcLookbackCmp != STC_LOOKBACK_CMP || g_stcShiftCmp != STC_SHIFT_CMP
            || g_stcGoldCmp != STC_GOLD_CMP || g_stcTrendCmp != STC_TREND_CMP
            || g_stcFlatCmp != STC_FLAT_CMP || g_stcSwapCmp != STC_SWAP_CMP
            || g_stcInplaceCmp != STC_INPLACE_CMP ) )
   {
      printf( "STC Fail: coverage counters (composite %d, ema %d, macd %d, lookback %d, "
              "shift %d, gold %d, trend %d, flat %d, swap %d, inplace %d) are not what "
              "this file was written with (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n",
              g_stcCompositeCmp, g_stcEmaCmp, g_stcMacdCmp, g_stcLookbackCmp, g_stcShiftCmp,
              g_stcGoldCmp, g_stcTrendCmp, g_stcFlatCmp, g_stcSwapCmp, g_stcInplaceCmp,
              STC_COMPOSITE_CMP, STC_EMA_CMP, STC_MACD_CMP, STC_LOOKBACK_CMP, STC_SHIFT_CMP,
              STC_GOLD_CMP, STC_TREND_CMP, STC_FLAT_CMP, STC_SWAP_CMP, STC_INPLACE_CMP );
      return TA_STC_VACUOUS;
   }

   return err;
}

/**** Local functions definitions. ****/

/* Integer state plus one IEEE add per bar: no libm, so every platform builds
 * the same doubles. c[1] = 99.8127323702862, c[1199] = 98.4624711415217. */
static void stcWalk( double *c, int nb )
{
   uint64_t x = 20260924u;
   int i;

   c[0] = 100.0;
   for( i = 1; i < nb; i++ )
   {
      x = 6364136223846793005ull * x + 1442695040888963407ull;
      c[i] = c[i-1] + ((double)(x >> 11) / 9007199254740992.0 - 0.5);
   }
}

static void stcSin( double *c, int nb )
{
   int i;

   for( i = 0; i < nb; i++ )
      c[i] = 100.0 + 10.0*sin(i/7.0) + 0.15*i + 2.0*sin(i/2.3);
}

/* walk(200), then a parabola from bar 199: the MACD line rises on every bar
 * from 208 and PF saturates at exactly 100 from 264. */
static int stcTrend( double *c )
{
   int k;

   stcWalk( c, 200 );
   for( k = 1; k <= 400; k++ )
      c[199+k] = c[199] + 0.002 * (double)(k*k);
   return 600;
}

/* The 300-bar walk shifted so that bar 299 is exactly p, then 2700 bars at p. */
static int stcFlatF( double *c, double p )
{
   static double w[300];
   int i;

   stcWalk( w, 300 );
   for( i = 0; i < 300; i++ )
      c[i] = (w[i] - w[299]) + p;
   for( i = 300; i < 3000; i++ )
      c[i] = p;
   return 3000;
}

/* walk(300), 60 bars each 0.2 below the previous, then flat at the last. */
static int stcFlatR( double *c )
{
   int i;

   stcWalk( c, 300 );
   for( i = 300; i < 360; i++ )
      c[i] = c[i-1] - 0.2;
   for( i = 360; i < 2000; i++ )
      c[i] = c[359];
   return 2000;
}

static void stcSetUnst( int k, int u )
{
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, (unsigned int)k );
   TA_SetUnstablePeriod( TA_FUNC_UNST_STC, (unsigned int)u );
}

/* Whole-series call at both unstable periods 0, checked for its range and,
 * when route is set, replayed on the language servers. */
static ErrorNumber stcCall( const char *tag, const double *x, int nb,
                            int fast, int slow, int cycle, int route,
                            double *out, int *begIdx, int *nbElement )
{
   TA_RetCode retCode;
   double optIn[3];
   ErrorNumber e;
   int cmpBefore;
   int lb = TA_STC_Lookback( fast, slow, cycle );

   retCode = TA_STC( 0, nb-1, x, fast, slow, cycle, begIdx, nbElement, out );
   if( retCode != TA_SUCCESS || *begIdx != lb || *nbElement != nb-lb )
   {
      printf( "STC %s Fail [%d,%d,%d]: rc=%d (%d,%d) expected (%d,%d)\n", tag, fast, slow,
              cycle, (int)retCode, *begIdx, *nbElement, lb, nb-lb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   if( !route || !server_verify_active() )
      return TA_TEST_PASS;

   optIn[0] = (double)fast;
   optIn[1] = (double)slow;
   optIn[2] = (double)cycle;
   cmpBefore = server_verify_comparisons();
   e = server_verify( "STC", 0, nb-1, nb, retCode, *begIdx, *nbElement,
                      (const TA_Real*[]){ x, NULL }, optIn, 3,
                      (const TA_Real*[]){ out, NULL }, NULL );
   if( e != TA_TEST_PASS )
      return e;
   if( server_verify_comparisons() == cmpBefore )
   {
      printf( "STC %s [%d,%d,%d]: compared no server despite live pipes\n", tag,
              fast, slow, cycle );
      return TA_SV_ROUTED_VACUOUS;
   }
   return TA_TEST_PASS;
}

static ErrorNumber stcCheck( const char *tag, int bar, double got, double want, double abs )
{
   double err;
   const char *mode;

   if( !checkOracleValue( got, want, 0.0, abs, &err, &mode ) )
   {
      printf( "STC %s Fail at bar %d: got %.17g expected %.17g (%s %.3e)\n",
              tag, bar, got, want, mode, err );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_stcGoldCmp++;
   return TA_TEST_PASS;
}

/* (3) */
static ErrorNumber test_stc_goldens( void )
{
   static double c[STC_WALK_NB], out[STC_WALK_NB];
   TA_Integer begIdx = 0, nbElement = 0;
   TA_RetCode rc;
   ErrorNumber e;
   int k;

   stcWalk( c, STC_WALK_NB );

   e = stcCall( "golden", c, STC_WALK_NB, 23, 50, 10, 1, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   for( k = 0; k < NB_OF(stcLean); k++ )
   {
      e = stcCheck( "LEAN 2.5.18090", stcLean[k].bar, out[stcLean[k].bar - begIdx],
                    stcLean[k].want, STC_LEAN_ABS );
      if( e != TA_TEST_PASS )
         return e;
   }
   e = stcCheck( "early (startIdx 0)", 73, out[73 - begIdx], STC_BAR73_FROM0, STC_EARLY_ABS );
   if( e != TA_TEST_PASS )
      return e;

   rc = TA_STC( 73, STC_WALK_NB-1, c, 23, 50, 10, &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS || begIdx != 73 || nbElement != STC_WALK_NB-73 )
   {
      printf( "STC early Fail: rc=%d (%d,%d)\n", (int)rc, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( k = 0; k < NB_OF(stcEarly); k++ )
   {
      e = stcCheck( "early (startIdx 73)", stcEarly[k].bar, out[stcEarly[k].bar - begIdx],
                    stcEarly[k].want, STC_EARLY_ABS );
      if( e != TA_TEST_PASS )
         return e;
   }

   return TA_TEST_PASS;
}

/* Two stochastic stages over TA_MIN/TA_MAX, each fraction held on a zero
 * range (0.0 before any), each smoother seeded on its first fraction. The
 * smoother step is fused, as the library's is: below 2^-1073 the product
 * 0.5*d is inexact and the unfused step rounds differently (F(p) reaches it). */
static void stcStage( const double *x, int n, int cycle, double *outSm,
                      double *lo, double *hi )
{
   TA_Integer b, m;
   double frac = 0.0, sm = 0.0, r;
   int j;

   TA_MIN( 0, n-1, x, cycle, &b, &m, lo );
   TA_MAX( 0, n-1, x, cycle, &b, &m, hi );
   for( j = 0; j < m; j++ )
   {
      r = hi[j] - lo[j];
      if( r > 0.0 )
         frac = ((x[j+cycle-1] - lo[j]) / r) * 100.0;
      sm = j == 0 ? frac : fma( 0.5, frac - sm, sm );
      outSm[j] = sm;
   }
}

/* TA_EMA's contract with the recurrence spelled prev + k*(x - prev). Keep it
 * independent of the form TA_EMA uses: STC_EMA_REL holds TA_EMA to it, a check
 * the compositions built from TA_EMA cannot make. */
static TA_RetCode stcRefEma( int startIdx, int endIdx, const double *in, int period,
                             TA_Integer *outBegIdx, TA_Integer *outNBElement, double *out )
{
   int lookback = TA_EMA_Lookback( period );
   double k = 2.0 / (double)(period + 1), prev = 0.0;
   int today, i, outIdx = 0;

   *outBegIdx = 0;
   *outNBElement = 0;
   if( startIdx < lookback )
      startIdx = lookback;
   if( startIdx > endIdx )
      return TA_SUCCESS;
   *outBegIdx = startIdx;

   if( period == 1 )
   {
      for( today = startIdx; today <= endIdx; today++ )
         out[outIdx++] = in[today];
      *outNBElement = outIdx;
      return TA_SUCCESS;
   }

   today = startIdx - lookback;
   for( i = 0; i < period; i++ )
      prev += in[today++];
   prev /= period;
   while( today <= startIdx )
   {
      prev = fma( in[today] - prev, k, prev );
      today++;
   }
   out[outIdx++] = prev;
   while( today <= endIdx )
   {
      prev = fma( in[today] - prev, k, prev );
      today++;
      out[outIdx++] = prev;
   }
   *outNBElement = outIdx;
   return TA_SUCCESS;
}

/* (1) The line is fed from L0 = startIdx - 2*(cycle-1) - u, so PFF is seeded
 * at startIdx - u. A chain fed from the EMA seed bar reads red at (k,u) = (5,0). */
static ErrorNumber stcComposite( const char *tag, const double *c, int nb,
                                 const StcTuple *t, int k, int u, int startIdx )
{
   static double out[STC_MAX_NB], ef[STC_MAX_NB], es[STC_MAX_NB], line[STC_MAX_NB];
   static double mac[STC_MAX_NB], sig[STC_MAX_NB], hist[STC_MAX_NB];
   static double pf[STC_MAX_NB], pff[STC_MAX_NB], lo[STC_MAX_NB], hi[STC_MAX_NB];
   static double rf[STC_MAX_NB], rs[STC_MAX_NB];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, b1, n1, b2, n2;
   int L0 = startIdx - 2*(t->cycle-1) - u;
   int nL = nb - L0, j;

   rc = TA_STC( startIdx, nb-1, c, t->fast, t->slow, t->cycle, &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS || begIdx != startIdx || nbElement != nb - startIdx )
   {
      printf( "STC %s Fail [%d,%d,%d k %d u %d start %d]: rc=%d (%d,%d)\n", tag, t->fast,
              t->slow, t->cycle, k, u, startIdx, (int)rc, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   rc  = TA_EMA( L0, nb-1, c, t->fast, &b1, &n1, ef );
   rc |= TA_EMA( L0, nb-1, c, t->slow, &b2, &n2, es );
   if( rc != TA_SUCCESS || b1 != L0 || b2 != L0 || n1 != nL || n2 != nL )
   {
      printf( "STC %s Fail: TA_EMA(%d) answered (%d,%d) (%d,%d)\n", tag, L0, b1, n1, b2, n2 );
      return TA_STC_VACUOUS;
   }
   rc  = stcRefEma( L0, nb-1, c, t->fast, &b1, &n1, rf );
   rc |= stcRefEma( L0, nb-1, c, t->slow, &b2, &n2, rs );
   if( rc != TA_SUCCESS || b1 != L0 || b2 != L0 || n1 != nL || n2 != nL )
   {
      printf( "STC %s Fail: stcRefEma(%d) answered (%d,%d) (%d,%d)\n", tag, L0, b1, n1, b2, n2 );
      return TA_STC_VACUOUS;
   }
   for( j = 0; j < nL; j++ )
   {
      if( !(fabs( ef[j] - rf[j] ) <= STC_EMA_REL * fabs( rf[j] ))
          || !(fabs( es[j] - rs[j] ) <= STC_EMA_REL * fabs( rs[j] )) )
      {
         printf( "STC %s Fail [%d,%d k %d start %d] at bar %d: TA_EMA %.17g, %.17g is not "
                 "the reference step's %.17g, %.17g within %.0e relative\n", tag, t->fast,
                 t->slow, k, startIdx, L0 + j, ef[j], es[j], rf[j], rs[j], STC_EMA_REL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      line[j] = ef[j] - es[j];
   }
   g_stcEmaCmp += 2 * nL;

   /* Under a level the signal EMA of period 1 adds no bar, as at a count of 0. */
   if( k == 0 || k > TA_INDEX_MAX )
   {
      rc = TA_MACD( L0, nb-1, c, t->fast, t->slow, 1, &b1, &n1, mac, sig, hist );
      if( rc != TA_SUCCESS || b1 != L0 || n1 != nL
          || memcmp( mac, line, (size_t)nL * sizeof(double) ) != 0 )
      {
         printf( "STC %s Fail [%d,%d start %d]: TA_EMA - TA_EMA is not TA_MACD's line\n",
                 tag, t->fast, t->slow, startIdx );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_stcMacdCmp += nL;
   }

   stcStage( line, nL, t->cycle, pf, lo, hi );
   stcStage( pf, nL - (t->cycle-1), t->cycle, pff, lo, hi );

   if( memcmp( out, pff + u, (size_t)nbElement * sizeof(double) ) != 0 )
   {
      for( j = 0; j < nbElement; j++ )
         if( out[j] != pff[j+u] )
            break;
      printf( "STC %s Fail [%d,%d,%d k %d u %d start %d] at bar %d: %.17g != %.17g "
              "(must be BIT-exact)\n", tag, t->fast, t->slow, t->cycle, k, u, startIdx,
              startIdx + j, out[j], pff[j+u] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_stcCompositeCmp += nbElement;
   return TA_TEST_PASS;
}

static ErrorNumber test_stc_composite( void )
{
   static double c[STC_MAX_NB];
   const char *tags[] = { "composite walk", "composite sin", "composite trend",
                          "composite F(130)" };
   ErrorNumber e;
   int s, it, ik, iu, is, nb, lb, u, startIdx;

   for( s = 0; s < NB_OF(tags); s++ )
   {
      if( s == 0 )
         stcWalk( c, nb = STC_WALK_NB );
      else if( s == 1 )
         stcSin( c, nb = STC_WALK_NB );
      else if( s == 2 )
         nb = stcTrend( c );
      else
         nb = stcFlatF( c, 130.0 );

      for( it = 0; it < NB_OF(stcTuples); it++ )
      for( ik = 0; ik < NB_OF(stcUnst); ik++ )
      for( iu = 0; iu < NB_OF(stcUnst); iu++ )
      {
         /* The count the STC id adds: its setting, or under a level that level's. */
         stcSetUnst( stcUnst[ik], 0 );
         u = TA_STC_Lookback( stcTuples[it].fast, stcTuples[it].slow, stcTuples[it].cycle );
         stcSetUnst( stcUnst[ik], stcUnst[iu] );
         lb = TA_STC_Lookback( stcTuples[it].fast, stcTuples[it].slow, stcTuples[it].cycle );
         u = lb - u;
         for( is = 0; is < NB_OF(stcStartOff) + NB_OF(stcStartAbs); is++ )
         {
            startIdx = is < NB_OF(stcStartOff) ? lb + stcStartOff[is]
                                               : stcStartAbs[is - NB_OF(stcStartOff)];
            if( startIdx < lb || startIdx >= nb )
               continue;
            e = stcComposite( tags[s], c, nb, &stcTuples[it], stcUnst[ik], u, startIdx );
            if( e != TA_TEST_PASS )
               return e;
         }
      }
   }

   stcSetUnst( 0, 0 );
   return TA_TEST_PASS;
}

/* (2) 67 at both ids 0. Rejects LEAN's 58, the 71 of EMA(3)-seeded smoothers,
 * ta4j's 76, a lookback built on TA_MACD_Lookback (counts k twice) and one
 * without the own id. */
static ErrorNumber test_stc_lookback( void )
{
   static const int ks[] = { 0, 1, 5 }, us[] = { 0, 1, 5 };
   static const int shiftK[] = { 0, 5 }, shiftU[] = { 1, 5, 20, 60 };
   static const int shiftS[] = { 0, 67, 100, 150, 300, 700 };
   static double c[STC_WALK_NB], a[STC_WALK_NB], b[STC_WALK_NB];
   TA_RetCode rc;
   TA_Integer begA, nbA, begB, nbB;
   int ik, iu, is, want, s0;

   stcWalk( c, STC_WALK_NB );

   for( ik = 0; ik < NB_OF(ks); ik++ )
   for( iu = 0; iu < NB_OF(us); iu++ )
   {
      want = 67 + ks[ik] + us[iu];
      stcSetUnst( ks[ik], us[iu] );
      rc = TA_STC( 0, STC_WALK_NB-1, c, 23, 50, 10, &begA, &nbA, a );
      if( rc != TA_SUCCESS || begA != want || nbA != STC_WALK_NB - want
          || TA_STC_Lookback( 23, 50, 10 ) != want || TA_STC_Lookback( 50, 23, 10 ) != want )
      {
         printf( "STC lookback Fail [k %d u %d]: outBegIdx %d, lookback %d, expected %d\n",
                 ks[ik], us[iu], begA, TA_STC_Lookback( 23, 50, 10 ), want );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      g_stcLookbackCmp++;
   }

   /* Under a level the count follows the periods: the swapped order must
    * resolve the same one, in the lookback and in the call. */
   for( ik = 0; ik < 2; ik++ )
   for( iu = 0; iu < 2; iu++ )
   {
      stcSetUnst( ik ? TA_UNSTABLE_AUTO_PREC_8 : TA_UNSTABLE_AUTO_PREC_4,
                  iu ? TA_UNSTABLE_AUTO_PREC_8 : TA_UNSTABLE_AUTO_PREC_4 );
      want = TA_STC_Lookback( 23, 50, 10 );
      rc = TA_STC( 0, STC_WALK_NB-1, c, 50, 23, 10, &begA, &nbA, a );
      if( rc != TA_SUCCESS || want <= 67 || TA_STC_Lookback( 50, 23, 10 ) != want
          || begA != want || nbA != STC_WALK_NB - want )
      {
         printf( "STC lookback Fail [level %d %d]: swapped periods answer lookback %d and "
                 "outBegIdx %d, expected %d\n", ik, iu, TA_STC_Lookback( 50, 23, 10 ),
                 begA, want );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      g_stcLookbackCmp++;
   }
   stcSetUnst( 0, 0 );

   if( TA_STC_Lookback( 1, 50, 10 ) != -1 || TA_STC_Lookback( 23, 1, 10 ) != -1
       || TA_STC_Lookback( 23, 50, 1 ) != -1 || TA_STC_Lookback( 100001, 50, 10 ) != -1
       || TA_STC_Lookback( 23, 50, 100001 ) != -1 )
   {
      printf( "STC lookback Fail: an out-of-range period is not rejected\n" );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   g_stcLookbackCmp++;

   /* (k,u) at S equals (k,0) at S-u on every bar >= S: u only moves the
    * chain's first bar back. The same identity in k does not hold. */
   for( ik = 0; ik < NB_OF(shiftK); ik++ )
   for( iu = 0; iu < NB_OF(shiftU); iu++ )
   for( is = 0; is < NB_OF(shiftS); is++ )
   {
      int k = shiftK[ik], u = shiftU[iu], j;

      stcSetUnst( k, u );
      rc = TA_STC( shiftS[is], STC_WALK_NB-1, c, 23, 50, 10, &begA, &nbA, a );
      s0 = begA;
      stcSetUnst( k, 0 );
      if( rc == TA_SUCCESS )
         rc = TA_STC( s0 - u, STC_WALK_NB-1, c, 23, 50, 10, &begB, &nbB, b );
      if( rc != TA_SUCCESS || s0 != (shiftS[is] > 67+k+u ? shiftS[is] : 67+k+u)
          || begB != s0 - u || nbB != nbA + u )
      {
         printf( "STC shift Fail [k %d u %d S %d]: rc=%d (%d,%d) (%d,%d)\n", k, u,
                 shiftS[is], (int)rc, begA, nbA, begB, nbB );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      if( memcmp( a, b + u, (size_t)nbA * sizeof(double) ) != 0 )
      {
         for( j = 0; j < nbA; j++ )
            if( a[j] != b[j+u] )
               break;
         printf( "STC shift Fail [k %d u %d S %d] at bar %d: %.17g != %.17g\n", k, u,
                 shiftS[is], s0 + j, a[j], b[j+u] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_stcShiftCmp += nbA;
   }
   stcSetUnst( 0, 0 );
   return TA_TEST_PASS;
}

/* (4) PF saturates at exactly 100 and the second range reaches exactly 0: a
 * zero-on-flat rule drops below 50 at bar 274. */
static ErrorNumber test_stc_trend( void )
{
   static double c[STC_MAX_NB], out[STC_MAX_NB];
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   int nb, i;

   nb = stcTrend( c );
   e = stcCall( "trend", c, nb, 23, 50, 10, 1, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   for( i = 264 - begIdx; i < nbElement; i++ )
   {
      if( out[i] != 100.0 )
      {
         printf( "STC trend Fail at bar %d: %.17g, expected exactly 100\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_stcTrendCmp++;
   }
   return TA_TEST_PASS;
}

/* (5) Flat runs make the MACD line an exact constant, so the exact > 0 range
 * test needs no guard band. */
static ErrorNumber test_stc_flat( void )
{
   static const double ps[] = { 130.0, 130.7, 101.3 };
   static double c[STC_MAX_NB], out[STC_MAX_NB];
   const double tiny = 4.9406564584124654e-324, tiny2 = 2.0 * tiny, zero = 0.0;
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   int nb, i, ip;

   for( i = 0; i < 1000; i++ )
      c[i] = 123.45;
   e = stcCall( "all-flat", c, 1000, 23, 50, 10, 1, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   for( i = 0; i < nbElement; i++ )
   {
      if( out[i] != 0.0 )
      {
         printf( "STC all-flat Fail at bar %d: %.17g\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_stcFlatCmp++;
   }

   nb = stcFlatR( c );
   e = stcCall( "R", c, nb, 23, 50, 10, 1, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   for( i = 415 - begIdx; i < nbElement; i++ )
   {
      if( out[i] != 100.0 )
      {
         printf( "STC R Fail at bar %d: %.17g, expected exactly 100\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_stcFlatCmp++;
   }

   /* Halving toward 0 through the subnormals: the fused step rounds
    * 2^-1074 - 2^-1075 to even, so the output reaches +0.0 and holds it. */
   for( ip = 0; ip < NB_OF(ps); ip++ )
   {
      nb = stcFlatF( c, ps[ip] );
      e = stcCall( "F", c, nb, 23, 50, 10, ip == 0, out, &begIdx, &nbElement );
      if( e != TA_TEST_PASS )
         return e;
      for( i = 1387 - begIdx; i < nbElement; i++ )
      {
         const double *want = begIdx+i == 1387 ? &tiny2 : begIdx+i == 1388 ? &tiny : &zero;
         if( memcmp( &out[i], want, sizeof(double) ) != 0 )
         {
            printf( "STC F(%g) Fail at bar %d: %a, expected %a\n", ps[ip],
                    begIdx+i, out[i], *want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_stcFlatCmp++;
      }
   }
   return TA_TEST_PASS;
}

/* (6) Fast and slow swap as in TA_MACD. Equal periods give an identically
 * zero line, hence 0.0 held throughout. */
static ErrorNumber test_stc_swap( void )
{
   static const StcTuple sw[] = { { 50, 23, 10 }, { 3, 2, 2 }, { 26, 12, 40 } };
   static const StcTuple eq[] = { { 23, 23, 10 }, { 2, 2, 2 }, { 40, 40, 40 } };
   static double c[STC_WALK_NB], a[STC_WALK_NB], b[STC_WALK_NB];
   TA_Integer begA, nbA, begB, nbB;
   ErrorNumber e;
   int k, i;

   stcWalk( c, STC_WALK_NB );

   for( k = 0; k < NB_OF(sw); k++ )
   {
      int f = sw[k].fast, s = sw[k].slow, cy = sw[k].cycle;
      e = stcCall( "swap", c, STC_WALK_NB, f, s, cy, 0, a, &begA, &nbA );
      if( e == TA_TEST_PASS )
         e = stcCall( "swap", c, STC_WALK_NB, s, f, cy, 0, b, &begB, &nbB );
      if( e != TA_TEST_PASS )
         return e;
      if( begA != (f > s ? f : s) - 1 + 2*(cy-1) || begA != begB || nbA != nbB
          || memcmp( a, b, (size_t)nbA * sizeof(double) ) != 0 )
      {
         printf( "STC swap Fail [%d,%d,%d]: (%d,%d) vs (%d,%d) or values differ\n",
                 f, s, cy, begA, nbA, begB, nbB );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_stcSwapCmp += nbA;
   }

   for( k = 0; k < NB_OF(eq); k++ )
   {
      e = stcCall( "fast == slow", c, STC_WALK_NB, eq[k].fast, eq[k].slow, eq[k].cycle,
                   k == 0, a, &begA, &nbA );
      if( e != TA_TEST_PASS )
         return e;
      for( i = 0; i < nbA; i++ )
      {
         if( a[i] != 0.0 )
         {
            printf( "STC fast == slow Fail [%d,%d] at bar %d: %.17g\n", eq[k].fast,
                    eq[k].cycle, begA+i, a[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_stcSwapCmp++;
      }
   }

   /* Two parameters off their defaults, which the generic sweep never sends. */
   return stcCall( "two off default", c, STC_WALK_NB, 12, 26, 10, 1, a, &begA, &nbA );
}

/* (7) outReal may alias inReal: every input bar is read before its slot is
 * written. */
static ErrorNumber test_stc_inplace( void )
{
   static const int starts[] = { 0, 300 };
   static double c[STC_WALK_NB], buf[STC_WALK_NB], out[STC_WALK_NB];
   TA_RetCode rc1, rc2;
   TA_Integer b1, n1, b2, n2;
   int it, is, iu;

   stcWalk( c, STC_WALK_NB );

   for( iu = 0; iu < 2; iu++ )
   for( it = 0; it < NB_OF(stcTuples); it++ )
   for( is = 0; is < NB_OF(starts); is++ )
   {
      const StcTuple *t = &stcTuples[it];

      stcSetUnst( iu*5, iu*5 );
      memcpy( buf, c, sizeof(c) );
      rc1 = TA_STC( starts[is], STC_WALK_NB-1, c, t->fast, t->slow, t->cycle, &b1, &n1, out );
      rc2 = TA_STC( starts[is], STC_WALK_NB-1, buf, t->fast, t->slow, t->cycle, &b2, &n2, buf );
      if( rc1 != TA_SUCCESS || rc2 != TA_SUCCESS || b1 != b2 || n1 != n2 || n1 <= 0
          || memcmp( out, buf, (size_t)n1 * sizeof(double) ) != 0 )
      {
         printf( "STC in-place Fail [%d,%d,%d unst %d start %d]\n", t->fast, t->slow,
                 t->cycle, iu*5, starts[is] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_stcInplaceCmp += n1;
   }
   stcSetUnst( 0, 0 );
   return TA_TEST_PASS;
}

/* An IIR chain under two ids: its own, which moves the whole chain back,
 * and EMA's, inherited through the MACD line and reaching only the line,
 * so both are swept together. Short periods: STC's envelope ignores 200 bars, and at the
 * defaults the 252-bar history leaves no output past them to compare. */
typedef struct { const TA_Real *in; } StcRangeParam;

static TA_RetCode stcRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   StcRangeParam *p = (StcRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_STC_Lookback( 5, 13, 4 );
   return TA_STC( startIdx, endIdx, p->in, 5, 13, 4,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_stc_range( const TA_Real *in )
{
   static const TA_FuncUnstId ids[] = { TA_FUNC_UNST_STC, TA_FUNC_UNST_EMA };
   StcRangeParam param;

   param.in = in;
   return doRangeTestMulti( stcRangeTestFunction, TA_STABLE_CONVERGING,
                            ids, NB_OF(ids), (void *)&param, 1, 0 );
}
