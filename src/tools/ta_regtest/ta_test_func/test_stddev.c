/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   First version.
 *
 */

/* Description:
 *     Test STDDEV function. This tests indirectly the VAR function.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_Integer doRangeTestFlag; /* One will do a call to doRangeTest */

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer optInTimePeriod;
   TA_Real    optInNbDeviation_1;

   TA_RetCode expectedRetCode;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;
} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /*************************/
   /*      STDDEV TEST      */
   /*************************/
   { 1, 0, 251, 5, 1.0, TA_SUCCESS,     0, 1.2856,  4,  252-4 }, /* First Value */
   { 0, 0, 251, 5, 1.0, TA_SUCCESS,     1, 0.4462,  4,  252-4 },
   { 0, 0, 251, 5, 1.0, TA_SUCCESS, 252-5, 0.7144,  4,  252-4 }, /* Last Value */

   { 1, 0, 251, 5, 1.5, TA_SUCCESS,     0, 1.9285,  4,  252-4 }, /* First Value */
   { 0, 0, 251, 5, 1.5, TA_SUCCESS,     1, 0.66937, 4,  252-4 },
   { 0, 0, 251, 5, 1.5, TA_SUCCESS, 252-5, 1.075,   4,  252-4 } /* Last Value */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/* ============================================================================
 * Cancellation-free variance validation (#118). These tests referee VAR/STDDEV
 * against implementation-INDEPENDENT oracles: a fresh per-window two-pass (the
 * standard trusted reference, pandas' rolling-var oracle), NIST StRD certified
 * values, and metamorphic laws (shift/scale invariance, non-negativity). Each
 * catches the catastrophic-cancellation class that E[x^2]-mean^2 (bug 90) fails
 * and that no cross-implementation gate can see (all implementations agree while
 * being co-wrong). All use population variance (divide by n), matching TA_VAR.
 * ==========================================================================*/

/* Deterministic LCG so the data is reproducible across platforms. */
static unsigned int sd_rng_state = 0u;
static double sd_rand( void )   /* uniform [-1, 1) */
{
   sd_rng_state = sd_rng_state * 1103515245u + 12345u;
   return ( (double)( ( sd_rng_state >> 8 ) & 0xffffffu ) / 8388608.0 ) - 1.0;
}

/* Trusted oracle: fresh two-pass population variance of the window [s, s+period),
 * accumulated in long double (~19 digits on x86; degrades gracefully to double on
 * ABIs where long double == double). Also returns the window mean for the
 * condition-number-aware tolerance below. */
static double sd_twopass_var( const double *x, int s, int period, double *outMean )
{
   long double sum = 0.0L, v = 0.0L, mean;
   int j;
   for( j = 0; j < period; j++ ) sum += (long double)x[s+j];
   mean = sum / (long double)period;
   for( j = 0; j < period; j++ ) { long double d = (long double)x[s+j] - mean; v += d * d; }
   if( outMean ) *outMean = (double)mean;
   return (double)( v / (long double)period );
}

/* Compare TA_VAR over a whole series to the fresh per-window two-pass oracle.
 * The tolerance is scaled by the window's condition number kappa = |mean|/stddev:
 * variance of two nearly-equal high-magnitude values is intrinsically ill-
 * conditioned (both the shipped code AND the oracle lose ~kappa*eps digits), so a
 * flat tolerance would false-red. A real regression (catastrophic cancellation:
 * ~kappa^2*eps, or unbounded) still dwarfs this margin. Exactly-0 windows must
 * come back bit-zero; no window may be negative. */
