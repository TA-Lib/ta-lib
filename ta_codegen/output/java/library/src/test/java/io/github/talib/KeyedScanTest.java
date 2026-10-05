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
 *  CC       Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100526 MF,CC  First version. The keyed block scan against the body it
 *                stands in for (issue #415).
 */

package io.github.talib;

import java.util.Random;

/**
 * The keyed twin of a block scan must give the bits of the body it stands in
 * for, and the guard must hand it nothing else.
 *
 * <p>Sits in this package to call {@code <n>KeyedImpl} directly: only that
 * shows a twin fed an unkeyable value answering differently, which is what
 * proves the guard edges below are load-bearing. The reference is always the
 * shipped body with the guard forced shut by a value planted one bar past the
 * compared range, never a second implementation.
 */
public final class KeyedScanTest {
    private static final Core CORE = new Core();
    private static final double NAN_SIGNED = Double.longBitsToDouble(0xfff8000000000001L);
    private static final double[] KEYABLE = { 0.0, Double.MIN_VALUE, 1.0, 3e37, Double.MAX_VALUE,
        Double.POSITIVE_INFINITY };
    private static final double[] UNKEYABLE = { -0.0, -Double.MIN_VALUE, -1.25, Double.NEGATIVE_INFINITY,
        Double.NaN, NAN_SIGNED };

    private static int failures;

    private static void check(boolean ok, String what) {
        if (!ok) {
            failures++;
            if (failures <= 20) {
                System.out.println("  FAIL: " + what);
            }
        }
    }

    private interface Call {
        RetCode run(boolean keyed, int s, int e, Object[] in, int p, MInteger b, MInteger n, double[][] out);
    }

    private static final class Fn {
        final String name;
        final int inputs, scanned, outputs;
        final Call dbl, flt;
        long keyedTrials, plantedTrials, aliasedTrials, values;
        final long[] edgeTrials = new long[2], bites = new long[2];

        Fn(String name, int inputs, int scanned, int outputs, Call dbl, Call flt) {
            this.name = name; this.inputs = inputs; this.scanned = scanned; this.outputs = outputs;
            this.dbl = dbl; this.flt = flt;
        }
    }

    private static final Fn[] FNS = {
        new Fn("MIN", 1, 1, 1,
            (k, s, e, in, p, b, n, o) -> k ? CORE.minKeyedImpl(s, e, (double[]) in[0], p, b, n, o[0])
                                           : CORE.minImpl(s, e, (double[]) in[0], p, b, n, o[0]),
            (k, s, e, in, p, b, n, o) -> k ? CORE.minKeyedImpl(s, e, (float[]) in[0], p, b, n, o[0])
                                           : CORE.minImpl(s, e, (float[]) in[0], p, b, n, o[0])),
        new Fn("MAX", 1, 1, 1,
            (k, s, e, in, p, b, n, o) -> k ? CORE.maxKeyedImpl(s, e, (double[]) in[0], p, b, n, o[0])
                                           : CORE.maxImpl(s, e, (double[]) in[0], p, b, n, o[0]),
            (k, s, e, in, p, b, n, o) -> k ? CORE.maxKeyedImpl(s, e, (float[]) in[0], p, b, n, o[0])
                                           : CORE.maxImpl(s, e, (float[]) in[0], p, b, n, o[0])),
        new Fn("MINMAX", 1, 1, 2,
            (k, s, e, in, p, b, n, o) -> k ? CORE.minmaxKeyedImpl(s, e, (double[]) in[0], p, b, n, o[0], o[1])
                                           : CORE.minmaxImpl(s, e, (double[]) in[0], p, b, n, o[0], o[1]),
            (k, s, e, in, p, b, n, o) -> k ? CORE.minmaxKeyedImpl(s, e, (float[]) in[0], p, b, n, o[0], o[1])
                                           : CORE.minmaxImpl(s, e, (float[]) in[0], p, b, n, o[0], o[1])),
        new Fn("MIDPOINT", 1, 1, 1,
            (k, s, e, in, p, b, n, o) -> k ? CORE.midpointKeyedImpl(s, e, (double[]) in[0], p, b, n, o[0])
                                           : CORE.midpointImpl(s, e, (double[]) in[0], p, b, n, o[0]),
            (k, s, e, in, p, b, n, o) -> k ? CORE.midpointKeyedImpl(s, e, (float[]) in[0], p, b, n, o[0])
                                           : CORE.midpointImpl(s, e, (float[]) in[0], p, b, n, o[0])),
        new Fn("MIDPRICE", 2, 2, 1,
            (k, s, e, in, p, b, n, o) -> k
                ? CORE.midpriceKeyedImpl(s, e, (double[]) in[0], (double[]) in[1], p, b, n, o[0])
                : CORE.midpriceImpl(s, e, (double[]) in[0], (double[]) in[1], p, b, n, o[0]),
            (k, s, e, in, p, b, n, o) -> k
                ? CORE.midpriceKeyedImpl(s, e, (float[]) in[0], (float[]) in[1], p, b, n, o[0])
                : CORE.midpriceImpl(s, e, (float[]) in[0], (float[]) in[1], p, b, n, o[0])),
        new Fn("WILLR", 3, 2, 1,
            (k, s, e, in, p, b, n, o) -> k
                ? CORE.willrKeyedImpl(s, e, (double[]) in[0], (double[]) in[1], (double[]) in[2], p, b, n, o[0])
                : CORE.willrImpl(s, e, (double[]) in[0], (double[]) in[1], (double[]) in[2], p, b, n, o[0]),
            (k, s, e, in, p, b, n, o) -> k
                ? CORE.willrKeyedImpl(s, e, (float[]) in[0], (float[]) in[1], (float[]) in[2], p, b, n, o[0])
                : CORE.willrImpl(s, e, (float[]) in[0], (float[]) in[1], (float[]) in[2], p, b, n, o[0])),
    };

    private static void helpers() {
        long rows = 0;
        for (int len : new int[] { 1, 2, 3, 5, 63, 64, 65, 129, 200 }) {
            for (int at : new int[] { 0, len / 2, len - 1, 62, 63, 64, 127, 128 }) {
                if (at >= len) {
                    continue;
                }
                for (double good : KEYABLE) {
                    double[] d = new double[len + 2];
                    float[] f = new float[len + 2];
                    java.util.Arrays.fill(d, 1.5);
                    java.util.Arrays.fill(f, 1.5f);
                    d[at + 1] = good;
                    f[at + 1] = (float) good;
                    check(Core.keyable(d, 1, len) && Core.keyable(f, 1, len), "keyable accepts " + good);
                    for (double bad : UNKEYABLE) {
                        d[0] = bad; d[len + 1] = bad; f[0] = (float) bad; f[len + 1] = (float) bad;
                        check(Core.keyable(d, 1, len) && Core.keyable(f, 1, len),
                              "keyable reads outside its range (" + bad + ")");
                        double keep = d[at + 1];
                        d[at + 1] = bad; f[at + 1] = (float) bad;
                        check(!Core.keyable(d, 1, len), "keyable(double[]) accepts " + bad + " at " + at + "/" + len);
                        check(!Core.keyable(f, 1, len), "keyable(float[]) accepts " + (float) bad);
                        d[at + 1] = keep; f[at + 1] = (float) keep;
                        d[0] = 1.5; d[len + 1] = 1.5; f[0] = 1.5f; f[len + 1] = 1.5f;
                        rows++;
                    }
                }
            }
        }
        long pairs = 0;
        long[] keys = new long[KEYABLE.length + 3];
        for (int i = 0; i < KEYABLE.length; i++) {
            keys[i] = Double.doubleToRawLongBits(KEYABLE[i]);
        }
        keys[KEYABLE.length] = 1;
        keys[KEYABLE.length + 1] = Double.doubleToRawLongBits(100.25);
        keys[KEYABLE.length + 2] = Double.doubleToRawLongBits(100.26);
        for (long a : keys) {
            for (long b : keys) {
                check(Core.keyMin(a, b) == Math.min(a, b), "keyMin(" + a + ", " + b + ")");
                check(Core.keyMax(a, b) == Math.max(a, b), "keyMax(" + a + ", " + b + ")");
                pairs++;
            }
        }
        check(rows >= 1000 && pairs >= 81, "helper table is vacuous: " + rows + " rows, " + pairs + " pairs");
    }

    private static double[][] data(Random r, Fn fn, int n, int mix) {
        double[][] in = new double[fn.inputs][n];
        double v = 100;
        for (int i = 0; i < n; i++) {
            v = Math.abs(v + Math.rint(r.nextGaussian() * 100) / 100);
            for (int k = 0; k < fn.inputs; k++) {
                double x = mix == 0 ? v + Math.rint(r.nextInt(300)) / 100 : r.nextInt(4) * 0.25;
                if (mix == 2 && r.nextInt(6) == 0) {
                    x = r.nextBoolean() ? 0.0 : Double.POSITIVE_INFINITY;
                }
                in[k][i] = x;
            }
        }
        return in;
    }

    private static Object[] typed(double[][] in, boolean wantFloat) {
        Object[] out = new Object[in.length];
        for (int k = 0; k < in.length; k++) {
            if (wantFloat) {
                float[] f = new float[in[k].length];
                for (int i = 0; i < f.length; i++) {
                    f[i] = (float) in[k][i];
                }
                out[k] = f;
            } else {
                out[k] = in[k].clone();
            }
        }
        return out;
    }

    private static void plant(Object[] in, int k, int at, double value) {
        if (in[k] instanceof float[]) {
            ((float[]) in[k])[at] = (float) value;
        } else {
            ((double[]) in[k])[at] = value;
        }
    }

    private static boolean keyable(Fn fn, Object[] in, int from, int to) {
        for (int k = 0; k < fn.scanned; k++) {
            boolean ok = in[k] instanceof float[] ? Core.keyable((float[]) in[k], from, to)
                                                  : Core.keyable((double[]) in[k], from, to);
            if (!ok) {
                return false;
            }
        }
        return true;
    }

    /** One call; the outputs and a code/begin/count triple, or null when nothing was computed. */
    private static double[][] run(Fn fn, boolean wantFloat, boolean keyed, int s, int e, Object[] in, int p,
                                  int n, int[] meta, int alias, int aliasOut) {
        double[][] out = new double[fn.outputs][n];
        if (alias >= 0) {
            out[aliasOut] = (double[]) in[alias];
        }
        MInteger b = new MInteger(), c = new MInteger();
        RetCode rc = (wantFloat ? fn.flt : fn.dbl).run(keyed, s, e, in, p, b, c, out);
        meta[0] = rc.ordinal(); meta[1] = b.value; meta[2] = c.value;
        return out;
    }

    private static boolean same(Fn fn, double[][] x, int xFrom, double[][] y, int count) {
        for (int k = 0; k < fn.outputs; k++) {
            for (int i = 0; i < count; i++) {
                if (Double.doubleToRawLongBits(x[k][xFrom + i]) != Double.doubleToRawLongBits(y[k][i])) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * The shipped body over {@code [s, e]} with the guard certainly shut: one more bar is
     * requested and an unkeyable value sits on it. A rolling extreme at a bar does not
     * depend on the bars after it. Every array is exactly as long as that call needs,
     * so this is also the only leg here where the transcribed body is held to its extent.
     */
    private static double[][] fallback(Fn fn, boolean wantFloat, int s, int e, double[][] data, int p,
                                       int[] meta) {
        double[][] cut = new double[data.length][];
        for (int k = 0; k < data.length; k++) {
            cut[k] = java.util.Arrays.copyOf(data[k], e + 2);
        }
        Object[] in = typed(cut, wantFloat);
        plant(in, 0, e + 1, -1.25);
        check(!keyable(fn, in, Math.max(s, p - 1) - (p - 1), e + 1), fn.name + ": the plant left the range keyable");
        double[][] out = run(fn, wantFloat, false, s, e + 1, in, p, e + 2 - Math.max(s, p - 1), meta, -1, 0);
        meta[2] -= 1;
        return out;
    }

    private static void sweep(Fn fn, boolean wantFloat, Random r) {
        int[] mk = new int[3], md = new int[3], mf = new int[3];
        for (int trial = 0; trial < 6000; trial++) {
            int n = 3 + r.nextInt(140), p = 2 + r.nextInt(40);
            int e = r.nextInt(n - 1), s = r.nextInt(e + 1);
            if (e < p - 1) {
                continue;
            }
            int from = Math.max(s, p - 1) - (p - 1);
            double[][] data = data(r, fn, n, r.nextInt(3));
            String where = fn.name + (wantFloat ? " float" : " double") + " trial " + trial;
            double[][] f = fallback(fn, wantFloat, s, e, data, p, mf);
            fn.plantedTrials++;

            // Keyed arm, guarded arm, and the body they both stand in for.
            Object[] in = typed(data, wantFloat);
            check(keyable(fn, in, from, e), where + ": keyable data is not");
            double[][] k = run(fn, wantFloat, true, s, e, in, p, n, mk, -1, 0);
            double[][] d = run(fn, wantFloat, false, s, e, in, p, n, md, -1, 0);
            check(java.util.Arrays.equals(mk, mf) && same(fn, k, 0, f, mf[2]), where + ": keyed differs");
            check(java.util.Arrays.equals(md, mf) && same(fn, d, 0, f, mf[2]), where + ": guarded differs");
            fn.keyedTrials++;
            fn.values += (long) mf[2] * fn.outputs;

            // Each output on each input in turn. The float inputs cannot alias a double[] output.
            for (int o = 0; !wantFloat && o < fn.outputs; o++) {
                for (int a = 0; a < fn.inputs; a++) {
                    for (int keyed = 0; keyed < 2; keyed++) {
                        Object[] ai = typed(data, false);
                        double[][] got = run(fn, false, keyed == 1, s, e, ai, p, n, md, a, o);
                        check(java.util.Arrays.equals(md, mf) && same(fn, got, 0, f, mf[2]),
                              where + ": output " + o + " on input " + a + (keyed == 1 ? " keyed" : " guarded")
                              + " differs");
                        fn.aliasedTrials++;
                    }
                }
            }

            // Guard edges: one unkeyable value on the first or the last bar the call reads.
            for (int edge = 0; edge < 2; edge++) {
                double bad = UNKEYABLE[r.nextInt(UNKEYABLE.length)];
                if (bad == 0.0 && trial % 2 == 0) {
                    bad = Double.NaN;
                }
                int scannedInput = r.nextInt(fn.scanned);
                double[][] poisoned = new double[fn.inputs][];
                for (int q = 0; q < fn.inputs; q++) {
                    poisoned[q] = data[q].clone();
                }
                poisoned[scannedInput][edge == 0 ? from : e] = bad;
                double[][] want = fallback(fn, wantFloat, s, e, poisoned, p, mf);
                Object[] pin = typed(poisoned, wantFloat);
                check(!keyable(fn, pin, from, e), where + ": poisoned edge still keyable");
                double[][] got = run(fn, wantFloat, false, s, e, pin, p, n, md, -1, 0);
                check(java.util.Arrays.equals(md, mf) && same(fn, got, 0, want, mf[2]),
                      where + ": the guard let " + bad + " through at the " + (edge == 0 ? "first" : "last") + " bar");
                fn.edgeTrials[edge]++;
                double[][] twin = run(fn, wantFloat, true, s, e, typed(poisoned, wantFloat), p, n, mk, -1, 0);
                if (!same(fn, twin, 0, want, mf[2])) {
                    fn.bites[edge]++;
                }
            }
        }
    }

    /** WILLR does not scan its close: the twin must give the body's bits whatever the close holds. */
    private static void anUnscannedInputMayHoldAnything(boolean wantFloat) {
        Fn willr = FNS[5];
        Random r = new Random(41500);
        int[] mk = new int[3], md = new int[3];
        long differs = 0;
        for (int trial = 0; trial < 300; trial++) {
            int n = 60, p = 2 + r.nextInt(20), e = n - 1, s = 0;
            double[][] data = data(r, willr, n, 0);
            double[][] clean = run(willr, wantFloat, false, s, e, typed(data, wantFloat), p, n, md, -1, 0);
            data[2][p + r.nextInt(n - p)] = Double.NaN;
            data[2][r.nextInt(n)] = -3.5;
            Object[] in = typed(data, wantFloat);
            check(keyable(willr, in, 0, e), "WILLR: the scanned inputs are not keyable");
            double[][] k = run(willr, wantFloat, true, s, e, in, p, n, mk, -1, 0);
            double[][] d = run(willr, wantFloat, false, s, e, in, p, n, md, -1, 0);
            check(java.util.Arrays.equals(mk, md) && same(willr, k, 0, d, md[2]), "WILLR: keyed differs on an unkeyable close");
            if (!same(willr, clean, 0, d, md[2])) {
                differs++;
            }
        }
        check(differs >= 250, "WILLR: the planted close changed nothing (" + differs + ")");
    }

    /** A twin with no row here would ship with its aliasing and its guard edges unproved. */
    private static void everyTwinHasARow() {
        java.util.TreeSet<String> declared = new java.util.TreeSet<>();
        for (java.lang.reflect.Method m : Core.class.getDeclaredMethods()) {
            if (m.getName().endsWith("KeyedImpl")) {
                declared.add(m.getName());
            }
        }
        java.util.TreeSet<String> rows = new java.util.TreeSet<>();
        for (Fn fn : FNS) {
            rows.add(fn.name.toLowerCase(java.util.Locale.ROOT) + "KeyedImpl");
        }
        check(declared.equals(rows), "keyed twins " + declared + " but rows for " + rows);
    }

    public static void main(String[] args) {
        everyTwinHasARow();
        helpers();
        for (boolean wantFloat : new boolean[] { false, true }) {
            Random r = new Random(415);
            for (Fn fn : FNS) {
                fn.keyedTrials = fn.plantedTrials = fn.aliasedTrials = fn.values = 0;
                fn.edgeTrials[0] = fn.edgeTrials[1] = fn.bites[0] = fn.bites[1] = 0;
                sweep(fn, wantFloat, r);
                String tier = fn.name + (wantFloat ? " float[]" : " double[]");
                check(fn.keyedTrials >= 2000 && fn.plantedTrials >= 2000 && fn.values >= 50000,
                      tier + ": the sweep is vacuous (" + fn.keyedTrials + " trials, " + fn.values + " values)");
                check(wantFloat || fn.aliasedTrials >= 4000L * fn.outputs, tier + ": no aliased trials");
                for (int edge = 0; edge < 2; edge++) {
                    check(fn.edgeTrials[edge] >= 2000, tier + ": no edge trials");
                    check(fn.bites[edge] >= 50,
                          tier + ": a twin fed an unkeyable " + (edge == 0 ? "first" : "last")
                          + " bar never differed (" + fn.bites[edge] + "), so the edge is not proved");
                }
                System.out.println("  " + tier + ": " + fn.keyedTrials + " keyed trials, " + fn.values
                    + " values, edge bites " + fn.bites[0] + "/" + fn.bites[1]);
            }
            anUnscannedInputMayHoldAnything(wantFloat);
        }
        if (failures > 0) {
            System.out.println("KeyedScanTest: " + failures + " FAILED");
            System.exit(1);
        }
        System.out.println("KeyedScanTest: OK");
    }
}
