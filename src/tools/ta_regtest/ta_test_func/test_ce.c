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
 *  103126 MF,CC  First version (#568).
 */

/* Description:
 *
 *   Test TA_CE (Chandelier Exit).
 *
 *   Legs:
 *     1. COMPOSITE, bitwise, from several starts: TA_MAX(high) and TA_MIN(low)
 *        offset by the multiple of TA_ATR, the three entered at the caller's
 *        startIdx. Also under a non-zero unstable period, which runs the
 *        warm-up loop between the seed and the first output.
 *     2. CKSP identity, bitwise: TA_CKSP with a stop period of 1 is this
 *        function on both lines, and is tested on its own.
 *     3. LOOKBACK tier, including after TA_SetUnstablePeriod(TA_FUNC_UNST_ATR)
 *        moves it.
 *     4. DEGENERATE: a flat series, where both lines equal the price.
 *     5. ALIASING of each output over its own natural input, and over the
 *        close.
 *     6. The generic start/end range sweep.
 *     7. GOLDEN rows from pandas-ta-classic, the one external implementation
 *        whose Wilder average equals TA_ATR's from bar P on.
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
#define CE_CAP 300
#define CE_GOLDEN_TOL 1e-14

typedef struct { int p; double x; } CeCfg;

static const CeCfg ceCfg[] = {
   { 22, 3.0 },
   { 10, 1.0 },
   {  5, 2.0 },
   {  2, 0.5 },
   { 14, 2.5 },
   { 40, 2.0 },
   { 10, 0.0 },
   {  2, 0.0 }
};
#define NB_CE_CFG (int)(sizeof(ceCfg)/sizeof(ceCfg[0]))

static ErrorNumber test_ce_composite ( const TA_History *history );
static ErrorNumber test_ce_cksp      ( const TA_History *history );
static ErrorNumber test_ce_lookback  ( const TA_History *history );
static ErrorNumber test_ce_flat      ( void );
static ErrorNumber test_ce_aliasing  ( const TA_History *history );
static ErrorNumber test_ce_range     ( const TA_History *history );
static ErrorNumber test_ce_golden    ( const TA_History *history );

/**** Global functions definitions.   ****/
ErrorNumber test_func_ce( TA_History *history )
{
   ErrorNumber retValue;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_ce_composite( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ce_cksp( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ce_lookback( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ce_flat();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ce_aliasing( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ce_range( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return test_ce_golden( history );
}

/**** Local functions definitions.     ****/

/* (1) Bitwise against the composition, from several starts. route: 0 for
 * nothing, 1 for every row but the defaults, 2 for every row.
 */
static ErrorNumber ce_composite_at( const TA_History *history, int route )
{
   static TA_Real gotL[CE_CAP], gotS[CE_CAP];
   static TA_Real atr[CE_CAP], hh[CE_CAP], ll[CE_CAP];
   static TA_Real refL[CE_CAP], refS[CE_CAP];
   static const int startBump[] = { 0, 1, 5, 40 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, b2, n2, b3, n3;
   int nb = (int)history->nbBars;
   int c, s, i;

   for( c = 0; c < NB_CE_CFG; c++ )
   {
      int p = ceCfg[c].p;
      double x = ceCfg[c].x;
      int lookback = TA_CE_Lookback( p, x );

      for( s = 0; s < (int)(sizeof(startBump)/sizeof(startBump[0])); s++ )
      {
         int startIdx = lookback + startBump[s];

         if( startIdx > nb-1 )
            continue;

         rc = TA_CE( startIdx, nb-1, history->high, history->low,
                     history->close, p, x, &begIdx, &nbElement, gotL, gotS );
         if( rc != TA_SUCCESS || begIdx != startIdx || nbElement != nb-startIdx )
         {
            printf( "Fail: TA_CE %d/%g start %d: rc=%d shape (%d,%d), "
                    "expected (%d,%d)\n", p, x, startIdx, (int)rc,
                    (int)begIdx, (int)nbElement, startIdx, nb-startIdx );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         if( TA_ATR( startIdx, nb-1, history->high, history->low,
                     history->close, p, &b2, &n2, atr ) != TA_SUCCESS
             || TA_MAX( startIdx, nb-1, history->high, p, &b3, &n3, hh )
                != TA_SUCCESS
             || b3 != b2 || n3 != n2
             || TA_MIN( startIdx, nb-1, history->low, p, &b3, &n3, ll )
                != TA_SUCCESS
             || b3 != b2 || n3 != n2 || b2 != startIdx || n2 != nbElement )
         {
            printf( "Fail: TA_CE %d/%g start %d: the composition did not "
                    "line up\n", p, x, startIdx );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < n2; i++ )
         {
            refL[i] = hh[i] - x*atr[i];
            refS[i] = ll[i] + x*atr[i];
         }

         for( i = 0; i < nbElement; i++ )
         {
            if( memcmp( &gotL[i], &refL[i], sizeof(TA_Real) ) != 0
                || memcmp( &gotS[i], &refS[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "Fail: TA_CE %d/%g start %d at out[%d]: "
                       "long %.17g vs %.17g, short %.17g vs %.17g "
                       "(must be BIT-exact)\n", p, x, startIdx, i,
                       gotL[i], refL[i], gotS[i], refS[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         /* Both parameters off their defaults at once, which the sweep never
          * sends; the defaults row is the sweep's own call, routed only under
          * a warm period.
          */
         if( route && startBump[s] == 0 && (route == 2 || p != 22 || x != 3.0)
             && server_verify_active() )
         {
            double optIn[2];
            ErrorNumber e;

            optIn[0] = (double)p;
            optIn[1] = x;
            e = server_verify( "CE", startIdx, nb-1, nb,
                               rc, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low,
                                                   history->close, NULL },
                               optIn, 2,
                               (const TA_Real*[]){ gotL, gotS, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }
   }

   return TA_TEST_PASS;
}

/* The composition holds at any unstable period, the legs being entered with
 * the same setting; a non-zero one is what runs the warm-up loop between the
 * seed and the first output. Routed as well: a warm period on a function
 * carrying no TA_FUNC_FLG_UNST_PER of its own is one the sweep never sends.
 */
static ErrorNumber test_ce_composite( const TA_History *history )
{
   ErrorNumber retValue;

   retValue = ce_composite_at( history, 1 );
   if( retValue != TA_TEST_PASS )
      return retValue;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 7 );
   retValue = ce_composite_at( history, 2 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 0 );
   return retValue;
}

/* (2) The Chande Kroll Stop with a stop period of 1 is this function: its
 * high stop is the long line and its low stop the short line, bit for bit.
 */
static ErrorNumber test_ce_cksp( const TA_History *history )
{
   static TA_Real gotL[CE_CAP], gotS[CE_CAP];
   static TA_Real refH[CE_CAP], refLo[CE_CAP];
   TA_Integer begIdx, nbElement, b2, n2;
   int nb = (int)history->nbBars;
   int c, i;

   for( c = 0; c < NB_CE_CFG; c++ )
   {
      int p = ceCfg[c].p;
      double x = ceCfg[c].x;

      if( TA_CE( 0, nb-1, history->high, history->low, history->close,
                 p, x, &begIdx, &nbElement, gotL, gotS ) != TA_SUCCESS
          || TA_CKSP( 0, nb-1, history->high, history->low, history->close,
                      p, x, 1, &b2, &n2, refH, refLo ) != TA_SUCCESS
          || b2 != begIdx || n2 != nbElement || nbElement <= 0 )
      {
         printf( "Fail: TA_CE %d/%g vs TA_CKSP %d/%g/1: shape (%d,%d) vs "
                 "(%d,%d)\n", p, x, p, x, (int)begIdx, (int)nbElement,
                 (int)b2, (int)n2 );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( memcmp( &gotL[i], &refH[i], sizeof(TA_Real) ) != 0
             || memcmp( &gotS[i], &refLo[i], sizeof(TA_Real) ) != 0 )
         {
            printf( "Fail: TA_CE %d/%g at out[%d]: long %.17g vs CKSP high "
                    "stop %.17g, short %.17g vs low stop %.17g\n", p, x, i,
                    gotL[i], refH[i], gotS[i], refLo[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) The lookback is atr_lookback(p), so it moves with the ATR's unstable
 * period. Asserted at the default and again after the setting moves.
 */
static ErrorNumber test_ce_lookback( const TA_History *history )
{
   static TA_Real gotL[CE_CAP], gotS[CE_CAP];
   static const int unstGrid[] = { 0, 1, 7 };
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int u, c;

   for( u = 0; u < (int)(sizeof(unstGrid)/sizeof(unstGrid[0])); u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, (unsigned int)unstGrid[u] );

      for( c = 0; c < NB_CE_CFG; c++ )
      {
         int p = ceCfg[c].p;
         double x = ceCfg[c].x;
         int want = TA_ATR_Lookback( p );
         int lookback = TA_CE_Lookback( p, x );

         if( lookback != want )
         {
            printf( "Fail: TA_CE_Lookback %d/%g = %d, expected "
                    "TA_ATR_Lookback(%d) = %d (unstable %d)\n",
                    p, x, lookback, p, want, unstGrid[u] );
            TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         if( lookback > nb-1 )
            continue;

         if( TA_CE( 0, nb-1, history->high, history->low, history->close,
                    p, x, &begIdx, &nbElement, gotL, gotS ) != TA_SUCCESS
             || begIdx != lookback || nbElement != nb-lookback )
         {
            printf( "Fail: TA_CE %d/%g unstable %d: shape (%d,%d), "
                    "expected (%d,%d)\n", p, x, unstGrid[u], (int)begIdx,
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
 * the multiplier cannot separate the lines from the price: both outputs are
 * the price itself, whatever the multiplier is. Routed across the language
 * servers, since the sweep sends no such series.
 */
static ErrorNumber test_ce_flat( void )
{
   #define CE_FLAT_N 40
   static TA_Real high[CE_FLAT_N], low[CE_FLAT_N], close[CE_FLAT_N];
   static TA_Real gotL[CE_FLAT_N], gotS[CE_FLAT_N];
   static const double level[] = { 130.0, 130.7 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int v, i, p = 10;
   double x = 3.0;

   for( v = 0; v < (int)(sizeof(level)/sizeof(level[0])); v++ )
   {
      for( i = 0; i < CE_FLAT_N; i++ )
      {
         high[i] = level[v];
         low[i] = level[v];
         close[i] = level[v];
      }

      rc = TA_CE( 0, CE_FLAT_N-1, high, low, close, p, x,
                  &begIdx, &nbElement, gotL, gotS );
      if( rc != TA_SUCCESS || nbElement != CE_FLAT_N - TA_CE_Lookback( p, x ) )
      {
         printf( "Fail: TA_CE flat %g: rc=%d nbElement=%d\n", level[v],
                 (int)rc, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( memcmp( &gotL[i], &level[v], sizeof(TA_Real) ) != 0
             || memcmp( &gotS[i], &level[v], sizeof(TA_Real) ) != 0 )
         {
            printf( "Fail: TA_CE flat %g at out[%d]: long %.17g short %.17g, "
                    "both expected %.17g\n", level[v], i, gotL[i], gotS[i],
                    level[v] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      if( server_verify_active() )
      {
         double optIn[2];
         ErrorNumber e;

         optIn[0] = (double)p;
         optIn[1] = x;
         e = server_verify( "CE", 0, CE_FLAT_N-1, CE_FLAT_N,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ high, low, close, NULL },
                            optIn, 2,
                            (const TA_Real*[]){ gotL, gotS, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (5) Each output over the input it is built from, and the long line over
 * the closes, which only the True Range reads.
 */
static ErrorNumber test_ce_aliasing( const TA_History *history )
{
   static TA_Real refL[CE_CAP], refS[CE_CAP];
   static TA_Real workH[CE_CAP], workL[CE_CAP], other[CE_CAP];
   TA_Integer begIdx, nbElement, b2, n2;
   int nb = (int)history->nbBars;
   int p = 22, i;
   double x = 3.0;

   if( TA_CE( 0, nb-1, history->high, history->low, history->close, p, x,
              &begIdx, &nbElement, refL, refS ) != TA_SUCCESS || nbElement <= 0 )
   {
      printf( "Fail: TA_CE aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < nb; i++ )
   {
      workH[i] = history->high[i];
      workL[i] = history->low[i];
   }
   if( TA_CE( 0, nb-1, workH, workL, history->close, p, x,
              &b2, &n2, workH, workL ) != TA_SUCCESS
       || b2 != begIdx || n2 != nbElement
       || memcmp( refL, workH, (size_t)nbElement*sizeof(TA_Real) ) != 0
       || memcmp( refS, workL, (size_t)nbElement*sizeof(TA_Real) ) != 0 )
   {
      printf( "Fail: TA_CE aliasing over inHigh / inLow\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   for( i = 0; i < nb; i++ )
      other[i] = history->close[i];
   if( TA_CE( 0, nb-1, history->high, history->low, other, p, x,
              &b2, &n2, other, workL ) != TA_SUCCESS
       || b2 != begIdx || n2 != nbElement
       || memcmp( refL, other, (size_t)nbElement*sizeof(TA_Real) ) != 0
       || memcmp( refS, workL, (size_t)nbElement*sizeof(TA_Real) ) != 0 )
   {
      printf( "Fail: TA_CE aliasing over inClose\n" );
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
static TA_RetCode ceRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                       TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                       TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                       TA_Integer *lookback, void *opaqueData,
                                       unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   static TA_Real other[CE_CAP];
   TA_RetCode retCode;

   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_CE_Lookback( 22, 3.0 );

   if( outputNb == 0 )
      retCode = TA_CE( startIdx, endIdx, h->high, h->low, h->close,
                       22, 3.0, outBegIdx, outNbElement,
                       outputBuffer, other );
   else
      retCode = TA_CE( startIdx, endIdx, h->high, h->low, h->close,
                       22, 3.0, outBegIdx, outNbElement,
                       other, outputBuffer );

   return retCode;
}

static ErrorNumber test_ce_range( const TA_History *history )
{
   return doRangeTestEx( ceRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_ATR,
                         (void *)history, 2, 0 );
}

/* (7) pandas-ta-classic 0.6.52 `ce(high, low, close, length, multiplier)`
 * (pandas 3.0.3, numpy 2.5.1) on this corpus, run through ta-lib-oracles
 * `capture_568_ce.py` at 90f5e1b; its CE_L column is the long line and
 * its CE_S column the short one. Chosen because its Wilder average is on
 * TA_ATR's bar from bar P on: the bar-0 true range implementations
 * (TradingView, LEAN, Tulip) are 1.1e-3 away at bar 22 on this corpus, and
 * on a 1200-bar series LEAN at 22/3 is still 7.2e-10 off from bar 300 on
 * (#568), so this corpus cannot freeze them. Its bar P-1 row, seeded one bar early, has no
 * counterpart here and is not frozen. A multiplier of 0 has no row either: pandas replaces it
 * with its default.
 *
 * 1e-14 relative. The two are the same arithmetic in a different order, and
 * the measured gap is 2.2e-16 at most over these rows.
 */
typedef struct { int p; double x; int bar; double lng; double shrt; } CeGolden;

static const CeGolden ceGolden[] = {
   { 22, 3.0,  22, 89.746818181818185, 94.503181818181815 },
   { 22, 3.0,  23, 89.756735537190082, 92.148264462809919 },
   { 22, 3.0,  50, 84.487485174432464, 91.012514825567536 },
   { 22, 3.0, 125, 120.93692847877892, 122.87307152122108 },
   { 22, 3.0, 200, 118.74679680062947, 116.00320319937053 },
   { 22, 3.0, 251, 111.62064300832449, 112.74935699167551 },
   { 10, 1.0,  10, 93.450000000000003, 92.364999999999995 },
   { 10, 1.0,  11, 96.642499999999998, 92.422499999999999 },
   { 10, 1.0,  50, 90.460604836563675, 87.574395163436321 },
   { 10, 1.0, 125, 128.52876272568332, 125.28123727431668 },
   { 10, 1.0, 200, 118.61622609032271, 108.00377390967729 },
   { 10, 1.0, 251, 108.03196852027311, 109.6480314797269 },
   {  5, 2.0,   5, 91.272999999999996, 96.507000000000005 },
   {  5, 2.0,   6, 91.093400000000003, 97.281599999999997 },
   {  5, 2.0,  50, 88.547153636164524, 92.987846363835473 },
   {  5, 2.0, 125, 124.91291256342313, 129.52708743657686 },
   {  5, 2.0, 200, 102.69695581123968, 112.04304418876033 },
   {  5, 2.0, 251, 105.78811106075358, 111.58188893924643 },
   {  2, 0.5,   2, 94.960000000000008, 92.819999999999993 },
   {  2, 0.5,   3, 94.995000000000005, 94.879999999999995 },
   {  2, 0.5,  50, 90.853644263098204, 89.896355736901796 },
   {  2, 0.5, 125, 130.49753465415509, 129.75246534584491 },
   {  2, 0.5, 200, 107.14187785390452, 106.54812214609548 },
   {  2, 0.5, 251, 109.3078751036619, 107.8121248963381 },
   { 14, 2.5,  14, 90.679464285714289, 95.695535714285711 },
   { 14, 2.5,  15, 90.905931122448976, 95.469068877551024 },
   { 14, 2.5,  50, 86.352230353847034, 89.147769646152966 },
   { 14, 2.5, 125, 122.84792695315977, 124.15207304684023 },
   { 14, 2.5, 200, 112.98158828364971, 114.13841171635029 },
   { 14, 2.5, 251, 104.59797629707965, 112.65202370292035 },
   { 40, 2.0,  40, 93.348250000000007, 87.151749999999993 },
   { 40, 2.0,  41, 93.248918750000001, 87.251081249999999 },
   { 40, 2.0,  50, 93.625655086309152, 86.874344913690848 },
   { 40, 2.0, 125, 124.89963154374283, 112.88036845625717 },
   { 40, 2.0, 200, 129.81325752626634, 111.74674247373368 },
   { 40, 2.0, 251, 114.73436590967198, 97.385634090328026 }
};

static ErrorNumber test_ce_golden( const TA_History *history )
{
   static TA_Real gotL[CE_CAP], gotS[CE_CAP];
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int g;

   for( g = 0; g < (int)(sizeof(ceGolden)/sizeof(ceGolden[0])); g++ )
   {
      const CeGolden *row = &ceGolden[g];
      double l, s;

      if( TA_CE( 0, nb-1, history->high, history->low, history->close,
                 row->p, row->x, &begIdx, &nbElement, gotL, gotS )
             != TA_SUCCESS
          || row->bar < begIdx || row->bar >= begIdx + nbElement )
      {
         printf( "Fail: TA_CE golden %d/%g: bar %d is outside the output\n",
                 row->p, row->x, row->bar );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      l = gotL[row->bar - begIdx];
      s = gotS[row->bar - begIdx];
      if( !(fabs( l - row->lng ) <= CE_GOLDEN_TOL * fabs( row->lng ))
          || !(fabs( s - row->shrt ) <= CE_GOLDEN_TOL * fabs( row->shrt )) )
      {
         printf( "Fail: TA_CE golden %d/%g bar %d: long %.17g vs %.17g, "
                 "short %.17g vs %.17g\n", row->p, row->x, row->bar,
                 l, row->lng, s, row->shrt );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}
