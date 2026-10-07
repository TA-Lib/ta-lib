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
 *  093026 KL,CC  First version (#477).
 *  093026 MF,CC  Frozen pandas-ta-classic rows; composite at a heap-sized
 *                window and under a non-zero unstable period.
 */

/* Description:
 *
 *   Test TA_CKSP (Chande Kroll Stop).
 *
 *   Legs:
 *     1. COMPOSITE, bitwise, per-leg anchored. With a = startIdx - (q-1),
 *        TA_ATR, TA_MAX and TA_MIN entered at a, the two first-stage stops
 *        formed from them, then TA_MAX / TA_MIN of those over q. This is the
 *        only leg that can see a wrong anchor: every external oracle computes
 *        from bar 0, so a whole-series comparison is blind to it.
 *     2. MULTIPLIER 0 identity, bitwise: with the ATR term gone the outputs
 *        are the plain extremes of the whole p+q-1 span. Pins both window
 *        lengths without ATR in the way.
 *     3. LOOKBACK tier, including after TA_SetUnstablePeriod(TA_FUNC_UNST_ATR)
 *        moves it. A wrong-by-one anchor is invisible in late bars.
 *     4. DEGENERATE: a flat series, where both stops equal the price.
 *     5. ALIASING of each output over its own natural input.
 *     6. The generic start/end range sweep.
 *     7. GOLDEN rows from pandas-ta-classic, the one external implementation
 *        that seeds its ATR on TA_ATR's bar.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define CKSP_CAP 300

typedef struct { int p; double x; int q; } CkspCfg;

static const CkspCfg ckspCfg[] = {
   { 10, 1.0,  9 },
   { 10, 3.0, 20 },
   {  5, 2.0,  3 },
   { 22, 3.0,  1 },
   {  2, 0.0,  1 },
   { 10, 0.0,  9 },
   {  2, 1.5,  2 },
   { 40, 2.0, 35 },
   {  5, 2.0,  9 }
};
#define NB_CKSP_CFG (int)(sizeof(ckspCfg)/sizeof(ckspCfg[0]))

static ErrorNumber test_cksp_composite ( const TA_History *history );
static ErrorNumber test_cksp_zero_mult ( const TA_History *history );
static ErrorNumber test_cksp_lookback  ( const TA_History *history );
static ErrorNumber test_cksp_flat      ( void );
static ErrorNumber test_cksp_aliasing  ( const TA_History *history );
static ErrorNumber test_cksp_range     ( const TA_History *history );
static ErrorNumber test_cksp_golden    ( const TA_History *history );

/**** Global functions definitions.   ****/
ErrorNumber test_func_cksp( TA_History *history )
{
   ErrorNumber retValue;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_cksp_composite( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_cksp_zero_mult( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_cksp_lookback( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_cksp_flat();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_cksp_aliasing( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_cksp_range( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return test_cksp_golden( history );
}

/**** Local functions definitions.     ****/

/* (1) Bitwise against the composition, from several starts. */
static ErrorNumber cksp_composite_at( const TA_History *history, int route )
{
   static TA_Real gotH[CKSP_CAP], gotL[CKSP_CAP];
   static TA_Real atr[CKSP_CAP], hh[CKSP_CAP], ll[CKSP_CAP];
   static TA_Real fh[CKSP_CAP], fl[CKSP_CAP];
   static TA_Real refH[CKSP_CAP], refL[CKSP_CAP];
   static const int startBump[] = { 0, 1, 5, 40 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, b2, n2, b3, n3;
   int nb = (int)history->nbBars;
   int c, s, i;

   for( c = 0; c < NB_CKSP_CFG; c++ )
   {
      int p = ckspCfg[c].p, q = ckspCfg[c].q;
      double x = ckspCfg[c].x;
      int lookback = TA_CKSP_Lookback( p, x, q );

      for( s = 0; s < (int)(sizeof(startBump)/sizeof(startBump[0])); s++ )
      {
         int startIdx = lookback + startBump[s];
         int a = startIdx - (q-1);

         if( startIdx > nb-1 )
            continue;

         rc = TA_CKSP( startIdx, nb-1, history->high, history->low,
                       history->close, p, x, q, &begIdx, &nbElement,
                       gotH, gotL );
         if( rc != TA_SUCCESS || begIdx != startIdx || nbElement != nb-startIdx )
         {
            printf( "Fail: TA_CKSP %d/%g/%d start %d: rc=%d shape (%d,%d), "
                    "expected (%d,%d)\n", p, x, q, startIdx, (int)rc,
                    (int)begIdx, (int)nbElement, startIdx, nb-startIdx );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         /* Each leg entered on its own bar, not on the caller's startIdx. */
         if( TA_ATR( a, nb-1, history->high, history->low, history->close, p,
                     &b2, &n2, atr ) != TA_SUCCESS
             || TA_MAX( a, nb-1, history->high, p, &b3, &n3, hh ) != TA_SUCCESS
             || b3 != b2 || n3 != n2
             || TA_MIN( a, nb-1, history->low, p, &b3, &n3, ll ) != TA_SUCCESS
             || b3 != b2 || n3 != n2 || b2 != a )
         {
            printf( "Fail: TA_CKSP %d/%g/%d start %d: the composition did not "
                    "line up at a=%d\n", p, x, q, startIdx, a );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < n2; i++ )
         {
            fh[i] = hh[i] - x*atr[i];
            fl[i] = ll[i] + x*atr[i];
         }

         if( q == 1 )
         {
            memcpy( refH, fh, (size_t)n2*sizeof(TA_Real) );
            memcpy( refL, fl, (size_t)n2*sizeof(TA_Real) );
         }
         else
         {
            if( TA_MAX( q-1, n2-1, fh, q, &b3, &n3, refH ) != TA_SUCCESS
                || TA_MIN( q-1, n2-1, fl, q, &b3, &n3, refL ) != TA_SUCCESS )
            {
               printf( "Fail: TA_CKSP %d/%g/%d start %d: stage two did not "
                       "run\n", p, x, q, startIdx );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
         }

         for( i = 0; i < nbElement; i++ )
         {
            if( memcmp( &gotH[i], &refH[i], sizeof(TA_Real) ) != 0
                || memcmp( &gotL[i], &refL[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "Fail: TA_CKSP %d/%g/%d start %d at out[%d]: "
                       "high %.17g vs %.17g, low %.17g vs %.17g "
                       "(must be BIT-exact)\n", p, x, q, startIdx, i,
                       gotH[i], refH[i], gotL[i], refL[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         /* Two or three parameters off their defaults at once, which the
          * sweep never sends.
          */
         if( route && startBump[s] == 0 && server_verify_active() )
         {
            double optIn[3];
            ErrorNumber e;

            optIn[0] = (double)p;
            optIn[1] = x;
            optIn[2] = (double)q;
            e = server_verify( "CKSP", startIdx, nb-1, nb,
                               rc, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low,
                                                   history->close, NULL },
                               optIn, 3,
                               (const TA_Real*[]){ gotH, gotL, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }
   }

   return TA_TEST_PASS;
}

/* The composition holds at any unstable period, the legs being entered with
 * the same setting; a non-zero one is what runs the warm-up loop between the
 * seed and the first stop.
 */
static ErrorNumber test_cksp_composite( const TA_History *history )
{
   ErrorNumber retValue;

   retValue = cksp_composite_at( history, 1 );
   if( retValue != TA_TEST_PASS )
      return retValue;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 7 );
   retValue = cksp_composite_at( history, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 0 );
   return retValue;
}

/* (2) At a multiplier of 0 the ATR term vanishes and the two stages collapse
 * into one window of p+q-1 bars. This pins both lengths, and it does so with
 * no ATR in the comparison at all.
 */
static ErrorNumber test_cksp_zero_mult( const TA_History *history )
{
   static TA_Real gotH[CKSP_CAP], gotL[CKSP_CAP];
   static TA_Real refH[CKSP_CAP], refL[CKSP_CAP];
   static const int pGrid[] = { 2, 5, 10, 22 };
   static const int qGrid[] = { 1, 3, 9, 20 };
   TA_Integer begIdx, nbElement, b2, n2;
   int nb = (int)history->nbBars;
   int a, b, i;

   for( a = 0; a < (int)(sizeof(pGrid)/sizeof(pGrid[0])); a++ )
   {
      for( b = 0; b < (int)(sizeof(qGrid)/sizeof(qGrid[0])); b++ )
      {
         int p = pGrid[a], q = qGrid[b];
         int span = p + q - 1;
         int lookback = TA_CKSP_Lookback( p, 0.0, q );

         if( lookback > nb-1 || span < 2 )
            continue;

         if( TA_CKSP( lookback, nb-1, history->high, history->low,
                      history->close, p, 0.0, q, &begIdx, &nbElement,
                      gotH, gotL ) != TA_SUCCESS )
         {
            printf( "Fail: TA_CKSP %d/0/%d: the call failed\n", p, q );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( TA_MAX( begIdx, nb-1, history->high, span, &b2, &n2, refH )
                != TA_SUCCESS
             || TA_MIN( begIdx, nb-1, history->low, span, &b2, &n2, refL )
                != TA_SUCCESS )
         {
            printf( "Fail: TA_CKSP %d/0/%d: the extremes did not run\n", p, q );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nbElement; i++ )
         {
            if( memcmp( &gotH[i], &refH[i], sizeof(TA_Real) ) != 0
                || memcmp( &gotL[i], &refL[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "Fail: TA_CKSP %d/0/%d at out[%d]: high %.17g vs "
                       "MAX(H,%d) %.17g, low %.17g vs MIN(L,%d) %.17g\n",
                       p, q, i, gotH[i], span, refH[i], gotL[i], span, refL[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) Under a count the lookback is atr_lookback(p) + q - 1, so it moves with
 * the ATR's unstable period. Asserted at the default and again after the setting moves,
 * because a wrong-by-one anchor shows up nowhere else once the series is long.
 */
static ErrorNumber test_cksp_lookback( const TA_History *history )
{
   static TA_Real gotH[CKSP_CAP], gotL[CKSP_CAP];
   static const int unstGrid[] = { 0, 1, 7 };
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int u, c;

   for( u = 0; u < (int)(sizeof(unstGrid)/sizeof(unstGrid[0])); u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, (unsigned int)unstGrid[u] );

      for( c = 0; c < NB_CKSP_CFG; c++ )
      {
         int p = ckspCfg[c].p, q = ckspCfg[c].q;
         double x = ckspCfg[c].x;
         int want = TA_ATR_Lookback( p ) + q - 1;
         int lookback = TA_CKSP_Lookback( p, x, q );

         if( lookback != want )
         {
            printf( "Fail: TA_CKSP_Lookback %d/%g/%d = %d, expected "
                    "TA_ATR_Lookback(%d) + %d - 1 = %d (unstable %d)\n",
                    p, x, q, lookback, p, q, want, unstGrid[u] );
            TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         if( lookback > nb-1 )
            continue;

         if( TA_CKSP( 0, nb-1, history->high, history->low, history->close,
                      p, x, q, &begIdx, &nbElement, gotH, gotL ) != TA_SUCCESS
             || begIdx != lookback || nbElement != nb-lookback )
         {
            printf( "Fail: TA_CKSP %d/%g/%d unstable %d: shape (%d,%d), "
                    "expected (%d,%d)\n", p, x, q, unstGrid[u], (int)begIdx,
                    (int)nbElement, lookback, nb-lookback );
            TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 0 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 0 );
   return TA_TEST_PASS;
}

/* (4) A series that never moves. Every True Range is 0, so the ATR is 0 and
 * the multiplier cannot separate the stops from the price: both outputs are
 * the price itself, whatever the multiplier is. Routed across the language
 * servers, since the sweep sends no such series.
 */
static ErrorNumber test_cksp_flat( void )
{
   #define CKSP_FLAT_N 40
   static TA_Real high[CKSP_FLAT_N], low[CKSP_FLAT_N], close[CKSP_FLAT_N];
   static TA_Real gotH[CKSP_FLAT_N], gotL[CKSP_FLAT_N];
   static const double level[] = { 130.0, 130.7 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int v, i, p = 10, q = 9;
   double x = 3.0;

   for( v = 0; v < (int)(sizeof(level)/sizeof(level[0])); v++ )
   {
      for( i = 0; i < CKSP_FLAT_N; i++ )
      {
         high[i] = level[v];
         low[i] = level[v];
         close[i] = level[v];
      }

      rc = TA_CKSP( 0, CKSP_FLAT_N-1, high, low, close, p, x, q,
                    &begIdx, &nbElement, gotH, gotL );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_CKSP flat %g: rc=%d\n", level[v], (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( memcmp( &gotH[i], &level[v], sizeof(TA_Real) ) != 0
             || memcmp( &gotL[i], &level[v], sizeof(TA_Real) ) != 0 )
         {
            printf( "Fail: TA_CKSP flat %g at out[%d]: high %.17g low %.17g, "
                    "both expected %.17g\n", level[v], i, gotH[i], gotL[i],
                    level[v] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      if( server_verify_active() )
      {
         double optIn[3];
         ErrorNumber e;

         optIn[0] = (double)p;
         optIn[1] = x;
         optIn[2] = (double)q;
         e = server_verify( "CKSP", 0, CKSP_FLAT_N-1, CKSP_FLAT_N,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ high, low, close, NULL },
                            optIn, 3,
                            (const TA_Real*[]){ gotH, gotL, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (5) Each output over the input it is built from. */
static ErrorNumber test_cksp_aliasing( const TA_History *history )
{
   static TA_Real refH[CKSP_CAP], refL[CKSP_CAP];
   static TA_Real workH[CKSP_CAP], workL[CKSP_CAP], other[CKSP_CAP];
   TA_Integer begIdx, nbElement, b2, n2;
   int nb = (int)history->nbBars;
   int p = 10, q = 9, i;
   double x = 1.0;

   if( TA_CKSP( 0, nb-1, history->high, history->low, history->close, p, x, q,
                &begIdx, &nbElement, refH, refL ) != TA_SUCCESS )
   {
      printf( "Fail: TA_CKSP aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* The high stop written over the highs, the low stop over the lows. */
   for( i = 0; i < nb; i++ )
   {
      workH[i] = history->high[i];
      workL[i] = history->low[i];
   }
   if( TA_CKSP( 0, nb-1, workH, workL, history->close, p, x, q,
                &b2, &n2, workH, workL ) != TA_SUCCESS
       || b2 != begIdx || n2 != nbElement
       || memcmp( refH, workH, (size_t)nbElement*sizeof(TA_Real) ) != 0
       || memcmp( refL, workL, (size_t)nbElement*sizeof(TA_Real) ) != 0 )
   {
      printf( "Fail: TA_CKSP aliasing over inHigh / inLow\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* And the high stop over the closes, which only the True Range reads. */
   for( i = 0; i < nb; i++ )
      other[i] = history->close[i];
   if( TA_CKSP( 0, nb-1, history->high, history->low, other, p, x, q,
                &b2, &n2, other, workL ) != TA_SUCCESS
       || b2 != begIdx || n2 != nbElement
       || memcmp( refH, other, (size_t)nbElement*sizeof(TA_Real) ) != 0 )
   {
      printf( "Fail: TA_CKSP aliasing over inClose\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (6) The generic start/end sweep.
 *
 * CONVERGING, not EPSILON: the extremes are finite windows, but the Average
 * True Range under them is a recursion seeded at this call's own start, so
 * two calls entered at different bars agree only as the unstable period warms.
 */
static TA_RetCode ckspRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   static TA_Real other[CKSP_CAP];
   TA_RetCode retCode;

   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_CKSP_Lookback( 10, 1.0, 9 );

   if( outputNb == 0 )
      retCode = TA_CKSP( startIdx, endIdx, h->high, h->low, h->close,
                         10, 1.0, 9, outBegIdx, outNbElement,
                         outputBuffer, other );
   else
      retCode = TA_CKSP( startIdx, endIdx, h->high, h->low, h->close,
                         10, 1.0, 9, outBegIdx, outNbElement,
                         other, outputBuffer );

   return retCode;
}

static ErrorNumber test_cksp_range( const TA_History *history )
{
   return doRangeTestEx( ckspRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_ATR,
                         (void *)history, 2, 0 );
}

/* (7) pandas-ta-classic 0.6.52 `cksp(high, low, close, p, x, q, tvmode=True)`
 * (pandas 3.0.3, numpy 2.5.1) on this corpus, run through ta-lib-oracles
 * `capture_477_cksp.py` at 2e5f2701; its CKSPl column is the high stop and its
 * CKSPs column the low one. Chosen because its Wilder average is seeded on
 * TA_ATR's bar, so it is comparable from the first output: the bar-0 true
 * range implementations (TradingView, LEAN, talipp, trading-signals) are
 * 3e-4 away there and need about 300 bars to converge.
 *
 * 1e-14 relative. The two are the same arithmetic in a different order, and
 * the measured gap is a few ulps.
 */
typedef struct { int p; double x; int q; int bar; double high; double low; } CkspGolden;

static const CkspGolden ckspGolden[] = {
   { 10, 1.0,  9,  18, 96.642499999999998, 90.186049708750005 },
   { 10, 1.0,  9,  19, 96.642499999999998, 90.092444737874999 },
   { 10, 1.0,  9, 100, 119.41407380766739, 107.7801244599106  },
   { 10, 1.0,  9, 180, 134.28803829751402, 125.84778404797029 },
   { 10, 1.0,  9, 251, 118.29444999590379, 107.66719935768752 },
   { 10, 3.0, 20,  29, 90.677499999999995, 90.593205103140775 },
   { 10, 3.0, 20, 100, 112.24222142300219, 93.552482782139862 },
   { 10, 3.0, 20, 180, 127.48411489254205, 131.17182796378751 },
   { 10, 3.0, 20, 251, 110.64334998771135, 109.33664333744092 },
   {  5, 2.0,  3,   7, 91.272999999999996, 95.563279999999992 },
   {  5, 2.0,  3,   8, 91.093400000000003, 95.414624000000003 },
   {  5, 2.0,  3, 100, 112.78832187955533, 117.08667812044467 },
   {  5, 2.0,  3, 251, 105.98763882594199, 111.58188893924643 },
   { 22, 3.0,  1,  22, 89.746818181818185, 94.503181818181815 },
   { 22, 3.0,  1,  23, 89.756735537190082, 92.148264462809919 },
   { 22, 3.0,  1, 251, 111.62064300832449, 112.74935699167551 },
   { 14, 2.5, 14,  27, 91.634603751840331, 88.987752511356774 },
   { 14, 2.5, 14, 180, 128.98162040702482, 129.78855916697415 },
   { 14, 2.5, 14, 251, 113.63510860489713, 112.65202370292035 },
   {  2, 0.5,  2,   3, 94.995000000000005, 92.819999999999993 },
   {  2, 0.5,  2,   4, 94.995000000000005, 94.301249999999996 },
   {  2, 0.5,  2, 251, 109.55575020732381, 107.8121248963381  }
};

static ErrorNumber test_cksp_golden( const TA_History *history )
{
   static TA_Real gotH[CKSP_CAP], gotL[CKSP_CAP];
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int g;

   for( g = 0; g < (int)(sizeof(ckspGolden)/sizeof(ckspGolden[0])); g++ )
   {
      const CkspGolden *row = &ckspGolden[g];
      double h, l;

      if( TA_CKSP( 0, nb-1, history->high, history->low, history->close,
                   row->p, row->x, row->q, &begIdx, &nbElement, gotH, gotL )
             != TA_SUCCESS
          || row->bar < begIdx || row->bar >= begIdx + nbElement )
      {
         printf( "Fail: TA_CKSP golden %d/%g/%d: bar %d is outside the output\n",
                 row->p, row->x, row->q, row->bar );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      h = gotH[row->bar - begIdx];
      l = gotL[row->bar - begIdx];
      if( !(fabs( h - row->high ) <= 1e-14 * fabs( row->high ))
          || !(fabs( l - row->low ) <= 1e-14 * fabs( row->low )) )
      {
         printf( "Fail: TA_CKSP golden %d/%g/%d bar %d: high %.17g vs %.17g, "
                 "low %.17g vs %.17g\n", row->p, row->x, row->q, row->bar,
                 h, row->high, l, row->low );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}
