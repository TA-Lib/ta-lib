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
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  110726 MF,CC First version (#584).
 *
 */

/* Description:
 *
 *   Test TA_ALLIGATOR (Bill Williams' Alligator, #584).
 *
 *   Legs:
 *     1. COMPOSITION. Each line against TA_RMA of TA_MEDPRICE entered at the
 *        Alligator's own outBegIdx, bit for bit, on period triples where each
 *        leg is the longest in turn, at four shift triples: the shifts must
 *        not move a value. Also the leg that drives server_verify.
 *     2. UNSTABLE PERIOD. The lookback is max(period) - 1 + U at each
 *        TA_FUNC_UNST_RMA setting, the call starts there, and leg 1's
 *        composition still holds.
 *     3. PERIOD 1. Every line is exactly (high + low) / 2 on every bar.
 *     4. EDGES. A range ending before the lookback returns nothing; a
 *        one-bar range at the lookback returns that bar of the full call; a
 *        call starting past the lookback still seeds each leg at its own
 *        lookback before startIdx.
 *     5. DISPLAY SHIFT. Each output reports its own shift parameter.
 *     6. ALIASING. Each output buffer over each input.
 *     7. GOLDEN VALUES from outside TA-Lib: see algGolden.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define ALG_NB_BAR 252

typedef struct { int jawP; int teethP; int lipsP; } AlgPeriods;
typedef struct { int jawS; int teethS; int lipsS; } AlgShifts;

static const AlgPeriods algPeriods[] =
{
   { 13,  8,  5 },   /* the published defaults */
   {  1,  1,  1 },   /* the minimum: every leg is the median price */
   {  2,  5,  8 },   /* jaw shortest, lips longest */
   {  5, 40,  8 },   /* teeth longest */
   { 13,  8, 40 },   /* lips longest */
   { 30, 13,  2 },
   {  8,  8,  8 }    /* all equal: three bit-identical lines */
};
#define ALG_NB_PERIODS ((int)(sizeof(algPeriods)/sizeof(algPeriods[0])))

static const AlgShifts algShifts[] =
{
   {  8,  5,  3 },   /* the published defaults */
   {  0,  0,  0 },
   {  1,  3,  5 },
   { 40,  8,  0 }
};
#define ALG_NB_SHIFTS ((int)(sizeof(algShifts)/sizeof(algShifts[0])))

static const int algUnst[] = { 0, 10, 30 };
#define ALG_NB_UNST ((int)(sizeof(algUnst)/sizeof(algUnst[0])))

/* Leg 7. Source: trading-signals 8.3.0 (npm, pure TypeScript, no TA-Lib
 * binding), its own Alligator class, run by capture_12_alligator.mjs at
 * ta-lib-oracles 1ba1b1f on Node 22.22.2 over the 252-bar corpus, captured as
 * hex-of-IEEE-bits and printed here with %.17g. The class draws each line
 * `shift` bars ahead, so a value it prints at bar b is the one computed at
 * b - shift: `bar` below is the computing bar, which only holds if the display
 * shift points forward.
 *
 * The library seeds every leg at the first bar it is fed, and this function
 * seeds each leg at outBegIdx - (period - 1), so each row comes from a run fed
 * from that leg's seed bar. A run fed from bar 0 seeds the short legs earlier
 * and differs by up to 1e-2 until the seed decays. The residual is the
 * (x - prev)/n + prev spelling against wAlpha*x + wBeta*prev: 2.3e-16 relative
 * at worst against this library on the orchestrator, 3.4e-16 against that
 * expression evaluated unfused. Tolerance 1e-13 relative.
 */
#define ALG_GOLDEN_TOL 1e-13

typedef struct { int jawP, jawS, teethP, teethS, lipsP, lipsS;
                 int output; int bar; double value; } AlgGolden;

