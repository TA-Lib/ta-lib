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
   {  2, 1.5,  2 }
};
#define NB_CKSP_CFG (int)(sizeof(ckspCfg)/sizeof(ckspCfg[0]))

static ErrorNumber test_cksp_composite ( const TA_History *history );
static ErrorNumber test_cksp_zero_mult ( const TA_History *history );
static ErrorNumber test_cksp_lookback  ( const TA_History *history );
static ErrorNumber test_cksp_flat      ( void );
static ErrorNumber test_cksp_aliasing  ( const TA_History *history );
static ErrorNumber test_cksp_range     ( const TA_History *history );

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

   return test_cksp_range( history );
}

/**** Local functions definitions.     ****/

/* (1) Bitwise against the composition, from several starts. */
static ErrorNumber test_cksp_composite( const TA_History *history )
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
      }
   }

   return TA_TEST_PASS;
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

/* (3) The lookback is atr_lookback(p) + q - 1, so it moves with the ATR's
 * unstable period. Asserted at the default and again after the setting moves,
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
 * CONVERGING, not EPSILON. The extremes are finite windows, but the Average
 * True Range under them is a recursion seeded at this call's own start, so two
 * calls entered at different bars hold different residues and only converge as
 * the unstable period is warmed. doRangeTestEx cross-checks the pair: EPSILON
 * with a non-NONE id is rejected, which is what caught this classification
 * (test_cvi.c:722-726 records the same rule).
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
