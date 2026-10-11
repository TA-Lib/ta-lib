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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  100726 KL,CC First version (proposal ICHIMOKU, #490).
 *
 */

/* Description:
 *
 *   Test TA_ICHIMOKU (Goichi Hosoda's Ichimoku Kinko Hyo, #490).
 *
 *   ICHIMOKU publishes no number of its own: three of its four outputs are
 *   TA_MIDPRICE over three different windows, and the fourth is the mean of
 *   two of them. So the gate is not a transcribed golden table -- it is
 *   COMPOSITION, bit for bit, against the library's own TA_MIDPRICE. That
 *   is stronger than a table at a tolerance and it cannot go stale: if
 *   midprice.c ever changes how it spells a midpoint, this file fails until
 *   ichimoku.c follows.
 *
 *   The extrema are selections, so the comparison is bit-exact whichever
 *   scan each side uses -- ichimoku.c currently carries the cached-extreme
 *   plus rescan idiom and midprice.c the block scan (#490 Q6 is which one
 *   ICHIMOKU should keep, a cost question, not a value one). Every leg here
 *   holds under either, which is why this file was written before that
 *   ruling rather than after it.
 *
 *   Legs:
 *     1. COMPOSITION, six parameter triples over the committed corpus. Each
 *        line is memcmp'd against TA_MIDPRICE at its own period, and Span A
 *        against (tenkan + kijun)/2.0 computed from the outputs themselves.
 *        Also the leg that drives server_verify.
 *     2. SPAN A IS TWO HALVINGS, NOT ONE QUARTERING. In real arithmetic
 *        (midT + midK)/2 and (hiT + loT + hiK + loK)/4 are the same number;
 *        in binary64 they are not. How often they differ was measured, not
 *        assumed: on the committed corpus it is 0 of 227 bars at the
 *        published 9/26 -- a reader who only ever runs the defaults sees
 *        nothing -- and 25 of 248 at 3/5, 31 of 251 at 2/2. So leg 1 does
 *        discriminate, but only on the triples it happens to carry. This
 *        leg removes that dependency: a four-decimal series on which the
 *        two spellings disagree on 30 of 63 bars, scanned here rather than
 *        read back from the function's own outputs, with the disagreements
 *        COUNTED. If a future edit made the series stop discriminating the
 *        leg says so instead of passing green while testing nothing.
 *     3. LOOKBACK. max(tenkan, kijun, senkouB) - 1, asserted on triples
 *        where each of the three is the longest in turn -- the ordering
 *        the published defaults (9 < 26 < 52) never exercises and that a
 *        "senkouB is the longest" simplification would get wrong.
 *     4. THE THREE WINDOWS ARE SEPARATE. Swapping the tenkan and senkouB
 *        periods swaps those two outputs bit for bit and leaves kijun
 *        alone; Span A, which reads tenkan, must NOT come back equal. The
 *        inequality is the control arm: without it the leg would also pass
 *        against a function that ignored its parameters.
 *     5. FLAT WINDOW. high = low = c on every bar: all four outputs are
 *        exactly c, including Span A, whose two halvings of c are c.
 *     6. DISPLAY SHIFT. The two leading spans are drawn kijun bars into the
 *        future and the two lines are drawn where they were computed, so
 *        the shift is +kijun for outputs 2 and 3 and 0 for 0 and 1 (rL9).
 *        It is metadata: the VALUES are written at the bar that computed
 *        them, which leg 1 is what proves.
 *     7. ALIASING. Each of the four output buffers over each input.
 *     8. RANGE INDEPENDENCE, swept via doRangeTestEx. TA_STABLE_EXACT: a
 *        window is rebuilt from its own bars, so no earlier bar can leak
 *        into a later one and a short range answers what a long one does.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define ICH_NB_BAR  252
#define ICH_SPANA_NB 69

/* Leg 1: for each triple, (252 - (longest - 1)) bars x 4 outputs.
 *   9/26/52 -> 201*4   3/5/7   -> 246*4   26/9/52  -> 201*4
 *  52/26/9  -> 201*4   2/2/2   -> 251*4   40/100/30 -> 153*4
 */
#define ICH_COMPOSE_CMP 5012
/* Leg 2: 63 output bars, plus the one count of how many of them
 * discriminate the two spellings. */
#define ICH_SPANA_CMP     64
/* 30 on x86-64 and on every platform that evaluates binary64 in binary64.
 * Held as a floor rather than an equality so that a host with a wider
 * evaluation format reports a weaker series rather than a false failure --
 * but a floor this far above zero still fails loudly if the series goes
 * flat. */
#define ICH_SPANA_MIN_DISCRIM 20
/* Leg 3: 8 triples, each checking _Lookback and then the call's begIdx. */
#define ICH_LOOKBACK_CMP  16
/* Leg 4: three memcmp equalities plus the one required inequality. */
#define ICH_SWAP_CMP       4
/* Leg 5: 201 bars x 4 outputs at the default triple. */
#define ICH_FLAT_CMP     804
/* Leg 6: 4 triples x 4 outputs. */
#define ICH_SHIFT_CMP     16
/* Leg 7: 4 output buffers x 2 inputs. */
#define ICH_ALIAS_CMP      8

static int g_ichComposeCmp;
static int g_ichSpanACmp;
static int g_ichLookbackCmp;
static int g_ichSwapCmp;
static int g_ichFlatCmp;
static int g_ichShiftCmp;
static int g_ichAliasCmp;

typedef struct { int tenkan; int kijun; int senkouB; } IchTriple;

static const IchTriple ichCompose[] =
{
   {  9,  26,  52 },   /* the published defaults */
   {  3,   5,   7 },   /* short windows, every bar near a window edge */
   { 26,   9,  52 },   /* tenkan longer than kijun */
   { 52,  26,   9 },   /* tenkan longest, senkouB shortest */
   {  2,   2,   2 },   /* the minimum, all three equal */
   { 40, 100,  30 }    /* kijun longest: the lookback's MAX, not senkouB */
};
#define ICH_NB_COMPOSE ((int)(sizeof(ichCompose)/sizeof(ichCompose[0])))

static const IchTriple ichLookback[] =
{
   {   9,  26,  52 },
   {  52,  26,   9 },
   {  26,  52,   9 },
   {   2,   2,   2 },
   { 100,   3,   3 },
   {   3, 100,   3 },
   {   3,   3, 100 },
   {   7,   5,   3 }
};
#define ICH_NB_LOOKBACK ((int)(sizeof(ichLookback)/sizeof(ichLookback[0])))

/* Leg 2's series. Four decimals and a range under a fifth of a point, so a
 * window's two extremes are close enough that halving twice and quartering
 * once land on different doubles. Built by searching random four-decimal
 * walks for the one that discriminates on the most bars at 3/5/7; the
 * values carry no meaning beyond that and the leg verifies the property
 * rather than trusting this comment.
 */
static const double ichSpanAHigh[ICH_SPANA_NB] =
{
   100.0507,100.0588,100.0712,100.0747,100.0355,100.0161,100.0374,100.0740,100.0313,100.0757,
   100.0535,100.0547,100.0349,100.0161,100.0516,100.0330,100.0439,100.0730,100.1163,100.1194,
   100.0387,100.0871,100.0720,100.0978,100.1085,100.1113,100.1113,100.1047,100.0371,100.1129,
   100.1021,100.0617,100.0405,100.1012,100.0792,100.0991,100.1170,100.1312,100.0808,100.1702,
   100.0991,100.1347,100.1393,100.0977,100.0543,100.0303,100.1217,100.0866,100.0887,100.0522,
   100.0357,100.0704,100.0634,100.1145,100.0756,100.0771,100.1314,100.1335,100.1506,100.0999,
   100.1050,100.1015,100.0796,100.0454,100.1215,100.0854,100.1027,100.1081,100.1393
};
static const double ichSpanALow[ICH_SPANA_NB] =
{
    99.9905,100.0037,100.0120,100.0036, 99.9920, 99.9948,100.0031, 99.9912,100.0160,100.0078,
    99.9851, 99.9770,100.0004,100.0075,100.0294,100.0154,100.0197,100.0055,100.0299,100.0297,
   100.0170,100.0180,100.0030,100.0088,100.0335,100.0300,100.0419,100.0390,100.0148,100.0332,
   100.0288,100.0312,100.0129,100.0379,100.0627,100.0679,100.0542,100.0770,100.0797,100.0905,
   100.0827,100.0730,100.0502,100.0274,100.0129,100.0288,100.0459,100.0370,100.0226,100.0322,
   100.0344,100.0435,100.0577,100.0388,100.0574,100.0604,100.0720,100.0739,100.0634,100.0624,
   100.0431,100.0241,100.0338,100.0434,100.0648,100.0791,100.0970,100.0965,100.1112
};

static ErrorNumber test_ich_compose ( const TA_History *history );
static ErrorNumber test_ich_spana   ( void );
static ErrorNumber test_ich_lookback( const TA_History *history );
static ErrorNumber test_ich_swap    ( const TA_History *history );
static ErrorNumber test_ich_flat    ( void );
static ErrorNumber test_ich_shift   ( void );
static ErrorNumber test_ich_aliasing( const TA_History *history );
static ErrorNumber test_ich_range   ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_ichimoku( TA_History *history )
{
   ErrorNumber retValue;

   g_ichComposeCmp = 0;
   g_ichSpanACmp = 0;
   g_ichLookbackCmp = 0;
   g_ichSwapCmp = 0;
   g_ichFlatCmp = 0;
   g_ichShiftCmp = 0;
   g_ichAliasCmp = 0;

   if( history->nbBars != ICH_NB_BAR )
   {
      printf( "Fail: TA_ICHIMOKU expects the %d-bar corpus, got %d\n",
              ICH_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   retValue = test_ich_compose( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_ich_spana();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_ich_lookback( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_ich_swap( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_ich_flat();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_ich_shift();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_ich_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_ich_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   if( g_ichComposeCmp  != ICH_COMPOSE_CMP
    || g_ichSpanACmp    != ICH_SPANA_CMP
    || g_ichLookbackCmp != ICH_LOOKBACK_CMP
    || g_ichSwapCmp     != ICH_SWAP_CMP
    || g_ichFlatCmp     != ICH_FLAT_CMP
    || g_ichShiftCmp    != ICH_SHIFT_CMP
    || g_ichAliasCmp    != ICH_ALIAS_CMP )
   {
      printf( "Fail: TA_ICHIMOKU comparison counts (compose %d, spanA %d, "
              "lookback %d, swap %d, flat %d, shift %d, alias %d) are not what "
              "this file asserts (%d, %d, %d, %d, %d, %d, %d)\n",
              g_ichComposeCmp, g_ichSpanACmp, g_ichLookbackCmp, g_ichSwapCmp,
              g_ichFlatCmp, g_ichShiftCmp, g_ichAliasCmp,
              ICH_COMPOSE_CMP, ICH_SPANA_CMP, ICH_LOOKBACK_CMP, ICH_SWAP_CMP,
              ICH_FLAT_CMP, ICH_SHIFT_CMP, ICH_ALIAS_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.    ****/

static int ichLongest( const IchTriple *p )
{
   int longest = p->tenkan;
   if( p->kijun > longest )
      longest = p->kijun;
   if( p->senkouB > longest )
      longest = p->senkouB;
   return longest;
}

/* The window's two extremes, scanned the slow way on purpose: this file must
 * not share an idiom with the function it is testing. */
static void ichWindow( const double *h, const double *l, int bar, int period,
                       double *hi, double *lo )
{
   int i;

   *hi = h[bar - period + 1];
   *lo = l[bar - period + 1];
   for( i = bar - period + 2; i <= bar; i++ )
   {
      if( h[i] > *hi ) *hi = h[i];
      if( l[i] < *lo ) *lo = l[i];
   }
}

/* (1) Composition against TA_MIDPRICE, bit for bit. */
static ErrorNumber test_ich_compose( const TA_History *history )
{
   static TA_Real outT[ICH_NB_BAR], outK[ICH_NB_BAR];
   static TA_Real outA[ICH_NB_BAR], outB[ICH_NB_BAR];
   static TA_Real midT[ICH_NB_BAR], midK[ICH_NB_BAR], midB[ICH_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   TA_Integer mBegT, mBegK, mBegB, mNb;
   int t, k, nb, longest, offT, offK, offB;

   nb = (int)history->nbBars;

   for( t = 0; t < ICH_NB_COMPOSE; t++ )
   {
      const IchTriple *p = &ichCompose[t];

      longest = ichLongest( p );

      rc = TA_ICHIMOKU( 0, nb-1, history->high, history->low,
                        p->tenkan, p->kijun, p->senkouB,
                        &begIdx, &nbElement, outT, outK, outA, outB );
      if( rc != TA_SUCCESS || begIdx != longest - 1
          || nbElement != nb - (longest - 1) )
      {
         printf( "Fail: TA_ICHIMOKU %d/%d/%d rc=%d range %d/%d (want %d/%d)\n",
                 p->tenkan, p->kijun, p->senkouB, (int)rc, (int)begIdx,
                 (int)nbElement, longest - 1, nb - (longest - 1) );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* Once per triple, send the same call through every language server.
       * Without this the group runs under --codegen having compared nothing
       * against any server, which --codegen reports as a failure.
       */
      if( server_verify_active() )
      {
         const double opt[3] = { (double)p->tenkan, (double)p->kijun,
                                 (double)p->senkouB };
         ErrorNumber e = server_verify( "ICHIMOKU", 0, nb-1, nb,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ history->high, history->low, NULL },
                            opt, 3,
                            (const TA_Real*[]){ outT, outK, outA, outB, NULL },
                            NULL );
         if( e != TA_TEST_PASS )
            return e;
      }

      rc = TA_MIDPRICE( 0, nb-1, history->high, history->low, p->tenkan,
                        &mBegT, &mNb, midT );
      if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
      rc = TA_MIDPRICE( 0, nb-1, history->high, history->low, p->kijun,
                        &mBegK, &mNb, midK );
      if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
      rc = TA_MIDPRICE( 0, nb-1, history->high, history->low, p->senkouB,
                        &mBegB, &mNb, midB );
      if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;

      /* TA_MIDPRICE starts as soon as its own window is full, which is on or
       * before ICHIMOKU's first bar; line them up on the bar, not the index. */
      offT = (int)begIdx - (int)mBegT;
      offK = (int)begIdx - (int)mBegK;
      offB = (int)begIdx - (int)mBegB;

      for( k = 0; k < nbElement; k++ )
      {
         double wantA;

         if( outT[k] != midT[k + offT] )
         {
            printf( "Fail: TA_ICHIMOKU %d/%d/%d bar %d outTenkanSen %.17g, "
                    "TA_MIDPRICE(%d) %.17g\n", p->tenkan, p->kijun, p->senkouB,
                    (int)begIdx + k, outT[k], p->tenkan, midT[k + offT] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ichComposeCmp++;

         if( outK[k] != midK[k + offK] )
         {
            printf( "Fail: TA_ICHIMOKU %d/%d/%d bar %d outKijunSen %.17g, "
                    "TA_MIDPRICE(%d) %.17g\n", p->tenkan, p->kijun, p->senkouB,
                    (int)begIdx + k, outK[k], p->kijun, midK[k + offK] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ichComposeCmp++;

         if( outB[k] != midB[k + offB] )
         {
            printf( "Fail: TA_ICHIMOKU %d/%d/%d bar %d outSenkouSpanB %.17g, "
                    "TA_MIDPRICE(%d) %.17g\n", p->tenkan, p->kijun, p->senkouB,
                    (int)begIdx + k, outB[k], p->senkouB, midB[k + offB] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ichComposeCmp++;

         wantA = ( outT[k] + outK[k] ) / 2.0;
         if( outA[k] != wantA )
         {
            printf( "Fail: TA_ICHIMOKU %d/%d/%d bar %d outSenkouSpanA %.17g, "
                    "(tenkan+kijun)/2 %.17g\n", p->tenkan, p->kijun,
                    p->senkouB, (int)begIdx + k, outA[k], wantA );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ichComposeCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (2) Span A halves two already-rounded midpoints. */
static ErrorNumber test_ich_spana( void )
{
   static TA_Real outT[ICH_SPANA_NB], outK[ICH_SPANA_NB];
   static TA_Real outA[ICH_SPANA_NB], outB[ICH_SPANA_NB];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, discrim;

   rc = TA_ICHIMOKU( 0, ICH_SPANA_NB-1, ichSpanAHigh, ichSpanALow, 3, 5, 7,
                     &begIdx, &nbElement, outT, outK, outA, outB );
   if( rc != TA_SUCCESS || begIdx != 6 || nbElement != ICH_SPANA_NB - 6 )
   {
      printf( "Fail: TA_ICHIMOKU span A rc=%d range %d/%d (want 6/%d)\n",
              (int)rc, (int)begIdx, (int)nbElement, ICH_SPANA_NB - 6 );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   discrim = 0;
   for( k = 0; k < nbElement; k++ )
   {
      int bar = (int)begIdx + k;
      double hiT, loT, hiK, loK, want, alt;

      ichWindow( ichSpanAHigh, ichSpanALow, bar, 3, &hiT, &loT );
      ichWindow( ichSpanAHigh, ichSpanALow, bar, 5, &hiK, &loK );

      want = ( (hiT + loT) / 2.0 + (hiK + loK) / 2.0 ) / 2.0;
      alt  = ( hiT + loT + hiK + loK ) / 4.0;

      if( outA[k] != want )
      {
         printf( "Fail: TA_ICHIMOKU span A bar %d got %.17g, two halvings "
                 "%.17g, one quartering %.17g\n", bar, outA[k], want, alt );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ichSpanACmp++;

      if( alt != want )
         discrim++;
   }

   /* The control arm. A leg that cannot tell the two spellings apart passes
    * against both of them, so the discrimination is asserted, not assumed. */
   if( discrim < ICH_SPANA_MIN_DISCRIM )
   {
      printf( "Fail: TA_ICHIMOKU span A series discriminates on only %d of %d "
              "bars (floor %d): this leg is no longer testing the spelling\n",
              discrim, nbElement, ICH_SPANA_MIN_DISCRIM );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_ichSpanACmp++;

   return TA_TEST_PASS;
}

/* (3) The lookback is the longest of the three, whichever one that is. */
static ErrorNumber test_ich_lookback( const TA_History *history )
{
   static TA_Real outT[ICH_NB_BAR], outK[ICH_NB_BAR];
   static TA_Real outA[ICH_NB_BAR], outB[ICH_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int t, nb, longest, got;

   nb = (int)history->nbBars;

   for( t = 0; t < ICH_NB_LOOKBACK; t++ )
   {
      const IchTriple *p = &ichLookback[t];

      longest = ichLongest( p );

      got = TA_ICHIMOKU_Lookback( p->tenkan, p->kijun, p->senkouB );
      if( got != longest - 1 )
      {
         printf( "Fail: TA_ICHIMOKU_Lookback(%d,%d,%d) = %d, expected %d\n",
                 p->tenkan, p->kijun, p->senkouB, got, longest - 1 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ichLookbackCmp++;

      rc = TA_ICHIMOKU( 0, nb-1, history->high, history->low,
                        p->tenkan, p->kijun, p->senkouB,
                        &begIdx, &nbElement, outT, outK, outA, outB );
      if( rc != TA_SUCCESS || begIdx != got )
      {
         printf( "Fail: TA_ICHIMOKU(%d,%d,%d) rc=%d begIdx=%d, lookback said "
                 "%d\n", p->tenkan, p->kijun, p->senkouB, (int)rc,
                 (int)begIdx, got );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      g_ichLookbackCmp++;
   }

   return TA_TEST_PASS;
}

/* (4) Swapping tenkan and senkouB swaps exactly those two outputs. */
static ErrorNumber test_ich_swap( const TA_History *history )
{
   static TA_Real aT[ICH_NB_BAR], aK[ICH_NB_BAR], aA[ICH_NB_BAR], aB[ICH_NB_BAR];
   static TA_Real bT[ICH_NB_BAR], bK[ICH_NB_BAR], bA[ICH_NB_BAR], bB[ICH_NB_BAR];
   TA_RetCode rc;
   TA_Integer beg1, nb1, beg2, nb2;
   size_t bytes;
   int nb;

   nb = (int)history->nbBars;

   rc = TA_ICHIMOKU( 0, nb-1, history->high, history->low, 9, 26, 52,
                     &beg1, &nb1, aT, aK, aA, aB );
   if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;

   rc = TA_ICHIMOKU( 0, nb-1, history->high, history->low, 52, 26, 9,
                     &beg2, &nb2, bT, bK, bA, bB );
   if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;

   /* The longest period is 52 either way, so the two calls publish the same
    * bars and can be compared index for index. */
   if( beg1 != beg2 || nb1 != nb2 )
   {
      printf( "Fail: TA_ICHIMOKU swap ranges differ: %d/%d vs %d/%d\n",
              (int)beg1, (int)nb1, (int)beg2, (int)nb2 );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   bytes = (size_t)nb1 * sizeof(double);

   if( memcmp( aT, bB, bytes ) != 0 )
   {
      printf( "Fail: TA_ICHIMOKU tenkan(9) != senkouB(9) after the swap\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_ichSwapCmp++;

   if( memcmp( aB, bT, bytes ) != 0 )
   {
      printf( "Fail: TA_ICHIMOKU senkouB(52) != tenkan(52) after the swap\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_ichSwapCmp++;

   if( memcmp( aK, bK, bytes ) != 0 )
   {
      printf( "Fail: TA_ICHIMOKU kijun moved when tenkan and senkouB were "
              "swapped\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_ichSwapCmp++;

   /* The control arm: Span A reads tenkan, so it must NOT survive the swap.
    * Without this the leg would also pass against a function that ignored
    * its parameters and wrote the same four buffers every time. */
   if( memcmp( aA, bA, bytes ) == 0 )
   {
      printf( "Fail: TA_ICHIMOKU span A came back identical after the swap, "
              "so this leg compared nothing\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_ichSwapCmp++;

   return TA_TEST_PASS;
}

/* (5) A flat window: every line is the constant, Span A included. */
static ErrorNumber test_ich_flat( void )
{
   static TA_Real h[ICH_NB_BAR], l[ICH_NB_BAR];
   static TA_Real outT[ICH_NB_BAR], outK[ICH_NB_BAR];
   static TA_Real outA[ICH_NB_BAR], outB[ICH_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i, k;
   const double c = 50.0;

   for( i = 0; i < ICH_NB_BAR; i++ )
   {
      h[i] = c;
      l[i] = c;
   }

   rc = TA_ICHIMOKU( 0, ICH_NB_BAR-1, h, l, 9, 26, 52,
                     &begIdx, &nbElement, outT, outK, outA, outB );
   if( rc != TA_SUCCESS || begIdx != 51 || nbElement != ICH_NB_BAR - 51 )
   {
      printf( "Fail: TA_ICHIMOKU flat rc=%d range %d/%d (want 51/%d)\n",
              (int)rc, (int)begIdx, (int)nbElement, ICH_NB_BAR - 51 );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( k = 0; k < nbElement; k++ )
   {
      if( outT[k] != c || outK[k] != c || outA[k] != c || outB[k] != c )
      {
         printf( "Fail: TA_ICHIMOKU flat bar %d: %.17g %.17g %.17g %.17g, "
                 "expected exactly %.17g on all four\n", (int)begIdx + k,
                 outT[k], outK[k], outA[k], outB[k], c );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ichFlatCmp += 4;
   }

   return TA_TEST_PASS;
}

/* (6) The display shift is +kijun on the two leading spans, 0 on the lines. */
static ErrorNumber test_ich_shift( void )
{
   static const IchTriple shiftCase[4] =
   {
      {  9,  26,  52 },
      {  3,   5,   7 },
      {  2,   2,   2 },
      { 40, 100,  30 }
   };
   int t, o;

   for( t = 0; t < 4; t++ )
   {
      const IchTriple *p = &shiftCase[t];

      for( o = 0; o < 4; o++ )
      {
         int want = ( o >= 2 ) ? p->kijun : 0;
         int got = TA_ICHIMOKU_DisplayShift( p->tenkan, p->kijun, p->senkouB, o );

         if( got != want )
         {
            printf( "Fail: TA_ICHIMOKU_DisplayShift(%d,%d,%d,%d) = %d, "
                    "expected %d\n", p->tenkan, p->kijun, p->senkouB, o,
                    got, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ichShiftCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (7) Each output buffer over each input. */
static ErrorNumber test_ich_aliasing( const TA_History *history )
{
   static TA_Real refT[ICH_NB_BAR], refK[ICH_NB_BAR];
   static TA_Real refA[ICH_NB_BAR], refB[ICH_NB_BAR];
   static TA_Real workH[ICH_NB_BAR], workL[ICH_NB_BAR];
   static TA_Real s0[ICH_NB_BAR], s1[ICH_NB_BAR], s2[ICH_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   const TA_Real *ref[4];
   TA_Real *buf[4];
   TA_Real *scratch[3];
   int nb, i, whichOut, whichIn, j, s;

   nb = (int)history->nbBars;

   rc = TA_ICHIMOKU( 0, nb-1, history->high, history->low, 9, 26, 52,
                     &begIdx, &nbElement, refT, refK, refA, refB );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_ICHIMOKU aliasing baseline failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   ref[0] = refT; ref[1] = refK; ref[2] = refA; ref[3] = refB;
   scratch[0] = s0; scratch[1] = s1; scratch[2] = s2;

   for( whichOut = 0; whichOut < 4; whichOut++ )
   {
      for( whichIn = 0; whichIn < 2; whichIn++ )
      {
         TA_Real *aliased;

         for( i = 0; i < nb; i++ )
         {
            workH[i] = history->high[i];
            workL[i] = history->low[i];
         }

         aliased = ( whichIn == 0 ) ? workH : workL;

         s = 0;
         for( j = 0; j < 4; j++ )
            buf[j] = ( j == whichOut ) ? aliased : scratch[s++];

         rc = TA_ICHIMOKU( 0, nb-1, workH, workL, 9, 26, 52,
                           &begIdx2, &nbElement2,
                           buf[0], buf[1], buf[2], buf[3] );
         if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "Fail: TA_ICHIMOKU aliasing output %d over %s rc=%d\n",
                    whichOut, ( whichIn == 0 ) ? "inHigh" : "inLow", (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( j = 0; j < 4; j++ )
         {
            if( memcmp( buf[j], ref[j], (size_t)nbElement * sizeof(double) ) != 0 )
            {
               printf( "Fail: TA_ICHIMOKU aliasing output %d over %s changed "
                       "output %d\n", whichOut,
                       ( whichIn == 0 ) ? "inHigh" : "inLow", j );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         g_ichAliasCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (8) The startIdx/endIdx range sweep. TA_STABLE_EXACT: each window is built
 * from its own bars only, so a bar's value cannot depend on how far back the
 * call started. */
static TA_RetCode ichRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   static TA_Real o0[ICH_NB_BAR], o1[ICH_NB_BAR], o2[ICH_NB_BAR], o3[ICH_NB_BAR];
   TA_Real *buf[4];

   (void)outputBufferInt;
   *isOutputInteger = 0;

   buf[0] = o0; buf[1] = o1; buf[2] = o2; buf[3] = o3;
   if( outputNb < 4 )
      buf[outputNb] = outputBuffer;

   *lookback = TA_ICHIMOKU_Lookback( 9, 26, 52 );

   return TA_ICHIMOKU( startIdx, endIdx, h->high, h->low, 9, 26, 52,
                       outBegIdx, outNbElement, buf[0], buf[1], buf[2], buf[3] );
}

static ErrorNumber test_ich_range( const TA_History *history )
{
   return doRangeTestEx( ichRangeTestFunction,
                         TA_STABLE_EXACT, TA_TEST_UNST_NONE,
                         (void *)history, 4, 0 );
}
