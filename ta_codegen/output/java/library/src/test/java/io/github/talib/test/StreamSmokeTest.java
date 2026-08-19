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

    /** Non-finite counters, incremented AT the assertion rather than derived. */
    private static int nfOpenRejects = 0;
    private static int nfBarRejects = 0;
    private static int nfStateHolds = 0;

    private interface Call { void run(); }

    /**
     * Run {@code r}; true when it threw for the reason under test.
     *
     * <p>The message is checked, not just the type. {@link
     * InsufficientHistoryException} extends {@link IllegalArgumentException}, so
     * catching the base class alone would let "rejected because the history was
     * too short" pass as "rejected the non-finite value" — and every probe here
     * deliberately supplies enough history, so that confusion would go unnoticed
     * the day a lookback grew.
     */
    private static boolean rejects(Call r) {
        try {
            r.run();
            return false;
        } catch (IllegalArgumentException e) {
            return String.valueOf(e.getMessage()).endsWith(": BadParam");
        }
    }

    private static void openMustReject(String what, Call r) {
        check(rejects(r), what + ": open must reject a non-finite parameter");
        nfOpenRejects++;
    }

    private static void barMustReject(String what, Call r) {
        check(rejects(r), what + ": update/peek must reject a non-finite bar");
        nfBarRejects++;
    }

    private static void stateMustHold(String what, double a, double b) {
        check(bitEq(a, b), what + ": a rejected bar must not move the handle");
        nfStateHolds++;
    }

    /**
     * Non-finite rejection is a property of SINGLE VALUES, never of arrays.
     *
     * <p>What is pinned: {@code update}/{@code peek} reject a non-finite bar in
     * any input slot; a real optional parameter that is NaN is rejected; and —
     * the property that makes the rejection useful rather than merely safe — the
     * handle is UNCHANGED by a rejected call, verified against a control stream
     * rather than by inspection.
     *
     * <p>What is deliberately NOT pinned: the warm-up history handed to
     * {@code Open}/{@code OpenAndFill}. It is an input array, and the library
     * does not scan input arrays — see {@code docs/error-handling-spec.md} rule
     * N-5. Passing a non-finite one is undefined behaviour.
     *
     * <p>Coverage is by stream TIER, not by function count: the check is emitted
     * from one place, but into the entry points of five different tiers. SMA is
     * the loop tier, MINUS_DI dual-mode, MA the dispatch tier (including its
     * identity arm, which never reaches a sub-stream at all), MAVP the
     * period-bank tier, and BBANDS/STOCH composed. CDLDOJI adds an integer
     * output over four price inputs.
     */
    private static void nonFiniteInputsAreRejected(
            Core core, double[] open, double[] high, double[] low, double[] close) {
        final double[] bad = { Double.NaN, Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY };
        final int warm = 60;

        for (final double v : bad) {
            /* --- update / peek, and the handle-unchanged property. --------- */
            final double[] cw = java.util.Arrays.copyOf(close, warm);
            final double[] hw = java.util.Arrays.copyOf(high, warm);
            final double[] lw = java.util.Arrays.copyOf(low, warm);
            final double[] ow = java.util.Arrays.copyOf(open, warm);

            final Core.SMA_Stream sa = core.SMA_Open(cw, 14);
            final Core.SMA_Stream sb = core.SMA_Open(cw, 14);
            barMustReject("SMA.update", () -> sa.update(v));
            barMustReject("SMA.peek", () -> sa.peek(v));
            stateMustHold("SMA", sa.update(close[warm]), sb.update(close[warm]));

            final Core.MINUS_DI_Stream da = core.MINUS_DI_Open(hw, lw, cw, 14);
            final Core.MINUS_DI_Stream db = core.MINUS_DI_Open(hw, lw, cw, 14);
            barMustReject("MINUS_DI.update(high)", () -> da.update(v, low[warm], close[warm]));
            barMustReject("MINUS_DI.update(low)", () -> da.update(high[warm], v, close[warm]));
            barMustReject("MINUS_DI.update(close)", () -> da.update(high[warm], low[warm], v));
            barMustReject("MINUS_DI.peek", () -> da.peek(v, low[warm], close[warm]));
            stateMustHold("MINUS_DI",
                da.update(high[warm], low[warm], close[warm]),
                db.update(high[warm], low[warm], close[warm]));

            final Core.MA_Stream ma = core.MA_Open(cw, 14, MAType.EMA);
            final Core.MA_Stream mb = core.MA_Open(cw, 14, MAType.EMA);
            barMustReject("MA.update", () -> ma.update(v));
            barMustReject("MA.peek", () -> ma.peek(v));
            stateMustHold("MA", ma.update(close[warm]), mb.update(close[warm]));

            /* Period 1 is the dispatch identity arm: it copies the bar to the
             * output and never reaches a sub-stream, so a check delegated to the
             * sub would miss it. */
            final Core.MA_Stream mi = core.MA_Open(cw, 1, MAType.SMA);
            barMustReject("MA(identity).update", () -> mi.update(v));
            barMustReject("MA(identity).peek", () -> mi.peek(v));

            final double[] pw = new double[warm];
            for (int i = 0; i < warm; i++) {
                pw[i] = 5.0 + (i % 11);
            }
            final Core.MAVP_Stream va = core.MAVP_Open(cw, pw, 2, 30, MAType.SMA);
            final Core.MAVP_Stream vb = core.MAVP_Open(cw, pw, 2, 30, MAType.SMA);
            barMustReject("MAVP.update(real)", () -> va.update(v, pw[0]));
            barMustReject("MAVP.update(period)", () -> va.update(close[warm], v));
            barMustReject("MAVP.peek(period)", () -> va.peek(close[warm], v));
            stateMustHold("MAVP",
                va.update(close[warm], pw[0]), vb.update(close[warm], pw[0]));

            final Core.BBANDS_Stream ba = core.BBANDS_Open(cw, 20, 2.0, 2.0, MAType.SMA);
            final Core.BBANDS_Stream bb = core.BBANDS_Open(cw, 20, 2.0, 2.0, MAType.SMA);
            barMustReject("BBANDS.update", () -> ba.update(v));
            barMustReject("BBANDS.peek", () -> ba.peek(v));
            Core.BBANDS_Stream.Value bav = ba.update(close[warm]);
            Core.BBANDS_Stream.Value bbv = bb.update(close[warm]);
            stateMustHold("BBANDS.upper", bav.realUpperBand(), bbv.realUpperBand());
            stateMustHold("BBANDS.lower", bav.realLowerBand(), bbv.realLowerBand());

            final Core.STOCH_Stream ka = core.STOCH_Open(hw, lw, cw, 5, 3, MAType.SMA, 3, MAType.SMA);
            final Core.STOCH_Stream kb = core.STOCH_Open(hw, lw, cw, 5, 3, MAType.SMA, 3, MAType.SMA);
            barMustReject("STOCH.update", () -> ka.update(v, low[warm], close[warm]));
            barMustReject("STOCH.peek", () -> ka.peek(high[warm], v, close[warm]));
            Core.STOCH_Stream.Value kav = ka.update(high[warm], low[warm], close[warm]);
            Core.STOCH_Stream.Value kbv = kb.update(high[warm], low[warm], close[warm]);
            stateMustHold("STOCH.slowK", kav.slowK(), kbv.slowK());
            stateMustHold("STOCH.slowD", kav.slowD(), kbv.slowD());

            final Core.CDLDOJI_Stream ja = core.CDLDOJI_Open(ow, hw, lw, cw);
            final Core.CDLDOJI_Stream jb = core.CDLDOJI_Open(ow, hw, lw, cw);
            barMustReject("CDLDOJI.update(open)",
                () -> ja.update(v, high[warm], low[warm], close[warm]));
            barMustReject("CDLDOJI.peek(close)",
                () -> ja.peek(open[warm], high[warm], low[warm], v));
            check(ja.update(open[warm], high[warm], low[warm], close[warm])
                    == jb.update(open[warm], high[warm], low[warm], close[warm]),
                  "CDLDOJI: a rejected bar must not move the handle");
            nfStateHolds++;
        }

        /* A NaN real PARAMETER. Not redundant with the range check: `x < min`
         * and `x > max` are both false for NaN, so a plain range test admits it —
         * which is why the streaming tier spells the same two comparisons
         * inverted. An infinity is already outside every declared bound. */
        openMustReject("BBANDS(nbDevUp=NaN)",
            () -> core.BBANDS_Open(java.util.Arrays.copyOf(close, warm), 20,
                                   Double.NaN, 2.0, MAType.SMA));
        openMustReject("BBANDS(nbDevDn=NaN)",
            () -> core.BBANDS_Open(java.util.Arrays.copyOf(close, warm), 20,
                                   2.0, Double.NaN, MAType.SMA));

        /* Non-vacuity. Literal floors: a count derived from the loop above moves
         * with it and would let the assertions inside be deleted. */
        check(nfOpenRejects >= 2 && nfBarRejects >= 57 && nfStateHolds >= 27,
              "the non-finite gate ran fewer checks than it was written with ("
              + nfOpenRejects + "/" + nfBarRejects + "/" + nfStateHolds + ")");
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
         * name would let the others regress to a hand-rolled class. The count is
         * asserted exactly, so a Value that stopped being generated is a failure
         * rather than a smaller sweep.
         *
         * The expectation is DERIVED FROM THE REGISTRY, not a literal. A literal
         * is a corpus count, and this suite also runs against an input/ that the
         * synth gate has injected fixtures into (scripts/synth_gate.py copies
         * every input_synth/synth<n>/ in before regenerating). The first fixture
         * with more than one real output therefore turns a correct tree red here,
         * with a message about MACD's Value that names nothing to do with the
         * change under test. Deriving it also strengthens the check: the
         * component count is now pinned per function against the registry's
         * output list, where before only MACD's was. */
        java.util.List<String> wrongValue = new java.util.ArrayList<String>();
        int expectedValueTypes = 0;
        for (io.github.talib.metadata.FunctionInfo vf : io.github.talib.metadata.Functions.all()) {
            if (vf.outputs().size() <= 1) {
                continue;
            }
            Class<?> handle = null;
            for (Class<?> nested : Core.class.getDeclaredClasses()) {
                if (nested.getSimpleName().equals(vf.name() + "_Stream")) {
                    handle = nested;
                    break;
                }
            }
            if (handle == null) {
                continue;                       // not stream-capable
            }
            expectedValueTypes++;
            Class<?> value = null;
            for (Class<?> inner : handle.getDeclaredClasses()) {
                if (inner.getSimpleName().equals("Value")) {
                    value = inner;
                    break;
                }
            }
            if (value == null) {
                wrongValue.add(vf.name() + ": no Value");
            } else if (!value.isRecord()) {
                wrongValue.add(vf.name() + ": Value is not a record");
            } else if (value.getRecordComponents().length != vf.outputs().size()) {
                wrongValue.add(vf.name() + ": Value has "
                    + value.getRecordComponents().length + " components, registry declares "
                    + vf.outputs().size());
            }
        }
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
        for (String w : wrongValue) {
            System.out.println("  (wrong Value: " + w + ")");
        }
        check(wrongValue.isEmpty(),
              "every multi-output stream handle carries a Value matching its registry outputs");
        check(expectedValueTypes > 0,
              "the registry named at least one multi-output stream handle (non-vacuity)");
        check(valueTypes == expectedValueTypes,
              expectedValueTypes + " multi-output handles carry a Value (found " + valueTypes + ")");
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

        nonFiniteInputsAreRejected(core, open, high, low, close);


        if (failures == 0) {
            System.out.println("StreamSmokeTest: ALL PASS");
        } else {
            System.out.println("StreamSmokeTest: " + failures + " FAILURES");
            System.exit(1);
        }
    }
}
