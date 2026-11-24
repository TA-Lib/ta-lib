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
 *  112326 MF,CC  First version (#609).
 *  112426 MF,CC  The MA-type takers refuse SSF past its cap (#609).
 */

/* Description:
 *
 *   Test TA_SSF, Ehlers' SuperSmoother:
 *
 *       y[i] = c1*(x[i] + x[i-1])/2 + c2*y[i-1] + c3*y[i-2]
 *
 *   with a complex pole pair at radius r = exp(-1.414*3.14159/P) and angle
 *   1.414*pi/P. A seed's transient decays as r^k, and its amplitude against
 *   the two seeded output slots is at most (1+r)/(r*sin(angle)).
 *
 *   Legs:
 *     1. DC: a constant returns the constant, within 4e-15*max(1,(P/20)^2)
 *        relative -- the recurrence's own conditioning near DC grows as P^2
 *        (issue #609, constant input, measured at most 0.55 of that bound).
 *     2. NYQUIST: an alternating +/-1 decays to 0, the (x + x[1])/2 zero. The
 *        2004 c1*x numerator leaves a gain of 0.0985 there at P = 10.
 *     3. GAIN: cos(2*pi*i/Q) settles on the transfer function evaluated here
 *        from Ehlers' coefficients as written (the cosine form, not the half
 *        angle the function uses), and |H| at Q = P = 10 is the issue's
 *        analytic 0.6950.
 *     4. LOOKBACK: max(P-1, 3) plus the unstable period, 0 at P = 1; and the
 *        seed sits at startIdx - lookback: a call from startIdx equals, bit for
 *        bit, a call on the input cut to start at the seed.
 *     5. ALIASING: outReal == inReal.
 *     6. The generic start/end range sweep.
 *     7. GOLDEN: issue #609's values over the 252-bar close, P = 10 and 20, at
 *        1e-13 relative: a 70-digit evaluation of the formula, Ehlers'
 *        constants and 3-bar seed, rounded to 15 digits. Every output row up to
 *        50 (and 100 at P = 20) still sees the seed. From bar 100 the P = 10
 *        rows are also the 2014 eSignal Traders' Tip run as published (issue
 *        #609, ta-lib-oracles 142efc8: within 1.4e-16 relative), the one
 *        independent implementation with Ehlers' constants. Plus P = 10000,
 *        the period cap, over 20000 bars against a 70-digit evaluation of the
 *        same formula: measured 6.8e-11 relative, held at 5e-10; the
 *        recurrence's own conditioning there is 1.2e-10 (issue #609, b).
 *     8. EDGES: one step outside the period range is refused.
 *     9. MA: TA_MAType_SSF is TA_SSF bit for bit, its lookback is TA_SSF's at a
 *        non-zero unstable period, a period over SSF's cap is refused through
 *        MA, and MAVP over a constant period array is TA_SSF.
 *
 *    10. TAKERS: every other function taking an MA type, with SSF on each of
 *        its MA-type parameters: one past SSF's cap the lookback is -1 and the
 *        call TA_BAD_PARAM, before any buffer is sized from it; at the cap
 *        both succeed.
 *
 *   SERVER_VERIFY: the constant and Nyquist shapes, the golden call at P = 20
 *   and the period cap are routed, and every taker's lookback at and past
 *   SSF's cap is compared: the --codegen sweep reaches none of them.
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
#define SSF_PI 3.14159265358979323846

#define SSF_N_MAX   120000
#define SSF_N_SHAPE  20000
#define SSF_N_ROUTE   2000
#define SSF_N_CAP_ROUTE 14000

#define NB_OF(a) ((int)(sizeof(a)/sizeof((a)[0])))

/* Pinned coverage counts: loop trips decided by periods and lengths alone. */
#define SSF_DC_CMP        434595
#define SSF_NYQ_CMP       135440
#define SSF_GAIN_CMP      40000
#define SSF_LOOKBACK_CMP  96
#define SSF_INPLACE_CMP   18386
#define SSF_ROUTE_CMP     40
#define SSF_GOLDEN_CMP    19
#define SSF_EDGE_CMP      6
#define SSF_MA_CMP        23
#define SSF_TAKER_CMP     54

static double ssfIn[SSF_N_MAX];
static double ssfOut[SSF_N_MAX];
static double ssfAlias[SSF_N_MAX];
static double ssfOut3[SSF_N_MAX];

static int g_ssfDcCmp;
static int g_ssfNyqCmp;
static int g_ssfGainCmp;
static int g_ssfLookbackCmp;
static int g_ssfInplaceCmp;
static int g_ssfRouteCmp;
static int g_ssfGoldenCmp;
static int g_ssfEdgeCmp;
static int g_ssfMaCmp;
static int g_ssfTakerCmp;

static const int ssfPeriods[] = { 2, 3, 5, 10, 20, 100, 1000, 10000 };
static const double ssfLevel[] = { 0.00777, 100.0, 30000.7 };
static const unsigned int ssfUnstValues[] = { 0, 1, 3, 50 };

static double      ssfRadius( int P );
static void        ssfShapeSeries( int n );
static ErrorNumber ssfRoute( const char *tag, const double *in, int n, int P,
                             TA_RetCode rc, int begIdx, int nbElement );
static ErrorNumber test_ssf_dc( void );
static ErrorNumber test_ssf_nyquist( void );
static ErrorNumber test_ssf_gain( void );
static ErrorNumber test_ssf_lookback( void );
static ErrorNumber test_ssf_inplace( void );
static ErrorNumber test_ssf_range( const TA_Real *in );
static ErrorNumber test_ssf_golden( const TA_Real *close );
static ErrorNumber test_ssf_edges( void );
static ErrorNumber test_ssf_ma( void );
static ErrorNumber test_ssf_takers( void );

/**** Global functions definitions. ****/
ErrorNumber test_func_ssf( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   g_ssfDcCmp = g_ssfNyqCmp = g_ssfGainCmp = g_ssfLookbackCmp = 0;
   g_ssfInplaceCmp = g_ssfRouteCmp = g_ssfGoldenCmp = g_ssfEdgeCmp = g_ssfMaCmp = 0;
   g_ssfTakerCmp = 0;

   err = test_ssf_dc();
   if( err == TA_TEST_PASS )
      err = test_ssf_nyquist();
   if( err == TA_TEST_PASS )
      err = test_ssf_gain();
   if( err == TA_TEST_PASS )
      err = test_ssf_lookback();
   if( err == TA_TEST_PASS )
      err = test_ssf_inplace();
   if( err == TA_TEST_PASS )
      err = test_ssf_golden( history->close );
   if( err == TA_TEST_PASS )
      err = test_ssf_edges();
   if( err == TA_TEST_PASS )
      err = test_ssf_ma();
   if( err == TA_TEST_PASS )
      err = test_ssf_takers();

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS )
      err = test_ssf_range( history->close );

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS
       && ( g_ssfDcCmp != SSF_DC_CMP || g_ssfNyqCmp != SSF_NYQ_CMP
            || g_ssfGainCmp != SSF_GAIN_CMP || g_ssfLookbackCmp != SSF_LOOKBACK_CMP
            || g_ssfInplaceCmp != SSF_INPLACE_CMP || g_ssfGoldenCmp != SSF_GOLDEN_CMP
            || g_ssfEdgeCmp != SSF_EDGE_CMP || g_ssfMaCmp != SSF_MA_CMP
            || g_ssfTakerCmp != SSF_TAKER_CMP
            || ( server_verify_active() && g_ssfRouteCmp != SSF_ROUTE_CMP ) ) )
   {
      printf( "SSF Fail: coverage counters (dc %d, nyquist %d, gain %d, lookback %d, "
              "inplace %d, routed %d, golden %d, edges %d, ma %d, takers %d) are not what "
              "this file was written with (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n",
              g_ssfDcCmp, g_ssfNyqCmp, g_ssfGainCmp, g_ssfLookbackCmp, g_ssfInplaceCmp,
              g_ssfRouteCmp, g_ssfGoldenCmp, g_ssfEdgeCmp, g_ssfMaCmp, g_ssfTakerCmp,
              SSF_DC_CMP, SSF_NYQ_CMP, SSF_GAIN_CMP, SSF_LOOKBACK_CMP, SSF_INPLACE_CMP,
              SSF_ROUTE_CMP, SSF_GOLDEN_CMP, SSF_EDGE_CMP, SSF_MA_CMP, SSF_TAKER_CMP );
      return TA_SSF_VACUOUS;
   }

   return err;
}

