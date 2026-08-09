/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * This file is part of the TA-LIB project.
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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF,CC    Mario Fortier, Claude (Anthropic AI)
 */
package io.github.talib.test;

import io.github.talib.CandleSettingType;
import io.github.talib.Core;
import io.github.talib.InsufficientHistoryException;
import io.github.talib.MAType;
import io.github.talib.OutRange;
import io.github.talib.RangeType;

/**
 * Streaming-API smoke test, deliberately junit-free (runnable as a plain
 * {@code main} — the checked-in junit harness is not runnable as shipped).
 * The exhaustive bit-exactness gate is {@code ta_regtest --codegen} driving
 * the JSON-RPC server's {@code stream_verify}; this test covers the
 * API-surface contract a Java USER sees: lifecycle, exceptions, Value
 * semantics, copy independence, and the reflection-layer non-interference
 * pinned during design review.
 */
public class StreamSmokeTest {

    private static int failures = 0;

    private static void check(boolean cond, String what) {
        if (!cond) {
            System.out.println("FAIL: " + what);
            failures++;
        }
    }

    /**
     * Every stream opener, given one bar of history, must report ITSELF using the
     * spelling the metadata registry publishes — {@code "<name> open: ..."}.
     *
     * <p>Swept over every registered function rather than sampled: the three
     * spellings of a function differ (the Java method {@code htTrendline}, the
     * registry name {@code HT_TRENDLINE}, C's {@code TA_HT_TRENDLINE}), so a
     * hardcoded prefix would pin whichever happened to be current, and two
     * hand-picked functions cannot see a composed opener naming its sub-stage.
     * The expectation is read from the registry so the message and the registry
     * move together.
     *
     * <p>Zero-lookback functions (ACOS, AD, OBV, …) succeed on one bar and are
     * counted, not asserted.
     */
    private static void openMessagesNameTheirOwnFunction(Core core) {
        int own = 0, noThrow = 0;
        java.util.List<String> substage = new java.util.ArrayList<String>();
        java.util.List<String> unexpected = new java.util.ArrayList<String>();

        for (io.github.talib.metadata.FunctionInfo f : io.github.talib.metadata.Functions.all()) {
            java.lang.reflect.Method open = null;
            for (java.lang.reflect.Method m : Core.class.getMethods()) {
                if (m.getName().equals(f.name() + "_Open")) {
                    open = m;
                    break;
                }
            }
            if (open == null) {
                unexpected.add(f.name() + ": no " + f.name() + "_Open");
                continue;
            }
            Class<?>[] pt = open.getParameterTypes();
            Object[] args = new Object[pt.length];
            for (int i = 0; i < pt.length; i++) {
                if (pt[i] == double[].class) {
                    args[i] = new double[1];           // one bar: shorter than any lookback
                } else if (pt[i] == int.class) {
                    args[i] = Integer.MIN_VALUE;       // documented default
                } else if (pt[i] == double.class) {
                    args[i] = Core.REAL_DEFAULT;
                } else if (pt[i] == MAType.class) {
                    args[i] = MAType.SMA;
                } else {
                    unexpected.add(f.name() + ": unhandled parameter " + pt[i].getName());
                }
            }
            // Which arm this function belongs in is a fact the registry already
            // knows, so assert it per function instead of pinning the two totals.
            int lookback = f.newCall(core).lookback();
            try {
                open.invoke(core, args);
                if (lookback > 0) {
                    unexpected.add(f.name() + ": accepted one bar despite lookback " + lookback);
                }
                noThrow++;                             // zero-lookback function
            } catch (java.lang.reflect.InvocationTargetException e) {
                Throwable t = e.getCause();
                String msg = String.valueOf(t.getMessage());
                if (!(t instanceof InsufficientHistoryException)) {
                    unexpected.add(f.name() + " -> " + t.getClass().getSimpleName() + ": " + msg);
                } else if (lookback == 0) {
                    unexpected.add(f.name() + ": rejected one bar despite lookback 0");
                } else if (msg.startsWith(f.name() + " open:")) {
                    own++;
                } else {
                    substage.add(f.name() + " -> \"" + msg + "\"");
                }
            } catch (IllegalAccessException e) {
                unexpected.add(f.name() + ": " + e);
            }
        }

        for (String u : unexpected) {
            System.out.println("  (unclassified: " + u + ")");
        }
        for (String s : substage) {
            System.out.println("  (reports a sub-stage's name: " + s + ")");
        }
        check(unexpected.isEmpty(), "every opener is reachable and rejects as documented");
        int registered = io.github.talib.metadata.Functions.all().size();
        check(own + noThrow + substage.size() == registered,
              "the sweep covered every registered function (saw "
              + (own + noThrow + substage.size()) + "/" + registered + ")");
        /* No allowlist. Until the composed shape got its own-lookback precheck,
         * APO/BBANDS/PPO/PVO reported "MA open:" and STDDEV "VAR open:" — the
         * sub-stream they delegate to. Every opener now names itself. */
        check(substage.isEmpty(), "no opener reports a sub-stage's name (saw " + substage.size() + ")");
        /* Non-vacuity: both arms must actually be reached. Which functions land
         * in which is asserted per function in the loop, against the registry. */
        check(noThrow > 0 && own > 0,
              "both arms exercised (" + noThrow + " zero-lookback, " + own + " throwing)");
    }

