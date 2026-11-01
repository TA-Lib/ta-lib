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
 *  103126 MF,CC  First version (#570).
 */

/* Description:
 *
 *   Test TA_MACDV (MACD-V, volatility-normalised MACD).
 *
 *   Legs:
 *     1. COMPOSITE, bitwise, from several starts: the line is TA_EMA(fast)
 *        minus TA_EMA(slow) over TA_ATR(slow), times 100, the three entered
 *        lookback(signal) bars before the first output; the signal is TA_EMA
 *        of that line. Cold, and under non-zero EMA and ATR unstable periods.
 *        At zero unstable periods, also TA_MACD(signal 1) over TA_ATR.
 *     2. LOOKBACK tier under each unstable period, and the fast/slow swap.
 *     3. ATR == 0: a moving prefix then a flat run, swept over startIdx until
 *        the ATR window lies in the run. Both 0/0 and x/0 must be reached.
 *        And a long flat run every leg was entered before, where the ATR
 *        only decays toward zero: the line must settle on 0.0, never grow.
 *        And the corpus scaled by powers of two: the outputs must not move.
 *        And closes outside their bars, under EMAs far above a tiny ATR.
 *     4. SIGNAL PERIOD 1: the signal is the line, the histogram zero.
 *     5. ALIASING of each output over each input.
 *     6. The generic start/end range sweep, both unstable ids.
 *     7. GOLDEN rows from ta4j.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"
#include "test_codegen.h"

/**** Local declarations. ****/
#define MV_CAP 300

typedef struct { int fast; int slow; int signal; } MvCfg;

static const MvCfg mvCfg[] = {
   { 12, 26,  9 },
   { 26, 12,  9 },
   {  3, 10, 16 },
   {  2,  2,  1 },
   {  5, 35,  5 },
   { 12, 26,  1 },
   {  2,  3, 30 },
   {  9,  9,  4 }
};
#define NB_MV_CFG (int)(sizeof(mvCfg)/sizeof(mvCfg[0]))

typedef struct { unsigned int ema; unsigned int atr; } MvUnst;

static const MvUnst mvUnst[] = { { 0, 0 }, { 0, 3 }, { 5, 0 }, { 2, 9 } };
#define NB_MV_UNST (int)(sizeof(mvUnst)/sizeof(mvUnst[0]))

static ErrorNumber test_macdv_composite( const TA_History *history );
static ErrorNumber test_macdv_lookback ( const TA_History *history );
static ErrorNumber test_macdv_flat     ( const TA_History *history );
static ErrorNumber test_macdv_long_flat( const TA_History *history );
static ErrorNumber test_macdv_scale    ( const TA_History *history );
static ErrorNumber test_macdv_odd_bars ( void );
static ErrorNumber test_macdv_signal1  ( const TA_History *history );
static ErrorNumber test_macdv_aliasing ( const TA_History *history );
static ErrorNumber test_macdv_range    ( const TA_History *history );
static ErrorNumber test_macdv_golden   ( const TA_History *history );

static void mv_set_unst( MvUnst u )
{
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, u.ema );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, u.atr );
}

static void mv_clear_unst( void )
{
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 0 );
}

/* The line as a caller composing TA_EMA and TA_ATR computes it, with the
 * neutral point MACDV answers where the spread or the ATR is zero at the
 * EMAs' rounding scale, or the ATR is subnormal. slow is after the swap.
 */
static TA_Real mv_line( TA_Real emaF, TA_Real emaS, TA_Real atr, int slow )
{
   TA_Real spread = emaF - emaS;
   TA_Real scale = fabs( emaF ) > fabs( emaS ) ? fabs( emaF ) : fabs( emaS );

   scale = scale * (double)(slow + 1);
   if( TA_IS_ZERO_SCALED(spread, scale) || TA_IS_ZERO_SCALED(atr, scale)
       || atr < DBL_MIN )
      return 0.0;
   return spread / atr * 100.0;
}

static int mv_same( const TA_Real *a, const TA_Real *b, int n )
{
   return memcmp( a, b, (size_t)n*sizeof(TA_Real) ) == 0;
}

