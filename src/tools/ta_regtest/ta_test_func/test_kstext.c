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
 *  100126 MF,CC  First version (#491).
 */

/* Test TA_KSTEXT (Know Sure Thing with selectable MA types).
 *
 *   1. SMA == TA_KST, bitwise. TA_KST carries the formula goldens (test_kst.c),
 *      so this is the leg that sees a wrong weight or scale.
 *   2. COMPOSITE per MA type, bitwise: four TA_ROC into four TA_MA, the
 *      weighted sum left to right as fused multiply-adds, then TA_MA for the
 *      signal, every type on the legs against every type on the signal.
 *      Proves the anchoring and which type reaches which average, not the
 *      formula.
 *   3. GOLDENS per MA type: rows the QuantConnect LEAN KnowSureThing class
 *      produced with that MovingAverageType on all five averages: the formula
 *      leg for the types TA_KST cannot vouch for.
 *   4. LOOKBACK pins, with and without an unstable period.
 *   5. FLAT and ZERO-PRICE input, ALIASING.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

#define KSTEXT_MAX_BARS 1200
#define KSTEXT_NB_PERIOD 9
#define KSTEXT_NB_OPT    11

/* ROC1..4, MA1..4, Signal: the C argument order. */
typedef struct { int p[KSTEXT_NB_PERIOD]; } KstextPeriods;

static const KstextPeriods kstextGrid[] =
{
   { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } },  /* defaults */
   { { 10, 15, 20, 30, 10, 10, 10, 15,  1 } },  /* S = 1: signal copies the line */
   { { 30, 20, 15, 10, 15, 10, 10, 10,  9 } },  /* legs reversed */
   { {  1,  1,  1,  1,  1,  1,  1,  1,  1 } },
   { {  1, 15, 20, 30,  1, 10, 10, 15,  9 } },  /* one leg unsmoothed */
   { {  9, 12, 18, 24,  6,  6,  6,  9,  9 } },  /* Pring's long-term set */
   { {  3,  4,  6, 10,  3,  4,  6,  8,  5 } },  /* MetaStock short-term weekly set */
   { { 39, 52, 78, 109, 26, 26, 26, 39, 9 } },  /* MetaStock long-term set */
   { {  2,  3,  4, 200,  2,  2,  2,  5,  3 } }, /* long lag: output near the tail */
   { {  5,  7, 11, 13,  2,  3,  5,  7,  2 } },  /* every MA period unlike the others */
};
#define NB_KSTEXT_GRID ((int)(sizeof(kstextGrid)/sizeof(kstextGrid[0])))

static const int kstextStartGrid[] = { 0, 100, 52, 53, 180, 251 };
#define NB_KSTEXT_START ((int)(sizeof(kstextStartGrid)/sizeof(kstextStartGrid[0])))

static const TA_MAType kstextTypes[] =
{
   TA_MAType_SMA,  TA_MAType_EMA,   TA_MAType_WMA,  TA_MAType_DEMA,
   TA_MAType_TEMA, TA_MAType_TRIMA, TA_MAType_KAMA, TA_MAType_MAMA,
   TA_MAType_T3,   TA_MAType_HMA,   TA_MAType_DISABLED, TA_MAType_DEFAULT,
   TA_MAType_ZLEMA, TA_MAType_RMA,  TA_MAType_VIDYA, TA_MAType_ALMA,
};
#define NB_KSTEXT_TYPES ((int)(sizeof(kstextTypes)/sizeof(kstextTypes[0])))

static ErrorNumber test_kstext_sma      ( const TA_History *history );
static ErrorNumber test_kstext_composite( const TA_History *history );
static ErrorNumber test_kstext_goldens  ( void );
static ErrorNumber test_kstext_lookback ( void );
static ErrorNumber test_kstext_flat_zero( void );
static ErrorNumber test_kstext_aliasing ( const TA_History *history );