static ErrorNumber sd_check_vs_twopass( const char *label, const double *x,
                                        int n, int period, double baseTol )
{
   static double out[8192];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int k;

   if( n > 8192 ) return TA_TEST_PASS;   /* buffer guard */
   rc = TA_VAR( 0, n-1, x, period, 1.0, &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS )
   {
      printf( "VAR #118 oracle[%s]: rc=%d\n", label, (int)rc );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( k = 0; k < (int)nbElement; k++ )
   {
      double mean = 0.0;
      double ref = sd_twopass_var( x, (int)begIdx + k - ( period - 1 ), period, &mean );
      double d, tol, kappa;
      if( out[k] < 0.0 )
      {
         printf( "VAR #118 oracle[%s]: NEGATIVE var period=%d bar=%d val=%.17g\n",
                 label, period, (int)begIdx + k, out[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( ref == 0.0 )
      {
         if( out[k] != 0.0 )
         {
            printf( "VAR #118 oracle[%s]: expected exact 0 period=%d bar=%d val=%.17g\n",
                    label, period, (int)begIdx + k, out[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         continue;
      }
      kappa = fabs( mean ) / sqrt( ref );
      tol   = baseTol + 100.0 * kappa * 2.2204460492503131e-16;
      d = fabs( out[k] - ref ) / fabs( ref );
      if( d > tol )
      {
         printf( "VAR #118 oracle[%s]: period=%d bar=%d val=%.17g ref=%.17g (rel %.3g > %.3g, kappa %.2g)\n",
                 label, period, (int)begIdx + k, out[k], ref, d, tol, kappa );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (P2) Metamorphic shift-invariance: var(x + c) == var(x). A large additive
 * offset is exactly what E[x^2]-mean^2 cannot survive (bug 90); the shifted-data
 * algorithm absorbs it. Non-vacuous on any input, no oracle needed. */
static ErrorNumber test_stddev_shift_invariance( void )
{
   enum { N = 400 };
   static double base[N], shifted[N], v0[N], v1[N];
   static const int    periods[] = { 2, 5, 20, 30 };
   static const double offsets[] = { 1.0e6, 1.0e8, 1.0e10 };
   TA_Integer b0, n0, b1, n1;
   int p, c, i, k;

   sd_rng_state = 0x1BADCAFEu;
   for( i = 0; i < N; i++ ) base[i] = 1000.0 * sd_rand();   /* spread ~577, mean ~0 */

   for( p = 0; p < 4; p++ )
   {
      int period = periods[p];
      for( c = 0; c < 3; c++ )
      {
         double off = offsets[c];
         for( i = 0; i < N; i++ ) shifted[i] = base[i] + off;
         TA_VAR( 0, N-1, base,    period, 1.0, &b0, &n0, v0 );
         TA_VAR( 0, N-1, shifted, period, 1.0, &b1, &n1, v1 );
         for( k = 0; k < (int)n0; k++ )
         {
            /* x+c cannot represent x's low bits: each value carries ~ulp(c) ~ c*eps
             * of representation error, so var(x+c) legitimately differs from var(x)
             * by ~c*eps/stddev. Tolerance accounts for that; a genuine offset-
             * sensitive regression (the E[x^2]-mean^2 class) fails by orders more. */
            double sd0 = sqrt( fabs( v0[k] ) );
            double tol = 1.0e-9 + 4.0 * off * 2.2204460492503131e-16
                                / ( sd0 > 1.0e-30 ? sd0 : 1.0e-30 );
            double d = ( v0[k] != 0.0 ) ? fabs( v1[k] - v0[k] ) / fabs( v0[k] )
                                        : fabs( v1[k] );
            if( d > tol )
            {
               printf( "VAR #118 shift-invariance: period=%d off=%g bar=%d "
                       "var(x+c)=%.17g var(x)=%.17g (rel %.3g > %.3g)\n",
                       period, off, k, v1[k], v0[k], d, tol );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}

/* (P5) Metamorphic scale-invariance: var(c*x) == c^2 * var(x). */
static ErrorNumber test_stddev_scale_invariance( void )
{
   enum { N = 300 };
   static double base[N], scaled[N], v0[N], v1[N];
   static const int    periods[] = { 2, 10, 25 };
   static const double scales[]  = { 1.0e3, 1.0e-3, 7.5 };
   TA_Integer b0, n0, b1, n1;
   int p, s, i, k;

   sd_rng_state = 0x5EED1234u;
   for( i = 0; i < N; i++ ) base[i] = 100.0 + 20.0 * sd_rand();

   for( p = 0; p < 3; p++ )
   {
      int period = periods[p];
      for( s = 0; s < 3; s++ )
      {
         double sc = scales[s];
         for( i = 0; i < N; i++ ) scaled[i] = sc * base[i];
         TA_VAR( 0, N-1, base,   period, 1.0, &b0, &n0, v0 );
         TA_VAR( 0, N-1, scaled, period, 1.0, &b1, &n1, v1 );
         for( k = 0; k < (int)n0; k++ )
         {
            double expected = sc * sc * v0[k];
            double d = ( expected != 0.0 ) ? fabs( v1[k] - expected ) / fabs( expected )
                                           : fabs( v1[k] );
            if( d > 1.0e-9 )
            {
               printf( "VAR #118 scale-invariance: period=%d scale=%g bar=%d "
                       "var(c*x)=%.17g c^2*var(x)=%.17g (rel %.3g)\n",
                       period, sc, k, v1[k], expected, d );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}

/* (P4) Non-negativity + exact-constant-zero over adversarial data:
 * high magnitude + low variance, a transient spike, and a mid-series level shift. */
static ErrorNumber test_stddev_nonneg_constant( void )
{
   enum { N = 500 };
   static double x[N], out[N];
   static const int periods[] = { 2, 5, 20, 50 };
   TA_Integer b, nb;
   int p, i, k;

   for( i = 0; i < N; i++ )      x[i] = 1.0e8 + (double)( ( i * 13 ) % 7 - 3 ) * 0.01;
   x[100] = 1.0e12;                                                  /* spike */
   for( i = 250; i < N; i++ )    x[i] = 3.0 + (double)( ( i * 7 ) % 5 - 2 ) * 0.1;   /* level shift */

   for( p = 0; p < 4; p++ )
   {
      int period = periods[p];
      TA_VAR( 0, N-1, x, period, 1.0, &b, &nb, out );
      for( k = 0; k < (int)nb; k++ )
         if( out[k] < 0.0 )
         {
            printf( "VAR #118 non-negativity: period=%d bar=%d val=%.17g\n",
                    period, (int)b + k, out[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      TA_STDDEV( 0, N-1, x, period, 1.0, &b, &nb, out );
      for( k = 0; k < (int)nb; k++ )
         if( out[k] < 0.0 )
         {
            printf( "STDDEV #118 non-negativity: period=%d bar=%d val=%.17g\n",
                    period, (int)b + k, out[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   /* Exactly-constant window -> variance exactly 0 (no rounding residue). */
   for( i = 0; i < N; i++ ) x[i] = 1234567.0;
   for( p = 0; p < 4; p++ )
   {
      int period = periods[p];
      TA_VAR( 0, N-1, x, period, 1.0, &b, &nb, out );
      for( k = 0; k < (int)nb; k++ )
         if( out[k] != 0.0 )
         {
            printf( "VAR #118 constant window not exactly 0: period=%d bar=%d val=%.17g\n",
                    period, (int)b + k, out[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }
   return TA_TEST_PASS;
}

/* (P3) NIST StRD certified values (Statistical Reference Datasets, univariate).
 * NumAcc1-4 are purpose-built cancellation stressors (large mean, tiny variance).
 * They certify the sample stddev (denom n-1); TA-Lib is population (denom n), so
 * the expected population variance is s^2 * (n-1)/n. NumAcc1 is integer-exact;
 * NumAcc2-4 carry decimal-representation error that grows with the offset, so the
 * tolerance is loosened accordingly (still far tighter than the ~1e-3 error the
 * old E[x^2]-mean^2 form produces). Data multisets reproduce the certified
 * variance exactly (order only affects the uncertified autocorrelation).
 * https://www.itl.nist.gov/div898/strd/univ/homepage.html */
static ErrorNumber test_stddev_nist_strd( void )
{
   static double buf[1001];
   TA_Integer b, nb;
   TA_RetCode rc;
   int i, idx;
   double gotVar, gotStd, expVar, expStd, dv, ds;

   struct { const char *name; double base; int n; double expVar; double tol; } cases[4];

   /* NumAcc1: {1e7+1, 1e7+2, 1e7+3}, N=3. pop var = 2/3 exactly (integers). */
   cases[0].name = "NumAcc1"; cases[0].base = 1.0e7; cases[0].n = 3;
   cases[0].expVar = 2.0/3.0;             cases[0].tol = 1.0e-12;
   /* NumAcc2/3/4: 500x(base+1.1), 500x(base+1.3), 1x(base+1.2), N=1001.
    * pop var = 10/1001 for exact 0.1 deviations; decimals cost ~ulp(base)/0.1. */
   cases[1].name = "NumAcc2"; cases[1].base = 0.0;       cases[1].n = 1001;
   cases[1].expVar = 10.0/1001.0;         cases[1].tol = 1.0e-12;
   cases[2].name = "NumAcc3"; cases[2].base = 999999.0;  cases[2].n = 1001;
   cases[2].expVar = 10.0/1001.0;         cases[2].tol = 1.0e-8;
   cases[3].name = "NumAcc4"; cases[3].base = 9999999.0; cases[3].n = 1001;
   cases[3].expVar = 10.0/1001.0;         cases[3].tol = 1.0e-6;

   for( i = 0; i < 4; i++ )
   {
      int n = cases[i].n;
      double base = cases[i].base;
      if( n == 3 )
      {
         buf[0] = base + 1.0; buf[1] = base + 2.0; buf[2] = base + 3.0;
      }
      else
      {
         idx = 0;
         buf[idx++] = base + 1.2;                       /* the single mid value */
         while( idx < n ) { buf[idx++] = base + 1.1; buf[idx++] = base + 1.3; }  /* 500 each */
      }
      rc = TA_VAR( 0, n-1, buf, n, 1.0, &b, &nb, &gotVar );
      if( rc != TA_SUCCESS || nb != 1 )
      {
         printf( "VAR #118 NIST %s: rc=%d nb=%d\n", cases[i].name, (int)rc, (int)nb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      TA_STDDEV( 0, n-1, buf, n, 1.0, &b, &nb, &gotStd );
      expVar = cases[i].expVar;
      expStd = sqrt( expVar );
      dv = fabs( gotVar - expVar ) / expVar;
      ds = fabs( gotStd - expStd ) / expStd;
      if( dv > cases[i].tol || ds > cases[i].tol )
      {
         printf( "VAR #118 NIST %s: var=%.17g (exp %.17g rel %.3g) std=%.17g (exp %.17g rel %.3g) tol=%.0e\n",
                 cases[i].name, gotVar, expVar, dv, gotStd, expStd, ds, cases[i].tol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (P1/P5) Per-window two-pass oracle on pandas' battle-tested adversarial arrays
 * (GH#47721 big-value-transiting-the-window, GH#52407 negative-variance, GH#42064
 * big-then-constant -> exact 0) plus random data at several magnitudes. */
static ErrorNumber test_stddev_pandas_oracle( void )
{
   ErrorNumber e;
   int i, m;

   /* GH#47721: a 1e10 spike enters then leaves a window-6; std must snap back. */
   {
      static const double a[] = { 1,-1,0,1,3,2,-2,10000000000.0,1,2,0,-2,1,3,0,1 };
      e = sd_check_vs_twopass( "GH47721", a, 16, 6, 1.0e-9 );
      if( e != TA_TEST_PASS ) return e;
   }
   /* GH#52407: tiny mixed magnitudes that produced a negative variance. */
   {
      static const double a[] = { 0.0, 0.0, 3.16188252e-18, 2.95781651e-16,
                                  2.23153542e-51, 0.0, 0.0, 5.39943432e-48,
                                  1.38206260e-73, 0.0 };
      e = sd_check_vs_twopass( "GH52407", a, 10, 3, 1.0e-9 );
      if( e != TA_TEST_PASS ) return e;
   }
   /* GH#42064: 1000 zeros with a 1000 spike at index 0; window-10 must be exact 0
    * once the spike leaves. */
   {
      static double a[1000];
      for( i = 0; i < 1000; i++ ) a[i] = 0.0;
      a[0] = 1000.0;
      e = sd_check_vs_twopass( "GH42064", a, 1000, 10, 1.0e-9 );
      if( e != TA_TEST_PASS ) return e;
   }
   /* Random data across magnitudes and periods, refereed against the two-pass. */
   {
      static double a[2000];
      static const double mag[] = { 1.0, 1.0e4, 1.0e8 };
      static const int    per[] = { 2, 5, 14, 50 };
      for( m = 0; m < 3; m++ )
      {
         int p;
         sd_rng_state = 0xC0FFEEu + (unsigned)m;
         for( i = 0; i < 2000; i++ ) a[i] = mag[m] * ( 1.0 + 1.0e-3 * sd_rand() );
         for( p = 0; p < 4; p++ )
         {
            e = sd_check_vs_twopass( "random", a, 2000, per[p], 1.0e-9 );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }
   return TA_TEST_PASS;
}

/**** Global functions definitions.   ****/
ErrorNumber test_func_stddev( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   for( i=0; i < NB_TEST; i++ )
   {
      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "%s Failed Bad Parameter for Test #%d (%d,%d)\n", __FILE__,
                 i, tableTest[i].expectedNbElement, history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "%s Failed Test #%d (Code=%d)\n", __FILE__,
                 i, retValue );
         return retValue;
      }
   }

   /* Cancellation-free variance validation (#118). */
   retValue = test_stddev_pandas_oracle();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed VAR/STDDEV two-pass oracle (#118) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_stddev_nist_strd();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed VAR/STDDEV NIST StRD pins (#118) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_stddev_shift_invariance();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed VAR/STDDEV shift-invariance (#118) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_stddev_scale_invariance();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed VAR/STDDEV scale-invariance (#118) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_stddev_nonneg_constant();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed VAR/STDDEV non-negativity/constant (#118) (Code=%d)\n", __FILE__, retValue ); return retValue; }

   /* All test succeed. */
   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/
static TA_RetCode rangeTestFunction( TA_Integer    startIdx,
                                     TA_Integer    endIdx,
                                     TA_Real      *outputBuffer,
                                     TA_Integer   *outputBufferInt,
                                     TA_Integer   *outBegIdx,
                                     TA_Integer   *outNbElement,
                                     TA_Integer   *lookback,
                                     void         *opaqueData,
                                     unsigned int  outputNb,
                                     unsigned int *isOutputInteger )
{
   TA_RetCode retCode;
   TA_RangeTestParam *testParam;

   (void)outputNb;
   (void)outputBufferInt;

   *isOutputInteger = 0;

   testParam = (TA_RangeTestParam *)opaqueData;

   retCode = TA_STDDEV(
                        startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        testParam->test->optInNbDeviation_1,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );


   *lookback = TA_STDDEV_Lookback( testParam->test->optInTimePeriod,
                       testParam->test->optInNbDeviation_1 );

   return retCode;
}

static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx;
   TA_Integer outNbElement;
   TA_RangeTestParam testParam;

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->close, history->nbBars );
   setInputBuffer( 1, history->close, history->nbBars );

   /* Make a simple first call. */
   retCode = TA_STDDEV(
                        test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInTimePeriod,
                        test->optInNbDeviation_1,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );

   errNb = checkDataSame( gBuffer[0].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   if( server_verify_active() )
   {
      errNb = server_verify("STDDEV", test->startIdx, test->endIdx, history->nbBars,
                            retCode, outBegIdx, outNbElement,
                            (const TA_Real*[]){ gBuffer[0].in, NULL },
                            (double[]){ (double)test->optInTimePeriod,
                                        test->optInNbDeviation_1 }, 2,
                            (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_STDDEV(
                        test->startIdx,
                        test->endIdx,
                        gBuffer[1].in,
                        test->optInTimePeriod,
                        test->optInNbDeviation_1,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[1].in );

   /* The previous call should have the same output as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[1].in, 0 );

   if( errNb != TA_TEST_PASS )
      return errNb;

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {
      errNb = doRangeTest(
                           rangeTestFunction,
                           TA_FUNC_UNST_NONE,
                           (void *)&testParam, 1, 0 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

