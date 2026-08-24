/* GENERATED FILE -- DO NOT EDIT.
 *
 * Produced by scripts/gen_test_reference.py from the datasets in
 * ta_test_reference.c, in exact rational arithmetic. Issue #251.
 *
 * Re-generate with:   scripts/gen_test_reference.py
 * Verify in place:    scripts/gen_test_reference.py --check
 */


#ifndef TA_TEST_REFERENCE_GOLDEN_H
#define TA_TEST_REFERENCE_GOLDEN_H

/* exact OLS slope of the transcribed doubles (NIST certifies 1.00211681802045 for the exact decimals) */
#define TA_TEST_REF_GOLDEN_NORRIS_B1 (1.0021168180204545)

/* exact OLS intercept (NIST certifies -0.262323073774029) */
#define TA_TEST_REF_GOLDEN_NORRIS_B0 (-0.26232307377402675)

/* exact Pearson r (NIST certifies R2 0.999993745883712, i.e. r 0.99999687293696671) */
#define TA_TEST_REF_GOLDEN_NORRIS_R (0.9999968729369666)

/* the periods the ladder tables below are indexed by */
#define TA_TEST_REF_GOLDEN_LADDER_PERIODS_N 4
extern const int ta_test_ref_golden_ladder_periods[TA_TEST_REF_GOLDEN_LADDER_PERIODS_N];

/* windows per period */
#define TA_TEST_REF_GOLDEN_LADDER_COUNTS_N 4
extern const int ta_test_ref_golden_ladder_counts[TA_TEST_REF_GOLDEN_LADDER_COUNTS_N];

/* pandas GH#65739 outlier_exit, period 9, no offset on y */
#define TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF0_N 10
extern const double ta_test_ref_golden_corr_outlier_off0[TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF0_N];

/* the same, y carrying a 1e13 shared offset */
#define TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF1E13_N 10
extern const double ta_test_ref_golden_corr_outlier_off1e13[TA_TEST_REF_GOLDEN_CORR_OUTLIER_OFF1E13_N];

/* pandas GH#65739 shared_offset 1e10, period 5 */
#define TA_TEST_REF_GOLDEN_CORR_SHARED_1E10_N 6
extern const double ta_test_ref_golden_corr_shared_1e10[TA_TEST_REF_GOLDEN_CORR_SHARED_1E10_N];

/* pandas GH#65739 shared_offset 1e14, period 5 */
#define TA_TEST_REF_GOLDEN_CORR_SHARED_1E14_N 6
extern const double ta_test_ref_golden_corr_shared_1e14[TA_TEST_REF_GOLDEN_CORR_SHARED_1E14_N];

/* pandas GH#65739 outlier_exit_no_nan, period 3 */
#define TA_TEST_REF_GOLDEN_CORR_NONAN_N 6
extern const double ta_test_ref_golden_corr_nonan[TA_TEST_REF_GOLDEN_CORR_NONAN_N];

/* pandas GH#65739 extreme_range rescaled to TA_REAL_MAX, period 5 */
#define TA_TEST_REF_GOLDEN_CORR_EXTREME_N 7
extern const double ta_test_ref_golden_corr_extreme[TA_TEST_REF_GOLDEN_CORR_EXTREME_N];

/* pandas GH#47721 (a 1e10 value transiting a window of 6), period 6 */
#define TA_TEST_REF_GOLDEN_VAR47721_N 11
extern const double ta_test_ref_golden_var47721[TA_TEST_REF_GOLDEN_VAR47721_N];

/* pandas GH#52407 (mixed tiny magnitudes -> negative variance), period 3 */
#define TA_TEST_REF_GOLDEN_VAR52407_N 8
extern const double ta_test_ref_golden_var52407[TA_TEST_REF_GOLDEN_VAR52407_N];

/* Wilkinson nasty.dat regressed on the bar index, period 9, series order X ROUND BIG LITTLE HUGE TINY ZERO */
#define TA_TEST_REF_GOLDEN_WILKINSON_SLOPE_N 7
extern const double ta_test_ref_golden_wilkinson_slope[TA_TEST_REF_GOLDEN_WILKINSON_SLOPE_N];

/* Wilkinson nasty.dat regressed on the bar index, period 9, series order X ROUND BIG LITTLE HUGE TINY ZERO */
#define TA_TEST_REF_GOLDEN_WILKINSON_INTERCEPT_N 7
extern const double ta_test_ref_golden_wilkinson_intercept[TA_TEST_REF_GOLDEN_WILKINSON_INTERCEPT_N];

/* Wilkinson nasty.dat regressed on the bar index, period 9, series order X ROUND BIG LITTLE HUGE TINY ZERO */
#define TA_TEST_REF_GOLDEN_WILKINSON_FIT_N 7
extern const double ta_test_ref_golden_wilkinson_fit[TA_TEST_REF_GOLDEN_WILKINSON_FIT_N];

/* Wilkinson nasty.dat regressed on the bar index, period 9, series order X ROUND BIG LITTLE HUGE TINY ZERO */
#define TA_TEST_REF_GOLDEN_WILKINSON_TSF_N 7
extern const double ta_test_ref_golden_wilkinson_tsf[TA_TEST_REF_GOLDEN_WILKINSON_TSF_N];

/* sliding-sum ladder, period 2 */
#define TA_TEST_REF_GOLDEN_LADDER_P2_SLOPE_N 39
extern const double ta_test_ref_golden_ladder_p2_slope[TA_TEST_REF_GOLDEN_LADDER_P2_SLOPE_N];

