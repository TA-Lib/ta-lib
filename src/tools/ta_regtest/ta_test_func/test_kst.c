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
 *  092926 MF,CC  First version (#472).
 */

/* Test TA_KST (Pring's Know Sure Thing).
 *
 *   1. COMPOSITE, bitwise: four TA_ROC into four TA_SMA, the weighted sum
 *      left to right as fused multiply-adds, then TA_SMA for the signal,
 *      each SMA called at the start its consumer needs. Proves the fusion and the anchoring, not
 *      the formula: it is blind to a global scale.
 *   2. GOLDENS: rows the ta-lib-oracles lean_serve TA_KST arm agrees with,
 *      the formula leg. |KST| > 10 on most of them, so a x100, /4, /10 or
 *      /120 scale misses by far more than the tolerance.
 *   3. LOOKBACK, FLAT and ZERO-PRICE input, ALIASING.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

#define KST_MAX_BARS 1200
#define KST_NB_OPT   9

/* ROC1..4, SMA1..4, Signal: the C argument order. */
typedef struct { int p[KST_NB_OPT]; } KstParams;

static const KstParams kstDefaults = { { 10, 15, 20, 30, 10, 10, 10, 15, 9 } };

static const KstParams kstGrid[] =
{
   { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } },  /* defaults */
   { { 10, 15, 20, 30, 10, 10, 10, 15,  1 } },  /* S = 1: signal copies the line */
   { { 10, 15, 20, 30, 10, 10, 10, 15, 10 } },  /* Pring's 10-day signal */
   { { 10, 15, 20, 30, 10, 10, 10, 15, 50 } },
   { { 30, 20, 15, 10, 15, 10, 10, 10,  9 } },  /* legs reversed */
   { {  1,  1,  1,  1,  1,  1,  1,  1,  1 } },
   { {  1, 15, 20, 30,  1, 10, 10, 15,  9 } },  /* one leg unsmoothed */
   { { 10, 13, 15, 20, 10, 13, 15, 20,  9 } },  /* intermediate set */
   { {  9, 12, 18, 24,  6,  6,  6,  9,  9 } },  /* long-term set */
   { {  2,  3,  4, 200,  2,  2,  2,  5,  3 } }, /* long lag: output near the tail */
   /* Each period alone at 1. */
   { {  1, 15, 20, 30, 10, 10, 10, 15,  9 } },
   { { 10,  1, 20, 30, 10, 10, 10, 15,  9 } },
   { { 10, 15,  1, 30, 10, 10, 10, 15,  9 } },
   { { 10, 15, 20,  1, 10, 10, 10, 15,  9 } },
   { { 10, 15, 20, 30,  1, 10, 10, 15,  9 } },
   { { 10, 15, 20, 30, 10,  1, 10, 15,  9 } },
   { { 10, 15, 20, 30, 10, 10,  1, 15,  9 } },
   { { 10, 15, 20, 30, 10, 10, 10,  1,  9 } },
};
#define NB_KST_GRID (sizeof(kstGrid)/sizeof(kstGrid[0]))

static const int kstStartGrid[] = { 0, 1, 44, 51, 52, 53, 100, 251 };
#define NB_KST_START (sizeof(kstStartGrid)/sizeof(kstStartGrid[0]))

static ErrorNumber test_kst_composite( const TA_History *history );
static ErrorNumber test_kst_goldens  ( const TA_History *history );
static ErrorNumber test_kst_lookback ( void );
static ErrorNumber test_kst_flat_zero( void );
static ErrorNumber test_kst_aliasing ( const TA_History *history );