ErrorNumber test_func_kstext( TA_History *history )
{
   ErrorNumber e;

   if( history->nbBars > KSTEXT_MAX_BARS )
   {
      printf( "Fail: KSTEXT test expects at most %d bars, got %d\n",
              KSTEXT_MAX_BARS, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( (e = test_kstext_sma( history ))       != TA_TEST_PASS ) return e;
   if( (e = test_kstext_composite( history )) != TA_TEST_PASS ) return e;
   if( (e = test_kstext_goldens())            != TA_TEST_PASS ) return e;
   if( (e = test_kstext_lookback())           != TA_TEST_PASS ) return e;
   if( (e = test_kstext_flat_zero())          != TA_TEST_PASS ) return e;
   if( (e = test_kstext_aliasing( history ))  != TA_TEST_PASS ) return e;

   return TA_TEST_PASS;
}

static TA_RetCode kstext_call( int startIdx, int endIdx, const double *in,
                               const KstextPeriods *k, TA_MAType rocType, TA_MAType sigType,
                               TA_Integer *beg, TA_Integer *nb,
                               double *outK, double *outS )
{
   const int *p = k->p;
   return TA_KSTEXT( startIdx, endIdx, in, p[0], p[1], p[2], p[3],
                     p[4], p[5], p[6], p[7], p[8], rocType, sigType,
                     beg, nb, outK, outS );
}

static int kstext_lookback( const KstextPeriods *k, TA_MAType rocType, TA_MAType sigType )
{
   const int *p = k->p;
   return TA_KSTEXT_Lookback( p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8],
                              rocType, sigType );
}

static void kstext_print( const char *what, int startIdx, const KstextPeriods *k,
                          TA_MAType rocType, TA_MAType sigType )
{
   printf( "Fail: KSTEXT %s start %d (%d,%d,%d,%d / %d,%d,%d,%d / %d) types (%d,%d)",
           what, startIdx, k->p[0], k->p[1], k->p[2], k->p[3],
           k->p[4], k->p[5], k->p[6], k->p[7], k->p[8], (int)rocType, (int)sigType );
}

static ErrorNumber kstext_route( const double *in, int nb, const KstextPeriods *k,
                                 TA_MAType rocType, TA_MAType sigType )
{
   static double outK[KSTEXT_MAX_BARS], outS[KSTEXT_MAX_BARS];
   double opt[KSTEXT_NB_OPT];
   TA_Integer beg, n;
   TA_RetCode rc;
   int i;

   if( !server_verify_active() )
      return TA_TEST_PASS;

   rc = kstext_call( 0, nb-1, in, k, rocType, sigType, &beg, &n, outK, outS );
   for( i = 0; i < KSTEXT_NB_PERIOD; i++ )
      opt[i] = (double)k->p[i];
   opt[9]  = (double)rocType;
   opt[10] = (double)sigType;
   return server_verify( "KSTEXT", 0, nb-1, nb, rc, beg, n,
                         (const TA_Real*[]){ in, NULL }, opt, KSTEXT_NB_OPT,
                         (const TA_Real*[]){ outK, outS, NULL }, NULL );
}

/* (1) */
static ErrorNumber test_kstext_sma( const TA_History *history )
{
   static double outK[KSTEXT_MAX_BARS], outS[KSTEXT_MAX_BARS];
   static double refK[KSTEXT_MAX_BARS], refS[KSTEXT_MAX_BARS];
   int nb = (int)history->nbBars, g, s, i, nbChecked = 0;
   TA_Integer beg, n, rBeg, rN;

   for( s = 0; s < NB_KSTEXT_START; s++ )
      for( g = 0; g < NB_KSTEXT_GRID; g++ )
      {
         const int *p = kstextGrid[g].p;
         int start = kstextStartGrid[s];

         if( kstext_call( start, nb-1, history->close, &kstextGrid[g],
                          TA_MAType_SMA, TA_MAType_SMA, &beg, &n, outK, outS ) != TA_SUCCESS ||
             TA_KST( start, nb-1, history->close, p[0], p[1], p[2], p[3],
                     p[4], p[5], p[6], p[7], p[8], &rBeg, &rN, refK, refS ) != TA_SUCCESS )
         {
            kstext_print( "SMA", start, &kstextGrid[g], TA_MAType_SMA, TA_MAType_SMA );
            printf( ": a call failed\n" );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( beg != rBeg || n != rN ||
             kstext_lookback( &kstextGrid[g], TA_MAType_SMA, TA_MAType_SMA ) !=
             TA_KST_Lookback( p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8] ) )
         {
            kstext_print( "SMA", start, &kstextGrid[g], TA_MAType_SMA, TA_MAType_SMA );
            printf( ": range (%d,%d), TA_KST (%d,%d)\n", (int)beg, (int)n, (int)rBeg, (int)rN );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < (int)n; i++ )
         {
            if( memcmp( &outK[i], &refK[i], sizeof(double) ) != 0 ||
                memcmp( &outS[i], &refS[i], sizeof(double) ) != 0 )
            {
               kstext_print( "SMA", start, &kstextGrid[g], TA_MAType_SMA, TA_MAType_SMA );
               printf( " bar %d: (%.17g, %.17g) != TA_KST (%.17g, %.17g)\n",
                       (int)beg + i, outK[i], outS[i], refK[i], refS[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         nbChecked += 2 * (int)n;
      }

   if( nbChecked < 10000 )
   {
      printf( "Fail: KSTEXT SMA compared only %d values against TA_KST\n", nbChecked );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* The composition, from shipped primitives only. Returns 0 on a failed call. */
static int kstext_reference( const double *in, int nb, int startIdx, const KstextPeriods *k,
                             TA_MAType rocType, TA_MAType sigType,
                             int *refBeg, int *refNb, double *refK, double *refS )
{
   static double roc[KSTEXT_MAX_BARS], rcma[4][KSTEXT_MAX_BARS], line[KSTEXT_MAX_BARS];
   TA_Integer b, n;
   int leg, i, s, lbLeg, lbSig, sigStart, lineNb;
   const int *p = k->p;

   s = 0;
   for( leg = 0; leg < 4; leg++ )
   {
      lbLeg = p[leg] + TA_MA_Lookback( p[4 + leg], rocType );
      if( lbLeg > s ) s = lbLeg;
   }
   lbSig = TA_MA_Lookback( p[8], sigType );
   s += lbSig;
   if( startIdx > s ) s = startIdx;
   *refBeg = s;
   *refNb  = 0;
   if( s > nb - 1 )
      return 1;
   sigStart = s - lbSig;

   for( leg = 0; leg < 4; leg++ )
   {
      int x = p[leg];
      if( TA_ROC( 0, nb-1, in, x, &b, &n, roc ) != TA_SUCCESS || b != x )
         return 0;
      /* roc[j] is bar j + x: the average's first output lands on sigStart. */
      if( TA_MA( sigStart - x, (int)n - 1, roc, p[4 + leg], rocType, &b, &n,
                 rcma[leg] ) != TA_SUCCESS || b != sigStart - x )
         return 0;
   }

   /* The generator fuses each multiply-add of the weighted sum, left to
    * right; the composition must too, or it diverges in the last bit. */
   lineNb = nb - sigStart;
   for( i = 0; i < lineNb; i++ )
      line[i] = fma( 4.0, rcma[3][i], fma( 3.0, rcma[2][i], fma( 2.0, rcma[1][i], rcma[0][i] ) ) );

   if( TA_MA( lbSig, lineNb - 1, line, p[8], sigType, &b, &n, refS ) != TA_SUCCESS
       || b != lbSig )
      return 0;
   for( i = 0; i < (int)n; i++ )
      refK[i] = line[lbSig + i];

   *refNb = (int)n;
   return 1;
}

/* Bitwise against the reference; counts the values compared into *nbChecked. */
static ErrorNumber kstext_compare_composite( const char *what, const double *in, int nb,
                                             int startIdx, const KstextPeriods *k,
                                             TA_MAType rocType, TA_MAType sigType,
                                             int *nbChecked )
{
   static double outK[KSTEXT_MAX_BARS], outS[KSTEXT_MAX_BARS];
   static double refK[KSTEXT_MAX_BARS], refS[KSTEXT_MAX_BARS];
   TA_Integer beg, n;
   int refBeg, refNb, i;

   if( kstext_call( startIdx, nb-1, in, k, rocType, sigType, &beg, &n, outK, outS )
       != TA_SUCCESS ||
       !kstext_reference( in, nb, startIdx, k, rocType, sigType, &refBeg, &refNb, refK, refS ) )
   {
      kstext_print( what, startIdx, k, rocType, sigType );
      printf( ": a call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( (int)n != refNb || (refNb > 0 && (int)beg != refBeg) ||
       kstext_lookback( k, rocType, sigType ) > refBeg )
   {
      kstext_print( what, startIdx, k, rocType, sigType );
      printf( ": range (%d,%d), composed (%d,%d)\n", (int)beg, (int)n, refBeg, refNb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < (int)n; i++ )
   {
      if( memcmp( &outK[i], &refK[i], sizeof(double) ) != 0 ||
          memcmp( &outS[i], &refS[i], sizeof(double) ) != 0 )
      {
         kstext_print( what, startIdx, k, rocType, sigType );
         printf( " bar %d: (%.17g, %.17g) != composed (%.17g, %.17g)\n",
                 (int)beg + i, outK[i], outS[i], refK[i], refS[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   *nbChecked += 2 * (int)n;
   return TA_TEST_PASS;
}

/* Every leg type against every signal type, over the period grid. Each pair
 * must compare something on its own: a type whose lookback outruns the series
 * on every period set would otherwise pass unseen. */
static ErrorNumber kstext_sweep_types( const char *what, const double *in, int nb,
                                       int nbStart, int perPairFloor )
{
   int r, t, g, s, nbChecked;
   ErrorNumber e;

   for( r = 0; r < NB_KSTEXT_TYPES; r++ )
      for( t = 0; t < NB_KSTEXT_TYPES; t++ )
      {
         nbChecked = 0;
         for( s = 0; s < nbStart; s++ )
            for( g = 0; g < NB_KSTEXT_GRID; g++ )
            {
               e = kstext_compare_composite( what, in, nb, kstextStartGrid[s], &kstextGrid[g],
                                             kstextTypes[r], kstextTypes[t], &nbChecked );
               if( e != TA_TEST_PASS )
                  return e;
            }
         if( nbChecked < perPairFloor )
         {
            printf( "Fail: KSTEXT composite %s types (%d,%d) compared only %d values\n",
                    what, (int)kstextTypes[r], (int)kstextTypes[t], nbChecked );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   return TA_TEST_PASS;
}

/* (2) */
static ErrorNumber test_kstext_composite( const TA_History *history )
{
   int nb = (int)history->nbBars;
   ErrorNumber e;

   e = kstext_sweep_types( "corpus", history->close, nb, NB_KSTEXT_START, 1000 );
   if( e != TA_TEST_PASS )
      return e;

   /* The unstable period lengthens every EMA-family average, legs and signal. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 7 );
   e = kstext_sweep_types( "unstable 7", history->close, nb, 2, 200 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   if( e != TA_TEST_PASS )
      return e;

   /* Both types off the default at once, which the sweep never sends. */
   e = kstext_route( history->close, nb, &kstextGrid[7], TA_MAType_EMA, TA_MAType_EMA );
   if( e != TA_TEST_PASS )
      return e;
   e = kstext_route( history->close, nb, &kstextGrid[9], TA_MAType_WMA, TA_MAType_RMA );
   if( e != TA_TEST_PASS )
      return e;
   return kstext_route( history->close, nb, &kstextGrid[5], TA_MAType_MAMA, TA_MAType_T3 );
}

/* (3) QuantConnect LEAN KnowSureThing at 5b0c9975d318 (Indicators 2.5.18090,
 * System.Decimal), one MovingAverageType on the five averages, through the
 * ta-lib-oracles lean_serve generic call (capture_491_kstext.py at 7705f2e),
 * transcribed unchanged. Tail rows only: LEAN feeds its averages from bar 0,
 * so a recursive type is seeded differently and the two meet once the seed
 * has decayed (below 1e-9 by bar 307 for every type here). On the last 200
 * bars LEAN is within 7.5e-12 absolute of TA_KSTEXT for TRIMA and 5.5e-13 for
 * the others. HMA is taken at Pring's long-term set: LEAN rounds the Hull
 * inner periods where TA_HMA truncates, and they agree at 6 and 9.
 *
 * c[i] = 100 + 10 sin(i/7) + 0.15 i + 2 sin(i/2.3), 1200 bars. The bound also
 * absorbs a libm sin() one ulp off the capture's.
 */
typedef struct { int grid; TA_MAType type; int bar; double kst, sig; } KstextRow;

static const KstextRow kstextLeanRows[] = {
   { 0, TA_MAType_EMA,   1100, -7.5560622038631564, -18.259741451262077 },
   { 0, TA_MAType_EMA,   1199, 47.340661507240526, 30.231655529569686 },
   { 0, TA_MAType_WMA,   1100, -8.2413493355353058, -23.514741669411713 },
   { 0, TA_MAType_WMA,   1199, 54.830890107765704, 43.490873546461202 },
   { 0, TA_MAType_DEMA,  1100, -0.12199009141182912, -16.984826948077874 },
   { 0, TA_MAType_DEMA,  1199, 67.963683618984419, 69.446000960661465 },
   { 0, TA_MAType_TEMA,  1100, 12.14676300971049, 7.7787627482488704 },
   { 0, TA_MAType_TEMA,  1199, 74.168185611887154, 78.641233110546537 },
   { 0, TA_MAType_TRIMA, 1100, -21.711560592943616, -33.258514896663122 },
   { 0, TA_MAType_TRIMA, 1199, 46.302485995140891, 27.955296702349774 },
   { 0, TA_MAType_KAMA,  1100, -0.92335121089156003, -14.257107863874934 },
   { 0, TA_MAType_KAMA,  1199, 68.644403440339502, 63.126697789300543 },
   { 0, TA_MAType_T3,    1100, -28.714733018087834, -37.183293299749948 },
   { 0, TA_MAType_T3,    1199, 37.151462922834156, 2.6769404922600333 },
   { 0, TA_MAType_ZLEMA, 1100, 8.8832127021892386, 1.2647627518416735 },
   { 0, TA_MAType_ZLEMA, 1199, 69.890248139724974, 71.054004006052082 },
   { 0, TA_MAType_RMA,   1100, -8.1401160694354733, -5.8584722534275055 },
   { 0, TA_MAType_RMA,   1199, 32.424841143321622, 12.171366146049388 },
   { 0, TA_MAType_ALMA,  1100, -3.2066327181050553, -21.223433029775205 },
   { 0, TA_MAType_ALMA,  1199, 59.441202215955045, 50.861545873692492 },
   { 5, TA_MAType_HMA,   1100, 29.055189168538465, 22.311900629867282 },
   { 5, TA_MAType_HMA,   1199, 65.469494683814148, 65.067321084169265 },
};

#define KSTEXT_LEAN_ABS 2e-11
#define KSTEXT_LEAN_REL 1e-12

static ErrorNumber test_kstext_goldens( void )
{
   static double synth[KSTEXT_MAX_BARS], outK[KSTEXT_MAX_BARS], outS[KSTEXT_MAX_BARS];
   TA_Integer beg, n;
   unsigned int r;
   int i;

   for( i = 0; i < KSTEXT_MAX_BARS; i++ )
      synth[i] = 100.0 + 10.0*sin( i/7.0 ) + 0.15*i + 2.0*sin( i/2.3 );

   for( r = 0; r < sizeof(kstextLeanRows)/sizeof(kstextLeanRows[0]); r++ )
   {
      const KstextRow *row = &kstextLeanRows[r];
      if( kstext_call( 0, KSTEXT_MAX_BARS-1, synth, &kstextGrid[row->grid], row->type, row->type,
                       &beg, &n, outK, outS ) != TA_SUCCESS ||
          row->bar < beg || row->bar >= beg + n )
      {
         kstext_print( "golden", 0, &kstextGrid[row->grid], row->type, row->type );
         printf( ": call failed or bar %d outside (%d,%d)\n", row->bar, (int)beg, (int)n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      i = row->bar - (int)beg;
      if( fabs( outK[i] - row->kst ) > KSTEXT_LEAN_ABS + KSTEXT_LEAN_REL*fabs( row->kst ) ||
          fabs( outS[i] - row->sig ) > KSTEXT_LEAN_ABS + KSTEXT_LEAN_REL*fabs( row->sig ) )
      {
         kstext_print( "golden", 0, &kstextGrid[row->grid], row->type, row->type );
         printf( " bar %d: (%.17g, %.17g) want (%.17g, %.17g)\n",
                 row->bar, outK[i], outS[i], row->kst, row->sig );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (4) A wrong-by-one anchor is invisible to a value comparison that shares it. */
static ErrorNumber test_kstext_lookback( void )
{
   static const struct { KstextPeriods k; TA_MAType roc, sig; int unst, want; } cases[] = {
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, TA_MAType_SMA,  TA_MAType_SMA,  0, 52 },
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, TA_MAType_EMA,  TA_MAType_EMA,  0, 52 },
      /* 30 + (14 + 5) on the legs, (8 + 5) on the signal. */
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, TA_MAType_EMA,  TA_MAType_EMA,  5, 62 },
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, TA_MAType_EMA,  TA_MAType_SMA,  5, 57 },
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, TA_MAType_SMA,  TA_MAType_EMA,  5, 57 },
      /* DEMA is 2(n-1), TEMA 3(n-1): 30 + 28 + 24. */
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, TA_MAType_DEMA, TA_MAType_TEMA, 0, 82 },
      /* MAMA's 32 at any period above 1: 30 + 32 + 32. */
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, TA_MAType_MAMA, TA_MAType_MAMA, 0, 94 },
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, TA_MAType_DISABLED, TA_MAType_DISABLED, 0, 30 },
      /* Each leg the unique max in turn, its MA period unlike every other. */
      { { { 50,  1,  1,  1, 40,  2,  3,  4,  1 } }, TA_MAType_WMA,  TA_MAType_WMA,  0, 89 },
      { { {  1, 50,  1,  1,  2, 40,  3,  4,  1 } }, TA_MAType_WMA,  TA_MAType_WMA,  0, 89 },
      { { {  1,  1, 50,  1,  2,  3, 40,  4,  1 } }, TA_MAType_WMA,  TA_MAType_WMA,  0, 89 },
      { { {  1,  1,  1, 50,  2,  3,  4, 40,  1 } }, TA_MAType_WMA,  TA_MAType_WMA,  0, 89 },
      /* The longest ROC and the longest MA on different legs: the max is over
       * the per-leg sums, not the sum of two maxima. */
      { { { 50,  1,  1,  1,  2,  3,  4, 40,  1 } }, TA_MAType_WMA,  TA_MAType_WMA,  0, 51 },
      { { {  1,  1,  1, 50, 40,  3,  4,  2,  1 } }, TA_MAType_WMA,  TA_MAType_WMA,  0, 51 },
   };
   unsigned int c;

   for( c = 0; c < sizeof(cases)/sizeof(cases[0]); c++ )
   {
      int got;
      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, cases[c].unst );
      got = kstext_lookback( &cases[c].k, cases[c].roc, cases[c].sig );
      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
      if( got != cases[c].want )
      {
         kstext_print( "lookback", 0, &cases[c].k, cases[c].roc, cases[c].sig );
         printf( " unstable %d = %d, want %d\n", cases[c].unst, got, cases[c].want );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }
   return TA_TEST_PASS;
}

/* (5a) Flat input is exactly 0.0 on both outputs for every type: each rate of
 * change is zero. A zero price follows TA_ROC's guard: finite, and bitwise the
 * composition, which carries it. */
static ErrorNumber test_kstext_flat_zero( void )
{
   static double in[300], outK[300], outS[300];
   TA_Integer beg, n;
   TA_RetCode rc;
   int i, t;
   ErrorNumber e;

   for( i = 0; i < 300; i++ )
      in[i] = 42.25;
   for( t = 0; t < NB_KSTEXT_TYPES; t++ )
   {
      rc = kstext_call( 0, 299, in, &kstextGrid[0], kstextTypes[t], kstextTypes[t],
                        &beg, &n, outK, outS );
      if( rc != TA_SUCCESS || n <= 0 )
      {
         printf( "Fail: KSTEXT flat type %d: rc %d nb %d\n", (int)kstextTypes[t], (int)rc, (int)n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < (int)n; i++ )
      {
         if( outK[i] != 0.0 || outS[i] != 0.0 )
         {
            printf( "Fail: KSTEXT flat type %d bar %d: (%.17g, %.17g) != 0.0\n",
                    (int)kstextTypes[t], (int)beg + i, outK[i], outS[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   for( i = 0; i < 300; i++ )
      in[i] = 100.0 + (double)(i % 13) * 0.37 - (double)(i % 5) * 0.11;
   in[100] = 0.0;
   for( t = 0; t < NB_KSTEXT_TYPES; t++ )
   {
      rc = kstext_call( 0, 299, in, &kstextGrid[0], kstextTypes[t], kstextTypes[t],
                        &beg, &n, outK, outS );
      if( rc != TA_SUCCESS || n <= 0 )
      {
         printf( "Fail: KSTEXT zero price type %d: rc %d nb %d\n",
                 (int)kstextTypes[t], (int)rc, (int)n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < (int)n; i++ )
      {
         if( !isfinite( outK[i] ) || !isfinite( outS[i] ) )
         {
            printf( "Fail: KSTEXT zero price type %d bar %d: (%.17g, %.17g) not finite\n",
                    (int)kstextTypes[t], (int)beg + i, outK[i], outS[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   e = kstext_sweep_types( "zero price", in, 300, 1, 500 );
   if( e != TA_TEST_PASS )
      return e;

   /* A hand-built series with a zero price, both types off the default. */
   return kstext_route( in, 300, &kstextGrid[2], TA_MAType_KAMA, TA_MAType_EMA );
}

/* (5b) Either output may be the input buffer, whatever the types. */
static ErrorNumber test_kstext_aliasing( const TA_History *history )
{
   static double buf[KSTEXT_MAX_BARS], outK[KSTEXT_MAX_BARS], outS[KSTEXT_MAX_BARS];
   static double refK[KSTEXT_MAX_BARS], refS[KSTEXT_MAX_BARS];
   int nb = (int)history->nbBars, which, t, i, nbChecked = 0;
   TA_Integer beg, n, rBeg, rN;

   for( t = 0; t < NB_KSTEXT_TYPES; t++ )
   {
      TA_MAType rocType = kstextTypes[t];
      TA_MAType sigType = kstextTypes[(t + 5) % NB_KSTEXT_TYPES];

      if( kstext_call( 0, nb-1, history->close, &kstextGrid[0], rocType, sigType,
                       &rBeg, &rN, refK, refS ) != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      for( which = 0; which < 2; which++ )
      {
         memcpy( buf, history->close, sizeof(double) * nb );
         if( kstext_call( 0, nb-1, buf, &kstextGrid[0], rocType, sigType, &beg, &n,
                          which == 0 ? buf : outK, which == 1 ? buf : outS ) != TA_SUCCESS
             || beg != rBeg || n != rN )
         {
            printf( "Fail: KSTEXT aliasing %s == inReal: call or range\n",
                    which == 0 ? "outKST" : "outKSTSignal" );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < (int)n; i++ )
         {
            const double *want      = which == 0 ? refK : refS;
            const double *other     = which == 0 ? outS : outK;
            const double *otherWant = which == 0 ? refS : refK;
            if( memcmp( &buf[i], &want[i], sizeof(double) ) != 0 ||
                memcmp( &other[i], &otherWant[i], sizeof(double) ) != 0 )
            {
               printf( "Fail: KSTEXT aliasing %s == inReal types (%d,%d) bar %d differs\n",
                       which == 0 ? "outKST" : "outKSTSignal",
                       (int)rocType, (int)sigType, (int)beg + i );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         nbChecked += 2 * (int)n;
      }
   }
   if( nbChecked < 5000 )
   {
      printf( "Fail: KSTEXT aliasing compared only %d values\n", nbChecked );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}
