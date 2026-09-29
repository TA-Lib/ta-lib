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
 *  092926 KL,CC  First version (#468).
 */

/* Description:
 *
 *   Test TA_IBS (Internal Bar Strength).
 *
 *   Legs:
 *     1. COMPOSITE, bitwise on every bar:
 *          TA_IBS == TA_BOP(L,H,L,C) + (0.5 - 0.5*TA_BOP(L,H,L,H))
 *        That composition is the one spelling that agrees on the degenerate
 *        bars too: its second call divides H-L by itself, so it is exactly 1.0
 *        when the bar has range and exactly 0.0 when it does not, and
 *        0.5 - 0.5*x is then exactly 0.0 or 0.5. Compared as bits, never with
 *        a tolerance -- a tolerance would also pass the reciprocal, the CLV
 *        and the midpoint spellings, which differ only in the last bits.
 *     2. FROZEN GOLDENS on the reference corpus, zero tolerance, as IEEE bit
 *        patterns.
 *     3. DEGENERATE BARS, by hand: flat, flat at zero, flat with an off-bar
 *        close, inverted, a one-ulp range at both ends, a close above the high,
 *        negative prices, and a NaN high. Batch, stream and every language
 *        server. None of these occurs in the corpus, so this leg is the only
 *        thing pinning the guard.
 *     4. SCALE INVARIANCE: the corpus times 2^k is the same bits. This is what
 *        a fixed tolerance band fails.
 *     5. LOOKBACK, ALIASING, RANGE.
 *
 *   No synthetic-series goldens. The series the card measures is built from
 *   sin(), so its inputs are libm's, not the tree's, and a bit pattern frozen
 *   from one libm is not a property of this library (issue #460). The corpus
 *   rows below are decimal literals in test_data.c and carry no such
 *   dependency.
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
#define IBS_MAX_BARS 300

/* The tree's spelling for a bit pattern (test_crsi.c:72): a plain
 * unsigned long long, with the width asserted at compile time. */
typedef char ibsBitsFitADouble[ sizeof(double) == sizeof(unsigned long long) ? 1 : -1 ];

typedef struct { int bar; unsigned long long bits; } IbsRow;

/* The corpus rows, as IEEE bit patterns. Each is the correctly rounded
 * quotient of the bar's own doubles: both subtractions are exact by Sterbenz
 * on these prices, so there is no accumulation and no backend freedom.
 */
static const IbsRow ibsSrefRow[] = {
   {   0, 0x3fd3333333333333ULL },   /* 0.3            */
   {   1, 0x3feede53480e7bd6ULL },
   {   2, 0x3fae1e1e1e1e1e1eULL },
   {  26, 0x3ff0000000000000ULL },   /* exactly 1.0    */
   {  47, 0x0000000000000000ULL },   /* exactly +0.0   */
   {  50, 0x3f9eb851eb852000ULL },
   { 100, 0x3fe9730ca63fd969ULL },
   { 101, 0x3fe6a00000000002ULL },
   { 150, 0x3fd4a062b2e43d97ULL },
   { 200, 0x3fe19191919191a2ULL },
   { 251, 0x3fdbc71c71c71c7eULL }
};

/* Section 5 of the card, proposed column. The NaN row is flagged rather than
 * given a bit pattern: which NaN a division yields is not specified.
 */
typedef struct { double high, low, close, want; int wantNaN; } IbsBar;

static const IbsBar ibsHand[] = {
   { 100.0,              100.0, 100.0,              0.5,  0 },  /* flat            */
   {   0.0,                0.0,   0.0,              0.5,  0 },  /* flat at zero    */
   { 100.0,              100.0, 101.0,              0.5,  0 },  /* flat, close off */
   {  99.0,              101.0, 100.5,              0.5,  0 },  /* inverted        */
   { 100.00000000000001, 100.0, 100.0,              0.0,  0 },  /* 1-ulp range, low  */
   { 100.00000000000001, 100.0, 100.00000000000001, 1.0,  0 },  /* 1-ulp range, high */
   { 101.0,               99.0, 102.0,              1.5,  0 },  /* close above high  */
   {  -1.0,               -3.0,  -2.5,              0.25, 0 },  /* negative prices   */
   {   0.0,               99.0, 100.0,              0.0,  1 }   /* NaN high, see below */
};
#define NB_IBS_HAND (int)(sizeof(ibsHand)/sizeof(ibsHand[0]))

static ErrorNumber test_ibs_composite ( const TA_History *history );
static ErrorNumber test_ibs_sref_rows ( const TA_History *history );
static ErrorNumber test_ibs_degenerate( void );
static ErrorNumber test_ibs_scale     ( const TA_History *history );
static ErrorNumber test_ibs_lookback  ( const TA_History *history );
static ErrorNumber test_ibs_aliasing  ( const TA_History *history );
static ErrorNumber test_ibs_range     ( const TA_History *history );

/**** Global functions definitions.   ****/
ErrorNumber test_func_ibs( TA_History *history )
{
   ErrorNumber retValue;

   /* One bar in, one bar out: no unstable period, and a leftover global
    * setting from an earlier group must not reach it. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_ibs_composite( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ibs_sref_rows( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ibs_degenerate();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ibs_scale( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ibs_lookback( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ibs_aliasing( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return test_ibs_range( history );
}

/**** Local functions definitions.     ****/

/* (1) Bitwise against the two-BOP composition, from several starts. */
static ErrorNumber test_ibs_composite( const TA_History *history )
{
   static double got[IBS_MAX_BARS], a[IBS_MAX_BARS], b[IBS_MAX_BARS];
   static const int startGrid[] = { 0, 1, 7, 100, 251 };
   TA_Integer begIdx, nbElement, bA, nA, bB, nB;
   TA_RetCode rc;
   int nb = (int)history->nbBars;
   int s, i;

   for( s = 0; s < (int)(sizeof(startGrid)/sizeof(startGrid[0])); s++ )
   {
      int startIdx = startGrid[s];

      if( startIdx > nb-1 )
         continue;

      rc = TA_IBS( startIdx, nb-1, history->high, history->low, history->close,
                   &begIdx, &nbElement, got );
      if( rc != TA_SUCCESS || begIdx != startIdx || nbElement != nb-startIdx )
      {
         printf( "Fail: TA_IBS start %d: rc=%d shape (%d,%d), expected (%d,%d)\n",
                 startIdx, (int)rc, (int)begIdx, (int)nbElement,
                 startIdx, nb-startIdx );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* The low is passed as the open in both calls, and the high as the close
       * in the second: that is what makes the second one H-L over itself. */
      if( TA_BOP( startIdx, nb-1, history->low, history->high, history->low,
                  history->close, &bA, &nA, a ) != TA_SUCCESS
          || TA_BOP( startIdx, nb-1, history->low, history->high, history->low,
                     history->high, &bB, &nB, b ) != TA_SUCCESS
          || bA != begIdx || nA != nbElement || bB != begIdx || nB != nbElement )
      {
         printf( "Fail: TA_IBS start %d: the BOP composition did not run\n",
                 startIdx );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         double want = a[i] + (0.5 - 0.5*b[i]);

         if( memcmp( &got[i], &want, sizeof(double) ) != 0 )
         {
            printf( "Fail: TA_IBS start %d bar %d: %.17g, composition %.17g "
                    "(must be BIT-exact)\n", startIdx, startIdx+i, got[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) Frozen rows, zero tolerance. */
static ErrorNumber test_ibs_sref_rows( const TA_History *history )
{
   static double out[IBS_MAX_BARS];
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int k;

   if( TA_IBS( 0, nb-1, history->high, history->low, history->close,
               &begIdx, &nbElement, out ) != TA_SUCCESS || begIdx != 0 )
   {
      printf( "Fail: TA_IBS reference rows: the call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( k = 0; k < (int)(sizeof(ibsSrefRow)/sizeof(ibsSrefRow[0])); k++ )
   {
      const IbsRow *r = &ibsSrefRow[k];
      unsigned long long bits;

      if( r->bar >= nbElement )
         continue;

      memcpy( &bits, &out[r->bar], sizeof(bits) );
      if( bits != r->bits )
      {
         double want;

         memcpy( &want, &r->bits, sizeof(want) );
         printf( "Fail: TA_IBS reference row bar %d: %.17g want %.17g "
                 "(bits %016llx want %016llx)\n", r->bar, out[r->bar], want,
                 (unsigned long long)bits, (unsigned long long)r->bits );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) The bars the corpus does not contain.
 *
 * Every value here is the guard's, not the ratio's, except the two one-ulp
 * rows, which are there because the midpoint spelling answers 0.5 and 1.5 on
 * them. The NaN row separates this guard from `range > 0.0 ? ratio : 0.5`,
 * which answers a finite 0.5 for a NaN high.
 */
static ErrorNumber test_ibs_degenerate( void )
{
   static double high[NB_IBS_HAND], low[NB_IBS_HAND], close[NB_IBS_HAND];
   static double out[NB_IBS_HAND], fill[NB_IBS_HAND];
   TA_IBS_Stream *stream = NULL;
   TA_Integer begIdx, nbElement, sBeg, sNb;
   TA_RetCode rc;
   double got, peek;
   int i;

   for( i = 0; i < NB_IBS_HAND; i++ )
   {
      high[i]  = ibsHand[i].wantNaN ? TA_REAL_MIN * 0.0 / 0.0 : ibsHand[i].high;
      low[i]   = ibsHand[i].low;
      close[i] = ibsHand[i].close;
   }

   rc = TA_IBS( 0, NB_IBS_HAND-1, high, low, close, &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS || begIdx != 0 || nbElement != NB_IBS_HAND )
   {
      printf( "Fail: TA_IBS degenerate: rc=%d shape (%d,%d)\n",
              (int)rc, (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < NB_IBS_HAND; i++ )
   {
      if( ibsHand[i].wantNaN )
      {
         if( !isnan( out[i] ) )
         {
            printf( "Fail: TA_IBS degenerate row %d (NaN high): %.17g, expected "
                    "NaN -- a guard spelled the other way round answers 0.5 "
                    "here and hides the bad input\n", i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         continue;
      }
      if( memcmp( &out[i], &ibsHand[i].want, sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_IBS degenerate row %d (H %.17g L %.17g C %.17g): "
                 "%.17g want %.17g\n", i, ibsHand[i].high, ibsHand[i].low,
                 ibsHand[i].close, out[i], ibsHand[i].want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* The state is empty, so the stream must reproduce the batch exactly, on
    * these bars as much as on ordinary ones. */
   rc = TA_IBS_OpenAndFill( &stream, high, low, close, NB_IBS_HAND,
                            &sBeg, &sNb, fill );
   if( rc != TA_SUCCESS || sBeg != 0 || sNb != NB_IBS_HAND )
   {
      printf( "Fail: TA_IBS_OpenAndFill degenerate: rc=%d shape (%d,%d)\n",
              (int)rc, (int)sBeg, (int)sNb );
      if( stream ) TA_IBS_Close( stream );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   TA_IBS_Close( stream );
   for( i = 0; i < NB_IBS_HAND; i++ )
   {
      if( ibsHand[i].wantNaN ? !isnan( fill[i] )
                             : memcmp( &fill[i], &out[i], sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_IBS_OpenAndFill degenerate row %d: %.17g vs batch "
                 "%.17g\n", i, fill[i], out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* Opened on the first bar only, so every degenerate bar goes through Peek
    * and Update rather than the fill loop. */
   stream = NULL;
   rc = TA_IBS_Open( &stream, high, low, close, 1, &got );
   if( rc != TA_SUCCESS || memcmp( &got, &out[0], sizeof(double) ) != 0 )
   {
      printf( "Fail: TA_IBS_Open degenerate: rc=%d %.17g vs %.17g\n",
              (int)rc, got, out[0] );
      if( stream ) TA_IBS_Close( stream );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 1; i < NB_IBS_HAND; i++ )
   {
      TA_RetCode rcPeek, rcUpdate;

      rcPeek   = TA_IBS_Peek( stream, high[i], low[i], close[i], &peek );
      rcUpdate = TA_IBS_Update( stream, high[i], low[i], close[i], &got );

      /* A non-finite bar is where the two tiers part, and the split is the
       * framework's, not this function's: every generated Update and Peek
       * opens with a TA_IS_FINITE test on its inputs and answers
       * TA_BAD_PARAM. The batch entry point and OpenAndFill have no such test
       * and let the NaN through, which is what the batch rows above pin. Both
       * halves are asserted here so that a later change to either is a test
       * failure rather than a silent divergence between the two tiers. */
      if( ibsHand[i].wantNaN )
      {
         if( rcPeek != TA_BAD_PARAM || rcUpdate != TA_BAD_PARAM )
         {
            printf( "Fail: TA_IBS stream row %d (NaN high): peek rc=%d update "
                    "rc=%d, expected TA_BAD_PARAM from both\n",
                    i, (int)rcPeek, (int)rcUpdate );
            TA_IBS_Close( stream );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         continue;
      }

      if( rcPeek != TA_SUCCESS || rcUpdate != TA_SUCCESS )
      {
         printf( "Fail: TA_IBS stream degenerate row %d: peek rc=%d update "
                 "rc=%d\n", i, (int)rcPeek, (int)rcUpdate );
         TA_IBS_Close( stream );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( memcmp( &peek, &got, sizeof(double) ) != 0
          || memcmp( &got, &out[i], sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_IBS stream degenerate row %d: peek %.17g update "
                 "%.17g batch %.17g\n", i, peek, got, out[i] );
         TA_IBS_Close( stream );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   TA_IBS_Close( stream );

   /* These bars are the ones the systematic sweep never sends, so this is the
    * call worth routing across the language servers. */
   if( server_verify_active() )
      return server_verify( "IBS", 0, NB_IBS_HAND-1, (unsigned int)NB_IBS_HAND,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ high, low, close, NULL },
                            NULL, 0,
                            (const TA_Real*[]){ out, NULL }, NULL );

   return TA_TEST_PASS;
}

/* (4) The same bits at any price scale.
 *
 * A power of two scales every input exactly, and the ratio of two exactly
 * scaled differences is the ratio itself. A fixed tolerance band does not
 * survive this: at 2^-50 a 1e-14 band sends nearly the whole corpus to 0.5.
 */
static ErrorNumber test_ibs_scale( const TA_History *history )
{
   static double base[IBS_MAX_BARS];
   static double h[IBS_MAX_BARS], l[IBS_MAX_BARS], c[IBS_MAX_BARS];
   static double out[IBS_MAX_BARS];
   static const int kGrid[] = { -900, -50, 40, 900 };
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int k, i;

   if( TA_IBS( 0, nb-1, history->high, history->low, history->close,
               &begIdx, &nbElement, base ) != TA_SUCCESS )
   {
      printf( "Fail: TA_IBS scale: the unscaled call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( k = 0; k < (int)(sizeof(kGrid)/sizeof(kGrid[0])); k++ )
   {
      for( i = 0; i < nb; i++ )
      {
         h[i] = ldexp( history->high[i],  kGrid[k] );
         l[i] = ldexp( history->low[i],   kGrid[k] );
         c[i] = ldexp( history->close[i], kGrid[k] );
      }

      if( TA_IBS( 0, nb-1, h, l, c, &begIdx, &nbElement, out ) != TA_SUCCESS )
      {
         printf( "Fail: TA_IBS scale 2^%d: the call failed\n", kGrid[k] );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < nbElement; i++ )
      {
         if( memcmp( &out[i], &base[i], sizeof(double) ) != 0 )
         {
            printf( "Fail: TA_IBS scale 2^%d bar %d: %.17g, unscaled %.17g\n",
                    kGrid[k], i, out[i], base[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5a) Nothing to warm up: the first output is the first bar asked for. */
static ErrorNumber test_ibs_lookback( const TA_History *history )
{
   static double out[IBS_MAX_BARS];
   TA_Integer begIdx, nbElement;
   int nb = (int)history->nbBars;
   int startIdx;

   if( TA_IBS_Lookback() != 0 )
   {
      printf( "Fail: TA_IBS_Lookback() = %d, expected 0\n", TA_IBS_Lookback() );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   for( startIdx = 0; startIdx < nb; startIdx++ )
   {
      if( TA_IBS( startIdx, nb-1, history->high, history->low, history->close,
                  &begIdx, &nbElement, out ) != TA_SUCCESS
          || begIdx != startIdx || nbElement != nb-startIdx )
      {
         printf( "Fail: TA_IBS start %d: shape (%d,%d), expected (%d,%d)\n",
                 startIdx, (int)begIdx, (int)nbElement, startIdx, nb-startIdx );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }

   return TA_TEST_PASS;
}

/* (5b) outReal over each input in turn. Bar i is read before it is written,
 * and the output index never runs ahead of the input index.
 */
static ErrorNumber test_ibs_aliasing( const TA_History *history )
{
   static double ref[IBS_MAX_BARS], work[IBS_MAX_BARS];
   const TA_Real *src[3];
   const char *name[3] = { "inHigh", "inLow", "inClose" };
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int nb = (int)history->nbBars;
   int which, i;

   if( TA_IBS( 0, nb-1, history->high, history->low, history->close,
               &begIdx, &nbElement, ref ) != TA_SUCCESS )
   {
      printf( "Fail: TA_IBS aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   src[0] = history->high;
   src[1] = history->low;
   src[2] = history->close;

   for( which = 0; which < 3; which++ )
   {
      const TA_Real *in[3];

      for( i = 0; i < nb; i++ )
         work[i] = src[which][i];
      for( i = 0; i < 3; i++ )
         in[i] = ( i == which ) ? work : src[i];

      if( TA_IBS( 0, nb-1, in[0], in[1], in[2], &begIdx2, &nbElement2, work )
             != TA_SUCCESS
          || begIdx2 != begIdx || nbElement2 != nbElement
          || memcmp( ref, work, (size_t)nbElement*sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_IBS aliasing over %s\n", name[which] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5c) The generic start/end sweep. */
static TA_RetCode ibsRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_IBS_Lookback();
   return TA_IBS( startIdx, endIdx, h->high, h->low, h->close,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_ibs_range( const TA_History *history )
{
   return doRangeTestEx( ibsRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)history, 1, 0 );
}
