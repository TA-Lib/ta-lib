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


/* GENERATED FILE — do not edit. Produced by ta_codegen
 * (generator/src/backends/java_examples.rs) from ta_codegen/input/.
 * MF,CC
 */

package io.github.talib.test;

import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

/**
 * Every function page's Java sample, run on the synthetic series: the
 * call answers a non-empty range whose values are finite.
 */
public class ExamplesTest {

    private static int failures = 0;
    private static int checks = 0;

    private static boolean check(String name, boolean ok) {
        checks++;
        if (!ok) {
            failures++;
            System.out.println("  FAIL: " + name + " example");
        }
        return ok;
    }

    private static boolean allFinite(double[] a, int n) {
        for (int i = 0; i < n; i++) {
            if (!Double.isFinite(a[i])) {
                return false;
            }
        }
        return true;
    }

    private static boolean noNaN(double[] a, int n) {
        for (int i = 0; i < n; i++) {
            if (Double.isNaN(a[i])) {
                return false;
            }
        }
        return true;
    }

    static void ac() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ac(0, high.length - 1, high, low, 5, 34, 5, out);
        if (!check("AC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("AC", allFinite(out, r.count()));
    }

    static void accbands() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] upperBand = new double[252];
        double[] middleBand = new double[252];
        double[] lowerBand = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.accbands(0, high.length - 1, high, low, close, 20,
                                   upperBand, middleBand, lowerBand);
        if (!check("ACCBANDS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last upperBand = " + upperBand[r.count() - 1]);
        check("ACCBANDS", allFinite(upperBand, r.count()));
    }

    static void acos() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -1 + 2 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.acos(0, series.length - 1, series, out);
        if (!check("ACOS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ACOS", noNaN(out, r.count()));
    }

