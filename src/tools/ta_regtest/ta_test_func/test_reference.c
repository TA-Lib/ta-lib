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
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  082326 MF,CC Initial coding (#251).
 *
 */

/* Description:
 *     Self-check for the shared numerical-reference battery.
 *
 *     Nothing here tests a TA function. It tests the referee -- and every other
 *     numerical suite in ta_regtest now leans on it, so a silent change to it
 *     would move bounds in five files at once without any of them going red.
 *
 *     Three things are pinned:
 *
 *     (1) THE ORACLE AGAINST THE BAKED GOLDENS. ta_test_reference.c accumulates
 *         in compensated double-double; scripts/gen_test_reference.py computes
 *         the same quantities in exact rational arithmetic, in another language,
 *         and bakes them as constants. Requiring the two to agree to 1e-15 is
 *         what keeps each honest: it catches a corrupted or stale golden table,
 *         and it catches a defect in the double-double primitives -- including
 *         the one failure mode the constants exist to prevent, an ABI where the
 *         oracle silently degrades to plain double.
 *
 *         Which way does the argument run? The GOLDEN is the reference wherever
 *         a test can use one; the oracle is for the LCG-driven sweeps that
 *         generate far more windows than it would be sensible to bake. This leg
 *         is what lets those sweeps borrow the goldens' credibility.
 *
 *     (2) THE DATASETS. NIST certifies Norris' fit of the exact decimals; what
 *         this library is handed is those decimals rounded to double. The two
 *         must agree to ~1e-14 -- if they did not, the transcription would be
 *         wrong, and it is a transcription of 72 numbers.
 *
 *     (3) THE RANDOM NUMBER GENERATOR. #251 replaced three per-file generators
 *         with one. Every measured tolerance in test_stddev.c, test_beta.c and
 *         test_linearreg.c was measured on the exact sequences those three
 *         produced, so the consolidation is only safe while the sequences are
 *         unchanged. These pins are what make that a checked claim rather than
 *         a remembered one.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_test_reference.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* The oracle is a correctly-rounded double of a value the goldens carry to the
 * same precision, so anything above a few ulp is a defect, not noise. */
#define REF_SELFCHECK_TOL 1.0e-15

/* How many comparisons this file MUST make, derived from the generated table
 * sizes rather than written down. A hand-maintained round number gets weaker
 * every time a table is added -- the first draft said "at least 600" against a
 * real 680, which is enough slack to delete every correlation and variance
 * golden and still pass. */
#define REF_EXPECTED_GOLDEN ( \
   TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF0_N + \
   TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF1E13_N + \
   2 * TA_TEST_REF_GOLDEN_CORR_SHARED_1E10_N + \
   TA_TEST_REF_GOLDEN_CORR_NONAN_N + \
   TA_TEST_REF_GOLDEN_CORR_EXTREME_N + \
   TA_TEST_REF_GOLDEN_VAR47721_N + \
   TA_TEST_REF_GOLDEN_VAR52407_N + \
   4 * TA_TEST_REF_WILKINSON_NB_SERIES + \
   5 * ( TA_TEST_REF_GOLDEN_LADDER_P2_SLOPE_N + \
         TA_TEST_REF_GOLDEN_LADDER_P5_SLOPE_N + \
         TA_TEST_REF_GOLDEN_LADDER_P14_SLOPE_N + \
         TA_TEST_REF_GOLDEN_LADDER_P30_SLOPE_N ) )
/* Norris (2) + NumAcc (4) + the constant-window contract (4) + the RNG pins
 * (12 draws + 1 reseed). */
#define REF_EXPECTED_OTHER 23
#define REF_EXPECTED_TOTAL ( REF_EXPECTED_GOLDEN + REF_EXPECTED_OTHER )

/**** Local functions declarations.    ****/
static ErrorNumber test_reference_goldens( void );
static ErrorNumber test_reference_datasets( void );
static ErrorNumber test_reference_rng( void );

/**** Local variables definitions.     ****/
static int ref_comparisons;   /* anti-vacuity: every check below bumps it */