static const AlgGolden algGolden[] =
{
   { 13,8,  8,5,  5,3, 0,  12, 93.982500000000016 },
   { 13,8,  8,5,  5,3, 0,  13, 93.736346153846171 },
   { 13,8,  8,5,  5,3, 0,  17, 92.851740532409735 },
   { 13,8,  8,5,  5,3, 0, 128, 123.56347152683894 },
   { 13,8,  8,5,  5,3, 0, 243, 107.29430594853415 },
   { 13,8,  8,5,  5,3, 1,  20, 91.588431128375248 },
   { 13,8,  8,5,  5,3, 1,  21, 91.079252237328348 },
   { 13,8,  8,5,  5,3, 1,  25, 88.0857781059144 },
   { 13,8,  8,5,  5,3, 1, 133, 131.71322106471939 },
   { 13,8,  8,5,  5,3, 1, 246, 108.84616569159761 },
   { 13,8,  8,5,  5,3, 2,  25, 86.409975027969637 },
   { 13,8,  8,5,  5,3, 2,  26, 85.696980022375712 },
   { 13,8,  8,5,  5,3, 2,  30, 86.264827017165089 },
   { 13,8,  8,5,  5,3, 2, 137, 132.77665031055284 },
   { 13,8,  8,5,  5,3, 2, 248, 109.37151034205378 },
   { 13,8,  8,5, 40,3, 2,  39, 89.350875000000002 },
   { 13,8,  8,5, 40,3, 2,  40, 89.204603125000006 },
   { 13,8,  8,5, 40,3, 2,  44, 89.166807920157481 },
   { 13,8,  8,5, 40,3, 2, 144, 117.55520492320518 },
   { 13,8,  8,5, 40,3, 2, 248, 109.20399071947523 },
   { 13,8,  8,5, 40,3, 0,  61, 87.520649675861918 },
   { 13,8,  8,5, 40,3, 0,  62, 87.759445854641768 },
   { 13,8,  8,5, 40,3, 0,  66, 89.034151963931649 },
   { 13,8,  8,5, 40,3, 0, 152, 124.81730647814466 },
   { 13,8,  8,5, 40,3, 0, 243, 107.29430587963529 },
   { 13,8,  8,5, 40,3, 1,  69, 90.158393922724414 },
   { 13,8,  8,5, 40,3, 1,  70, 89.919844682383868 },
   { 13,8,  8,5, 40,3, 1,  74, 87.665126851172772 },
   { 13,8,  8,5, 40,3, 1, 158, 124.49266366349764 },
   { 13,8,  8,5, 40,3, 1, 246, 108.84616569159761 },
   { 13,3,  8,5,  5,7, 0,  12, 93.982500000000016 },
   { 13,3,  8,5,  5,7, 0,  13, 93.736346153846171 },
   { 13,3,  8,5,  5,7, 0,  17, 92.851740532409735 },
   { 13,3,  8,5,  5,7, 0, 130, 125.45866804653733 },
   { 13,3,  8,5,  5,7, 0, 248, 107.96580943563613 },
   { 13,3,  8,5,  5,7, 1,  15, 92.913560791015627 },
   { 13,3,  8,5,  5,7, 1,  16, 92.693740692138675 },
   { 13,3,  8,5,  5,7, 1,  20, 91.588431128375248 },
   { 13,3,  8,5,  5,7, 1, 131, 130.15737037024576 },
   { 13,3,  8,5,  5,7, 1, 246, 108.84616569159761 },
   { 13,3,  8,5,  5,7, 2,  16, 92.381252799999999 },
   { 13,3,  8,5,  5,7, 2,  17, 91.848502240000002 },
   { 13,3,  8,5,  5,7, 2,  21, 90.278210517504007 },
   { 13,3,  8,5,  5,7, 2, 130, 131.93207003857057 },
   { 13,3,  8,5,  5,7, 2, 244, 109.30448813977972 },
   { 13,0,  8,0,  5,0, 0,  12, 93.982500000000016 },
   { 13,0,  8,0,  5,0, 0,  13, 93.736346153846171 },
   { 13,0,  8,0,  5,0, 0,  17, 92.851740532409735 },
   { 13,0,  8,0,  5,0, 0, 132, 127.22587691539275 },
   { 13,0,  8,0,  5,0, 0, 251, 108.19287150877526 },
   { 13,0,  8,0,  5,0, 1,  17, 92.321710605621348 },
   { 13,0,  8,0,  5,0, 1,  18, 92.107746779918685 },
   { 13,0,  8,0,  5,0, 1,  22, 90.499033207662308 },
   { 13,0,  8,0,  5,0, 1, 134, 132.30406843162947 },
   { 13,0,  8,0,  5,0, 1, 251, 109.0015614861658 },
   { 13,0,  8,0,  5,0, 2,  20, 90.969013146880002 },
   { 13,0,  8,0,  5,0, 2,  21, 90.278210517504007 },
   { 13,0,  8,0,  5,0, 2,  25, 86.409975027969637 },
   { 13,0,  8,0,  5,0, 2, 136, 133.89331288819105 },
   { 13,0,  8,0,  5,0, 2, 251, 109.16701329513154 }
};
#define ALG_NB_GOLDEN ((int)(sizeof(algGolden)/sizeof(algGolden[0])))