/* sliding-sum ladder, period 2 */
#define TA_TEST_REF_GOLDEN_LADDER_P2_INTERCEPT_N 39
extern const double ta_test_ref_golden_ladder_p2_intercept[TA_TEST_REF_GOLDEN_LADDER_P2_INTERCEPT_N];

/* sliding-sum ladder, period 2 */
#define TA_TEST_REF_GOLDEN_LADDER_P2_FIT_N 39
extern const double ta_test_ref_golden_ladder_p2_fit[TA_TEST_REF_GOLDEN_LADDER_P2_FIT_N];

/* sliding-sum ladder, period 2 */
#define TA_TEST_REF_GOLDEN_LADDER_P2_TSF_N 39
extern const double ta_test_ref_golden_ladder_p2_tsf[TA_TEST_REF_GOLDEN_LADDER_P2_TSF_N];

/* sliding-sum ladder population sigma, period 2 */
#define TA_TEST_REF_GOLDEN_LADDER_P2_SIGMA_N 39
extern const double ta_test_ref_golden_ladder_p2_sigma[TA_TEST_REF_GOLDEN_LADDER_P2_SIGMA_N];

/* sliding-sum ladder, period 5 */
#define TA_TEST_REF_GOLDEN_LADDER_P5_SLOPE_N 36
extern const double ta_test_ref_golden_ladder_p5_slope[TA_TEST_REF_GOLDEN_LADDER_P5_SLOPE_N];

/* sliding-sum ladder, period 5 */
#define TA_TEST_REF_GOLDEN_LADDER_P5_INTERCEPT_N 36
extern const double ta_test_ref_golden_ladder_p5_intercept[TA_TEST_REF_GOLDEN_LADDER_P5_INTERCEPT_N];

/* sliding-sum ladder, period 5 */
#define TA_TEST_REF_GOLDEN_LADDER_P5_FIT_N 36
extern const double ta_test_ref_golden_ladder_p5_fit[TA_TEST_REF_GOLDEN_LADDER_P5_FIT_N];

/* sliding-sum ladder, period 5 */
#define TA_TEST_REF_GOLDEN_LADDER_P5_TSF_N 36
extern const double ta_test_ref_golden_ladder_p5_tsf[TA_TEST_REF_GOLDEN_LADDER_P5_TSF_N];

/* sliding-sum ladder population sigma, period 5 */
#define TA_TEST_REF_GOLDEN_LADDER_P5_SIGMA_N 36
extern const double ta_test_ref_golden_ladder_p5_sigma[TA_TEST_REF_GOLDEN_LADDER_P5_SIGMA_N];

/* sliding-sum ladder, period 14 */
#define TA_TEST_REF_GOLDEN_LADDER_P14_SLOPE_N 27
extern const double ta_test_ref_golden_ladder_p14_slope[TA_TEST_REF_GOLDEN_LADDER_P14_SLOPE_N];

/* sliding-sum ladder, period 14 */
#define TA_TEST_REF_GOLDEN_LADDER_P14_INTERCEPT_N 27
extern const double ta_test_ref_golden_ladder_p14_intercept[TA_TEST_REF_GOLDEN_LADDER_P14_INTERCEPT_N];

/* sliding-sum ladder, period 14 */
#define TA_TEST_REF_GOLDEN_LADDER_P14_FIT_N 27
extern const double ta_test_ref_golden_ladder_p14_fit[TA_TEST_REF_GOLDEN_LADDER_P14_FIT_N];

/* sliding-sum ladder, period 14 */
#define TA_TEST_REF_GOLDEN_LADDER_P14_TSF_N 27
extern const double ta_test_ref_golden_ladder_p14_tsf[TA_TEST_REF_GOLDEN_LADDER_P14_TSF_N];

/* sliding-sum ladder population sigma, period 14 */
#define TA_TEST_REF_GOLDEN_LADDER_P14_SIGMA_N 27
extern const double ta_test_ref_golden_ladder_p14_sigma[TA_TEST_REF_GOLDEN_LADDER_P14_SIGMA_N];

/* sliding-sum ladder, period 30 */
#define TA_TEST_REF_GOLDEN_LADDER_P30_SLOPE_N 11
extern const double ta_test_ref_golden_ladder_p30_slope[TA_TEST_REF_GOLDEN_LADDER_P30_SLOPE_N];

/* sliding-sum ladder, period 30 */
#define TA_TEST_REF_GOLDEN_LADDER_P30_INTERCEPT_N 11
extern const double ta_test_ref_golden_ladder_p30_intercept[TA_TEST_REF_GOLDEN_LADDER_P30_INTERCEPT_N];

/* sliding-sum ladder, period 30 */
#define TA_TEST_REF_GOLDEN_LADDER_P30_FIT_N 11
extern const double ta_test_ref_golden_ladder_p30_fit[TA_TEST_REF_GOLDEN_LADDER_P30_FIT_N];

/* sliding-sum ladder, period 30 */
#define TA_TEST_REF_GOLDEN_LADDER_P30_TSF_N 11
extern const double ta_test_ref_golden_ladder_p30_tsf[TA_TEST_REF_GOLDEN_LADDER_P30_TSF_N];

/* sliding-sum ladder population sigma, period 30 */
#define TA_TEST_REF_GOLDEN_LADDER_P30_SIGMA_N 11
extern const double ta_test_ref_golden_ladder_p30_sigma[TA_TEST_REF_GOLDEN_LADDER_P30_SIGMA_N];

#endif /* TA_TEST_REFERENCE_GOLDEN_H */