    static void ad() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ad(0, high.length - 1, high, low, close, volume, out);
        if (!check("AD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("AD", allFinite(out, r.count()));
    }

    static void add() {
        double[] series0 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series0[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] series1 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series1[i] = 95 + 10 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.add(0, series0.length - 1, series0, series1, out);
        if (!check("ADD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ADD", allFinite(out, r.count()));
    }

    static void adosc() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.adosc(0, high.length - 1, high, low, close, volume, 3, 10, out);
        if (!check("ADOSC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ADOSC", allFinite(out, r.count()));
    }

    static void adr() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.adr(0, high.length - 1, high, low, 14, out);
        if (!check("ADR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ADR", allFinite(out, r.count()));
    }

    static void adx() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.adx(0, high.length - 1, high, low, close, 14, out);
        if (!check("ADX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ADX", allFinite(out, r.count()));
    }

    static void adxr() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.adxr(0, high.length - 1, high, low, close, 14, out);
        if (!check("ADXR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ADXR", allFinite(out, r.count()));
    }

    static void alligator() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] jaw = new double[252];
        double[] teeth = new double[252];
        double[] lips = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.alligator(0, high.length - 1, high, low, 13, 8, 8, 5, 5, 3, jaw, teeth, lips);
        if (!check("ALLIGATOR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last jaw = " + jaw[r.count() - 1]);
        check("ALLIGATOR", allFinite(jaw, r.count()));
    }

    static void alma() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.alma(0, series.length - 1, series, 9, 6.0, 0.85, out);
        if (!check("ALMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ALMA", allFinite(out, r.count()));
    }

    static void ao() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ao(0, high.length - 1, high, low, 5, 34, out);
        if (!check("AO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("AO", allFinite(out, r.count()));
    }

    static void apo() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.apo(0, series.length - 1, series, 12, 26, MAType.EMA, out);
        if (!check("APO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("APO", allFinite(out, r.count()));
    }

    static void aroon() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] aroonDown = new double[252];
        double[] aroonUp = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.aroon(0, high.length - 1, high, low, 14, aroonDown, aroonUp);
        if (!check("AROON", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last aroonDown = " + aroonDown[r.count() - 1]);
        check("AROON", allFinite(aroonDown, r.count()));
    }

    static void aroonosc() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.aroonosc(0, high.length - 1, high, low, 14, out);
        if (!check("AROONOSC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("AROONOSC", allFinite(out, r.count()));
    }

    static void asi() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.asi(0, open.length - 1, open, high, low, close, 3.0, out);
        if (!check("ASI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ASI", allFinite(out, r.count()));
    }

    static void asin() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -1 + 2 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.asin(0, series.length - 1, series, out);
        if (!check("ASIN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ASIN", noNaN(out, r.count()));
    }

    static void atan() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -5 + 10 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.atan(0, series.length - 1, series, out);
        if (!check("ATAN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ATAN", allFinite(out, r.count()));
    }

    static void atr() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.atr(0, high.length - 1, high, low, close, 14, out);
        if (!check("ATR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ATR", allFinite(out, r.count()));
    }

    static void avgdev() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.avgdev(0, series.length - 1, series, 14, out);
        if (!check("AVGDEV", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("AVGDEV", allFinite(out, r.count()));
    }

    static void avgprice() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.avgprice(0, open.length - 1, open, high, low, close, out);
        if (!check("AVGPRICE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("AVGPRICE", allFinite(out, r.count()));
    }

    static void bbands() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] upperBand = new double[252];
        double[] middleBand = new double[252];
        double[] lowerBand = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.bbands(0, series.length - 1, series, 20, 2.0, 2.0, MAType.SMA,
                                 upperBand, middleBand, lowerBand);
        if (!check("BBANDS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last upperBand = " + upperBand[r.count() - 1]);
        check("BBANDS", allFinite(upperBand, r.count()));
    }

    static void bbw() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.bbw(0, series.length - 1, series, 20, 2.0, 2.0, MAType.SMA, out);
        if (!check("BBW", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("BBW", allFinite(out, r.count()));
    }

    static void beta() {
        double[] series0 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series0[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] series1 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series1[i] = 100 + 10 * Math.sin(0.1 * i + 0.7);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.beta(0, series0.length - 1, series0, series1, 5, out);
        if (!check("BETA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("BETA", allFinite(out, r.count()));
    }

    static void bop() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.bop(0, open.length - 1, open, high, low, close, out);
        if (!check("BOP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("BOP", allFinite(out, r.count()));
    }

    static void cci() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cci(0, high.length - 1, high, low, close, 14, out);
        if (!check("CCI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CCI", allFinite(out, r.count()));
    }

    static void cdl2crows() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdl2crows(0, open.length - 1, open, high, low, close, out);
        if (!check("CDL2CROWS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdl3blackcrows() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdl3blackcrows(0, open.length - 1, open, high, low, close, out);
        if (!check("CDL3BLACKCROWS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdl3inside() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdl3inside(0, open.length - 1, open, high, low, close, out);
        if (!check("CDL3INSIDE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdl3linestrike() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdl3linestrike(0, open.length - 1, open, high, low, close, out);
        if (!check("CDL3LINESTRIKE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdl3outside() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdl3outside(0, open.length - 1, open, high, low, close, out);
        if (!check("CDL3OUTSIDE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdl3starsinsouth() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdl3starsinsouth(0, open.length - 1, open, high, low, close, out);
        if (!check("CDL3STARSINSOUTH", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdl3whitesoldiers() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdl3whitesoldiers(0, open.length - 1, open, high, low, close, out);
        if (!check("CDL3WHITESOLDIERS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlabandonedbaby() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlabandonedbaby(0, open.length - 1, open, high, low, close, 0.3, out);
        if (!check("CDLABANDONEDBABY", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdladvanceblock() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdladvanceblock(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLADVANCEBLOCK", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlbelthold() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlbelthold(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLBELTHOLD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlbreakaway() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlbreakaway(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLBREAKAWAY", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlclosingmarubozu() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlclosingmarubozu(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLCLOSINGMARUBOZU", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlconcealbabyswall() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlconcealbabyswall(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLCONCEALBABYSWALL", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlcounterattack() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlcounterattack(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLCOUNTERATTACK", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdldarkcloudcover() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdldarkcloudcover(0, open.length - 1, open, high, low, close, 0.5, out);
        if (!check("CDLDARKCLOUDCOVER", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdldoji() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdldoji(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLDOJI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdldojistar() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdldojistar(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLDOJISTAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdldragonflydoji() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdldragonflydoji(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLDRAGONFLYDOJI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlengulfing() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlengulfing(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLENGULFING", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdleveningdojistar() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdleveningdojistar(0, open.length - 1, open, high, low, close, 0.3, out);
        if (!check("CDLEVENINGDOJISTAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdleveningstar() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdleveningstar(0, open.length - 1, open, high, low, close, 0.3, out);
        if (!check("CDLEVENINGSTAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlgapsidesidewhite() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlgapsidesidewhite(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLGAPSIDESIDEWHITE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlgravestonedoji() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlgravestonedoji(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLGRAVESTONEDOJI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlhammer() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlhammer(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLHAMMER", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlhangingman() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlhangingman(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLHANGINGMAN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlharami() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlharami(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLHARAMI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlharamicross() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlharamicross(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLHARAMICROSS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlhighwave() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlhighwave(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLHIGHWAVE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlhikkake() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlhikkake(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLHIKKAKE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlhikkakemod() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlhikkakemod(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLHIKKAKEMOD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlhomingpigeon() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlhomingpigeon(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLHOMINGPIGEON", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlidentical3crows() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlidentical3crows(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLIDENTICAL3CROWS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlinneck() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlinneck(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLINNECK", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlinvertedhammer() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlinvertedhammer(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLINVERTEDHAMMER", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlkicking() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlkicking(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLKICKING", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlkickingbylength() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlkickingbylength(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLKICKINGBYLENGTH", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlladderbottom() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlladderbottom(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLLADDERBOTTOM", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdllongleggeddoji() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdllongleggeddoji(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLLONGLEGGEDDOJI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdllongline() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdllongline(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLLONGLINE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlmarubozu() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlmarubozu(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLMARUBOZU", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlmatchinglow() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlmatchinglow(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLMATCHINGLOW", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlmathold() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlmathold(0, open.length - 1, open, high, low, close, 0.5, out);
        if (!check("CDLMATHOLD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlmorningdojistar() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlmorningdojistar(0, open.length - 1, open, high, low, close, 0.3, out);
        if (!check("CDLMORNINGDOJISTAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlmorningstar() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlmorningstar(0, open.length - 1, open, high, low, close, 0.3, out);
        if (!check("CDLMORNINGSTAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlonneck() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlonneck(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLONNECK", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlpiercing() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlpiercing(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLPIERCING", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlrickshawman() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlrickshawman(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLRICKSHAWMAN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlrisefall3methods() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlrisefall3methods(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLRISEFALL3METHODS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlseparatinglines() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlseparatinglines(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLSEPARATINGLINES", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlshootingstar() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlshootingstar(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLSHOOTINGSTAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlshortline() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlshortline(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLSHORTLINE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlspinningtop() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlspinningtop(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLSPINNINGTOP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlstalledpattern() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlstalledpattern(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLSTALLEDPATTERN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlsticksandwich() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlsticksandwich(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLSTICKSANDWICH", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdltakuri() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdltakuri(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLTAKURI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdltasukigap() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdltasukigap(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLTASUKIGAP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlthrusting() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlthrusting(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLTHRUSTING", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdltristar() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdltristar(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLTRISTAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlunique3river() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlunique3river(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLUNIQUE3RIVER", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlupsidegap2crows() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlupsidegap2crows(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLUPSIDEGAP2CROWS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void cdlxsidegap3methods() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cdlxsidegap3methods(0, open.length - 1, open, high, low, close, out);
        if (!check("CDLXSIDEGAP3METHODS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void ce() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] outLong = new double[252];
        double[] outShort = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ce(0, high.length - 1, high, low, close, 22, 3.0, outLong, outShort);
        if (!check("CE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last outLong = " + outLong[r.count() - 1]);
        check("CE", allFinite(outLong, r.count()));
    }

    static void ceil() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -3 + 6 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ceil(0, series.length - 1, series, out);
        if (!check("CEIL", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CEIL", allFinite(out, r.count()));
    }

    static void cg() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cg(0, series.length - 1, series, 10, out);
        if (!check("CG", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CG", allFinite(out, r.count()));
    }

    static void chop() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.chop(0, high.length - 1, high, low, close, 14, out);
        if (!check("CHOP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CHOP", allFinite(out, r.count()));
    }

    static void choptr() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.choptr(0, high.length - 1, high, low, close, 14, out);
        if (!check("CHOPTR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CHOPTR", allFinite(out, r.count()));
    }

    static void cksp() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] highStop = new double[252];
        double[] lowStop = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cksp(0, high.length - 1, high, low, close, 10, 1.0, 9, highStop, lowStop);
        if (!check("CKSP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last highStop = " + highStop[r.count() - 1]);
        check("CKSP", allFinite(highStop, r.count()));
    }

    static void cmf() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cmf(0, high.length - 1, high, low, close, volume, 20, out);
        if (!check("CMF", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CMF", allFinite(out, r.count()));
    }

    static void cmo() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cmo(0, series.length - 1, series, 14, out);
        if (!check("CMO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CMO", allFinite(out, r.count()));
    }

    static void cmou() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cmou(0, series.length - 1, series, 14, out);
        if (!check("CMOU", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CMOU", allFinite(out, r.count()));
    }

    static void coppock() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.coppock(0, series.length - 1, series, 10, 11, 14, out);
        if (!check("COPPOCK", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("COPPOCK", allFinite(out, r.count()));
    }

    static void correl() {
        double[] series0 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series0[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] series1 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series1[i] = 100 + 10 * Math.sin(0.1 * i + 0.7);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.correl(0, series0.length - 1, series0, series1, 30, out);
        if (!check("CORREL", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CORREL", allFinite(out, r.count()));
    }

    static void cos() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -6.3 + 12.6 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cos(0, series.length - 1, series, out);
        if (!check("COS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("COS", allFinite(out, r.count()));
    }

    static void cosh() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -5 + 10 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cosh(0, series.length - 1, series, out);
        if (!check("COSH", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("COSH", noNaN(out, r.count()));
    }

    static void crsi() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.crsi(0, series.length - 1, series, 3, 2, 100, out);
        if (!check("CRSI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CRSI", allFinite(out, r.count()));
    }

    static void cti() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cti(0, series.length - 1, series, 20, out);
        if (!check("CTI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CTI", allFinite(out, r.count()));
    }

    static void cumsum() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cumsum(0, series.length - 1, series, out);
        if (!check("CUMSUM", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CUMSUM", allFinite(out, r.count()));
    }

    static void cvi() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.cvi(0, high.length - 1, high, low, 10, 10, out);
        if (!check("CVI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("CVI", allFinite(out, r.count()));
    }

    static void dema() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.dema(0, series.length - 1, series, 30, out);
        if (!check("DEMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("DEMA", allFinite(out, r.count()));
    }

    static void div() {
        double[] series0 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series0[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] series1 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series1[i] = 95 + 10 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.div(0, series0.length - 1, series0, series1, out);
        if (!check("DIV", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("DIV", noNaN(out, r.count()));
    }

    static void donchian() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] upperBand = new double[252];
        double[] middleBand = new double[252];
        double[] lowerBand = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.donchian(0, high.length - 1, high, low, 20, upperBand, middleBand, lowerBand);
        if (!check("DONCHIAN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last upperBand = " + upperBand[r.count() - 1]);
        check("DONCHIAN", allFinite(upperBand, r.count()));
    }

    static void dosc() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.dosc(0, series.length - 1, series, 14, 5, 3, 9, out);
        if (!check("DOSC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("DOSC", allFinite(out, r.count()));
    }

    static void dpo() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.dpo(0, series.length - 1, series, 20, out);
        if (!check("DPO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("DPO", allFinite(out, r.count()));
    }

    static void dx() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.dx(0, high.length - 1, high, low, close, 14, out);
        if (!check("DX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("DX", allFinite(out, r.count()));
    }

    static void efi() {
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.efi(0, close.length - 1, close, volume, 13, out);
        if (!check("EFI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("EFI", allFinite(out, r.count()));
    }

    static void ema() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ema(0, series.length - 1, series, 30, out);
        if (!check("EMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("EMA", allFinite(out, r.count()));
    }

    static void emv() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.emv(0, high.length - 1, high, low, volume, 14, 10000.0, out);
        if (!check("EMV", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("EMV", allFinite(out, r.count()));
    }

    static void er() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.er(0, series.length - 1, series, 10, out);
        if (!check("ER", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ER", allFinite(out, r.count()));
    }

    static void eri() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] bullPower = new double[252];
        double[] bearPower = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.eri(0, high.length - 1, high, low, close, 13, bullPower, bearPower);
        if (!check("ERI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last bullPower = " + bullPower[r.count() - 1]);
        check("ERI", allFinite(bullPower, r.count()));
    }

    static void exp() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -5 + 10 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.exp(0, series.length - 1, series, out);
        if (!check("EXP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("EXP", noNaN(out, r.count()));
    }

    static void fisher() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] fisher = new double[252];
        double[] trigger = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.fisher(0, high.length - 1, high, low, 10, fisher, trigger);
        if (!check("FISHER", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last fisher = " + fisher[r.count() - 1]);
        check("FISHER", allFinite(fisher, r.count()));
    }

    static void floor() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -3 + 6 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.floor(0, series.length - 1, series, out);
        if (!check("FLOOR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("FLOOR", allFinite(out, r.count()));
    }

    static void fosc() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.fosc(0, series.length - 1, series, 5, out);
        if (!check("FOSC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("FOSC", allFinite(out, r.count()));
    }

    static void fractal() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        int[] swingHigh = new int[252];
        int[] swingLow = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.fractal(0, high.length - 1, high, low, 2, 2, swingHigh, swingLow);
        if (!check("FRACTAL", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last swingHigh = " + swingHigh[r.count() - 1]);
    }

    static void frama() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.frama(0, high.length - 1, high, low, 16, out);
        if (!check("FRAMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("FRAMA", allFinite(out, r.count()));
    }

    static void ha() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] haOpen = new double[252];
        double[] haHigh = new double[252];
        double[] haLow = new double[252];
        double[] haClose = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ha(0, open.length - 1, open, high, low, close, haOpen, haHigh, haLow, haClose);
        if (!check("HA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last haOpen = " + haOpen[r.count() - 1]);
        check("HA", allFinite(haOpen, r.count()));
    }

    static void hma() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.hma(0, series.length - 1, series, 20, out);
        if (!check("HMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("HMA", allFinite(out, r.count()));
    }

    static void htDcperiod() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.htDcperiod(0, series.length - 1, series, out);
        if (!check("HT_DCPERIOD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("HT_DCPERIOD", allFinite(out, r.count()));
    }

    static void htDcphase() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.htDcphase(0, series.length - 1, series, out);
        if (!check("HT_DCPHASE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("HT_DCPHASE", allFinite(out, r.count()));
    }

    static void htPhasor() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] inPhase = new double[252];
        double[] quadrature = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.htPhasor(0, series.length - 1, series, inPhase, quadrature);
        if (!check("HT_PHASOR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last inPhase = " + inPhase[r.count() - 1]);
        check("HT_PHASOR", allFinite(inPhase, r.count()));
    }

    static void htSine() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] sine = new double[252];
        double[] leadSine = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.htSine(0, series.length - 1, series, sine, leadSine);
        if (!check("HT_SINE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last sine = " + sine[r.count() - 1]);
        check("HT_SINE", allFinite(sine, r.count()));
    }

    static void htTrendline() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.htTrendline(0, series.length - 1, series, out);
        if (!check("HT_TRENDLINE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("HT_TRENDLINE", allFinite(out, r.count()));
    }

    static void htTrendmode() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.htTrendmode(0, series.length - 1, series, out);
        if (!check("HT_TRENDMODE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void ibs() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ibs(0, high.length - 1, high, low, close, out);
        if (!check("IBS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("IBS", allFinite(out, r.count()));
    }

    static void ichimoku() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] tenkanSen = new double[252];
        double[] kijunSen = new double[252];
        double[] senkouSpanA = new double[252];
        double[] senkouSpanB = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ichimoku(0, high.length - 1, high, low, 9, 26, 52,
                                   tenkanSen, kijunSen, senkouSpanA, senkouSpanB);
        if (!check("ICHIMOKU", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last tenkanSen = " + tenkanSen[r.count() - 1]);
        check("ICHIMOKU", allFinite(tenkanSen, r.count()));
    }

    static void imi() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.imi(0, open.length - 1, open, close, 14, out);
        if (!check("IMI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("IMI", allFinite(out, r.count()));
    }

    static void kama() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.kama(0, series.length - 1, series, 30, out);
        if (!check("KAMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("KAMA", allFinite(out, r.count()));
    }

    static void kc() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] upperBand = new double[252];
        double[] middleBand = new double[252];
        double[] lowerBand = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.kc(0, high.length - 1, high, low, close, 20, 10, 2.0,
                             upperBand, middleBand, lowerBand);
        if (!check("KC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last upperBand = " + upperBand[r.count() - 1]);
        check("KC", allFinite(upperBand, r.count()));
    }

    static void kdj() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] k = new double[252];
        double[] d = new double[252];
        double[] j = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.kdj(0, high.length - 1, high, low, close, 9, 3, MAType.RMA, 3, MAType.RMA,
                              k, d, j);
        if (!check("KDJ", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last k = " + k[r.count() - 1]);
        check("KDJ", allFinite(k, r.count()));
    }

    static void kst() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] kst = new double[252];
        double[] kstSignal = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.kst(0, series.length - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9,
                              kst, kstSignal);
        if (!check("KST", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last kst = " + kst[r.count() - 1]);
        check("KST", allFinite(kst, r.count()));
    }

    static void kstext() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] kst = new double[252];
        double[] kstSignal = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.kstext(0, series.length - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9, MAType.SMA, MAType.SMA,
                                 kst, kstSignal);
        if (!check("KSTEXT", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last kst = " + kst[r.count() - 1]);
        check("KSTEXT", allFinite(kst, r.count()));
    }

    static void kurtosis() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.kurtosis(0, series.length - 1, series, 30, out);
        if (!check("KURTOSIS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("KURTOSIS", noNaN(out, r.count()));
    }

    static void kvo() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] kvo = new double[252];
        double[] kvoSignal = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.kvo(0, high.length - 1, high, low, close, volume, 34, 55, 13, kvo, kvoSignal);
        if (!check("KVO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last kvo = " + kvo[r.count() - 1]);
        check("KVO", allFinite(kvo, r.count()));
    }

    static void linearreg() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.linearreg(0, series.length - 1, series, 14, out);
        if (!check("LINEARREG", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("LINEARREG", allFinite(out, r.count()));
    }

    static void linearregAngle() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.linearregAngle(0, series.length - 1, series, 14, out);
        if (!check("LINEARREG_ANGLE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("LINEARREG_ANGLE", allFinite(out, r.count()));
    }

    static void linearregIntercept() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.linearregIntercept(0, series.length - 1, series, 14, out);
        if (!check("LINEARREG_INTERCEPT", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("LINEARREG_INTERCEPT", allFinite(out, r.count()));
    }

    static void linearregSlope() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.linearregSlope(0, series.length - 1, series, 14, out);
        if (!check("LINEARREG_SLOPE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("LINEARREG_SLOPE", allFinite(out, r.count()));
    }

    static void ln() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 0.1 + 9.9 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ln(0, series.length - 1, series, out);
        if (!check("LN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("LN", noNaN(out, r.count()));
    }

    static void log10() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 0.1 + 9.9 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.log10(0, series.length - 1, series, out);
        if (!check("LOG10", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("LOG10", noNaN(out, r.count()));
    }

    static void ma() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ma(0, series.length - 1, series, 30, MAType.SMA, out);
        if (!check("MA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MA", allFinite(out, r.count()));
    }

    static void macd() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] macd = new double[252];
        double[] macdSignal = new double[252];
        double[] macdHist = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.macd(0, series.length - 1, series, 12, 26, 9, macd, macdSignal, macdHist);
        if (!check("MACD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last macd = " + macd[r.count() - 1]);
        check("MACD", allFinite(macd, r.count()));
    }

    static void macdext() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] macd = new double[252];
        double[] macdSignal = new double[252];
        double[] macdHist = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.macdext(0, series.length - 1, series, 12, MAType.SMA, 26, MAType.SMA, 9, MAType.SMA,
                                  macd, macdSignal, macdHist);
        if (!check("MACDEXT", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last macd = " + macd[r.count() - 1]);
        check("MACDEXT", allFinite(macd, r.count()));
    }

    static void macdfix() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] macd = new double[252];
        double[] macdSignal = new double[252];
        double[] macdHist = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.macdfix(0, series.length - 1, series, 9, macd, macdSignal, macdHist);
        if (!check("MACDFIX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last macd = " + macd[r.count() - 1]);
        check("MACDFIX", allFinite(macd, r.count()));
    }

    static void macdv() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] macdv = new double[252];
        double[] macdvSignal = new double[252];
        double[] macdvHist = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.macdv(0, high.length - 1, high, low, close, 12, 26, 9,
                                macdv, macdvSignal, macdvHist);
        if (!check("MACDV", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last macdv = " + macdv[r.count() - 1]);
        check("MACDV", allFinite(macdv, r.count()));
    }

    static void mama() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] mama = new double[252];
        double[] fama = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.mama(0, series.length - 1, series, 0.5, 0.05, mama, fama);
        if (!check("MAMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last mama = " + mama[r.count() - 1]);
        check("MAMA", allFinite(mama, r.count()));
    }

    static void marketfi() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.marketfi(0, high.length - 1, high, low, volume, out);
        if (!check("MARKETFI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MARKETFI", allFinite(out, r.count()));
    }

    static void massi() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.massi(0, high.length - 1, high, low, 9, 25, out);
        if (!check("MASSI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MASSI", allFinite(out, r.count()));
    }

    static void mavp() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] periods = new double[252]; // one period per bar
        for (int i = 0; i < 252; i++) periods[i] = 5 + (i % 10);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.mavp(0, series.length - 1, series, periods, 2, 30, MAType.SMA, out);
        if (!check("MAVP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MAVP", allFinite(out, r.count()));
    }

    static void max() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.max(0, series.length - 1, series, 30, out);
        if (!check("MAX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MAX", allFinite(out, r.count()));
    }

    static void maxindex() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.maxindex(0, series.length - 1, series, 30, out);
        if (!check("MAXINDEX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void mcgd() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.mcgd(0, series.length - 1, series, 14, out);
        if (!check("MCGD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MCGD", allFinite(out, r.count()));
    }

    static void median() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.median(0, series.length - 1, series, 30, out);
        if (!check("MEDIAN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MEDIAN", allFinite(out, r.count()));
    }

    static void medprice() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.medprice(0, high.length - 1, high, low, out);
        if (!check("MEDPRICE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MEDPRICE", allFinite(out, r.count()));
    }

    static void mfi() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.mfi(0, high.length - 1, high, low, close, volume, 14, out);
        if (!check("MFI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MFI", allFinite(out, r.count()));
    }

    static void midpoint() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.midpoint(0, series.length - 1, series, 14, out);
        if (!check("MIDPOINT", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MIDPOINT", allFinite(out, r.count()));
    }

    static void midprice() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.midprice(0, high.length - 1, high, low, 14, out);
        if (!check("MIDPRICE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MIDPRICE", allFinite(out, r.count()));
    }

    static void min() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.min(0, series.length - 1, series, 30, out);
        if (!check("MIN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MIN", allFinite(out, r.count()));
    }

    static void minindex() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        int[] out = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.minindex(0, series.length - 1, series, 30, out);
        if (!check("MININDEX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
    }

    static void minmax() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] min = new double[252];
        double[] max = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.minmax(0, series.length - 1, series, 30, min, max);
        if (!check("MINMAX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last min = " + min[r.count() - 1]);
        check("MINMAX", allFinite(min, r.count()));
    }

    static void minmaxindex() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        int[] minIdx = new int[252];
        int[] maxIdx = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.minmaxindex(0, series.length - 1, series, 30, minIdx, maxIdx);
        if (!check("MINMAXINDEX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last minIdx = " + minIdx[r.count() - 1]);
    }

    static void minusDi() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.minusDi(0, high.length - 1, high, low, close, 14, out);
        if (!check("MINUS_DI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MINUS_DI", allFinite(out, r.count()));
    }

    static void minusDm() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.minusDm(0, high.length - 1, high, low, 14, out);
        if (!check("MINUS_DM", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MINUS_DM", allFinite(out, r.count()));
    }

    static void mom() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.mom(0, series.length - 1, series, 10, out);
        if (!check("MOM", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MOM", allFinite(out, r.count()));
    }

    static void mult() {
        double[] series0 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series0[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] series1 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series1[i] = 95 + 10 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.mult(0, series0.length - 1, series0, series1, out);
        if (!check("MULT", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("MULT", allFinite(out, r.count()));
    }

    static void natr() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.natr(0, high.length - 1, high, low, close, 14, out);
        if (!check("NATR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("NATR", allFinite(out, r.count()));
    }

    static void nvi() {
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.nvi(0, close.length - 1, close, volume, out);
        if (!check("NVI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("NVI", allFinite(out, r.count()));
    }

    static void obv() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.obv(0, series.length - 1, series, volume, out);
        if (!check("OBV", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("OBV", allFinite(out, r.count()));
    }

    static void percentb() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.percentb(0, series.length - 1, series, 20, 2.0, 2.0, MAType.SMA, out);
        if (!check("PERCENTB", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PERCENTB", allFinite(out, r.count()));
    }

    static void percentile() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.percentile(0, series.length - 1, series, 100, 50.0, out);
        if (!check("PERCENTILE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PERCENTILE", allFinite(out, r.count()));
    }

    static void percentrank() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.percentrank(0, series.length - 1, series, 100, out);
        if (!check("PERCENTRANK", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PERCENTRANK", allFinite(out, r.count()));
    }

    static void plusDi() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.plusDi(0, high.length - 1, high, low, close, 14, out);
        if (!check("PLUS_DI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PLUS_DI", allFinite(out, r.count()));
    }

    static void plusDm() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.plusDm(0, high.length - 1, high, low, 14, out);
        if (!check("PLUS_DM", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PLUS_DM", allFinite(out, r.count()));
    }

    static void ppo() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ppo(0, series.length - 1, series, 12, 26, MAType.EMA, out);
        if (!check("PPO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PPO", allFinite(out, r.count()));
    }

    static void pso() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.pso(0, high.length - 1, high, low, close, 8, 5, out);
        if (!check("PSO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PSO", allFinite(out, r.count()));
    }

    static void pvi() {
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.pvi(0, close.length - 1, close, volume, out);
        if (!check("PVI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PVI", allFinite(out, r.count()));
    }

    static void pvo() {
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.pvo(0, volume.length - 1, volume, 12, 26, MAType.EMA, out);
        if (!check("PVO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PVO", allFinite(out, r.count()));
    }

    static void pvt() {
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.pvt(0, close.length - 1, close, volume, out);
        if (!check("PVT", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("PVT", allFinite(out, r.count()));
    }

    static void qstick() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.qstick(0, open.length - 1, open, close, 10, out);
        if (!check("QSTICK", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("QSTICK", allFinite(out, r.count()));
    }

    static void rma() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rma(0, series.length - 1, series, 30, out);
        if (!check("RMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("RMA", allFinite(out, r.count()));
    }

    static void roc() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.roc(0, series.length - 1, series, 10, out);
        if (!check("ROC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ROC", allFinite(out, r.count()));
    }

    static void rocp() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rocp(0, series.length - 1, series, 10, out);
        if (!check("ROCP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ROCP", allFinite(out, r.count()));
    }

    static void rocr() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rocr(0, series.length - 1, series, 10, out);
        if (!check("ROCR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ROCR", allFinite(out, r.count()));
    }

    static void rocr100() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rocr100(0, series.length - 1, series, 10, out);
        if (!check("ROCR100", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ROCR100", allFinite(out, r.count()));
    }

    static void rogerssatchell() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rogerssatchell(0, open.length - 1, open, high, low, close, 10, 252.0, out);
        if (!check("ROGERSSATCHELL", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ROGERSSATCHELL", allFinite(out, r.count()));
    }

    static void rsi() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rsi(0, series.length - 1, series, 14, out);
        if (!check("RSI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("RSI", allFinite(out, r.count()));
    }

    static void rvi() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rvi(0, series.length - 1, series, 14, 10, out);
        if (!check("RVI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("RVI", allFinite(out, r.count()));
    }

    static void rvir() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rvir(0, high.length - 1, high, low, 14, 10, out);
        if (!check("RVIR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("RVIR", allFinite(out, r.count()));
    }

    static void rvol() {
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.rvol(0, volume.length - 1, volume, 20, out);
        if (!check("RVOL", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("RVOL", noNaN(out, r.count()));
    }

    static void sar() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sar(0, high.length - 1, high, low, 0.02, 0.2, out);
        if (!check("SAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SAR", allFinite(out, r.count()));
    }

    static void sarext() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sarext(0, high.length - 1, high, low, 0.0, 0.0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2,
                                 out);
        if (!check("SAREXT", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SAREXT", allFinite(out, r.count()));
    }

    static void sharpe() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sharpe(0, series.length - 1, series, 20, 0.0, 252.0, out);
        if (!check("SHARPE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SHARPE", noNaN(out, r.count()));
    }

    static void si() {
        double[] open = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) open[i] = 100 + 10 * Math.sin(0.1 * i - 0.05);
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.si(0, open.length - 1, open, high, low, close, 3.0, out);
        if (!check("SI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SI", allFinite(out, r.count()));
    }

    static void sin() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -6.3 + 12.6 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sin(0, series.length - 1, series, out);
        if (!check("SIN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SIN", allFinite(out, r.count()));
    }

    static void sinh() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -5 + 10 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sinh(0, series.length - 1, series, out);
        if (!check("SINH", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SINH", noNaN(out, r.count()));
    }

    static void skew() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.skew(0, series.length - 1, series, 30, out);
        if (!check("SKEW", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SKEW", allFinite(out, r.count()));
    }

    static void sma() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sma(0, series.length - 1, series, 30, out);
        if (!check("SMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SMA", allFinite(out, r.count()));
    }

    static void smi() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] smi = new double[252];
        double[] smiSignal = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.smi(0, high.length - 1, high, low, close, 13, 2, 25, 9, smi, smiSignal);
        if (!check("SMI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last smi = " + smi[r.count() - 1]);
        check("SMI", allFinite(smi, r.count()));
    }

    static void sortino() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sortino(0, series.length - 1, series, 20, 0.0, 252.0, out);
        if (!check("SORTINO", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SORTINO", noNaN(out, r.count()));
    }

    static void sqrt() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sqrt(0, series.length - 1, series, out);
        if (!check("SQRT", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SQRT", noNaN(out, r.count()));
    }

    static void sqzmom() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] momentum = new double[252];
        int[] squeeze = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sqzmom(0, high.length - 1, high, low, close, 20, 2.0, 20, 2.0, 1.5, 1.0,
                                 momentum, squeeze);
        if (!check("SQZMOM", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last momentum = " + momentum[r.count() - 1]);
        check("SQZMOM", allFinite(momentum, r.count()));
    }

    static void stc() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.stc(0, series.length - 1, series, 23, 50, 10, out);
        if (!check("STC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("STC", allFinite(out, r.count()));
    }

    static void stddev() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.stddev(0, series.length - 1, series, 5, 1.0, out);
        if (!check("STDDEV", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("STDDEV", allFinite(out, r.count()));
    }

    static void stoch() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] slowK = new double[252];
        double[] slowD = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.stoch(0, high.length - 1, high, low, close, 5, 3, MAType.SMA, 3, MAType.SMA,
                                slowK, slowD);
        if (!check("STOCH", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last slowK = " + slowK[r.count() - 1]);
        check("STOCH", allFinite(slowK, r.count()));
    }

    static void stochf() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] fastK = new double[252];
        double[] fastD = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.stochf(0, high.length - 1, high, low, close, 5, 3, MAType.SMA, fastK, fastD);
        if (!check("STOCHF", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last fastK = " + fastK[r.count() - 1]);
        check("STOCHF", allFinite(fastK, r.count()));
    }

    static void stochrsi() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] fastK = new double[252];
        double[] fastD = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.stochrsi(0, series.length - 1, series, 14, 5, 3, MAType.SMA, fastK, fastD);
        if (!check("STOCHRSI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last fastK = " + fastK[r.count() - 1]);
        check("STOCHRSI", allFinite(fastK, r.count()));
    }

    static void sub() {
        double[] series0 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series0[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] series1 = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series1[i] = 95 + 10 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sub(0, series0.length - 1, series0, series1, out);
        if (!check("SUB", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SUB", allFinite(out, r.count()));
    }

    static void sum() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.sum(0, series.length - 1, series, 30, out);
        if (!check("SUM", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SUM", allFinite(out, r.count()));
    }

    static void supertrend() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] supertrend = new double[252];
        int[] trend = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.supertrend(0, high.length - 1, high, low, close, 10, 3.0, supertrend, trend);
        if (!check("SUPERTREND", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last supertrend = " + supertrend[r.count() - 1]);
        check("SUPERTREND", allFinite(supertrend, r.count()));
    }

    static void swak2php() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.swak2php(0, series.length - 1, series, 20, out);
        if (!check("SWAK_2PHP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SWAK_2PHP", allFinite(out, r.count()));
    }

    static void swakBp() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.swakBp(0, series.length - 1, series, 20, 0.1, out);
        if (!check("SWAK_BP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SWAK_BP", allFinite(out, r.count()));
    }

    static void swakButter() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.swakButter(0, series.length - 1, series, 20, out);
        if (!check("SWAK_BUTTER", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SWAK_BUTTER", allFinite(out, r.count()));
    }

    static void swakGauss() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.swakGauss(0, series.length - 1, series, 20, out);
        if (!check("SWAK_GAUSS", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SWAK_GAUSS", allFinite(out, r.count()));
    }

    static void swakHp() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.swakHp(0, series.length - 1, series, 20, out);
        if (!check("SWAK_HP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("SWAK_HP", allFinite(out, r.count()));
    }

    static void t3() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.t3(0, series.length - 1, series, 5, 0.7, out);
        if (!check("T3", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("T3", allFinite(out, r.count()));
    }

    static void tan() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -1.2 + 2.4 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.tan(0, series.length - 1, series, out);
        if (!check("TAN", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TAN", allFinite(out, r.count()));
    }

    static void tanh() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = -3 + 6 * i / 251.0;
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.tanh(0, series.length - 1, series, out);
        if (!check("TANH", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TANH", allFinite(out, r.count()));
    }

    static void tema() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.tema(0, series.length - 1, series, 30, out);
        if (!check("TEMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TEMA", allFinite(out, r.count()));
    }

    static void trange() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.trange(0, high.length - 1, high, low, close, out);
        if (!check("TRANGE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TRANGE", allFinite(out, r.count()));
    }

    static void trima() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.trima(0, series.length - 1, series, 30, out);
        if (!check("TRIMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TRIMA", allFinite(out, r.count()));
    }

    static void trix() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.trix(0, series.length - 1, series, 30, out);
        if (!check("TRIX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TRIX", allFinite(out, r.count()));
    }

    static void tsf() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.tsf(0, series.length - 1, series, 14, out);
        if (!check("TSF", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TSF", allFinite(out, r.count()));
    }

    static void tsi() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.tsi(0, series.length - 1, series, 25, 13, out);
        if (!check("TSI", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TSI", allFinite(out, r.count()));
    }

    static void typprice() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.typprice(0, high.length - 1, high, low, close, out);
        if (!check("TYPPRICE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("TYPPRICE", allFinite(out, r.count()));
    }

    static void ultosc() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.ultosc(0, high.length - 1, high, low, close, 7, 14, 28, out);
        if (!check("ULTOSC", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ULTOSC", allFinite(out, r.count()));
    }

    static void var() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.var(0, series.length - 1, series, 5, 1.0, out);
        if (!check("VAR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("VAR", allFinite(out, r.count()));
    }

    static void vhf() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.vhf(0, series.length - 1, series, 28, out);
        if (!check("VHF", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("VHF", allFinite(out, r.count()));
    }

    static void vidya() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.vidya(0, series.length - 1, series, 12, 9, out);
        if (!check("VIDYA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("VIDYA", allFinite(out, r.count()));
    }

    static void vortex() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] plusVi = new double[252];
        double[] minusVi = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.vortex(0, high.length - 1, high, low, close, 14, plusVi, minusVi);
        if (!check("VORTEX", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last plusVi = " + plusVi[r.count() - 1]);
        check("VORTEX", allFinite(plusVi, r.count()));
    }

    static void vwap() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.vwap(0, high.length - 1, high, low, close, volume, out);
        if (!check("VWAP", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("VWAP", allFinite(out, r.count()));
    }

    static void vwma() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] volume = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) volume[i] = 10000 + 100 * i + 2000 * Math.sin(0.3 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.vwma(0, series.length - 1, series, volume, 30, out);
        if (!check("VWMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("VWMA", noNaN(out, r.count()));
    }

    static void wad() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.wad(0, high.length - 1, high, low, close, out);
        if (!check("WAD", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("WAD", allFinite(out, r.count()));
    }

    static void wavetrend() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] wt1 = new double[252];
        double[] wt2 = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.wavetrend(0, high.length - 1, high, low, close, 10, 21, 4, wt1, wt2);
        if (!check("WAVETREND", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last wt1 = " + wt1[r.count() - 1]);
        check("WAVETREND", allFinite(wt1, r.count()));
    }

    static void wclprice() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.wclprice(0, high.length - 1, high, low, close, out);
        if (!check("WCLPRICE", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("WCLPRICE", allFinite(out, r.count()));
    }

    static void willr() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] close = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) close[i] = 100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.willr(0, high.length - 1, high, low, close, 14, out);
        if (!check("WILLR", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("WILLR", allFinite(out, r.count()));
    }

    static void wma() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.wma(0, series.length - 1, series, 30, out);
        if (!check("WMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("WMA", allFinite(out, r.count()));
    }

    static void zigzag() {
        double[] high = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) high[i] = 101 + 10 * Math.sin(0.1 * i);
        double[] low = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) low[i] = 99 + 10 * Math.sin(0.1 * i);
        double[] zigZag = new double[252];
        int[] trend = new int[252];
        int[] pivotIdx = new int[252];

        Core core = Core.DEFAULT;
        OutRange r = core.zigzag(0, high.length - 1, high, low, 5.0, 1, zigZag, trend, pivotIdx);
        if (!check("ZIGZAG", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last zigZag = " + zigZag[r.count() - 1]);
        check("ZIGZAG", allFinite(zigZag, r.count()));
    }

    static void zlema() {
        double[] series = new double[252]; // your data, oldest first
        for (int i = 0; i < 252; i++) series[i] = 100 + 10 * Math.sin(0.1 * i);
        double[] out = new double[252];

        Core core = Core.DEFAULT;
        OutRange r = core.zlema(0, series.length - 1, series, 30, out);
        if (!check("ZLEMA", r.count() > 0)) return;
        System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
        check("ZLEMA", allFinite(out, r.count()));
    }

    public static void main(String[] args) {
        ac();
        accbands();
        acos();
        ad();
        add();
        adosc();
        adr();
        adx();
        adxr();
        alligator();
        alma();
        ao();
        apo();
        aroon();
        aroonosc();
        asi();
        asin();
        atan();
        atr();
        avgdev();
        avgprice();
        bbands();
        bbw();
        beta();
        bop();
        cci();
        cdl2crows();
        cdl3blackcrows();
        cdl3inside();
        cdl3linestrike();
        cdl3outside();
        cdl3starsinsouth();
        cdl3whitesoldiers();
        cdlabandonedbaby();
        cdladvanceblock();
        cdlbelthold();
        cdlbreakaway();
        cdlclosingmarubozu();
        cdlconcealbabyswall();
        cdlcounterattack();
        cdldarkcloudcover();
        cdldoji();
        cdldojistar();
        cdldragonflydoji();
        cdlengulfing();
        cdleveningdojistar();
        cdleveningstar();
        cdlgapsidesidewhite();
        cdlgravestonedoji();
        cdlhammer();
        cdlhangingman();
        cdlharami();
        cdlharamicross();
        cdlhighwave();
        cdlhikkake();
        cdlhikkakemod();
        cdlhomingpigeon();
        cdlidentical3crows();
        cdlinneck();
        cdlinvertedhammer();
        cdlkicking();
        cdlkickingbylength();
        cdlladderbottom();
        cdllongleggeddoji();
        cdllongline();
        cdlmarubozu();
        cdlmatchinglow();
        cdlmathold();
        cdlmorningdojistar();
        cdlmorningstar();
        cdlonneck();
        cdlpiercing();
        cdlrickshawman();
        cdlrisefall3methods();
        cdlseparatinglines();
        cdlshootingstar();
        cdlshortline();
        cdlspinningtop();
        cdlstalledpattern();
        cdlsticksandwich();
        cdltakuri();
        cdltasukigap();
        cdlthrusting();
        cdltristar();
        cdlunique3river();
        cdlupsidegap2crows();
        cdlxsidegap3methods();
        ce();
        ceil();
        cg();
        chop();
        choptr();
        cksp();
        cmf();
        cmo();
        cmou();
        coppock();
        correl();
        cos();
        cosh();
        crsi();
        cti();
        cumsum();
        cvi();
        dema();
        div();
        donchian();
        dosc();
        dpo();
        dx();
        efi();
        ema();
        emv();
        er();
        eri();
        exp();
        fisher();
        floor();
        fosc();
        fractal();
        frama();
        ha();
        hma();
        htDcperiod();
        htDcphase();
        htPhasor();
        htSine();
        htTrendline();
        htTrendmode();
        ibs();
        ichimoku();
        imi();
        kama();
        kc();
        kdj();
        kst();
        kstext();
        kurtosis();
        kvo();
        linearreg();
        linearregAngle();
        linearregIntercept();
        linearregSlope();
        ln();
        log10();
        ma();
        macd();
        macdext();
        macdfix();
        macdv();
        mama();
        marketfi();
        massi();
        mavp();
        max();
        maxindex();
        mcgd();
        median();
        medprice();
        mfi();
        midpoint();
        midprice();
        min();
        minindex();
        minmax();
        minmaxindex();
        minusDi();
        minusDm();
        mom();
        mult();
        natr();
        nvi();
        obv();
        percentb();
        percentile();
        percentrank();
        plusDi();
        plusDm();
        ppo();
        pso();
        pvi();
        pvo();
        pvt();
        qstick();
        rma();
        roc();
        rocp();
        rocr();
        rocr100();
        rogerssatchell();
        rsi();
        rvi();
        rvir();
        rvol();
        sar();
        sarext();
        sharpe();
        si();
        sin();
        sinh();
        skew();
        sma();
        smi();
        sortino();
        sqrt();
        sqzmom();
        stc();
        stddev();
        stoch();
        stochf();
        stochrsi();
        sub();
        sum();
        supertrend();
        swak2php();
        swakBp();
        swakButter();
        swakGauss();
        swakHp();
        t3();
        tan();
        tanh();
        tema();
        trange();
        trima();
        trix();
        tsf();
        tsi();
        typprice();
        ultosc();
        var();
        vhf();
        vidya();
        vortex();
        vwap();
        vwma();
        wad();
        wavetrend();
        wclprice();
        willr();
        wma();
        zigzag();
        zlema();
        System.out.println("ExamplesTest: " + checks + " checks, " + failures + " failures");
        System.exit(failures == 0 ? 0 : 1);
    }
}
