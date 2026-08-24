#ifndef TA_TEST_REFERENCE_H
#define TA_TEST_REFERENCE_H

/* Shared numerical-reference battery for ta_regtest (issue #251).
 *
 * Three things live here, each of which used to be copied per test file:
 *
 *   1. The external datasets -- NIST StRD, Wilkinson's "nasty.dat", the pandas
 *      rolling-window adversarial arrays -- with their provenance.
 *   2. One trusted oracle family (variance, correlation, OLS slope, linear
 *      regression on the bar index), accumulated in compensated double-double.
 *   3. One random-number generator.
 *
 * Baked golden values computed OUTSIDE this binary are declared in
 * ta_test_reference_golden.h, which this header includes.
 *
 * WHY DOUBLE-DOUBLE AND NOT `long double`
 *
 * Every oracle in this suite used to accumulate in `long double`. That is 64
 * mantissa bits on x86 Linux, 53 on MSVC and 113 on AArch64 -- so a pin written
 * and measured on Linux was quietly WEAKER on Windows, where the referee has
 * exactly the precision of the thing it referees. The compensated form below
 * carries ~106 bits of mantissa on every ABI using nothing but `double`, so the
 * bounds these tests assert mean the same thing everywhere.
 *
 * It deliberately does not depend on -ffp-contract, which is the usual trap
 * here. Compensated arithmetic needs an EXACT product, and Dekker's splitting
 * computes one through an `a*b - c` expression the optimiser is free to
 * contract into an FMA, which changes it. Two-product below calls the C99
 * `fma()` FUNCTION instead -- a single correctly-rounded operation, not
 * something a flag can synthesise or withhold. The only other `a*b + c` shapes
 * in the file are correction terms whose exactness is not required, so
 * contracting them is harmless. This project does build with -ffp-contract=off
 * everywhere (CLAUDE.md: load-bearing for the PR #96 FMA contract), but nothing
 * here leans on that.
 */

#include "ta_test_reference_golden.h"

/* ---------------------------------------------------------------------------
 * Oracles. Every one of them accumulates in double-double and returns a
 * `double` good to ~0.5 ulp of the true result -- WHILE THE WINDOW'S DYNAMIC
 * RANGE FITS THE 106-BIT BUDGET. That caveat is not decoration: the scaled
 * deviations are n*x - sum(x), so one value large enough to set the pair's
 * exponent can push the residue that carries the answer off the bottom. On
 * ta_test_ref_pd_extreme_x (3e37 against neighbours of ~3, an ODD period so the
 * outlier lands on a near-zero regression weight) ta_test_ref_linreg returns a
 * slope of 0 where the true slope is 0.5. Even periods on the same array are
 * fine. Nothing in this suite feeds it such a window today; if you point a new
 * sweep at pd_extreme, referee it against a BAKED golden, not against these.
 *
 * The same form has a floor at the other end: the deviations are formed as
 * n*x - sum(x) and then SQUARED, so for values below ~1e-162 the square
 * underflows and every oracle here returns 0.0 -- a variance, a sigma and a
 * correlation alike. Measured: 1e-160 still works (var 1.98e-322, already
 * denormal), 1e-170 does not. Such values are inside this library's declared
 * input range, so this is a real limit and not a theoretical one; it is left as
 * a documented floor rather than fixed by rescaling, because no dataset here
 * goes near it (the smallest, pandas GH#52407, bottoms out at 1.4e-73).
 *
 * All of them take a window as (array, startOffset, period) and use the
 * POPULATION convention (divide by n), matching TA_VAR / TA_STDDEV / TA_CORREL.
 * -------------------------------------------------------------------------*/

/* Population variance of x[s .. s+period-1]. Writes the window mean through
 * outMean when it is not NULL (used for the kappa = |mean|/sigma conditioning
 * term the tolerances in this suite are built from).
 *
 * A window whose values are all bit-identical returns exactly 0.0 -- that is a
 * documented TA-Lib contract several tests assert, so the oracle must not
 * emit a rounding residue there. */
double ta_test_ref_var( const double *x, int s, int period, double *outMean );

