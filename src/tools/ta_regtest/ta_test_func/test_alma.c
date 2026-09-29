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
 *  092926 MF,CC  First version (issue #475).
 */

/* Description:
 *
 *   Test TA_ALMA (Arnaud Legoux Moving Average).
 *
 *   Legs:
 *     1. Frozen goldens on the 252-bar reference close: a 60-digit reference
 *        on the exact binary inputs, with m floored in binary64.
 *     2. External golden: Tulip Indicators 0.9.2 (be18abb) tests/extra.txt
 *        "alma 9 0.85 6", 72 SPY closes and 64 values at 7 decimals. They are
 *        LEAN's spy_alma.txt values, from the LEAN implementer's spreadsheet.
 *     3. Tulip's beta/alma.c expressions, transcribed below, bit for bit over
 *        periods, sigmas, offsets and start indexes, in place and not.
 *     4. Period 1 is the input bit for bit, signed zeros included.
 *     5. Range corners: small periods stay inside each window's range, up to
 *        rounding, and period 100000 inside the input's range. At sigma
 *        TA_REAL_MAX every weight but the peak's underflows to 0, so the output
 *        is the peak bar exactly (the newest at offset 1, the oldest at 0).
 *        Out-of-range parameters are rejected; the lookback ignores sigma and
 *        offset.
 *     6. The startIdx/endIdx range sweep.
 *     7. The stream tier (Open, then Peek and Update per bar) against the
 *        batch, bit for bit, period 1 and its signed zeros included.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define ALMA_GOLD_REL  1e-14
#define ALMA_SPY_NB    72
#define ALMA_SPY_ABS   5.1e-8
#define ALMA_SYN_NB    1200
#define ALMA_BIG_N     100000

#define ALMA_TULIP_CMP   6844176
#define ALMA_PERIOD1_CMP 8
#define ALMA_STREAM_CMP  22736
#define ALMA_CORNER_CMP  7194

typedef struct { int n; double sigma, offset; int bar; double want; } AlmaGolden;

/* The 51/6/0.58 rows pin the binary64 floor: 0.58*50 is 28.999999999999996,
 * so m is 28. Flooring the decimal product gives 29 and moves them by 1e-3. */
static const AlmaGolden almaGolden[] =
{
   {  9, 6.0, 0.85,   8,  93.049324397804160 },
   {  9, 6.0, 0.85,   9,  92.400070475501106 },
   {  9, 6.0, 0.85,  50,  90.257029267088856 },
   {  9, 6.0, 0.85, 251, 109.06030240999498  },
   { 21, 6.0, 0.85,  20,  91.044563805532251 },
   { 21, 6.0, 0.85, 251, 109.04462188666197  },
   { 10, 3.0, 0.5,    9,  93.544299039850312 },
   { 10, 3.0, 0.5,  251, 109.15588767703132  },
   { 30, 6.0, 1.0,   29,  85.710949448171946 },
   { 30, 10.0, 0.0, 251, 103.70854631713051  },
   { 51, 6.0, 0.58,  50,  86.951963106025674 },
   { 51, 6.0, 0.58, 251, 105.42604265746272  },
};
#define NB_OF(a) ((int)(sizeof(a)/sizeof((a)[0])))

static const double almaSpyClose[ALMA_SPY_NB] =
{
   151.89, 149, 150.02, 151.91, 151.61, 152.11, 152.92, 154.29,
   154.5, 154.78, 155.44, 156.03, 155.68, 155.9, 156.73, 155.83,
   154.97, 154.61, 155.69, 154.36, 155.6, 154.95, 156.19, 156.19,
   156.67, 156.05, 156.82, 155.23, 155.86, 155.16, 156.21, 156.75,
   158.67, 159.19, 158.8, 155.12, 157.41, 155.11, 154.14, 155.48,
   156.17, 157.78, 157.88, 158.52, 158.24, 159.3, 159.68, 158.28,
   159.75, 161.37, 161.78, 162.6, 163.34, 162.88, 163.41, 163.54,
   165.23, 166.12, 165.34, 166.94, 166.93, 167.17, 165.93, 165.45,
   165.31, 166.3, 165.22, 165.83, 163.45, 164.35, 163.56, 161.27
};

static const double almaSpyAlma[ALMA_SPY_NB-8] =
{
   153.0103125, 153.7129632, 154.3261091, 154.8567064, 155.2769595,
   155.5872882, 155.8547348, 156.0149609, 155.9837527, 155.702711,
   155.4071973, 155.1334738, 155.0737871, 155.0664572, 155.2145257,
   155.4710268, 155.826227, 156.1015857, 156.3111052, 156.2779321,
   156.1359503, 155.8736587, 155.7274243, 155.8114836, 156.3056075,
   157.1093269, 157.9327267, 158.0937167, 157.7905353, 157.0635226,
   156.2478143, 155.6254658, 155.3414542, 155.6297745, 156.3075108,
   157.1057808, 157.7226669, 158.2017231, 158.6251992, 158.8730608,
   159.0659722, 159.3901716, 159.9773159, 160.803581, 161.6660303,
   162.3282367, 162.7933684, 163.0869453, 163.4751077, 164.09043,
   164.7491986, 165.4150535, 165.9539775, 166.4178948, 166.6412687,
   166.5371323, 166.1764273, 165.8875152, 165.7073031, 165.6811555,
   165.4210407, 165.0254396, 164.5209698, 163.8395316
};

static const int    almaGridN[]      = { 1, 2, 3, 7, 8, 9, 10, 15, 16, 17, 21, 51, 101, 200 };
static const double almaGridSigma[]  = { 0.01, 1.0, 6.0, 6.5, 50.0, 3e37 };
static const double almaGridOffset[] = { 0.0, 0.29, 0.5, 0.58, 0.85, 1.0 };
static const int    almaGridStart[]  = { 0, 1, 8, 9, 17, 250, 1191, 1199 };

static int g_almaGoldCmp;
static int g_almaTulipCmp;
static int g_almaPeriod1Cmp;
static int g_almaStreamCmp;
static int g_almaCornerCmp;

static void        almaSynth( double *c );
static ErrorNumber almaRoute( const char *tag, const double *x, int nb, int n,
                              double sigma, double offset,
                              TA_RetCode retCode, int begIdx, int nbElement,
                              const double *out );
static ErrorNumber test_alma_goldens( const TA_History *history );
static ErrorNumber test_alma_spy( void );
static ErrorNumber test_alma_tulip( void );
static ErrorNumber test_alma_period1( void );
static ErrorNumber almaSmallCorners( void );
static ErrorNumber test_alma_corners( void );
static ErrorNumber test_alma_range( const TA_Real *in );
static ErrorNumber test_alma_stream( void );

/**** Global functions definitions. ****/
ErrorNumber test_func_alma( TA_History *history )
{
   ErrorNumber err;

   g_almaGoldCmp = g_almaTulipCmp = g_almaPeriod1Cmp = g_almaStreamCmp = g_almaCornerCmp = 0;

   err = test_alma_goldens( history );
   if( err == TA_TEST_PASS )
      err = test_alma_spy();
   if( err == TA_TEST_PASS )
      err = test_alma_tulip();
   if( err == TA_TEST_PASS )
      err = test_alma_period1();
   if( err == TA_TEST_PASS )
      err = test_alma_corners();
   if( err == TA_TEST_PASS )
      err = test_alma_range( history->close );
   if( err == TA_TEST_PASS )
      err = test_alma_stream();

   /* Literal counts: every input is fixed, so each leg is deterministic. */
   if( err == TA_TEST_PASS
       && ( g_almaGoldCmp != NB_OF(almaGolden) + ALMA_SPY_NB - 8
            || g_almaTulipCmp != ALMA_TULIP_CMP || g_almaPeriod1Cmp != ALMA_PERIOD1_CMP
            || g_almaStreamCmp != ALMA_STREAM_CMP || g_almaCornerCmp != ALMA_CORNER_CMP ) )
   {
      printf( "ALMA Fail: coverage counters (golden %d, tulip %d, period1 %d, stream %d, "
              "corner %d) are not what this file was written with (%d, %d, %d, %d, %d)\n",
              g_almaGoldCmp, g_almaTulipCmp, g_almaPeriod1Cmp, g_almaStreamCmp,
              g_almaCornerCmp, NB_OF(almaGolden) + ALMA_SPY_NB - 8, ALMA_TULIP_CMP,
              ALMA_PERIOD1_CMP, ALMA_STREAM_CMP, ALMA_CORNER_CMP );
      return TA_ALMA_VACUOUS;
   }

   return err;
}

/**** Local functions definitions. ****/
static void almaSynth( double *c )
{
   int i;

   for( i = 0; i < ALMA_SYN_NB; i++ )
      c[i] = 100.0 + 10.0*sin(i/7.0) + 0.15*i + 2.0*sin(i/2.3);
}

/* Replays a whole-series call on the language servers. */
static ErrorNumber almaRoute( const char *tag, const double *x, int nb, int n,
                              double sigma, double offset,
                              TA_RetCode retCode, int begIdx, int nbElement,
                              const double *out )
{
   double optIn[3];
   ErrorNumber e;
   int cmpBefore;

   if( !server_verify_active() )
      return TA_TEST_PASS;

   optIn[0] = (double)n;
   optIn[1] = sigma;
   optIn[2] = offset;
   cmpBefore = server_verify_comparisons();
   e = server_verify( "ALMA", 0, nb-1, nb, retCode, begIdx, nbElement,
                      (const TA_Real*[]){ x, NULL }, optIn, 3,
                      (const TA_Real*[]){ out, NULL }, NULL );
   if( e != TA_TEST_PASS )
      return e;
   if( server_verify_comparisons() == cmpBefore )
   {
      printf( "ALMA %s [%d,%g,%g]: compared no server despite live pipes\n",
              tag, n, sigma, offset );
      return TA_SV_ROUTED_VACUOUS;
   }
   return TA_TEST_PASS;
}

/* (1) */
static ErrorNumber test_alma_goldens( const TA_History *history )
{
   static double out[252];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   const AlmaGolden *g;
   double got, rel;
   int nb, k;
   ErrorNumber e;

   nb = (int)history->nbBars;
   for( k = 0; k < NB_OF(almaGolden); k++ )
   {
      g = &almaGolden[k];
      retCode = TA_ALMA( 0, nb-1, history->close, g->n, g->sigma, g->offset,
                         &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != g->n-1 || nbElement != nb-g->n+1 )
      {
         printf( "ALMA golden Fail [%d,%g,%g]: rc=%d (%d,%d)\n", g->n, g->sigma,
                 g->offset, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      got = out[g->bar - begIdx];
      rel = fabs( got - g->want ) / fabs( g->want );
      if( !(rel <= ALMA_GOLD_REL) )
      {
         printf( "ALMA golden Fail [%d,%g,%g] bar %d: %.17g, want %.17g (rel %.2e)\n",
                 g->n, g->sigma, g->offset, g->bar, got, g->want, rel );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_almaGoldCmp++;

      /* One replay per configuration the sweep does not send: it already sends
       * the defaults on this series. */
      if( (k == 0 || g->n != almaGolden[k-1].n || g->sigma != almaGolden[k-1].sigma
           || g->offset != almaGolden[k-1].offset)
          && !(g->n == 9 && g->sigma == 6.0 && g->offset == 0.85) )
      {
         e = almaRoute( "golden", history->close, nb, g->n, g->sigma, g->offset,
                        retCode, begIdx, nbElement, out );
         if( e != TA_TEST_PASS )
            return e;
      }
   }
   return TA_TEST_PASS;
}

/* (2) */
static ErrorNumber test_alma_spy( void )
{
   double out[ALMA_SPY_NB];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   retCode = TA_ALMA( 0, ALMA_SPY_NB-1, almaSpyClose, 9, 6.0, 0.85,
                      &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 8 || nbElement != ALMA_SPY_NB-8 )
   {
      printf( "ALMA spy Fail: rc=%d (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbElement; i++ )
   {
      if( !(fabs( out[i] - almaSpyAlma[i] ) <= ALMA_SPY_ABS) )
      {
         printf( "ALMA spy Fail bar %d: %.10f, want %.7f\n", i+8, out[i], almaSpyAlma[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_almaGoldCmp++;
   }
   return almaRoute( "spy", almaSpyClose, ALMA_SPY_NB, 9, 6.0, 0.85,
                     retCode, begIdx, nbElement, out );
}

/* Tulip 0.9.2 beta/alma.c, expression for expression, except that its
 * pow(x, 2) is written x*x: glibc returns the same bits, but not every CRT
 * this suite runs under is bound to. */
static void almaTulip( const double *x, int nb, int n, double sigma, double offset,
                       double *out )
{
   double *w;
   double m, s, norm, sum;
   int i, j;

   w = (double *)malloc( sizeof(double) * (size_t)n );
   if( !w )
   {
      memset( out, 0, sizeof(double) * (size_t)(nb - n + 1) );
      return;
   }
   m = floor( offset * (n - 1) );
   s = n / sigma;
   norm = 0;
   for( i = 0; i < n; i++ )
   {
      w[i] = exp( -1 * ((i - m) * (i - m)) / (2 * (s * s)) );
      norm += w[i];
   }
   for( i = 0; i < n; i++ )
      w[i] /= norm;
   for( i = n-1; i < nb; i++ )
   {
      sum = 0;
      for( j = 0; j < n; j++ )
         sum += x[i - n + j + 1] * w[j];
      out[i-(n-1)] = sum;
   }
   free( w );
}

/* (3) Any startIdx gives the tail of the whole-series call, and outReal may
 * be inReal. */
static ErrorNumber test_alma_tulip( void )
{
   static double c[ALMA_SYN_NB], want[ALMA_SYN_NB], out[ALMA_SYN_NB], buf[ALMA_SYN_NB];
   TA_Integer begIdx, nbElement, expBeg;
   TA_RetCode retCode;
   int a, b, d, t, i, n, pass;
   double sigma, offset;

   almaSynth( c );
   for( a = 0; a < NB_OF(almaGridN); a++ )
   for( b = 0; b < NB_OF(almaGridSigma); b++ )
   for( d = 0; d < NB_OF(almaGridOffset); d++ )
   {
      n = almaGridN[a];
      sigma = almaGridSigma[b];
      offset = almaGridOffset[d];
      almaTulip( c, ALMA_SYN_NB, n, sigma, offset, want );

      for( t = 0; t < NB_OF(almaGridStart); t++ )
      for( pass = 0; pass < 2; pass++ )
      {
         expBeg = almaGridStart[t] < n-1 ? n-1 : almaGridStart[t];
         if( pass == 0 )
            retCode = TA_ALMA( almaGridStart[t], ALMA_SYN_NB-1, c, n, sigma, offset,
                               &begIdx, &nbElement, out );
         else
         {
            memcpy( buf, c, sizeof(buf) );
            retCode = TA_ALMA( almaGridStart[t], ALMA_SYN_NB-1, buf, n, sigma, offset,
                               &begIdx, &nbElement, buf );
            memcpy( out, buf, sizeof(buf) );
         }
         if( retCode != TA_SUCCESS || begIdx != expBeg || nbElement != ALMA_SYN_NB-expBeg )
         {
            printf( "ALMA tulip Fail [%d,%g,%g] start %d%s: rc=%d (%d,%d)\n", n, sigma,
                    offset, almaGridStart[t], pass ? " in place" : "",
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nbElement; i++ )
         {
            if( memcmp( &out[i], &want[begIdx-(n-1)+i], sizeof(double) ) != 0 )
            {
               printf( "ALMA tulip Fail [%d,%g,%g] start %d%s bar %d: %.17g, want %.17g\n",
                       n, sigma, offset, almaGridStart[t], pass ? " in place" : "",
                       begIdx+i, out[i], want[begIdx-(n-1)+i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_almaTulipCmp++;
         }
      }
   }
   return TA_TEST_PASS;
}

/* (4) A sum started at 0.0 would turn -0.0 into +0.0. */
static ErrorNumber test_alma_period1( void )
{
   static const double x[] = { -0.0, 0.0, 1.25, -0.0, -3.5, 0.0, 1e300, -1e-310 };
   double out[NB_OF(x)];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   retCode = TA_ALMA( 0, NB_OF(x)-1, x, 1, 6.0, 0.85, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != NB_OF(x) )
   {
      printf( "ALMA period1 Fail: rc=%d (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbElement; i++ )
   {
      if( memcmp( &out[i], &x[i], sizeof(double) ) != 0 )
      {
         printf( "ALMA period1 Fail bar %d: %.17g, want %.17g\n", i, out[i], x[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_almaPeriod1Cmp++;
   }
   return almaRoute( "period1", x, NB_OF(x), 1, 6.0, 0.85,
                     retCode, begIdx, nbElement, out );
}

/* Small periods at the parameter corners, on the synthetic series: every
 * output inside its own window's range up to rounding, routed to the servers
 * since the sweep moves one parameter at a time. At sigma TA_REAL_MAX and
 * offset 1 the kernel is one bar wide, so the output is the newest bar. */
static ErrorNumber almaSmallCorners( void )
{
   static const struct { int n; double sigma, offset; } corner[] =
   {
      { 2, 0.01, 0.0 }, { 2, 3e37, 1.0 }, { 9, 0.01, 1.0 }, { 9, 3e37, 0.0 },
      { 60, 0.01, 0.5 }, { 60, 3e37, 1.0 },
   };
   static double c[ALMA_SYN_NB], out[ALMA_SYN_NB];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   ErrorNumber e;
   double lo, hi;
   int k, i, j;

   almaSynth( c );
   for( k = 0; k < NB_OF(corner); k++ )
   {
      retCode = TA_ALMA( 0, ALMA_SYN_NB-1, c, corner[k].n, corner[k].sigma,
                         corner[k].offset, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != corner[k].n-1
          || nbElement != ALMA_SYN_NB-corner[k].n+1 )
      {
         printf( "ALMA corner Fail [%d,%g,%g]: rc=%d (%d,%d)\n", corner[k].n,
                 corner[k].sigma, corner[k].offset, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         lo = hi = c[begIdx+i];
         for( j = begIdx+i-corner[k].n+1; j <= begIdx+i; j++ )
         {
            if( c[j] < lo ) lo = c[j];
            if( c[j] > hi ) hi = c[j];
         }
         if( !(out[i] >= lo*(1.0-4e-16) && out[i] <= hi*(1.0+4e-16)) )
         {
            printf( "ALMA corner Fail [%d,%g,%g] bar %d: %.17g outside [%.17g,%.17g]\n",
                    corner[k].n, corner[k].sigma, corner[k].offset, begIdx+i, out[i], lo, hi );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( corner[k].sigma == 3e37 && corner[k].offset == 1.0
             && memcmp( &out[i], &c[begIdx+i], sizeof(double) ) != 0 )
         {
            printf( "ALMA corner Fail [%d,%g,%g] bar %d: %.17g, newest bar %.17g\n",
                    corner[k].n, corner[k].sigma, corner[k].offset, begIdx+i, out[i],
                    c[begIdx+i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_almaCornerCmp++;
      }
      e = almaRoute( "corner", c, ALMA_SYN_NB, corner[k].n, corner[k].sigma,
                     corner[k].offset, retCode, begIdx, nbElement, out );
      if( e != TA_TEST_PASS )
         return e;
   }
   return TA_TEST_PASS;
}

/* (5) */
static ErrorNumber test_alma_corners( void )
{
   static const double sig[] = { 0.01, 3e37 };
   static const double off[] = { 0.0, 1.0 };
   static const struct { int n; double sigma, offset; } bad[] =
   {
      { 0, 6.0, 0.85 }, { 100001, 6.0, 0.85 }, { 9, 0.0099, 0.85 }, { 9, -6.0, 0.85 },
      { 9, 6.0, -0.01 }, { 9, 6.0, 1.01 },
   };
   double *x, *out, lo, hi;
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   ErrorNumber e;
   int nb, i, a, b, k;

   nb = ALMA_BIG_N + 64;
   x   = (double *)malloc( sizeof(double) * (size_t)nb );
   out = (double *)malloc( sizeof(double) * (size_t)nb );
   if( !x || !out )
   {
      free( x ); free( out );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   for( i = 0; i < nb; i++ )
      x[i] = 100.0 + 10.0*sin(i/7.0) + 0.15*(i % 1000);

   for( a = 0; a < NB_OF(sig); a++ )
   for( b = 0; b < NB_OF(off); b++ )
   {
      retCode = TA_ALMA( 0, nb-1, x, ALMA_BIG_N, sig[a], off[b], &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != ALMA_BIG_N-1 || nbElement != nb-ALMA_BIG_N+1 )
      {
         printf( "ALMA corner Fail [%d,%g,%g]: rc=%d (%d,%d)\n", ALMA_BIG_N, sig[a],
                 off[b], (int)retCode, begIdx, nbElement );
         free( x ); free( out );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      /* Non-negative weights summing to one keep each output in its window's
       * range; lo/hi bound every window here. */
      lo = 100.0 - 10.0;
      hi = 100.0 + 10.0 + 0.15*999;
      for( i = 0; i < nbElement; i++ )
      {
         if( !(out[i] >= lo && out[i] <= hi) )
         {
            printf( "ALMA corner Fail [%d,%g,%g] bar %d: %.17g\n", ALMA_BIG_N, sig[a],
                    off[b], begIdx+i, out[i] );
            free( x ); free( out );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( sig[a] == 3e37 )
         {
            const double *peak = off[b] == 1.0 ? &x[begIdx+i] : &x[begIdx+i-ALMA_BIG_N+1];
            if( memcmp( &out[i], peak, sizeof(double) ) != 0 )
            {
               printf( "ALMA corner Fail [%d,%g,%g] bar %d: %.17g, peak bar %.17g\n",
                       ALMA_BIG_N, sig[a], off[b], begIdx+i, out[i], *peak );
               free( x ); free( out );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_almaCornerCmp++;
         }
      }
   }
   free( x );
   free( out );

   for( k = 0; k < NB_OF(bad); k++ )
   {
      double y[16] = { 0 };
      double o[16];
      retCode = TA_ALMA( 0, 15, y, bad[k].n, bad[k].sigma, bad[k].offset,
                         &begIdx, &nbElement, o );
      if( retCode != TA_BAD_PARAM )
      {
         printf( "ALMA corner Fail: [%d,%g,%g] returned %d, want TA_BAD_PARAM\n",
                 bad[k].n, bad[k].sigma, bad[k].offset, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }

   e = almaSmallCorners();
   if( e != TA_TEST_PASS )
      return e;

   for( a = 0; a < NB_OF(almaGridN); a++ )
   for( b = 0; b < NB_OF(almaGridSigma); b++ )
   for( k = 0; k < NB_OF(almaGridOffset); k++ )
   {
      if( TA_ALMA_Lookback( almaGridN[a], almaGridSigma[b], almaGridOffset[k] )
          != almaGridN[a]-1 )
      {
         printf( "ALMA lookback Fail [%d,%g,%g]: %d\n", almaGridN[a], almaGridSigma[b],
                 almaGridOffset[k],
                 TA_ALMA_Lookback( almaGridN[a], almaGridSigma[b], almaGridOffset[k] ) );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }
   return TA_TEST_PASS;
}

/* (6) A finite window recomputed from scratch: bit-exact across ranges. */
typedef struct { int n; double sigma, offset; const TA_Real *in; } AlmaRangeParam;

static TA_RetCode almaRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   AlmaRangeParam *p = (AlmaRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_ALMA_Lookback( p->n, p->sigma, p->offset );
   return TA_ALMA( startIdx, endIdx, p->in, p->n, p->sigma, p->offset,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_alma_range( const TA_Real *in )
{
   AlmaRangeParam param;

   param.n      = 9;
   param.sigma  = 6.0;
   param.offset = 0.85;
   param.in     = in;

   return doRangeTestEx( almaRangeTestFunction,
                         TA_STABLE_EXACT, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}

/* (7) */
static ErrorNumber almaStreamOne( const double *x, int nb, int n, double sigma, double offset,
                                  int historyLen )
{
   static double want[ALMA_SYN_NB];
   TA_ALMA_Stream *st = NULL;
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   double got, peeked;
   int t;

   retCode = TA_ALMA( 0, nb-1, x, n, sigma, offset, &begIdx, &nbElement, want );
   if( retCode != TA_SUCCESS || begIdx != n-1 )
   {
      printf( "ALMA stream Fail [%d,%g,%g]: batch rc=%d beg %d\n", n, sigma, offset,
              (int)retCode, begIdx );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   retCode = TA_ALMA_Open( &st, x, historyLen, n, sigma, offset, &got );
   if( retCode != TA_SUCCESS )
   {
      printf( "ALMA stream Fail [%d,%g,%g]: Open(%d) rc=%d\n", n, sigma, offset,
              historyLen, (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   t = historyLen-1;
   for( ;; )
   {
      if( memcmp( &got, &want[t-begIdx], sizeof(double) ) != 0 )
      {
         printf( "ALMA stream Fail [%d,%g,%g] bar %d: %.17g, want %.17g\n", n, sigma,
                 offset, t, got, want[t-begIdx] );
         TA_ALMA_Close( st );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_almaStreamCmp++;
      if( ++t >= nb )
         break;
      retCode = TA_ALMA_Peek( st, x[t], &peeked );
      if( retCode == TA_SUCCESS )
         retCode = TA_ALMA_Update( st, x[t], &got );
      if( retCode != TA_SUCCESS || memcmp( &peeked, &got, sizeof(double) ) != 0 )
      {
         printf( "ALMA stream Fail [%d,%g,%g] bar %d: rc=%d peek %.17g update %.17g\n",
                 n, sigma, offset, t, (int)retCode, peeked, got );
         TA_ALMA_Close( st );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   TA_ALMA_Close( st );
   return TA_TEST_PASS;
}

static ErrorNumber test_alma_stream( void )
{
   static const double z[] = { -0.0, 0.0, 1.25, -0.0, -3.5, 0.0, 1e300, -1e-310 };
   static double c[ALMA_SYN_NB];
   static const int    sn[]   = { 2, 3, 9, 17, 51 };
   static const double ssig[] = { 1.0, 6.0 };
   static const double soff[] = { 0.0, 0.58, 0.85, 1.0 };
   ErrorNumber e;
   int a, b, d, h;

   e = almaStreamOne( z, NB_OF(z), 1, 6.0, 0.85, 1 );
   if( e != TA_TEST_PASS )
      return e;

   almaSynth( c );
   for( a = 0; a < NB_OF(sn); a++ )
   for( b = 0; b < NB_OF(ssig); b++ )
   for( d = 0; d < NB_OF(soff); d++ )
   for( h = sn[a]; h <= sn[a]+1; h++ )
   {
      e = almaStreamOne( c, 300, sn[a], ssig[b], soff[d], h );
      if( e != TA_TEST_PASS )
         return e;
   }
   return TA_TEST_PASS;
}