/* Leg 1: periods x shifts x 3 legs, one memcmp each. */
#define ALG_COMPOSE_CMP  (ALG_NB_PERIODS*ALG_NB_SHIFTS*3)
/* Leg 2: per period triple and setting, _Lookback and the call's begIdx, then
 * the three legs. */
#define ALG_UNST_CMP     (ALG_NB_PERIODS*ALG_NB_UNST*(2+3))
/* Leg 3: shifts x 252 bars x 3 outputs. */
#define ALG_PERIOD1_CMP  (ALG_NB_SHIFTS*ALG_NB_BAR*3)
/* Leg 4: the empty range, three outputs of the one-bar range, then three legs
 * at each of two late starts. */
#define ALG_EDGE_CMP     10
/* Leg 5: shifts x 3 outputs. */
#define ALG_SHIFT_CMP    (ALG_NB_SHIFTS*3)
/* Leg 6: 3 output buffers x 2 inputs. */
#define ALG_ALIAS_CMP    6
/* Leg 7: one per row. */
#define ALG_GOLDEN_CMP   ALG_NB_GOLDEN

static int g_algComposeCmp;
static int g_algUnstCmp;
static int g_algPeriod1Cmp;
static int g_algEdgeCmp;
static int g_algShiftCmp;
static int g_algAliasCmp;
static int g_algGoldenCmp;