/* sqrt of the above, with the square root also taken in double-double, so the
 * result is a correctly-rounded sigma rather than the square root of a rounded
 * variance. */
double ta_test_ref_stddev( const double *x, int s, int period, double *outMean );

/* Pearson correlation of x and y over the window, clamped to [-1, 1].
 * Returns 0.0 when either side has no variance, matching TA_CORREL's
 * documented degenerate contract. */
double ta_test_ref_corr( const double *x, const double *y, int s, int period );

/* OLS slope of y on x over the window (the quantity TA_BETA computes, once its
 * caller has turned prices into returns). Returns 0.0 when the regressor has no
 * variance, matching TA_BETA's contract.
 *
 * outKappa, when not NULL, receives max|x| / rms(x - mean(x)) over the window:
 * how large the regressor values are next to how much they actually vary. A
 * window of near-identical regressor values has a well-defined slope that
 * neither this oracle nor the shipped code can resolve better than ~kappa*eps. */
double ta_test_ref_slope( const double *x, const double *y, int s, int period,
                          double *outKappa );

/* Least-squares fit of y[s .. s+period-1] against the BAR INDEX, which is what
 * the TA_LINEARREG family regresses on -- there is no second input array.
 *
 * x runs 0 .. period-1 with 0 at the OLDEST bar of the window, so the four
 * outputs line up with four shipped functions:
 *
 *   outSlope     TA_LINEARREG_SLOPE       m
 *   outIntercept TA_LINEARREG_INTERCEPT   b, the fitted value at x = 0
 *   outFit       TA_LINEARREG             b + m*(period-1), at the newest bar
 *   outForecast  TA_TSF                   b + m*period, one bar ahead
 *
 * (TA_LINEARREG_ANGLE is atan(m) in degrees; it has no separate oracle because
 * the only thing it adds is a libm call the caller can make itself.)
 *
 * There is deliberately no conditioning out-parameter here, unlike
 * ta_test_ref_slope above. One existed, reporting the cancellation ratio of the
 * numerator the shipped recurrence evaluates; every caller passed NULL, and the
 * only reading it supported was a trap. It went to 0.0 both for a genuinely flat
 * fit and for a fit whose numerator cancelled below the 106-bit budget (the
 * pd_extreme case above), and on the second of those the oracle is 100% wrong
 * while the shipped function also returns -0 -- so a test that saw kappa == 0
 * and asserted exactness would have passed while checking nothing. A future leg
 * that wants a conditioning term for this family should add one that
 * distinguishes those two cases, rather than revive that one.
 *
 * Any out-pointer may be NULL. */
void ta_test_ref_linreg( const double *y, int s, int period,
                         double *outSlope, double *outIntercept,
                         double *outFit, double *outForecast );

/* 1 when every value of the window is bit-identical. Such a window makes a
 * correlation, a slope and an angle undefined, so it is evidence for nothing
 * and callers skip it. */
int ta_test_ref_window_is_constant( const double *v, int s, int period );

/* ---------------------------------------------------------------------------
 * One random-number generator.
 *
 * Two mappings share one LCG state because the two suites that use it were
 * written against different ranges and their measured tolerances depend on the
 * exact sequence. Callers re-seed before each block, so sharing the state is
 * safe as long as no two blocks interleave -- none do.
 * -------------------------------------------------------------------------*/
void   ta_test_ref_lcg_seed( unsigned int seed );
double ta_test_ref_lcg_sym( void );    /* uniform [-1.0, 1.0)  */
double ta_test_ref_lcg_half( void );   /* uniform [-0.5, 0.5)  */

/* xorshift32, kept separate from the LCG because its stream is what the CORREL
 * affine-invariance corpus was measured on. */
void   ta_test_ref_xorshift_seed( unsigned int seed );
double ta_test_ref_xorshift_unit( void );   /* uniform [0.0, 1.0] */

/* ---------------------------------------------------------------------------
 * Datasets. All public domain (NIST StRD is a US Government work) or
 * BSD-3-Clause (the Wilkinson arrays via scipy, the pandas arrays), into a
 * BSD-3-Clause project. See the issue #251 "Out of scope" note: no licensing or
 * attribution change is wanted, and none is needed.
 * -------------------------------------------------------------------------*/

