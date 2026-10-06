/* Auto-Stabilization counts. K is the level's number of e-folds and X its digit
 * count: a count written in K bounds the seed's weight by e^-K, one written in
 * X is sized by measurement. Every count is non-decreasing in the period: MACD, STC, MAVP, APO, PPO, PVO,
 * ADOSC and MACDFIX index a shorter-period leg on that.
 */

int ta_auto_stabilization_ema(int K, int period) {
   return period > 1 ? (K * (period + 1) + 1) / 2 : 0;
}

int ta_auto_stabilization_wilder(int K, int period) {
   return period > 1 ? K * period : 0;
}

/* A repeated real pole of a two-pole filter of critical period `period`. */
int ta_auto_stabilization_two_pole(int K, int period) {
   return ((K + 5) * (period + 2) + 8) / 9;
}

int ta_auto_stabilization_hilbert(int X) {
   return 80 + 50 * X;
}

/* Saturates at TA_INDEX_MAX. */
int ta_auto_stabilization_vidya(int X, int period, int root) {
   return 2 * X * (period + 1) * root > 100000000 ? 100000000 : 2 * X * (period + 1) * root;
}
