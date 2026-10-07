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

/* For an output that divides by the smoothed value: at a period of 2 the
 * ratio needs more bars than the value it divides by.
 */
int ta_auto_stabilization_wilder_ratio(int K, int period) {
   return period > 1 ? K * period : 0;
}

/* One short at a period of 2 for K = 6 and K = 15: check a new level against
 * the envelope of rules_check.py before passing it.
 */
int ta_auto_stabilization_two_pole(int K, int period) {
   return ((K + 3) * (period + 2) + 8) / 9;
}

int ta_auto_stabilization_hilbert(int X) {
   return 80 + 50 * X;
}

/* The alpha depends on the window alone, so two starts close by 1 - alpha a
 * bar: the count is sized on how slowly price series let that run, and no
 * series needs more than 99 bars an e-fold, the alpha's floor of e^-4.6.
 */
int ta_auto_stabilization_frama(int K, int X, int root) {
   return 9 * (X + 4) * (root + 2) / 2 < 99 * K ? 9 * (X + 4) * (root + 2) / 2 : 99 * K;
}

/* ADOSC's two EMAs seed on one value, so the first differences of two starts
 * can cancel. The sum of min(fastest, slowest / 2^j) stands for fastest times
 * the log of the periods' ratio, and keeps the count non-decreasing in both.
 */
int ta_auto_stabilization_adosc(int fastest, int slowest) {
   return (15 * fastest + 3 * (
      min( slowest / 2, fastest )
      + min( slowest / 4, fastest )
      + min( slowest / 8, fastest )
      + min( slowest / 16, fastest )
      + min( slowest / 32, fastest )
      + min( slowest / 64, fastest )
      + min( slowest / 128, fastest )
      + min( slowest / 256, fastest )
      + min( slowest / 512, fastest )
      + min( slowest / 1024, fastest )
      + min( slowest / 2048, fastest )
      + min( slowest / 4096, fastest )
      + min( slowest / 8192, fastest )
      + min( slowest / 16384, fastest )
      + min( slowest / 32768, fastest )
      + min( slowest / 65536, fastest )
   ) + 7) / 8;
}

/* Saturates at TA_INDEX_MAX. */
int ta_auto_stabilization_vidya(int X, int period, int root) {
   return 2 * X * (period + 1) * root > 100000000 ? 100000000 : 2 * X * (period + 1) * root;
}