/**** Local functions definitions. ****/
static double ssfRadius( int P )
{
   return exp( -1.414 * 3.14159 / (double)P );
}

/* 100 + 10*sin(i/7) + 0.15*i + 2*sin(i/2.3): a trend, a slow cycle and a fast
 * one, the series of issue #609's period-cap measurements. */
static void ssfShapeSeries( int n )
{
   int i;
   for( i = 0; i < n; i++ )
      ssfIn[i] = 100.0 + 10.0*sin( (double)i/7.0 ) + 0.15*(double)i + 2.0*sin( (double)i/2.3 );
}

/* Replay one call on every live language server. The caller has made the C
 * call over [0, n-1] into ssfOut. */
static ErrorNumber ssfRoute( const char *tag, const double *in, int n, int P,
                             TA_RetCode rc, int begIdx, int nbElement )
{
   double optIn[1];
   ErrorNumber e;
   int cmpBefore;

   if( !server_verify_active() )
      return TA_TEST_PASS;

   optIn[0] = (double)P;
   cmpBefore = server_verify_comparisons();
   e = server_verify( "SSF", 0, n-1, n, rc, begIdx, nbElement,
                      (const TA_Real*[]){ in, NULL }, optIn, 1,
                      (const TA_Real*[]){ ssfOut, NULL }, NULL );
   if( e != TA_TEST_PASS )
      return e;
   if( server_verify_comparisons() == cmpBefore )
   {
      printf( "SSF %s [P=%d]: compared no server despite live pipes\n", tag, P );
      return TA_SV_ROUTED_VACUOUS;
   }
   g_ssfRouteCmp++;
   return TA_TEST_PASS;
}

/* (1) A constant input. The seed is the constant, so there is no transient and
 * the only error is the coefficients' and the recurrence's conditioning. */
