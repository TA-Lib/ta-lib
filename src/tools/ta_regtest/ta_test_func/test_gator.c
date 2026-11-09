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
 *  110726 MF,CC First version (#583).
 *
 */

/* Description:
 *
 *   Test TA_GATOR (Bill Williams' Gator Oscillator, #583).
 *
 *   Legs:
 *     1. COMPOSITION. Each histogram against TA_RMA of TA_MEDPRICE, each leg
 *        entered at the output's first bar less its read-back distance, bit for
 *        bit by memcmp, on period triples where each leg is the longest in
 *        turn and shift triples covering every ordering of the shifts. Also
 *        the leg that drives server_verify.
 *     2. UNSTABLE PERIOD. The lookback is max(period - 1 + U + read-back) at
 *        each TA_FUNC_UNST_RMA setting, the call starts there, and leg 1's
 *        composition still holds.
 *     3. SIGNED ZERO. A zero spread is +0.0, by memcmp.
 *     4. EDGES. A range ending before the lookback returns nothing; a
 *        one-bar range at the lookback returns that bar of the full call; a
 *        call starting past the lookback still seeds each leg at its own
 *        lookback before its read-back bar.
 *     5. DISPLAY SHIFT. The smaller shift of each histogram's two lines.
 *     6. ALIASING. Each output buffer over each input.
 *     7. GOLDEN VALUES from outside TA-Lib: see gtrP1 and gtrTail.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define GTR_NB_BAR 252

typedef struct { int jawP; int teethP; int lipsP; } GtrPeriods;
typedef struct { int jawS; int teethS; int lipsS; } GtrShifts;

static const GtrPeriods gtrPeriods[] =
{
   { 13,  8,  5 },   /* the published defaults */
   {  1,  1,  1 },
   {  2,  5,  8 },   /* jaw shortest, lips longest */
   {  5, 40,  8 },   /* teeth longest */
   { 13,  8, 40 },   /* lips longest */
   { 30, 13,  2 }
};
#define GTR_NB_PERIODS ((int)(sizeof(gtrPeriods)/sizeof(gtrPeriods[0])))

static const GtrShifts gtrShifts[] =
{
   {  8,  5,  3 },   /* the published defaults: jaw and teeth read back */
   {  3,  5,  7 },   /* teeth and lips read back */
   {  2,  9,  4 },   /* teeth read back for both histograms */
   {  0,  0,  0 },
   {  5,  5,  5 },
   { 40,  8,  0 }
};
#define GTR_NB_SHIFTS ((int)(sizeof(gtrShifts)/sizeof(gtrShifts[0])))

static const int gtrUnst[] = { 0, 10, 30 };
#define GTR_NB_UNST ((int)(sizeof(gtrUnst)/sizeof(gtrUnst[0])))

/* Leg 7. Source: trading-signals 8.3.0 (npm, pure TypeScript, no TA-Lib
 * binding), its own GatorOscillator class, run by capture_12_alligator.mjs at
 * ta-lib-oracles 1ba1b1f on Node 22.22.2 over the 252-bar corpus fed from
 * bar 0, captured as hex-of-IEEE-bits and printed here with %.17g. The class
 * prints each histogram where a chart draws it, so a value it prints at bar b
 * is the one this function computes at b - TA_GATOR_DisplayShift: `bar` below
 * is the computing bar.
 *
 * gtrP1: every period 1, so every line is the median price whatever bar it
 * seeds on, and each row pins the read-back distances, the pairing and the
 * sign at that bar. Measured worst over every bar of the three tuples: 0.
 * Tolerance 1e-12 absolute.
 *
 * gtrTail: real periods. The class seeds every line at bar 0 and this
 * function seeds each line at its own lookback, so the two converge, measured
 * worst 5.5e-12 absolute from bar 200 on over the three tuples. These rows
 * are what pin each period to its line, which period 1 cannot. Tolerance
 * 1e-10 absolute: the outputs are spreads that cross zero, so a relative
 * bound is unbounded near a crossing.
 */