    private static boolean bitEq(double a, double b) {
        return Double.doubleToRawLongBits(a) == Double.doubleToRawLongBits(b);
    }

    public static void main(String[] args) {
        final int n = 300;
        double[] close = new double[n];
        double[] high = new double[n];
        double[] low = new double[n];
        double[] open = new double[n];
        for (int i = 0; i < n; i++) {
            close[i] = 100.0 + 10.0 * Math.sin(0.1 * i) + 0.013 * i;
            high[i] = close[i] + Math.abs(Math.sin(1.3 * i));
            low[i] = close[i] - Math.abs(Math.sin(1.7 * i));
            open[i] = close[i] - 0.4;   /* a clear (non-doji) real body */
        }
        Core core = new Core();
        OutRange batchRange;

        /* Lifecycle: open == batch at the last bar, update tracks batch. */
        double[] batch = new double[n];
        batchRange = core.SMA(0, n - 1, close, 14, batch);
        check(!batchRange.isEmpty(), "batch SMA produced values");
        int lb = core.SMA_Lookback(14);
        Core.SMA_Stream s = core.SMA_Open(java.util.Arrays.copyOf(close, lb + 1), 14);
        check(bitEq(s.value(), batch[0]), "open value == first batch output");
        for (int t = lb + 1; t < n; t++) {
            double peeked = s.peek(close[t]);
            double updated = s.update(close[t]);
            check(bitEq(peeked, updated), "peek == update @" + t);
            check(bitEq(s.value(), updated), "value() == update @" + t);
            check(bitEq(updated, batch[t - batchRange.begIdx()]), "update == batch @" + t);
        }

        /* peek does not commit; copy() forks independently. */
        Core.SMA_Stream a = core.SMA_Open(java.util.Arrays.copyOf(close, 40), 14);
        double before = a.value();
        a.peek(12345.0);
        check(bitEq(a.value(), before), "peek must not commit");
        Core.SMA_Stream b = a.copy();
        a.update(111.0);
        check(!bitEq(a.value(), b.value()), "copy is independent (diverges)");
        b.update(111.0);
        check(bitEq(a.value(), b.value()), "copy is equivalent (same input, same bits)");

        /* fillRange() is never null: a plain open leaves it empty, and a
         * successful openAndFill always writes at least one value (a history
         * shorter than lookback + 1 throws instead), so isEmpty() separates the
         * two without a null check. */
        check(s.fillRange() != null && s.fillRange().isEmpty(),
              "plain open leaves fillRange empty, never null");
        double[] warm = new double[batchRange.count()];
        Core.SMA_Stream f = core.SMA_OpenAndFill(close, 14, warm);
        check(f.fillRange().equals(batchRange), "openAndFill fillRange == the batch range");
        check(bitEq(warm[batchRange.count() - 1], f.value()),
              "last filled value == the handle's value");
        check(f.copy().fillRange().equals(batchRange), "copy carries the fill range");

        /* Exceptions: typed insufficient history; plain IAE for bad params;
         * aliasing rejection on openAndFill; update/peek never throw. */
        try {
            core.SMA_Open(java.util.Arrays.copyOf(close, lb), 14);
            check(false, "short history must throw");
        } catch (InsufficientHistoryException e) {
            check(e instanceof IllegalArgumentException, "IHE extends IAE");
        }
        openMessagesNameTheirOwnFunction(core);
        try {
            core.SMA_Open(close, -3);
            check(false, "bad param must throw");
        } catch (InsufficientHistoryException e) {
            check(false, "bad param must NOT be typed as insufficient history");
        } catch (IllegalArgumentException e) {
            /* expected */
        }
        try {
            core.SMA_OpenAndFill(close, 14, close);
            check(false, "openAndFill output aliasing input must throw");
        } catch (IllegalArgumentException e) {
            /* expected */
        }

        /* Integer.MIN_VALUE keeps its batch meaning (documented default). */
        check(bitEq(core.SMA_Open(close, Integer.MIN_VALUE).value(),
                    core.SMA_Open(close, 30).value()),
              "MIN_VALUE selects the default");

        /* Multi-output Value: named components, equals/hashCode/toString. */
        Core.MACD_Stream m = core.MACD_Open(close, 12, 26, 9);
        Core.MACD_Stream.Value v1 = m.update(close[n - 1]);
        check(m.value() == v1, "multi-output value() returns the cached instance");
        Core.MACD_Stream.Value v2 = m.peek(close[n - 1] + 1.0);
        check(!v1.equals(v2), "distinct bars produce non-equal Values");
        check(v1.toString().contains("macdSignal="), "Value toString names fields");
        java.util.HashSet<Core.MACD_Stream.Value> set = new java.util.HashSet<Core.MACD_Stream.Value>();
        set.add(v1);
        check(set.contains(m.value()), "Value hashCode/equals contract");

        /* Components are readable by name, in batch output order, and carry the
         * same bits the batch call produces. Nothing else pins the component
         * NAMES from a test: they are otherwise only checked by javac on the
         * generated sub-stream consumers.
         *
         * Compared at the OPEN value, not at v1: opening over the whole series
         * already positions the handle at the last bar, so the update above
         * advances it past the end of what any batch call computes. */
        double[] bM = new double[n], bS = new double[n], bH = new double[n];
        OutRange mr = core.MACD(0, n - 1, close, 12, 26, 9, bM, bS, bH);
        Core.MACD_Stream.Value vOpen = core.MACD_Open(close, 12, 26, 9).value();
        int lastM = mr.count() - 1;
        check(bitEq(vOpen.macd(),       bM[lastM]), "Value.macd() == batch outMACD");
        check(bitEq(vOpen.macdSignal(), bS[lastM]), "Value.macdSignal() == batch outMACDSignal");
        check(bitEq(vOpen.macdHist(),   bH[lastM]), "Value.macdHist() == batch outMACDHist");
        /* Value is a record, so a consumer on JDK 21+ can destructure it in a
         * record pattern. Asserted by reflection rather than by the pattern
         * itself: this suite compiles at --release 17, where the syntax does not
         * exist. */
        check(Core.MACD_Stream.Value.class.isRecord(), "Value is a record");
        check(Core.MACD_Stream.Value.class.getRecordComponents().length == 3,
              "Value has one component per batch output");

        /* ...and EVERY multi-output handle, not just MACD: one class checked by
         * name would let the other thirteen regress to a hand-rolled class. The
         * count is asserted exactly, so a Value that stopped being generated is
         * a failure rather than a smaller sweep. */
        int valueTypes = 0, records = 0;
        for (Class<?> nested : Core.class.getDeclaredClasses()) {
            for (Class<?> inner : nested.getDeclaredClasses()) {
                if (!inner.getSimpleName().equals("Value")) {
                    continue;
                }
                valueTypes++;
                if (inner.isRecord()) {
                    records++;
                } else {
                    System.out.println("  (not a record: " + nested.getSimpleName() + ".Value)");
                }
            }
        }
        check(valueTypes == 14, "14 multi-output handles carry a Value (found " + valueTypes + ")");
        check(records == valueTypes, "every Value is a record");

        /* Dispatch DX: every MAType opens through the same entry point. */
        for (MAType ty : MAType.values()) {
            Core.MA_Stream ma =
                core.MA_Open(close, 14, ty);
            ma.update(close[n - 1]);
        }

        /* Settings are per-instance and frozen into the stream at open: a core
         * with a huge BodyDoji factor calls every candle a doji, the default
         * core calls none of these one. */
        Core tuned = Core.builder()
            .candleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 1.0e9)
            .build();
        Core.CDLDOJI_Stream d1 = core.CDLDOJI_Open(
            java.util.Arrays.copyOf(open, 30), java.util.Arrays.copyOf(high, 30),
            java.util.Arrays.copyOf(low, 30), java.util.Arrays.copyOf(close, 30));
        Core.CDLDOJI_Stream d2 = tuned.CDLDOJI_Open(
            java.util.Arrays.copyOf(open, 30), java.util.Arrays.copyOf(high, 30),
            java.util.Arrays.copyOf(low, 30), java.util.Arrays.copyOf(close, 30));
        check(d1.value() == 0 && d2.value() == 100,
              "candle settings captured per Core instance");


        if (failures == 0) {
            System.out.println("StreamSmokeTest: ALL PASS");
        } else {
            System.out.println("StreamSmokeTest: " + failures + " FAILURES");
            System.exit(1);
        }
    }
}
