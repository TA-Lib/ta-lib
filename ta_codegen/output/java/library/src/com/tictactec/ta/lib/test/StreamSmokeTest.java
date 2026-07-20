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
package com.tictactec.ta.lib.test;

import com.tictactec.ta.lib.CandleSettingType;
import com.tictactec.ta.lib.Core;
import com.tictactec.ta.lib.InsufficientHistoryException;
import com.tictactec.ta.lib.MAType;
import com.tictactec.ta.lib.MInteger;
import com.tictactec.ta.lib.RangeType;
import com.tictactec.ta.lib.RetCode;
import com.tictactec.ta.lib.meta.CoreMetaData;

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
        MInteger beg = new MInteger(), nb = new MInteger();

        /* Lifecycle: open == batch at the last bar, update tracks batch. */
        double[] batch = new double[n];
        check(core.sma(0, n - 1, close, 14, beg, nb, batch) == RetCode.Success, "batch SMA");
        int lb = core.smaLookback(14);
        Core.SmaStream s = core.smaOpen(java.util.Arrays.copyOf(close, lb + 1), 14);
        check(bitEq(s.value(), batch[0]), "open value == first batch output");
        for (int t = lb + 1; t < n; t++) {
            double peeked = s.peek(close[t]);
            double updated = s.update(close[t]);
            check(bitEq(peeked, updated), "peek == update @" + t);
            check(bitEq(s.value(), updated), "value() == update @" + t);
            check(bitEq(updated, batch[t - beg.value]), "update == batch @" + t);
        }

        /* peek does not commit; copy() forks independently. */
        Core.SmaStream a = core.smaOpen(java.util.Arrays.copyOf(close, 40), 14);
        double before = a.value();
        a.peek(12345.0);
        check(bitEq(a.value(), before), "peek must not commit");
        Core.SmaStream b = a.copy();
        a.update(111.0);
        check(!bitEq(a.value(), b.value()), "copy is independent (diverges)");
        b.update(111.0);
        check(bitEq(a.value(), b.value()), "copy is equivalent (same input, same bits)");

        /* Exceptions: typed insufficient history; plain IAE for bad params;
         * aliasing rejection on openAndFill; update/peek never throw. */
        try {
            core.smaOpen(java.util.Arrays.copyOf(close, lb), 14);
            check(false, "short history must throw");
        } catch (InsufficientHistoryException e) {
            check(e instanceof IllegalArgumentException, "IHE extends IAE");
            check(e.getMessage().startsWith("TA_SMA open:"), "stable message prefix");
        }
        try {
            core.smaOpen(close, -3);
            check(false, "bad param must throw");
        } catch (InsufficientHistoryException e) {
            check(false, "bad param must NOT be typed as insufficient history");
        } catch (IllegalArgumentException e) {
            /* expected */
        }
        try {
            core.smaOpenAndFill(close, 14, beg, nb, close);
            check(false, "openAndFill output aliasing input must throw");
        } catch (IllegalArgumentException e) {
            /* expected */
        }

        /* Integer.MIN_VALUE keeps its batch meaning (documented default). */
        check(bitEq(core.smaOpen(close, Integer.MIN_VALUE).value(),
                    core.smaOpen(close, 30).value()),
              "MIN_VALUE selects the default");

        /* Multi-output Value: named final fields, equals/hashCode/toString. */
        Core.MacdStream m = core.macdOpen(close, 12, 26, 9);
        Core.MacdStream.Value v1 = m.update(close[n - 1]);
        check(m.value() == v1, "multi-output value() returns the cached instance");
        Core.MacdStream.Value v2 = m.peek(close[n - 1] + 1.0);
        check(!v1.equals(v2), "distinct bars produce non-equal Values");
        check(v1.toString().contains("macdSignal="), "Value toString names fields");
        java.util.HashSet<Core.MacdStream.Value> set = new java.util.HashSet<Core.MacdStream.Value>();
        set.add(v1);
        check(set.contains(m.value()), "Value hashCode/equals contract");

        /* Dispatch DX: every MAType opens through the same entry point. */
        for (MAType ty : MAType.values()) {
            Core.MovingAverageStream ma =
                core.movingAverageOpen(close, 14, ty);
            ma.update(close[n - 1]);
        }

        /* Settings are per-instance and frozen into the stream at open: a core
         * with a huge BodyDoji factor calls every candle a doji, the default
         * core calls none of these one. */
        Core tuned = new Core();
        tuned.SetCandleSettings(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 1.0e9);
        Core.CdlDojiStream d1 = core.cdlDojiOpen(
            java.util.Arrays.copyOf(open, 30), java.util.Arrays.copyOf(high, 30),
            java.util.Arrays.copyOf(low, 30), java.util.Arrays.copyOf(close, 30));
        Core.CdlDojiStream d2 = tuned.cdlDojiOpen(
            java.util.Arrays.copyOf(open, 30), java.util.Arrays.copyOf(high, 30),
            java.util.Arrays.copyOf(low, 30), java.util.Arrays.copyOf(close, 30));
        check(d1.value() == 0 && d2.value() == 100,
              "candle settings captured per Core instance");

        /* Reflection layer unchanged: the streaming surface must be invisible
         * to CoreMetaData (RetCode-return + Lookback-pair discovery) — pinned
         * at design review; a rename that pairs with an existing Lookback
         * would throw IncompleteAnnotationException here. */
        try {
            final int[] metaFuncs = {0};
            CoreMetaData.forEachFunc(new com.tictactec.ta.lib.meta.TaFuncService() {
                public void execute(CoreMetaData mi) { metaFuncs[0]++; }
            });
            check(metaFuncs[0] >= 165, "CoreMetaData still enumerates the batch API (" + metaFuncs[0] + ")");
            check(CoreMetaData.getInstance("SMA") != null, "CoreMetaData getInstance(SMA)");
        } catch (Exception e) {
            check(false, "CoreMetaData broke: " + e);
        }

        if (failures == 0) {
            System.out.println("StreamSmokeTest: ALL PASS");
        } else {
            System.out.println("StreamSmokeTest: " + failures + " FAILURES");
            System.exit(1);
        }
    }
}
