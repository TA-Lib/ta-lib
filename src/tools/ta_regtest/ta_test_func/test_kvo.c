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
 *  100726 KL,CC  First version (proposal KVO, #484).
 */

/* Description:
 *
 *   Test TA_KVO (Klinger Volume Oscillator, #484).
 *
 *   KVO is not a chain of shipped calls, so there is no composition gate to
 *   borrow. Two ANALYTIC identities stand in its place, and both are worth
 *   more than a golden table because neither depends on a reference value:
 *
 *     fast == slow  ->  the two averages are the same series and KVO is
 *                       EXACTLY zero on every bar, and so is the trigger.
 *     fast <-> slow ->  the output is negated BIT FOR BIT, trigger included:
 *                       a - b = -(b - a) is exact in IEEE, and the trigger's
 *                       recursion commutes with negation.
 *
 *   Together they pin the ANTISYMMETRY of KVO = EMA(vf, fast) - EMA(vf, slow)
 *   and the trigger's dependence on it. MEASURED on this corpus: 232 of 232
 *   bars exactly zero, and 197 of 197 bars exactly negated.
 *
 *   What they do NOT pin, measured rather than assumed: swapping the two
 *   smoothing constants inside the body negates every output, and a negated
 *   implementation satisfies both identities. The golden leg is what catches
 *   that (bar 55 reads +14085008.025879472 against the expected
 *   -14085008.025879476). The identities say what structure the two averages
 *   form, not which of them is the fast one.
 *
 *   The TIE rule is the single line most likely to be wrong and least likely
 *   to be noticed. MEASURED: the committed corpus holds exactly ONE tie in
 *   H+L+C (bar 187), so this file does not rely on the corpus alone -- but
 *   that one tie does carry weight: reading it as +1, or as a reversal, moves
 *   KVO by up to 1.0e7 from the next bar on. A built series forces ties where
 *   they also change the cumulative range.
 *
 *   The GOLDEN leg holds the 1997 factor, from a 60-digit evaluation over the
 *   committed corpus. MEASURED: the library reproduces those 28 rows to
 *   5.4e-15 by |a-b|/max(|b|,1); the tolerance is 1e-12.
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
#include "test_codegen.h"

/**** Local declarations.    ****/

#define KVO_NB_BAR 252

/* 4 parameter sets x 7 bars x 2 outputs. */
#define KVO_GOLDEN_CMP     56
/* 20/20/7 over a 252-bar corpus: 252 - (1 + 19) outputs, two each. */
#define KVO_ZERO_CMP      464
/* 34/55/13: 252 - 55 outputs, two each. */
#define KVO_NEGATE_CMP    394
/* The built tie series is 72 bars with a lookback of 1 + ema_lookback(11) =
 * 11, so 61 outputs; the leg compares 8 frozen rows and both outputs of every
 * bar for finiteness: 8*2 + 61*2. */
#define KVO_TIE_CMP       138
/* 4 sets x 4 unstable periods, _Lookback and the call's own begIdx. */
#define KVO_LOOKBACK_CMP   32
/* Each output aliased onto each of the four price inputs, two outputs
 * checked per call. */
#define KVO_ALIAS_CMP      16

static int g_kvoGoldenCmp;
static int g_kvoZeroCmp;
static int g_kvoNegateCmp;
static int g_kvoTieCmp;
static int g_kvoLookbackCmp;
static int g_kvoAliasCmp;

typedef struct
{
   int    fast;
   int    slow;
   int    signal;
   int    bar;
   double kvo;
   double sig;
} KvoGolden;

/* From a 60-digit evaluation of the #484 form A factor |2*(dm/cm) - 1| over
 * the committed corpus, rounded once to 17 significant digits. The 2/2/2 set
 * is the minimum-period edge, where fast == slow makes every value an exact
 * zero -- kept here as a row so the golden leg sees it too, not only the
 * identity leg.
 */
static const KvoGolden kvoGolden[] =
{
   { 34, 55, 13,  55, -14085008.025879476,  -8966101.2141860742 },
   { 34, 55, 13,  56,  -9120998.970013978,  -8988229.4650186319 },
   { 34, 55, 13,  57,  -4966175.7100215768, -8413650.3571619093 },
   { 34, 55, 13,  65,  13509357.43638581,    2582426.3576765927 },
   { 34, 55, 13, 100,  -6269892.8696958618,  6444021.696940396  },
   { 34, 55, 13, 180,  -1378884.0822162263,  -671951.10126817168 },
   { 34, 55, 13, 251,   9424397.1070166472, 13608043.287732257 },
   { 10, 20,  5,  20, -40840318.848738924, -32157100.182964899 },
   { 10, 20,  5,  21, -62040739.314577028, -42118313.226835608 },
   { 10, 20,  5,  22, -70808619.00152941,  -51681748.485066876 },
   { 10, 20,  5,  30,  -6570297.7432931187, -31618114.124490514 },
   { 10, 20,  5, 100, -46333268.300248899, -41743796.014934063 },
   { 10, 20,  5, 180, -15754718.974364594,   -745977.70484030596 },
   { 10, 20,  5, 251, -12384408.756199948,  -2364062.0247912519 },
   {  2,  2,  2,   2,          0.0,                 0.0 },
   {  2,  2,  2,   3,          0.0,                 0.0 },
   {  2,  2,  2,   4,          0.0,                 0.0 },
   {  2,  2,  2,  12,          0.0,                 0.0 },
   {  2,  2,  2, 100,          0.0,                 0.0 },
   {  2,  2,  2, 180,          0.0,                 0.0 },
   {  2,  2,  2, 251,          0.0,                 0.0 },
   { 55, 34, 13,  55,  14085008.025879476,   8966101.2141860742 },
   { 55, 34, 13,  56,   9120998.970013978,   8988229.4650186319 },
   { 55, 34, 13,  57,   4966175.7100215768,  8413650.3571619093 },
   { 55, 34, 13,  65, -13509357.43638581,   -2582426.3576765927 },
   { 55, 34, 13, 100,   6269892.8696958618, -6444021.696940396  },
   { 55, 34, 13, 180,   1378884.0822162263,   671951.10126817168 },
   { 55, 34, 13, 251,  -9424397.1070166472,-13608043.287732257 }
};

#define KVO_NB_GOLDEN ((int)(sizeof(kvoGolden)/sizeof(kvoGolden[0])))

/* MEASURED: 5.371e-15 by |a-b|/max(|b|,1) over the rows above. */
#define KVO_GOLDEN_TOL 1e-12

static ErrorNumber test_kvo_golden  ( const TA_History *history );
static ErrorNumber test_kvo_zero    ( const TA_History *history );
static ErrorNumber test_kvo_negate  ( const TA_History *history );
static ErrorNumber test_kvo_tie     ( void );
static ErrorNumber test_kvo_lookback( const TA_History *history );
static ErrorNumber test_kvo_aliasing( const TA_History *history );
static ErrorNumber test_kvo_range   ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_kvo( TA_History *history )
{
   ErrorNumber retValue;

   g_kvoGoldenCmp = 0;
   g_kvoZeroCmp = 0;
   g_kvoNegateCmp = 0;
   g_kvoTieCmp = 0;
   g_kvoLookbackCmp = 0;
   g_kvoAliasCmp = 0;

   if( history->nbBars != KVO_NB_BAR )
   {
      printf( "Fail: TA_KVO expects the %d-bar corpus, got %d\n",
              KVO_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   retValue = test_kvo_golden( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_kvo_zero( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_kvo_negate( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_kvo_tie();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_kvo_lookback( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_kvo_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_kvo_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   if( g_kvoGoldenCmp   != KVO_GOLDEN_CMP
    || g_kvoZeroCmp     != KVO_ZERO_CMP
    || g_kvoNegateCmp   != KVO_NEGATE_CMP
    || g_kvoTieCmp      != KVO_TIE_CMP
    || g_kvoLookbackCmp != KVO_LOOKBACK_CMP
    || g_kvoAliasCmp    != KVO_ALIAS_CMP )
   {
      printf( "Fail: TA_KVO comparison counts (golden %d, zero %d, negate %d, "
              "tie %d, lookback %d, alias %d) are not what this file asserts "
              "(%d, %d, %d, %d, %d, %d)\n",
              g_kvoGoldenCmp, g_kvoZeroCmp, g_kvoNegateCmp, g_kvoTieCmp,
              g_kvoLookbackCmp, g_kvoAliasCmp,
              KVO_GOLDEN_CMP, KVO_ZERO_CMP, KVO_NEGATE_CMP, KVO_TIE_CMP,
              KVO_LOOKBACK_CMP, KVO_ALIAS_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* (1) GOLDEN: the 1997 factor, from 60 digits over the committed corpus. */
static ErrorNumber test_kvo_golden( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   static TA_Real k1[KVO_NB_BAR], k2[KVO_NB_BAR];
   int i, lastFast = -1, lastSlow = -1;

   for( i = 0; i < KVO_NB_GOLDEN; i++ )
   {
      const KvoGolden *g = &kvoGolden[i];
      int longest = g->fast;
      int want, which;

      if( g->slow > longest ) longest = g->slow;
      if( g->signal > longest ) longest = g->signal;
      want = 1 + (longest - 1);

      rc = TA_KVO( 0, (int)history->nbBars - 1,
                   history->high, history->low, history->close,
                   history->volume, g->fast, g->slow, g->signal,
                   &begIdx, &nbElement, k1, k2 );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_KVO golden rc=%d (%d/%d/%d)\n",
                 (int)rc, g->fast, g->slow, g->signal );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != want || g->bar < begIdx )
      {
         printf( "Fail: TA_KVO golden range: begIdx=%d (want %d), bar %d\n",
                 (int)begIdx, want, g->bar );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      for( which = 0; which < 2; which++ )
      {
         double got = which == 0 ? k1[g->bar - begIdx] : k2[g->bar - begIdx];
         double ref = which == 0 ? g->kvo : g->sig;
         double err;

         /* The fast == slow rows are an EXACT zero, not a small number: a
          * tolerance there would accept any implementation that merely gets
          * close to cancelling. */
         if( ref == 0.0 )
         {
            if( got != 0.0 )
            {
               printf( "Fail: TA_KVO golden bar %d (%d/%d/%d) out%d: %.17g, "
                       "expected exactly 0\n", g->bar, g->fast, g->slow,
                       g->signal, which + 1, got );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         else
         {
            err = fabs( got - ref ) / (fabs( ref ) > 1.0 ? fabs( ref ) : 1.0);
            if( !( err <= KVO_GOLDEN_TOL ) )
            {
               printf( "Fail: TA_KVO golden bar %d (%d/%d/%d) out%d: %.17g, "
                       "expected %.17g (rel %.3g, tol %.1e)\n",
                       g->bar, g->fast, g->slow, g->signal, which + 1,
                       got, ref, err, KVO_GOLDEN_TOL );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         g_kvoGoldenCmp++;
      }

      if( (g->fast != lastFast || g->slow != lastSlow) && server_verify_active() )
      {
         const double opt[3] = { (double)g->fast, (double)g->slow,
                                 (double)g->signal };
         ErrorNumber e;

         e = server_verify( "KVO", 0, (int)history->nbBars - 1,
                            (int)history->nbBars,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ history->high, history->low,
                                                history->close,
                                                history->volume, NULL },
                            opt, 3,
                            (const TA_Real*[]){ k1, k2, NULL }, NULL );
         if( e != TA_TEST_PASS ) return e;
         lastFast = g->fast;
         lastSlow = g->slow;
      }
   }

   return TA_TEST_PASS;
}

/* (2) IDENTITY, fast == slow: the two averages are the same series, so the
 * oscillator cancels exactly and the trigger, seeded on that zero, never
 * leaves it. No reference value is involved, so this holds whatever the
 * factor or the tie rule do. */
static ErrorNumber test_kvo_zero( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   static TA_Real k1[KVO_NB_BAR], k2[KVO_NB_BAR];
   int i;

   rc = TA_KVO( 0, (int)history->nbBars - 1,
                history->high, history->low, history->close, history->volume,
                20, 20, 7, &begIdx, &nbElement, k1, k2 );
   if( rc != TA_SUCCESS || begIdx != 20 )
   {
      printf( "Fail: TA_KVO 20/20/7 rc=%d begIdx=%d\n", (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < (int)nbElement; i++ )
   {
      if( k1[i] != 0.0 || k2[i] != 0.0 )
      {
         printf( "Fail: TA_KVO at fast == slow, bar %d: KVO %.17g SIG %.17g, "
                 "expected exactly 0 -- the two averages are the same series\n",
                 (int)begIdx + i, k1[i], k2[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_kvoZeroCmp += 2;
   }

   return TA_TEST_PASS;
}

/* (3) IDENTITY, fast <-> slow: the output is negated bit for bit.
 *
 * a - b = -(b - a) is exact in IEEE, and the trigger's recursion commutes
 * with negation because rounding to nearest is symmetric. No reference value
 * is involved, so this holds whatever the factor or the tie rule do -- which
 * is also its limit: an implementation whose output is uniformly negated
 * satisfies it too, and only the golden leg sees that. */
static ErrorNumber test_kvo_negate( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer bA, nA, bB, nB;
   static TA_Real a1[KVO_NB_BAR], a2[KVO_NB_BAR];
   static TA_Real b1[KVO_NB_BAR], b2[KVO_NB_BAR];
   int i;

   rc = TA_KVO( 0, (int)history->nbBars - 1,
                history->high, history->low, history->close, history->volume,
                34, 55, 13, &bA, &nA, a1, a2 );
   if( rc != TA_SUCCESS ) goto badrc;
   rc = TA_KVO( 0, (int)history->nbBars - 1,
                history->high, history->low, history->close, history->volume,
                55, 34, 13, &bB, &nB, b1, b2 );
   if( rc != TA_SUCCESS ) goto badrc;

   if( bA != bB || nA != nB )
   {
      printf( "Fail: TA_KVO 34/55 and 55/34 answered different ranges: "
              "%d/%d against %d/%d\n", (int)bA, (int)nA, (int)bB, (int)nB );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   for( i = 0; i < (int)nA; i++ )
   {
      if( a1[i] != -b1[i] || a2[i] != -b2[i] )
      {
         printf( "Fail: TA_KVO bar %d is not the exact negation under a "
                 "fast/slow swap: %.17g against %.17g, SIG %.17g against "
                 "%.17g\n", (int)bA + i, a1[i], b1[i], a2[i], b2[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_kvoNegateCmp += 2;
   }

   return TA_TEST_PASS;

badrc:
   printf( "Fail: TA_KVO negation leg rc=%d\n", (int)rc );
   return TA_TESTUTIL_TFRR_BAD_RETCODE;
}

/* (4) TIE: the article's tenet that the existing trend is maintained when the
 * high-low-close sum repeats.
 *
 * The committed corpus holds ONE such bar, so this leg builds a series that
 * forces them: every third bar repeats the sum while carrying a DIFFERENT
 * range, so a mis-read tie changes the trend sign and resets the cumulative
 * range at the same time. The tie count is asserted -- a series that stopped
 * containing ties would leave this leg green and empty.
 *
 * The prices are exact decimal values, so the frozen rows below carry no
 * dependence on the platform's libm.
 */
#define KVO_TIE_NB   72
#define KVO_TIE_TIES 24

static const KvoGolden kvoTieGolden[] =
{
   { 5, 11, 3, 11,  -7824.355571948191,  -3320.619635820387 },
   { 5, 11, 3, 12,   3038.5382414931805,  -141.04069716360337 },
   { 5, 11, 3, 13,   7328.7456429517288,  3593.8524728940624 },
   { 5, 11, 3, 14,  12026.20568476207,    7810.0290788280663 },
   { 5, 11, 3, 15,  -2094.1215919570627,  2857.9537434355016 },
   { 5, 11, 3, 40,  14134.09093145475,    8329.1473707604655 },
   { 5, 11, 3, 70,   7099.0378161020908,  3318.4518638941504 },
   { 5, 11, 3, 71,  10038.3129888146,     6678.3824263543747 }
};

#define KVO_NB_TIE_GOLDEN ((int)(sizeof(kvoTieGolden)/sizeof(kvoTieGolden[0])))

static ErrorNumber test_kvo_tie( void )
{
   static double h[KVO_TIE_NB], l[KVO_TIE_NB], c[KVO_TIE_NB], v[KVO_TIE_NB];
   static TA_Real k1[KVO_TIE_NB], k2[KVO_TIE_NB];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   double base, half, drift;
   int i, ties;

   for( i = 0; i < KVO_TIE_NB; i++ )
   {
      base = 300.0;
      half = 1.0 + (double)(i % 5);
      drift = ((i / 12) % 2) ? 3.0 : -3.0;
      if( i % 3 == 0 )
      {
         h[i] = base + half;
         l[i] = base - half;
         c[i] = base;
      }
      else
      {
         h[i] = base + half + drift;
         l[i] = base - half + drift;
         c[i] = base + drift;
      }
      v[i] = 1000.0 + 17.0 * (double)(i % 7);
   }

   ties = 0;
   for( i = 1; i < KVO_TIE_NB; i++ )
      if( (h[i]+l[i]+c[i]) == (h[i-1]+l[i-1]+c[i-1]) )
         ties++;
   if( ties != KVO_TIE_TIES )
   {
      printf( "Fail: TA_KVO tie series carries %d ties, not the %d this leg "
              "is built on -- without them it tests nothing\n",
              ties, KVO_TIE_TIES );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   rc = TA_KVO( 0, KVO_TIE_NB - 1, h, l, c, v, 5, 11, 3,
                &begIdx, &nbElement, k1, k2 );
   if( rc != TA_SUCCESS || begIdx != 11 )
   {
      printf( "Fail: TA_KVO tie series rc=%d begIdx=%d\n",
              (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < (int)nbElement; i++ )
   {
      if( !isfinite( k1[i] ) || !isfinite( k2[i] ) )
      {
         printf( "Fail: TA_KVO tie series bar %d: KVO %.17g SIG %.17g\n",
                 (int)begIdx + i, k1[i], k2[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_kvoTieCmp += 2;
   }

   for( i = 0; i < KVO_NB_TIE_GOLDEN; i++ )
   {
      const KvoGolden *g = &kvoTieGolden[i];
      int which;

      for( which = 0; which < 2; which++ )
      {
         double got = which == 0 ? k1[g->bar - begIdx] : k2[g->bar - begIdx];
         double ref = which == 0 ? g->kvo : g->sig;
         double err = fabs( got - ref )
                    / (fabs( ref ) > 1.0 ? fabs( ref ) : 1.0);
         if( !( err <= KVO_GOLDEN_TOL ) )
         {
            printf( "Fail: TA_KVO tie series bar %d out%d: %.17g, expected "
                    "%.17g (rel %.3g, tol %.1e)\n",
                    g->bar, which + 1, got, ref, err, KVO_GOLDEN_TOL );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_kvoTieCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (5) LOOKBACK: one EMA term, not three -- the smoothings share an anchor --
 * plus the reference bar. */
static ErrorNumber test_kvo_lookback( const TA_History *history )
{
   static const int fSet[] = { 34, 10,  2, 55 };
   static const int sSet[] = { 55, 20,  2, 34 };
   static const int gSet[] = { 13,  5,  2, 13 };
   static const int uSet[] = {  0,  1,  3,  7 };
   static TA_Real k1[KVO_NB_BAR], k2[KVO_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int set, u, want, got, longest;

   for( u = 0; u < 4; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, uSet[u] );

      for( set = 0; set < 4; set++ )
      {
         longest = fSet[set];
         if( sSet[set] > longest ) longest = sSet[set];
         if( gSet[set] > longest ) longest = gSet[set];
         want = 1 + (longest - 1 + uSet[u]);

         got = TA_KVO_Lookback( fSet[set], sSet[set], gSet[set] );
         if( got != want )
         {
            printf( "Fail: TA_KVO_Lookback(%d,%d,%d) = %d at unstable %d, "
                    "expected %d (one EMA term, not three)\n",
                    fSet[set], sSet[set], gSet[set], got, uSet[u], want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_kvoLookbackCmp++;

         rc = TA_KVO( 0, (int)history->nbBars - 1, history->high,
                      history->low, history->close, history->volume,
                      fSet[set], sSet[set], gSet[set],
                      &begIdx, &nbElement, k1, k2 );
         if( rc != TA_SUCCESS || begIdx != want )
         {
            printf( "Fail: TA_KVO(%d,%d,%d) at unstable %d answered rc=%d "
                    "begIdx=%d, expected %d\n",
                    fSet[set], sSet[set], gSet[set], uSet[u],
                    (int)rc, (int)begIdx, want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
         g_kvoLookbackCmp++;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;
}

/* (6) Either output may be any of the four inputs. */
static ErrorNumber test_kvo_aliasing( const TA_History *history )
{
   const TA_Real *src[4];
   static const char * const name[4] = { "inHigh", "inLow", "inClose", "inVolume" };
   static TA_Real r1[KVO_NB_BAR], r2[KVO_NB_BAR];
   static TA_Real work[KVO_NB_BAR], other[KVO_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int which, slot, i, nb;

   nb = (int)history->nbBars;

   rc = TA_KVO( 0, nb-1, history->high, history->low, history->close,
                history->volume, 34, 55, 13, &begIdx, &nbElement, r1, r2 );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_KVO aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   src[0] = history->high;
   src[1] = history->low;
   src[2] = history->close;
   src[3] = history->volume;

   for( which = 0; which < 4; which++ )
   for( slot = 0; slot < 2; slot++ )
   {
      for( i = 0; i < nb; i++ )
         work[i] = src[which][i];

      rc = TA_KVO( 0, nb-1,
                   which == 0 ? work : history->high,
                   which == 1 ? work : history->low,
                   which == 2 ? work : history->close,
                   which == 3 ? work : history->volume,
                   34, 55, 13, &begIdx2, &nbElement2,
                   slot == 0 ? work : other,
                   slot == 0 ? other : work );
      if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "Fail: TA_KVO with outKVO%s aliased onto %s answered rc=%d\n",
                 slot == 0 ? "" : "Signal", name[which], (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < (int)nbElement; i++ )
      {
         double gotK = slot == 0 ? work[i] : other[i];
         double gotS = slot == 0 ? other[i] : work[i];
         if( gotK != r1[i] || gotS != r2[i] )
         {
            printf( "Fail: TA_KVO aliased onto %s differs at bar %d: "
                    "%.17g/%.17g against %.17g/%.17g\n",
                    name[which], (int)begIdx + i, gotK, gotS, r1[i], r2[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      g_kvoAliasCmp += 2;
   }

   return TA_TEST_PASS;
}

static TA_RetCode kvoRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   TA_RetCode rc;
   static TA_Real buf1[KVO_NB_BAR], buf2[KVO_NB_BAR];
   int i;

   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_KVO_Lookback( 34, 55, 13 );
   rc = TA_KVO( startIdx, endIdx, h->high, h->low, h->close, h->volume,
                34, 55, 13, outBegIdx, outNbElement, buf1, buf2 );
   if( rc != TA_SUCCESS ) return rc;

   for( i = 0; i < (int)(*outNbElement); i++ )
      outputBuffer[i] = outputNb == 0 ? buf1[i] : buf2[i];

   return TA_SUCCESS;
}

/* (7) Range sweep. KVO converges: the cumulative range two starts carry
 * differs only until the first trend change after both, which resets it for
 * both alike, and what remains is the EMA seed error, fading at the slow
 * average's rate (54/56 per bar at the defaults). It is a small difference of
 * two large averages -- on this corpus mean |KVO| is 7% of either -- so that
 * error starts at ~14x the output and outlives the envelope's 100-bar grace.
 * The grace this sweep needs is the one the codegen sweep reads, keyed on the
 * name in test_codegen.c (MEASURED there: 160 fails, 175 holds, 197 and up
 * compares nothing on 252 bars). */
static ErrorNumber test_kvo_range( const TA_History *history )
{
   static const TA_FuncUnstId id = TA_FUNC_UNST_EMA;
   return doRangeTestMulti( kvoRangeTestFunction,
                            TA_STABLE_CONVERGING, &id, 1,
                            (void *)history, 2, 0,
                            codegen_range_grace( "KVO" ) );
}
