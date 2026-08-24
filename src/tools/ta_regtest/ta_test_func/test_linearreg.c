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
 *     Numerical-reference tests for the TA_LINEARREG family and TA_TSF.
 *
 *     Before this file these five had no DEDICATED reference file, which is not
 *     the same as being uncovered, and an earlier draft of this comment claimed
 *     the stronger thing. What already existed: ta_test_legacy.c (frozen v0.6.4
 *     agreement -- same lineage, so it proves agreement, not correctness); the
 *     cross-language gates (the four backends agree with the C); the --codegen
 *     leg's range test, which classes all five EPSILON via
 *     test_codegen.c stability_class(); and test_period_boundary.c's
 *     testLinearRegRampOverflowProbe (#142), which checks all five against a
 *     closed form at period 1025. THIS FILE DOES NOT SUBSUME THAT LAST ONE --
 *     no leg here exceeds period 60, and its teeth are the UBSan nightly.
 *
 *     What was missing is what #251 adds: external reference datasets, and
 *     arbitrary-value goldens computed outside the binary. Issue #103 rebuilt
 *     these functions on O(1) sliding sums, the same class of change as #118
 *     (VAR) and #242 (CORREL/BETA), and both of those turned out to be hiding a
 *     cancellation defect.
 *
 *     A NOTE ON WHAT NIST StRD CAN AND CANNOT PIN HERE
 *
 *     Issue #251 proposed refereeing this family with NIST StRD Norris, whose
 *     certified B0/B1 are exactly an intercept and a slope. It does not fit:
 *     Norris regresses y on an arbitrary x, and these functions take ONE input
 *     array and regress it on the BAR INDEX. There is no second input to hand
 *     Norris' x to. (Norris stays where it already is, refereeing TA_CORREL and
 *     TA_BETA, which do take two series.)
 *
 *     What fits perfectly instead is Wilkinson's "nasty.dat", the other set
 *     already in the shared battery: its X column is 1..9, equally spaced, so
 *     "regress each column on X" -- Wilkinson's own task IV.B -- is precisely
 *     what these functions compute at period 9, on RAW values with no
 *     price-to-return round trip. BIG and LITTLE are the cancellation
 *     stressors, and LITTLE is the one that catches this library (L1 below).
 *
 *     The legs:
 *
 *       L1  Wilkinson nasty.dat vs baked exact-rational goldens
 *       L2  the exact affine identity y = a*i + b, where all five answers are
 *           closed forms and no oracle is involved at all
 *       L3  the sliding-sum ladder vs baked goldens, at four periods
 *       L4  NIST StRD NumAcc3/NumAcc4, the univariate cancellation stressors
 *       L5  metamorphic shift / scale / time-reversal laws
 *       L6  an exactly constant window
 *       L7  the four outputs' internal consistency
 *       L8  random walks at four magnitudes, refereed by the shared oracle
 *       L9  range stability: the same bar computed with and without history
 *
 *     TWO TOLERANCE TERMS, AND WHAT EACH ONE SAYS ABOUT THE FUNCTIONS
 *
 *     Every leg uses the same bound, and both of its terms are derived from the
 *     arithmetic rather than fitted to the output:
 *
 *       slope   |err| <= eps * ( C_SLOPE*scaleNow + C_DRIFT*scaleHist*bars^1.5/period )
 *       values  |err| <= eps * ( period*C_VALUE*scaleNow + C_DRIFT*scaleHist*bars^1.5 )
 *
 *     scaleNow is |window mean| + sigma. scaleHist is the largest magnitude the
 *     call has seen up to this bar, and `bars` is how many outputs precede this
 *     one in the SAME call.
 *
 *     TERM 1, the cancellation. The slope is evaluated as
 *     (n*SumXY - SumX*SumY)/Divisor, and those two products are both about
 *     mean*n^3/2 while their difference is about slope*n^4/12 -- so the
 *     subtraction loses digits in proportion to how large the LEVEL is next to
 *     the TREND, and the absolute error left in the slope is ~eps*|mean| however
 *     small the slope itself is. The other three value outputs are the fitted
 *     line evaluated at one x, so they inherit that error multiplied by up to
 *     period/2 against values of size |mean| -- which is why they stay
 *     RELATIVELY accurate to a few ulp while the slope does not. (ANGLE is
 *     atan of the slope, so it inherits the slope's problem, not theirs.) Wilkinson LITTLE (a value of 1 carrying
 *     a 1e-8 spread) comes back 6.2e-9 wrong; a perfectly straight line at 2^26
 *     rising 2^-26 per bar returns a slope 21% to 100% wrong on the very FIRST
 *     output of the call, before any recurrence has run -- and at period 2 that
 *     is not a figure of speech: it returns -0.0 for a strictly positive,
 *     exactly representable slope, at every bar of that series. This is not new: the
 *     pre-#103 code evaluated the same expression on the same unshifted sums,
 *     and it is the defect class #118 fixed in TA_VAR and #242 in
 *     TA_CORREL/TA_BETA -- anchoring the window before summing would remove it.
 *
 *     TERM 2, the drift, and it IS new: #103 made SumY and SumXY running totals
 *     that are never recomputed, so every bar's rounding is added to a residue
 *     that no later bar can subtract. It grows with how far into the CALL a bar
 *     sits, and its size is set by the largest value the sums have ever held --
 *     not by the current window. A single large print therefore corrupts every
 *     later bar, permanently and by a growing amount, and TA_SUCCESS is still
 *     returned. L9 measures it directly; the numbers and the reproduction are
 *     in issue #254.
 *
 *     C_DRIFT is therefore an allowance for an open defect (#254), not a property
 *     worth preserving: when it is fixed the term should be deleted, and the legs
 *     below will pass with several orders of margin. Its exponent is 1.5, not 1:
 *     the residue is a random walk of per-bar roundings rather than their sum,
 *     and this library's cumulative-error law is already documented as n^1.5
 *     (#180). That matters for tightness, not tidiness -- against bars^1 the
 *     worst measured ratio is 82.5 and the constant has to be 300, which is loose
 *     enough to be worthless at small k; against bars^1.5 it is 3.63 and the
 *     constant is 11. All three constants are ~3x their worst measured ratio,
 *     calibrated on disjoint window sets (k <= 4 for the cancellation terms,
 *     k >= 16 for drift) so neither absorbs the other.
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

#define LR_EPS      2.2204460492503131e-16
/* All three are ~3x the worst ratio measured over every corpus in this file.
 * The two terms are calibrated on DISJOINT window sets so neither absorbs the
 * other: the cancellation constants on windows with k <= 4 (where the residue
 * has not had time to build), the drift constant on k >= 16 (where it
 * dominates). Re-measure the same way if you change a corpus. */
#define LR_C_SLOPE  50.0   /* cancellation, slope        (measured worst 15.6) */
#define LR_C_VALUE  40.0   /* cancellation, other outputs (measured worst 10.7) */
#define LR_C_DRIFT  11.0   /* #103 running-sum residue    (measured worst 3.63) */
#define LR_DEG      ( 180.0 / 3.14159265358979323846 )
#define LR_N        2048

/* The five outputs of one call, in one place, so every leg reads the same shape.
 * It does NOT by itself stop a leg testing a subset. L5 is the one that does:
 * its shift and scale laws cover SLOPE, LINEARREG, INTERCEPT and TSF, and the
 * reversal law covers SLOPE alone, because negation is the only one of the five
 * whose law under a reversal is worth stating. ANGLE is atan(SLOPE), so L5
 * reaches it only through the slope. Every other leg (L1-L4, L6-L8) puts all
 * five through lr_compare. */
typedef struct
{
   double slope[LR_N];
   double intercept[LR_N];
   double fit[LR_N];
   double tsf[LR_N];
   double angle[LR_N];
   /* max|y| over [0, begIdx+k], i.e. the largest value the running sums have
    * absorbed by the time this output is produced -- the scale the #103 residue
    * lives on, which is NOT the current window's scale once a large print has
    * gone by. */
   double hist[LR_N];
   int    begIdx;
   int    nbElement;
} LR_Out;

/**** Local functions declarations.    ****/
static ErrorNumber test_linearreg_wilkinson( void );
static ErrorNumber test_linearreg_affine_identity( void );
static ErrorNumber test_linearreg_ladder_golden( void );
static ErrorNumber test_linearreg_nist_numacc( void );
static ErrorNumber test_linearreg_metamorphic( void );
static ErrorNumber test_linearreg_constant_window( void );
static ErrorNumber test_linearreg_internal_consistency( void );
static ErrorNumber test_linearreg_random_walk( void );
static ErrorNumber test_linearreg_range_stability( void );

/**** Local variables definitions.     ****/
static LR_Out lr_out;
static double lr_scratch[LR_N];

/**** Global functions definitions.   ****/
ErrorNumber test_func_linearreg( TA_History *history )
{
   ErrorNumber retValue;

   (void)history;   /* every leg below builds its own series */


   retValue = test_linearreg_wilkinson();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG Wilkinson nasty.dat (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_linearreg_affine_identity();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG affine identity (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_linearreg_ladder_golden();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG sliding-sum ladder (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_linearreg_nist_numacc();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG NIST StRD NumAcc (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_linearreg_metamorphic();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG metamorphic laws (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_linearreg_constant_window();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG constant window (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_linearreg_internal_consistency();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG internal consistency (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_linearreg_random_walk();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG random-walk oracle (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }
   retValue = test_linearreg_range_stability();
   if( retValue != TA_TEST_PASS ) { printf( "%s Failed LINEARREG range stability (#251) (Code=%d)\n", __FILE__, retValue ); return retValue; }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* One call per output, same range and period, into lr_out. Returns non-zero on
 * an unexpected return code or a disagreement between the five about how many
 * values they produced -- which is itself worth asserting, since the lookback
 * is shared and a divergence there would be a real defect. */
static int lr_run( const char *label, const double *y, int n, int period )
{
   TA_Integer b[5], nb[5];
   TA_RetCode rc[5];
   int i;

   if( n > LR_N ) { printf( "LINEARREG #251 [%s]: series of %d exceeds LR_N\n", label, n ); return 1; }

   rc[0] = TA_LINEARREG_SLOPE    ( 0, n-1, y, period, &b[0], &nb[0], lr_out.slope );
   rc[1] = TA_LINEARREG_INTERCEPT( 0, n-1, y, period, &b[1], &nb[1], lr_out.intercept );
   rc[2] = TA_LINEARREG          ( 0, n-1, y, period, &b[2], &nb[2], lr_out.fit );
   rc[3] = TA_TSF                ( 0, n-1, y, period, &b[3], &nb[3], lr_out.tsf );
   rc[4] = TA_LINEARREG_ANGLE    ( 0, n-1, y, period, &b[4], &nb[4], lr_out.angle );

   for( i = 0; i < 5; i++ )
      if( rc[i] != TA_SUCCESS || b[i] != b[0] || nb[i] != nb[0] )
      {
         printf( "LINEARREG #251 [%s]: output %d rc=%d beg=%d nb=%d (output 0: rc=%d beg=%d nb=%d)\n",
                 label, i, (int)rc[i], (int)b[i], (int)nb[i], (int)rc[0], (int)b[0], (int)nb[0] );
         return 1;
      }
   lr_out.begIdx    = (int)b[0];
   lr_out.nbElement = (int)nb[0];

   {
      double hi = 0.0;
      int k;
      for( i = 0; i <= lr_out.begIdx; i++ ) if( fabs( y[i] ) > hi ) hi = fabs( y[i] );
      for( k = 0; k < lr_out.nbElement; k++ )
      {
         const int bar = lr_out.begIdx + k;
         if( fabs( y[bar] ) > hi ) hi = fabs( y[bar] );
         lr_out.hist[k] = hi;
      }
   }
   return 0;
}

/* The scale a window's answers live on: how big the values are, plus how much
 * they vary. Adding sigma is what keeps the bound from collapsing to zero on a
 * window whose mean happens to cross zero -- there the level cancellation this
 * model describes is absent, but the spread is still real. */
static double lr_scale( const double *y, int s, int period )
{
   double mean = 0.0;
   double sigma = ta_test_ref_stddev( y, s, period, &mean );
   return fabs( mean ) + sigma;
}

/* Referee one call's five outputs against four expected values (and the angle
 * derived from the expected slope). `expect*` are absolute values, not
 * tolerances -- every caller either bakes them, states them in closed form, or
 * takes them from the shared oracle. */
static ErrorNumber lr_compare( const char *label, const double *y, int period,
                               int k, double eSlope, double eIntercept,
                               double eFit, double eTsf )
{
   const int bar   = lr_out.begIdx + k;
   const double sc = lr_scale( y, bar - period + 1, period );
   /* Term 2: the residue the running sums have accumulated over the k outputs
    * this call has already produced, on the scale of the largest value they
    * have held. Zero on the first output of every call, which is why the
    * single-window legs below get the tight bound automatically. */
   /* k^1.5, not k. The residue is a random walk of per-bar roundings, not a
    * sum of them, and this library's cumulative-error law is already documented
    * as n^1.5 (#180). Fitting the exponent rather than inflating a linear
    * constant is what makes this term tight: against k the worst measured ratio
    * over every corpus here is 82.5, against k^1.5 it is 3.63. */
   const double drift    = LR_C_DRIFT * LR_EPS * lr_out.hist[k]
                           * (double)k * sqrt( (double)k ) / (double)period;
   const double tolSlope = LR_C_SLOPE * LR_EPS * sc + drift;
   const double tolValue = ( LR_C_VALUE * LR_EPS * sc + drift ) * (double)period;
   const double eAngle   = atan( eSlope ) * LR_DEG;
   /* d(angle)/d(slope) is LR_DEG/(1+m^2); the second term is atan()'s own
    * rounding, which is libm's business and not this library's. */
   const double tolAngle = tolSlope * LR_DEG / ( 1.0 + eSlope * eSlope )
                           + 4.0 * LR_EPS * fabs( eAngle );

   struct { const char *name; double got, want, tol; } q[5];
   int i;

   q[0].name = "SLOPE";     q[0].got = lr_out.slope[k];     q[0].want = eSlope;     q[0].tol = tolSlope;
   q[1].name = "INTERCEPT"; q[1].got = lr_out.intercept[k]; q[1].want = eIntercept; q[1].tol = tolValue;
   q[2].name = "LINEARREG"; q[2].got = lr_out.fit[k];       q[2].want = eFit;       q[2].tol = tolValue;
   q[3].name = "TSF";       q[3].got = lr_out.tsf[k];       q[3].want = eTsf;       q[3].tol = tolValue;
   q[4].name = "ANGLE";     q[4].got = lr_out.angle[k];     q[4].want = eAngle;     q[4].tol = tolAngle;

   for( i = 0; i < 5; i++ )
   {
      double d;
      if( q[i].got != q[i].got )
      {
         printf( "LINEARREG #251 [%s]: %s NaN period=%d bar=%d\n",
                 label, q[i].name, period, bar );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      d = fabs( q[i].got - q[i].want );
      if( d > q[i].tol )
      {
         printf( "LINEARREG #251 [%s]: %s period=%d bar=%d val=%.17g want=%.17g "
                 "(|diff| %.3g > %.3g; scale %.3g, high-water %.3g, %d bars into the call)\n",
                 label, q[i].name, period, bar, q[i].got, q[i].want, d, q[i].tol,
                 sc, lr_out.hist[k], k );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (L1) Wilkinson's "nasty.dat" regressed on the bar index -- his task IV.B,
 * which is what these functions natively do, at period 9 because X is 1..9.
 *
 * The expected values are BAKED (#251): scripts/gen_test_reference.py evaluates
 * the least-squares fit of these exact doubles in exact rational arithmetic.
 * FIVE columns -- X, ROUND, BIG, HUGE, ZERO -- have all four answers exactly
 * representable, and their goldens read as such (X and BIG both give a slope of
 * exactly 1, HUGE 1e12).
 *
 * The other two are the point of the exercise, and they are not the two you
 * would guess. LITTLE's SLOPE *is* exactly representable, but its stored doubles
 * are not exactly 1e-8 apart, so that slope is 9.999999994736442e-09 rather than
 * 1e-8, and its intercept/fit/forecast are rounded. TINY is the trap: all four
 * of its answers are rounded, while the goldens read 1e-12 / 1e-12 / 9e-12 /
 * 1e-11 and look exact -- because 1e-12 .. 9e-12 are not nine equally spaced
 * doubles. Nothing but an exact computation distinguishes those two cases. */
static ErrorNumber test_linearreg_wilkinson( void )
{
   int i;

   for( i = 0; i < TA_TEST_REF_WILKINSON_NB_SERIES; i++ )
   {
      const double *w = ta_test_ref_wilkinson_series[i];
      ErrorNumber e;
      char label[64];

      snprintf( label, sizeof label, "wilkinson %s", ta_test_ref_wilkinson_names[i] );
      if( lr_run( label, w, TA_TEST_REF_WILKINSON_N, TA_TEST_REF_WILKINSON_N ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      if( lr_out.nbElement != 1 )
      {
         printf( "LINEARREG #251 [%s]: nb=%d, expected 1\n", label, lr_out.nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      e = lr_compare( label, w, TA_TEST_REF_WILKINSON_N, 0,
                      ta_test_ref_golden_wilkinson_slope[i],
                      ta_test_ref_golden_wilkinson_intercept[i],
                      ta_test_ref_golden_wilkinson_fit[i],
                      ta_test_ref_golden_wilkinson_tsf[i] );
      if( e != TA_TEST_PASS ) return e;
   }
   return TA_TEST_PASS;
}

/* (L2) The exact identity. For y[i] = a*i + b every window's least-squares fit
 * IS the line, so all five answers are closed forms:
 *
 *   SLOPE = a,  INTERCEPT = a*(oldest bar) + b,  LINEARREG = a*bar + b,
 *   TSF = a*(bar+1) + b,  ANGLE = atan(a) in degrees.
 *
 * No oracle and no golden table are involved, which makes this the strongest
 * leg in the file: nothing here can be co-wrong with the thing it judges.
 *
 * The pairs are chosen for what they stress, not for coverage: a=1e-8 under
 * b=1e8 is the regime where the slope's absolute error (~eps*|mean|) swamps the
 * slope itself, and a=0 asserts that a genuinely flat line reads as flat. 400
 * bars so the O(1) recurrence is running, not just priming. */
static ErrorNumber test_linearreg_affine_identity( void )
{
   enum { N = 400 };
   /* Every pair is chosen so that a*i + b is EXACTLY representable for i < N:
    * the small slopes are negative powers of two and the large offsets are
    * multiples of a power of two, so a*i has no bits below b's ulp. That is what
    * lets this leg compare against the closed form with no oracle and no
    * representation slack -- feed it 1e-8 instead of 2^-27 and the stored series
    * is no longer on the line, so the "exact" answer would not be exact.
    *
    * 2^-27 under b=1 is Wilkinson LITTLE's regime. 2^-26 under b=2^26 is the
    * one that matters: a perfectly straight line where the level is 2^52 times
    * the per-bar step, which is where TA_LINEARREG_SLOPE runs out of digits
    * entirely (see the tolerance-model note at the top of this file). */
   static const double as[8] = { 1.0, -0.25, 7.4505805969238281e-09, 1.0e6,
                                 0.0, 1.4901161193847656e-08, 3.0, -9.5367431640625e-07 };
   static const double bs[8] = { 0.0, 1000.0, 1.0, 0.0,
                                 12345.0, 67108864.0, -5.0e7, 99614720.0 };
   static const int periods[6] = { 2, 3, 5, 14, 30, 60 };
   int c, p, i, k;

   for( c = 0; c < 8; c++ )
      for( p = 0; p < 6; p++ )
      {
         const int period = periods[p];
         const double a = as[c], b = bs[c];
         char label[96];

         snprintf( label, sizeof label, "affine a=%g b=%g", a, b );
         for( i = 0; i < N; i++ ) lr_scratch[i] = a * (double)i + b;
         if( lr_run( label, lr_scratch, N, period ) )
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;

         for( k = 0; k < lr_out.nbElement; k++ )
         {
            const int bar = lr_out.begIdx + k;
            const int oldest = bar - period + 1;
            ErrorNumber e = lr_compare( label, lr_scratch, period, k,
                                        a,
                                        a * (double)oldest + b,
                                        a * (double)bar + b,
                                        a * (double)( bar + 1 ) + b );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   return TA_TEST_PASS;
}

/* (L3) The sliding-sum ladder against baked goldens, at four periods.
 *
 * This is the leg that would catch drift in the O(1) recurrence itself, because
 * the ladder's answers are arbitrary -- no identity and no round number to hide
 * behind.
 *
 * ITS RESOLUTION IS THE LIBRARY'S, NOT THE GOLDENS'. Measured, this leg detects a
 * library error of about 0.1% on the 1e8-level windows, and that floor is the
 * function's own: an intrinsic cancellation of ~eps*|mean| against a slope of
 * 0.01 is already 1e-4 relative, so no honest bound can do better there. A
 * golden corrupted by less than that is invisible HERE -- and caught anyway,
 * because test_reference.c compares every golden against the oracle at 1e-15.
 * Two layers, different jobs: that leg protects the table, this one protects the
 * library. Sabotage-checked both ways. Its four regimes (an eight-digit level under a one-cent spread, a
 * single 1e12 print transiting the window, a four-decade level shift, and an
 * exactly flat tail) are described where the series is defined. */
static ErrorNumber test_linearreg_ladder_golden( void )
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
   int t, k;

   for( t = 0; t < TA_TEST_REF_GOLDEN_LADDER_PERIODS_N; t++ )
   {
      const int period = ta_test_ref_golden_ladder_periods[t];
      char label[64];

      snprintf( label, sizeof label, "ladder p%d", period );
      if( lr_run( label, ta_test_ref_ladder, TA_TEST_REF_LADDER_N, period ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      if( lr_out.nbElement != ta_test_ref_golden_ladder_counts[t] )
      {
         printf( "LINEARREG #251 [%s]: %d windows but %d baked values -- this test and "
                 "scripts/gen_test_reference.py disagree about the corpus\n",
                 label, lr_out.nbElement, ta_test_ref_golden_ladder_counts[t] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( k = 0; k < lr_out.nbElement; k++ )
      {
         ErrorNumber e = lr_compare( label, ta_test_ref_ladder, period, k,
                                     gSlope[t][k], gIcept[t][k], gFit[t][k], gTsf[t][k] );
         if( e != TA_TEST_PASS ) return e;
      }
   }
   return TA_TEST_PASS;
}

/* (L4) NIST StRD NumAcc3/NumAcc4 -- 1001 values that are a large offset carrying
 * a spread of 0.1, purpose-built to break software that subtracts one big number
 * from another. NIST certifies their VARIANCE, not a regression, so the referee
 * here is the shared oracle rather than a certificate; what the datasets
 * contribute is the conditioning. NumAcc4's offset is ten times NumAcc3's, which
 * is the point of running both: the bound scales with it and the outputs must
 * keep up. */
static ErrorNumber test_linearreg_nist_numacc( void )
{
   static const int periods[5] = { 2, 5, 14, 30, 60 };
   int which, p, k, n;

   for( which = 3; which <= 4; which++ )
   {
      char label[64];
      n = ta_test_ref_numacc( which, lr_scratch );
      snprintf( label, sizeof label, "NIST NumAcc%d", which );
      for( p = 0; p < 5; p++ )
      {
         const int period = periods[p];
         if( lr_run( label, lr_scratch, n, period ) )
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         for( k = 0; k < lr_out.nbElement; k++ )
         {
            double sl, ic, ft, ts;
            ErrorNumber e;
            ta_test_ref_linreg( lr_scratch, lr_out.begIdx + k - ( period - 1 ), period,
                                &sl, &ic, &ft, &ts );
            e = lr_compare( label, lr_scratch, period, k, sl, ic, ft, ts );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }
   return TA_TEST_PASS;
}

/* (L5) Metamorphic laws. Each is exact in real arithmetic and needs no oracle:
 *
 *   shift    slope(y+c) == slope(y)          LINEARREG(y+c) == LINEARREG(y)+c
 *   scale    slope(c*y) == c*slope(y)        LINEARREG(c*y) == c*LINEARREG(y)
 *   reverse  slope(reverse(y)) == -slope(y)
 *
 * The reversal is the one that cannot be satisfied by accident: it is the only
 * law here that a sign error or an off-by-one in the index weighting breaks
 * while leaving the shift and scale laws intact.
 *
 * The bounds come from the same model, evaluated on whichever of the two series
 * carries the larger scale -- a shift by 1e10 makes the transformed series a
 * genuinely worse-conditioned problem, and the law cannot hold tighter than
 * that. */
static ErrorNumber test_linearreg_metamorphic( void )
{
   enum { N = 300 };
   static double base[N], other[N], keepSlope[LR_N], keepFit[LR_N], keepHist[LR_N];
   static double keepIcept[LR_N], keepTsf[LR_N];
   static const int periods[4] = { 2, 5, 14, 30 };
   static const double shifts[3] = { 1.0e4, 1.0e8, 1.0e10 };
   static const double scales[3] = { 1.0e3, 1.0e-3, -7.5 };
   int p, c, i, k;

   ta_test_ref_lcg_seed( 0x11EA6E60u );
   for( i = 0; i < N; i++ ) base[i] = 500.0 + 40.0 * ta_test_ref_lcg_sym();

   for( p = 0; p < 4; p++ )
   {
      const int period = periods[p];

      if( lr_run( "metamorphic base", base, N, period ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      memcpy( keepSlope, lr_out.slope, sizeof(double) * (size_t)lr_out.nbElement );
      memcpy( keepFit,   lr_out.fit,   sizeof(double) * (size_t)lr_out.nbElement );
      memcpy( keepHist,  lr_out.hist,  sizeof(double) * (size_t)lr_out.nbElement );
      memcpy( keepIcept, lr_out.intercept, sizeof(double) * (size_t)lr_out.nbElement );
      memcpy( keepTsf,   lr_out.tsf,   sizeof(double) * (size_t)lr_out.nbElement );

      for( c = 0; c < 6; c++ )
      {
         const int isShift = ( c < 3 );
         const double k1 = isShift ? 1.0 : scales[c-3];       /* slope multiplier */
         const double k0 = isShift ? shifts[c] : 0.0;         /* value offset     */
         char label[96];

         if( isShift ) snprintf( label, sizeof label, "shift +%g", k0 );
         else          snprintf( label, sizeof label, "scale x%g", k1 );

         for( i = 0; i < N; i++ ) other[i] = k1 * base[i] + k0;
         if( lr_run( label, other, N, period ) ) return TA_TESTUTIL_TFRR_BAD_CALCULATION;

         for( k = 0; k < lr_out.nbElement; k++ )
         {
            const int s = lr_out.begIdx + k - ( period - 1 );
            /* BOTH sides of a metamorphic law were computed, so both carry the
             * model's error and the bound is their sum -- the transformed
             * series' own scale, plus the base's scale carried through the
             * transform. Sizing it from the transformed side alone reds on
             * scale x1e-3, where the base's 1e-13 slope error shrinks to 1e-16
             * but the transformed series' own is 1e-15. */
            const double sc = lr_scale( other, s, period ) + fabs( k1 ) * lr_scale( base, s, period );
            /* Both sides drifted, so both residues are in the bound. */
            const double drift = LR_C_DRIFT * LR_EPS
                                 * ( lr_out.hist[k] + fabs( k1 ) * keepHist[k] )
                                 * (double)k * sqrt( (double)k ) / (double)period;
            const double tolSlope = LR_C_SLOPE * LR_EPS * sc + drift;
            const double tolValue = ( LR_C_VALUE * LR_EPS * sc + drift ) * (double)period;
            const double wantSlope = k1 * keepSlope[k];
            const double wantFit   = k1 * keepFit[k] + k0;
            /* The same two laws hold for the other two value outputs, and they
             * are the ones an index-weighting error would move differently:
             * INTERCEPT is the fit at the OLDEST bar and TSF one bar past the
             * newest, so a shift or scale that came out right at the middle and
             * wrong at the ends shows up here and nowhere else in this leg. */
            const double wantIcept = k1 * keepIcept[k] + k0;
            const double wantTsf   = k1 * keepTsf[k]   + k0;

            if( fabs( lr_out.slope[k] - wantSlope ) > tolSlope )
            {
               printf( "LINEARREG #251 [%s]: SLOPE period=%d bar=%d val=%.17g "
                       "transformed-base=%.17g (|diff| %.3g > %.3g)\n",
                       label, period, lr_out.begIdx + k, lr_out.slope[k], wantSlope,
                       fabs( lr_out.slope[k] - wantSlope ), tolSlope );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            {
               struct { const char *name; double got, want; } v[3];
               int q;
               v[0].name = "LINEARREG"; v[0].got = lr_out.fit[k];       v[0].want = wantFit;
               v[1].name = "INTERCEPT"; v[1].got = lr_out.intercept[k]; v[1].want = wantIcept;
               v[2].name = "TSF";       v[2].got = lr_out.tsf[k];       v[2].want = wantTsf;
               for( q = 0; q < 3; q++ )
                  if( fabs( v[q].got - v[q].want ) > tolValue )
                  {
                     printf( "LINEARREG #251 [%s]: %s period=%d bar=%d val=%.17g "
                             "transformed-base=%.17g (|diff| %.3g > %.3g)\n",
                             label, v[q].name, period, lr_out.begIdx + k, v[q].got,
                             v[q].want, fabs( v[q].got - v[q].want ), tolValue );
                     return TA_TESTUTIL_TFRR_BAD_CALCULATION;
                  }
            }
         }
      }

      /* Time reversal. The window ending at `bar` of the reversed series holds
       * the same values as some window of the original, in the opposite order,
       * so its slope is the negation. This is the only law here that a sign
       * error or an off-by-one in the index weighting breaks while leaving the
       * shift and scale laws intact. */
      for( i = 0; i < N; i++ ) other[i] = base[N-1-i];
      if( lr_run( "reverse", other, N, period ) ) return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      for( k = 0; k < lr_out.nbElement; k++ )
      {
         const int bar = lr_out.begIdx + k;
         const int s = bar - period + 1;
         /* [s, bar] reversed is [N-1-bar, N-1-s] forward, which is the output
          * indexed by its own last bar N-1-s. */
         const int mirror = ( N - 1 - s ) - ( period - 1 );
         double sc, tolSlope;
         if( mirror < 0 || mirror >= lr_out.nbElement ) continue;
         sc = lr_scale( other, s, period ) + lr_scale( base, N-1-bar, period );
         tolSlope = LR_C_SLOPE * LR_EPS * sc
                    + LR_C_DRIFT * LR_EPS * ( lr_out.hist[k] + keepHist[mirror] )
                      * (double)( k > mirror ? k : mirror )
                      * sqrt( (double)( k > mirror ? k : mirror ) ) / (double)period;
         if( fabs( lr_out.slope[k] + keepSlope[mirror] ) > tolSlope )
         {
            printf( "LINEARREG #251 [reverse]: SLOPE period=%d bar=%d val=%.17g "
                    "-(forward %.17g) (|diff| %.3g > %.3g)\n",
                    period, bar, lr_out.slope[k], keepSlope[mirror],
                    fabs( lr_out.slope[k] + keepSlope[mirror] ), tolSlope );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   return TA_TEST_PASS;
}

/* (L6) A window that is exactly constant. The fitted line is the constant, so
 * LINEARREG, TSF and LINEARREG_INTERCEPT must all return it and the slope must
 * be flat -- and the interesting part is that the window is reached from a
 * NOISY prefix, so the O(1) sums arrive carrying the noise's residue and have to
 * shed it. That is the linear-regression analogue of the flat-tail test #243
 * added for TA_VAR/TA_STDDEV.
 *
 * Observed, and deliberately NOT asserted as such: the slope comes back exactly
 * 0.0 at every level tried except 1e-6, where a residue of up to 1.4e-22
 * survives -- about eps*level/period, i.e. ordinary rounding rather than a
 * stale anchor. Pinning "exactly zero" would therefore be pinning an accident
 * of which levels happen to be representable; the bound below is the model, and
 * it holds at every level including that one. */
static ErrorNumber test_linearreg_constant_window( void )
{
   enum { N = 600, NOISY = 100 };
   static const double levels[5] = { 100.0, 1234.56789, 1.0e8, 1.0e-6, 1.0e11 };
   static const int periods[4] = { 2, 5, 20, 49 };
   int li, p, i, k;

   for( li = 0; li < 5; li++ )
   {
      const double lvl = levels[li];
      char label[64];

      snprintf( label, sizeof label, "constant tail @%g", lvl );
      ta_test_ref_lcg_seed( 0xF1A77A11u + (unsigned)li );
      for( i = 0; i < N; i++ )
         lr_scratch[i] = ( i < NOISY ) ? lvl * ( 1.0 + 0.05 * ta_test_ref_lcg_sym() ) : lvl;

      for( p = 0; p < 4; p++ )
      {
         const int period = periods[p];
         const int firstFlat = NOISY + period - 1;   /* first wholly-flat window */
         if( lr_run( label, lr_scratch, N, period ) )
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         for( k = 0; k < lr_out.nbElement; k++ )
         {
            ErrorNumber e;
            if( lr_out.begIdx + k < firstFlat ) continue;
            e = lr_compare( label, lr_scratch, period, k, 0.0, lvl, lvl, lvl );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }
   return TA_TEST_PASS;
}

/* (L7) The four value outputs describe ONE line, so three identities hold
 * between them by construction:
 *
 *   LINEARREG == INTERCEPT + SLOPE*(period-1)
 *   TSF       == INTERCEPT + SLOPE*period
 *   ANGLE     == atan(SLOPE) in degrees
 *
 * These are same-lineage -- all five entry points evaluate the same recurrence,
 * so this proves consistency and not correctness, which is why it is the last
 * leg and not the first. It is still worth its few lines: the five are separate
 * generated functions with separate output expressions, and an edit that
 * touched one of them alone would show up here at once.
 *
 * The bound is the value model plus the rounding of the right-hand side itself,
 * which is evaluated here in plain double exactly as the library does. */
static ErrorNumber test_linearreg_internal_consistency( void )
{
   enum { N = 500 };
   static const int periods[5] = { 2, 5, 14, 30, 60 };
   static const double mags[3] = { 1.0, 1.0e4, 1.0e9 };
   int m, p, i, k;

   for( m = 0; m < 3; m++ )
   {
      ta_test_ref_lcg_seed( 0xC047157Eu + (unsigned)m );
      lr_scratch[0] = mags[m];
      for( i = 1; i < N; i++ )
         lr_scratch[i] = lr_scratch[i-1] * ( 1.0 + 0.02 * ta_test_ref_lcg_sym() );

      for( p = 0; p < 5; p++ )
      {
         const int period = periods[p];
         char label[64];
         snprintf( label, sizeof label, "consistency mag=%g", mags[m] );
         if( lr_run( label, lr_scratch, N, period ) )
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         for( k = 0; k < lr_out.nbElement; k++ )
         {
            const int bar = lr_out.begIdx + k;
            const double sc  = lr_scale( lr_scratch, bar - period + 1, period );
            const double tol = LR_C_VALUE * LR_EPS * sc * (double)period;
            const double wantFit = lr_out.intercept[k] + lr_out.slope[k] * (double)(period-1);
            const double wantTsf = lr_out.intercept[k] + lr_out.slope[k] * (double)period;
            const double wantAng = atan( lr_out.slope[k] ) * LR_DEG;

            if( fabs( lr_out.fit[k] - wantFit ) > tol ||
                fabs( lr_out.tsf[k] - wantTsf ) > tol )
            {
               printf( "LINEARREG #251 [%s]: period=%d bar=%d LINEARREG=%.17g (b+m*(p-1)=%.17g) "
                       "TSF=%.17g (b+m*p=%.17g) tol=%.3g\n", label, period, bar,
                       lr_out.fit[k], wantFit, lr_out.tsf[k], wantTsf, tol );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            if( fabs( lr_out.angle[k] - wantAng ) > 8.0 * LR_EPS * fabs( wantAng ) )
            {
               printf( "LINEARREG #251 [%s]: period=%d bar=%d ANGLE=%.17g atan(SLOPE)=%.17g\n",
                       label, period, bar, lr_out.angle[k], wantAng );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}

/* (L8) The arbitrary-value referee: geometric random walks at four magnitudes,
 * judged bar by bar by the shared double-double oracle. The identities above
 * pin exact answers on data chosen to be hostile; this leg is what would catch a
 * defect that only shows on ordinary data with no round number in sight. */
static ErrorNumber test_linearreg_random_walk( void )
{
   enum { N = 1000 };
   static const double mags[4] = { 1.0, 1.0e4, 1.0e8, 1.0e11 };
   static const int periods[5] = { 2, 5, 14, 30, 60 };
   int m, p, i, k;

   for( m = 0; m < 4; m++ )
   {
      ta_test_ref_lcg_seed( 0xC0FFEEu + (unsigned)m );
      lr_scratch[0] = mags[m];
      for( i = 1; i < N; i++ )
         lr_scratch[i] = lr_scratch[i-1] * ( 1.0 + 0.01 * ta_test_ref_lcg_sym() );

      for( p = 0; p < 5; p++ )
      {
         const int period = periods[p];
         char label[64];
         snprintf( label, sizeof label, "random walk @%g", mags[m] );
         if( lr_run( label, lr_scratch, N, period ) )
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         for( k = 0; k < lr_out.nbElement; k++ )
         {
            double sl, ic, ft, ts;
            ErrorNumber e;
            ta_test_ref_linreg( lr_scratch, lr_out.begIdx + k - ( period - 1 ), period,
                                &sl, &ic, &ft, &ts );
            e = lr_compare( label, lr_scratch, period, k, sl, ic, ft, ts );
            if( e != TA_TEST_PASS ) return e;
         }
      }
   }
   return TA_TEST_PASS;
}

/* (L9) RANGE STABILITY -- issue #254, and the leg that survives the corpus
 * change the ladder needed.
 *
 * The value at a bar must not depend on where the CALL started. ta_regtest
 * enforces that through doRangeTest, and the --codegen leg already applies it to
 * these five: test_codegen.c's stability_class() puts them in TA_STABLE_EPSILON,
 * with a note saying #103 moved them OUT of TA_STABLE_EXACT precisely because
 * the O(1) recurrence made them range-dependent. So the drift is not unnoticed
 * and not unenforced -- an earlier draft of this comment said both, and issue
 * #254 carries the correction.
 *
 * THE TIER IS ABSOLUTE, NOT RELATIVE. TA_STABLE_EPSILON is
 * TA_REAL_EQ(val1, val2, 1e-10) (test_util.c), and TA_REAL_EQ(x,v,ep) is
 * ((v-ep) <= x) && (x <= (v+ep)) (ta_func/ta_utility.h) -- an absolute band.
 * Reading a relative error against it, as that same draft did, overstates the
 * defect by orders of magnitude on a quantity that crosses zero.
 *
 * Measured absolute |full-range call - same bar computed alone|, geometric
 * random walk at $100, 1.5% steps, seed 0xBEEF. Bold = over the 1e-10 tier:
 *
 *     bars     period 5     period 14    period 30
 *      252     2.67e-12     4.84e-13     4.84e-14
 *     1000     5.34e-11     5.46e-12     2.62e-13
 *     2000    *1.62e-10*    1.19e-11     4.92e-13
 *     5000    *5.68e-10*    1.93e-11     6.83e-12
 *    20000    *2.07e-09*   *2.03e-10*    3.79e-11
 *
 * and with ONE 1000x bad print at bar 60, period 5: 4.45e-10 at 252 bars,
 * 2.25e-09 at 1000, 1.21e-08 at 5000, 4.82e-08 at 20000 -- over the tier at
 * every length, including the 252 bars of ta_regtest's own history.
 *
 * So the surviving claim is narrower than "an unnoticed defect", and sharper:
 * THE EPSILON CLASSIFICATION IS CORPUS-LIMITED AND NOTHING SAYS SO. "~1e-9 drift
 * across ranges" reads as a fixed magnitude. It is not -- the residue grows
 * without bound in the length of the call, because SumY and SumXY are running
 * totals that are never recomputed, so each bar's rounding joins a residue no
 * later bar can subtract, and its size is set by the largest value the sums have
 * ever held rather than by the current window. The class therefore holds on the
 * 252-bar history the gate actually uses and stops holding at ~2000 bars, or at
 * 252 bars if the series contains a single large print.
 *
 * This leg pins the growth law rather than any one number, so it stays useful
 * whichever way #254 is resolved: a fix makes it pass with orders of margin, and
 * a change that made the accumulation worse fails it. The repair, the one #118
 * and #242 already use elsewhere here, is a periodic re-anchor -- recompute both
 * sums from the window every N bars, which is the priming scan these functions
 * already contain. #254 carries the sizing question and the streaming-handle
 * consequence.
 *
 * Non-vacuity is structural: the comparison is against the SAME function called
 * on one window, so there is no oracle to be co-wrong with.
 *
 * MEASURED HEADROOM, because "it has a bound" and "the bound is near what it
 * measures" are different claims: this leg runs 13x below its own bound at
 * period 2, 32x at 5, 56x at 14 and 111x at 30. It would therefore catch a
 * residue an order or two worse than today's, which is what pinning a growth
 * law needs, and it is not tightened further because the margin also absorbs
 * libm and platform variation.
 *
 * The two corpora come out COMPARABLE (13x/70x/56x/111x with the bad print),
 * which is worth recording because the opposite is the natural guess and it is
 * wrong: hist[k] makes the bound 1000x looser after the spike, so the outlier
 * corpus looks like it must be the slacker test. It is not, because the worst
 * ratio occurs EARLY, before the spike enters hist, and past bar 60 the error
 * and the bound rise together. Measured, not reasoned -- the reasoning gave the
 * wrong answer here.
 */
static ErrorNumber test_linearreg_range_stability( void )
{
   enum { N = 1200 };
   static const int periods[4] = { 2, 5, 14, 30 };
   static double one[8];
   int variant, p, i, k;

   for( variant = 0; variant < 2; variant++ )
   {
      char label[64];
      snprintf( label, sizeof label, "range stability%s",
                variant ? " + one 1000x bad print" : "" );
      ta_test_ref_lcg_seed( 0xBEEFu );
      lr_scratch[0] = 100.0;
      for( i = 1; i < N; i++ )
         lr_scratch[i] = lr_scratch[i-1] * ( 1.0 + 0.015 * ta_test_ref_lcg_sym() );
      if( variant ) lr_scratch[60] *= 1000.0;

      for( p = 0; p < 4; p++ )
      {
         const int period = periods[p];
         if( lr_run( label, lr_scratch, N, period ) )
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         for( k = 0; k < lr_out.nbElement; k++ )
         {
            const int bar = lr_out.begIdx + k;
            TA_Integer b2, nb2;
            TA_RetCode rc;
            double tol, d;

            /* startIdx == endIdx == bar: the lookback clamp leaves one output,
             * produced by the priming scan alone, so no recurrence and no
             * history contribute to it. */
            rc = TA_LINEARREG_SLOPE( bar, bar, lr_scratch, period, &b2, &nb2, one );
            if( rc != TA_SUCCESS || nb2 != 1 || (int)b2 != bar )
            {
               printf( "LINEARREG #251 [%s]: single-window call rc=%d beg=%d nb=%d "
                       "(wanted SUCCESS,%d,1)\n", label, (int)rc, (int)b2, (int)nb2, bar );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            tol = LR_C_SLOPE * LR_EPS * lr_scale( lr_scratch, bar - period + 1, period )
                  + LR_C_DRIFT * LR_EPS * lr_out.hist[k]
                    * (double)k * sqrt( (double)k ) / (double)period;
            d = fabs( lr_out.slope[k] - one[0] );
            if( d > tol )
            {
               printf( "LINEARREG #251 [%s]: SLOPE period=%d bar=%d full-range=%.17g "
                       "single-window=%.17g (|diff| %.3g > %.3g, %d bars into the call)\n",
                       label, period, bar, lr_out.slope[k], one[0], d, tol, k );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}