#define GTR_P1_TOL   1e-12
#define GTR_TAIL_TOL 1e-10

typedef struct { int jawP, jawS, teethP, teethS, lipsP, lipsS;
                 int output; int bar; double value; } GtrGolden;

static const GtrGolden gtrP1[] =
{
   {  1,8,  1,5,  1,3, 0,   3, 2.8449999999999989 },
   {  1,8,  1,5,  1,3, 0,   4, 1.2349999999999994 },
   {  1,8,  1,5,  1,3, 0,  84, 1.3425000000000011 },
   {  1,8,  1,5,  1,3, 0, 165, 1.6899999999999977 },
   {  1,8,  1,5,  1,3, 0, 246, 0.21999999999999886 },
   {  1,8,  1,5,  1,3, 1,   5, -0.73499999999999943 },
   {  1,8,  1,5,  1,3, 1,   6, -0.90749999999999886 },
   {  1,8,  1,5,  1,3, 1,  86, -2.1875 },
   {  1,8,  1,5,  1,3, 1, 167, -2.8449999999999989 },
   {  1,8,  1,5,  1,3, 1, 248, -0.59000000000000341 },
   {  1,3,  1,5,  1,7, 0,   4, 0.90500000000000114 },
   {  1,3,  1,5,  1,7, 0,   5, 0.73499999999999943 },
   {  1,3,  1,5,  1,7, 0,  85, 1.3149999999999977 },
   {  1,3,  1,5,  1,7, 0, 167, 2.8449999999999989 },
   {  1,3,  1,5,  1,7, 0, 248, 0.59000000000000341 },
   {  1,3,  1,5,  1,7, 1,   2, -3.3125 },
   {  1,3,  1,5,  1,7, 1,   3, -1.6724999999999994 },
   {  1,3,  1,5,  1,7, 1,  83, -3.2049999999999983 },
   {  1,3,  1,5,  1,7, 1, 165, -0.18999999999999773 },
   {  1,3,  1,5,  1,7, 1, 246, -0.22499999999999432 },
   {  1,2,  1,9,  1,4, 0,   7, 0.26500000000000057 },
   {  1,2,  1,9,  1,4, 0,   8, 2.2175000000000011 },
   {  1,2,  1,9,  1,4, 0,  88, 6.2349999999999994 },
   {  1,2,  1,9,  1,4, 0, 169, 6.4099999999999966 },
   {  1,2,  1,9,  1,4, 0, 249, 0.21500000000000341 },
   {  1,2,  1,9,  1,4, 1,   7, -3.5775000000000006 },
   {  1,2,  1,9,  1,4, 1,   8, -3.8900000000000006 },
   {  1,2,  1,9,  1,4, 1,  87, -5.0949999999999989 },
   {  1,2,  1,9,  1,4, 1, 167, -4.5349999999999966 },
   {  1,2,  1,9,  1,4, 1, 247, -0.78000000000000114 }
};
#define GTR_NB_P1 ((int)(sizeof(gtrP1)/sizeof(gtrP1[0])))

