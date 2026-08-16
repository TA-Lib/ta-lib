// FuzzData.cs — bit-exact port of src/tools/ta_regtest/fuzz_data.h.
// Deterministic input generator: every double is bit-for-bit identical to the C
// generator (the CLR never contracts a*b+c into an FMA, matching the C side's
// FP_CONTRACT OFF / -ffp-contract=off).
//
// Unsigned 64-bit C arithmetic maps to C# `ulong`: wrapping `*`, `+`, `^` and a
// LOGICAL `>>` are what `ulong` already gives. The whole PRNG body still sits in
// `unchecked` so the port cannot change meaning if a consumer project ever turns
// on CheckForOverflowUnderflow.
//
// Global namespace, no `using` directives and no namespace of its own, so this
// file drops beside (or is appended to) the generated server exactly the way
// templates/java/FuzzData.java does. `internal` throughout: nothing here is part
// of the shipped surface.

/// <summary>Deterministic OHLCV+OI input generator, a bit-exact port of the C
/// <c>fuzz_data.h</c> so a (shape, seed, n) tuple reproduces the same doubles in
/// every language server.</summary>
internal static class FuzzData
{
    // ---- Data shapes (keep FuzzNShapes last). ----

    /// <summary>Geometric random walk around 100 (typical prices).</summary>
    internal const int FuzzRandwalk = 0;
    /// <summary>Strictly increasing.</summary>
    internal const int FuzzMonoUp = 1;
    /// <summary>Strictly decreasing.</summary>
    internal const int FuzzMonoDown = 2;
    /// <summary>Flat O=H=L=C (degenerate: hh==ll, zero variance).</summary>
    internal const int FuzzConstant = 3;
    /// <summary>Small integer set — many equal values / ties.</summary>
    internal const int FuzzTieHeavy = 4;
    /// <summary>Alternating huge (1e9) / tiny (1e-7) magnitudes.</summary>
    internal const int FuzzExtreme = 5;
    /// <summary>Sprinkled 0.0 / -0.0 and small signed values.</summary>
    internal const int FuzzWithZeros = 6;
    /// <summary>Inside-bar cascades + breakouts + confirmations.</summary>
    internal const int FuzzCandle = 7;
    /// <summary>Symmetric high=-low bars: high+low == 0 EXACTLY.</summary>
    internal const int FuzzZerosum = 8;
    /// <summary>Number of shapes.</summary>
    internal const int FuzzNShapes = 9;

    // ---- splitmix64 PRNG (deterministic, self-contained) ----

    /// <summary>One splitmix64 step; advances the state and returns the mixed
    /// 64-bit value.</summary>
    internal static ulong FuzzSmNext(ref ulong s)
    {
        unchecked
        {
            ulong z = (s += 0x9E3779B97F4A7C15UL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9UL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBUL;
            return z ^ (z >> 31);
        }
    }

    /// <summary>Uniform double in [0,1) from the top 53 bits (the shift leaves a
    /// value below 2^53, so the ulong to double conversion is exact).</summary>
    internal static double FuzzSmUnit(ref ulong s)
    {
        return (double)(FuzzSmNext(ref s) >> 11) * (1.0 / 9007199254740992.0);
    }

    // ---- FuzzCandle: deterministic, pattern-rich inside-bar OHLC ----

    /// <summary>One candle bar (clamped to a valid candle); writes at index p
    /// only when p is still below n.</summary>
    internal static int FuzzCdlBar(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                   int p, int n, double O, double H, double L, double Cl)
    {
        double hi = H, lo = L;
        if (hi < O) hi = O;
        if (hi < Cl) hi = Cl; // clamp to a valid candle
        if (lo > O) lo = O;
        if (lo > Cl) lo = Cl;
        if (p < n)
        {
            o[p] = O; h[p] = hi; l[p] = lo; c[p] = Cl; v[p] = 1000.0; oi[p] = 100.0;
            p++;
        }
        return p;
    }

    /// <summary>k flat filler bars around baseVal.</summary>
    internal static int FuzzCdlFlat(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                    int p, int n, int k, double baseVal)
    {
        while (k-- > 0) p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 1.0, baseVal - 1.0, baseVal);
        return p;
    }