static ErrorNumber test_alg_compose ( const TA_History *history );
static ErrorNumber test_alg_unst    ( const TA_History *history );
static ErrorNumber test_alg_period1 ( const TA_History *history );
static ErrorNumber test_alg_edges   ( const TA_History *history );
static ErrorNumber test_alg_shift   ( void );
static ErrorNumber test_alg_aliasing( const TA_History *history );
static ErrorNumber test_alg_golden  ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_alligator( TA_History *history )
{
   ErrorNumber retValue;
   unsigned int savedUnst;

   g_algComposeCmp = 0;
   g_algUnstCmp = 0;
   g_algPeriod1Cmp = 0;
   g_algEdgeCmp = 0;
   g_algShiftCmp = 0;
   g_algAliasCmp = 0;
   g_algGoldenCmp = 0;

   if( history->nbBars != ALG_NB_BAR )
   {
      printf( "Fail: TA_ALLIGATOR expects the %d-bar corpus, got %d\n",
              ALG_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   /* DO_TEST does not reset unstable periods, and an earlier group may have
    * left one set: every leg below assumes 0 unless it sets one itself. */
   savedUnst = TA_GetUnstablePeriod( TA_FUNC_UNST_RMA );
   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );

   retValue = test_alg_compose( history );
   if( retValue == TA_TEST_PASS ) retValue = test_alg_unst( history );
   if( retValue == TA_TEST_PASS ) retValue = test_alg_period1( history );
   if( retValue == TA_TEST_PASS ) retValue = test_alg_edges( history );
   if( retValue == TA_TEST_PASS ) retValue = test_alg_shift();
   if( retValue == TA_TEST_PASS ) retValue = test_alg_aliasing( history );
   if( retValue == TA_TEST_PASS ) retValue = test_alg_golden( history );

   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, savedUnst );

   if( retValue != TA_TEST_PASS )
      return retValue;

   if( g_algComposeCmp != ALG_COMPOSE_CMP
    || g_algUnstCmp    != ALG_UNST_CMP
    || g_algPeriod1Cmp != ALG_PERIOD1_CMP
    || g_algEdgeCmp    != ALG_EDGE_CMP
    || g_algShiftCmp   != ALG_SHIFT_CMP
    || g_algAliasCmp   != ALG_ALIAS_CMP
    || g_algGoldenCmp  != ALG_GOLDEN_CMP )
   {
      printf( "Fail: TA_ALLIGATOR comparison counts (compose %d, unst %d, "
              "period1 %d, edge %d, shift %d, alias %d, golden %d) are not what "
              "this file asserts (%d, %d, %d, %d, %d, %d, %d)\n",
              g_algComposeCmp, g_algUnstCmp, g_algPeriod1Cmp, g_algEdgeCmp,
              g_algShiftCmp, g_algAliasCmp, g_algGoldenCmp,
              ALG_COMPOSE_CMP, ALG_UNST_CMP, ALG_PERIOD1_CMP, ALG_EDGE_CMP,
              ALG_SHIFT_CMP, ALG_ALIAS_CMP, ALG_GOLDEN_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.    ****/

static int algLongest( const AlgPeriods *p )
{
   int longest = p->jawP;
   if( p->teethP > longest )
      longest = p->teethP;
   if( p->lipsP > longest )
      longest = p->lipsP;
   return longest;
}

/* The three lines of one call against TA_RMA(TA_MEDPRICE) entered at the
 * call's own outBegIdx, which is where each leg's seed window is anchored.
 * Adds one to *counter per memcmp. */
static ErrorNumber algCheckComposition( const TA_History *history,
                                        const AlgPeriods *p, const AlgShifts *s,
                                        TA_Integer begIdx, TA_Integer nbElement,
                                        const TA_Real *out[3], int *counter )
{
   static TA_Real med[ALG_NB_BAR], ref[ALG_NB_BAR];
   static const char *legName[3] = { "outJaw", "outTeeth", "outLips" };
   TA_RetCode rc;
   TA_Integer mBeg, mNb, rBeg, rNb;
   int nb, leg, period;

   nb = (int)history->nbBars;

   rc = TA_MEDPRICE( 0, nb-1, history->high, history->low, &mBeg, &mNb, med );
   if( rc != TA_SUCCESS || mBeg != 0 || mNb != nb )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   for( leg = 0; leg < 3; leg++ )
   {
      period = ( leg == 0 ) ? p->jawP : ( leg == 1 ) ? p->teethP : p->lipsP;

      rc = TA_RMA( begIdx, nb-1, med, period, &rBeg, &rNb, ref );
      if( rc != TA_SUCCESS || rBeg != begIdx || rNb != nbElement )
      {
         printf( "Fail: TA_ALLIGATOR reference TA_RMA(%d) rc=%d range %d/%d, "
                 "Alligator %d/%d\n", period, (int)rc, (int)rBeg, (int)rNb,
                 (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( memcmp( out[leg], ref, (size_t)nbElement * sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_ALLIGATOR %d,%d,%d,%d,%d,%d %s differs from "
                 "TA_RMA(TA_MEDPRICE, %d) started at bar %d\n",
                 p->jawP, s->jawS, p->teethP, s->teethS, p->lipsP, s->lipsS,
                 legName[leg], period, (int)begIdx );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      (*counter)++;
   }

   return TA_TEST_PASS;
}

/* (1) Composition, at every shift triple. */
static ErrorNumber test_alg_compose( const TA_History *history )
{
   static TA_Real outJ[ALG_NB_BAR], outT[ALG_NB_BAR], outL[ALG_NB_BAR];
   const TA_Real *out[3];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   int t, s, nb, longest;

   nb = (int)history->nbBars;
   out[0] = outJ; out[1] = outT; out[2] = outL;

   for( t = 0; t < ALG_NB_PERIODS; t++ )
   {
      const AlgPeriods *p = &algPeriods[t];
      longest = algLongest( p );

      for( s = 0; s < ALG_NB_SHIFTS; s++ )
      {
         const AlgShifts *sh = &algShifts[s];

         rc = TA_ALLIGATOR( 0, nb-1, history->high, history->low,
                            p->jawP, sh->jawS, p->teethP, sh->teethS,
                            p->lipsP, sh->lipsS,
                            &begIdx, &nbElement, outJ, outT, outL );
         if( rc != TA_SUCCESS || begIdx != longest - 1
             || nbElement != nb - (longest - 1) )
         {
            printf( "Fail: TA_ALLIGATOR %d,%d,%d,%d,%d,%d rc=%d range %d/%d "
                    "(want %d/%d)\n", p->jawP, sh->jawS, p->teethP, sh->teethS,
                    p->lipsP, sh->lipsS, (int)rc, (int)begIdx, (int)nbElement,
                    longest - 1, nb - (longest - 1) );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         /* Most period triples move two or three parameters at once, which
          * the --codegen sweep never sends. One shift triple per period triple
          * is enough: the shifts reach no value. */
         if( s == 2 && server_verify_active() )
         {
            const double opt[6] = { (double)p->jawP, (double)sh->jawS,
                                    (double)p->teethP, (double)sh->teethS,
                                    (double)p->lipsP, (double)sh->lipsS };
            e = server_verify( "ALLIGATOR", 0, nb-1, nb,
                               rc, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low, NULL },
                               opt, 6,
                               (const TA_Real*[]){ outJ, outT, outL, NULL },
                               NULL );
            if( e != TA_TEST_PASS )
               return e;
         }

         e = algCheckComposition( history, p, sh, begIdx, nbElement, out,
                                  &g_algComposeCmp );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (2) TA_FUNC_UNST_RMA reaches all three legs through the one id. */
static ErrorNumber test_alg_unst( const TA_History *history )
{
   static TA_Real outJ[ALG_NB_BAR], outT[ALG_NB_BAR], outL[ALG_NB_BAR];
   const TA_Real *out[3];
   const AlgShifts *sh = &algShifts[0];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   int t, u, nb, want, got;

   nb = (int)history->nbBars;
   out[0] = outJ; out[1] = outT; out[2] = outL;

   for( u = 0; u < ALG_NB_UNST; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, (unsigned int)algUnst[u] );

      for( t = 0; t < ALG_NB_PERIODS; t++ )
      {
         const AlgPeriods *p = &algPeriods[t];

         want = algLongest( p ) - 1 + algUnst[u];
         got = TA_ALLIGATOR_Lookback( p->jawP, sh->jawS, p->teethP, sh->teethS,
                                      p->lipsP, sh->lipsS );
         if( got != want )
         {
            printf( "Fail: TA_ALLIGATOR_Lookback(%d,%d,%d) at unstable %d = %d, "
                    "expected %d\n", p->jawP, p->teethP, p->lipsP, algUnst[u],
                    got, want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_algUnstCmp++;

         rc = TA_ALLIGATOR( 0, nb-1, history->high, history->low,
                            p->jawP, sh->jawS, p->teethP, sh->teethS,
                            p->lipsP, sh->lipsS,
                            &begIdx, &nbElement, outJ, outT, outL );
         if( rc != TA_SUCCESS || begIdx != want || nbElement != nb - want )
         {
            printf( "Fail: TA_ALLIGATOR(%d,%d,%d) at unstable %d rc=%d range "
                    "%d/%d, lookback %d\n", p->jawP, p->teethP, p->lipsP,
                    algUnst[u], (int)rc, (int)begIdx, (int)nbElement, want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         g_algUnstCmp++;

         e = algCheckComposition( history, p, sh, begIdx, nbElement, out,
                                  &g_algUnstCmp );
         if( e != TA_TEST_PASS )
         {
            TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
            return e;
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
   return TA_TEST_PASS;
}

/* (3) Period 1: the smoother is the identity, so each line is the median. */
static ErrorNumber test_alg_period1( const TA_History *history )
{
   static TA_Real outJ[ALG_NB_BAR], outT[ALG_NB_BAR], outL[ALG_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int s, k, nb;

   nb = (int)history->nbBars;

   for( s = 0; s < ALG_NB_SHIFTS; s++ )
   {
      const AlgShifts *sh = &algShifts[s];

      rc = TA_ALLIGATOR( 0, nb-1, history->high, history->low,
                         1, sh->jawS, 1, sh->teethS, 1, sh->lipsS,
                         &begIdx, &nbElement, outJ, outT, outL );
      if( rc != TA_SUCCESS || begIdx != 0 || nbElement != nb )
      {
         printf( "Fail: TA_ALLIGATOR period 1 rc=%d range %d/%d (want 0/%d)\n",
                 (int)rc, (int)begIdx, (int)nbElement, nb );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( k = 0; k < nb; k++ )
      {
         double want = ( history->high[k] + history->low[k] ) / 2.0;

         if( outJ[k] != want || outT[k] != want || outL[k] != want )
         {
            printf( "Fail: TA_ALLIGATOR period 1 bar %d: %.17g %.17g %.17g, "
                    "median %.17g\n", k, outJ[k], outT[k], outL[k], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_algPeriod1Cmp += 3;
      }
   }

   return TA_TEST_PASS;
}

/* (4) The range edges at the default parameters. */
static ErrorNumber test_alg_edges( const TA_History *history )
{
   static TA_Real fullJ[ALG_NB_BAR], fullT[ALG_NB_BAR], fullL[ALG_NB_BAR];
   TA_Real oneJ[1], oneT[1], oneL[1];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, fBeg, fNb;
   static const AlgPeriods late[2] = { { 13, 8, 5 }, { 13, 8, 40 } };
   static const int lateStart[2] = { 50, 100 };
   const AlgShifts *sh = &algShifts[0];
   ErrorNumber e;
   int lookback, t;

   lookback = TA_ALLIGATOR_Lookback( 13, 8, 8, 5, 5, 3 );

   begIdx = -1;
   nbElement = -1;
   rc = TA_ALLIGATOR( 0, lookback - 1, history->high, history->low,
                      13, 8, 8, 5, 5, 3, &begIdx, &nbElement, oneJ, oneT, oneL );
   if( rc != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "Fail: TA_ALLIGATOR endIdx %d below lookback %d: rc=%d range "
              "%d/%d (want 0/0)\n", lookback - 1, lookback, (int)rc,
              (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_algEdgeCmp++;

   rc = TA_ALLIGATOR( 0, (int)history->nbBars - 1, history->high, history->low,
                      13, 8, 8, 5, 5, 3, &fBeg, &fNb, fullJ, fullT, fullL );
   if( rc != TA_SUCCESS || fBeg != lookback )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   /* At startIdx == lookback every leg seeds where the full call seeds it. */
   rc = TA_ALLIGATOR( lookback, lookback, history->high, history->low,
                      13, 8, 8, 5, 5, 3, &begIdx, &nbElement, oneJ, oneT, oneL );
   if( rc != TA_SUCCESS || begIdx != lookback || nbElement != 1 )
   {
      printf( "Fail: TA_ALLIGATOR one-bar range at %d: rc=%d range %d/%d\n",
              lookback, (int)rc, (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( oneJ[0] != fullJ[0] ) return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   g_algEdgeCmp++;
   if( oneT[0] != fullT[0] ) return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   g_algEdgeCmp++;
   if( oneL[0] != fullL[0] ) return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   g_algEdgeCmp++;

   for( t = 0; t < 2; t++ )
   {
      const TA_Real *out[3];

      out[0] = fullJ; out[1] = fullT; out[2] = fullL;
      rc = TA_ALLIGATOR( lateStart[t], (int)history->nbBars - 1,
                         history->high, history->low,
                         late[t].jawP, sh->jawS, late[t].teethP, sh->teethS,
                         late[t].lipsP, sh->lipsS,
                         &begIdx, &nbElement, fullJ, fullT, fullL );
      if( rc != TA_SUCCESS || begIdx != lateStart[t]
          || nbElement != (int)history->nbBars - lateStart[t] )
      {
         printf( "Fail: TA_ALLIGATOR startIdx %d rc=%d range %d/%d\n",
                 lateStart[t], (int)rc, (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      e = algCheckComposition( history, &late[t], sh, begIdx, nbElement, out,
                               &g_algEdgeCmp );
      if( e != TA_TEST_PASS )
         return e;
   }

   return TA_TEST_PASS;
}

/* (5) Each output's display shift is its own shift parameter. */
static ErrorNumber test_alg_shift( void )
{
   int s, o, want, got;

   for( s = 0; s < ALG_NB_SHIFTS; s++ )
   {
      const AlgShifts *sh = &algShifts[s];

      for( o = 0; o < 3; o++ )
      {
         want = ( o == 0 ) ? sh->jawS : ( o == 1 ) ? sh->teethS : sh->lipsS;
         got = TA_ALLIGATOR_DisplayShift( 13, sh->jawS, 8, sh->teethS,
                                          5, sh->lipsS, o );
         if( got != want )
         {
            printf( "Fail: TA_ALLIGATOR_DisplayShift(13,%d,8,%d,5,%d,%d) = %d, "
                    "expected %d\n", sh->jawS, sh->teethS, sh->lipsS, o,
                    got, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_algShiftCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (6) Each output buffer over each input. */
static ErrorNumber test_alg_aliasing( const TA_History *history )
{
   static TA_Real refJ[ALG_NB_BAR], refT[ALG_NB_BAR], refL[ALG_NB_BAR];
   static TA_Real workH[ALG_NB_BAR], workL[ALG_NB_BAR];
   static TA_Real s0[ALG_NB_BAR], s1[ALG_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   const TA_Real *ref[3];
   TA_Real *buf[3];
   TA_Real *scratch[2];
   int nb, i, whichOut, whichIn, j, s;

   nb = (int)history->nbBars;

   rc = TA_ALLIGATOR( 0, nb-1, history->high, history->low, 13, 8, 8, 5, 5, 3,
                      &begIdx, &nbElement, refJ, refT, refL );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   ref[0] = refJ; ref[1] = refT; ref[2] = refL;
   scratch[0] = s0; scratch[1] = s1;

   for( whichOut = 0; whichOut < 3; whichOut++ )
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
         for( j = 0; j < 3; j++ )
            buf[j] = ( j == whichOut ) ? aliased : scratch[s++];

         rc = TA_ALLIGATOR( 0, nb-1, workH, workL, 13, 8, 8, 5, 5, 3,
                            &begIdx2, &nbElement2, buf[0], buf[1], buf[2] );
         if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "Fail: TA_ALLIGATOR aliasing output %d over %s rc=%d\n",
                    whichOut, ( whichIn == 0 ) ? "inHigh" : "inLow", (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( j = 0; j < 3; j++ )
         {
            if( memcmp( buf[j], ref[j], (size_t)nbElement * sizeof(double) ) != 0 )
            {
               printf( "Fail: TA_ALLIGATOR aliasing output %d over %s changed "
                       "output %d\n", whichOut,
                       ( whichIn == 0 ) ? "inHigh" : "inLow", j );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         g_algAliasCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (7) Values from outside TA-Lib. */
static ErrorNumber algCheckGolden( const TA_History *history,
                                   const AlgGolden *g, int nbRow, double tol,
                                   const char *what )
{
   static TA_Real out[3][ALG_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int r, k;
   double got, rel;

   for( r = 0; r < nbRow; r++ )
   {
      rc = TA_ALLIGATOR( 0, (int)history->nbBars-1, history->high, history->low,
                         g[r].jawP, g[r].jawS, g[r].teethP, g[r].teethS,
                         g[r].lipsP, g[r].lipsS, &begIdx, &nbElement,
                         out[0], out[1], out[2] );
      k = g[r].bar - (int)begIdx;
      if( rc != TA_SUCCESS || k < 0 || k >= (int)nbElement )
      {
         printf( "Fail: TA_ALLIGATOR %s golden row %d rc=%d range %d/%d does "
                 "not hold bar %d\n", what, r, (int)rc, (int)begIdx,
                 (int)nbElement, g[r].bar );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      got = out[g[r].output][k];
      rel = fabs( got - g[r].value ) / fabs( g[r].value );
      if( !(rel <= tol) )
      {
         printf( "Fail: TA_ALLIGATOR %s golden %d,%d,%d,%d,%d,%d output %d bar "
                 "%d got %.17g, trading-signals %.17g (rel %.3g > %.3g)\n",
                 what, g[r].jawP, g[r].jawS, g[r].teethP, g[r].teethS,
                 g[r].lipsP, g[r].lipsS, g[r].output, g[r].bar, got,
                 g[r].value, rel, tol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_algGoldenCmp++;
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_alg_golden( const TA_History *history )
{
   return algCheckGolden( history, algGolden, ALG_NB_GOLDEN,
                          ALG_GOLDEN_TOL, "anchored" );
}
