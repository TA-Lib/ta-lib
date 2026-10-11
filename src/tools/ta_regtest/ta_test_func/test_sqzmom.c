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
 *  100826 KL,CC  Creation (#488).
 *  101126 MF,CC  Factor order, sub-ranges, short series, range test (#488).
 */

/* Description:
 *
 *   Test TA_SQZMOM.
 *
 *   Every leg of SQZMOM is a shipped function, so this file adds no numerical
 *   logic: it composes the card's formula out of the SAME public functions and
 *   requires the two to agree BIT FOR BIT. The reference deliberately calls
 *   TA_BBANDS, the function the card's formula names, while the implementation
 *   takes the MA + STDDEV pair instead -- so leg 1 tests that substitution
 *   rather than assuming it.
 *
 *   Legs:
 *     1. COMPOSITION. The formula through TA_BBANDS, TA_SMA, TA_TRANGE,
 *        TA_MIDPRICE and TA_LINEARREG, over five parameter triples. Bit for
 *        bit on both outputs. Also the leg that drives server_verify.
 *     2. THE ORDINAL AT ITS BOUNDARY. Leg 1's factors put every bar far from a
 *        Keltner edge, so the ordinal comparison there has almost no
 *        discrimination: scaling the band by 1+1e-7 moves not one bar. This
 *        leg sets the normal factor to a value read off the data itself --
 *        (BBU - KCM)/BAND at a chosen bar -- so that bar sits ON the edge, and
 *        asserts both that some bar really is within a few ulps of it (or the
 *        leg proved nothing) and that the two still agree. Tuned this way the
 *        comparison detects a relative band error of 6.2e-15.
 *     3. LOOKBACK. TA_SQZMOM_Lookback against the max of the four chains, and
 *        the call's own begIdx against it, at the triples the card measured
 *        against pandas, ta4j and trading-signals: 38 at (20,20), 26 at
 *        (20,14), 38 at (14,20).
 *     4. NESTING. The three channels share a centre and BAND >= 0, so they
 *        nest. Every bar reporting 3 must also satisfy the normal and wide
 *        tests, every 2 the wide test, and -1 must fail the wide test on both
 *        sides. A level the nesting forbids is a mis-ordered ladder.
 *     5. FLAT. A constant series has BAND == 0 and BBU == BBL == KCM, so no
 *        test is strictly satisfied on either side and every bar reads 0.
 *     6. ALIASING. Each output over each input in turn. The abstract layer
 *        caught this one: the body must read every bar input before it writes
 *        a caller buffer, which is why TA_LINEARREG runs last.
 *     7. EDGES. A period below its range and factors out of order are
 *        rejected; nbdev == 0 collapses the bands onto the middle band, which
 *        is inside every channel of positive width, so every bar reads 3; a
 *        series one bar short gives no output and the next bar gives one.
 *     8. SUB-RANGE. The composition again, entered above the lookback and
 *        ended before the last bar. Four legs keep running sums, so the
 *        reference starts each of them where the implementation does.
 *     9. RANGE INDEPENDENCE, swept via doRangeTestEx.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define SQZ_NB_BAR 252

/* A leg that compares nothing passes silently without these counts.
 * 11 cases; the bars each produces are 214, 226, 214, 250, 203, 233, 233, 212,
 * 54, 214 and 214, which is 2267, and each bar compares both outputs. */
#define SQZ_COMPOSE_CMP   4534
#define SQZ_BOUNDARY_CMP   642   /* 3 tuned runs x 214 bars */
/* One bar per tuned run lands on the edge -- the bar the factor was read from.
 * The floor is what stops the leg passing when the tuning lands nowhere near
 * one, which would make it leg 1 again. */
#define SQZ_BOUNDARY_EDGE    3
#define SQZ_LOOKBACK_CMP    23   /* 11 cases x (lookback, begIdx), and the unstable-period check */
#define SQZ_NEST_CMP      2267   /* the same 2267 bars, one check each */
#define SQZ_FLAT_CMP       214
#define SQZ_ALIAS_CMP        6   /* 2 outputs x 3 inputs */
#define SQZ_EDGE_CMP       229   /* 3 return codes, the 214 bars of the nbDev=0 run, 8 on factor order, 4 on short series */
#define SQZ_RANGE_CMP      590   /* 152 and 143 bars, both outputs */

static int g_sqzComposeCmp;
static int g_sqzBoundaryCmp;
static int g_sqzBoundaryEdge;
static int g_sqzLookbackCmp;
static int g_sqzNestCmp;
static int g_sqzFlatCmp;
static int g_sqzAliasCmp;
static int g_sqzEdgeCmp;
static int g_sqzRangeCmp;

typedef struct
{
   int    bbPeriod;
   double nbDev;
   int    kcPeriod;
   double wide, normal, narrow;
   int    wantLookback;      /* -1: do not check */
} SqzCase;

/* The three the card measured against pandas / ta4j / trading-signals, plus the
 * shortest periods the ranges allow and a pair where the band leg dominates. */
static const SqzCase sqzCases[] =
{
   { 20, 2.0, 20, 2.0, 1.5, 1.0, 38 },
   { 20, 2.0, 14, 2.0, 1.5, 1.0, 26 },
   { 14, 2.0, 20, 2.0, 1.5, 1.0, 38 },
   {  2, 2.0,  2, 2.0, 1.5, 1.0,  2 },
   { 50, 2.5,  9, 3.0, 1.5, 0.5, 49 },
   { 20, 2.0,  3, 2.0, 1.5, 1.0, 19 },
   { 20, 2.0,  4, 2.0, 1.5, 1.0, 19 },
   { 20, 2.0, 21, 2.0, 1.5, 1.0, 40 },
   { 20, 2.0,100, 2.0, 1.5, 1.0,198 },
   { 20, 2.0, 20, 1.5, 1.5, 1.0, 38 },   /* wide == normal: the classic squeeze */
   { 20, 2.0, 20, 2.0, 1.5, 1.5, 38 },   /* normal == narrow */
};
#define SQZ_NB_CASE ((int)(sizeof(sqzCases)/sizeof(sqzCases[0])))

/* Scratch for the reference composition. */
static TA_Real refBBU[SQZ_NB_BAR], refBBM[SQZ_NB_BAR], refBBL[SQZ_NB_BAR];
static TA_Real refKCM[SQZ_NB_BAR], refBand[SQZ_NB_BAR], refStage[SQZ_NB_BAR];
static TA_Real refDev[SQZ_NB_BAR], refMom[SQZ_NB_BAR];
static TA_Integer refSqz[SQZ_NB_BAR];
static TA_Real gotMom[SQZ_NB_BAR];
static TA_Integer gotSqz[SQZ_NB_BAR];

/**** Local functions declarations.    ****/

static ErrorNumber sqzReference( const TA_Real *high, const TA_Real *low,
                                 const TA_Real *close, int nbBars,
                                 const SqzCase *c, int startIdx,
                                 int *refBeg, int *refNb );
static ErrorNumber sqzLegCompose( TA_History *history );
static ErrorNumber sqzLegBoundary( TA_History *history );
static ErrorNumber sqzLegLookback( TA_History *history );
static ErrorNumber sqzLegNesting( TA_History *history );
static ErrorNumber sqzLegFlat( void );
static ErrorNumber sqzLegAlias( TA_History *history );
static ErrorNumber sqzLegEdges( TA_History *history );
static ErrorNumber sqzLegSubRange( TA_History *history );
static ErrorNumber sqzLegRange( TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_sqzmom( TA_History *history )
{
   ErrorNumber e;

   if( (int)history->nbBars != SQZ_NB_BAR )
   {
      printf( "Fail: SQZMOM expects %d bars, history has %d\n",
              SQZ_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   g_sqzComposeCmp = g_sqzBoundaryCmp = g_sqzBoundaryEdge = 0;
   g_sqzLookbackCmp = g_sqzNestCmp = g_sqzFlatCmp = 0;
   g_sqzAliasCmp = g_sqzEdgeCmp = g_sqzRangeCmp = 0;

   if( (e = sqzLegCompose ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegBoundary( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegLookback( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegNesting ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegFlat    (         )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegAlias   ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegEdges   ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegSubRange( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegRange   ( history )) != TA_TEST_PASS ) return e;

   /* Every leg must have compared what it says it compares. Without these a
    * leg that silently stopped early reads exactly like a leg that passed. */
   if( g_sqzComposeCmp  != SQZ_COMPOSE_CMP  ||
       g_sqzBoundaryCmp != SQZ_BOUNDARY_CMP ||
       g_sqzBoundaryEdge < SQZ_BOUNDARY_EDGE ||
       g_sqzLookbackCmp != SQZ_LOOKBACK_CMP ||
       g_sqzNestCmp     != SQZ_NEST_CMP     ||
       g_sqzFlatCmp     != SQZ_FLAT_CMP     ||
       g_sqzAliasCmp    != SQZ_ALIAS_CMP    ||
       g_sqzEdgeCmp     != SQZ_EDGE_CMP     ||
       g_sqzRangeCmp    != SQZ_RANGE_CMP )
   {
      printf( "Fail: SQZMOM comparison counts %d/%d/%d/%d/%d/%d/%d/%d/%d, expected "
              "%d/%d/>=%d/%d/%d/%d/%d/%d/%d\n",
              g_sqzComposeCmp, g_sqzBoundaryCmp, g_sqzBoundaryEdge,
              g_sqzLookbackCmp, g_sqzNestCmp, g_sqzFlatCmp, g_sqzAliasCmp,
              g_sqzEdgeCmp, g_sqzRangeCmp,
              SQZ_COMPOSE_CMP, SQZ_BOUNDARY_CMP, SQZ_BOUNDARY_EDGE,
              SQZ_LOOKBACK_CMP, SQZ_NEST_CMP, SQZ_FLAT_CMP, SQZ_ALIAS_CMP,
              SQZ_EDGE_CMP, SQZ_RANGE_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* The card's formula, every leg a public call, nothing of its own but the two
 * lines the card writes out. Fills refMom/refSqz, 0-based at *refBeg. The
 * output starts at startIdx or at the lookback, whichever is later. */
static ErrorNumber sqzReference( const TA_Real *high, const TA_Real *low,
                                 const TA_Real *close, int nbBars,
                                 const SqzCase *c, int startIdx,
                                 int *refBeg, int *refNb )
{
   TA_RetCode rc;
   TA_Integer b1, n1;
   int i, beg, nb, stage, stageLen, leg;

   beg = c->bbPeriod - 1;
   leg = 1 + (c->kcPeriod - 1);
   if( leg > beg ) beg = leg;
   leg = 2 * (c->kcPeriod - 1);
   if( leg > beg ) beg = leg;
   if( startIdx > beg ) beg = startIdx;

   nb = nbBars - beg;
   stage = beg - (c->kcPeriod - 1);
   stageLen = nbBars - stage;

   rc = TA_BBANDS( beg, nbBars-1, close, c->bbPeriod, c->nbDev, c->nbDev,
                   TA_MAType_SMA, &b1, &n1, refBBU, refBBM, refBBL );
   if( rc != TA_SUCCESS || n1 != nb ) goto refFail;

   rc = TA_SMA( beg, nbBars-1, close, c->kcPeriod, &b1, &n1, refKCM );
   if( rc != TA_SUCCESS || n1 != nb ) goto refFail;

   rc = TA_TRANGE( stage, nbBars-1, high, low, close, &b1, &n1, refBand );
   if( rc != TA_SUCCESS || n1 != stageLen ) goto refFail;
   rc = TA_SMA( 0, stageLen-1, refBand, c->kcPeriod, &b1, &n1, refBand );
   if( rc != TA_SUCCESS || n1 != nb ) goto refFail;

   rc = TA_MIDPRICE( stage, nbBars-1, high, low, c->kcPeriod, &b1, &n1, refDev );
   if( rc != TA_SUCCESS || n1 != stageLen ) goto refFail;
   rc = TA_SMA( stage, nbBars-1, close, c->kcPeriod, &b1, &n1, refStage );
   if( rc != TA_SUCCESS || n1 != stageLen ) goto refFail;

   for( i = 0; i < stageLen; i++ )
      refDev[i] = close[stage+i] - (refDev[i] + refStage[i]) * 0.5;

   rc = TA_LINEARREG( 0, stageLen-1, refDev, c->kcPeriod, &b1, &n1, refMom );
   if( rc != TA_SUCCESS || n1 != nb ) goto refFail;

   for( i = 0; i < nb; i++ )
   {
      double up = refBBU[i], lo = refBBL[i], ce = refKCM[i], bd = refBand[i];

      if( lo > ce - c->narrow*bd && up < ce + c->narrow*bd )
         refSqz[i] = 3;
      else if( lo > ce - c->normal*bd && up < ce + c->normal*bd )
         refSqz[i] = 2;
      else if( lo > ce - c->wide*bd && up < ce + c->wide*bd )
         refSqz[i] = 1;
      else if( lo < ce - c->wide*bd && up > ce + c->wide*bd )
         refSqz[i] = -1;
      else
         refSqz[i] = 0;
   }

   *refBeg = beg;
   *refNb  = nb;
   return TA_TEST_PASS;

refFail:
   printf( "Fail: SQZMOM reference leg failed (b=%d k=%d) rc=%d nb=%d\n",
           c->bbPeriod, c->kcPeriod, (int)rc, (int)n1 );
   return TA_TESTUTIL_TFRR_BAD_RETCODE;
}

/* Leg 1. The composition, bit for bit, and the only leg that reaches a server. */
static ErrorNumber sqzLegCompose( TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, i, refBeg, refNb, nbBars;
   ErrorNumber e;

   nbBars = (int)history->nbBars;

   for( k = 0; k < SQZ_NB_CASE; k++ )
   {
      const SqzCase *c = &sqzCases[k];
      int cmpBefore = 0;

      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      c->bbPeriod, c->nbDev, c->kcPeriod,
                      c->wide, c->normal, c->narrow,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_SQZMOM b=%d k=%d rc=%d\n",
                 c->bbPeriod, c->kcPeriod, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( (e = sqzReference( history->high, history->low, history->close,
                             nbBars, c, 0, &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      if( begIdx != refBeg || nbElement != refNb )
      {
         printf( "Fail: TA_SQZMOM b=%d k=%d begIdx %d/%d nbElement %d/%d\n",
                 c->bbPeriod, c->kcPeriod, (int)begIdx, refBeg,
                 (int)nbElement, refNb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      /* Once per case, the same call through every language server. Without
       * this the group runs under --codegen having compared nothing against
       * any server, which --codegen reports as a failure. */
      cmpBefore = server_verify_comparisons();
      if( server_verify_active() )
      {
         const double opt[6] = { (double)c->bbPeriod, c->nbDev,
                                 (double)c->kcPeriod,
                                 c->wide, c->normal, c->narrow };
         e = server_verify( "SQZMOM", 0, nbBars-1, nbBars,
                rc, begIdx, nbElement,
                (const TA_Real*[]){ history->high, history->low,
                                    history->close, NULL },
                opt, 6,
                (const TA_Real*[]){ gotMom, NULL },
                (const TA_Integer*[]){ gotSqz, NULL } );
         if( e != TA_TEST_PASS )
            return e;
         /* "No failure reported" and "nothing was compared" are the same
          * observation without this. */
         if( server_verify_comparisons() == cmpBefore )
         {
            printf( "SQZMOM [b=%d k=%d]: compared no server despite a live "
                    "session\n", c->bbPeriod, c->kcPeriod );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( memcmp( &gotMom[i], &refMom[i], sizeof(TA_Real) ) != 0 )
         {
            printf( "Fail: TA_SQZMOM b=%d k=%d bar %d outMomentum %.17g, the "
                    "composition gives %.17g\n", c->bbPeriod, c->kcPeriod,
                    refBeg+i, gotMom[i], refMom[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzComposeCmp++;

         if( gotSqz[i] != refSqz[i] )
         {
            printf( "Fail: TA_SQZMOM b=%d k=%d bar %d outSqueeze %d, the "
                    "composition gives %d\n", c->bbPeriod, c->kcPeriod,
                    refBeg+i, (int)gotSqz[i], (int)refSqz[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzComposeCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 2. The ordinal decided at a channel edge.
 *
 * At leg 1's factors every bar sits far from an edge: scaling BAND by 1+1e-7
 * moves not one ordinal, so that leg proves the ladder is wired up and almost
 * nothing about its arithmetic. Here the normal factor is read off the data --
 * (BBU - KCM)/BAND at a chosen bar -- so that bar lies ON the upper edge. The
 * leg asserts that some bar really is within a few ulps of an edge, because a
 * tuned factor that lands nowhere near one would prove nothing either.
 */
static ErrorNumber sqzLegBoundary( TA_History *history )
{
   static const int tuneBar[3] = { 60, 120, 200 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int t, i, refBeg, refNb, nbBars;
   ErrorNumber e;
   SqzCase c;

   nbBars = (int)history->nbBars;

   for( t = 0; t < 3; t++ )
   {
      double tuned;

      c.bbPeriod = 20; c.nbDev = 2.0; c.kcPeriod = 20;
      c.wide = 2.0; c.normal = 1.5; c.narrow = 1.0; c.wantLookback = -1;

      /* First pass: read the factor at which tuneBar[t] flips its upper test. */
      if( (e = sqzReference( history->high, history->low, history->close,
                             nbBars, &c, 0, &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      i = tuneBar[t] - refBeg;
      if( i < 0 || i >= refNb || !(refBand[i] > 0.0) )
      {
         printf( "Fail: SQZMOM boundary leg, bar %d is outside the output or "
                 "has a zero band\n", tuneBar[t] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
      tuned = (refBBU[i] - refKCM[i]) / refBand[i];
      if( !(tuned > 0.0) )
      {
         printf( "Fail: SQZMOM boundary leg, bar %d gives a non-positive "
                 "factor %.17g\n", tuneBar[t], tuned );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      /* Second pass at that factor: the chosen bar now sits on the edge. */
      c.wide = tuned + 0.5; c.normal = tuned; c.narrow = tuned - 0.5;
      if( !(c.narrow > 0.0) ) c.narrow = tuned * 0.5;

      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      c.bbPeriod, c.nbDev, c.kcPeriod,
                      c.wide, c.normal, c.narrow,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_SQZMOM boundary rc=%d\n", (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( (e = sqzReference( history->high, history->low, history->close,
                             nbBars, &c, 0, &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      if( begIdx != refBeg || nbElement != refNb )
      {
         printf( "Fail: SQZMOM boundary begIdx %d/%d nbElement %d/%d\n",
                 (int)begIdx, refBeg, (int)nbElement, refNb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( i = 0; i < nbElement; i++ )
      {
         double edge, gap, ulp;

         if( gotSqz[i] != refSqz[i] )
         {
            printf( "Fail: TA_SQZMOM boundary bar %d outSqueeze %d, the "
                    "composition gives %d\n", refBeg+i, (int)gotSqz[i],
                    (int)refSqz[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzBoundaryCmp++;

         /* How many bars actually landed on an edge: without at least a few,
          * this leg is leg 1 again under another name. */
         edge = refKCM[i] + c.normal * refBand[i];
         gap  = fabs( refBBU[i] - edge );
         ulp  = fabs( edge ) * 2.220446049250313e-16;
         if( gap <= 4.0 * ulp )
            g_sqzBoundaryEdge++;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 3. The lookback is the max of the four chains, and the call starts there. */
static ErrorNumber sqzLegLookback( TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, want, nbBars;

   nbBars = (int)history->nbBars;

   for( k = 0; k < SQZ_NB_CASE; k++ )
   {
      const SqzCase *c = &sqzCases[k];
      int got = TA_SQZMOM_Lookback( c->bbPeriod, c->nbDev, c->kcPeriod,
                                    c->wide, c->normal, c->narrow );

      want = c->bbPeriod - 1;
      if( 1 + (c->kcPeriod - 1) > want ) want = 1 + (c->kcPeriod - 1);
      if( 2 * (c->kcPeriod - 1) > want ) want = 2 * (c->kcPeriod - 1);

      if( got != want || (c->wantLookback >= 0 && got != c->wantLookback) )
      {
         printf( "Fail: TA_SQZMOM_Lookback(b=%d,k=%d) = %d, expected %d"
                 " (card: %d)\n", c->bbPeriod, c->kcPeriod, got, want,
                 c->wantLookback );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzLookbackCmp++;

      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      c->bbPeriod, c->nbDev, c->kcPeriod,
                      c->wide, c->normal, c->narrow,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS || begIdx != want )
      {
         printf( "Fail: TA_SQZMOM(b=%d,k=%d) rc=%d begIdx=%d, expected %d\n",
                 c->bbPeriod, c->kcPeriod, (int)rc, (int)begIdx, want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzLookbackCmp++;
   }

   /* No leg may be routed through a function that has an unstable period. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 50 );
   k = TA_SQZMOM_Lookback( 20, 2.0, 20, 2.0, 1.5, 1.0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   if( k != 38 )
   {
      printf( "Fail: TA_SQZMOM_Lookback is %d under an unstable period of 50, "
              "expected 38\n", k );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_sqzLookbackCmp++;

   return TA_TEST_PASS;
}

/* Leg 4. The ladder nests. The three channels share a centre and BAND >= 0, so
 * a bar inside the narrow one is inside the normal and the wide one too. A
 * level the nesting forbids means the ladder was tested in the wrong order.
 */
static ErrorNumber sqzLegNesting( TA_History *history )
{
   int k, i, refBeg, refNb, nbBars;
   ErrorNumber e;

   nbBars = (int)history->nbBars;

   for( k = 0; k < SQZ_NB_CASE; k++ )
   {
      const SqzCase *c = &sqzCases[k];

      if( (e = sqzReference( history->high, history->low, history->close,
                             nbBars, c, 0, &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      for( i = 0; i < refNb; i++ )
      {
         double up = refBBU[i], lo = refBBL[i], ce = refKCM[i], bd = refBand[i];
         int inW, inN, inR, bad = 0;

         inR = ( lo > ce - c->narrow*bd && up < ce + c->narrow*bd );
         inN = ( lo > ce - c->normal*bd && up < ce + c->normal*bd );
         inW = ( lo > ce - c->wide*bd   && up < ce + c->wide*bd   );

         if( bd < 0.0 )
            bad = 1;                                   /* a mean of true ranges is never negative */
         else if( inR && !(inN && inW) )
            bad = 1;                                   /* narrow must imply normal and wide */
         else if( inN && !inW )
            bad = 1;                                   /* normal must imply wide */
         else if( refSqz[i] == 3 && !inR )
            bad = 1;
         else if( refSqz[i] == 2 && !inN )
            bad = 1;
         else if( refSqz[i] == 1 && !inW )
            bad = 1;
         else if( refSqz[i] == -1 && inW )
            bad = 1;

         if( bad )
         {
            printf( "Fail: SQZMOM nesting b=%d k=%d bar %d level %d, "
                    "inside narrow/normal/wide = %d/%d/%d, band %.17g\n",
                    c->bbPeriod, c->kcPeriod, refBeg+i, (int)refSqz[i],
                    inR, inN, inW, bd );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzNestCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 5. A constant series. BAND is 0 and the bands collapse onto the centre,
 * so no strict inequality holds on either side and every bar reads 0.
 */
static ErrorNumber sqzLegFlat( void )
{
   static TA_Real flat[SQZ_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i;

   for( i = 0; i < SQZ_NB_BAR; i++ )
      flat[i] = 42.5;

   rc = TA_SQZMOM( 0, SQZ_NB_BAR-1, flat, flat, flat,
                   20, 2.0, 20, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_SUCCESS || begIdx != 38 )
   {
      printf( "Fail: SQZMOM flat rc=%d begIdx=%d, expected 38\n",
              (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < nbElement; i++ )
   {
      if( gotSqz[i] != 0 )
      {
         printf( "Fail: SQZMOM flat bar %d outSqueeze %d, expected 0: with a "
                 "zero band no channel has width and no test is strict\n",
                 (int)begIdx+i, (int)gotSqz[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzFlatCmp++;
   }

   return TA_TEST_PASS;
}

/* Leg 6. Aliasing. The body must read every bar input before it writes a caller
 * buffer. It did not at first: TA_LINEARREG wrote outMomentum in the middle and
 * TA_TRANGE then read a high or low the caller had aliased onto it. The
 * abstract layer caught that; this pins it in the group itself.
 */
static ErrorNumber sqzLegAlias( TA_History *history )
{
   static TA_Real buf[SQZ_NB_BAR];
   static TA_Real baseMom[SQZ_NB_BAR];
   static TA_Integer baseSqz[SQZ_NB_BAR];
   static TA_Integer ibuf[SQZ_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, baseBeg, baseNb;
   int src, out, i, nbBars;

   nbBars = (int)history->nbBars;

   rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                   20, 2.0, 20, 2.0, 1.5, 1.0,
                   &baseBeg, &baseNb, baseMom, baseSqz );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: SQZMOM alias baseline rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( out = 0; out < 2; out++ )
   {
      for( src = 0; src < 3; src++ )
      {
         const TA_Real *h = history->high, *l = history->low, *c = history->close;

         memcpy( buf, src==0 ? history->high : src==1 ? history->low
                                             : history->close,
                 nbBars * sizeof(TA_Real) );
         if( src == 0 ) h = buf; else if( src == 1 ) l = buf; else c = buf;

         rc = TA_SQZMOM( 0, nbBars-1, h, l, c, 20, 2.0, 20, 2.0, 1.5, 1.0,
                         &begIdx, &nbElement,
                         out==0 ? buf : gotMom,
                         out==0 ? gotSqz : ibuf );
         if( rc != TA_SUCCESS || begIdx != baseBeg || nbElement != baseNb )
         {
            printf( "Fail: SQZMOM alias out%d over input %d rc=%d\n",
                    out, src, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nbElement; i++ )
         {
            if( out == 0 )
            {
               if( memcmp( &buf[i], &baseMom[i], sizeof(TA_Real) ) != 0 ||
                   gotSqz[i] != baseSqz[i] )
                  goto aliasDiffers;
            }
            else
            {
               if( memcmp( &gotMom[i], &baseMom[i], sizeof(TA_Real) ) != 0 ||
                   ibuf[i] != baseSqz[i] )
                  goto aliasDiffers;
            }
         }
         g_sqzAliasCmp++;
         continue;

      aliasDiffers:
         printf( "Fail: SQZMOM out%d aliased over input %d differs at bar %d\n",
                 out, src, baseBeg+i );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 7. Parameter edges. A period below its range is rejected; nbdev of 0
 * collapses both bands onto the middle band, which is strictly inside every
 * channel of positive width, so every bar reads the tightest level.
 */
static ErrorNumber sqzLegEdges( TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i, nbBars;

   nbBars = (int)history->nbBars;

   rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                   1, 2.0, 20, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_BAD_PARAM )
   {
      printf( "Fail: SQZMOM bbPeriod=1 rc=%d, expected TA_BAD_PARAM\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_sqzEdgeCmp++;

   rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                   20, 2.0, 1, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_BAD_PARAM )
   {
      printf( "Fail: SQZMOM kcPeriod=1 rc=%d, expected TA_BAD_PARAM\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_sqzEdgeCmp++;

   rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                   20, 0.0, 20, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_SUCCESS || begIdx != 38 )
   {
      printf( "Fail: SQZMOM nbDev=0 rc=%d begIdx=%d\n", (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_sqzEdgeCmp++;

   for( i = 0; i < nbElement; i++ )
   {
      if( gotSqz[i] != 3 )
      {
         printf( "Fail: SQZMOM nbDev=0 bar %d outSqueeze %d, expected 3: both "
                 "bands sit on the middle band\n", (int)begIdx+i,
                 (int)gotSqz[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzEdgeCmp++;
   }

   /* Factor order: wide >= normal >= narrow, ties allowed. */
   {
      static const struct { double wide, normal, narrow; TA_RetCode want; } order[] =
      {
         { 1.4, 1.5, 1.0, TA_BAD_PARAM },   /* normal above wide */
         { 2.0, 1.5, 1.6, TA_BAD_PARAM },   /* narrow above normal */
         { 2.0, 1.5,-0.5, TA_BAD_PARAM },   /* below the range */
         { 1.5, 1.5, 1.5, TA_SUCCESS },
      };

      for( i = 0; i < (int)(sizeof(order)/sizeof(order[0])); i++ )
      {
         int lb = TA_SQZMOM_Lookback( 20, 2.0, 20, order[i].wide,
                                      order[i].normal, order[i].narrow );

         rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                         20, 2.0, 20, order[i].wide, order[i].normal,
                         order[i].narrow, &begIdx, &nbElement, gotMom, gotSqz );
         if( rc != order[i].want )
         {
            printf( "Fail: SQZMOM factors %g/%g/%g rc=%d, expected %d\n",
                    order[i].wide, order[i].normal, order[i].narrow,
                    (int)rc, (int)order[i].want );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         g_sqzEdgeCmp++;

         if( lb != ( order[i].want == TA_SUCCESS ? 38 : -1 ) )
         {
            printf( "Fail: TA_SQZMOM_Lookback factors %g/%g/%g = %d\n",
                    order[i].wide, order[i].normal, order[i].narrow, lb );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzEdgeCmp++;
      }
   }

   /* One bar short of the lookback: success and nothing written. */
   rc = TA_SQZMOM( 0, 37, history->high, history->low, history->close,
                   20, 2.0, 20, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_SUCCESS || nbElement != 0 )
   {
      printf( "Fail: SQZMOM over 38 bars rc=%d nbElement=%d, expected no output\n",
              (int)rc, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_sqzEdgeCmp++;

   /* Exactly one output bar, equal to the first bar of the full run. */
   {
      TA_Real oneMom = 0.0;
      TA_Integer oneSqz = 99;

      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      20, 2.0, 20, 2.0, 1.5, 1.0,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS || begIdx != 38 )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      rc = TA_SQZMOM( 0, 38, history->high, history->low, history->close,
                      20, 2.0, 20, 2.0, 1.5, 1.0,
                      &begIdx, &nbElement, &oneMom, &oneSqz );
      if( rc != TA_SUCCESS || begIdx != 38 || nbElement != 1 )
      {
         printf( "Fail: SQZMOM over 39 bars rc=%d begIdx=%d nbElement=%d\n",
                 (int)rc, (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      g_sqzEdgeCmp++;

      if( memcmp( &oneMom, &gotMom[0], sizeof(TA_Real) ) != 0 )
      {
         printf( "Fail: SQZMOM single bar outMomentum %.17g, the full run gives "
                 "%.17g\n", oneMom, gotMom[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzEdgeCmp++;

      if( oneSqz != gotSqz[0] )
      {
         printf( "Fail: SQZMOM single bar outSqueeze %d, the full run gives %d\n",
                 (int)oneSqz, (int)gotSqz[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzEdgeCmp++;
   }

   return TA_TEST_PASS;
}

/* Leg 8. Entered above the lookback and ended early. Leg 1 always starts at the
 * lookback, where the staged range begins as early as it ever can.
 */
static ErrorNumber sqzLegSubRange( TA_History *history )
{
   static const struct { int start, nbBars; } sub[2] = { { 100, 252 }, { 57, 200 } };
   const SqzCase *c = &sqzCases[0];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int t, i, refBeg, refNb;
   ErrorNumber e;

   for( t = 0; t < 2; t++ )
   {
      rc = TA_SQZMOM( sub[t].start, sub[t].nbBars-1,
                      history->high, history->low, history->close,
                      c->bbPeriod, c->nbDev, c->kcPeriod,
                      c->wide, c->normal, c->narrow,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_SQZMOM from bar %d rc=%d\n", sub[t].start, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( (e = sqzReference( history->high, history->low, history->close,
                             sub[t].nbBars, c, sub[t].start,
                             &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      if( begIdx != sub[t].start || begIdx != refBeg || nbElement != refNb )
      {
         printf( "Fail: TA_SQZMOM from bar %d begIdx %d/%d nbElement %d/%d\n",
                 sub[t].start, (int)begIdx, refBeg, (int)nbElement, refNb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( memcmp( &gotMom[i], &refMom[i], sizeof(TA_Real) ) != 0
             || gotSqz[i] != refSqz[i] )
         {
            printf( "Fail: TA_SQZMOM from bar %d, bar %d gives %.17g/%d, the "
                    "composition %.17g/%d\n", sub[t].start, refBeg+i,
                    gotMom[i], (int)gotSqz[i], refMom[i], (int)refSqz[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzRangeCmp += 2;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 9. A sub-range equals the same slice of a full run, within the drift of
 * the running sums; the ordinal exactly. */
static TA_RetCode sqzRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   static TA_Real otherMom[MAX_RANGE_SIZE];
   static TA_Integer otherSqz[MAX_RANGE_SIZE];

   *lookback = TA_SQZMOM_Lookback( 20, 2.0, 20, 2.0, 1.5, 1.0 );
   *isOutputInteger = ( outputNb == 1 );

   return TA_SQZMOM( startIdx, endIdx, h->high, h->low, h->close,
                     20, 2.0, 20, 2.0, 1.5, 1.0, outBegIdx, outNbElement,
                     outputNb == 0 ? outputBuffer : otherMom,
                     outputNb == 1 ? outputBufferInt : otherSqz );
}

static ErrorNumber sqzLegRange( TA_History *history )
{
   return doRangeTestEx( sqzRangeTestFunction, TA_STABLE_EPSILON,
                         TA_TEST_UNST_NONE, (void *)history, 2, 0 );
}