/**** Global functions definitions.   ****/
ErrorNumber test_func_macdv( TA_History *history )
{
   ErrorNumber retValue;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_macdv_composite( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_macdv_lookback( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_macdv_flat( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_macdv_long_flat( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_macdv_scale( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_macdv_odd_bars();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_macdv_signal1( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_macdv_aliasing( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_macdv_range( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return test_macdv_golden( history );
}

/**** Local functions definitions.     ****/

/* The reference: each callee entered at lineIdx, startIdx less the signal
 * EMA's lookback, which is where its own lookback lets it start only because
 * startIdx is at or past MACDV's. Writes the line from lineIdx on into
 * refLine and the three outputs from startIdx on. Returns 0 if a callee
 * refused or answered another range.
 */
static int mv_reference( const TA_Real *high, const TA_Real *low,
                         const TA_Real *close, int nb, int startIdx,
                         int fast, int slow, int signal,
                         TA_Real *refLine, TA_Real *refL, TA_Real *refS,
                         TA_Real *refH, int *outNb )
{
   static TA_Real emaF[MV_CAP], emaS[MV_CAP], atr[MV_CAP];
   TA_Integer b1, n1, b2, n2, b3, n3, b4, n4;
   int t, lineIdx, nLine, i;

   if( slow < fast ) { t = slow; slow = fast; fast = t; }
   lineIdx = startIdx - TA_EMA_Lookback( signal );

   if( TA_EMA( lineIdx, nb-1, close, fast, &b1, &n1, emaF ) != TA_SUCCESS
       || TA_EMA( lineIdx, nb-1, close, slow, &b2, &n2, emaS ) != TA_SUCCESS
       || TA_ATR( lineIdx, nb-1, high, low, close, slow, &b3, &n3, atr )
          != TA_SUCCESS
       || b1 != lineIdx || b2 != lineIdx || b3 != lineIdx
       || n1 != nb-lineIdx || n2 != n1 || n3 != n1 )
      return 0;

   nLine = n1;
   for( i = 0; i < nLine; i++ )
      refLine[i] = mv_line( emaF[i], emaS[i], atr[i], slow );

   if( TA_EMA( startIdx-lineIdx, nLine-1, refLine, signal, &b4, &n4, refS )
          != TA_SUCCESS
       || b4 != startIdx-lineIdx || n4 != nb-startIdx )
      return 0;

   for( i = 0; i < n4; i++ )
   {
      refL[i] = refLine[i+b4];
      refH[i] = refL[i] - refS[i];
   }
   *outNb = n4;
   return 1;
}

/* (1) route: 0 for nothing, 1 for the rows the sweep never sends (two
 * parameters off their defaults), 2 for every row.
 */
static ErrorNumber mv_composite_at( const TA_History *history, int route,
                                    const char *label )
{
   static TA_Real gotL[MV_CAP], gotS[MV_CAP], gotH[MV_CAP];
   static TA_Real refLine[MV_CAP], refL[MV_CAP], refS[MV_CAP], refH[MV_CAP];
   static const int startBump[] = { 0, 1, 7, 60 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int c, s, i, refNb;

   for( c = 0; c < NB_MV_CFG; c++ )
   {
      const MvCfg *k = &mvCfg[c];
      int lookback = TA_MACDV_Lookback( k->fast, k->slow, k->signal );
      int offDefault = (k->fast != 12) + (k->slow != 26) + (k->signal != 9);

      for( s = 0; s < (int)(sizeof(startBump)/sizeof(startBump[0])); s++ )
      {
         int startIdx = lookback + startBump[s];

         if( startIdx > nb-1 )
            continue;

         rc = TA_MACDV( startIdx, nb-1, history->high, history->low,
                        history->close, k->fast, k->slow, k->signal,
                        &begIdx, &nbElement, gotL, gotS, gotH );
         if( rc != TA_SUCCESS || begIdx != startIdx || nbElement != nb-startIdx )
         {
            printf( "Fail: TA_MACDV %d/%d/%d start %d (%s): rc=%d shape "
                    "(%d,%d), expected (%d,%d)\n", k->fast, k->slow, k->signal,
                    startIdx, label, (int)rc, (int)begIdx, (int)nbElement,
                    startIdx, nb-startIdx );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         if( !mv_reference( history->high, history->low, history->close, nb,
                            startIdx, k->fast, k->slow, k->signal,
                            refLine, refL, refS, refH, &refNb )
             || refNb != nbElement )
         {
            printf( "Fail: TA_MACDV %d/%d/%d start %d (%s): the composition "
                    "did not line up\n", k->fast, k->slow, k->signal, startIdx,
                    label );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nbElement; i++ )
         {
            if( memcmp( &gotL[i], &refL[i], sizeof(TA_Real) ) != 0
                || memcmp( &gotS[i], &refS[i], sizeof(TA_Real) ) != 0
                || memcmp( &gotH[i], &refH[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "Fail: TA_MACDV %d/%d/%d start %d (%s) at out[%d]: "
                       "line %.17g vs %.17g, signal %.17g vs %.17g, hist "
                       "%.17g vs %.17g (must be BIT-exact)\n", k->fast,
                       k->slow, k->signal, startIdx, label, i, gotL[i],
                       refL[i], gotS[i], refS[i], gotH[i], refH[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         if( route && startBump[s] == 0 && (route == 2 || offDefault >= 2)
             && server_verify_active() )
         {
            double optIn[3];
            ErrorNumber e;

            optIn[0] = (double)k->fast;
            optIn[1] = (double)k->slow;
            optIn[2] = (double)k->signal;
            e = server_verify( "MACDV", startIdx, nb-1, nb,
                               rc, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low,
                                                   history->close, NULL },
                               optIn, 3,
                               (const TA_Real*[]){ gotL, gotS, gotH, NULL },
                               NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }
   }

   return TA_TEST_PASS;
}

/* At zero unstable periods TA_MACD at a signal period of 1 is the raw spread
 * entered at the same bar, so the line is also TA_MACD over TA_ATR. Not under
 * an EMA unstable period: MACD's own signal stage then adds that period.
 */
static ErrorNumber mv_macd_identity( const TA_History *history )
{
   static TA_Real gotL[MV_CAP], gotS[MV_CAP], gotH[MV_CAP];
   static TA_Real macd[MV_CAP], msig[MV_CAP], mhist[MV_CAP], atr[MV_CAP];
   static TA_Real emaF[MV_CAP], emaS[MV_CAP];
   TA_Integer begIdx, nbElement, b2, n2, b3, n3, b4, n4;
   int nb = (int)history->nbBars;
   int c, i, t;

   for( c = 0; c < NB_MV_CFG; c++ )
   {
      int fast = mvCfg[c].fast, slow = mvCfg[c].slow, signal = mvCfg[c].signal;
      int startIdx = TA_MACDV_Lookback( fast, slow, signal );
      int a = startIdx - TA_EMA_Lookback( signal );

      if( TA_MACDV( startIdx, nb-1, history->high, history->low,
                    history->close, fast, slow, signal,
                    &begIdx, &nbElement, gotL, gotS, gotH ) != TA_SUCCESS
          || begIdx != startIdx )
      {
         printf( "Fail: TA_MACDV %d/%d/%d: rc or begIdx at start %d\n",
                 fast, slow, signal, startIdx );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( slow < fast ) { t = slow; slow = fast; fast = t; }
      if( TA_MACD( a, nb-1, history->close, fast, slow, 1,
                   &b2, &n2, macd, msig, mhist ) != TA_SUCCESS
          || TA_ATR( a, nb-1, history->high, history->low, history->close,
                     slow, &b3, &n3, atr ) != TA_SUCCESS
          || TA_EMA( a, nb-1, history->close, fast, &b4, &n4, emaF ) != TA_SUCCESS
          || TA_EMA( a, nb-1, history->close, slow, &b4, &n4, emaS ) != TA_SUCCESS
          || b2 != a || b3 != a || b4 != a || n2 != n3 || n4 != n3
          || n2 != nbElement + (startIdx-a) )
      {
         printf( "Fail: TA_MACDV vs TA_MACD %d/%d/1 and TA_ATR %d from %d: "
                 "shape (%d,%d) (%d,%d)\n", fast, slow, slow, a, (int)b2,
                 (int)n2, (int)b3, (int)n3 );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         int j = i + (startIdx - a);
         TA_Real spread = emaF[j] - emaS[j];
         TA_Real want = mv_line( emaF[j], emaS[j], atr[j], slow );

         if( memcmp( &macd[j], &spread, sizeof(TA_Real) ) != 0
             || memcmp( &gotL[i], &want, sizeof(TA_Real) ) != 0 )
         {
            printf( "Fail: TA_MACDV %d/%d/%d at out[%d]: %.17g, TA_MACD %.17g "
                    "over TA_ATR %.17g\n", mvCfg[c].fast, mvCfg[c].slow, signal,
                    i, gotL[i], macd[j], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* Cold, then warm. A warm period on a function carrying no
 * TA_FUNC_FLG_UNST_PER of its own is one the sweep never sends, so every warm
 * row is routed.
 */
static ErrorNumber test_macdv_composite( const TA_History *history )
{
   ErrorNumber retValue;
   char label[64];
   int u;

   retValue = mv_macd_identity( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   for( u = 0; u < NB_MV_UNST; u++ )
   {
      sprintf( label, "unstable EMA %u ATR %u", mvUnst[u].ema, mvUnst[u].atr );
      mv_set_unst( mvUnst[u] );
      retValue = mv_composite_at( history, u == 0 ? 1 : 2, label );
      mv_clear_unst();
      if( retValue != TA_TEST_PASS )
         return retValue;
   }

   return TA_TEST_PASS;
}

/* (2) The lookback is the later of the slow EMA's and the ATR's, plus the
 * signal EMA's, each read from its own function, so it moves with either
 * unstable period. The swap is symmetric in the lookback and in the values.
 */
static ErrorNumber test_macdv_lookback( const TA_History *history )
{
   static TA_Real gotL[MV_CAP], gotS[MV_CAP], gotH[MV_CAP];
   static TA_Real swL[MV_CAP], swS[MV_CAP], swH[MV_CAP];
   TA_Integer begIdx, nbElement, b2, n2;
   int nb = (int)history->nbBars;
   int u, c;

   for( u = 0; u < NB_MV_UNST; u++ )
   {
      mv_set_unst( mvUnst[u] );

      for( c = 0; c < NB_MV_CFG; c++ )
      {
         int fast = mvCfg[c].fast, slow = mvCfg[c].slow, signal = mvCfg[c].signal;
         int hi = slow > fast ? slow : fast;
         int e = TA_EMA_Lookback( hi ), a = TA_ATR_Lookback( hi );
         int want = (a > e ? a : e) + TA_EMA_Lookback( signal );
         int lookback = TA_MACDV_Lookback( fast, slow, signal );

         if( lookback != want
             || TA_MACDV_Lookback( slow, fast, signal ) != want )
         {
            printf( "Fail: TA_MACDV_Lookback %d/%d/%d = %d (swapped %d), "
                    "expected %d (unstable EMA %u ATR %u)\n", fast, slow,
                    signal, lookback, TA_MACDV_Lookback( slow, fast, signal ),
                    want, mvUnst[u].ema, mvUnst[u].atr );
            mv_clear_unst();
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         if( lookback > nb-1 )
            continue;

         if( TA_MACDV( 0, nb-1, history->high, history->low, history->close,
                       fast, slow, signal, &begIdx, &nbElement,
                       gotL, gotS, gotH ) != TA_SUCCESS
             || begIdx != lookback || nbElement != nb-lookback
             || TA_MACDV( 0, nb-1, history->high, history->low, history->close,
                          slow, fast, signal, &b2, &n2,
                          swL, swS, swH ) != TA_SUCCESS
             || b2 != begIdx || n2 != nbElement )
         {
            printf( "Fail: TA_MACDV %d/%d/%d unstable EMA %u ATR %u: shape "
                    "(%d,%d), expected (%d,%d)\n", fast, slow, signal,
                    mvUnst[u].ema, mvUnst[u].atr, (int)begIdx,
                    (int)nbElement, lookback, nb-lookback );
            mv_clear_unst();
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( !mv_same( gotL, swL, nbElement ) || !mv_same( gotS, swS, nbElement )
             || !mv_same( gotH, swH, nbElement ) )
         {
            printf( "Fail: TA_MACDV %d/%d/%d differs from %d/%d/%d\n", fast,
                    slow, signal, slow, fast, signal );
            mv_clear_unst();
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   mv_clear_unst();
   return TA_TEST_PASS;
}

/* (3) ATR == 0. Bars 0..MV_FLAT_AT-1 are the corpus, the rest one price.
 * The ATR is exactly zero only once its whole seed window, previous closes
 * included, lies in the run, which the minimum start never reaches, so
 * startIdx is swept. Every output is held bitwise to the reference, which
 * pins the zero test as tightly as the value test, and must be finite.
 *
 * Both degenerate quotients are built, not left to rounding: fast == slow
 * makes the spread exactly zero (0/0), and an EMA unstable period seeds both
 * EMAs back in the moving prefix while the ATR window is all flat, a spread
 * far above any rounding band over a zero ATR (x/0). The warm settings also
 * run the loop between the signal seed and the first output, which a cold
 * call never enters.
 */
#define MV_FLAT_N  200
#define MV_FLAT_AT 60

static ErrorNumber test_macdv_flat( const TA_History *history )
{
   static TA_Real high[MV_FLAT_N], low[MV_FLAT_N], close[MV_FLAT_N];
   static TA_Real gotL[MV_FLAT_N], gotS[MV_FLAT_N], gotH[MV_FLAT_N];
   static TA_Real refLine[MV_FLAT_N], refL[MV_FLAT_N], refS[MV_FLAT_N], refH[MV_FLAT_N];
   static TA_Real emaF[MV_FLAT_N], emaS[MV_FLAT_N], atr[MV_FLAT_N];
   static const double level[] = { 130.0, 130.7 };
   static const MvCfg flatCfg[] = { { 12, 26, 9 }, { 9, 9, 4 } };
   static const MvUnst flatUnst[] = { { 0, 0 }, { 5, 0 }, { 2, 9 } };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, b1, n1, b2, n2, b3, n3;
   int nbZeroOverZero = 0, nbXOverZero = 0;
   int v, c, u, i, startIdx, refNb, routed;

   for( u = 0; u < (int)(sizeof(flatUnst)/sizeof(flatUnst[0])); u++ )
   for( c = 0; c < (int)(sizeof(flatCfg)/sizeof(flatCfg[0])); c++ )
   for( v = 0; v < (int)(sizeof(level)/sizeof(level[0])); v++ )
   {
      int fast = flatCfg[c].fast, slow = flatCfg[c].slow, signal = flatCfg[c].signal;
      int lookback, lineLb;

      mv_set_unst( flatUnst[u] );
      lookback = TA_MACDV_Lookback( fast, slow, signal );
      lineLb = TA_EMA_Lookback( signal );

      for( i = 0; i < MV_FLAT_N; i++ )
      {
         high[i]  = i < MV_FLAT_AT ? history->high[i]  : level[v];
         low[i]   = i < MV_FLAT_AT ? history->low[i]   : level[v];
         close[i] = i < MV_FLAT_AT ? history->close[i] : level[v];
      }

      routed = 0;
      for( startIdx = lookback; startIdx < MV_FLAT_N; startIdx++ )
      {
         int lineIdx = startIdx - lineLb;
         int zeroAtFirst;

         rc = TA_MACDV( startIdx, MV_FLAT_N-1, high, low, close,
                        fast, slow, signal, &begIdx, &nbElement,
                        gotL, gotS, gotH );
         if( rc != TA_SUCCESS || begIdx != startIdx
             || nbElement != MV_FLAT_N-startIdx
             || !mv_reference( high, low, close, MV_FLAT_N, startIdx,
                               fast, slow, signal, refLine, refL, refS, refH, &refNb )
             || refNb != nbElement
             || TA_EMA( lineIdx, MV_FLAT_N-1, close, fast, &b1, &n1, emaF ) != TA_SUCCESS
             || TA_EMA( lineIdx, MV_FLAT_N-1, close, slow, &b2, &n2, emaS ) != TA_SUCCESS
             || TA_ATR( lineIdx, MV_FLAT_N-1, high, low, close, slow,
                        &b3, &n3, atr ) != TA_SUCCESS
             || b1 != lineIdx || b2 != lineIdx || b3 != lineIdx )
         {
            printf( "Fail: TA_MACDV flat %g %d/%d/%d unstable EMA %u ATR %u "
                    "start %d: rc=%d shape (%d,%d)\n", level[v], fast, slow,
                    signal, flatUnst[u].ema, flatUnst[u].atr, startIdx,
                    (int)rc, (int)begIdx, (int)nbElement );
            mv_clear_unst();
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nbElement; i++ )
         {
            if( !isfinite( gotL[i] ) || !isfinite( gotS[i] ) || !isfinite( gotH[i] )
                || memcmp( &gotL[i], &refL[i], sizeof(TA_Real) ) != 0
                || memcmp( &gotS[i], &refS[i], sizeof(TA_Real) ) != 0
                || memcmp( &gotH[i], &refH[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "Fail: TA_MACDV flat %g %d/%d/%d unstable EMA %u ATR %u "
                       "start %d at out[%d]: line %.17g vs %.17g, signal %.17g "
                       "vs %.17g, hist %.17g vs %.17g\n", level[v], fast, slow,
                       signal, flatUnst[u].ema, flatUnst[u].atr, startIdx, i,
                       gotL[i], refL[i], gotS[i], refS[i], gotH[i], refH[i] );
               mv_clear_unst();
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         /* Counted at the first line bar, by construction: the two EMAs are
          * one computation when fast == slow, and a warm EMA's spread is
          * whole percent of the price.
          */
         zeroAtFirst = atr[0] == 0.0;
         if( zeroAtFirst && fast == slow && emaF[0] - emaS[0] == 0.0 )
            nbZeroOverZero++;
         if( zeroAtFirst && flatUnst[u].ema > 0
             && fabs( emaF[0] - emaS[0] ) > 1e-6 * level[v] )
            nbXOverZero++;

         if( server_verify_active() && (startIdx == lookback
                                        || (zeroAtFirst && !routed)) )
         {
            double optIn[3];
            ErrorNumber e;

            if( zeroAtFirst )
               routed = 1;
            optIn[0] = (double)fast;
            optIn[1] = (double)slow;
            optIn[2] = (double)signal;
            e = server_verify( "MACDV", startIdx, MV_FLAT_N-1, MV_FLAT_N,
                               rc, begIdx, nbElement,
                               (const TA_Real*[]){ high, low, close, NULL },
                               optIn, 3,
                               (const TA_Real*[]){ gotL, gotS, gotH, NULL },
                               NULL );
            if( e != TA_TEST_PASS )
            {
               mv_clear_unst();
               return e;
            }
         }
      }
   }
   mv_clear_unst();

   if( nbZeroOverZero == 0 || nbXOverZero == 0 )
   {
      printf( "Fail: TA_MACDV flat: 0/0 met %d times, x/0 %d times; both must "
              "be met\n", nbZeroOverZero, nbXOverZero );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (3c) Scale invariance: the corpus multiplied by 2^-40 and by 2^40 gives
 * the same three outputs bit for bit. Every operation scales exactly, and so
 * does a zero test relative to the price; an absolute one does not.
 */
static ErrorNumber test_macdv_scale( const TA_History *history )
{
   static TA_Real high[MV_CAP], low[MV_CAP], close[MV_CAP];
   static TA_Real ref[3][MV_CAP], got[3][MV_CAP];
   static const double factor[] = { 0x1p-40, 0x1p40 };
   TA_Integer begIdx, nbElement, b2, n2;
   int nb = (int)history->nbBars;
   int f, i;

   if( TA_MACDV( 0, nb-1, history->high, history->low, history->close,
                 12, 26, 9, &begIdx, &nbElement, ref[0], ref[1], ref[2] )
          != TA_SUCCESS || nbElement <= 0 )
   {
      printf( "Fail: TA_MACDV scale: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( f = 0; f < (int)(sizeof(factor)/sizeof(factor[0])); f++ )
   {
      for( i = 0; i < nb; i++ )
      {
         high[i]  = history->high[i]  * factor[f];
         low[i]   = history->low[i]   * factor[f];
         close[i] = history->close[i] * factor[f];
      }
      if( TA_MACDV( 0, nb-1, high, low, close, 12, 26, 9,
                    &b2, &n2, got[0], got[1], got[2] ) != TA_SUCCESS
          || b2 != begIdx || n2 != nbElement
          || !mv_same( ref[0], got[0], nbElement )
          || !mv_same( ref[1], got[1], nbElement )
          || !mv_same( ref[2], got[2], nbElement ) )
      {
         printf( "Fail: TA_MACDV prices times %a differ from the unscaled "
                 "outputs\n", factor[f] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3b) The corpus, then a flat run every leg was entered before. The ATR
 * never reaches exactly zero there and the spread stalls at the EMAs'
 * rounding residue, so with an exact test their quotient overflows. From
 * `settle` bars into the run every line value must be +0.0, and every output
 * must stay finite. Each case is decided by one test alone:
 *   - slow 26: the spread falls inside the band while the ATR is still
 *     orders above it (measured: zero from 352 bars on);
 *   - slow 300: the same, but the residue is wider than a band without the
 *     period factor (zero from 3739 bars on);
 *   - price 0: no scale is left, only the ATR's subnormal floor decides
 *     (zero from 18113 bars on; earlier on a host that flushes subnormals).
 * Routed where the request fits server_verify's 256 KB buffer, about 5000
 * bars of three inputs: the sweep sends no such series.
 */
typedef struct { double level; int slow; int flat; int settle; } MvLongFlat;

static const MvLongFlat mvLongFlat[] = {
   { 130.0,  26,  3000,   600 },
   { 130.7,  26,  3000,   600 },
   { 130.7, 300,  6000,  5000 },
   {   0.0,  26, 20000, 19000 }
};
#define MV_LONG_MAX (MV_CAP + 20000)
#define MV_SV_MAX_BARS 5000

static ErrorNumber test_macdv_long_flat( const TA_History *history )
{
   static TA_Real high[MV_LONG_MAX], low[MV_LONG_MAX], close[MV_LONG_MAX];
   static TA_Real gotL[MV_LONG_MAX], gotS[MV_LONG_MAX], gotH[MV_LONG_MAX];
   const TA_Real zero = 0.0;
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int v, i, nbSettled;

   for( v = 0; v < (int)(sizeof(mvLongFlat)/sizeof(mvLongFlat[0])); v++ )
   {
      const MvLongFlat *k = &mvLongFlat[v];
      int n = nb + k->flat;

      for( i = 0; i < n; i++ )
      {
         high[i]  = i < nb ? history->high[i]  : k->level;
         low[i]   = i < nb ? history->low[i]   : k->level;
         close[i] = i < nb ? history->close[i] : k->level;
      }

      rc = TA_MACDV( 0, n-1, high, low, close, 12, k->slow, 9,
                     &begIdx, &nbElement, gotL, gotS, gotH );
      if( rc != TA_SUCCESS || nbElement != n - begIdx )
      {
         printf( "Fail: TA_MACDV long flat %g 12/%d/9: rc=%d shape (%d,%d)\n",
                 k->level, k->slow, (int)rc, (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      nbSettled = 0;
      for( i = 0; i < nbElement; i++ )
      {
         if( !isfinite( gotL[i] ) || !isfinite( gotS[i] ) || !isfinite( gotH[i] ) )
         {
            printf( "Fail: TA_MACDV long flat %g 12/%d/9 at bar %d: line %.17g "
                    "signal %.17g hist %.17g\n", k->level, k->slow, begIdx+i,
                    gotL[i], gotS[i], gotH[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( begIdx+i >= nb + k->settle )
         {
            if( memcmp( &gotL[i], &zero, sizeof(TA_Real) ) != 0 )
            {
               printf( "Fail: TA_MACDV long flat %g 12/%d/9 at bar %d, %d bars "
                       "into the run: line %.17g, expected +0.0\n", k->level,
                       k->slow, begIdx+i, begIdx+i-nb, gotL[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            nbSettled++;
         }
      }
      if( nbSettled != k->flat - k->settle )
      {
         printf( "Fail: TA_MACDV long flat %g 12/%d/9: %d settled bars "
                 "compared, expected %d\n", k->level, k->slow, nbSettled,
                 k->flat - k->settle );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      if( server_verify_active() && n <= MV_SV_MAX_BARS )
      {
         double optIn[3];
         ErrorNumber e;

         optIn[0] = 12.0;
         optIn[1] = (double)k->slow;
         optIn[2] = 9.0;
         e = server_verify( "MACDV", 0, n-1, n, rc, begIdx, nbElement,
                            (const TA_Real*[]){ high, low, close, NULL },
                            optIn, 3,
                            (const TA_Real*[]){ gotL, gotS, gotH, NULL },
                            NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (3d) A close outside its own bar, which no layer rejects. Here every True
 * Range but one is zero, so the ATR is a subnormal-sized remnant of 1e-300
 * under EMAs of 1e30, and the last close is set to values that would make a
 * scale taken from the close vanish. Every output must be finite and every
 * line value +0.0.
 */
#define MV_ODD_N 41

static ErrorNumber test_macdv_odd_bars( void )
{
   static TA_Real high[MV_ODD_N], low[MV_ODD_N], close[MV_ODD_N];
   static TA_Real gotL[MV_ODD_N], gotS[MV_ODD_N], gotH[MV_ODD_N];
   static const double lastClose[] = { 0.0, 1e-290, -1e-290, 1.0 };
   const TA_Real zero = 0.0;
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int v, i;

   for( v = 0; v < (int)(sizeof(lastClose)/sizeof(lastClose[0])); v++ )
   {
      for( i = 0; i < MV_ODD_N; i++ )
      {
         close[i] = i < 2 ? 0.0 : 1e30;
         high[i]  = i < 2 ? 0.0 : close[i-1];
         low[i]   = high[i];
      }
      high[1] = 1e-300;
      close[MV_ODD_N-1] = lastClose[v];

      rc = TA_MACDV( 0, MV_ODD_N-1, high, low, close, 12, 26, 9,
                     &begIdx, &nbElement, gotL, gotS, gotH );
      if( rc != TA_SUCCESS || nbElement <= 0 )
      {
         printf( "Fail: TA_MACDV odd bars, last close %g: rc=%d nbElement=%d\n",
                 lastClose[v], (int)rc, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( !isfinite( gotS[i] ) || !isfinite( gotH[i] )
             || memcmp( &gotL[i], &zero, sizeof(TA_Real) ) != 0 )
         {
            printf( "Fail: TA_MACDV odd bars, last close %g, at out[%d]: line "
                    "%.17g signal %.17g hist %.17g\n", lastClose[v], i,
                    gotL[i], gotS[i], gotH[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      if( server_verify_active() )
      {
         double optIn[3] = { 12.0, 26.0, 9.0 };
         ErrorNumber e = server_verify( "MACDV", 0, MV_ODD_N-1, MV_ODD_N,
                                        rc, begIdx, nbElement,
                                        (const TA_Real*[]){ high, low, close, NULL },
                                        optIn, 3,
                                        (const TA_Real*[]){ gotL, gotS, gotH, NULL },
                                        NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (4) A signal period of 1: the signal is the line, bit for bit, the
 * histogram is +0.0 or -0.0.
 */
static ErrorNumber test_macdv_signal1( const TA_History *history )
{
   static TA_Real gotL[MV_CAP], gotS[MV_CAP], gotH[MV_CAP];
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int i;

   if( TA_MACDV( 0, nb-1, history->high, history->low, history->close,
                 12, 26, 1, &begIdx, &nbElement, gotL, gotS, gotH )
          != TA_SUCCESS || nbElement <= 0 )
   {
      printf( "Fail: TA_MACDV 12/26/1: the call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < nbElement; i++ )
   {
      if( memcmp( &gotL[i], &gotS[i], sizeof(TA_Real) ) != 0 || gotH[i] != 0.0 )
      {
         printf( "Fail: TA_MACDV 12/26/1 at out[%d]: line %.17g signal "
                 "%.17g hist %.17g\n", i, gotL[i], gotS[i], gotH[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) Each output over each input, the other two outputs in scratch. */
static ErrorNumber test_macdv_aliasing( const TA_History *history )
{
   static TA_Real ref[3][MV_CAP], got[3][MV_CAP], work[MV_CAP];
   TA_Integer begIdx, nbElement, b2, n2;
   int nb = (int)history->nbBars;
   int in, out, i;

   if( TA_MACDV( 0, nb-1, history->high, history->low, history->close,
                 12, 26, 9, &begIdx, &nbElement, ref[0], ref[1], ref[2] )
          != TA_SUCCESS || nbElement <= 0 )
   {
      printf( "Fail: TA_MACDV aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( in = 0; in < 3; in++ )
   {
      const TA_Real *src = in == 0 ? history->high
                         : in == 1 ? history->low : history->close;

      for( out = 0; out < 3; out++ )
      {
         TA_Real *o[3];

         for( i = 0; i < nb; i++ )
            work[i] = src[i];
         o[0] = got[0]; o[1] = got[1]; o[2] = got[2];
         o[out] = work;

         if( TA_MACDV( 0, nb-1,
                       in == 0 ? work : history->high,
                       in == 1 ? work : history->low,
                       in == 2 ? work : history->close,
                       12, 26, 9, &b2, &n2, o[0], o[1], o[2] ) != TA_SUCCESS
             || b2 != begIdx || n2 != nbElement
             || !mv_same( ref[0], o[0], nbElement )
             || !mv_same( ref[1], o[1], nbElement )
             || !mv_same( ref[2], o[2], nbElement ) )
         {
            printf( "Fail: TA_MACDV aliasing output %d over input %d\n",
                    out, in );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) The generic start/end sweep. CONVERGING over both ids: every leg is a
 * recursion seeded at the call's own start.
 */
static TA_RetCode macdvRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                          TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                          TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                          TA_Integer *lookback, void *opaqueData,
                                          unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   static TA_Real o[3][MV_CAP];
   TA_RetCode retCode;
   int i;

   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_MACDV_Lookback( 12, 26, 9 );

   retCode = TA_MACDV( startIdx, endIdx, h->high, h->low, h->close,
                       12, 26, 9, outBegIdx, outNbElement, o[0], o[1], o[2] );
   if( retCode == TA_SUCCESS )
      for( i = 0; i < (int)*outNbElement; i++ )
         outputBuffer[i] = o[outputNb][i];

   return retCode;
}

static ErrorNumber test_macdv_range( const TA_History *history )
{
   static const TA_FuncUnstId ids[] = { TA_FUNC_UNST_EMA, TA_FUNC_UNST_ATR };
   return doRangeTestMulti( macdvRangeTestFunction,
                            TA_STABLE_CONVERGING, ids, 2,
                            (void *)history, 3, 0,
                            codegen_range_grace( "MACDV" ) );
}

/* (7) ta4j 0.22.6 `VolatilityNormalizedMACDIndicator(close, fast, slow,
 * slow, signal, 100)`, its `getSignalLine(signal)` and `getHistogram(signal)`
 * (OpenJDK 21.0.12.1, DoubleNum), run through ta-lib-oracles
 * `capture_570_macdv.py` at c959c0a on this corpus tiled 8 times end to end.
 *
 * ta4j seeds every EMA and its Wilder ATR from a single bar where TA-Lib
 * seeds from an SMA, so the early bars cannot be frozen: the rows start at bar
 * 1500, where the two conventions have converged. What is left is the EMA step
 * spelled prev + (x-prev)*k against k*x + beta*prev, amplified by the
 * fast - slow cancellation: 9.5e-13 absolute at most from bar 1500 on, over
 * every configuration captured. 3e-12 absolute; an ATR period off by one, a
 * missing x100 or the histogram's sign each miss by whole percent. ta4j refuses fast > slow, so the swap has no row here (leg 2 pins
 * it), and fast == slow has none either: its line is identically zero.
 */
#define MV_TILES 8
#define MV_GOLDEN_ABS 3e-12
#define MV_GOLDEN_REL 0.0

typedef struct { int fast; int slow; int signal; int bar; double line; double sig; double hist; } MvGolden;

static const MvGolden mvGolden[] = {
   { 12, 26,  9, 1500, 56.966491110905025, 59.213860258454247, -2.2473691475492217 },
   { 12, 26,  9, 1700, -44.173021295881739, -11.933605127418453, -32.239416168463286 },
   { 12, 26,  9, 1900, 110.06054676063211, 134.31910477791411, -24.258558017281999 },
   { 12, 26,  9, 2015, 25.337046697914563, 36.652635891788861, -11.315589193874299 },
   {  5, 35,  5, 1500, 70.682985367928126, 111.73376841689452, -41.050783048966395 },
   {  5, 35,  5, 1700, -120.22653447980449, -82.331736176349395, -37.89479830345509 },
   {  5, 35,  5, 1900, 189.01058922568561, 263.21579080667118, -74.205201580985573 },
   {  5, 35,  5, 2015, 26.928270272848469, 39.435893281545788, -12.507623008697319 },
   {  3, 10, 16, 1500, -22.896056107784108, 48.5934437919027, -71.489499899686805 },
   {  3, 10, 16, 1700, -80.360907103467369, -32.92192720023202, -47.438979903235349 },
   {  3, 10, 16, 1900, -38.795465128187864, 64.850065670723055, -103.64553079891093 },
   {  3, 10, 16, 2015, -17.727722930906406, 7.2086267787181626, -24.936349709624569 },
   {  8, 17,  9, 1500, 38.219436079092489, 64.423251378050239, -26.20381529895775 },
   {  8, 17,  9, 1700, -57.624818437687168, -27.107742141891137, -30.517076295796031 },
   {  8, 17,  9, 1900, 53.038543176133913, 93.207381621179465, -40.168838445045552 },
   {  8, 17,  9, 2015, 4.4381860571910217, 17.873824725576089, -13.435638668385067 },
   {  2,  3, 30, 1500, -13.21989758069437, 3.3214212303805435, -16.541318811074913 },
   {  2,  3, 30, 1700, -17.344646416029562, -4.3106986911485077, -13.033947724881054 },
   {  2,  3, 30, 1900, -25.314081864101407, 6.2727804489805887, -31.586862313081994 },
   {  2,  3, 30, 2015, -9.709660440018725, 0.83009439034315158, -10.539754830361877 },
   { 12, 26,  1, 1500, 56.966491110905025, 56.966491110905025, 0 },
   { 12, 26,  1, 1700, -44.173021295881739, -44.173021295881739, 0 },
   { 12, 26,  1, 1900, 110.06054676063211, 110.06054676063211, 0 },
   { 12, 26,  1, 2015, 25.337046697914563, 25.337046697914563, 0 }
};

static ErrorNumber test_macdv_golden( const TA_History *history )
{
   static TA_Real high[MV_CAP*MV_TILES], low[MV_CAP*MV_TILES], close[MV_CAP*MV_TILES];
   static TA_Real gotL[MV_CAP*MV_TILES], gotS[MV_CAP*MV_TILES], gotH[MV_CAP*MV_TILES];
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int n = nb * MV_TILES;
   int g, i;

   if( nb != 252 )
   {
      printf( "Fail: TA_MACDV golden: captured on the 252-bar corpus, got %d\n", nb );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   for( i = 0; i < n; i++ )
   {
      high[i]  = history->high[i % nb];
      low[i]   = history->low[i % nb];
      close[i] = history->close[i % nb];
   }

   for( g = 0; g < (int)(sizeof(mvGolden)/sizeof(mvGolden[0])); g++ )
   {
      const MvGolden *row = &mvGolden[g];
      double got[3], want[3];
      int k;

      if( TA_MACDV( 0, n-1, high, low, close, row->fast, row->slow, row->signal,
                    &begIdx, &nbElement, gotL, gotS, gotH ) != TA_SUCCESS
          || row->bar < begIdx || row->bar >= begIdx + nbElement )
      {
         printf( "Fail: TA_MACDV golden %d/%d/%d: bar %d is outside the output\n",
                 row->fast, row->slow, row->signal, row->bar );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* The tiled series is one the sweep never sends: replay each
       * configuration once on the language servers.
       */
      if( (g == 0 || mvGolden[g-1].fast != row->fast || mvGolden[g-1].slow != row->slow
           || mvGolden[g-1].signal != row->signal) && server_verify_active() )
      {
         double optIn[3];
         ErrorNumber e;

         optIn[0] = (double)row->fast;
         optIn[1] = (double)row->slow;
         optIn[2] = (double)row->signal;
         e = server_verify( "MACDV", 0, n-1, n, TA_SUCCESS, begIdx, nbElement,
                            (const TA_Real*[]){ high, low, close, NULL },
                            optIn, 3,
                            (const TA_Real*[]){ gotL, gotS, gotH, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }

      got[0] = gotL[row->bar - begIdx];  want[0] = row->line;
      got[1] = gotS[row->bar - begIdx];  want[1] = row->sig;
      got[2] = gotH[row->bar - begIdx];  want[2] = row->hist;
      for( k = 0; k < 3; k++ )
      {
         if( !checkOracleValue( got[k], want[k], MV_GOLDEN_REL, MV_GOLDEN_ABS, NULL, NULL ) )
         {
            printf( "Fail: TA_MACDV golden %d/%d/%d bar %d output %d: %.17g vs "
                    "ta4j %.17g\n", row->fast, row->slow, row->signal, row->bar,
                    k, got[k], want[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}