/* NIST StRD "Norris" -- the only linear-least-squares set in the collection
 * with a single predictor, which is what makes its certified R-Squared a
 * certified Pearson r and its B1 a certified simple regression slope.
 * https://www.itl.nist.gov/div898/strd/lls/data/Norris.shtml */
#define TA_TEST_REF_NORRIS_N 36
extern const double ta_test_ref_norris_x[TA_TEST_REF_NORRIS_N];
extern const double ta_test_ref_norris_y[TA_TEST_REF_NORRIS_N];
/* Certified to 15 digits by NIST, independent of every implementation. */
#define TA_TEST_REF_NORRIS_B0  (-0.262323073774029)
#define TA_TEST_REF_NORRIS_B1  ( 1.00211681802045)
#define TA_TEST_REF_NORRIS_R2  ( 0.999993745883712)
/* +sqrt(R2); the sign is B1's. */
#define TA_TEST_REF_NORRIS_R   ( 0.99999687293696671)

/* Wilkinson's "nasty.dat", the classic reliability quiz for statistical
 * software, as used by scipy's linregress tests. X is 1..9 -- equally spaced --
 * so "regress each column on X" is exactly what the TA_LINEARREG family does
 * with period 9, and the columns are the cancellation stressors: BIG carries an
 * eight-digit offset under a unit spread, LITTLE a 1e-8 spread under a value of
 * 1. Read as PRICE series instead, BIG and LITTLE both carry a per-bar return
 * of ~1e-8, which is the regime TA_BETA's old absolute epsilon swallowed. */
#define TA_TEST_REF_WILKINSON_N 9
extern const double ta_test_ref_wilkinson_x     [TA_TEST_REF_WILKINSON_N];
extern const double ta_test_ref_wilkinson_round [TA_TEST_REF_WILKINSON_N];
extern const double ta_test_ref_wilkinson_big   [TA_TEST_REF_WILKINSON_N];
extern const double ta_test_ref_wilkinson_little[TA_TEST_REF_WILKINSON_N];
extern const double ta_test_ref_wilkinson_huge  [TA_TEST_REF_WILKINSON_N];
extern const double ta_test_ref_wilkinson_tiny  [TA_TEST_REF_WILKINSON_N];
extern const double ta_test_ref_wilkinson_zero  [TA_TEST_REF_WILKINSON_N];

/* The seven columns in the order the golden tables index them. */
#define TA_TEST_REF_WILKINSON_NB_SERIES 7
extern const double *const ta_test_ref_wilkinson_series[TA_TEST_REF_WILKINSON_NB_SERIES];
extern const char  *const ta_test_ref_wilkinson_names [TA_TEST_REF_WILKINSON_NB_SERIES];

/* pandas' rolling-corr adversarial arrays (GH#65739): a large value transiting
 * the window, an offset shared by the whole series, a sum driven negative by
 * cancellation, and an extreme dynamic range. The magnitudes of the last one
 * are rescaled from pandas' 1e308 to this library's declared input bound
 * (TA_REAL_MAX, 3e37) -- borrow the shape, not the literal, the domains differ.
 *
 * The offsets the test files add on top are not baked into the arrays because
 * each offset is a separate case with its own golden table. */
#define TA_TEST_REF_PD_OUTLIER_N 18
extern const double ta_test_ref_pd_outlier_x[TA_TEST_REF_PD_OUTLIER_N];
extern const double ta_test_ref_pd_outlier_y[TA_TEST_REF_PD_OUTLIER_N];
#define TA_TEST_REF_PD_SHARED_N 10
extern const double ta_test_ref_pd_shared_x[TA_TEST_REF_PD_SHARED_N];
extern const double ta_test_ref_pd_shared_y[TA_TEST_REF_PD_SHARED_N];
#define TA_TEST_REF_PD_NONAN_N 8
extern const double ta_test_ref_pd_nonan_x[TA_TEST_REF_PD_NONAN_N];
extern const double ta_test_ref_pd_nonan_y[TA_TEST_REF_PD_NONAN_N];
#define TA_TEST_REF_PD_EXTREME_N 11
extern const double ta_test_ref_pd_extreme_x[TA_TEST_REF_PD_EXTREME_N];
extern const double ta_test_ref_pd_extreme_y[TA_TEST_REF_PD_EXTREME_N];