/**** Global functions definitions.   ****/
ErrorNumber test_func_reference( TA_History *history )
{
   ErrorNumber retValue;

   (void)history;

   ref_comparisons = 0;

   retValue = test_reference_goldens();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed oracle-vs-golden self-check (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_reference_datasets();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed dataset self-check (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_reference_rng();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed RNG stream pins (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }

   /* "No failure reported" and "nothing was ever compared" are otherwise the
    * same observation. EXACT, not a floor: every term is a generated table size,
    * so adding a table moves both sides together and dropping one moves only the
    * left. */
   if( ref_comparisons != REF_EXPECTED_TOTAL )
   {
      printf( "%s Reference self-check made %d comparisons, expected exactly %d "
              "(%d golden + %d dataset/RNG). A table stopped being enumerated, or "
              "one was added without updating REF_EXPECTED_OTHER.\n",
              __FILE__, ref_comparisons, (int)REF_EXPECTED_TOTAL,
              (int)REF_EXPECTED_GOLDEN, (int)REF_EXPECTED_OTHER );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   printf( "\n  Reference battery (#251): %d checks (baked goldens, datasets, RNG streams)\n",
           ref_comparisons );

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

static ErrorNumber ref_cmp( const char *what, int idx, double got, double want )
{
   double d = ( want != 0.0 ) ? fabs( got - want ) / fabs( want ) : fabs( got - want );
   ref_comparisons++;
   if( d > REF_SELFCHECK_TOL )
   {
      printf( "REFERENCE #251 %s[%d]: oracle=%.17g golden=%.17g (rel %.3g > %.3g)\n",
              what, idx, got, want, d, REF_SELFCHECK_TOL );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (1) Every baked value, recomputed by the double-double oracle. */
static ErrorNumber test_reference_goldens( void )
{
   static double shifted[TA_TEST_REF_PD_OUTLIER_N];
   static double sx[TA_TEST_REF_PD_SHARED_N], sy[TA_TEST_REF_PD_SHARED_N];
   ErrorNumber e;
   int i, k, t;

   /* --- correlation ------------------------------------------------------ */
   for( k = 0; k < TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF0_N; k++ )
   {
      e = ref_cmp( "corr outlier off0", k,
                   ta_test_ref_corr( ta_test_ref_pd_outlier_x, ta_test_ref_pd_outlier_y, k, 9 ),
                   ta_test_ref_golden_corr_outlier_off0[k] );
      if( e != TA_TEST_PASS ) return e;
   }
   for( i = 0; i < TA_TEST_REF_PD_OUTLIER_N; i++ )
      shifted[i] = ta_test_ref_pd_outlier_y[i] + 1.0e13;
   for( k = 0; k < TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF1E13_N; k++ )
   {
      e = ref_cmp( "corr outlier off1e13", k,
                   ta_test_ref_corr( ta_test_ref_pd_outlier_x, shifted, k, 9 ),
                   ta_test_ref_golden_corr_outlier_off1e13[k] );
      if( e != TA_TEST_PASS ) return e;
   }
   {
      static const double offs[2] = { 1.0e10, 1.0e14 };
      static const double *const gold[2] = { ta_test_ref_golden_corr_shared_1e10,
                                             ta_test_ref_golden_corr_shared_1e14 };
      for( t = 0; t < 2; t++ )
      {
         for( i = 0; i < TA_TEST_REF_PD_SHARED_N; i++ )
         {
            sx[i] = ta_test_ref_pd_shared_x[i] + offs[t];
            sy[i] = ta_test_ref_pd_shared_y[i] + offs[t];
         }
         for( k = 0; k < TA_TEST_REF_GOLDEN_CORR_SHARED_1E10_N; k++ )
         {
            e = ref_cmp( "corr shared_offset", k, ta_test_ref_corr( sx, sy, k, 5 ), gold[t][k] );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }
   for( k = 0; k < TA_TEST_REF_GOLDEN_CORR_NONAN_N; k++ )
   {
      e = ref_cmp( "corr nonan", k,
                   ta_test_ref_corr( ta_test_ref_pd_nonan_x, ta_test_ref_pd_nonan_y, k, 3 ),
                   ta_test_ref_golden_corr_nonan[k] );
      if( e != TA_TEST_PASS ) return e;
   }
   for( k = 0; k < TA_TEST_REF_GOLDEN_CORR_EXTREME_N; k++ )
   {
      e = ref_cmp( "corr extreme_range", k,
                   ta_test_ref_corr( ta_test_ref_pd_extreme_x, ta_test_ref_pd_extreme_y, k, 5 ),
                   ta_test_ref_golden_corr_extreme[k] );
      if( e != TA_TEST_PASS ) return e;
   }

   /* --- variance --------------------------------------------------------- */
   for( k = 0; k < TA_TEST_REF_GOLDEN_VAR47721_N; k++ )
   {
      e = ref_cmp( "var GH47721", k, ta_test_ref_var( ta_test_ref_pd_var47721, k, 6, NULL ),
                   ta_test_ref_golden_var47721[k] );
      if( e != TA_TEST_PASS ) return e;
   }
   for( k = 0; k < TA_TEST_REF_GOLDEN_VAR52407_N; k++ )
   {
      e = ref_cmp( "var GH52407", k, ta_test_ref_var( ta_test_ref_pd_var52407, k, 3, NULL ),
                   ta_test_ref_golden_var52407[k] );
      if( e != TA_TEST_PASS ) return e;
   }

   /* --- linear regression: Wilkinson ------------------------------------- */
   for( i = 0; i < TA_TEST_REF_WILKINSON_NB_SERIES; i++ )
   {
      double sl, ic, ft, ts;
      ta_test_ref_linreg( ta_test_ref_wilkinson_series[i], 0, TA_TEST_REF_WILKINSON_N,
                          &sl, &ic, &ft, &ts );
      e = ref_cmp( "wilkinson slope",     i, sl, ta_test_ref_golden_wilkinson_slope[i] );
      if( e != TA_TEST_PASS ) return e;
      e = ref_cmp( "wilkinson intercept", i, ic, ta_test_ref_golden_wilkinson_intercept[i] );
      if( e != TA_TEST_PASS ) return e;
      e = ref_cmp( "wilkinson fit",       i, ft, ta_test_ref_golden_wilkinson_fit[i] );
      if( e != TA_TEST_PASS ) return e;
      e = ref_cmp( "wilkinson tsf",       i, ts, ta_test_ref_golden_wilkinson_tsf[i] );
      if( e != TA_TEST_PASS ) return e;
   }

   /* --- linear regression and sigma: the ladder -------------------------- */
   {
      static const double *const gSlope[4] = {
         ta_test_ref_golden_ladder_p2_slope,  ta_test_ref_golden_ladder_p5_slope,
         ta_test_ref_golden_ladder_p14_slope, ta_test_ref_golden_ladder_p30_slope };
      static const double *const gIcept[4] = {
         ta_test_ref_golden_ladder_p2_intercept,  ta_test_ref_golden_ladder_p5_intercept,
         ta_test_ref_golden_ladder_p14_intercept, ta_test_ref_golden_ladder_p30_intercept };
      static const double *const gFit[4] = {
         ta_test_ref_golden_ladder_p2_fit,  ta_test_ref_golden_ladder_p5_fit,
         ta_test_ref_golden_ladder_p14_fit, ta_test_ref_golden_ladder_p30_fit };
      static const double *const gTsf[4] = {
         ta_test_ref_golden_ladder_p2_tsf,  ta_test_ref_golden_ladder_p5_tsf,
         ta_test_ref_golden_ladder_p14_tsf, ta_test_ref_golden_ladder_p30_tsf };
      static const double *const gSig[4] = {
         ta_test_ref_golden_ladder_p2_sigma,  ta_test_ref_golden_ladder_p5_sigma,
         ta_test_ref_golden_ladder_p14_sigma, ta_test_ref_golden_ladder_p30_sigma };

      for( t = 0; t < TA_TEST_REF_GOLDEN_LADDER_PERIODS_N; t++ )
      {
         const int period = ta_test_ref_golden_ladder_periods[t];
         if( ta_test_ref_golden_ladder_counts[t] != TA_TEST_REF_LADDER_N - period + 1 )
         {
            printf( "REFERENCE #251: ladder period %d claims %d windows, the series "
                    "gives %d\n", period, ta_test_ref_golden_ladder_counts[t],
                    TA_TEST_REF_LADDER_N - period + 1 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( k = 0; k < ta_test_ref_golden_ladder_counts[t]; k++ )
         {
            double sl, ic, ft, ts;
            ta_test_ref_linreg( ta_test_ref_ladder, k, period, &sl, &ic, &ft, &ts );
            e = ref_cmp( "ladder slope",     k, sl, gSlope[t][k] ); if( e != TA_TEST_PASS ) return e;
            e = ref_cmp( "ladder intercept", k, ic, gIcept[t][k] ); if( e != TA_TEST_PASS ) return e;
            e = ref_cmp( "ladder fit",       k, ft, gFit[t][k] );   if( e != TA_TEST_PASS ) return e;
            e = ref_cmp( "ladder tsf",       k, ts, gTsf[t][k] );   if( e != TA_TEST_PASS ) return e;
            e = ref_cmp( "ladder sigma",     k,
                         ta_test_ref_stddev( ta_test_ref_ladder, k, period, NULL ),
                         gSig[t][k] );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }
   return TA_TEST_PASS;
}

/* (2) The transcribed datasets, checked against what certifies them. */
static ErrorNumber test_reference_datasets( void )
{
   double r, b1, kappa;
   double var, mean = 0.0;
   int i, n;
   static double buf[1001];

   /* NIST certifies the fit of the exact decimals; the goldens are the exact
    * fit of those decimals ROUNDED to double. A transcription error would move
    * them apart by far more than a rounding. */
   if( fabs( TA_TEST_REF_GOLDEN_NORRIS_R - TA_TEST_REF_NORRIS_R ) > 1.0e-14 ||
       fabs( TA_TEST_REF_GOLDEN_NORRIS_B1 - TA_TEST_REF_NORRIS_B1 ) > 1.0e-13 ||
       fabs( TA_TEST_REF_GOLDEN_NORRIS_B0 - TA_TEST_REF_NORRIS_B0 ) > 1.0e-13 )
   {
      printf( "REFERENCE #251 Norris: exact-for-these-doubles (%.17g, %.17g, %.17g) "
              "does not match the NIST certificate (%.17g, %.17g, %.17g)\n",
              TA_TEST_REF_GOLDEN_NORRIS_R, TA_TEST_REF_GOLDEN_NORRIS_B1,
              TA_TEST_REF_GOLDEN_NORRIS_B0, TA_TEST_REF_NORRIS_R,
              TA_TEST_REF_NORRIS_B1, TA_TEST_REF_NORRIS_B0 );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   /* And the oracle must reproduce them from the arrays themselves, which is
    * what ties the certificate to the 72 numbers actually stored. */
   r  = ta_test_ref_corr( ta_test_ref_norris_x, ta_test_ref_norris_y, 0, TA_TEST_REF_NORRIS_N );
   b1 = ta_test_ref_slope( ta_test_ref_norris_x, ta_test_ref_norris_y, 0,
                           TA_TEST_REF_NORRIS_N, &kappa );
   if( fabs( r - TA_TEST_REF_GOLDEN_NORRIS_R ) > 1.0e-15 ||
       fabs( b1 - TA_TEST_REF_GOLDEN_NORRIS_B1 ) > 1.0e-15 )
   {
      printf( "REFERENCE #251 Norris: oracle r=%.17g b1=%.17g vs golden %.17g %.17g\n",
              r, b1, TA_TEST_REF_GOLDEN_NORRIS_R, TA_TEST_REF_GOLDEN_NORRIS_B1 );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   /* B0 is not recomputed here: the oracle exports a slope and a correlation
    * for an arbitrary-x regression, not an intercept, and the certificate
    * comparison above already covers it. */
   ref_comparisons += 2;

   /* NIST StRD NumAcc1..4: certified population variance, from the built
    * series rather than a transcribed one, so this checks the builder too. */
   {
      static const double expVar[4] = { 2.0/3.0, 10.0/1001.0, 10.0/1001.0, 10.0/1001.0 };
      static const double tol[4]    = { 1.0e-15, 1.0e-15, 1.0e-9, 1.0e-7 };
      for( i = 0; i < 4; i++ )
      {
         n = ta_test_ref_numacc( i+1, buf );
         if( n != ( i == 0 ? 3 : 1001 ) )
         {
            printf( "REFERENCE #251 NumAcc%d: built %d values\n", i+1, n );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         var = ta_test_ref_var( buf, 0, n, &mean );
         ref_comparisons++;
         if( fabs( var - expVar[i] ) / expVar[i] > tol[i] )
         {
            printf( "REFERENCE #251 NumAcc%d: oracle var=%.17g certified %.17g (rel %.3g > %.3g)"
                    " -- the tolerance is the DATA's decimal-representation error, not the "
                    "oracle's\n",
                    i+1, var, expVar[i], fabs( var - expVar[i] ) / expVar[i], tol[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* A window of bit-identical values must give exactly zero, not a residue --
    * several suites assert that contract of the SHIPPED functions, so the
    * referee has to hold it too. */
   for( i = 0; i < 8; i++ ) buf[i] = 1.0e11 + 0.0;
   if( ta_test_ref_var( buf, 0, 8, NULL ) != 0.0 ||
       ta_test_ref_stddev( buf, 0, 8, NULL ) != 0.0 ||
       ta_test_ref_corr( buf, buf, 0, 8 ) != 0.0 ||
       ta_test_ref_slope( buf, buf, 0, 8, NULL ) != 0.0 )
   {
      printf( "REFERENCE #251: a constant window did not give a bit-zero from every oracle\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   ref_comparisons += 4;

   return TA_TEST_PASS;
}

/* (3) The consolidated generator must emit exactly the streams the three it
 * replaced emitted. Every tolerance in the suites that use it was measured on
 * these sequences. */
static ErrorNumber test_reference_rng( void )
{
   static const double lcgSym[4]  = { -0.5367124080657959, 0.33585023880004883,
                                       0.93741476535797119, 0.71201968193054199 };
   static const double lcgHalf[4] = { 0.29852801561355591, -0.35036760568618774,
                                      -0.33416271209716797, 0.21563214063644409 };
   static const double xor32[4]   = { 0.12227054245822842, 0.85467307545586324,
                                      0.032608529793240255, 0.63552300297550923 };
   double v = 0.0;
   int i;

   /* The first three draws, then the 1000th: a change to the mapping shows in
    * the first, a change to the state update shows by the 1000th. */
   ta_test_ref_lcg_seed( 0x1BADCAFEu );
   for( i = 0; i < 1000; i++ )
   {
      v = ta_test_ref_lcg_sym();
      if( ( i < 3 && v != lcgSym[i] ) || ( i == 999 && v != lcgSym[3] ) )
      {
         printf( "REFERENCE #251 RNG: lcg_sym draw %d = %.17g, expected %.17g -- the shared "
                 "generator no longer produces the sequence every measured tolerance in "
                 "test_stddev.c and test_linearreg.c was measured on\n",
                 i, v, i < 3 ? lcgSym[i] : lcgSym[3] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( !( v >= -1.0 && v < 1.0 ) )
      {
         printf( "REFERENCE #251 RNG: lcg_sym draw %d = %.17g is outside [-1, 1)\n", i, v );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   ta_test_ref_lcg_seed( 7u );
   for( i = 0; i < 1000; i++ )
   {
      v = ta_test_ref_lcg_half();
      if( ( i < 3 && v != lcgHalf[i] ) || ( i == 999 && v != lcgHalf[3] ) )
      {
         printf( "REFERENCE #251 RNG: lcg_half draw %d = %.17g, expected %.17g\n",
                 i, v, i < 3 ? lcgHalf[i] : lcgHalf[3] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( !( v >= -0.5 && v < 0.5 ) )
      {
         printf( "REFERENCE #251 RNG: lcg_half draw %d = %.17g is outside [-0.5, 0.5)\n", i, v );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   ta_test_ref_xorshift_seed( 2463534242u );
   for( i = 0; i < 1000; i++ )
   {
      v = ta_test_ref_xorshift_unit();
      if( ( i < 3 && v != xor32[i] ) || ( i == 999 && v != xor32[3] ) )
      {
         printf( "REFERENCE #251 RNG: xorshift draw %d = %.17g, expected %.17g\n",
                 i, v, i < 3 ? xor32[i] : xor32[3] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( !( v >= 0.0 && v <= 1.0 ) )
      {
         printf( "REFERENCE #251 RNG: xorshift draw %d = %.17g is outside [0, 1]\n", i, v );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   ref_comparisons += 12;

   /* Re-seeding must reset the state: the two mappings share it, and they are
    * only safe to share because every caller re-seeds before its block. */
   ta_test_ref_lcg_seed( 0x1BADCAFEu );
   if( ta_test_ref_lcg_sym() != lcgSym[0] )
   {
      printf( "REFERENCE #251 RNG: re-seeding did not reset the shared LCG state\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   ref_comparisons++;

   return TA_TEST_PASS;
}