static const GtrGolden gtrTail[] =
{
   { 13,8,  8,5,  5,3, 0, 200, 6.1842742068585039 },
   { 13,8,  8,5,  5,3, 0, 215, 6.7863322635499657 },
   { 13,8,  8,5,  5,3, 0, 231, 0.82195483053071428 },
   { 13,8,  8,5,  5,3, 0, 246, 1.55185974306346 },
   { 13,8,  8,5,  5,3, 1, 200, -5.035335563917215 },
   { 13,8,  8,5,  5,3, 1, 216, -2.6182196198058421 },
   { 13,8,  8,5,  5,3, 1, 232, -3.1877108683612363 },
   { 13,8,  8,5,  5,3, 1, 248, -0.52534465045617651 },
   { 13,3,  8,5,  5,7, 0, 200, 1.5192968781745577 },
   { 13,3,  8,5,  5,7, 0, 216, 3.5702294497672966 },
   { 13,3,  8,5,  5,7, 0, 232, 0.92305983774409128 },
   { 13,3,  8,5,  5,7, 0, 248, 0.88035625596147327 },
   { 13,3,  8,5,  5,7, 1, 200, -1.2408161701438019 },
   { 13,3,  8,5,  5,7, 1, 215, -1.7056289445588533 },
   { 13,3,  8,5,  5,7, 1, 231, -0.63548362118110902 },
   { 13,3,  8,5,  5,7, 1, 246, -0.4583224481821162 },
   {  5,8,  8,5, 13,3, 0, 200, 0.17652610729753349 },
   {  5,8,  8,5, 13,3, 0, 215, 0.81886570960810445 },
   {  5,8,  8,5, 13,3, 0, 231, 0.40778054661817009 },
   {  5,8,  8,5, 13,3, 0, 246, 0.51194448312703855 },
   {  5,8,  8,5, 13,3, 1, 200, -1.5192968781745577 },
   {  5,8,  8,5, 13,3, 1, 216, -3.5702294497672966 },
   {  5,8,  8,5, 13,3, 1, 232, -0.92305983774409128 },
   {  5,8,  8,5, 13,3, 1, 248, -0.88035625596147327 }
};
#define GTR_NB_TAIL ((int)(sizeof(gtrTail)/sizeof(gtrTail[0])))

/* Leg 1: periods x shifts x 2 outputs, one memcmp each. */
#define GTR_COMPOSE_CMP  (GTR_NB_PERIODS*GTR_NB_SHIFTS*2)
/* Leg 2: per period triple and setting, _Lookback and the call's range, then
 * the two outputs. */
#define GTR_UNST_CMP     (GTR_NB_PERIODS*GTR_NB_UNST*(2+2))
/* Leg 3: both outputs at all periods 1 and shifts 0, then upper with jaw equal
 * to teeth, then lower with teeth equal to lips. */
#define GTR_ZERO_CMP     4
/* Leg 4: the empty range, two outputs of the one-bar range, then two outputs
 * at each of two late starts. */
#define GTR_EDGE_CMP     7
/* Leg 5: shifts x 2 outputs. */
#define GTR_SHIFT_CMP    (GTR_NB_SHIFTS*2)
/* Leg 6: 2 output buffers x 2 inputs. */
#define GTR_ALIAS_CMP    4
/* Leg 7: one per row. */
#define GTR_GOLDEN_CMP   (GTR_NB_P1+GTR_NB_TAIL)

static int g_gtrComposeCmp;
static int g_gtrUnstCmp;
static int g_gtrZeroCmp;
static int g_gtrEdgeCmp;
static int g_gtrShiftCmp;
static int g_gtrAliasCmp;
static int g_gtrGoldenCmp;