static ErrorNumber test_ssf_dc( void )
{
   int pi, li, i, n, begIdx, nbElement;
   TA_RetCode rc;
   ErrorNumber e;

   for( pi = 0; pi < NB_OF(ssfPeriods); pi++ )
   for( li = 0; li < NB_OF(ssfLevel); li++ )
   {
      int P = ssfPeriods[pi];
      double L = ssfLevel[li];
      double bound = 4e-15 * ( (P > 20) ? ((double)P/20.0)*((double)P/20.0) : 1.0 );

      n = 12*P < 4000 ? 4000 : 12*P;
      for( i = 0; i < n; i++ )
         ssfIn[i] = L;

      rc = TA_SSF( 0, n-1, ssfIn, P, &begIdx, &nbElement, ssfOut );
      if( rc != TA_SUCCESS || begIdx != TA_SSF_Lookback( P ) || begIdx + nbElement != n )
      {
         printf( "SSF DC Fail [P=%d L=%g]: rc=%d (%d,%d)\n", P, L, (int)rc, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         double r = fabs( ssfOut[i] - L ) / L;
         if( !( r <= bound ) )
         {
            printf( "SSF DC Fail [P=%d L=%g] at bar %d: %.17g, relative %.3e > %.3e\n",
                    P, L, begIdx+i, ssfOut[i], r, bound );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ssfDcCmp++;
      }
   }

   for( i = 0; i < SSF_N_ROUTE; i++ )
      ssfIn[i] = 100.0;
   rc = TA_SSF( 0, SSF_N_ROUTE-1, ssfIn, 10, &begIdx, &nbElement, ssfOut );
   e = ssfRoute( "dc", ssfIn, SSF_N_ROUTE, 10, rc, begIdx, nbElement );
   return e;
}

/* (2) An alternating +/-1: the Nyquist frequency, where the numerator is
 * exactly zero. What is left after the seed is the free response. */
static ErrorNumber test_ssf_nyquist( void )
{
   int pi, i, n, k0, begIdx, nbElement;
   TA_RetCode rc;

   n = SSF_N_SHAPE;
   for( i = 0; i < n; i++ )
      ssfIn[i] = (i & 1) ? -1.0 : 1.0;

   for( pi = 0; pi < NB_OF(ssfPeriods); pi++ )
   {
      int P = ssfPeriods[pi];
      double worst = 0.0, bound;

      /* Compared from bar 4P on, where r^(4P) = exp(-17.8). */
      k0 = 4*P;
      if( k0 >= n/2 )
         continue;

      rc = TA_SSF( 0, n-1, ssfIn, P, &begIdx, &nbElement, ssfOut );
      if( rc != TA_SUCCESS || begIdx + nbElement != n )
      {
         printf( "SSF Nyquist Fail [P=%d]: rc=%d (%d,%d)\n", P, (int)rc, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = k0 - begIdx; i < nbElement; i++ )
      {
         double d = fabs( ssfOut[i] );
         if( d > worst ) worst = d;
      }
      /* Bars 1 and 2 hold +/-1 and the forcing is zero from bar 3, so this is
       * the free response of the pair: |y[t]| <= (1+r)/(r*sin(angle)) * r^(t-1). */
      {
         double r = ssfRadius( P );
         bound = 2.0 * (1.0 + r) / ( r * sin( 1.414 * SSF_PI / (double)P ) )
               * pow( r, (double)(k0 - 1) ) + 1e-15;
      }
      if( !( worst <= bound ) )
      {
         printf( "SSF Nyquist Fail [P=%d]: worst %.3e > %.3e\n", P, worst, bound );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfNyqCmp += nbElement - (k0 - begIdx);
   }

   rc = TA_SSF( 0, SSF_N_ROUTE-1, ssfIn, 10, &begIdx, &nbElement, ssfOut );
   return ssfRoute( "nyquist", ssfIn, SSF_N_ROUTE, 10, rc, begIdx, nbElement );
}

/* (3) A sinusoid of Q bars a cycle settles on Re(H(w) e^(jwi)). H is computed
 * here from Ehlers' coefficients as he writes them. */
static ErrorNumber test_ssf_gain( void )
{
   static const struct { int P; int Q; } cell[] =
   {
      { 10, 10 }, { 10, 5 }, { 10, 40 }, { 20, 20 }, { 20, 7 }, { 3, 8 }, { 100, 100 }, { 100, 50 }
   };
   int c, i, n, k0, begIdx, nbElement;
   TA_RetCode rc;

   n = SSF_N_SHAPE;
   for( c = 0; c < NB_OF(cell); c++ )
   {
      int P = cell[c].P, Q = cell[c].Q;
      double a1 = ssfRadius( P );
      double c2 = 2.0 * a1 * cos( 1.414 * SSF_PI / (double)P );
      double c3 = -a1 * a1;
      double c1 = 1.0 - c2 - c3;
      double w  = 2.0 * SSF_PI / (double)Q;
      /* H = c1*(1 + z)/2 / (1 - c2*z - c3*z^2), z = e^(-jw) */
      double nr = 0.5 * c1 * ( 1.0 + cos(w) ), ni = -0.5 * c1 * sin(w);
      double dr = 1.0 - c2*cos(w) - c3*cos(2.0*w), di = c2*sin(w) + c3*sin(2.0*w);
      double dd = dr*dr + di*di;
      double hr = ( nr*dr + ni*di ) / dd, hi = ( ni*dr - nr*di ) / dd;
      double worst = 0.0, bound;

      if( P == 10 && Q == 10 && !( fabs( sqrt( hr*hr + hi*hi ) - 0.6950 ) <= 5e-5 ) )
      {
         printf( "SSF gain Fail: |H| at P = Q = 10 is %.6f, the issue's analytic 0.6950\n",
                 sqrt( hr*hr + hi*hi ) );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( i = 0; i < n; i++ )
         ssfIn[i] = cos( w * (double)i );

      rc = TA_SSF( 0, n-1, ssfIn, P, &begIdx, &nbElement, ssfOut );
      if( rc != TA_SUCCESS || begIdx + nbElement != n )
      {
         printf( "SSF gain Fail [P=%d Q=%d]: rc=%d (%d,%d)\n", P, Q, (int)rc, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      k0 = n * 3 / 4;
      for( i = k0 - begIdx; i < nbElement; i++ )
      {
         int t = begIdx + i;
         /* Re(H e^(jwt)), with the input's own phase so that the probe's libm
          * argument reduction cancels. */
         double want = hr * ssfIn[t] - hi * sin( w * (double)t );
         double d = fabs( ssfOut[i] - want );
         if( d > worst ) worst = d;
      }
      /* The transient is gone (r^15000 is under 1e-280 at these periods);
       * 1e-10 is the probe's own floor: cos and sin at arguments up to 2.5e4
       * lose |arg|*eps, and 20000 recursive steps round. A wrong c2 misses by
       * order 1e-2. */
      bound = 1e-10;
      if( !( worst <= bound ) )
      {
         printf( "SSF gain Fail [P=%d Q=%d]: worst %.3e > %.3e\n", P, Q, worst, bound );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfGainCmp += nbElement - (k0 - begIdx);
   }

   return TA_TEST_PASS;
}

/* (4) The lookback, and where the seed sits. */
static ErrorNumber test_ssf_lookback( void )
{
   static const int perLb[] = { 1, 2, 3, 4, 5, 10, 20, 10000 };
   static const int perSeed[] = { 2, 3, 4, 10 };
   int ui, pi, k, i, n, b1, n1, b2, n2;
   TA_RetCode rc1, rc2;

   n = 600;
   for( i = 0; i < n; i++ )
      ssfIn[i] = 100.0 + (double)(i % 17) - (double)(i % 5) + 0.25*(double)(i % 3);

   for( ui = 0; ui < NB_OF(ssfUnstValues); ui++ )
   {
      unsigned int u = ssfUnstValues[ui];

      TA_SetUnstablePeriod( TA_FUNC_UNST_SSF, u );

      for( pi = 0; pi < NB_OF(perLb); pi++ )
      {
         int P = perLb[pi];
         int want = ( P == 1 ? 0 : ( P > 4 ? P - 1 : 3 ) ) + (int)u;
         int lb = TA_SSF_Lookback( P );
         if( lb != want )
         {
            printf( "SSF lookback Fail [P=%d unst %u]: %d, expected %d\n", P, u, lb, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ssfLookbackCmp++;
      }

      /* A call from startIdx seeds at startIdx - lookback: the same call on
       * the input cut to begin there, from startIdx = lookback, gives the
       * same bits. */
      for( pi = 0; pi < NB_OF(perSeed); pi++ )
      {
         int P = perSeed[pi];
         int lb = TA_SSF_Lookback( P );
         const int starts[] = { lb, lb + 1, lb + 7, 300 };

         for( k = 0; k < NB_OF(starts); k++ )
         {
            int s = starts[k], cut = s - lb;

            rc1 = TA_SSF( s, n-1, ssfIn, P, &b1, &n1, ssfOut );
            rc2 = TA_SSF( lb, n-1-cut, ssfIn + cut, P, &b2, &n2, ssfAlias );
            if( rc1 != TA_SUCCESS || rc2 != TA_SUCCESS || b1 != s || b2 != lb
                || n1 != n - s || n2 != n1
                || memcmp( ssfOut, ssfAlias, (size_t)n1 * sizeof(double) ) != 0 )
            {
               printf( "SSF seed Fail [P=%d unst %u start %d]: rc=%d/%d (%d,%d) (%d,%d)\n",
                       P, u, s, (int)rc1, (int)rc2, b1, n1, b2, n2 );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_ssfLookbackCmp++;
         }
      }

      TA_SetUnstablePeriod( TA_FUNC_UNST_SSF, 0 );
   }

   return TA_TEST_PASS;
}

/* (5) outReal may alias inReal: x[i-1] is carried in a local, never re-read. */
static ErrorNumber test_ssf_inplace( void )
{
   static const int per[] = { 1, 2, 3, 10, 100 };
   static const int starts[] = { 0, 300 };
   int pi, is, i, n, b1, n1, b2, n2;
   TA_RetCode rc1, rc2;

   n = 2000;
   for( i = 0; i < n; i++ )
      ssfIn[i] = 100.0 + (double)(i % 23) * 0.5 - (double)(i % 7);

   for( pi = 0; pi < NB_OF(per); pi++ )
   for( is = 0; is < NB_OF(starts); is++ )
   {
      memcpy( ssfAlias, ssfIn, (size_t)n * sizeof(double) );
      rc1 = TA_SSF( starts[is], n-1, ssfIn, per[pi], &b1, &n1, ssfOut );
      rc2 = TA_SSF( starts[is], n-1, ssfAlias, per[pi], &b2, &n2, ssfAlias );
      if( rc1 != TA_SUCCESS || rc2 != TA_SUCCESS || b1 != b2 || n1 != n2 || n1 <= 0
          || memcmp( ssfOut, ssfAlias, (size_t)n1 * sizeof(double) ) != 0 )
      {
         printf( "SSF in-place Fail [P=%d start %d]\n", per[pi], starts[is] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfInplaceCmp += n1;
   }

   return TA_TEST_PASS;
}

/* (6) The generic range sweep under TA_STABLE_CONVERGING. */
static TA_RetCode ssfRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_SSF_Lookback( 10 );
   return TA_SSF( startIdx, endIdx, (const TA_Real *)opaqueData, 10,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_ssf_range( const TA_Real *in )
{
   return doRangeTestEx( ssfRangeTestFunction, TA_STABLE_CONVERGING, TA_FUNC_UNST_SSF,
                         (void *)in, 1, 0 );
}

/* (7) Frozen values. */
#define SSF_GOLDEN_ROWS 10
#define SSF_CAP_ROWS     6

static ErrorNumber test_ssf_golden( const TA_Real *close )
{
   /* Issue #609, "Golden values": ref252 close, unstable period 0, startIdx 0.
    * Rows 3 to 5 are seed state below both lookbacks, and row 10 is below
    * P = 20's: kept so the table reads as published, and skipped. */
   static const int bar[SSF_GOLDEN_ROWS] = { 3, 4, 5, 10, 20, 50, 100, 150, 200, 251 };
   static const struct { int P; double want[SSF_GOLDEN_ROWS]; } golden[] =
   {
      { 10, { 94.285158337482, 94.2867698371732, 94.2661009806956, 92.4834595343629,
              89.8900426803413, 90.4757033729488, 114.180382749508, 121.688788240409,
              106.810205780275, 108.997025648303 } },
      { 20, { 94.1212629522085, 93.9835228340202, 93.9124893114572, 92.9910157811491,
              91.351079458768, 89.8299198560279, 116.231842893303, 122.278808504334,
              109.802328079071, 109.000233405625 } }
   };
   /* P = 10000 over ssfShapeSeries(20000): a 70-digit evaluation of the
    * formula over the doubles of that series (mpmath, 70 digits). */
   static const int    capBar[SSF_CAP_ROWS]  = { 9999, 10000, 12500, 15000, 17500, 19999 };
   static const double capWant[SSF_CAP_ROWS] =
   {
      1.20414412985210856e+03, 1.20431463489252565e+03, 1.62537636385333849e+03,
      2.01518792612665106e+03, 2.38952467410601957e+03, 2.76257416774691319e+03
   };
   int g, k, begIdx, nbElement;
   TA_RetCode rc;
   ErrorNumber e;

   for( g = 0; g < NB_OF(golden); g++ )
   {
      int P = golden[g].P;
      rc = TA_SSF( 0, 251, close, P, &begIdx, &nbElement, ssfOut );
      if( rc != TA_SUCCESS || begIdx != P - 1 || nbElement != 252 - begIdx )
      {
         printf( "SSF golden Fail [P=%d]: rc=%d (%d,%d)\n", P, (int)rc, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( k = 0; k < SSF_GOLDEN_ROWS; k++ )
      {
         double want = golden[g].want[k], got;
         if( bar[k] < begIdx )
            continue;
         got = ssfOut[bar[k] - begIdx];
         if( !( fabs( got - want ) <= 1e-13 * fabs( want ) ) )
         {
            printf( "SSF golden Fail [P=%d] at bar %d: %.17g, expected %.15g\n",
                    P, bar[k], got, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ssfGoldenCmp++;
      }
      if( P == 20 )
      {
         e = ssfRoute( "golden", close, 252, P, rc, begIdx, nbElement );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   ssfShapeSeries( SSF_N_SHAPE );
   rc = TA_SSF( 0, SSF_N_SHAPE-1, ssfIn, 10000, &begIdx, &nbElement, ssfOut );
   if( rc != TA_SUCCESS || begIdx != 9999 || nbElement != SSF_N_SHAPE - 9999 )
   {
      printf( "SSF cap Fail: rc=%d (%d,%d)\n", (int)rc, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( k = 0; k < SSF_CAP_ROWS; k++ )
   {
      double got = ssfOut[capBar[k] - begIdx];
      if( !( fabs( got - capWant[k] ) <= 5e-10 * capWant[k] ) )
      {
         printf( "SSF cap Fail at bar %d: %.17g, expected %.17g\n", capBar[k], got, capWant[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfGoldenCmp++;
   }
   /* Routed over 14000 bars: a transcendental call is compared element by
    * element, at most 4096 outputs. */
   rc = TA_SSF( 0, SSF_N_CAP_ROUTE-1, ssfIn, 10000, &begIdx, &nbElement, ssfOut );
   return ssfRoute( "cap", ssfIn, SSF_N_CAP_ROUTE, 10000, rc, begIdx, nbElement );
}

/* (8) One step outside the period range. */
static ErrorNumber test_ssf_edges( void )
{
   static const int bad[]  = { 0, -1, 10001 };
   static const int good[] = { 1, 2, 3 };
   int k, i, begIdx, nbElement;
   TA_RetCode rc;

   ssfShapeSeries( 400 );

   for( k = 0; k < NB_OF(bad); k++ )
   {
      rc = TA_SSF( 0, 399, ssfIn, bad[k], &begIdx, &nbElement, ssfOut );
      if( rc != TA_BAD_PARAM || TA_SSF_Lookback( bad[k] ) != -1 )
      {
         printf( "SSF edge Fail [P=%d]: rc=%d lookback %d, expected TA_BAD_PARAM\n",
                 bad[k], (int)rc, TA_SSF_Lookback( bad[k] ) );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfEdgeCmp++;
   }

   for( k = 0; k < NB_OF(good); k++ )
   {
      rc = TA_SSF( 0, 399, ssfIn, good[k], &begIdx, &nbElement, ssfOut );
      if( rc != TA_SUCCESS || begIdx + nbElement != 400 )
      {
         printf( "SSF edge Fail [P=%d]: rc=%d (%d,%d)\n", good[k], (int)rc, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         /* Every period is stable, and the gain never exceeds 1 at P = 2. */
         if( !( fabs( ssfOut[i] - 160.0 ) < 100.0 ) )
         {
            printf( "SSF edge Fail [P=%d] at bar %d: %.17g\n", good[k], begIdx+i, ssfOut[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      g_ssfEdgeCmp++;
   }

   return TA_TEST_PASS;
}

/* (9) TA_MAType_SSF, through MA and MAVP. */
static ErrorNumber test_ssf_ma( void )
{
   /* Not 1: MA answers period 1 itself, before it dispatches. */
   static const int per[] = { 2, 3, 10, 200, 10000 };
   static const unsigned int unst[] = { 0, 7 };
   int pi, ui, i, n, b1, n1, b2, n2;
   TA_RetCode rc1, rc2;

   n = SSF_N_SHAPE;
   ssfShapeSeries( n );

   for( ui = 0; ui < NB_OF(unst); ui++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_SSF, unst[ui] );
      for( pi = 0; pi < NB_OF(per); pi++ )
      {
         int P = per[pi];

         /* MA forwards to the callee, which clamps to its own lookback, so no
          * value can show a wrong MA lookback arm: only this compare can. */
         if( TA_MA_Lookback( P, TA_MAType_SSF ) != TA_SSF_Lookback( P ) )
         {
            printf( "SSF MA lookback Fail [P=%d unst %u]: %d, TA_SSF's %d\n", P, unst[ui],
                    TA_MA_Lookback( P, TA_MAType_SSF ), TA_SSF_Lookback( P ) );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ssfMaCmp++;

         rc1 = TA_SSF( 0, n-1, ssfIn, P, &b1, &n1, ssfOut );
         rc2 = TA_MA( 0, n-1, ssfIn, P, TA_MAType_SSF, &b2, &n2, ssfAlias );
         if( rc1 != TA_SUCCESS || rc2 != TA_SUCCESS || b1 != b2 || n1 != n2 || n1 <= 0
             || memcmp( ssfOut, ssfAlias, (size_t)n1 * sizeof(double) ) != 0 )
         {
            printf( "SSF MA Fail [P=%d unst %u]: rc=%d/%d (%d,%d) (%d,%d)\n", P, unst[ui],
                    (int)rc1, (int)rc2, b1, n1, b2, n2 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_ssfMaCmp++;
      }
   }
   TA_SetUnstablePeriod( TA_FUNC_UNST_SSF, 0 );

   /* Selecting SSF caps MA's period at SSF's own. SMA is the control. */
   rc1 = TA_MA( 0, n-1, ssfIn, 10001, TA_MAType_SSF, &b1, &n1, ssfOut );
   rc2 = TA_MA( 0, n-1, ssfIn, 10001, TA_MAType_SMA, &b2, &n2, ssfAlias );
   if( rc1 != TA_BAD_PARAM || TA_MA_Lookback( 10001, TA_MAType_SSF ) != -1
       || rc2 != TA_SUCCESS || TA_MA_Lookback( 10001, TA_MAType_SMA ) != 10000 )
   {
      printf( "SSF MA cap Fail: rc=%d lookback %d; SMA rc=%d lookback %d\n", (int)rc1,
              TA_MA_Lookback( 10001, TA_MAType_SSF ), (int)rc2,
              TA_MA_Lookback( 10001, TA_MAType_SMA ) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_ssfMaCmp++;

   /* MAVP over a constant period array runs that one period from MAVP's own
    * lookback, which is SSF's at the largest period. */
   {
      static double periods[SSF_N_SHAPE];
      int lb = TA_MAVP_Lookback( 2, 30, TA_MAType_SSF );

      for( i = 0; i < n; i++ )
         periods[i] = 10.0;
      if( lb != TA_SSF_Lookback( 30 ) )
      {
         printf( "SSF MAVP lookback Fail: %d, TA_SSF's %d\n", lb, TA_SSF_Lookback( 30 ) );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      rc1 = TA_SSF( lb, n-1, ssfIn, 10, &b1, &n1, ssfOut );
      rc2 = TA_MAVP( 0, n-1, ssfIn, periods, 2, 30, TA_MAType_SSF, &b2, &n2, ssfAlias );
      if( rc1 != TA_SUCCESS || rc2 != TA_SUCCESS || b1 != b2 || n1 != n2 || n1 <= 0
          || memcmp( ssfOut, ssfAlias, (size_t)n1 * sizeof(double) ) != 0 )
      {
         printf( "SSF MAVP Fail: rc=%d/%d (%d,%d) (%d,%d)\n", (int)rc1, (int)rc2, b1, n1, b2, n2 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfMaCmp++;

      rc2 = TA_MAVP( 0, n-1, ssfIn, periods, 2, 10001, TA_MAType_SSF, &b2, &n2, ssfAlias );
      if( rc2 != TA_BAD_PARAM )
      {
         printf( "SSF MAVP cap Fail: rc=%d, expected TA_BAD_PARAM\n", (int)rc2 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfMaCmp++;
   }

   return TA_TEST_PASS;
}

/* (10) The MA-type takers. A callee lookback of -1 used to be added into the
 * taker's own lookback and buffer sizes (a heap overflow in TA_MACDEXT, found
 * by the abstract-API fuzzer). */
#define SSF_NB_TAKER 18
static const char * const ssfTakerName[SSF_NB_TAKER] =
{
   "BBANDS", "BBW", "PERCENTB", "MACDEXT fast", "MACDEXT slow", "MACDEXT signal",
   "APO", "PPO", "PVO", "STOCH slowK", "STOCH slowD", "STOCHF", "STOCHRSI",
   "KDJ slowK", "KDJ slowD", "KSTEXT ROC MAs", "KSTEXT signal", "MAENV"
};

static TA_RetCode ssfTaker( int k, int P, int n, int *lookback, int *begIdx, int *nbElement )
{
   const double *x = ssfIn;
   double *o1 = ssfOut, *o2 = ssfAlias, *o3 = ssfOut3;
   const TA_MAType S = TA_MAType_SSF, M = TA_MAType_SMA;

   switch( k )
   {
   case 0:
      *lookback = TA_BBANDS_Lookback( P, 2.0, 2.0, S );
      return TA_BBANDS( 0, n-1, x, P, 2.0, 2.0, S, begIdx, nbElement, o1, o2, o3 );
   case 1:
      *lookback = TA_BBW_Lookback( P, 2.0, 2.0, S );
      return TA_BBW( 0, n-1, x, P, 2.0, 2.0, S, begIdx, nbElement, o1 );
   case 2:
      *lookback = TA_PERCENTB_Lookback( P, 2.0, 2.0, S );
      return TA_PERCENTB( 0, n-1, x, P, 2.0, 2.0, S, begIdx, nbElement, o1 );
   case 3:
      *lookback = TA_MACDEXT_Lookback( P, S, 26, M, 9, M );
      return TA_MACDEXT( 0, n-1, x, P, S, 26, M, 9, M, begIdx, nbElement, o1, o2, o3 );
   case 4:
      *lookback = TA_MACDEXT_Lookback( 12, M, P, S, 9, M );
      return TA_MACDEXT( 0, n-1, x, 12, M, P, S, 9, M, begIdx, nbElement, o1, o2, o3 );
   case 5:
      *lookback = TA_MACDEXT_Lookback( 12, M, 26, M, P, S );
      return TA_MACDEXT( 0, n-1, x, 12, M, 26, M, P, S, begIdx, nbElement, o1, o2, o3 );
   case 6:
      *lookback = TA_APO_Lookback( 12, P, S );
      return TA_APO( 0, n-1, x, 12, P, S, begIdx, nbElement, o1 );
   case 7:
      *lookback = TA_PPO_Lookback( 12, P, S );
      return TA_PPO( 0, n-1, x, 12, P, S, begIdx, nbElement, o1 );
   case 8:
      *lookback = TA_PVO_Lookback( 12, P, S );
      return TA_PVO( 0, n-1, x, 12, P, S, begIdx, nbElement, o1 );
   case 9:
      *lookback = TA_STOCH_Lookback( 5, P, S, 3, M );
      return TA_STOCH( 0, n-1, x, x, x, 5, P, S, 3, M, begIdx, nbElement, o1, o2 );
   case 10:
      *lookback = TA_STOCH_Lookback( 5, 3, M, P, S );
      return TA_STOCH( 0, n-1, x, x, x, 5, 3, M, P, S, begIdx, nbElement, o1, o2 );
   case 11:
      *lookback = TA_STOCHF_Lookback( 5, P, S );
      return TA_STOCHF( 0, n-1, x, x, x, 5, P, S, begIdx, nbElement, o1, o2 );
   case 12:
      *lookback = TA_STOCHRSI_Lookback( 14, 5, P, S );
      return TA_STOCHRSI( 0, n-1, x, 14, 5, P, S, begIdx, nbElement, o1, o2 );
   case 13:
      *lookback = TA_KDJ_Lookback( 9, P, S, 3, M );
      return TA_KDJ( 0, n-1, x, x, x, 9, P, S, 3, M, begIdx, nbElement, o1, o2, o3 );
   case 14:
      *lookback = TA_KDJ_Lookback( 9, 3, M, P, S );
      return TA_KDJ( 0, n-1, x, x, x, 9, 3, M, P, S, begIdx, nbElement, o1, o2, o3 );
   case 15:
      *lookback = TA_KSTEXT_Lookback( 10, 15, 20, 30, P, 10, 10, 15, 9, S, M );
      return TA_KSTEXT( 0, n-1, x, 10, 15, 20, 30, P, 10, 10, 15, 9, S, M,
                        begIdx, nbElement, o1, o2 );
   case 16:
      *lookback = TA_KSTEXT_Lookback( 10, 15, 20, 30, 10, 10, 10, 15, P, M, S );
      return TA_KSTEXT( 0, n-1, x, 10, 15, 20, 30, 10, 10, 10, 15, P, M, S,
                        begIdx, nbElement, o1, o2 );
   default:
      *lookback = TA_MAENV_Lookback( P, 2.5, 2.5, S );
      return TA_MAENV( 0, n-1, x, P, 2.5, 2.5, S, begIdx, nbElement, o1, o2, o3 );
   }
}

/* ssfTaker's parameters, in signature order, for the lookback comparison. */
static int ssfTakerOpt( int k, int P, double opt[], const char **name )
{
   const double S = (double)TA_MAType_SSF, M = (double)TA_MAType_SMA, p = (double)P;
   static const char * const fn[SSF_NB_TAKER] =
   {
      "BBANDS", "BBW", "PERCENTB", "MACDEXT", "MACDEXT", "MACDEXT", "APO", "PPO", "PVO",
      "STOCH", "STOCH", "STOCHF", "STOCHRSI", "KDJ", "KDJ", "KSTEXT", "KSTEXT", "MAENV"
   };
   const double v[SSF_NB_TAKER][11] =
   {
      { p, 2, 2, S }, { p, 2, 2, S }, { p, 2, 2, S },
      { p, S, 26, M, 9, M }, { 12, M, p, S, 9, M }, { 12, M, 26, M, p, S },
      { 12, p, S }, { 12, p, S }, { 12, p, S },
      { 5, p, S, 3, M }, { 5, 3, M, p, S }, { 5, p, S }, { 14, 5, p, S },
      { 9, p, S, 3, M }, { 9, 3, M, p, S },
      { 10, 15, 20, 30, p, 10, 10, 15, 9, S, M }, { 10, 15, 20, 30, 10, 10, 10, 15, p, M, S },
      { p, 2.5, 2.5, S }
   };
   static const int nb[SSF_NB_TAKER] = { 4, 4, 4, 6, 6, 6, 3, 3, 3, 5, 5, 3, 4, 5, 5, 11, 11, 4 };
   int i;

   for( i = 0; i < nb[k]; i++ )
      opt[i] = v[k][i];
   *name = fn[k];
   return nb[k];
}

static ErrorNumber test_ssf_takers( void )
{
   int k, n, lookback, begIdx, nbElement;
   TA_RetCode rc;
   ErrorNumber e;

   n = SSF_N_SHAPE;
   ssfShapeSeries( n );

   for( k = 0; k < SSF_NB_TAKER; k++ )
   {
      begIdx = nbElement = -7;
      rc = ssfTaker( k, 10001, n, &lookback, &begIdx, &nbElement );
      if( lookback != -1 )
      {
         printf( "SSF taker Fail [%s]: lookback %d one past SSF's cap, expected -1\n",
                 ssfTakerName[k], lookback );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfTakerCmp++;
      if( rc != TA_BAD_PARAM )
      {
         printf( "SSF taker Fail [%s]: rc=%d one past SSF's cap, expected TA_BAD_PARAM\n",
                 ssfTakerName[k], (int)rc );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfTakerCmp++;

      rc = ssfTaker( k, 10000, n, &lookback, &begIdx, &nbElement );
      if( rc != TA_SUCCESS || lookback < 0 || begIdx != lookback || begIdx + nbElement != n )
      {
         printf( "SSF taker Fail [%s]: at SSF's cap rc=%d lookback %d (%d,%d)\n",
                 ssfTakerName[k], (int)rc, lookback, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_ssfTakerCmp++;
   }

   /* Every language's lookback for the same parameters: one past the cap it
    * is the rejection, -1. server_verify() compares no rejected call. */
   if( server_verify_active() )
   {
      static const int caps[] = { 10001, 10000 };
      double opt[11];
      const char *name;
      int nbOpt, c, before;

      for( k = 0; k < SSF_NB_TAKER; k++ )
      for( c = 0; c < NB_OF(caps); c++ )
      {
         nbOpt = ssfTakerOpt( k, caps[c], opt, &name );
         before = server_verify_lookback_values();
         e = server_verify_lookback_value( name, opt, nbOpt );
         if( e != TA_TEST_PASS )
            return e;
         if( server_verify_lookback_values() == before )
         {
            printf( "SSF taker %s [P=%d]: compared no server lookback despite live pipes\n",
                    ssfTakerName[k], caps[c] );
            return TA_SV_ROUTED_VACUOUS;
         }
         g_ssfRouteCmp++;
      }
   }

   return TA_TEST_PASS;
}