    /// <summary>Hikkake window: 3 bars + optional confirm. dir +1 bull/-1 bear;
    /// brk 0=intact,1=break P1,2=break P2,3/4=break the breakout; conf
    /// 0=none,1=confirm next bar,2=confirm one bar too late (expired).</summary>
    internal static int FuzzCdlHikkake(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                       int p, int n, double baseVal, double w, int dir, int brk, int conf)
    {
        double H1 = baseVal + w, L1 = baseVal - w;
        double H2 = (brk == 1) ? baseVal + w + 0.5 : baseVal + 0.6 * w;
        double L2 = (brk == 2) ? baseVal - w - 0.5 : baseVal - 0.6 * w;
        double H3, L3, cc;
        if (dir > 0)
        {
            H3 = (brk == 3) ? H2 + 0.4 * w : H2 - 0.4 * w;
            L3 = L2 - 0.4 * w;
        }
        else
        {
            H3 = (brk == 4) ? H2 - 0.4 * w : H2 + 0.4 * w;
            L3 = L2 + 0.4 * w;
        }
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, H1, L1, baseVal);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, H2, L2, baseVal);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, H3, L3, baseVal);
        if (conf == 2) p = FuzzCdlFlat(o, h, l, c, v, oi, p, n, 3, baseVal);
        if (conf != 0)
        {
            cc = dir > 0 ? H2 + w : L2 - w;
            p = FuzzCdlBar(o, h, l, c, v, oi, p, n, cc, cc + 0.5, cc - 0.5, cc);
        }
        return p;
    }

    /// <summary>Modified-hikkake window: 4 nested/breakout bars + optional
    /// confirm.</summary>
    internal static int FuzzCdlHikkakemod(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                          int p, int n, double baseVal, double w, int dir, int brk, int conf)
    {
        double H1 = baseVal + w, L1 = baseVal - w;
        double H2 = (brk == 1) ? baseVal + w + 0.5 : baseVal + 0.7 * w;
        double L2 = (brk == 1) ? baseVal - w - 0.5 : baseVal - 0.7 * w;
        double H3 = (brk == 2) ? H2 + 0.3 * w : baseVal + 0.45 * w;
        double L3 = (brk == 2) ? L2 - 0.3 * w : baseVal - 0.45 * w;
        double H4, L4, C2, cc;
        if (dir > 0)
        {
            H4 = (brk == 3) ? H3 + 0.3 * w : H3 - 0.3 * w;
            L4 = L3 - 0.3 * w;
        }
        else
        {
            H4 = (brk == 3) ? H3 - 0.3 * w : H3 + 0.3 * w;
            L4 = L3 + 0.3 * w;
        }
        C2 = (brk == 4) ? baseVal : (dir > 0 ? L2 : H2); // 2nd close near low/high
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, H1, L1, baseVal);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, H2, L2, C2);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, H3, L3, baseVal);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, H4, L4, baseVal);
        if (conf != 0)
        {
            cc = dir > 0 ? H3 + w : L3 - w;
            p = FuzzCdlBar(o, h, l, c, v, oi, p, n, cc, cc + 0.5, cc - 0.5, cc);
        }
        return p;
    }

    // ---- FuzzCandle deterministic pattern catalog (issue #109) ----

    /// <summary>Neutral primer: k alternating small-body bars around baseVal, so
    /// the following window's candle-setting averages do not depend on its
    /// neighbours.</summary>
    internal static int FuzzCdlPrimer(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                      int p, int n, int k, double baseVal, double bd, double hr)
    {
        int i;
        for (i = 0; i < k; i++)
        {
            double O = ((i & 1) != 0) ? baseVal : baseVal + bd;
            double C = ((i & 1) != 0) ? baseVal + bd : baseVal;
            p = FuzzCdlBar(o, h, l, c, v, oi, p, n, O, baseVal + bd + hr, baseVal - hr, C);
        }
        return p;
    }

    /// <summary>CDL2CROWS (bearish, -100 on the 3rd candle).</summary>
    internal static int FuzzCdl2crows(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                      int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 12.5, baseVal - 0.5, baseVal + 12.0);            // 1st white long
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 18.0, baseVal + 18.5, baseVal + 13.5, baseVal + 14.0);    // 2nd black gap up
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 16.0, baseVal + 16.5, baseVal + 5.5, baseVal + 6.0);      // 3rd black inside
        return p;
    }

    /// <summary>CDL3BLACKCROWS (bearish, -100 on the 3rd crow).</summary>
    internal static int FuzzCdl3blackcrows(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                           int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 11, baseVal - 1, baseVal + 10);      // i-3: white long body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 8, baseVal + 8, baseVal, baseVal);            // i-2: 1st black
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 5, baseVal + 5, baseVal - 5, baseVal - 5);    // i-1: 2nd black inside
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 2, baseVal + 2, baseVal - 8, baseVal - 8);    // i:   3rd black inside
        return p;
    }

    /// <summary>CDL3WHITESOLDIERS (bullish, +100 on the 3rd soldier).</summary>
    internal static int FuzzCdl3whitesoldiers(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                              int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 10.2, baseVal - 0.2, baseVal + 10.0);             // 1st white long
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 5.0, baseVal + 15.2, baseVal + 4.8, baseVal + 15.0);       // 2nd white
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 10.0, baseVal + 20.2, baseVal + 9.8, baseVal + 20.0);      // 3rd white
        return p;
    }

    /// <summary>CDL3STARSINSOUTH (bullish, +100 on the 3rd candle).</summary>
    internal static int FuzzCdl3starsinsouth(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                             int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 8.0, baseVal + 8.0, baseVal - 12.0, baseVal);         // 1st black long + long lower shadow
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 6.0, baseVal + 6.0, baseVal - 4.0, baseVal + 2.0);    // 2nd black smaller body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 1.0, baseVal + 1.0, baseVal, baseVal);                // 3rd black tiny marubozu
        return p;
    }

    /// <summary>CDL3LINESTRIKE (three-white branch, +100 on the strike).</summary>
    internal static int FuzzCdl3linestrike(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                           int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 5.0, baseVal - 1.0, baseVal + 4.0);            // i-3: 1st white soldier
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 2.0, baseVal + 7.0, baseVal + 1.0, baseVal + 6.0);      // i-2: 2nd white
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 4.0, baseVal + 9.0, baseVal + 3.0, baseVal + 8.0);      // i-1: 3rd white
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 10.0, baseVal + 11.0, baseVal - 3.0, baseVal - 2.0);    // i:   4th black strike
        return p;
    }

    /// <summary>CDLCONCEALBABYSWALL (bullish, +100 on the 4th candle).</summary>
    internal static int FuzzCdlConcealbabyswall(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                                int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal, baseVal - 3.0, baseVal - 3.0);                     // 1st black marubozu
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 4.0, baseVal - 4.0, baseVal - 7.0, baseVal - 7.0);         // 2nd black marubozu
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 9.0, baseVal - 6.0, baseVal - 12.0, baseVal - 11.0);       // 3rd black gapdown+shadow
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 5.0, baseVal - 4.0, baseVal - 16.0, baseVal - 15.0);       // 4th black engulfs 3rd
        return p;
    }

    /// <summary>CDLMATHOLD (bullish, +100 on the 5th candle).</summary>
    internal static int FuzzCdlMathold(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                       int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 21.0, baseVal - 1.0, baseVal + 20.0);                // c1 white long
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 31.0, baseVal + 32.0, baseVal + 29.0, baseVal + 30.0);        // c2 short black gap up
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 16.0, baseVal + 17.0, baseVal + 14.0, baseVal + 15.0);        // c3 short, in 1st range
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 14.0, baseVal + 15.0, baseVal + 12.0, baseVal + 13.0);        // c4 short, falling
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 35.0, baseVal + 41.0, baseVal + 34.0, baseVal + 40.0);        // c5 white breakout
        return p;
    }

    /// <summary>CDLRISEFALL3METHODS (rising branch, +100 on the 5th candle).</summary>
    internal static int FuzzCdlRisefall3methods(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                                int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 12.5, baseVal - 0.5, baseVal + 12.0);            // 1st long white
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 10.0, baseVal + 10.5, baseVal + 8.5, baseVal + 9.0);      // 2nd small black
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 8.0, baseVal + 8.5, baseVal + 6.5, baseVal + 7.0);        // 3rd small black falling
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 6.0, baseVal + 6.5, baseVal + 4.5, baseVal + 5.0);        // 4th small black falling
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 6.0, baseVal + 20.5, baseVal + 5.5, baseVal + 20.0);      // 5th long white breakout
        return p;
    }

    /// <summary>CDLADVANCEBLOCK (bearish, -100 on the 3rd candle).</summary>
    internal static int FuzzCdlAdvanceblock(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                            int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 6.0, baseVal, baseVal + 6.0);                    // 1st white long body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 5.0, baseVal + 7.0, baseVal + 5.0, baseVal + 7.0);        // 2nd white, shorter
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 7.0, baseVal + 8.0, baseVal + 7.0, baseVal + 8.0);        // 3rd white, shortest
        return p;
    }

    /// <summary>CDLINNECK (bearish, -100 on the 2nd candle). Wider primer
    /// (hr=6).</summary>
    internal static int FuzzCdlInneck(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                      int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 6.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 10.0, baseVal + 12.0, baseVal - 1.0, baseVal);            // 1st long black
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 5.0, baseVal + 1.0, baseVal - 6.0, baseVal + 0.35);       // 2nd white into neck
        return p;
    }

    /// <summary>CDLUNIQUE3RIVER (bullish, +100 on the 3rd candle).</summary>
    internal static int FuzzCdlUnique3river(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                            int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 10.0, baseVal + 11.0, baseVal - 12.0, baseVal - 10.0);    // 1st black long body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 5.0, baseVal + 6.0, baseVal - 15.0, baseVal - 5.0);       // 2nd black harami+low
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 4.0, baseVal - 2.0, baseVal - 5.0, baseVal - 3.0);        // 3rd white short body
        return p;
    }

    /// <summary>CDLKICKING: 2nd pattern bar (index 13) = +100 (bullish).</summary>
    internal static int FuzzCdlKicking(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                       int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 20, baseVal + 20, baseVal, baseVal);                      // idx12: BLACK long marubozu
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 30, baseVal + 50, baseVal + 30, baseVal + 50);            // idx13: WHITE long marubozu, gap up
        return p;
    }

    /// <summary>CDLKICKINGBYLENGTH: 2nd pattern bar (index 13) = +100
    /// (bullish).</summary>
    internal static int FuzzCdlKickingbylength(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                               int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 40, baseVal + 40, baseVal + 20, baseVal + 20);            // bar12: BLACK long marubozu
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 45, baseVal + 70, baseVal + 45, baseVal + 70);            // bar13: WHITE long marubozu, gap up
        return p;
    }

    /// <summary>CDLDARKCLOUDCOVER: 2nd pattern bar (index 13) = -100.</summary>
    internal static int FuzzCdlDarkcloudcover(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                              int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 21, baseVal - 1, baseVal + 20);                  // idx12: white LONG body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 30, baseVal + 31, baseVal + 4, baseVal + 5);              // idx13: black, closes below midpoint
        return p;
    }

    /// <summary>CDLPIERCING: 2nd pattern candle (buffer index 13) fires
    /// +100.</summary>
    internal static int FuzzCdlPiercing(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                        int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 20, baseVal + 21, baseVal - 1, baseVal);                  // 1st: black LONG body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 10, baseVal + 16, baseVal - 11, baseVal + 15);            // 2nd: white LONG body, pierces
        return p;
    }

    /// <summary>CDLTHRUSTING: 2nd pattern bar (index 13) = -100 (always
    /// bearish).</summary>
    internal static int FuzzCdlThrusting(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                         int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 10, baseVal + 11, baseVal - 1, baseVal);                  // 1st: BLACK LONG body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 10, baseVal + 4, baseVal - 12, baseVal + 2.5);            // 2nd: WHITE thrusting
        return p;
    }

    /// <summary>CDLHOMINGPIGEON: 2nd pattern bar (index 13) = +100 (always
    /// bullish).</summary>
    internal static int FuzzCdlHomingpigeon(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                            int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 20, baseVal + 21, baseVal - 1, baseVal);                  // idx12: black LONG body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 7, baseVal + 8, baseVal + 4, baseVal + 5);                // idx13: black SHORT body, engulfed
        return p;
    }

    /// <summary>CDL3INSIDE: 3rd pattern bar (index 14) = -100 (bearish
    /// three-inside-down).</summary>
    internal static int FuzzCdl3inside(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                       int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 12.5, baseVal - 0.5, baseVal + 12);              // 1st: long WHITE body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 6, baseVal + 7.5, baseVal + 5.5, baseVal + 7);            // 2nd: SHORT body, engulfed
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 8, baseVal + 8.5, baseVal - 2.5, baseVal - 2);            // 3rd: BLACK, closes below 1st open
        return p;
    }

    /// <summary>CDLIDENTICAL3CROWS: 3rd pattern bar (index 14) = -100.</summary>
    internal static int FuzzCdlIdentical3crows(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                               int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal, baseVal - 3, baseVal - 3);                        // 1st crow: black, lower shadow 0
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 3, baseVal - 3, baseVal - 6, baseVal - 6);                // 2nd crow: opens at 1st close
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 6, baseVal - 6, baseVal - 9, baseVal - 9);                // 3rd crow: opens at 2nd close
        return p;
    }

    /// <summary>CDLSTALLEDPATTERN: 3rd pattern bar (index 14) = -100.</summary>
    internal static int FuzzCdlStalledpattern(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                              int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 12, baseVal, baseVal + 12);                      // 1st: white LONG body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 8, baseVal + 25, baseVal + 8, baseVal + 25);              // 2nd: white LONG body, no upper shadow
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 25, baseVal + 26, baseVal + 25, baseVal + 26);            // 3rd: small white on 2nd's shoulder
        return p;
    }

    /// <summary>CDLUPSIDEGAP2CROWS: 3rd pattern bar (index 14) = -100.</summary>
    internal static int FuzzCdlUpsidegap2crows(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                               int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 10.5, baseVal - 0.5, baseVal + 10);              // bar12: white LONG body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 16, baseVal + 16.5, baseVal + 14.5, baseVal + 15);        // bar13: black SHORT body, gap up
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 18, baseVal + 18.5, baseVal + 11.5, baseVal + 12);        // bar14: black
        return p;
    }

    /// <summary>CDLBREAKAWAY: 5th pattern bar (index 16) = +100
    /// (1st-candle-black branch).</summary>
    internal static int FuzzCdlBreakaway(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                         int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 20, baseVal + 21, baseVal + 7, baseVal + 8);              // idx12: black LONG body
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 4, baseVal + 5, baseVal - 1, baseVal);                    // idx13: black, body gaps DOWN
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 2, baseVal, baseVal - 5, baseVal - 4);                    // idx14: lower high & low than 2nd
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal - 7, baseVal - 6, baseVal - 11, baseVal - 9);               // idx15: black, lower high & low than 3rd
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 5, baseVal + 7, baseVal + 4, baseVal + 6);                // idx16: white, close in gap
        return p;
    }

    /// <summary>CDLLADDERBOTTOM: 5th pattern bar (index 16) = +100 (always
    /// bullish).</summary>
    internal static int FuzzCdlLadderbottom(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                            int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 20, baseVal + 21, baseVal + 14, baseVal + 15);            // i-4: black, highest open & close
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 15, baseVal + 16, baseVal + 9, baseVal + 10);             // i-3: black, lower
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 10, baseVal + 11, baseVal + 4, baseVal + 5);              // i-2: black, lower
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 5, baseVal + 12, baseVal - 2, baseVal);                   // i-1: black, big upper shadow
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 6, baseVal + 15, baseVal + 6, baseVal + 15);              // i:   white breakout
        return p;
    }

    /// <summary>CDLXSIDEGAP3METHODS: 3rd pattern bar (index 14) = +100
    /// (bullish).</summary>
    internal static int FuzzCdlXsidegap3methods(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                                int p, int n, double baseVal)
    {
        p = FuzzCdlPrimer(o, h, l, c, v, oi, p, n, 6, baseVal, 2.0, 1.0);
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal, baseVal + 6, baseVal - 1, baseVal + 5);                    // 1st: white body [100,105]
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 8, baseVal + 14, baseVal + 7, baseVal + 13);              // 2nd: white body, gap up
        p = FuzzCdlBar(o, h, l, c, v, oi, p, n, baseVal + 10, baseVal + 11, baseVal + 1, baseVal + 2);              // 3rd: black, fills the gap
        return p;
    }

    /// <summary>Lay the deterministic per-family catalog (issue #109); one entry
    /// per otherwise-vacuous pattern.</summary>
    internal static int FuzzCdlCatalog(double[] o, double[] h, double[] l, double[] c, double[] v, double[] oi,
                                       int p, int n)
    {
        p = FuzzCdl2crows(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdl3blackcrows(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdl3whitesoldiers(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdl3starsinsouth(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdl3linestrike(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlConcealbabyswall(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlMathold(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlRisefall3methods(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlAdvanceblock(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlInneck(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlUnique3river(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlKicking(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlKickingbylength(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlDarkcloudcover(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlPiercing(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlThrusting(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlHomingpigeon(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdl3inside(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlIdentical3crows(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlStalledpattern(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlUpsidegap2crows(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlBreakaway(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlLadderbottom(o, h, l, c, v, oi, p, n, 100.0);
        p = FuzzCdlXsidegap3methods(o, h, l, c, v, oi, p, n, 100.0);
        return p;
    }

    /// <summary>Fill the arrays with the FuzzCandle shape: the fixed catalog
    /// prefix, then seed-driven hikkake windows separated by flat filler.</summary>
    internal static void FuzzCandleGen(int seed, int n,
                                       double[] o, double[] h, double[] l, double[] c,
                                       double[] v, double[] oi)
    {
        ulong s;
        unchecked
        {
            s = 0x243F6A8885A308D3UL
                ^ ((ulong)(uint)seed * 0xD1B54A32D192ED03UL);
        }
        int p = FuzzCdlFlat(o, h, l, c, v, oi, 0, n, 6, 100.0);
        p = FuzzCdlCatalog(o, h, l, c, v, oi, p, n); // deterministic per-family windows (#109)
        while (p < n - 16)
        {
            double baseVal = 100.0 + (FuzzSmUnit(ref s) - 0.5) * 40.0;
            double w = 8.0 + FuzzSmUnit(ref s) * 20.0;
            int variant = (int)(FuzzSmUnit(ref s) * 10.0);
            int dir = ((FuzzSmNext(ref s) & 1UL) != 0UL) ? 1 : -1;
            switch (variant)
            {
                case 0: p = FuzzCdlHikkake(o, h, l, c, v, oi, p, n, baseVal, w, dir, 0, 1); break;
                case 1: p = FuzzCdlHikkake(o, h, l, c, v, oi, p, n, baseVal, w, dir, 0, 2); break;
                case 2: p = FuzzCdlHikkake(o, h, l, c, v, oi, p, n, baseVal, w, dir, 1, 0); break;
                case 3: p = FuzzCdlHikkake(o, h, l, c, v, oi, p, n, baseVal, w, dir, 2, 0); break;
                case 4: p = FuzzCdlHikkake(o, h, l, c, v, oi, p, n, baseVal, w, dir, dir > 0 ? 3 : 4, 0); break;
                case 5: p = FuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, baseVal, w, dir, 0, 1); break;
                case 6: p = FuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, baseVal, w, dir, 0, 0); break;
                case 7: p = FuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, baseVal, w, dir, 1, 0); break;
                case 8: p = FuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, baseVal, w, dir, 3, 0); break;
                default: p = FuzzCdlHikkakemod(o, h, l, c, v, oi, p, n, baseVal, w, dir, 4, 1); break;
            }
            p = FuzzCdlFlat(o, h, l, c, v, oi, p, n, 6, baseVal);
        }
        while (p < n) p = FuzzCdlFlat(o, h, l, c, v, oi, p, n, 1, 100.0);
    }

    // ---- FuzzZerosum: bars whose high+low is exactly zero ----

    /// <summary>Fill the arrays with the FuzzZerosum shape: symmetric
    /// high=-low bars (high+low == +0.0 exactly) and all-zero bars interleaved
    /// with ordinary positive bars.</summary>
    internal static void FuzzZerosumGen(int seed, int n,
                                        double[] o, double[] h, double[] l, double[] c,
                                        double[] v, double[] oi)
    {
        ulong s;
        unchecked
        {
            s = 0x243F6A8885A308D3UL
                ^ ((ulong)(uint)seed * 0xD1B54A32D192ED03UL)
                ^ ((ulong)FuzzZerosum << 32);
        }
        int i;
        for (i = 0; i < n; i++)
        {
            double r = FuzzSmUnit(ref s);
            double open, hi, lo, close, a, bse, w, t;
            if (r < 0.34)
            {
                // symmetric zero-sum bar: hi + lo == +0.0 exactly (else branch).
                a = 1.0 + FuzzSmUnit(ref s) * 50.0;
                hi = a; lo = -a; open = 0.0; close = 0.0;
            }
            else if (r < 0.50)
            {
                // all-zero degenerate bar: hi + lo == 0 with hi == lo == 0.
                hi = 0.0; lo = 0.0; open = 0.0; close = 0.0;
            }
            else
            {
                // ordinary positive bar around 100 (hi + lo != 0 -> then branch).
                bse = 90.0 + FuzzSmUnit(ref s) * 20.0;
                w = 0.5 + FuzzSmUnit(ref s) * 5.0;
                close = bse;
                t = FuzzSmUnit(ref s) - 0.5;
                open = bse + t;
                hi = (open > close) ? open : close;
                hi = hi + w;
                lo = (open < close) ? open : close;
                lo = lo - w;
            }
            o[i] = open; h[i] = hi; l[i] = lo; c[i] = close;
            t = FuzzSmUnit(ref s) * 1000.0;
            v[i] = 1000.0 + t;
            t = FuzzSmUnit(ref s) * 100.0;
            oi[i] = 100.0 + t;
        }
    }

    /// <summary>Fill OHLCV+OI arrays (length n) from (shape, seed).
    /// high &gt;= max(o,c), low &lt;= min(o,c). Mul/add is split into separate
    /// statements to match the C, which does that to keep a compiler from
    /// contracting one side of the comparison into an FMA.</summary>
    internal static void FuzzGen(int shape, int seed, int n,
                                 double[] o, double[] h, double[] l, double[] c,
                                 double[] v, double[] oi)
    {
        if (shape == FuzzCandle) { FuzzCandleGen(seed, n, o, h, l, c, v, oi); return; }
        if (shape == FuzzZerosum) { FuzzZerosumGen(seed, n, o, h, l, c, v, oi); return; }
        ulong s;
        unchecked
        {
            s = 0x243F6A8885A308D3UL
                ^ ((ulong)(uint)seed * 0xD1B54A32D192ED03UL)
                ^ ((ulong)(uint)shape << 32);
        }
        double walk = 100.0;
        int i;
        for (i = 0; i < n; i++)
        {
            double close, open, hi, lo, mag, t;

            if (shape == FuzzConstant)
            {
                o[i] = h[i] = l[i] = c[i] = 42.0;
                v[i] = 1000000.0; oi[i] = 10000.0;
                continue;
            }
            if (shape == FuzzTieHeavy)
            {
                close = (double)(3 + (int)(FuzzSmUnit(ref s) * 5.0)); // {3..7}
                o[i] = close;
                c[i] = close;
                h[i] = close + (double)(FuzzSmNext(ref s) & 1UL);
                l[i] = close - (double)(FuzzSmNext(ref s) & 1UL);
                v[i] = (double)(1 + (int)(FuzzSmUnit(ref s) * 4.0)) * 1000.0;
                oi[i] = 1000.0;
                continue;
            }

            switch (shape)
            {
                case FuzzMonoUp:
                    t = (double)i * 0.5; close = 10.0 + t; break;
                case FuzzMonoDown:
                    t = (double)i * 0.25; close = 500.0 - t; break;
                case FuzzExtreme:
                    t = FuzzSmUnit(ref s);
                    close = ((FuzzSmNext(ref s) & 1UL) != 0UL) ? (1.0 + t) * 1.0e9
                                                               : (1.0 + t) * 1.0e-7;
                    break;
                case FuzzWithZeros:
                    {
                        double r = FuzzSmUnit(ref s);
                        if (r < 0.15) close = 0.0;
                        else if (r < 0.30) close = -0.0;
                        else { t = r - 0.5; close = t * 8.0; }
                        break;
                    }
                case FuzzRandwalk:
                default:
                    t = FuzzSmUnit(ref s) - 0.5;
                    t = t * 0.04;
                    walk = walk * (1.0 + t);
                    close = walk;
                    break;
            }

            mag = System.Math.Abs(close) * 0.01 + 0.001;
            t = FuzzSmUnit(ref s) - 0.5;
            t = t * mag;
            open = close + t;
            hi = (open > close) ? open : close;
            t = FuzzSmUnit(ref s) * mag;
            hi = hi + t;
            lo = (open < close) ? open : close;
            t = FuzzSmUnit(ref s) * mag;
            lo = lo - t;

            o[i] = open; h[i] = hi; l[i] = lo; c[i] = close;
            t = FuzzSmUnit(ref s) * 1.0e6;
            v[i] = 1000.0 + t;
            t = FuzzSmUnit(ref s) * 1.0e4;
            oi[i] = 100.0 + t;
        }
    }
}