/* pandas' rolling-var adversarial arrays: GH#47721 (a 1e10 value transiting a
 * window of 6) and GH#52407 (mixed tiny magnitudes that produced a NEGATIVE
 * variance). GH#42064 -- 1000 zeros behind a spike -- is built by its caller
 * rather than transcribed. */
#define TA_TEST_REF_PD_VAR47721_N 16
extern const double ta_test_ref_pd_var47721[TA_TEST_REF_PD_VAR47721_N];
#define TA_TEST_REF_PD_VAR52407_N 10
extern const double ta_test_ref_pd_var52407[TA_TEST_REF_PD_VAR52407_N];

/* The sliding-sum ladder (#251, for the #103 recurrence class). Four regimes in
 * one series so a single sweep meets all of them: an eight-digit level under a
 * one-cent spread (bars 0-11), a 20% print transiting the window (bar 12), the
 * level again (13-23), a 10:1 split (24-32), and an exactly flat tail (33-39)
 * where the least-squares slope is exactly zero.
 *
 * EVERY DISTURBANCE HERE IS ONE THE RUNNING SUMS CAN SHED, and that is the whole
 * design. An earlier draft put a 1e12 print at bar 12. It was a fair stressor and
 * the library's behaviour on it is real -- but because the sums never re-anchor
 * (issue #254) they keep ~ulp(1e12) of residue for the rest of the CALL, so the
 * only honest bound over bars 13-39 was ~1e-4 absolute. Measured, that let the
 * leg reading these goldens accept a slope 73x wrong and an angle 72 degrees
 * wrong: one violent print does not merely make its own window untestable, it
 * makes everything after it in the same call untestable, and the leg then reads
 * as coverage while checking almost nothing.
 *
 * A disturbance the sums CAN shed is worth more, because it makes RECOVERY an
 * assertable property: once the print leaves the window the error has to return
 * to the ordinary regime, and a tight bound can say so. The unrecoverable case is
 * not lost -- it is pinned where the measurement still works, by the
 * range-stability leg (test_linearreg.c L9), which compares a bar against the
 * same bar computed with NO history and is therefore tight by construction.
 *
 * The golden tables cover this series at TA_TEST_REF_LADDER_PERIODS. */
#define TA_TEST_REF_LADDER_N 40
extern const double ta_test_ref_ladder[TA_TEST_REF_LADDER_N];

/* The 60 integer tick counts issue #243 was reported on -- a real quoted
 * instrument's closes, in ticks. test_stddev.c, test_bbands.c and
 * test_correl.c each carried their own copy; the value of the series is that
 * multiplying it by a tick size walks a REAL price shape across twelve decades
 * without changing its shape. */
#define TA_TEST_REF_TICKS60_N 60
extern const int ta_test_ref_ticks60[TA_TEST_REF_TICKS60_N];

/* The NIST StRD univariate cancellation stressors NumAcc1..NumAcc4 are not
 * transcribed either -- they are three or 1001 copies of two values, so they
 * are built here. Fills `buf` and returns the count.
 *
 *   NumAcc1  {1e7+1, 1e7+2, 1e7+3}                     certified pop var 2/3
 *   NumAcc2  base 0,       500x1.1 500x1.3 1x1.2       certified pop var 10/1001
 *   NumAcc3  base 999999                               ditto
 *   NumAcc4  base 9999999                              ditto
 *
 * The certified values are the SAMPLE variance (divisor n-1); the population
 * value returned above is s^2*(n-1)/n, which is what TA-Lib computes.
 * https://www.itl.nist.gov/div898/strd/univ/homepage.html */
int ta_test_ref_numacc( int which /* 1..4 */, double *buf /* >= 1001 */ );

#endif /* TA_TEST_REFERENCE_H */
