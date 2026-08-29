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
    private static int checks = 0;

    private static void check(boolean cond, String what) {
        checks++;
        if (!cond) {
            System.out.println("FAIL: " + what);
            failures++;
        }
    }

    /** The registry's SCREAMING_SNAKE name as the streaming tier's method spells it (#278). */
    private static String camelCase(String screaming) {
        StringBuilder sb = new StringBuilder();
        for (String word : screaming.split("_")) {
            if (word.isEmpty()) {
                continue;
            }
            String w = word.toLowerCase(java.util.Locale.ROOT);
            if (sb.length() == 0) {
                sb.append(w);
            } else {
                sb.append(Character.toUpperCase(w.charAt(0))).append(w.substring(1));
            }
        }
        return sb.toString();
    }

    /** The registry's SCREAMING_SNAKE name as the streaming tier's type spells it (#278). */
    private static String pascalCase(String screaming) {
        String c = camelCase(screaming);
        return c.isEmpty() ? c : Character.toUpperCase(c.charAt(0)) + c.substring(1);
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
                if (m.getName().equals(camelCase(f.name()) + "Open")) {
                    open = m;
                    break;
                }
            }
            if (open == null) {
                unexpected.add(f.name() + ": no " + camelCase(f.name()) + "Open");
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
     * does not scan input arrays — see {@code docs/error-handling-spec.md},
     * "Non-finite input". Passing a non-finite one is undefined behaviour.
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

            final Core.SmaStream sa = core.smaOpen(cw, 14);
            final Core.SmaStream sb = core.smaOpen(cw, 14);
            barMustReject("SMA.update", () -> sa.update(v));
            barMustReject("SMA.peek", () -> sa.peek(v));
            stateMustHold("SMA", sa.update(close[warm]), sb.update(close[warm]));

            final Core.MinusDiStream da = core.minusDiOpen(hw, lw, cw, 14);
            final Core.MinusDiStream db = core.minusDiOpen(hw, lw, cw, 14);
            barMustReject("MINUS_DI.update(high)", () -> da.update(v, low[warm], close[warm]));
            barMustReject("MINUS_DI.update(low)", () -> da.update(high[warm], v, close[warm]));
            barMustReject("MINUS_DI.update(close)", () -> da.update(high[warm], low[warm], v));
            barMustReject("MINUS_DI.peek", () -> da.peek(v, low[warm], close[warm]));
            stateMustHold("MINUS_DI",
                da.update(high[warm], low[warm], close[warm]),
                db.update(high[warm], low[warm], close[warm]));

            final Core.MaStream ma = core.maOpen(cw, 14, MAType.EMA);
            final Core.MaStream mb = core.maOpen(cw, 14, MAType.EMA);
            barMustReject("MA.update", () -> ma.update(v));
            barMustReject("MA.peek", () -> ma.peek(v));
            stateMustHold("MA", ma.update(close[warm]), mb.update(close[warm]));

            /* Period 1 is the dispatch identity arm: it copies the bar to the
             * output and never reaches a sub-stream, so a check delegated to the
             * sub would miss it. */
            final Core.MaStream mi = core.maOpen(cw, 1, MAType.SMA);
            barMustReject("MA(identity).update", () -> mi.update(v));
            barMustReject("MA(identity).peek", () -> mi.peek(v));

            final double[] pw = new double[warm];
            for (int i = 0; i < warm; i++) {
                pw[i] = 5.0 + (i % 11);
            }
            final Core.MavpStream va = core.mavpOpen(cw, pw, 2, 30, MAType.SMA);
            final Core.MavpStream vb = core.mavpOpen(cw, pw, 2, 30, MAType.SMA);
            barMustReject("MAVP.update(real)", () -> va.update(v, pw[0]));
            barMustReject("MAVP.update(period)", () -> va.update(close[warm], v));
            barMustReject("MAVP.peek(period)", () -> va.peek(close[warm], v));
            stateMustHold("MAVP",
                va.update(close[warm], pw[0]), vb.update(close[warm], pw[0]));

            final Core.BbandsStream ba = core.bbandsOpen(cw, 20, 2.0, 2.0, MAType.SMA);
            final Core.BbandsStream bb = core.bbandsOpen(cw, 20, 2.0, 2.0, MAType.SMA);
            barMustReject("BBANDS.update", () -> ba.update(v));
            barMustReject("BBANDS.peek", () -> ba.peek(v));
            Core.BbandsStream.Value bav = ba.update(close[warm]);
            Core.BbandsStream.Value bbv = bb.update(close[warm]);
            stateMustHold("BBANDS.upper", bav.realUpperBand(), bbv.realUpperBand());
            stateMustHold("BBANDS.lower", bav.realLowerBand(), bbv.realLowerBand());

            final Core.StochStream ka = core.stochOpen(hw, lw, cw, 5, 3, MAType.SMA, 3, MAType.SMA);
            final Core.StochStream kb = core.stochOpen(hw, lw, cw, 5, 3, MAType.SMA, 3, MAType.SMA);
            barMustReject("STOCH.update", () -> ka.update(v, low[warm], close[warm]));
            barMustReject("STOCH.peek", () -> ka.peek(high[warm], v, close[warm]));
            Core.StochStream.Value kav = ka.update(high[warm], low[warm], close[warm]);
            Core.StochStream.Value kbv = kb.update(high[warm], low[warm], close[warm]);
            stateMustHold("STOCH.slowK", kav.slowK(), kbv.slowK());
            stateMustHold("STOCH.slowD", kav.slowD(), kbv.slowD());

            final Core.CdldojiStream ja = core.cdldojiOpen(ow, hw, lw, cw);
            final Core.CdldojiStream jb = core.cdldojiOpen(ow, hw, lw, cw);
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
            () -> core.bbandsOpen(java.util.Arrays.copyOf(close, warm), 20,
                                   Double.NaN, 2.0, MAType.SMA));
        openMustReject("BBANDS(nbDevDn=NaN)",
            () -> core.bbandsOpen(java.util.Arrays.copyOf(close, warm), 20,
                                   2.0, Double.NaN, MAType.SMA));

        /* Non-vacuity. Literal floors: a count derived from the loop above moves
         * with it and would let the assertions inside be deleted. */
        check(nfOpenRejects >= 2 && nfBarRejects >= 57 && nfStateHolds >= 27,
              "the non-finite gate ran fewer checks than it was written with ("
              + nfOpenRejects + "/" + nfBarRejects + "/" + nfStateHolds + ")");
    }

    /** UpdateAndFill counters, incremented AT the assertion rather than derived. */
    private static int ufCommits = 0;
    private static int ufValues = 0;
    private static int ufSlots = 0;

    private static final int UF_N = 6;
    private static final int UF_BAD = 3;
    private static final double UF_CANARY = -1.2345678901234e300;
    private static final int UF_CANARY_I = -987654321;

    /**
     * {@code updateAndFill} is n back-to-back {@code update}s and nothing else,
     * so a non-finite bar {@code k} throws exactly as {@code update} would — and
     * the bars before it stay committed with their values written.
     *
     * <p>That is the one place in the API where a call fails AND leaves output
     * behind, so what it leaves is pinned against a CONTROL handle driven over
     * the same first {@code k} bars one at a time: same {@code outRange()}, same
     * values, same answer on the next good bar, and nothing written at or above
     * {@code k}. A whole-array pre-scan would satisfy "it throws" and fail every
     * one of those.
     *
     * <p>Coverage is by the emitters {@code updateAndFill} is generated from,
     * which is one for every tier — so SMA stands for the whole step-loop family,
     * BBANDS adds three outputs, MA both dispatch arms, MAVP the period bank and
     * CDLDOJI an integer output over four inputs.
     */
    private static void updateAndFillCommitsThePrefix(
            Core core, double[] open, double[] high, double[] low, double[] close) {
        final double[] bad = { Double.NaN, Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY };
        final int warm = 60;
        final double[] cw = java.util.Arrays.copyOf(close, warm);
        final double[] hw = java.util.Arrays.copyOf(high, warm);
        final double[] lw = java.util.Arrays.copyOf(low, warm);
        final double[] ow = java.util.Arrays.copyOf(open, warm);
        final double[] pw = new double[warm];
        for (int i = 0; i < warm; i++) {
            pw[i] = 5.0 + (i % 11);
        }

        for (final double v : bad) {
            final double[] bars = new double[UF_N];
            final double[] goodBars = new double[UF_N];
            for (int i = 0; i < UF_N; i++) {
                bars[i] = close[warm + i];
                goodBars[i] = close[warm + i];
            }
            bars[UF_BAD] = v;

            /* --- the shared step loop --------------------------------------- */
            final Core.SmaStream sa = core.smaOpen(cw, 14);
            final Core.SmaStream sb = core.smaOpen(cw, 14);
            final double[] want = new double[UF_BAD];
            for (int i = 0; i < UF_BAD; i++) {
                want[i] = sb.update(bars[i]);
            }
            final double[] out = new double[UF_N];
            java.util.Arrays.fill(out, UF_CANARY);
            barMustReject("SMA.updateAndFill", () -> sa.updateAndFill(bars, out));
            ufRangeEq("SMA", sa.outRange(), sb.outRange());
            for (int i = 0; i < UF_BAD; i++) {
                ufValueEq("SMA", out[i], want[i]);
            }
            for (int i = UF_BAD; i < UF_N; i++) {
                ufUntouched("SMA", out[i], UF_CANARY);
            }
            stateMustHold("SMA(updateAndFill)",
                sa.update(close[warm + UF_N]), sb.update(close[warm + UF_N]));

            /* --- composed, three outputs ------------------------------------ */
            final Core.BbandsStream ba = core.bbandsOpen(cw, 20, 2.0, 2.0, MAType.SMA);
            final Core.BbandsStream bb = core.bbandsOpen(cw, 20, 2.0, 2.0, MAType.SMA);
            final Core.BbandsStream.Value[] wantB = new Core.BbandsStream.Value[UF_BAD];
            for (int i = 0; i < UF_BAD; i++) {
                wantB[i] = bb.update(bars[i]);
            }
            final double[] bu = new double[UF_N];
            final double[] bm = new double[UF_N];
            final double[] bl = new double[UF_N];
            java.util.Arrays.fill(bu, UF_CANARY);
            java.util.Arrays.fill(bm, UF_CANARY);
            java.util.Arrays.fill(bl, UF_CANARY);
            barMustReject("BBANDS.updateAndFill", () -> ba.updateAndFill(bars, bu, bm, bl));
            ufRangeEq("BBANDS", ba.outRange(), bb.outRange());
            for (int i = 0; i < UF_BAD; i++) {
                ufValueEq("BBANDS.upper", bu[i], wantB[i].realUpperBand());
                ufValueEq("BBANDS.middle", bm[i], wantB[i].realMiddleBand());
                ufValueEq("BBANDS.lower", bl[i], wantB[i].realLowerBand());
            }
            for (int i = UF_BAD; i < UF_N; i++) {
                ufUntouched("BBANDS.upper", bu[i], UF_CANARY);
                ufUntouched("BBANDS.middle", bm[i], UF_CANARY);
                ufUntouched("BBANDS.lower", bl[i], UF_CANARY);
            }
            /* value() must name the last COMMITTED bar, not the one before the
             * call: the multi-output cache is refreshed on the throwing path
             * too. */
            check(bitEq(ba.value().realUpperBand(), wantB[UF_BAD - 1].realUpperBand()),
                  "BBANDS: value() must name the last committed bar after a partial fill");
            ufValues++;

            /* --- dispatch, both arms (period 1 is the identity loop) --------- */
            for (final int period : new int[] { 1, 14 }) {
                final Core.MaStream ma = core.maOpen(cw, period, MAType.SMA);
                final Core.MaStream mb = core.maOpen(cw, period, MAType.SMA);
                final double[] wantM = new double[UF_BAD];
                for (int i = 0; i < UF_BAD; i++) {
                    wantM[i] = mb.update(bars[i]);
                }
                final double[] mo = new double[UF_N];
                java.util.Arrays.fill(mo, UF_CANARY);
                barMustReject("MA(" + period + ").updateAndFill", () -> ma.updateAndFill(bars, mo));
                ufRangeEq("MA(" + period + ")", ma.outRange(), mb.outRange());
                for (int i = 0; i < UF_BAD; i++) {
                    ufValueEq("MA", mo[i], wantM[i]);
                }
                for (int i = UF_BAD; i < UF_N; i++) {
                    ufUntouched("MA", mo[i], UF_CANARY);
                }
            }

            /* --- period bank: poison the PERIOD series, the input that reaches
             * an (int) cast ---------------------------------------------------- */
            final double[] pers = new double[UF_N];
            for (int i = 0; i < UF_N; i++) {
                pers[i] = 2.0 + (i % 8);
            }
            pers[UF_BAD] = v;
            final Core.MavpStream va = core.mavpOpen(cw, pw, 2, 30, MAType.SMA);
            final Core.MavpStream vb = core.mavpOpen(cw, pw, 2, 30, MAType.SMA);
            final double[] wantV = new double[UF_BAD];
            for (int i = 0; i < UF_BAD; i++) {
                wantV[i] = vb.update(goodBars[i], pers[i]);
            }
            final double[] vo = new double[UF_N];
            java.util.Arrays.fill(vo, UF_CANARY);
            barMustReject("MAVP.updateAndFill", () -> va.updateAndFill(goodBars, pers, vo));
            ufRangeEq("MAVP", va.outRange(), vb.outRange());
            for (int i = 0; i < UF_BAD; i++) {
                ufValueEq("MAVP", vo[i], wantV[i]);
            }
            for (int i = UF_BAD; i < UF_N; i++) {
                ufUntouched("MAVP", vo[i], UF_CANARY);
            }

            /* --- integer output, four inputs; poison the LOW ----------------- */
            final double[] opens = new double[UF_N];
            final double[] highs = new double[UF_N];
            final double[] lows = new double[UF_N];
            for (int i = 0; i < UF_N; i++) {
                opens[i] = open[warm + i];
                highs[i] = high[warm + i];
                lows[i] = low[warm + i];
            }
            lows[UF_BAD] = v;
            final Core.CdldojiStream ja = core.cdldojiOpen(ow, hw, lw, cw);
            final Core.CdldojiStream jb = core.cdldojiOpen(ow, hw, lw, cw);
            final int[] wantJ = new int[UF_BAD];
            for (int i = 0; i < UF_BAD; i++) {
                wantJ[i] = jb.update(opens[i], highs[i], lows[i], goodBars[i]);
            }
            final int[] jo = new int[UF_N];
            java.util.Arrays.fill(jo, UF_CANARY_I);
            barMustReject("CDLDOJI.updateAndFill",
                () -> ja.updateAndFill(opens, highs, lows, goodBars, jo));
            ufRangeEq("CDLDOJI", ja.outRange(), jb.outRange());
            for (int i = 0; i < UF_BAD; i++) {
                check(jo[i] == wantJ[i], "CDLDOJI: updateAndFill wrote " + jo[i]
                      + " where update returned " + wantJ[i]);
                ufValues++;
            }
            for (int i = UF_BAD; i < UF_N; i++) {
                check(jo[i] == UF_CANARY_I, "CDLDOJI: updateAndFill wrote past the rejected bar");
                ufSlots++;
            }
        }

        /* The rejections Java can make that C cannot: array lengths. Plus the
         * two the language does allow it to see — an output that IS an input,
         * and a zero-bar call, which is a success that changes nothing. */
        final Core.SmaStream s = core.smaOpen(cw, 14);
        final OutRange before = s.outRange();
        final double[] out = new double[UF_N];
        java.util.Arrays.fill(out, UF_CANARY);
        final double[] bars = new double[UF_N];
        for (int i = 0; i < UF_N; i++) {
            bars[i] = close[warm + i];
        }
        s.updateAndFill(new double[0], out);
        check(before.begIdx() == s.outRange().begIdx() && before.count() == s.outRange().count(),
              "a zero-bar updateAndFill must not move the handle");
        ufCommits++;
        check(bitEq(out[0], UF_CANARY), "a zero-bar updateAndFill must write nothing");
        ufSlots++;
        barMustReject("SMA.updateAndFill(short output)",
            () -> s.updateAndFill(bars, new double[UF_N - 1]));
        barMustReject("SMA.updateAndFill(output aliases input)",
            () -> s.updateAndFill(bars, bars));
        check(before.begIdx() == s.outRange().begIdx() && before.count() == s.outRange().count(),
              "a rejected updateAndFill must not move the handle");
        ufCommits++;
        /* Control: the same call, correctly sized, succeeds and advances by
         * exactly the bars it was handed — so the rejections above cannot be
         * passing because updateAndFill rejects everything. */
        s.updateAndFill(bars, out);
        check(s.outRange().count() == before.count() + UF_N,
              "updateAndFill must advance by every bar it commits");
        ufCommits++;

        /* Non-vacuity. Literal floors, every counter incremented at its
         * assertion. */
        check(ufCommits >= 21 && ufValues >= 75 && ufSlots >= 73,
              "the updateAndFill gate ran fewer checks than it was written with ("
              + ufCommits + "/" + ufValues + "/" + ufSlots + ")");
    }

    /** Handles have no common supertype — {@code outRange()} is declared on each
     *  generated class — so the caller passes the two ranges, not the handles. */
    private static void ufRangeEq(String what, OutRange ra, OutRange rb) {
        check(ra.begIdx() == rb.begIdx() && ra.count() == rb.count(),
              what + ": updateAndFill committed (" + ra.begIdx() + "," + ra.count()
              + "), " + UF_BAD + " updates committed (" + rb.begIdx() + "," + rb.count() + ")");
        ufCommits++;
    }

    private static void ufValueEq(String what, double a, double b) {
        check(bitEq(a, b), what + ": updateAndFill wrote " + a + " where update returned " + b);
        ufValues++;
    }

    private static void ufUntouched(String what, double x, double canary) {
        check(bitEq(x, canary), what + ": updateAndFill wrote past the bar it rejected");
        ufSlots++;
    }

    /* ---- the registry-wide peek/copy sweep (#172 C4) --------------------- */

    /** The order the emitter expands a PRICE bundle into {@code double[]} params. */
    private static final int[] PRICE_BITS = {
        io.github.talib.metadata.InputFlags.PRICE_OPEN,
        io.github.talib.metadata.InputFlags.PRICE_HIGH,
        io.github.talib.metadata.InputFlags.PRICE_LOW,
        io.github.talib.metadata.InputFlags.PRICE_CLOSE,
        io.github.talib.metadata.InputFlags.PRICE_VOLUME,
        io.github.talib.metadata.InputFlags.PRICE_OPENINTEREST,
    };

    private static final String[] PRICE_SLOTS = {
        "open", "high", "low", "close", "volume", "openinterest",
    };

    /** Counters for the sweep's non-vacuity floors, incremented at the assertion. */
    private static int swNonCommit = 0;
    private static int swCopy = 0;
    private static int swMoved = 0;

    /**
     * Bar {@code i} of the series feeding one input slot.
     *
     * <p>Shaped so the OHLC relation holds bar by bar — {@code high} above both
     * ends of the body, {@code low} below both — because a handle fed
     * high &lt; low is not exercising the algorithm, it is exercising whatever
     * the algorithm does with nonsense. Volume is positive for the same reason
     * (MARKETFI, and every other divisor of volume). The two unnamed real
     * series are given different phases so a two-series function (BETA, CORREL)
     * is not silently correlating a series with itself.
     */
    private static double slotBar(String slot, int i, boolean unitDomain) {
        if (unitDomain) {
            /* ACOS/ASIN are defined on [-1, 1] and return NaN outside it, so a
             * price series makes every assertion below compare NaN to NaN —
             * true, and vacuous. Measured: with a price series ACOS, ASIN and
             * TANH are the three handles the peek control below cannot catch. */
            return slot.equals("inReal1") ? 0.5 * Math.cos(0.07 * i)
                                          : 0.6 * Math.sin(0.1 * i);
        }
        double close = 100.0 + 10.0 * Math.sin(0.1 * i) + 0.013 * i;
        if (slot.equals("open")) {
            /* The real body VARIES bar to bar. A fixed offset makes close-open
             * a constant, which leaves QSTICK — an average of exactly that —
             * flat over the whole corpus, and a flat series proves nothing. */
            return close - 0.4 - 0.3 * Math.sin(0.23 * i);
        } else if (slot.equals("high")) {
            return close + Math.abs(Math.sin(1.3 * i)) + 0.9;
        } else if (slot.equals("low")) {
            return close - Math.abs(Math.sin(1.7 * i)) - 0.9;
        } else if (slot.equals("volume")) {
            return 1000.0 + 10.0 * i;
        } else if (slot.equals("openinterest")) {
            return 500.0 + i;
        } else if (slot.equals("inPeriods")) {
            return 5.0 + (i % 20);          // MAVP: a period, not a price
        } else if (slot.equals("inReal1")) {
            return 95.0 + 8.0 * Math.cos(0.07 * i) + 0.01 * i;
        }
        return close;                        // close, inReal, inReal0
    }

    /**
     * A bar far off the series, used only to probe that {@code peek} commits
     * nothing.
     *
     * <p>An in-series bar is a weak probe for that: it leaves a rolling
     * extremum's window where it was, so a {@code peek} that committed would be
     * invisible there. A gap moves it — in ONE direction only, which is why the
     * sweep probes with {@code up} both ways: a gap up never moves MIN, a gap
     * down never moves MAX. Still a legal bar — finite, {@code high} above the
     * body, {@code low} below, volume positive — because the point is to move
     * the state, not to test rejection.
     *
     * <p>The other direction the gap has to flip is the BODY: a fixed
     * {@code close - open} offset leaves a bullish bar bullish however far it
     * gaps, and a candlestick pattern reads the direction, not the level.
     */
    private static double outlierBar(String slot, int i, boolean up, boolean unitDomain) {
        if (unitDomain) {
            return up ? 0.98 : -0.98;
        }
        double base = (up ? 1.4 : 0.6) * (100.0 + 10.0 * Math.sin(0.1 * i) + 0.013 * i);
        if (slot.equals("open")) {
            return up ? base - 2.0 : base + 2.0;   // a body, and it flips direction
        } else if (slot.equals("high")) {
            return base + 3.0;
        } else if (slot.equals("low")) {
            return base - 3.0;
        } else if (slot.equals("volume")) {
            return up ? 9000.0 : 40.0;
        } else if (slot.equals("openinterest")) {
            return up ? 900.0 : 40.0;
        } else if (slot.equals("inPeriods")) {
            return up ? 27.0 : 3.0;
        } else if (slot.equals("inReal1")) {
            return 0.9 * base;
        }
        return base;
    }

    /**
     * One slot label per {@code double[]} parameter the opener takes, derived
     * from the registry rather than from the method signature — so that the
     * count the registry declares and the count the emitter emitted are two
     * independent facts this sweep can compare.
     */
    private static java.util.List<String> declaredSlots(
            io.github.talib.metadata.FunctionInfo f, java.util.List<String> unhandled) {
        java.util.List<String> slots = new java.util.ArrayList<String>();
        for (io.github.talib.metadata.InputInfo in : f.inputs()) {
            switch (in.type()) {
                case PRICE:
                    for (int b = 0; b < PRICE_BITS.length; b++) {
                        if ((in.flags() & PRICE_BITS[b]) != 0) {
                            slots.add(PRICE_SLOTS[b]);
                        }
                    }
                    break;
                case REAL:
                    slots.add(in.paramName());
                    break;
                default:
                    unhandled.add(f.name() + ": input type " + in.type() + " not handled here");
                    break;
            }
        }
        return slots;
    }

    /**
     * A handle's current value as raw components — one element for a
     * single-output handle, one per batch output for a {@code Value} record.
     *
     * <p>Boxing through {@link Number} is lossless for both shapes the
     * generator emits ({@code double} and {@code int}), so the callers can
     * still compare bit for bit.
     */
    private static double[] components(Object v) {
        if (v instanceof Number) {
            return new double[] { ((Number) v).doubleValue() };
        }
        java.lang.reflect.RecordComponent[] rc = v.getClass().getRecordComponents();
        if (rc == null) {
            throw new AssertionError("not a value: " + v.getClass());
        }
        double[] out = new double[rc.length];
        for (int i = 0; i < rc.length; i++) {
            try {
                out[i] = ((Number) rc[i].getAccessor().invoke(v)).doubleValue();
            } catch (ReflectiveOperationException e) {
                throw new AssertionError(e);
            }
        }
        return out;
    }

    /** Component-wise {@link #bitEq(double, double)}; a length mismatch is a
     *  mismatch, never an exception. */
    private static boolean allBitEq(double[] a, double[] b) {
        if (a.length != b.length) {
            return false;
        }
        for (int i = 0; i < a.length; i++) {
            if (!bitEq(a[i], b[i])) {
                return false;
            }
        }
        return true;
    }

    /**
     * The one method {@code c} declares under {@code name}, taking the
     * {@code double} shape.
     *
     * <p>There is exactly one opener per function today (176 methods, no
     * {@code float[]} overload). The filter is there so that adding one later
     * makes this sweep keep testing the {@code double} API rather than picking
     * whichever overload {@code getMethods()} happened to return first — an
     * order the JLS does not specify, which is the difference between a gate
     * that fails and a gate that flakes.
     */
    private static java.lang.reflect.Method methodNamed(Class<?> c, String name) {
        for (java.lang.reflect.Method m : c.getMethods()) {
            if (!m.getName().equals(name) || m.getDeclaringClass() != c) {
                continue;
            }
            boolean floatShape = false;
            for (Class<?> p : m.getParameterTypes()) {
                floatShape |= p == float[].class || p == float.class;
            }
            if (!floatShape) {
                return m;
            }
        }
        return null;
    }

    /**
     * {@code peek} does not commit and {@code copy} forks — on EVERY handle the
     * registry publishes, not on the handful they were proven on (#172 C4).
     *
     * <p>These two are the Java-specific half of the streaming contract, and
     * what is asserted here is per function, so a tier that forgot to deep-copy
     * one field is a named failure rather than a coverage gap.
     *
     * <p><b>The gap this closes is the RANGE, not the fork.</b> The server's
     * {@code stream_verify} does fork a handle, and drives the fork and the
     * original to the end against batch, so a {@code copy()} that returned
     * {@code this} goes red there too. What that leg compares is values; its
     * range checks sit on the fill, the prefix, the update-fill and the
     * anchored open — none on a copy, none after a peek. So a copy constructor
     * that carries every numeric field and forgets the two range fields is
     * invisible to every other gate in the tree. That is what earns this sweep
     * its place, and why the range assertions must not be traded for the value
     * ones as the cheaper half.
     *
     * <p>Four properties, each one a defect if it fails:
     * <ol>
     *   <li>{@code peek} leaves {@code value()} and {@code outRange()} where
     *       they were, and returns what the {@code update} that follows returns;
     *   <li>{@code update} advances {@code outRange().count()} by exactly one;
     *   <li>a fresh {@code copy()} carries the range and the value; and
     *   <li>updating the copy moves NOTHING on the original, and feeding the
     *       original that same bar afterwards reproduces the copy bit for bit.
     * </ol>
     *
     * <p>Property 4 is what a shared-state {@code copy()} fails: the original's
     * count moves with the copy's update. It is checked on the count as well as
     * on the value because a count always moves, where a value need not — a CDL
     * pattern returning 0 on both bars would hide a shared handle behind a
     * coincidence.
     *
     * <p><b>Sabotage-proved by rewriting the generator's emitter, each result
     * paired with the same sabotage driven through {@code stream_verify}, and
     * the miss recorded rather than rounded off:</b>
     * <ul>
     *   <li>{@code copy()} → {@code return this}: 176 of 176 named here, and
     *       the range assertions are what do it — the value assertions alone
     *       name 108, so 68 handles would have shared state silently.
     *       {@code stream_verify} also goes red.
     *   <li>the copy constructor drops the two range fields: 176 of 176 named
     *       here, {@code stream_verify} fully green. The one defect class with
     *       no other cover.
     *   <li>{@code peek} moves the range: named here, {@code stream_verify}
     *       also goes red.
     * </ul>
     * With every {@code peek} rewritten to step the handle instead of a scratch
     * copy (95 of the 176 handles are emitted in that shape), it names 81 of the
     * 95. The 14 it does not are candlestick patterns whose output is 0 on both
     * sides of the corruption; making them observable needs bars that trigger
     * the pattern, which is what the MC/DC suites (#219) are for, not a corpus
     * this sweep can carry.
     */
    private static void peekAndCopyHoldOnEveryHandle(Core core) {
        java.util.List<String> unhandled = new java.util.ArrayList<String>();
        int swept = 0;

        for (io.github.talib.metadata.FunctionInfo f : io.github.talib.metadata.Functions.all()) {
            String name = f.name();
            /* The registry name and the Java spelling are two different strings
             * since #278 (HT_TRENDLINE -> HtTrendlineStream, htTrendlineOpen), so
             * both are derived from the registry rather than hardcoded: a rename
             * that misses a function shows up here as a named miss. */
            String handleName = pascalCase(name) + "Stream";
            Class<?> handle = null;
            for (Class<?> nested : Core.class.getDeclaredClasses()) {
                if (nested.getSimpleName().equals(handleName)) {
                    handle = nested;
                    break;
                }
            }
            if (handle == null) {
                unhandled.add(name + ": no " + handleName);
                continue;
            }
            java.lang.reflect.Method open = methodNamed(Core.class, camelCase(name) + "Open");
            java.lang.reflect.Method update = methodNamed(handle, "update");
            java.lang.reflect.Method peek = methodNamed(handle, "peek");
            java.lang.reflect.Method copy = methodNamed(handle, "copy");
            java.lang.reflect.Method value = methodNamed(handle, "value");
            java.lang.reflect.Method range = methodNamed(handle, "outRange");
            if (open == null || update == null || peek == null
                    || copy == null || value == null || range == null) {
                unhandled.add(name + ": handle is missing one of open/update/peek/copy/value/outRange");
                continue;
            }

            java.util.List<String> slots = declaredSlots(f, unhandled);
            boolean unitDomain = f.group().equals("Math Transform");
            int lookback = f.newCall(core).lookback();
            int n = lookback + 4;            // enough produced values for a count to be wrong in
            Class<?>[] pt = open.getParameterTypes();
            Object[] args = new Object[pt.length];
            int slot = 0;
            boolean skip = false;
            for (int i = 0; i < pt.length; i++) {
                if (pt[i] == double[].class) {
                    if (slot >= slots.size()) {
                        unhandled.add(name + ": the opener takes more series than the registry declares ("
                                      + slots.size() + ")");
                        skip = true;
                        break;
                    }
                    double[] a = new double[n];
                    for (int k = 0; k < n; k++) {
                        a[k] = slotBar(slots.get(slot), k, unitDomain);
                    }
                    args[i] = a;
                    slot++;
                } else if (pt[i] == int.class) {
                    args[i] = Integer.MIN_VALUE;       // documented default
                } else if (pt[i] == double.class) {
                    args[i] = Core.REAL_DEFAULT;
                } else if (pt[i] == MAType.class) {
                    args[i] = MAType.SMA;
                } else {
                    unhandled.add(name + ": unhandled opener parameter " + pt[i].getName());
                    skip = true;
                    break;
                }
            }
            if (skip) {
                continue;
            }
            if (slot != slots.size()) {
                unhandled.add(name + ": the registry declares " + slots.size()
                              + " series, the opener takes " + slot);
                continue;
            }
            if (update.getParameterCount() != slots.size()
                    || peek.getParameterCount() != slots.size()) {
                unhandled.add(name + ": update/peek take " + update.getParameterCount() + "/"
                              + peek.getParameterCount() + " bars, the opener " + slots.size()
                              + " series");
                continue;
            }

            Object[] barA = new Object[slots.size()];
            Object[] barB = new Object[slots.size()];
            Object[] barUp = new Object[slots.size()];
            Object[] barDown = new Object[slots.size()];
            for (int j = 0; j < slots.size(); j++) {
                barA[j] = Double.valueOf(slotBar(slots.get(j), n, unitDomain));
                barB[j] = Double.valueOf(slotBar(slots.get(j), n + 1, unitDomain));
                barUp[j] = Double.valueOf(outlierBar(slots.get(j), n, true, unitDomain));
                barDown[j] = Double.valueOf(outlierBar(slots.get(j), n, false, unitDomain));
            }

            try {
                Object h = open.invoke(core, args);
                Object ref = open.invoke(core, args);   // the same handle, never peeked
                double[] v0 = components(value.invoke(h));
                OutRange r0 = (OutRange) range.invoke(h);

                /* 1 — peek commits nothing, probed with a bar far enough off
                 * the series to move a window or reclassify a candle. */
                peek.invoke(h, barUp);
                peek.invoke(h, barDown);
                check(allBitEq(v0, components(value.invoke(h))),
                      name + ": peek must not commit value()");
                check(r0.equals(range.invoke(h)),
                      name + ": peek must not move outRange()");
                swNonCommit++;

                /* 1b/2 — and it predicts the update that follows. */
                double[] peeked = components(peek.invoke(h, barA));
                double[] updated = components(update.invoke(h, barA));
                check(allBitEq(peeked, updated), name + ": peek == the update that follows");
                /* ...having left nothing of the two peeks behind. value() alone
                 * cannot see that: a handle whose ring was corrupted by a
                 * committed peek still reports whatever its output is for the
                 * bar just fed, and for a candlestick pattern that is 0 either
                 * way. The reference handle is what makes the corruption
                 * visible — same opener, same bar, never peeked. */
                check(allBitEq(updated, components(update.invoke(ref, barA))),
                      name + ": peek left state behind (the next update differs from a handle that never peeked)");
                check(allBitEq(updated, components(value.invoke(h))),
                      name + ": value() == the last update");
                OutRange r1 = (OutRange) range.invoke(h);
                check(r1.equals(new OutRange(r0.begIdx(), r0.count() + 1)),
                      name + ": update adds one to outRange.count (" + r0 + " -> " + r1 + ")");
                if (!allBitEq(v0, updated)) {
                    swMoved++;
                }

                /* 3 — a fresh copy carries the range and the value. */
                Object c = copy.invoke(h);
                check(r1.equals(range.invoke(c)), name + ": copy carries the range");
                check(allBitEq(updated, components(value.invoke(c))),
                      name + ": copy carries the value");

                /* 4 — and forks: the copy's update moves only the copy. */
                double[] onCopy = components(update.invoke(c, barB));
                check(r1.equals(range.invoke(h)),
                      name + ": the copy's update moved the original's outRange");
                check(allBitEq(updated, components(value.invoke(h))),
                      name + ": the copy's update moved the original's value()");
                double[] onOriginal = components(update.invoke(h, barB));
                check(allBitEq(onCopy, onOriginal),
                      name + ": copy is equivalent (same bar, same bits)");
                check(range.invoke(h).equals(range.invoke(c)),
                      name + ": copy and original agree on the range after the same bar");
                swCopy++;
                swept++;
            } catch (java.lang.reflect.InvocationTargetException e) {
                unhandled.add(name + " -> " + e.getCause().getClass().getSimpleName()
                              + ": " + e.getCause().getMessage());
            } catch (IllegalAccessException e) {
                unhandled.add(name + ": " + e);
            }
        }

        for (String u : unhandled) {
            System.out.println("  (not swept: " + u + ")");
        }
        check(unhandled.isEmpty(), "every registered handle is reachable and opens on its own lookback");
        int registered = io.github.talib.metadata.Functions.all().size();
        check(swept == registered,
              "the peek/copy sweep covered every registered handle (" + swept + "/" + registered + ")");
        /* Non-vacuity. The counters are incremented at their assertions, and
         * swMoved is the one that says the corpus discriminates at all: a series
         * that left every handle's value where the open put it would satisfy
         * every property above without exercising one of them. */
        check(swNonCommit == registered && swCopy == registered,
              "both halves ran on every handle (" + swNonCommit + " peek, " + swCopy + " copy of "
              + registered + ")");
        check(swMoved > registered / 2,
              "the sweep's bars move most handles off their open value (" + swMoved + "/"
              + registered + ")");
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
        Core.SmaStream s = core.smaOpen(java.util.Arrays.copyOf(close, lb + 1), 14);
        check(bitEq(s.value(), batch[0]), "open value == first batch output");
        /* The handle's range is the batch range over the bars it has been fed
         * (issue #241) — checked at every bar, so an increment that fires on the
         * wrong side of a reject or skips a tier shows up at the bar it happens
         * rather than only at the end. */
        check(s.outRange().equals(new OutRange(lb, 1)),
              "a plain open over lookback + 1 bars reports (lookback, 1)");
        for (int t = lb + 1; t < n; t++) {
            double peeked = s.peek(close[t]);
            check(s.outRange().equals(new OutRange(lb, t - lb)), "peek does not move outRange @" + t);
            double updated = s.update(close[t]);
            check(bitEq(peeked, updated), "peek == update @" + t);
            check(bitEq(s.value(), updated), "value() == update @" + t);
            check(bitEq(updated, batch[t - batchRange.begIdx()]), "update == batch @" + t);
            check(s.outRange().equals(new OutRange(lb, t - lb + 1)),
                  "update adds one to outRange.count @" + t);
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

        /* The invariant #241 exists for: feed a stream n bars by ANY mixture of
         * opener and updates and its outRange is the batch range over those same
         * n bars. `s` above was opened over lookback + 1 bars and updated to the
         * end of the series; `f` takes the whole history in one openAndFill.
         * Neither is ever empty — a successful open writes at least one value —
         * so OutRange.EMPTY no longer separates the two openers. */
        check(s.outRange().equals(batchRange),
              "open + updates over n bars == the batch range over n bars");
        double[] warm = new double[batchRange.count()];
        Core.SmaStream f = core.smaOpenAndFill(close, 14, warm);
        check(f.outRange().equals(batchRange), "openAndFill outRange == the batch range");
        check(bitEq(warm[batchRange.count() - 1], f.value()),
              "last filled value == the handle's value");
        check(f.copy().outRange().equals(batchRange), "copy carries the range");
        /* A fork diverges: the copy's count only grows with ITS updates. */
        Core.SmaStream g = f.copy();
        g.update(close[n - 1]);
        check(g.outRange().equals(new OutRange(batchRange.begIdx(), batchRange.count() + 1)),
              "a copy's own update extends only the copy");
        check(f.outRange().equals(batchRange), "the original is untouched by the copy's update");

        /* A history shorter than the bank's shared anchor is InsufficientHistory,
         * not a handle carrying a nonsense range (#241).
         *
         * Honest about what this does and does not gate: MAVP rejects here at
         * its own-lookback precheck, which predates #241, so this would pass
         * with or without the post-clamp history re-check that commit 96d1052f8
         * added — measured, by removing that guard from the emitter. Outside
         * Rust the re-check is not reachable from the public API at all (the
         * public openers pass startIdx = 0, where the clamp is a no-op), and the
         * one caller that anchors is the OpenInternal seam, contracted on
         * startIdx <= endIdx: its transcribed bodies index before they check, so
         * driving it out of contract is undefined rather than a rejection —
         * taAdOpenInternal(45, 40) segfaults under ASan. The re-check is
         * gated in the generator instead, by
         * identity_anchor_clamps_before_it_rechecks_in_every_backend. What this
         * asserts is the public contract around it, which is worth its own line.
         */
        try {
            core.mavpOpen(java.util.Arrays.copyOf(close, 10),
                           java.util.Arrays.copyOf(close, 10), 1, 30, MAType.SMA);
            check(false, "mavpOpen on a history shorter than the bank's anchor must throw");
        } catch (InsufficientHistoryException e) {
            /* expected */
        }
        /* The positive half, so this is not a rejection sweep: one more bar than
         * the anchor needs, and the range is the anchor and the bars after it. */
        {
            int mavpLb = core.MAVP_Lookback(1, 30, MAType.SMA);
            double[] px = java.util.Arrays.copyOf(close, mavpLb + 3);
            Core.MavpStream mv = core.mavpOpen(px, px, 1, 30, MAType.SMA);
            check(mv.outRange().equals(new OutRange(mavpLb, 3)),
                  "mavpOpen just past its anchor reports (lookback, 3), got " + mv.outRange());
        }

        /* Exceptions: typed insufficient history; plain IAE for bad params;
         * aliasing rejection on openAndFill; update/peek never throw. */
        try {
            core.smaOpen(java.util.Arrays.copyOf(close, lb), 14);
            check(false, "short history must throw");
        } catch (InsufficientHistoryException e) {
            check(e instanceof IllegalArgumentException, "IHE extends IAE");
        }
        openMessagesNameTheirOwnFunction(core);
        try {
            core.smaOpen(close, -3);
            check(false, "bad param must throw");
        } catch (InsufficientHistoryException e) {
            check(false, "bad param must NOT be typed as insufficient history");
        } catch (IllegalArgumentException e) {
            /* expected */
        }
        try {
            core.smaOpenAndFill(close, 14, close);
            check(false, "openAndFill output aliasing input must throw");
        } catch (IllegalArgumentException e) {
            /* expected */
        }

        /* Integer.MIN_VALUE keeps its batch meaning (documented default). */
        check(bitEq(core.smaOpen(close, Integer.MIN_VALUE).value(),
                    core.smaOpen(close, 30).value()),
              "MIN_VALUE selects the default");

        /* Multi-output Value: named components, equals/hashCode/toString. */
        Core.MacdStream m = core.macdOpen(close, 12, 26, 9);
        Core.MacdStream.Value v1 = m.update(close[n - 1]);
        check(m.value() == v1, "multi-output value() returns the cached instance");
        Core.MacdStream.Value v2 = m.peek(close[n - 1] + 1.0);
        check(!v1.equals(v2), "distinct bars produce non-equal Values");
        check(v1.toString().contains("macdSignal="), "Value toString names fields");
        java.util.HashSet<Core.MacdStream.Value> set = new java.util.HashSet<Core.MacdStream.Value>();
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
        Core.MacdStream.Value vOpen = core.macdOpen(close, 12, 26, 9).value();
        int lastM = mr.count() - 1;
        check(bitEq(vOpen.macd(),       bM[lastM]), "Value.macd() == batch outMACD");
        check(bitEq(vOpen.macdSignal(), bS[lastM]), "Value.macdSignal() == batch outMACDSignal");
        check(bitEq(vOpen.macdHist(),   bH[lastM]), "Value.macdHist() == batch outMACDHist");
        /* Value is a record, so a consumer on JDK 21+ can destructure it in a
         * record pattern. Asserted by reflection rather than by the pattern
         * itself: this suite compiles at --release 17, where the syntax does not
         * exist. */
        check(Core.MacdStream.Value.class.isRecord(), "Value is a record");
        check(Core.MacdStream.Value.class.getRecordComponents().length == 3,
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
                if (nested.getSimpleName().equals(pascalCase(vf.name()) + "Stream")) {
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
            Core.MaStream ma =
                core.maOpen(close, 14, ty);
            ma.update(close[n - 1]);
        }

        /* Settings are per-instance and frozen into the stream at open: a core
         * with a huge BodyDoji factor calls every candle a doji, the default
         * core calls none of these one. */
        Core tuned = Core.builder()
            .candleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 1.0e9)
            .build();
        Core.CdldojiStream d1 = core.cdldojiOpen(
            java.util.Arrays.copyOf(open, 30), java.util.Arrays.copyOf(high, 30),
            java.util.Arrays.copyOf(low, 30), java.util.Arrays.copyOf(close, 30));
        Core.CdldojiStream d2 = tuned.cdldojiOpen(
            java.util.Arrays.copyOf(open, 30), java.util.Arrays.copyOf(high, 30),
            java.util.Arrays.copyOf(low, 30), java.util.Arrays.copyOf(close, 30));
        check(d1.value() == 0 && d2.value() == 100,
              "candle settings captured per Core instance");

        nonFiniteInputsAreRejected(core, open, high, low, close);
        updateAndFillCommitsThePrefix(core, open, high, low, close);
        peekAndCopyHoldOnEveryHandle(core);


        if (failures == 0) {
            System.out.println("StreamSmokeTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("StreamSmokeTest: " + failures + " FAILURES");
            System.exit(1);
        }
    }
}