static ErrorNumber test_gtr_compose ( const TA_History *history );
static ErrorNumber test_gtr_unst    ( const TA_History *history );
static ErrorNumber test_gtr_zero    ( const TA_History *history );
static ErrorNumber test_gtr_edges   ( const TA_History *history );
static ErrorNumber test_gtr_shift   ( void );
static ErrorNumber test_gtr_aliasing( const TA_History *history );
static ErrorNumber test_gtr_golden  ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_gator( TA_History *history )
{
   ErrorNumber retValue;
   unsigned int savedUnst;

   g_gtrComposeCmp = 0;
   g_gtrUnstCmp = 0;
   g_gtrZeroCmp = 0;
   g_gtrEdgeCmp = 0;
   g_gtrShiftCmp = 0;
   g_gtrAliasCmp = 0;
   g_gtrGoldenCmp = 0;

   if( history->nbBars != GTR_NB_BAR )
   {
      printf( "Fail: TA_GATOR expects the %d-bar corpus, got %d\n",
              GTR_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   /* DO_TEST does not reset unstable periods, and an earlier group may have
    * left one set: every leg below assumes 0 unless it sets one itself. */
   savedUnst = TA_GetUnstablePeriod( TA_FUNC_UNST_RMA );
   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );

   retValue = test_gtr_compose( history );
   if( retValue == TA_TEST_PASS ) retValue = test_gtr_unst( history );
   if( retValue == TA_TEST_PASS ) retValue = test_gtr_zero( history );
   if( retValue == TA_TEST_PASS ) retValue = test_gtr_edges( history );
   if( retValue == TA_TEST_PASS ) retValue = test_gtr_shift();
   if( retValue == TA_TEST_PASS ) retValue = test_gtr_aliasing( history );
   if( retValue == TA_TEST_PASS ) retValue = test_gtr_golden( history );

   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, savedUnst );

   if( retValue != TA_TEST_PASS )
      return retValue;

   if( g_gtrComposeCmp != GTR_COMPOSE_CMP
    || g_gtrUnstCmp    != GTR_UNST_CMP
    || g_gtrZeroCmp    != GTR_ZERO_CMP
    || g_gtrEdgeCmp    != GTR_EDGE_CMP
    || g_gtrShiftCmp   != GTR_SHIFT_CMP
    || g_gtrAliasCmp   != GTR_ALIAS_CMP
    || g_gtrGoldenCmp  != GTR_GOLDEN_CMP )
   {
      printf( "Fail: TA_GATOR comparison counts (compose %d, unst %d, zero %d, "
              "edge %d, shift %d, alias %d, golden %d) are not what this file "
              "asserts (%d, %d, %d, %d, %d, %d, %d)\n",
              g_gtrComposeCmp, g_gtrUnstCmp, g_gtrZeroCmp, g_gtrEdgeCmp,
              g_gtrShiftCmp, g_gtrAliasCmp, g_gtrGoldenCmp,
              GTR_COMPOSE_CMP, GTR_UNST_CMP, GTR_ZERO_CMP, GTR_EDGE_CMP,
              GTR_SHIFT_CMP, GTR_ALIAS_CMP, GTR_GOLDEN_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.    ****/

static int gtrBack( int shift, int other )
{
   return shift > other ? shift - other : 0;
}

/* The lookback restated from the definition: each leg's period - 1 + U plus
 * its read-back distance, the longest of the four. */
static int gtrExpectedLookback( const GtrPeriods *p, const GtrShifts *s, int unst )
{
   int leg[4], i, longest;

   leg[0] = p->jawP   - 1 + unst + gtrBack( s->jawS,   s->teethS );
   leg[1] = p->teethP - 1 + unst + gtrBack( s->teethS, s->jawS   );
   leg[2] = p->teethP - 1 + unst + gtrBack( s->teethS, s->lipsS  );
   leg[3] = p->lipsP  - 1 + unst + gtrBack( s->lipsS,  s->teethS );

   longest = leg[0];
   for( i = 1; i < 4; i++ )
      if( leg[i] > longest )
         longest = leg[i];
   return longest;
}

/* One leg of one histogram: TA_RMA of the median entered `back` bars before
 * the call's outBegIdx, which is where that leg's seed window is anchored. */
static ErrorNumber gtrLeg( const TA_History *history, const TA_Real *med,
                           int period, int back, TA_Integer begIdx,
                           TA_Integer nbElement, TA_Real *leg )
{
   TA_RetCode rc;
   TA_Integer rBeg, rNb;
   int nb = (int)history->nbBars;

   rc = TA_RMA( begIdx - back, nb - 1 - back, med, period, &rBeg, &rNb, leg );
   if( rc != TA_SUCCESS || rBeg != begIdx - back || rNb != nbElement )
   {
      printf( "Fail: TA_GATOR reference TA_RMA(%d) back %d rc=%d range %d/%d, "
              "Gator %d/%d\n", period, back, (int)rc, (int)rBeg, (int)rNb,
              (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   return TA_TEST_PASS;
}

/* Both histograms of one call against their four reference legs. Adds one to
 * *counter per memcmp. */
static ErrorNumber gtrCheckComposition( const TA_History *history,
                                        const GtrPeriods *p, const GtrShifts *s,
                                        TA_Integer begIdx, TA_Integer nbElement,
                                        const TA_Real *outU, const TA_Real *outL,
                                        int *counter )
{
   static TA_Real med[GTR_NB_BAR], a[GTR_NB_BAR], b[GTR_NB_BAR];
   static TA_Real refU[GTR_NB_BAR], refL[GTR_NB_BAR];
   TA_RetCode rc;
   TA_Integer mBeg, mNb;
   ErrorNumber e;
   int nb, k;

   nb = (int)history->nbBars;

   rc = TA_MEDPRICE( 0, nb-1, history->high, history->low, &mBeg, &mNb, med );
   if( rc != TA_SUCCESS || mBeg != 0 || mNb != nb )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   e = gtrLeg( history, med, p->jawP, gtrBack( s->jawS, s->teethS ),
               begIdx, nbElement, a );
   if( e != TA_TEST_PASS ) return e;
   e = gtrLeg( history, med, p->teethP, gtrBack( s->teethS, s->jawS ),
               begIdx, nbElement, b );
   if( e != TA_TEST_PASS ) return e;
   for( k = 0; k < nbElement; k++ )
      refU[k] = fabs( a[k] - b[k] );

   e = gtrLeg( history, med, p->teethP, gtrBack( s->teethS, s->lipsS ),
               begIdx, nbElement, a );
   if( e != TA_TEST_PASS ) return e;
   e = gtrLeg( history, med, p->lipsP, gtrBack( s->lipsS, s->teethS ),
               begIdx, nbElement, b );
   if( e != TA_TEST_PASS ) return e;
   for( k = 0; k < nbElement; k++ )
      refL[k] = ( a[k] < b[k] ) ? a[k] - b[k] : b[k] - a[k];

   if( memcmp( outU, refU, (size_t)nbElement * sizeof(double) ) != 0 )
   {
      printf( "Fail: TA_GATOR %d,%d,%d,%d,%d,%d outUpper differs from "
              "|TA_RMA(jaw) - TA_RMA(teeth)| read back from bar %d\n",
              p->jawP, s->jawS, p->teethP, s->teethS, p->lipsP, s->lipsS,
              (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   (*counter)++;

   if( memcmp( outL, refL, (size_t)nbElement * sizeof(double) ) != 0 )
   {
      printf( "Fail: TA_GATOR %d,%d,%d,%d,%d,%d outLower differs from "
              "-|TA_RMA(teeth) - TA_RMA(lips)| read back from bar %d\n",
              p->jawP, s->jawS, p->teethP, s->teethS, p->lipsP, s->lipsS,
              (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   (*counter)++;

   return TA_TEST_PASS;
}

/* (1) Composition, at every shift ordering. */
static ErrorNumber test_gtr_compose( const TA_History *history )
{
   static TA_Real outU[GTR_NB_BAR], outL[GTR_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   int t, s, nb, want;

   nb = (int)history->nbBars;

   for( t = 0; t < GTR_NB_PERIODS; t++ )
   {
      const GtrPeriods *p = &gtrPeriods[t];

      for( s = 0; s < GTR_NB_SHIFTS; s++ )
      {
         const GtrShifts *sh = &gtrShifts[s];

         want = gtrExpectedLookback( p, sh, 0 );
         rc = TA_GATOR( 0, nb-1, history->high, history->low,
                        p->jawP, sh->jawS, p->teethP, sh->teethS,
                        p->lipsP, sh->lipsS,
                        &begIdx, &nbElement, outU, outL );
         if( rc != TA_SUCCESS || begIdx != want || nbElement != nb - want )
         {
            printf( "Fail: TA_GATOR %d,%d,%d,%d,%d,%d rc=%d range %d/%d "
                    "(want %d/%d)\n", p->jawP, sh->jawS, p->teethP, sh->teethS,
                    p->lipsP, sh->lipsS, (int)rc, (int)begIdx, (int)nbElement,
                    want, nb - want );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         /* Most period triples move two or three parameters at once, and
          * every triple here moves all three shifts, which the --codegen
          * sweep never sends. */
         if( s == 1 && server_verify_active() )
         {
            const double opt[6] = { (double)p->jawP, (double)sh->jawS,
                                    (double)p->teethP, (double)sh->teethS,
                                    (double)p->lipsP, (double)sh->lipsS };
            e = server_verify( "GATOR", 0, nb-1, nb,
                               rc, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low, NULL },
                               opt, 6,
                               (const TA_Real*[]){ outU, outL, NULL },
                               NULL );
            if( e != TA_TEST_PASS )
               return e;
         }

         e = gtrCheckComposition( history, p, sh, begIdx, nbElement, outU, outL,
                                  &g_gtrComposeCmp );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (2) TA_FUNC_UNST_RMA reaches all four legs through the one id. */
static ErrorNumber test_gtr_unst( const TA_History *history )
{
   static TA_Real outU[GTR_NB_BAR], outL[GTR_NB_BAR];
   const GtrShifts *sh = &gtrShifts[1];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   int t, u, nb, want, got;

   nb = (int)history->nbBars;

   for( u = 0; u < GTR_NB_UNST; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, (unsigned int)gtrUnst[u] );

      for( t = 0; t < GTR_NB_PERIODS; t++ )
      {
         const GtrPeriods *p = &gtrPeriods[t];

         want = gtrExpectedLookback( p, sh, gtrUnst[u] );
         got = TA_GATOR_Lookback( p->jawP, sh->jawS, p->teethP, sh->teethS,
                                  p->lipsP, sh->lipsS );
         if( got != want )
         {
            printf( "Fail: TA_GATOR_Lookback(%d,%d,%d) at unstable %d = %d, "
                    "expected %d\n", p->jawP, p->teethP, p->lipsP, gtrUnst[u],
                    got, want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_gtrUnstCmp++;

         rc = TA_GATOR( 0, nb-1, history->high, history->low,
                        p->jawP, sh->jawS, p->teethP, sh->teethS,
                        p->lipsP, sh->lipsS,
                        &begIdx, &nbElement, outU, outL );
         if( rc != TA_SUCCESS || begIdx != want || nbElement != nb - want )
         {
            printf( "Fail: TA_GATOR(%d,%d,%d) at unstable %d rc=%d range "
                    "%d/%d, lookback %d\n", p->jawP, p->teethP, p->lipsP,
                    gtrUnst[u], (int)rc, (int)begIdx, (int)nbElement, want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         g_gtrUnstCmp++;

         e = gtrCheckComposition( history, p, sh, begIdx, nbElement, outU, outL,
                                  &g_gtrUnstCmp );
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

/* (3) A zero spread is +0.0. memcmp, because -0.0 == +0.0. */
static ErrorNumber test_gtr_zero( const TA_History *history )
{
   static TA_Real outU[GTR_NB_BAR], outL[GTR_NB_BAR], zero[GTR_NB_BAR];
   static const struct { int p[6]; int output; } zcase[3] =
   {
      { { 1, 0, 1, 0, 1, 0 }, -1 },   /* every line the median: both outputs */
      { { 8, 5, 8, 5, 5, 3 },  0 },   /* jaw is the teeth: upper */
      { { 13, 8, 5, 3, 5, 3 }, 1 }    /* teeth is the lips: lower */
   };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int c, nb;
   size_t bytes;

   nb = (int)history->nbBars;
   memset( zero, 0, sizeof(zero) );

   for( c = 0; c < 3; c++ )
   {
      const int *p = zcase[c].p;

      rc = TA_GATOR( 0, nb-1, history->high, history->low,
                     p[0], p[1], p[2], p[3], p[4], p[5],
                     &begIdx, &nbElement, outU, outL );
      if( rc != TA_SUCCESS || nbElement <= 0 )
      {
         printf( "Fail: TA_GATOR zero case %d rc=%d range %d/%d\n", c, (int)rc,
                 (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      bytes = (size_t)nbElement * sizeof(double);

      if( zcase[c].output != 1 )
      {
         if( memcmp( outU, zero, bytes ) != 0 )
         {
            printf( "Fail: TA_GATOR zero case %d outUpper is not +0.0 on every "
                    "bar\n", c );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_gtrZeroCmp++;
      }
      if( zcase[c].output != 0 )
      {
         if( memcmp( outL, zero, bytes ) != 0 )
         {
            printf( "Fail: TA_GATOR zero case %d outLower is not +0.0 on every "
                    "bar\n", c );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_gtrZeroCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (4) The range edges. */
static ErrorNumber test_gtr_edges( const TA_History *history )
{
   static TA_Real fullU[GTR_NB_BAR], fullL[GTR_NB_BAR];
   static const GtrPeriods late[2] = { { 13, 8, 5 }, { 13, 8, 40 } };
   static const int lateStart[2] = { 50, 100 };
   const GtrShifts *sh = &gtrShifts[0];
   TA_Real oneU[1], oneL[1];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, fBeg, fNb;
   ErrorNumber e;
   int lookback, t, nb;

   nb = (int)history->nbBars;
   lookback = TA_GATOR_Lookback( 13, 8, 8, 5, 5, 3 );

   begIdx = -1;
   nbElement = -1;
   rc = TA_GATOR( 0, lookback - 1, history->high, history->low,
                  13, 8, 8, 5, 5, 3, &begIdx, &nbElement, oneU, oneL );
   if( rc != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "Fail: TA_GATOR endIdx %d below lookback %d: rc=%d range %d/%d "
              "(want 0/0)\n", lookback - 1, lookback, (int)rc, (int)begIdx,
              (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_gtrEdgeCmp++;

   rc = TA_GATOR( 0, nb - 1, history->high, history->low,
                  13, 8, 8, 5, 5, 3, &fBeg, &fNb, fullU, fullL );
   if( rc != TA_SUCCESS || fBeg != lookback )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   /* At startIdx == lookback every leg seeds where the full call seeds it. */
   rc = TA_GATOR( lookback, lookback, history->high, history->low,
                  13, 8, 8, 5, 5, 3, &begIdx, &nbElement, oneU, oneL );
   if( rc != TA_SUCCESS || begIdx != lookback || nbElement != 1 )
   {
      printf( "Fail: TA_GATOR one-bar range at %d: rc=%d range %d/%d\n",
              lookback, (int)rc, (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( memcmp( oneU, fullU, sizeof(double) ) != 0 ) return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   g_gtrEdgeCmp++;
   if( memcmp( oneL, fullL, sizeof(double) ) != 0 ) return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   g_gtrEdgeCmp++;

   for( t = 0; t < 2; t++ )
   {
      rc = TA_GATOR( lateStart[t], nb - 1, history->high, history->low,
                     late[t].jawP, sh->jawS, late[t].teethP, sh->teethS,
                     late[t].lipsP, sh->lipsS,
                     &begIdx, &nbElement, fullU, fullL );
      if( rc != TA_SUCCESS || begIdx != lateStart[t]
          || nbElement != nb - lateStart[t] )
      {
         printf( "Fail: TA_GATOR startIdx %d rc=%d range %d/%d\n",
                 lateStart[t], (int)rc, (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      e = gtrCheckComposition( history, &late[t], sh, begIdx, nbElement,
                               fullU, fullL, &g_gtrEdgeCmp );
      if( e != TA_TEST_PASS )
         return e;
   }

   return TA_TEST_PASS;
}

/* (5) Each histogram is drawn at the smaller shift of its two lines. */
static ErrorNumber test_gtr_shift( void )
{
   int s, o, want, got;

   for( s = 0; s < GTR_NB_SHIFTS; s++ )
   {
      const GtrShifts *sh = &gtrShifts[s];

      for( o = 0; o < 2; o++ )
      {
         if( o == 0 )
            want = sh->jawS < sh->teethS ? sh->jawS : sh->teethS;
         else
            want = sh->teethS < sh->lipsS ? sh->teethS : sh->lipsS;
         got = TA_GATOR_DisplayShift( 13, sh->jawS, 8, sh->teethS,
                                      5, sh->lipsS, o );
         if( got != want )
         {
            printf( "Fail: TA_GATOR_DisplayShift(13,%d,8,%d,5,%d,%d) = %d, "
                    "expected %d\n", sh->jawS, sh->teethS, sh->lipsS, o,
                    got, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_gtrShiftCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (6) Each output buffer over each input. */
static ErrorNumber test_gtr_aliasing( const TA_History *history )
{
   static TA_Real refU[GTR_NB_BAR], refL[GTR_NB_BAR];
   static TA_Real workH[GTR_NB_BAR], workL[GTR_NB_BAR], s0[GTR_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   const TA_Real *ref[2];
   TA_Real *buf[2];
   int nb, i, whichOut, whichIn, j;

   nb = (int)history->nbBars;

   rc = TA_GATOR( 0, nb-1, history->high, history->low, 13, 8, 8, 5, 5, 3,
                  &begIdx, &nbElement, refU, refL );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   ref[0] = refU; ref[1] = refL;

   for( whichOut = 0; whichOut < 2; whichOut++ )
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
         buf[whichOut] = aliased;
         buf[1 - whichOut] = s0;

         rc = TA_GATOR( 0, nb-1, workH, workL, 13, 8, 8, 5, 5, 3,
                        &begIdx2, &nbElement2, buf[0], buf[1] );
         if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "Fail: TA_GATOR aliasing output %d over %s rc=%d\n",
                    whichOut, ( whichIn == 0 ) ? "inHigh" : "inLow", (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( j = 0; j < 2; j++ )
         {
            if( memcmp( buf[j], ref[j], (size_t)nbElement * sizeof(double) ) != 0 )
            {
               printf( "Fail: TA_GATOR aliasing output %d over %s changed "
                       "output %d\n", whichOut,
                       ( whichIn == 0 ) ? "inHigh" : "inLow", j );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         g_gtrAliasCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* (7) Values from outside TA-Lib. */
static ErrorNumber gtrCheckGolden( const TA_History *history,
                                   const GtrGolden *g, int nbRow, double tol,
                                   const char *what )
{
   static TA_Real out[2][GTR_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int r, k;
   double got;

   for( r = 0; r < nbRow; r++ )
   {
      rc = TA_GATOR( 0, (int)history->nbBars-1, history->high, history->low,
                     g[r].jawP, g[r].jawS, g[r].teethP, g[r].teethS,
                     g[r].lipsP, g[r].lipsS, &begIdx, &nbElement,
                     out[0], out[1] );
      k = g[r].bar - (int)begIdx;
      if( rc != TA_SUCCESS || k < 0 || k >= (int)nbElement )
      {
         printf( "Fail: TA_GATOR %s golden row %d rc=%d range %d/%d does not "
                 "hold bar %d\n", what, r, (int)rc, (int)begIdx,
                 (int)nbElement, g[r].bar );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      got = out[g[r].output][k];
      if( !(fabs( got - g[r].value ) <= tol) )
      {
         printf( "Fail: TA_GATOR %s golden %d,%d,%d,%d,%d,%d output %d bar %d "
                 "got %.17g, trading-signals %.17g (abs %.3g > %.3g)\n",
                 what, g[r].jawP, g[r].jawS, g[r].teethP, g[r].teethS,
                 g[r].lipsP, g[r].lipsS, g[r].output, g[r].bar, got,
                 g[r].value, fabs( got - g[r].value ), tol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_gtrGoldenCmp++;
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_gtr_golden( const TA_History *history )
{
   ErrorNumber e;

   e = gtrCheckGolden( history, gtrP1, GTR_NB_P1, GTR_P1_TOL, "period-1" );
   if( e != TA_TEST_PASS )
      return e;

   return gtrCheckGolden( history, gtrTail, GTR_NB_TAIL, GTR_TAIL_TOL, "tail" );
}