ErrorNumber test_func_kst( TA_History *history )
{
   ErrorNumber e;

   if( history->nbBars > KST_MAX_BARS )
   {
      printf( "Fail: KST test expects at most %d bars, got %d\n",
              KST_MAX_BARS, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   if( (e = test_kst_composite( history )) != TA_TEST_PASS ) return e;
   if( (e = test_kst_goldens( history ))   != TA_TEST_PASS ) return e;
   if( (e = test_kst_lookback())           != TA_TEST_PASS ) return e;
   if( (e = test_kst_flat_zero())          != TA_TEST_PASS ) return e;
   if( (e = test_kst_aliasing( history ))  != TA_TEST_PASS ) return e;

   return TA_TEST_PASS;
}

static TA_RetCode kst_call( int startIdx, int endIdx, const double *in,
                            const KstParams *k, TA_Integer *beg, TA_Integer *nb,
                            double *outK, double *outS )
{
   const int *p = k->p;
   return TA_KST( startIdx, endIdx, in, p[0], p[1], p[2], p[3],
                  p[4], p[5], p[6], p[7], p[8], beg, nb, outK, outS );
}

static int kst_lookback( const KstParams *k )
{
   const int *p = k->p;
   return TA_KST_Lookback( p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8] );
}

/* The composition, from shipped primitives only. Returns 0 on a failed call. */
static int kst_reference( const double *in, int nb, int startIdx, const KstParams *k,
                          int *refBeg, int *refNb, double *refK, double *refS )
{
   static double roc[4][KST_MAX_BARS], rcma[4][KST_MAX_BARS], line[KST_MAX_BARS];
   TA_Integer b, n, bLeg[4], nLeg[4];
   int leg, i, s, sigStart, lineNb;
   const int *p = k->p;

   s = kst_lookback( k );
   if( startIdx > s ) s = startIdx;
   sigStart = s - (p[8] - 1);
   if( s > nb - 1 )
   {
      *refBeg = s;
      *refNb  = 0;
      return 1;
   }

   for( leg = 0; leg < 4; leg++ )
   {
      int x = p[leg], a = p[4 + leg];
      if( TA_ROC( 0, nb-1, in, x, &b, &n, roc[leg] ) != TA_SUCCESS || b != x )
         return 0;
      /* roc[leg][j] is bar j + x: its SMA's first output lands on sigStart. */
      if( TA_SMA( sigStart - x, (int)n - 1, roc[leg], a, &bLeg[leg], &nLeg[leg],
                  rcma[leg] ) != TA_SUCCESS || bLeg[leg] != sigStart - x )
         return 0;
   }

   /* The generator fuses each multiply-add of the weighted sum, left to
    * right; the composition must too, or it diverges in the last bit. */
   lineNb = nb - sigStart;
   for( i = 0; i < lineNb; i++ )
      line[i] = fma( 4.0, rcma[3][i], fma( 3.0, rcma[2][i], fma( 2.0, rcma[1][i], rcma[0][i] ) ) );

   if( TA_SMA( p[8] - 1, lineNb - 1, line, p[8], &b, &n, refS ) != TA_SUCCESS
       || b != p[8] - 1 )
      return 0;
   for( i = 0; i < (int)n; i++ )
      refK[i] = line[p[8] - 1 + i];

   *refBeg = s;
   *refNb  = (int)n;
   return 1;
}

static void kst_print_params( const KstParams *k )
{
   printf( "(%d,%d,%d,%d / %d,%d,%d,%d / %d)", k->p[0], k->p[1], k->p[2], k->p[3],
           k->p[4], k->p[5], k->p[6], k->p[7], k->p[8] );
}

/* Bitwise against the reference; counts the values compared into *nbChecked. */
static ErrorNumber kst_compare_composite( const char *what, const double *in, int nb,
                                          int startIdx, const KstParams *k,
                                          int *nbChecked )
{
   static double outK[KST_MAX_BARS], outS[KST_MAX_BARS];
   static double refK[KST_MAX_BARS], refS[KST_MAX_BARS];
   TA_Integer beg, n;
   int refBeg, refNb, i;

   if( kst_call( startIdx, nb-1, in, k, &beg, &n, outK, outS ) != TA_SUCCESS ||
       !kst_reference( in, nb, startIdx, k, &refBeg, &refNb, refK, refS ) )
   {
      printf( "Fail: KST composite %s start %d ", what, startIdx );
      kst_print_params( k );
      printf( ": a call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( (int)n != refNb || (refNb > 0 && (int)beg != refBeg) )
   {
      printf( "Fail: KST composite %s start %d ", what, startIdx );
      kst_print_params( k );
      printf( ": range (%d,%d), composed (%d,%d)\n", (int)beg, (int)n, refBeg, refNb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < (int)n; i++ )
   {
      if( memcmp( &outK[i], &refK[i], sizeof(double) ) != 0 ||
          memcmp( &outS[i], &refS[i], sizeof(double) ) != 0 )
      {
         printf( "Fail: KST composite %s start %d ", what, startIdx );
         kst_print_params( k );
         printf( " bar %d: fused (%.17g, %.17g) != composed (%.17g, %.17g)\n",
                 (int)beg + i, outK[i], outS[i], refK[i], refS[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   *nbChecked += 2 * (int)n;
   return TA_TEST_PASS;
}

/* (1) */
static ErrorNumber test_kst_composite( const TA_History *history )
{
   unsigned int g, s;
   int nbChecked = 0;
   ErrorNumber e;

   for( s = 0; s < NB_KST_START; s++ )
      for( g = 0; g < NB_KST_GRID; g++ )
      {
         e = kst_compare_composite( "corpus", history->close, (int)history->nbBars,
                                    kstStartGrid[s], &kstGrid[g], &nbChecked );
         if( e != TA_TEST_PASS )
            return e;
      }

   if( nbChecked < 20000 )
   {
      printf( "Fail: KST composite compared only %d values\n", nbChecked );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (2) Defaults, startIdx 0, outBegIdx 52 (issue #472). The ta-lib-oracles
 * lean_serve TA_KST arm (QuantConnect LEAN KnowSureThing at 5b0c9975d318,
 * arm commit d980f73, System.Decimal) is within 2.3e-14 relative of every
 * row and 1.4e-13 absolute of the near-zero ones; talipp 2.7.0,
 * trading-signals 8.3.0, ta4j 0.22.6 and pandas-ta-classic 0.6.52 (/100)
 * agree to 4.4e-13 absolute over the whole series.
 */
typedef struct { int bar; double kst, sig; } KstRow;

static const KstRow kstSrefRows[] = {
   {  52,   12.617953885816052,   -4.912483480427277  },
   { 100,  212.50126664937474,   238.32543888562304   },
   { 125,   94.69437542859484,    80.47394775021051   },
   { 147,   -0.026599423720778503, 56.600837985711124 },
   { 200, -115.60022401400767,   -79.36922879966707   },
   { 251,   72.73572624500805,   102.53617000123519   },
};

/* c[i] = 100 + 10 sin(i/7) + 0.15 i + 2 sin(i/2.3), 1200 bars. */
static const KstRow kstSynthRows[] = {
   {   52,   90.22729714889196,     19.37172545635623   },
   {   60,  155.2548388125425,     130.37805044030702   },
   {  100,  126.46621129558608,     79.0202034088191    },
   {  250,   -6.701535280977936,    41.84464326132026   },
   {  500,   97.74533602392901,     80.2667008888041    },
   {  619,   -0.0020065820173034155, -25.943570514699648 },
   {  800,   43.09843836915679,      8.970305977183077  },
   { 1000,  -20.091989177139745,     6.189223833010446  },
   { 1199,   46.590336314877064,    26.86125737591456   },
};

/* Absorbs re-association or re-anchoring of the sums (measured 7.1e-15
 * absolute between the two anchorings) with margin. The synthetic series
 * also absorbs a libm sin() one ulp off the capture's, which moves an output
 * by up to ~1e-13 absolute. */
#define KST_SREF_ABS  1e-13
#define KST_SREF_REL  1e-14
#define KST_SYNTH_ABS 1e-12
#define KST_SYNTH_REL 1e-12

static ErrorNumber kst_check_rows( const char *what, const double *in, int nb,
                                   const KstRow *rows, int nbRows,
                                   double tolAbs, double tolRel )
{
   static double outK[KST_MAX_BARS], outS[KST_MAX_BARS];
   TA_Integer beg, n;
   TA_RetCode rc;
   int r;

   rc = kst_call( 0, nb-1, in, &kstDefaults, &beg, &n, outK, outS );
   if( rc != TA_SUCCESS || beg != 52 || n != nb - 52 )
   {
      printf( "Fail: KST goldens %s: rc %d range (%d,%d), expected (52,%d)\n",
              what, (int)rc, (int)beg, (int)n, nb - 52 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( r = 0; r < nbRows; r++ )
   {
      int i = rows[r].bar - (int)beg;
      if( fabs( outK[i] - rows[r].kst ) > tolAbs + tolRel*fabs( rows[r].kst ) ||
          fabs( outS[i] - rows[r].sig ) > tolAbs + tolRel*fabs( rows[r].sig ) )
      {
         printf( "Fail: KST golden %s bar %d: (%.17g, %.17g) want (%.17g, %.17g)\n",
                 what, rows[r].bar, outK[i], outS[i], rows[r].kst, rows[r].sig );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   if( server_verify_active() )
   {
      double opt[KST_NB_OPT];
      for( r = 0; r < KST_NB_OPT; r++ )
         opt[r] = (double)kstDefaults.p[r];
      return server_verify( "KST", 0, nb-1, nb, rc, beg, n,
                            (const TA_Real*[]){ in, NULL }, opt, KST_NB_OPT,
                            (const TA_Real*[]){ outK, outS, NULL }, NULL );
   }
   return TA_TEST_PASS;
}

static ErrorNumber test_kst_goldens( const TA_History *history )
{
   static double synth[KST_MAX_BARS];
   ErrorNumber e;
   int i;

   e = kst_check_rows( "corpus", history->close, (int)history->nbBars, kstSrefRows,
                       (int)(sizeof(kstSrefRows)/sizeof(kstSrefRows[0])),
                       KST_SREF_ABS, KST_SREF_REL );
   if( e != TA_TEST_PASS )
      return e;

   for( i = 0; i < KST_MAX_BARS; i++ )
      synth[i] = 100.0 + 10.0*sin( i/7.0 ) + 0.15*i + 2.0*sin( i/2.3 );
   return kst_check_rows( "synthetic", synth, KST_MAX_BARS, kstSynthRows,
                          (int)(sizeof(kstSynthRows)/sizeof(kstSynthRows[0])),
                          KST_SYNTH_ABS, KST_SYNTH_REL );
}

/* (3a) A wrong-by-one anchor is invisible to a value comparison that shares it. */
static ErrorNumber test_kst_lookback( void )
{
   static const struct { KstParams k; int want; } cases[] = {
      { { { 10, 15, 20, 30, 10, 10, 10, 15,  9 } }, 52 },
      { { { 10, 15, 20, 30, 10, 10, 10, 15, 10 } }, 53 },
      { { { 10, 15, 20, 30, 10, 10, 10, 15, 50 } }, 93 },
      { { { 30, 20, 15, 10, 15, 10, 10, 10,  9 } }, 52 },
      { { {  1,  1,  1,  1,  1,  1,  1,  1,  1 } },  1 },
      /* Each leg the unique max in turn, its SMA period unlike every other. */
      { { { 50,  1,  1,  1, 40,  2,  3,  4,  1 } }, 89 },
      { { {  1, 50,  1,  1,  2, 40,  3,  4,  1 } }, 89 },
      { { {  1,  1, 50,  1,  2,  3, 40,  4,  1 } }, 89 },
      { { {  1,  1,  1, 50,  2,  3,  4, 40,  1 } }, 89 },
   };
   unsigned int c;

   for( c = 0; c < sizeof(cases)/sizeof(cases[0]); c++ )
   {
      int got = kst_lookback( &cases[c].k );
      if( got != cases[c].want )
      {
         printf( "Fail: TA_KST_Lookback " );
         kst_print_params( &cases[c].k );
         printf( " = %d, want %d\n", got, cases[c].want );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }
   return TA_TEST_PASS;
}

/* (3b) Flat input is exactly 0.0 on both outputs. A zero price follows
 * TA_ROC's guard: finite, and bitwise the composition, which carries it. */
static ErrorNumber test_kst_flat_zero( void )
{
   static double in[300], outK[300], outS[300];
   TA_Integer beg, n;
   TA_RetCode rc;
   int i, nbChecked = 0;
   ErrorNumber e;

   for( i = 0; i < 300; i++ )
      in[i] = 42.25;
   rc = kst_call( 0, 299, in, &kstDefaults, &beg, &n, outK, outS );
   if( rc != TA_SUCCESS || n <= 0 )
   {
      printf( "Fail: KST flat: rc %d nb %d\n", (int)rc, (int)n );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < (int)n; i++ )
   {
      if( outK[i] != 0.0 || outS[i] != 0.0 )
      {
         printf( "Fail: KST flat bar %d: (%.17g, %.17g) != 0.0\n",
                 (int)beg + i, outK[i], outS[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   for( i = 0; i < 300; i++ )
      in[i] = 100.0 + (double)(i % 13) * 0.37 - (double)(i % 5) * 0.11;
   in[100] = 0.0;
   rc = kst_call( 0, 299, in, &kstDefaults, &beg, &n, outK, outS );
   if( rc != TA_SUCCESS || n <= 0 )
   {
      printf( "Fail: KST zero price: rc %d nb %d\n", (int)rc, (int)n );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < (int)n; i++ )
   {
      if( !isfinite( outK[i] ) || !isfinite( outS[i] ) )
      {
         printf( "Fail: KST zero price bar %d: (%.17g, %.17g) not finite\n",
                 (int)beg + i, outK[i], outS[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   for( i = 0; i < (int)(sizeof(kstGrid)/sizeof(kstGrid[0])); i++ )
   {
      e = kst_compare_composite( "zero price", in, 300, 0, &kstGrid[i], &nbChecked );
      if( e != TA_TEST_PASS )
         return e;
   }
   /* Negative values: the guard is TA_ROC's != 0.0, not a sign test. */
   for( i = 0; i < 300; i++ )
      in[i] = (double)(i % 11) * 0.73 - 3.1 - (double)(i % 4) * 0.29;
   for( i = 0; i < (int)(sizeof(kstGrid)/sizeof(kstGrid[0])); i++ )
   {
      e = kst_compare_composite( "negative", in, 300, 0, &kstGrid[i], &nbChecked );
      if( e != TA_TEST_PASS )
         return e;
   }
   in[100] = 0.0;

   if( nbChecked == 0 )
   {
      printf( "Fail: KST zero price compared nothing\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( server_verify_active() )
   {
      double opt[KST_NB_OPT];
      for( i = 0; i < KST_NB_OPT; i++ )
         opt[i] = (double)kstGrid[4].p[i];   /* legs reversed: nine off defaults */
      rc = kst_call( 0, 299, in, &kstGrid[4], &beg, &n, outK, outS );
      return server_verify( "KST", 0, 299, 300, rc, beg, n,
                            (const TA_Real*[]){ in, NULL }, opt, KST_NB_OPT,
                            (const TA_Real*[]){ outK, outS, NULL }, NULL );
   }
   return TA_TEST_PASS;
}

/* (3c) Either output may be the input buffer. */
static ErrorNumber test_kst_aliasing( const TA_History *history )
{
   static double buf[KST_MAX_BARS], outK[KST_MAX_BARS], outS[KST_MAX_BARS];
   static double refK[KST_MAX_BARS], refS[KST_MAX_BARS];
   int nb = (int)history->nbBars, which, g, i;
   TA_Integer beg, n, rBeg, rN;

   for( g = 0; g < (int)NB_KST_GRID; g++ )
   {
      if( kst_call( 0, nb-1, history->close, &kstGrid[g], &rBeg, &rN, refK, refS )
          != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      for( which = 0; which < 2; which++ )
      {
         memcpy( buf, history->close, sizeof(double) * nb );
         if( kst_call( 0, nb-1, buf, &kstGrid[g], &beg, &n,
                       which == 0 ? buf : outK, which == 1 ? buf : outS ) != TA_SUCCESS
             || beg != rBeg || n != rN )
         {
            printf( "Fail: KST aliasing %s == inReal: call or range\n",
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
               printf( "Fail: KST aliasing %s == inReal ",
                       which == 0 ? "outKST" : "outKSTSignal" );
               kst_print_params( &kstGrid[g] );
               printf( " bar %d differs\n", (int)beg + i );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}
