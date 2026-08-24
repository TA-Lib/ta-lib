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
#include "ta_test_reference.h"

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
   /* Was 1.075, wrong in the third digit (#188). The table contradicted itself:
    * its own nbDev=1.0 row is 0.7144 and STDDEV scales linearly with nbDev, so
    * this must be 0.7144*1.5. Confirmed independently against released v0.6.4,
    * which gives 0.71435565370949350 at nbDev=1.0 -> 1.0715334805642403 here.
    * The 0.01 window is why it passed for two decades. */
   { 0, 0, 251, 5, 1.5, TA_SUCCESS, 252-5, 1.071533, 4,  252-4 } /* Last Value */
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

/* The RNG and the trusted oracle both moved to ta_test_reference.{h,c} (#251):
 * this file, test_correl.c and test_beta.c had three copies of each, and the
 * oracle accumulated in `long double`, which is 64 mantissa bits here and 53 on
 * MSVC -- so every bound below was quietly weaker on Windows than the number it
 * is written as. ta_test_ref_var() carries ~106 bits on every ABI.
 *
 * The stream is unchanged: ta_test_ref_lcg_sym() is bit-for-bit the local LCG
 * the tolerances in this file were measured against.
 */

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
      double ref = ta_test_ref_var( x, (int)begIdx + k - ( period - 1 ), period, &mean );
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

/* The same comparison, but the reference VALUE is a baked constant rather than
 * anything this binary computed (#251). The runtime oracle is still consulted,
 * for the window's conditioning only -- kappa is a tolerance model, not a
 * correctness claim, so it may share the binary's fate; the number the shipped
 * function is judged against may not.
 *
 * The goldens come from scripts/gen_test_reference.py, which evaluates the
 * population variance of these windows in EXACT RATIONAL arithmetic (every
 * input is a double, so every sum and product is exact) and rounds once. There
 * is no shared code with TA-Lib for it to be co-wrong with -- the #228 trap --
 * and a constant does not change precision when the ABI does. */
