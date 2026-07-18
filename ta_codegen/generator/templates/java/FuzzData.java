// FuzzData.java — bit-exact port of src/tools/ta_regtest/fuzz_data.h.
// Deterministic input generator: every double is bit-for-bit identical to the C
// generator (Java never contracts a*b+c into FMA, matching FP_CONTRACT OFF).
// Unsigned 64-bit C arithmetic maps to Java long (wrapping * and ^ identical);
// logical right shifts use >>>. Default package, javac 21, no imports.

class FuzzData {

   // ---- Data shapes (keep FUZZ_NSHAPES last). ----
   static final int FUZZ_RANDWALK = 0;  // geometric random walk around 100 (typical prices)
   static final int FUZZ_MONO_UP = 1;   // strictly increasing
   static final int FUZZ_MONO_DOWN = 2; // strictly decreasing
   static final int FUZZ_CONSTANT = 3;  // flat O=H=L=C (degenerate: hh==ll, zero variance)
   static final int FUZZ_TIE_HEAVY = 4; // small integer set — many equal values / ties
   static final int FUZZ_EXTREME = 5;   // alternating huge (1e9) / tiny (1e-7) magnitudes
   static final int FUZZ_WITH_ZEROS = 6;// sprinkled 0.0 / -0.0 and small signed values
   static final int FUZZ_CANDLE = 7;    // inside-bar cascades + breakouts + confirmations
   static final int FUZZ_ZEROSUM = 8;   // symmetric high=-low bars: high+low == 0 EXACTLY
   static final int FUZZ_NSHAPES = 9;

   // ---- splitmix64 PRNG (deterministic, self-contained) ----
   static long fuzzSmNext(long[] s) {
      long z = (s[0] += 0x9E3779B97F4A7C15L);
      z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
      z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
      return z ^ (z >>> 31);
   }

   // Uniform double in [0,1) from the top 53 bits (>>> 11 fits 53 bits: exact).
   static double fuzzSmUnit(long[] s) {
      return (double) (fuzzSmNext(s) >>> 11) * (1.0 / 9007199254740992.0);
   }

   // ---- FUZZ_CANDLE: deterministic, pattern-rich inside-bar OHLC ----
   // One candle bar (clamped to a valid candle); writes at index p only when p < n.
   static int fuzzCdlBar(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                         int p, int n, double O, double H, double L, double Cl) {
      double hi = H, lo = L;
      if (hi < O) hi = O;
      if (hi < Cl) hi = Cl; // clamp to a valid candle
      if (lo > O) lo = O;
      if (lo > Cl) lo = Cl;
      if (p < n) {
         o[p] = O; h[p] = hi; l[p] = lo; c[p] = Cl; v[p] = 1000.0; oi[p] = 100.0;
         p++;
      }
      return p;
   }

