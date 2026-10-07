/* Auto-Stabilization counts. K is the level's number of e-folds and X its digit
 * count: a count written in K bounds the seed's weight by e^-K, one written in
 * X is sized by measurement. Every count is non-decreasing in the period: MACD, STC, MAVP, APO, PPO, PVO,
 * ADOSC and MACDFIX index a shorter-period leg on that.
 */

int ta_auto_stabilization_ema(int K, int period) {
   return period > 1 ? (K * period + 1) / 2 : 0;
}

int ta_auto_stabilization_wilder(int K, int period) {
   return period > 1 ? (K * (2 * period - 1) + 1) / 2 : 0;
}

/* For an output that divides by the smoothed value. The half bar per e-fold
 * over the pole is what holds the level at a period of 2 or 3, where the ratio
 * shows a late difference larger than an early one.
 */
int ta_auto_stabilization_wilder_ratio(int K, int period) {
   return period > 1 ? K * period : 0;
}

/* A repeated real pole of a two-pole filter of critical period `period`: an
 * e-fold costs under (period + 2) / 9.76 bars, and the mismatch moves as
 * (A + B*k) * pole^k. Its worst case crosses zero right after the seed, so the
 * fixed part covers the linear term against the larger of the first difference
 * and the peak. That part grows as the log of K and the 9 pays for it, but
 * period 2 has no bar to spare: K = 6 and K = 15 are one short there. Check a
 * new level against the envelope before passing it.
 */
int ta_auto_stabilization_two_pole(int K, int period) {
   return ((K + 3) * (period + 2) + 8) / 9;
}

int ta_auto_stabilization_hilbert(int X) {
   return 80 + 50 * X;
}

/* Saturates at TA_INDEX_MAX. */
int ta_auto_stabilization_vidya(int X, int period, int root) {
   return 2 * X * (period + 1) * root > 100000000 ? 100000000 : 2 * X * (period + 1) * root;
}