static ErrorNumber sd_check_vs_golden( const char *label, const double *x, int n,
                                       int period, const double *golden, int nbGolden,
                                       double baseTol )
{
   static double out[8192];
   TA_Integer begIdx, nbElement;
   TA_RetCode rc;
   int k;

   if( n > 8192 )
   {
      printf( "VAR #251 golden[%s]: series of %d exceeds the output buffer\n", label, n );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( n - period + 1 != nbGolden )
   {
      printf( "VAR #251 golden[%s]: %d windows but %d baked values -- the test and "
              "scripts/gen_test_reference.py disagree about the corpus\n",
              label, n - period + 1, nbGolden );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   rc = TA_VAR( 0, n-1, x, period, 1.0, &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS || (int)nbElement != nbGolden )
   {
      printf( "VAR #251 golden[%s]: rc=%d nb=%d (expected SUCCESS,%d)\n",
              label, (int)rc, (int)nbElement, nbGolden );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( k = 0; k < nbGolden; k++ )
   {
      double mean = 0.0;
      double ref  = golden[k];
      double d, tol, kappa;

      if( out[k] < 0.0 )
      {
         printf( "VAR #251 golden[%s]: NEGATIVE var period=%d bar=%d val=%.17g\n",
                 label, period, (int)begIdx + k, out[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( ref == 0.0 )
      {
         if( out[k] != 0.0 )
         {
            printf( "VAR #251 golden[%s]: expected exact 0 period=%d bar=%d val=%.17g\n",
                    label, period, (int)begIdx + k, out[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         continue;
      }
      ta_test_ref_var( x, (int)begIdx + k - ( period - 1 ), period, &mean );
      kappa = fabs( mean ) / sqrt( ref );
      tol   = baseTol + 100.0 * kappa * 2.2204460492503131e-16;
      d = fabs( out[k] - ref ) / fabs( ref );
      if( d > tol )
      {
         printf( "VAR #251 golden[%s]: period=%d bar=%d val=%.17g golden=%.17g "
                 "(rel %.3g > %.3g, kappa %.2g)\n",
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

   ta_test_ref_lcg_seed( 0x1BADCAFEu );
   for( i = 0; i < N; i++ ) base[i] = 1000.0 * ta_test_ref_lcg_sym();   /* spread ~577, mean ~0 */

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

   ta_test_ref_lcg_seed( 0x5EED1234u );
   for( i = 0; i < N; i++ ) base[i] = 100.0 + 20.0 * ta_test_ref_lcg_sym();

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
   int i;
   double gotVar, gotStd, expVar, expStd, dv, ds;

   struct { const char *name; int n; double expVar; double tol; } cases[4];

   /* The data lives in ta_test_ref_numacc(); what stays here is the certified
    * answer and the tolerance each base earns.
    * NumAcc1: {1e7+1, 1e7+2, 1e7+3}, N=3. pop var = 2/3 exactly (integers). */
   cases[0].name = "NumAcc1"; cases[0].n = 3;
   cases[0].expVar = 2.0/3.0;             cases[0].tol = 1.0e-12;
   /* NumAcc2/3/4: 500x(base+1.1), 500x(base+1.3), 1x(base+1.2), N=1001.
    * pop var = 10/1001 for exact 0.1 deviations; decimals cost ~ulp(base)/0.1. */
   cases[1].name = "NumAcc2"; cases[1].n = 1001;
   cases[1].expVar = 10.0/1001.0;         cases[1].tol = 1.0e-12;
   cases[2].name = "NumAcc3"; cases[2].n = 1001;
   cases[2].expVar = 10.0/1001.0;         cases[2].tol = 1.0e-8;
   cases[3].name = "NumAcc4"; cases[3].n = 1001;
   cases[3].expVar = 10.0/1001.0;         cases[3].tol = 1.0e-6;

   for( i = 0; i < 4; i++ )
   {
      /* The series themselves come from the shared battery (#251); this file
       * only keeps the certified variances and the tolerance each one earns. */
      int n = ta_test_ref_numacc( i+1, buf );
      if( n != cases[i].n )
      {
         printf( "VAR #118 NIST %s: shared battery returned %d values, expected %d\n",
                 cases[i].name, n, cases[i].n );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
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

   /* GH#47721: a 1e10 spike enters then leaves a window-6; std must snap back.
    * GH#52407: tiny mixed magnitudes that produced a NEGATIVE variance.
    * Both arrays and both reference tables now come from the shared battery. */
   e = sd_check_vs_golden( "GH47721", ta_test_ref_pd_var47721, TA_TEST_REF_PD_VAR47721_N,
                           6, ta_test_ref_golden_var47721,
                           TA_TEST_REF_GOLDEN_VAR47721_N, 1.0e-9 );
   if( e != TA_TEST_PASS ) return e;
   e = sd_check_vs_golden( "GH52407", ta_test_ref_pd_var52407, TA_TEST_REF_PD_VAR52407_N,
                           3, ta_test_ref_golden_var52407,
                           TA_TEST_REF_GOLDEN_VAR52407_N, 1.0e-9 );
   if( e != TA_TEST_PASS ) return e;
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
         ta_test_ref_lcg_seed( 0xC0FFEEu + (unsigned)m );
         for( i = 0; i < 2000; i++ ) a[i] = mag[m] * ( 1.0 + 1.0e-3 * ta_test_ref_lcg_sym() );
         for( p = 0; p < 4; p++ )
         {
            e = sd_check_vs_twopass( "random", a, 2000, per[p], 1.0e-9 );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }
   return TA_TEST_PASS;
}

/* ============================================================================
 * Issue #243: a small but plainly non-zero standard deviation must not collapse
 * to exactly 0.
 *
 * STDDEV squares its answer away. It square-roots TA_VAR's output, and a variance
 * is a SQUARED quantity: a series quoted in 1e-8 ticks has a variance around
 * 1e-16, five orders under the fixed TA_EPSILON (1e-14) band that used to be
 * applied to it. So a $100.00 instrument on a 1e-8 tick returned exactly 0 for
 * every bar -- and BBANDS, which inherits the deviation, put all three bands on
 * top of each other -- while TA_VAR, cancellation-free since #118, had the right
 * answer all along. A dead-zone on a squared quantity has to be RELATIVE to the
 * window's own scale; a fixed absolute one is a cliff at a price level.
 *
 * The gap this closes: every #118 oracle referees TA_VAR. STDDEV appears only in
 * a non-negativity sweep (vacuous while the clamp exists -- it is what forces the
 * sign) and in the NIST pins, whose variance is ~1e-2. No test ever put STDDEV on
 * a window below the clamp, and the scale ladder in test_stddev_scale_invariance
 * stops at 1e-3 -- four decades short of the cliff, and VAR-only besides.
 * ==========================================================================*/

/* The issue's series: 60 integer ticks, walked down five decades past the cliff
 * at two price levels (the spread shrinking with the level, and the spread
 * shrinking alone), refereed bar by bar against the shared double-double two-pass sigma.
 *
 * Tolerance. The deviations x-mean are computed exactly (Sterbenz), so the
 * shifted-data form tracks the two-pass to ~1e-13 until the DATA quantizes: once
 * kappa*eps = ulp(mean)/sigma approaches 1 the stored doubles no longer resolve
 * the spread, and the measured error there follows ~2e-8*(kappa*eps)^2. The bound
 * below keeps >=62x margin over that. It stays >=6 orders under the relative-1.0
 * error a collapse produces at every rung (max kappa*eps on the ladder that ships
 * is 0.45, so the bound never exceeds 2.0e-7): no cell is vacuous. Those three
 * figures are measured on the 12-decade ladder BELOW; an earlier draft quoted
 * ~5 / ~2.5e-5 / 70x, which describe a 13-decade version that was never
 * committed. The shipped bound is tighter than the prose used to claim.
 */
static const double sd243_bases[2]  = { 0.0, 100.0 };
static const int    sd243_periods[4] = { 2, 5, 20, 30 };

static double sd243_tol( double kappa )
{
   const double ke = kappa * 2.2204460492503131e-16;
   return 1.0e-11 + 1.0e-6 * ke * ke;
}

static ErrorNumber test_stddev_small_scale( void )
{
   enum { N = 60 };
   static double x[N], sd[N];
   TA_Integer b, nb;
   TA_RetCode rc;
   int bi, pi, i, k, decade;

   for( bi = 0; bi < 2; bi++ )
   for( pi = 0; pi < 4; pi++ )
   {
      const int period = sd243_periods[pi];
      double tick = 1.0e-2;
      for( decade = 0; decade < 12; decade++, tick /= 10.0 )
      {
         for( i = 0; i < N; i++ ) x[i] = sd243_bases[bi] + (double)ta_test_ref_ticks60[i] * tick;

         rc = TA_STDDEV( 0, N-1, x, period, 1.0, &b, &nb, sd );
         if( rc != TA_SUCCESS )
         {
            printf( "STDDEV #243: rc=%d base=%g tick=%g period=%d\n",
                    (int)rc, sd243_bases[bi], tick, period );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( k = 0; k < (int)nb; k++ )
         {
            double mean = 0.0;
            double refVar = ta_test_ref_var( x, (int)b + k - ( period - 1 ), period, &mean );
            double refSig, kappa, tol, d;
            if( refVar == 0.0 )
            {
               if( sd[k] != 0.0 )
               {
                  printf( "STDDEV #243: expected exact 0 base=%g tick=%g period=%d bar=%d val=%.17g\n",
                          sd243_bases[bi], tick, period, (int)b + k, sd[k] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               continue;
            }
            refSig = sqrt( refVar );
            if( sd[k] == 0.0 )
            {
               printf( "STDDEV #243: COLLAPSED to 0 base=%g tick=%g period=%d bar=%d "
                       "(two-pass sigma %.17g)\n",
                       sd243_bases[bi], tick, period, (int)b + k, refSig );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            kappa = fabs( mean ) / refSig;
            tol   = sd243_tol( kappa );
            d     = fabs( sd[k] - refSig ) / refSig;
            if( d > tol )
            {
               printf( "STDDEV #243: base=%g tick=%g period=%d bar=%d val=%.17g "
                       "ref=%.17g (rel %.3g > %.3g, kappa %.2g)\n",
                       sd243_bases[bi], tick, period, (int)b + k, sd[k], refSig,
                       d, tol, kappa );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}

/* The other half of #243: removing the absolute dead-zone at the square root must
 * not turn a genuinely flat stretch into band noise.
 *
 * A window sitting wholly inside a flat region ENTERED MID-SERIES is not the same
 * case as an all-constant input. The reseed re-anchors on the window MEAN, which
 * is itself only correct to an ulp, so every deviation is that same ulp and the
 * corrected two-pass differences two equal quantities -- leaving a residue of
 * about eps^3*price^2 (~2e-44 at $100). The absolute clamp swallowed it below the
 * 1e-14 line and nowhere else; the scale-relative floor that replaces it swallows
 * it at every price level, because it asks whether the fresh anchor resolved any
 * spread at all rather than comparing a squared quantity to a constant.
 *
 * Un-fixed this is red on TA_VAR (~2e-44, not 0) and green on TA_STDDEV for the
 * wrong reason -- the clamp is what zeroes it.
 */
static ErrorNumber test_stddev_flat_tail_exact_zero( void )
{
   enum { N = 600, NOISY = 100 };
   static double x[N], out[N];
   /* 1e11 is not decoration. The small-scale ladder above bottoms out at a
    * non-zero reference variance of 2.47e-27, so on its own it is satisfied by
    * ANY per-bar absolute cliff below that -- "just make TA_EPSILON smaller"
    * passes it, which is the exact defect class this file exists to reject. At
    * 1e11/period 49 the residue this test requires to be bit-zero is 2.59e-26,
    * ABOVE that floor, so no single absolute constant can satisfy both tests at
    * once. They are only jointly satisfiable by a scale-relative floor. */
   static const double levels[5] = { 100.0, 1234.56789, 1.0e8, 1.0e-6, 1.0e11 };
   static const int    periods[4] = { 2, 5, 20, 49 };
   TA_Integer b, nb;
   int li, pi, i, k, firstFlatWindow;

   for( li = 0; li < 5; li++ )
   {
      const double lvl = levels[li];
      ta_test_ref_lcg_seed( 0xF1A77A11u + (unsigned)li );
      for( i = 0; i < N; i++ )
         x[i] = ( i < NOISY ) ? lvl * ( 1.0 + 0.05 * ta_test_ref_lcg_sym() ) : lvl;

      for( pi = 0; pi < 4; pi++ )
      {
         const int period = periods[pi];
         int f;
         /* First bar whose whole window is inside the flat tail. */
         firstFlatWindow = NOISY + period - 1;

         for( f = 0; f < 2; f++ )
         {
            const char *name = f ? "STDDEV" : "VAR";
            TA_RetCode rc = f ? TA_STDDEV( 0, N-1, x, period, 1.0, &b, &nb, out )
                              : TA_VAR   ( 0, N-1, x, period, 1.0, &b, &nb, out );
            if( rc != TA_SUCCESS )
            {
               printf( "%s #243 flat tail: rc=%d level=%g period=%d\n",
                       name, (int)rc, lvl, period );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            for( k = 0; k < (int)nb; k++ )
            {
               if( (int)b + k < firstFlatWindow ) continue;
               if( out[k] != 0.0 )
               {
                  printf( "%s #243 flat tail: level=%g period=%d bar=%d val=%.17g "
                          "(window is exactly constant, expected bit-zero)\n",
                          name, lvl, period, (int)b + k, out[k] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
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

   /* Scale-relative dead-zone (#243). */
   retValue = test_stddev_small_scale();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed STDDEV small-scale ladder (#243) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_stddev_flat_tail_exact_zero();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed VAR/STDDEV flat-tail exact zero (#243) (Code=%d)\n", __FILE__, retValue ); return retValue; }

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
                           TA_TEST_UNST_NONE,
                           (void *)&testParam, 1, 0 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