   static int fuzzCdlFlat(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                          int p, int n, int k, double base) {
      while (k-- > 0) p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 1.0, base - 1.0, base);
      return p;
   }

   // Hikkake window: 3 bars + optional confirm. dir +1 bull/-1 bear;
   // brk 0=intact,1=break P1,2=break P2,3/4=break the breakout; conf
   // 0=none,1=confirm next bar,2=confirm one bar too late (expired).
   static int fuzzCdlHikkake(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                             int p, int n, double base, double w, int dir, int brk, int conf) {
      double H1 = base + w, L1 = base - w;
      double H2 = (brk == 1) ? base + w + 0.5 : base + 0.6 * w;
      double L2 = (brk == 2) ? base - w - 0.5 : base - 0.6 * w;
      double H3, L3, cc;
      if (dir > 0) {
         H3 = (brk == 3) ? H2 + 0.4 * w : H2 - 0.4 * w;
         L3 = L2 - 0.4 * w;
      } else {
         H3 = (brk == 4) ? H2 - 0.4 * w : H2 + 0.4 * w;
         L3 = L2 + 0.4 * w;
      }
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, H1, L1, base);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, H2, L2, base);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, H3, L3, base);
      if (conf == 2) p = fuzzCdlFlat(o, h, l, c, v, oi, p, n, 3, base);
      if (conf != 0) {
         cc = dir > 0 ? H2 + w : L2 - w;
         p = fuzzCdlBar(o, h, l, c, v, oi, p, n, cc, cc + 0.5, cc - 0.5, cc);
      }
      return p;
   }

   // Modified-hikkake window: 4 nested/breakout bars + optional confirm.
   static int fuzzCdlHikkakemod(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                int p, int n, double base, double w, int dir, int brk, int conf) {
      double H1 = base + w, L1 = base - w;
      double H2 = (brk == 1) ? base + w + 0.5 : base + 0.7 * w;
      double L2 = (brk == 1) ? base - w - 0.5 : base - 0.7 * w;
      double H3 = (brk == 2) ? H2 + 0.3 * w : base + 0.45 * w;
      double L3 = (brk == 2) ? L2 - 0.3 * w : base - 0.45 * w;
      double H4, L4, C2, cc;
      if (dir > 0) {
         H4 = (brk == 3) ? H3 + 0.3 * w : H3 - 0.3 * w;
         L4 = L3 - 0.3 * w;
      } else {
         H4 = (brk == 3) ? H3 - 0.3 * w : H3 + 0.3 * w;
         L4 = L3 + 0.3 * w;
      }
      C2 = (brk == 4) ? base : (dir > 0 ? L2 : H2); // 2nd close near low/high
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, H1, L1, base);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, H2, L2, C2);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, H3, L3, base);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, H4, L4, base);
      if (conf != 0) {
         cc = dir > 0 ? H3 + w : L3 - w;
         p = fuzzCdlBar(o, h, l, c, v, oi, p, n, cc, cc + 0.5, cc - 0.5, cc);
      }
      return p;
   }

   // ---- FUZZ_CANDLE deterministic pattern catalog (issue #109) ----
   // Neutral primer: k alternating small-body bars around base.
   static int fuzzCdlPrimer(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                            int p, int n, int k, double base, double bd, double hr) {
      int i;
      for (i = 0; i < k; i++) {
         double O = ((i & 1) != 0) ? base : base + bd;
         double C = ((i & 1) != 0) ? base + bd : base;
         p = fuzzCdlBar(o, h, l, c, v, oi, p, n, O, base + bd + hr, base - hr, C);
      }
      return p;
   }

   // CDL2CROWS (bearish, -100 on the 3rd candle).
   static int fuzzCdl2crows(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                            int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 12.5, base - 0.5, base + 12.0);        // 1st white long
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 18.0, base + 18.5, base + 13.5, base + 14.0); // 2nd black gap up
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 16.0, base + 16.5, base + 5.5, base + 6.0);   // 3rd black inside
      return p;
   }

   // CDL3BLACKCROWS (bearish, -100 on the 3rd crow).
   static int fuzzCdl3blackcrows(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                 int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 11, base - 1, base + 10);      // i-3: white long body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 8, base + 8, base, base);            // i-2: 1st black
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 5, base + 5, base - 5, base - 5);    // i-1: 2nd black inside
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 2, base + 2, base - 8, base - 8);    // i:   3rd black inside
      return p;
   }

   // CDL3WHITESOLDIERS (bullish, +100 on the 3rd soldier).
   static int fuzzCdl3whitesoldiers(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                    int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 10.2, base - 0.2, base + 10.0);         // 1st white long
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 5.0, base + 15.2, base + 4.8, base + 15.0);   // 2nd white
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 10.0, base + 20.2, base + 9.8, base + 20.0);  // 3rd white
      return p;
   }

   // CDL3STARSINSOUTH (bullish, +100 on the 3rd candle).
   static int fuzzCdl3starsinsouth(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                   int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 8.0, base + 8.0, base - 12.0, base);        // 1st black long + long lower shadow
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 6.0, base + 6.0, base - 4.0, base + 2.0);   // 2nd black smaller body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 1.0, base + 1.0, base, base);               // 3rd black tiny marubozu
      return p;
   }

   // CDL3LINESTRIKE (three-white branch, +100 on the strike).
   static int fuzzCdl3linestrike(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                 int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 5.0, base - 1.0, base + 4.0);          // i-3: 1st white soldier
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 2.0, base + 7.0, base + 1.0, base + 6.0);    // i-2: 2nd white
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 4.0, base + 9.0, base + 3.0, base + 8.0);    // i-1: 3rd white
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 10.0, base + 11.0, base - 3.0, base - 2.0);  // i:   4th black strike
      return p;
   }

   // CDLCONCEALBABYSWALL (bullish, +100 on the 4th candle).
   static int fuzzCdlConcealbabyswall(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                      int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base, base - 3.0, base - 3.0);                 // 1st black marubozu
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 4.0, base - 4.0, base - 7.0, base - 7.0);     // 2nd black marubozu
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 9.0, base - 6.0, base - 12.0, base - 11.0);   // 3rd black gapdown+shadow
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 5.0, base - 4.0, base - 16.0, base - 15.0);   // 4th black engulfs 3rd
      return p;
   }

   // CDLMATHOLD (bullish, +100 on the 5th candle).
   static int fuzzCdlMathold(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                             int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 21.0, base - 1.0, base + 20.0);           // c1 white long
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 31.0, base + 32.0, base + 29.0, base + 30.0);   // c2 short black gap up
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 16.0, base + 17.0, base + 14.0, base + 15.0);   // c3 short, in 1st range
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 14.0, base + 15.0, base + 12.0, base + 13.0);   // c4 short, falling
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 35.0, base + 41.0, base + 34.0, base + 40.0);   // c5 white breakout
      return p;
   }

   // CDLRISEFALL3METHODS (rising branch, +100 on the 5th candle).
   static int fuzzCdlRisefall3methods(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                      int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 12.5, base - 0.5, base + 12.0);        // 1st long white
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 10.0, base + 10.5, base + 8.5, base + 9.0);  // 2nd small black
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 8.0, base + 8.5, base + 6.5, base + 7.0);    // 3rd small black falling
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 6.0, base + 6.5, base + 4.5, base + 5.0);    // 4th small black falling
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 6.0, base + 20.5, base + 5.5, base + 20.0);  // 5th long white breakout
      return p;
   }

   // CDLADVANCEBLOCK (bearish, -100 on the 3rd candle).
   static int fuzzCdlAdvanceblock(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                  int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 6.0, base, base + 6.0);               // 1st white long body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 5.0, base + 7.0, base + 5.0, base + 7.0);   // 2nd white, shorter
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 7.0, base + 8.0, base + 7.0, base + 8.0);   // 3rd white, shortest
      return p;
   }

   // CDLINNECK (bearish, -100 on the 2nd candle). Wider primer (hr=6).
   static int fuzzCdlInneck(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                            int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 6.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 10.0, base + 12.0, base - 1.0, base);        // 1st long black
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 5.0, base + 1.0, base - 6.0, base + 0.35);   // 2nd white into neck
      return p;
   }

   // CDLUNIQUE3RIVER (bullish, +100 on the 3rd candle).
   static int fuzzCdlUnique3river(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                  int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 10.0, base + 11.0, base - 12.0, base - 10.0); // 1st black long body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 5.0, base + 6.0, base - 15.0, base - 5.0);    // 2nd black harami+low
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 4.0, base - 2.0, base - 5.0, base - 3.0);     // 3rd white short body
      return p;
   }

   // CDLKICKING: 2nd pattern bar (index 13) = +100 (bullish).
   static int fuzzCdlKicking(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                             int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 20, base + 20, base, base);                  // idx12: BLACK long marubozu
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 30, base + 50, base + 30, base + 50);        // idx13: WHITE long marubozu, gap up
      return p;
   }

   // CDLKICKINGBYLENGTH: 2nd pattern bar (index 13) = +100 (bullish).
   static int fuzzCdlKickingbylength(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                     int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 40, base + 40, base + 20, base + 20);        // bar12: BLACK long marubozu
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 45, base + 70, base + 45, base + 70);        // bar13: WHITE long marubozu, gap up
      return p;
   }

   // CDLDARKCLOUDCOVER: 2nd pattern bar (index 13) = -100.
   static int fuzzCdlDarkcloudcover(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                    int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 21, base - 1, base + 20);              // idx12: white LONG body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 30, base + 31, base + 4, base + 5);          // idx13: black, closes below midpoint
      return p;
   }

   // CDLPIERCING: 2nd pattern candle (buffer index 13) fires +100.
   static int fuzzCdlPiercing(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                              int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 20, base + 21, base - 1, base);              // 1st: black LONG body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 10, base + 16, base - 11, base + 15);        // 2nd: white LONG body, pierces
      return p;
   }

   // CDLTHRUSTING: 2nd pattern bar (index 13) = -100 (always bearish).
   static int fuzzCdlThrusting(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                               int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 10, base + 11, base - 1, base);              // 1st: BLACK LONG body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 10, base + 4, base - 12, base + 2.5);        // 2nd: WHITE thrusting
      return p;
   }

   // CDLHOMINGPIGEON: 2nd pattern bar (index 13) = +100 (always bullish).
   static int fuzzCdlHomingpigeon(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                  int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 20, base + 21, base - 1, base);              // idx12: black LONG body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 7, base + 8, base + 4, base + 5);            // idx13: black SHORT body, engulfed
      return p;
   }

   // CDL3INSIDE: 3rd pattern bar (index 14) = -100 (bearish three-inside-down).
   static int fuzzCdl3inside(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                             int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 12.5, base - 0.5, base + 12);          // 1st: long WHITE body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 6, base + 7.5, base + 5.5, base + 7);        // 2nd: SHORT body, engulfed
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 8, base + 8.5, base - 2.5, base - 2);        // 3rd: BLACK, closes below 1st open
      return p;
   }

   // CDLIDENTICAL3CROWS: 3rd pattern bar (index 14) = -100.
   static int fuzzCdlIdentical3crows(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                     int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base, base - 3, base - 3);                    // 1st crow: black, lower shadow 0
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 3, base - 3, base - 6, base - 6);            // 2nd crow: opens at 1st close
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 6, base - 6, base - 9, base - 9);            // 3rd crow: opens at 2nd close
      return p;
   }

   // CDLSTALLEDPATTERN: 3rd pattern bar (index 14) = -100.
   static int fuzzCdlStalledpattern(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                    int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 12, base, base + 12);                  // 1st: white LONG body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 8, base + 25, base + 8, base + 25);          // 2nd: white LONG body, no upper shadow
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 25, base + 26, base + 25, base + 26);        // 3rd: small white on 2nd's shoulder
      return p;
   }

   // CDLUPSIDEGAP2CROWS: 3rd pattern bar (index 14) = -100.
   static int fuzzCdlUpsidegap2crows(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                     int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 10.5, base - 0.5, base + 10);          // bar12: white LONG body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 16, base + 16.5, base + 14.5, base + 15);    // bar13: black SHORT body, gap up
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 18, base + 18.5, base + 11.5, base + 12);    // bar14: black
      return p;
   }

   // CDLBREAKAWAY: 5th pattern bar (index 16) = +100 (1st-candle-black branch).
   static int fuzzCdlBreakaway(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                               int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 20, base + 21, base + 7, base + 8);          // idx12: black LONG body
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 4, base + 5, base - 1, base);                // idx13: black, body gaps DOWN
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 2, base, base - 5, base - 4);                // idx14: lower high & low than 2nd
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base - 7, base - 6, base - 11, base - 9);           // idx15: black, lower high & low than 3rd
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 5, base + 7, base + 4, base + 6);            // idx16: white, close in gap
      return p;
   }

   // CDLLADDERBOTTOM: 5th pattern bar (index 16) = +100 (always bullish).
   static int fuzzCdlLadderbottom(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                  int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 20, base + 21, base + 14, base + 15);        // i-4: black, highest open & close
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 15, base + 16, base + 9, base + 10);         // i-3: black, lower
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 10, base + 11, base + 4, base + 5);          // i-2: black, lower
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 5, base + 12, base - 2, base);               // i-1: black, big upper shadow
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 6, base + 15, base + 6, base + 15);          // i:   white breakout
      return p;
   }

   // CDLXSIDEGAP3METHODS: 3rd pattern bar (index 14) = +100 (bullish).
   static int fuzzCdlXsidegap3methods(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                      int p, int n, double base) {
      p = fuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base, base + 6, base - 1, base + 5);                // 1st: white body [100,105]
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 8, base + 14, base + 7, base + 13);          // 2nd: white body, gap up
      p = fuzzCdlBar(o, h, l, c, v, oi, p, n, base + 10, base + 11, base + 1, base + 2);          // 3rd: black, fills the gap
      return p;
   }

   // Lay the deterministic per-family catalog (issue #109); one entry per pattern.
   static int fuzzCdlCatalog(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                             int p, int n) {
      p = fuzzCdl2crows(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdl3blackcrows(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdl3whitesoldiers(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdl3starsinsouth(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdl3linestrike(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlConcealbabyswall(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlMathold(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlRisefall3methods(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlAdvanceblock(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlInneck(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlUnique3river(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlKicking(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlKickingbylength(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlDarkcloudcover(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlPiercing(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlThrusting(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlHomingpigeon(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdl3inside(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlIdentical3crows(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlStalledpattern(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlUpsidegap2crows(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlBreakaway(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlLadderbottom(o, h, l, c, v, oi, p, n, 100.0);
      p = fuzzCdlXsidegap3methods(o, h, l, c, v, oi, p, n, 100.0);
      return p;
   }

   static void fuzzCandleGen(int seed, int n,
                             double[] o, double[] h, double[] l, double[] c,
                             double[] v, double[] oi) {
      long[] s = new long[1];
      s[0] = 0x243F6A8885A308D3L
             ^ (((long) seed & 0xFFFFFFFFL) * 0xD1B54A32D192ED03L);
      int p = fuzzCdlFlat(o, h, l, c, v, oi, 0, n, 6, 100.0);
      p = fuzzCdlCatalog(o, h, l, c, v, oi, p, n); // deterministic per-family windows (#109)
      while (p < n - 16) {
         double base = 100.0 + (fuzzSmUnit(s) - 0.5) * 40.0;
         double w = 8.0 + fuzzSmUnit(s) * 20.0;
         int variant = (int) (fuzzSmUnit(s) * 10.0);
         int dir = ((fuzzSmNext(s) & 1L) != 0) ? 1 : -1;
         switch (variant) {
            case 0:  p = fuzzCdlHikkake(o, h, l, c, v, oi, p, n, base, w, dir, 0, 1); break;
            case 1:  p = fuzzCdlHikkake(o, h, l, c, v, oi, p, n, base, w, dir, 0, 2); break;
            case 2:  p = fuzzCdlHikkake(o, h, l, c, v, oi, p, n, base, w, dir, 1, 0); break;
            case 3:  p = fuzzCdlHikkake(o, h, l, c, v, oi, p, n, base, w, dir, 2, 0); break;
            case 4:  p = fuzzCdlHikkake(o, h, l, c, v, oi, p, n, base, w, dir, dir > 0 ? 3 : 4, 0); break;
            case 5:  p = fuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 0, 1); break;
            case 6:  p = fuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 0, 0); break;
            case 7:  p = fuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 1, 0); break;
            case 8:  p = fuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 3, 0); break;
            default: p = fuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 4, 1); break;
         }
         p = fuzzCdlFlat(o, h, l, c, v, oi, p, n, 6, base);
      }
      while (p < n) p = fuzzCdlFlat(o, h, l, c, v, oi, p, n, 1, 100.0);
   }

   // ---- FUZZ_ZEROSUM: bars whose high+low is exactly zero ----
   static void fuzzZerosumGen(int seed, int n,
                              double[] o, double[] h, double[] l, double[] c,
                              double[] v, double[] oi) {
      long[] s = new long[1];
      s[0] = 0x243F6A8885A308D3L
             ^ (((long) seed & 0xFFFFFFFFL) * 0xD1B54A32D192ED03L)
             ^ ((long) FUZZ_ZEROSUM << 32);
      int i;
      for (i = 0; i < n; i++) {
         double r = fuzzSmUnit(s);
         double open, hi, lo, close, a, base, w, t;
         if (r < 0.34) {
            // symmetric zero-sum bar: hi + lo == +0.0 exactly (else branch).
            a = 1.0 + fuzzSmUnit(s) * 50.0;
            hi = a; lo = -a; open = 0.0; close = 0.0;
         } else if (r < 0.50) {
            // all-zero degenerate bar: hi + lo == 0 with hi == lo == 0.
            hi = 0.0; lo = 0.0; open = 0.0; close = 0.0;
         } else {
            // ordinary positive bar around 100 (hi + lo != 0 -> then branch).
            base = 90.0 + fuzzSmUnit(s) * 20.0;
            w = 0.5 + fuzzSmUnit(s) * 5.0;
            close = base;
            t = fuzzSmUnit(s) - 0.5;
            open = base + t;
            hi = (open > close) ? open : close;
            hi = hi + w;
            lo = (open < close) ? open : close;
            lo = lo - w;
         }
         o[i] = open; h[i] = hi; l[i] = lo; c[i] = close;
         t = fuzzSmUnit(s) * 1000.0;
         v[i] = 1000.0 + t;
         t = fuzzSmUnit(s) * 100.0;
         oi[i] = 100.0 + t;
      }
   }

   // Fill OHLCV+OI arrays (length n) from (shape,seed). high>=max(o,c),
   // low<=min(o,c). Mul/add split into statements so nothing contracts.
   static void fuzzGen(int shape, int seed, int n,
                       double[] o, double[] h, double[] l, double[] c,
                       double[] v, double[] oi) {
      if (shape == FUZZ_CANDLE) { fuzzCandleGen(seed, n, o, h, l, c, v, oi); return; }
      if (shape == FUZZ_ZEROSUM) { fuzzZerosumGen(seed, n, o, h, l, c, v, oi); return; }
      long[] s = new long[1];
      s[0] = 0x243F6A8885A308D3L
             ^ (((long) seed & 0xFFFFFFFFL) * 0xD1B54A32D192ED03L)
             ^ (((long) shape & 0xFFFFFFFFL) << 32);
      double walk = 100.0;
      int i;
      for (i = 0; i < n; i++) {
         double close, open, hi, lo, mag, t;

         if (shape == FUZZ_CONSTANT) {
            o[i] = h[i] = l[i] = c[i] = 42.0;
            v[i] = 1000000.0; oi[i] = 10000.0;
            continue;
         }
         if (shape == FUZZ_TIE_HEAVY) {
            close = (double) (3 + (int) (fuzzSmUnit(s) * 5.0)); // {3..7}
            o[i] = close;
            c[i] = close;
            h[i] = close + (double) (fuzzSmNext(s) & 1L);
            l[i] = close - (double) (fuzzSmNext(s) & 1L);
            v[i] = (double) (1 + (int) (fuzzSmUnit(s) * 4.0)) * 1000.0;
            oi[i] = 1000.0;
            continue;
         }

         switch (shape) {
            case FUZZ_MONO_UP:
               t = (double) i * 0.5; close = 10.0 + t; break;
            case FUZZ_MONO_DOWN:
               t = (double) i * 0.25; close = 500.0 - t; break;
            case FUZZ_EXTREME:
               t = fuzzSmUnit(s);
               close = ((fuzzSmNext(s) & 1L) != 0) ? (1.0 + t) * 1.0e9
                                                   : (1.0 + t) * 1.0e-7;
               break;
            case FUZZ_WITH_ZEROS: {
               double r = fuzzSmUnit(s);
               if (r < 0.15)      close = 0.0;
               else if (r < 0.30) close = -0.0;
               else             { t = r - 0.5; close = t * 8.0; }
               break;
            }
            case FUZZ_RANDWALK:
            default:
               t = fuzzSmUnit(s) - 0.5;
               t = t * 0.04;
               walk = walk * (1.0 + t);
               close = walk;
               break;
         }

         mag = Math.abs(close) * 0.01 + 0.001;
         t = fuzzSmUnit(s) - 0.5;
         t = t * mag;
         open = close + t;
         hi = (open > close) ? open : close;
         t = fuzzSmUnit(s) * mag;
         hi = hi + t;
         lo = (open < close) ? open : close;
         t = fuzzSmUnit(s) * mag;
         lo = lo - t;

         o[i] = open; h[i] = hi; l[i] = lo; c[i] = close;
         t = fuzzSmUnit(s) * 1.0e6;
         v[i] = 1000.0 + t;
         t = fuzzSmUnit(s) * 1.0e4;
         oi[i] = 100.0 + t;
      }
   }
}
