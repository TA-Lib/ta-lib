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
 *  100626 KL,CC  First version (proposal WAVETREND, #476).
 */

/* Description:
 *
 *   Test TA_WAVETREND (LazyBear's WaveTrend Oscillator, #476).
 *
 *   The two guards are why this ships as a function rather than as a chain of
 *   shipped calls, so they are what the legs have to discriminate.
 *
 *   The COMPOSITION leg holds the arithmetic: on moving data TA_WAVETREND must
 *   land on the same BITS as TA_TYPPRICE, TA_EMA, the absolute distance,
 *   TA_EMA, the quotient, TA_EMA and TA_SMA, each entered at its callee's
 *   lookback. That chain is written here WITHOUT either guard, which is what
 *   keeps it an independent check of them rather than a restatement -- and
 *   the leg therefore asserts its own premise, that the fixpoint test fires on
 *   ZERO bars of this corpus. A guard that quietly triggered on moving data
 *   would move values no golden at a sane tolerance could see; this is what
 *   would catch that.
 *
 *   The FLAT leg holds the first guard. It cannot be held by frozen values:
 *   the guard exists because a double-precision exponential average FREEZES
 *   once its step falls under half an ulp, which a high-precision reference
 *   never does, so no 60-digit table can predict the guarded output. It is
 *   pinned by properties instead, each one measured: every output finite, and
 *   |WT1| decaying rather than drifting. The naive form walks to -66.67 on
 *   the same data, so the margin is thirteen orders of magnitude.
 *
 *   The GOLDEN leg holds the formula, from a 60-digit evaluation over the
 *   committed corpus. MEASURED: the library reproduces those 28 rows to
 *   3.9e-13 by |a-b|/max(|b|,1); the tolerance is 1e-11.
 *
 *   Every comparison count is pinned, so a leg that stops comparing fails.
 *
 *   SERVER_VERIFY: the golden leg, once per parameter set.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define WT_NB_BAR 252

/* The flat run: the first WT_FLAT_MOVE corpus bars, then flat at the last of
 * them. 450 flat bars is past the freeze at a channel period of 10 (MEASURED:
 * the decay turns geometric between bars 300 and 325). */
#define WT_FLAT_NB   600
#define WT_FLAT_MOVE 150

/* 4 parameter sets x 7 bars x 2 outputs. */
#define WT_GOLDEN_CMP     56
/* 4 sets x 3 unstable periods x 3 startIdx values (the lookback, one past it
 * and 40 past it), two outputs on every output bar of a 252-bar corpus:
 * 2 * sum of (252 - startIdx), skipping any startIdx past bar 250. Computed
 * from the parameter table, not read back from a run. */
#define WT_COMPOSE_CMP 13848
/* 4 sets x 4 unstable periods: _Lookback and the call's own begIdx. */
#define WT_LOOKBACK_CMP   32
/* Every output bar of the 600-bar flat run, at two channel periods: the
 * lookbacks are 2*9+20+3 = 41 and 2*99+39+7 = 244, so 559 + 356. Computed
 * from the parameters, not read back from a run. */
#define WT_FLAT_CMP      915
/* Both outputs of the 120-bar subnormal series past its 41-bar lookback. */
#define WT_SUBNORMAL_CMP 158
/* outWT1 and outWT2 each aliased onto each of the three price inputs. */
#define WT_ALIAS_CMP       6

static int g_wtGoldenCmp;
static int g_wtComposeCmp;
static int g_wtLookbackCmp;
static int g_wtFlatCmp;
static int g_wtSubnormalCmp;
static int g_wtAliasCmp;

typedef struct
{
   int    n1;
   int    n2;
   int    n3;
   int    bar;
   double wt1;
   double wt2;
} WtGolden;

/* From a 60-digit evaluation of LazyBear's lines 14-21 over the committed
 * corpus, rounded once to 17 significant digits. The 2/1/1 set is the
 * minimum-period edge, where the signal average is the identity and WT2
 * equals WT1. The fixpoint guard fires on none of these bars (asserted by the
 * composition leg), so the reference needs neither guard.
 */
static const WtGolden wtGolden[] =
{
   { 10, 21, 4,  41, -32.909502909801517, -35.577882554674595 },
   { 10, 21, 4,  42, -20.583680564892962, -32.466423000898438 },
   { 10, 21, 4,  43,  -9.6666585870679391, -25.847097821526958 },
   { 10, 21, 4,  49,  21.558617340463968,  18.256459235159582 },
   { 10, 21, 4, 100,  31.754477452876667,  35.185325674282076 },
   { 10, 21, 4, 180,  21.493853896217523,  33.670405176952414 },
   { 10, 21, 4, 251,   1.0710257379549579,  9.7580185658593059 },
   {  9, 12, 3,  29, -38.247606870206724, -49.836743081550097 },
   {  9, 12, 3,  30, -36.032800850706288, -41.343217219324757 },
   {  9, 12, 3,  31, -28.899828458036886, -34.393412059649968 },
   {  9, 12, 3,  37, -14.452827314168218,  -1.1132545806795429 },
   {  9, 12, 3, 100,  19.378125951031727,  19.398425093876611 },
   {  9, 12, 3, 180,   6.2391084601602849, 17.642319988529582 },
   {  9, 12, 3, 251, -14.212903944253124,  -1.8011575760496956 },
   { 20, 40, 8,  84,  41.243192011668476,  32.020669787566014 },
   { 20, 40, 8,  85,  42.288131514897707,  35.068390556783733 },
   { 20, 40, 8,  86,  43.973123006190576,  37.463036688332551 },
   { 20, 40, 8,  92,  57.17828639945882,   49.410145559922178 },
   { 20, 40, 8, 100,  51.796332577396917,  55.700767108596402 },
   { 20, 40, 8, 180,  31.527395644500178,  37.030062098878687 },
   { 20, 40, 8, 251,  15.053683866420357,  15.910860452397158 },
   {  2,  1, 1,   2,  58.663148636763438,  58.663148636763438 },
   {  2,  1, 1,   3,  34.63638976717445,   34.63638976717445  },
   {  2,  1, 1,   4, -44.031531531531527, -44.031531531531527 },
   {  2,  1, 1,  10,  87.43921937446396,   87.43921937446396  },
   {  2,  1, 1, 100,  27.785318786918545,  27.785318786918545 },
   {  2,  1, 1, 180, -58.057334981372314, -58.057334981372314 },
   {  2,  1, 1, 251, -91.185932853846495, -91.185932853846495 }
};

#define WT_NB_GOLDEN ((int)(sizeof(wtGolden)/sizeof(wtGolden[0])))

/* MEASURED: 3.854e-13 by |a-b|/max(|b|,1) over the rows above. */
#define WT_GOLDEN_TOL 1e-11

static ErrorNumber test_wt_golden  ( const TA_History *history );
static ErrorNumber test_wt_compose ( const TA_History *history );
static ErrorNumber test_wt_lookback( const TA_History *history );
static ErrorNumber test_wt_flat    ( const TA_History *history );
static ErrorNumber test_wt_subnormal( void );
static ErrorNumber test_wt_aliasing( const TA_History *history );
static ErrorNumber test_wt_range   ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_wavetrend( TA_History *history )
{
   ErrorNumber retValue;

   g_wtGoldenCmp = 0;
   g_wtComposeCmp = 0;
   g_wtLookbackCmp = 0;
   g_wtFlatCmp = 0;
   g_wtSubnormalCmp = 0;
   g_wtAliasCmp = 0;

   if( history->nbBars != WT_NB_BAR )
   {
      printf( "Fail: TA_WAVETREND expects the %d-bar corpus, got %d\n",
              WT_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   retValue = test_wt_golden( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_wt_compose( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_wt_lookback( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_wt_flat( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_wt_subnormal();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_wt_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_wt_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   if( g_wtGoldenCmp   != WT_GOLDEN_CMP
    || g_wtComposeCmp  != WT_COMPOSE_CMP
    || g_wtLookbackCmp != WT_LOOKBACK_CMP
    || g_wtFlatCmp     != WT_FLAT_CMP
    || g_wtSubnormalCmp != WT_SUBNORMAL_CMP
    || g_wtAliasCmp    != WT_ALIAS_CMP )
   {
      printf( "Fail: TA_WAVETREND comparison counts (golden %d, compose %d, "
              "lookback %d, flat %d, subnormal %d, alias %d) are not what this "
              "file asserts (%d, %d, %d, %d, %d, %d)\n",
              g_wtGoldenCmp, g_wtComposeCmp, g_wtLookbackCmp, g_wtFlatCmp,
              g_wtSubnormalCmp, g_wtAliasCmp,
              WT_GOLDEN_CMP, WT_COMPOSE_CMP, WT_LOOKBACK_CMP, WT_FLAT_CMP,
              WT_SUBNORMAL_CMP, WT_ALIAS_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* (1) GOLDEN: the formula, from 60 digits over the committed corpus. */
static ErrorNumber test_wt_golden( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   static TA_Real w1[WT_NB_BAR], w2[WT_NB_BAR];
   int k, lastN1 = -1, lastN2 = -1;

   for( k = 0; k < WT_NB_GOLDEN; k++ )
   {
      const WtGolden *g = &wtGolden[k];
      int want = 2*(g->n1 - 1) + (g->n2 - 1) + (g->n3 - 1);
      double got, ref, err;
      int which;

      rc = TA_WAVETREND( 0, (int)history->nbBars - 1,
                         history->high, history->low, history->close,
                         g->n1, g->n2, g->n3, &begIdx, &nbElement, w1, w2 );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_WAVETREND golden rc=%d (%d/%d/%d)\n",
                 (int)rc, g->n1, g->n2, g->n3 );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != want || g->bar < begIdx )
      {
         printf( "Fail: TA_WAVETREND golden range: begIdx=%d (want %d), "
                 "bar %d\n", (int)begIdx, want, g->bar );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      for( which = 0; which < 2; which++ )
      {
         got = which == 0 ? w1[g->bar - begIdx] : w2[g->bar - begIdx];
         ref = which == 0 ? g->wt1 : g->wt2;
         /* The oscillator crosses zero, so one unit is the floor of the
          * denominator: a relative test alone would be meaningless there. */
         err = fabs( got - ref ) / (fabs( ref ) > 1.0 ? fabs( ref ) : 1.0);
         if( !( err <= WT_GOLDEN_TOL ) )
         {
            printf( "Fail: TA_WAVETREND golden bar %d (%d/%d/%d) WT%d: %.17g, "
                    "expected %.17g (rel %.3g, tol %.1e)\n",
                    g->bar, g->n1, g->n2, g->n3, which + 1, got, ref,
                    err, WT_GOLDEN_TOL );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_wtGoldenCmp++;
      }

      if( (g->n1 != lastN1 || g->n2 != lastN2) && server_verify_active() )
      {
         const double opt[3] = { (double)g->n1, (double)g->n2, (double)g->n3 };
         ErrorNumber e;

         e = server_verify( "WAVETREND", 0, (int)history->nbBars - 1,
                            (int)history->nbBars,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ history->high, history->low,
                                                history->close, NULL },
                            opt, 3,
                            (const TA_Real*[]){ w1, w2, NULL }, NULL );
         if( e != TA_TEST_PASS ) return e;
         lastN1 = g->n1;
         lastN2 = g->n2;
      }
   }

   return TA_TEST_PASS;
}

/* (2) COMPOSITION, bit-exact, on moving data.
 *
 * The chain here carries NEITHER guard, so it is an independent check of them
 * rather than a restatement -- which is only valid while neither guard would
 * have changed anything. The divisor guard cannot fire on this corpus (every
 * deviation average is far from zero), and the fixpoint test is counted and
 * asserted to fire on no bar at all: if it ever did, this leg's premise would
 * be void and it would be comparing the function against a different formula.
 */
static ErrorNumber test_wt_compose( const TA_History *history )
{
   static const int n1Set[] = { 10,  9, 20,  2 };
   static const int n2Set[] = { 21, 12, 40,  1 };
   static const int n3Set[] = {  4,  3,  8,  1 };
   static const int uSet[]  = {  0,  2,  5 };
   static TA_Real ap[WT_NB_BAR], esa[WT_NB_BAR], dev[WT_NB_BAR];
   static TA_Real d[WT_NB_BAR], ci[WT_NB_BAR], w1[WT_NB_BAR], w2[WT_NB_BAR];
   static TA_Real mine1[WT_NB_BAR], mine2[WT_NB_BAR];
   TA_RetCode rc;
   TA_Integer bA, nA, bE, nE, bD, nD, b1, n1, b2, n2, bW, nW;
   int set, u, j, k, a, startIdx, LE1, LE2, Lg, total, fixCount;

   fixCount = 0;

   for( set = 0; set < 4; set++ )
   {
      for( u = 0; u < 3; u++ )
      {
         if( TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, uSet[u] ) != TA_SUCCESS )
         {
            printf( "Fail: TA_WAVETREND compose could not set the EMA unstable period\n" );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         LE1 = TA_EMA_Lookback( n1Set[set] );
         LE2 = TA_EMA_Lookback( n2Set[set] );
         Lg  = TA_SMA_Lookback( n3Set[set] );
         total = TA_WAVETREND_Lookback( n1Set[set], n2Set[set], n3Set[set] );

         if( total != 2*LE1 + LE2 + Lg )
         {
            printf( "Fail: TA_WAVETREND_Lookback(%d,%d,%d)=%d is not the sum "
                    "of its callees\n",
                    n1Set[set], n2Set[set], n3Set[set], total );
            goto restore_bad;
         }

         for( j = 0; j < 3; j++ )
         {
            startIdx = total + (j == 0 ? 0 : (j == 1 ? 1 : 40));
            if( startIdx > WT_NB_BAR - 2 ) continue;
            /* The chain starts where the function's own warm-up starts.
             * TA_TYPPRICE has no lookback of its own to absorb -- unlike a
             * chain whose first stage is TA_RSI -- so the entry is the WHOLE
             * lookback, not the part past the first stage. */
            a = startIdx - total;

            rc = TA_WAVETREND( startIdx, WT_NB_BAR - 1, history->high,
                               history->low, history->close,
                               n1Set[set], n2Set[set], n3Set[set],
                               &bW, &nW, mine1, mine2 );
            if( rc != TA_SUCCESS ) goto badrc;
            if( bW != startIdx )
            {
               printf( "Fail: TA_WAVETREND compose begIdx=%d, expected %d\n",
                       (int)bW, startIdx );
               goto restore_bad;
            }

            rc = TA_TYPPRICE( a, WT_NB_BAR - 1, history->high, history->low,
                              history->close, &bA, &nA, ap );
            if( rc != TA_SUCCESS ) goto badrc;

            rc = TA_EMA( 0, (int)nA - 1, ap, n1Set[set], &bE, &nE, esa );
            if( rc != TA_SUCCESS ) goto badrc;

            for( k = 0; k < (int)nE; k++ )
            {
               double num = ap[k + bE] - esa[k];
               if( k > 0 && ap[k + bE] == ap[k + bE - 1] && esa[k] == esa[k-1] )
                  fixCount++;
               dev[k] = num < 0.0 ? -num : num;
            }

            rc = TA_EMA( 0, (int)nE - 1, dev, n1Set[set], &bD, &nD, d );
            if( rc != TA_SUCCESS ) goto badrc;

            for( k = 0; k < (int)nD; k++ )
               ci[k] = (ap[k + bD + bE] - esa[k + bD]) / (0.015 * d[k]);

            rc = TA_EMA( 0, (int)nD - 1, ci, n2Set[set], &b1, &n1, w1 );
            if( rc != TA_SUCCESS ) goto badrc;
            rc = TA_SMA( 0, (int)n1 - 1, w1, n3Set[set], &b2, &n2, w2 );
            if( rc != TA_SUCCESS ) goto badrc;

            if( bE != LE1 || bD != LE1 || b1 != LE2 || b2 != Lg
             || (int)n2 != (int)nW )
            {
               printf( "Fail: TA_WAVETREND compose stage offsets %d/%d/%d/%d "
                       "(want %d/%d/%d/%d), %d values against %d\n",
                       (int)bE, (int)bD, (int)b1, (int)b2,
                       LE1, LE1, LE2, Lg, (int)n2, (int)nW );
               goto restore_bad;
            }

            for( k = 0; k < (int)nW; k++ )
            {
               if( mine1[k] != w1[k + Lg] || mine2[k] != w2[k] )
               {
                  printf( "Fail: TA_WAVETREND compose %d/%d/%d unst=%d "
                          "start=%d bar %d: WT1 %.17g vs %.17g, WT2 %.17g vs "
                          "%.17g\n",
                          n1Set[set], n2Set[set], n3Set[set], uSet[u],
                          startIdx, startIdx + k, mine1[k], w1[k + Lg],
                          mine2[k], w2[k] );
                  goto restore_bad;
               }
               g_wtComposeCmp += 2;
            }
         }
      }
   }

   /* The premise. Without this the leg would silently stop testing the
    * guards' absence the day the corpus or the stages change. */
   if( fixCount != 0 )
   {
      printf( "Fail: TA_WAVETREND the fixpoint condition held on %d bar(s) of "
              "the corpus, so the guard-free chain above is not the formula "
              "this function computes and the leg proves nothing\n", fixCount );
      goto restore_bad;
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;

badrc:
   printf( "Fail: TA_WAVETREND compose sub-call rc=%d\n", (int)rc );
restore_bad:
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TESTUTIL_TFRR_BAD_CALCULATION;
}

/* (3) LOOKBACK: the EMA unstable period counts THREE times. */
static ErrorNumber test_wt_lookback( const TA_History *history )
{
   static const int n1Set[] = { 10,  9, 20,  2 };
   static const int n2Set[] = { 21, 12, 40,  1 };
   static const int n3Set[] = {  4,  3,  8,  1 };
   static const int uSet[]  = {  0,  1,  3,  7 };
   static TA_Real w1[WT_NB_BAR], w2[WT_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int set, u, want, got;

   for( u = 0; u < 4; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, uSet[u] );

      for( set = 0; set < 4; set++ )
      {
         want = 2*(n1Set[set] - 1 + uSet[u])
              + (n2Set[set] - 1 + uSet[u])
              + (n3Set[set] - 1);
         got = TA_WAVETREND_Lookback( n1Set[set], n2Set[set], n3Set[set] );
         if( got != want )
         {
            printf( "Fail: TA_WAVETREND_Lookback(%d,%d,%d) = %d at unstable "
                    "%d, expected %d (the EMA term counts three times)\n",
                    n1Set[set], n2Set[set], n3Set[set], got, uSet[u], want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_wtLookbackCmp++;

         rc = TA_WAVETREND( 0, (int)history->nbBars - 1, history->high,
                            history->low, history->close,
                            n1Set[set], n2Set[set], n3Set[set],
                            &begIdx, &nbElement, w1, w2 );
         if( rc != TA_SUCCESS || begIdx != want )
         {
            printf( "Fail: TA_WAVETREND(%d,%d,%d) at unstable %d answered "
                    "rc=%d begIdx=%d, expected %d\n",
                    n1Set[set], n2Set[set], n3Set[set], uSet[u],
                    (int)rc, (int)begIdx, want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
         g_wtLookbackCmp++;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;
}

/* (4) FLAT: the fixpoint guard.
 *
 * This leg cannot be written as frozen values. The guard exists because a
 * DOUBLE-PRECISION exponential average stops moving once its step falls under
 * half an ulp -- it then freezes a few ulps away from the price, and that
 * frozen gap is a real non-zero distance which the naive form divides by its
 * own average, walking the oscillator to +/-1/0.015 = +/-66.67. A
 * high-precision reference never freezes, so no 60-digit table can predict
 * the guarded output. The properties below are pinned instead, each measured
 * against this implementation:
 *
 *   channel 10, 450 flat bars: WT1 leaves the last moving bar at -24.4 and
 *   decays to -6.5e-12 by bar 575, turning geometric between bars 300 and 325
 *   where the average freezes. The naive form is at -66.67 over the same
 *   stretch, so WT_FLAT_SETTLED is thirteen orders of magnitude clear of it.
 *
 *   channel 100 is the control for the OTHER direction: 450 flat bars are not
 *   enough for its much smaller step to fall under half an ulp, so nothing
 *   freezes and the run is ordinary convergence (still 6.3 at bar 575). It is
 *   here so that "every output finite and bounded" is not quietly passing
 *   because the guard fired everywhere.
 */
#define WT_FLAT_SETTLED 1e-9

static ErrorNumber test_wt_flat( const TA_History *history )
{
   static double h[WT_FLAT_NB], l[WT_FLAT_NB], c[WT_FLAT_NB];
   static TA_Real w1[WT_FLAT_NB], w2[WT_FLAT_NB];
   static const int n1Set[] = { 10, 100 };
   static const int n2Set[] = { 21,  40 };
   static const int n3Set[] = {  4,   8 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   double flat, firstFlat;
   int i, k, set, firstFlatIdx;

   for( i = 0; i < WT_FLAT_MOVE; i++ )
   {
      h[i] = history->high[i];
      l[i] = history->low[i];
      c[i] = history->close[i];
   }
   flat = history->close[WT_FLAT_MOVE - 1];
   for( i = WT_FLAT_MOVE; i < WT_FLAT_NB; i++ )
   {
      h[i] = flat;
      l[i] = flat;
      c[i] = flat;
   }

   for( set = 0; set < 2; set++ )
   {
      rc = TA_WAVETREND( 0, WT_FLAT_NB - 1, h, l, c,
                         n1Set[set], n2Set[set], n3Set[set],
                         &begIdx, &nbElement, w1, w2 );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_WAVETREND flat rc=%d (%d/%d/%d)\n",
                 (int)rc, n1Set[set], n2Set[set], n3Set[set] );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* The reference bar is the first OUTPUT bar that is also flat. At a
       * channel period of 100 the lookback (244) runs past the start of the
       * flat run (150), so WT_FLAT_MOVE - begIdx is negative and indexing by
       * it reads outside the buffer -- which the plain build happened to
       * survive and ASan did not.
       */
      firstFlatIdx = WT_FLAT_MOVE - (int)begIdx;
      if( firstFlatIdx < 0 )
         firstFlatIdx = 0;
      firstFlat = fabs( w1[firstFlatIdx] );

      for( k = 0; k < (int)nbElement; k++ )
      {
         /* Finite everywhere. The chain this replaces emits NaN here once the
          * flat prefix is long enough to zero the deviation average, and
          * carries it into every later bar. */
         if( !isfinite( w1[k] ) || !isfinite( w2[k] ) )
         {
            printf( "Fail: TA_WAVETREND flat bar %d (%d/%d/%d): WT1 %.17g "
                    "WT2 %.17g\n", (int)begIdx + k, n1Set[set], n2Set[set],
                    n3Set[set], w1[k], w2[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         /* Never larger than where the flat run started. The naive form grows
          * to 66.67 from well under it, so this is the assertion that fails
          * loudly without the guard. */
         if( k >= firstFlatIdx && fabs( w1[k] ) > firstFlat )
         {
            printf( "Fail: TA_WAVETREND flat bar %d (%d/%d/%d): |WT1| %.17g "
                    "rose above its value at the first flat bar, %.17g\n",
                    (int)begIdx + k, n1Set[set], n2Set[set], n3Set[set],
                    fabs( w1[k] ), firstFlat );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_wtFlatCmp++;
      }

      /* Channel 10 freezes inside this run and must have settled; channel 100
       * does not, and asserting it had would be asserting the wrong thing. */
      if( set == 0 )
      {
         double last = fabs( w1[nbElement - 1] );
         if( !( last < WT_FLAT_SETTLED ) )
         {
            printf( "Fail: TA_WAVETREND flat run at channel %d ended at |WT1| "
                    "%.17g, expected below %.1e (the naive form sits at "
                    "66.67 here)\n", n1Set[set], last, WT_FLAT_SETTLED );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}


/* (5b) SUBNORMAL: the divisor guard, and the reason it tests the PRODUCT.
 *
 * `0.015 * d` underflows to zero while d itself is still positive, so a guard
 * written `if( d > 0.0 )` divides by zero -- the lesson of #395. That is not a
 * theoretical distinction: MEASURED on the series below, built from the
 * smallest subnormal, this implementation answers 79 finite zeros and the
 * `d > 0.0` form answers 79 non-finite values, every one of them NaN out of a
 * successful call, which #112 forbids.
 *
 * The prices here are not a market; they are the smallest inputs the type can
 * carry, which is where the two spellings part company.
 */
#define WT_SUB_NB 120

static ErrorNumber test_wt_subnormal( void )
{
   static double h[WT_SUB_NB], l[WT_SUB_NB], c[WT_SUB_NB];
   static TA_Real w1[WT_SUB_NB], w2[WT_SUB_NB];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   double tiny, zig;
   int i;

   tiny = nextafter( 0.0, 1.0 );
   for( i = 0; i < WT_SUB_NB; i++ )
   {
      zig = (double)((i % 7) - 3);
      c[i] = tiny * (10.0 + zig);
      h[i] = c[i] + tiny;
      l[i] = c[i] - tiny;
   }

   rc = TA_WAVETREND( 0, WT_SUB_NB - 1, h, l, c, 10, 21, 4,
                      &begIdx, &nbElement, w1, w2 );
   if( rc != TA_SUCCESS || begIdx != 41 )
   {
      printf( "Fail: TA_WAVETREND subnormal rc=%d begIdx=%d\n",
              (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < (int)nbElement; i++ )
   {
      if( !isfinite( w1[i] ) || !isfinite( w2[i] ) )
      {
         printf( "Fail: TA_WAVETREND subnormal bar %d: WT1 %.17g WT2 %.17g -- "
                 "the divisor guard has to test 0.015*d, not d\n",
                 (int)begIdx + i, w1[i], w2[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      /* Exactly zero, not merely finite: every distance underflows, so the
       * oscillator is at its neutral point on every bar. */
      if( w1[i] != 0.0 || w2[i] != 0.0 )
      {
         printf( "Fail: TA_WAVETREND subnormal bar %d: WT1 %.17g WT2 %.17g, "
                 "expected exactly 0\n", (int)begIdx + i, w1[i], w2[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_wtSubnormalCmp += 2;
   }

   return TA_TEST_PASS;
}

/* (5) Either output may be any of the three inputs. */
static ErrorNumber test_wt_aliasing( const TA_History *history )
{
   const TA_Real *src[3];
   static const char * const name[3] = { "inHigh", "inLow", "inClose" };
   static TA_Real r1[WT_NB_BAR], r2[WT_NB_BAR];
   static TA_Real work[WT_NB_BAR], other[WT_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int which, slot, i, nb;

   nb = (int)history->nbBars;

   rc = TA_WAVETREND( 0, nb-1, history->high, history->low, history->close,
                      10, 21, 4, &begIdx, &nbElement, r1, r2 );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_WAVETREND aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   src[0] = history->high;
   src[1] = history->low;
   src[2] = history->close;

   for( which = 0; which < 3; which++ )
   {
      for( slot = 0; slot < 2; slot++ )
      {
         for( i = 0; i < nb; i++ )
            work[i] = src[which][i];

         rc = TA_WAVETREND( 0, nb-1,
                            which == 0 ? work : history->high,
                            which == 1 ? work : history->low,
                            which == 2 ? work : history->close,
                            10, 21, 4, &begIdx2, &nbElement2,
                            slot == 0 ? work : other,
                            slot == 0 ? other : work );
         if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "Fail: TA_WAVETREND with outWT%d aliased onto %s answered "
                    "rc=%d\n", slot + 1, name[which], (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         for( i = 0; i < (int)nbElement; i++ )
         {
            double gotW1 = slot == 0 ? work[i] : other[i];
            double gotW2 = slot == 0 ? other[i] : work[i];
            if( gotW1 != r1[i] || gotW2 != r2[i] )
            {
               printf( "Fail: TA_WAVETREND with outWT%d aliased onto %s "
                       "differs at bar %d: %.17g/%.17g vs %.17g/%.17g\n",
                       slot + 1, name[which], (int)begIdx + i,
                       gotW1, gotW2, r1[i], r2[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         g_wtAliasCmp++;
      }
   }

   return TA_TEST_PASS;
}

static TA_RetCode wtRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                       TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                       TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                       TA_Integer *lookback, void *opaqueData,
                                       unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   TA_RetCode rc;
   static TA_Real buf1[WT_NB_BAR], buf2[WT_NB_BAR];
   int i;

   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_WAVETREND_Lookback( 10, 21, 4 );
   rc = TA_WAVETREND( startIdx, endIdx, h->high, h->low, h->close,
                      10, 21, 4, outBegIdx, outNbElement, buf1, buf2 );
   if( rc != TA_SUCCESS ) return rc;

   for( i = 0; i < (int)(*outNbElement); i++ )
      outputBuffer[i] = outputNb == 0 ? buf1[i] : buf2[i];

   return TA_SUCCESS;
}

/* (6) Range sweep. Three exponential stages, all carrying the one id. */
static ErrorNumber test_wt_range( const TA_History *history )
{
   return doRangeTestEx( wtRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_EMA,
                         (void *)history, 2, 0 );
}
