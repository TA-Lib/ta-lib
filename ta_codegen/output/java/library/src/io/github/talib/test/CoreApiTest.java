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
 *  072526 MF,CC  First Version — immutable Core / CoreBuilder surface.
 */

package io.github.talib.test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;

import io.github.talib.CandleSetting;
import io.github.talib.CandleSettingType;
import io.github.talib.Core;
import io.github.talib.CoreBuilder;
import io.github.talib.FuncUnstId;
import io.github.talib.OutRange;
import io.github.talib.RangeType;

/**
 * Junit-free tests for the {@link Core} configuration surface: immutability,
 * the builder, and the settings actually reaching the indicators.
 *
 * <p>Deliberately dependency-free — `junit.jar` is not in the tree and the
 * numerical correctness of the indicators themselves is proven far more strongly
 * by `ta_regtest --codegen` / `--xlang-hash` against the C reference. What is
 * tested here is the *API contract*, which no cross-language harness covers.
 * Each check is a self-contained static method so the migration to JUnit 5,
 * when the Maven track lands, is an `@Test` annotation and nothing else.
 *
 * <p>Every assertion is non-vacuous: the settings checks verify the setting
 * changes a computed result, not merely that a getter echoes what was stored.
 */
public class CoreApiTest {

    private static int failures = 0;
    private static int checks = 0;

    private static void check(boolean condition, String what) {
        checks++;
        if (!condition) {
            failures++;
            System.out.println("  FAIL: " + what);
        }
    }

    /** Asserts that `body` throws `expected`. */
    private static void checkThrows(Class<? extends RuntimeException> expected,
                                    Runnable body, String what) {
        checks++;
        try {
            body.run();
            failures++;
            System.out.println("  FAIL: " + what + " (no exception thrown)");
        } catch (RuntimeException e) {
            if (!expected.isInstance(e)) {
                failures++;
                System.out.println("  FAIL: " + what + " (threw " + e.getClass().getName() + ")");
            }
        }
    }

    /* ---------------------------------------------------------------- data */

    /** Deterministic synthetic closes; enough bars for a 14-period RSI. */
    private static double[] closes(int n) {
        double[] out = new double[n];
        for (int i = 0; i < n; i++) {
            out[i] = 100.0 + 10.0 * Math.sin(i / 7.0) + 3.0 * Math.cos(i / 3.0);
        }
        return out;
    }

    /* ------------------------------------------------------------- checks */

    static void defaultsAreDefaults() {
        for (Core core : new Core[] { Core.DEFAULT, new Core(), Core.builder().build() }) {
            boolean allZero = true;
            for (FuncUnstId id : FuncUnstId.values()) {
                if (id.ordinal() >= FuncUnstId.All.ordinal()) {
                    continue;   // All / None are sentinels, not functions
                }
                if (core.getUnstablePeriod(id) != 0) {
                    allZero = false;
                }
            }
            check(allZero, "a default Core has every unstable period at 0");
        }
    }

    static void builderSetsOnePeriod() {
        Core core = Core.builder().unstablePeriod(FuncUnstId.Rsi, 10).build();
        check(core.getUnstablePeriod(FuncUnstId.Rsi) == 10, "builder sets the named period");
        check(core.getUnstablePeriod(FuncUnstId.Ema) == 0, "builder leaves other periods alone");
        check(Core.DEFAULT.getUnstablePeriod(FuncUnstId.Rsi) == 0,
              "building does not disturb Core.DEFAULT");
    }

    static void builderAllIsSetAll() {
        Core core = Core.builder().unstablePeriod(FuncUnstId.All, 7).build();
        boolean everySlot = true;
        for (FuncUnstId id : FuncUnstId.values()) {
            if (id.ordinal() < FuncUnstId.All.ordinal() && core.getUnstablePeriod(id) != 7) {
                everySlot = false;
            }
        }
        check(everySlot, "FuncUnstId.All sets every unstable period (C TA_FUNC_UNST_ALL)");
    }

    /** The unstable period must actually move the first valid output index. */
    static void unstablePeriodReachesTheIndicator() {
        double[] in = closes(120);
        double[] out0 = new double[in.length], out9 = new double[in.length];

        Core plain = Core.DEFAULT;
        Core tuned = Core.builder().unstablePeriod(FuncUnstId.Rsi, 9).build();

        OutRange r0 = plain.rsi(0, in.length - 1, in, 14, out0);
        OutRange r9 = tuned.rsi(0, in.length - 1, in, 14, out9);
        check(!r0.isEmpty() && !r9.isEmpty(), "both rsi calls produced values");
        check(r9.begIdx() == r0.begIdx() + 9,
              "unstable period shifts begIdx by exactly that many bars ("
              + r0.begIdx() + " -> " + r9.begIdx() + ")");
        check(plain.rsiLookback(14) + 9 == tuned.rsiLookback(14),
              "the lookback is unstable-period aware per Core instance");
    }

    /** A candle setting must actually change a pattern's verdict. */
    static void candleSettingReachesTheIndicator() {
        int n = 60;
        double[] open = new double[n], high = new double[n], low = new double[n], close = new double[n];
        for (int i = 0; i < n; i++) {
            open[i] = 100.0;
            close[i] = 100.5;          // small but non-zero body
            high[i] = 101.0;
            low[i] = 99.0;
        }

        Core tuned = Core.builder()
            .candleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 1.0e9)
            .build();

        int[] outD = new int[n], outT = new int[n];

        OutRange rD = Core.DEFAULT.cdlDoji(0, n - 1, open, high, low, close, outD);
        OutRange rT = tuned.cdlDoji(0, n - 1, open, high, low, close, outT);
        check(!rD.isEmpty() && !rT.isEmpty(), "cdlDoji produced output on both cores");
        check(outD[rD.count() - 1] == 0, "default core: this candle is not a doji");
        check(outT[rT.count() - 1] == 100, "tuned core: a huge BodyDoji factor calls it a doji");
    }

    static void restoreCandleDefaultUndoesAnOverride() {
        CoreBuilder b = Core.builder()
            .candleSetting(CandleSettingType.BodyDoji, RangeType.RealBody, 3, 42.0);
        Core overridden = b.build();
        Core restored = b.restoreCandleDefault(CandleSettingType.BodyDoji).build();

        int n = 60;
        double[] open = new double[n], high = new double[n], low = new double[n], close = new double[n];
        for (int i = 0; i < n; i++) {
            open[i] = 100.0; close[i] = 100.5; high[i] = 101.0; low[i] = 99.0;
        }
        int[] o1 = new int[n], o2 = new int[n], o3 = new int[n];

        OutRange q1 = overridden.cdlDoji(0, n - 1, open, high, low, close, o1);
        OutRange q2 = restored.cdlDoji(0, n - 1, open, high, low, close, o2);
        OutRange q3 = Core.DEFAULT.cdlDoji(0, n - 1, open, high, low, close, o3);

        check(o1[q1.count() - 1] != o3[q3.count() - 1],
              "the override changed the verdict (so the restore below is not vacuous)");
        check(o2[q2.count() - 1] == o3[q3.count() - 1],
              "restoreCandleDefault returns the setting to TA-Lib's default");
    }

    /** A built Core must not observe later builder mutations. */
    static void builtCoreIsIsolatedFromTheBuilder() {
        CoreBuilder b = Core.builder().unstablePeriod(FuncUnstId.Rsi, 3);
        Core first = b.build();
        b.unstablePeriod(FuncUnstId.Rsi, 99);
        Core second = b.build();
        check(first.getUnstablePeriod(FuncUnstId.Rsi) == 3,
              "reusing the builder does not mutate an already-built Core");
        check(second.getUnstablePeriod(FuncUnstId.Rsi) == 99, "the second build sees the new value");
    }

    static void toBuilderRoundTripsAndDoesNotAlias() {
        Core original = Core.builder()
            .unstablePeriod(FuncUnstId.Rsi, 5)
            .candleSetting(CandleSettingType.BodyLong, RangeType.HighLow, 4, 2.0)
            .build();
        Core derived = original.toBuilder().unstablePeriod(FuncUnstId.Ema, 8).build();

        check(derived.getUnstablePeriod(FuncUnstId.Rsi) == 5, "toBuilder carries settings over");
        check(derived.getUnstablePeriod(FuncUnstId.Ema) == 8, "the derived Core has the new setting");
        check(original.getUnstablePeriod(FuncUnstId.Ema) == 0,
              "deriving does not mutate the original Core");
    }

    static void candleSettingIsImmutable() {
        CandleSetting cs = new CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 0.1);
        check(cs.settingType() == CandleSettingType.BodyDoji, "CandleSetting.settingType()");
        check(cs.rangeType() == RangeType.HighLow, "CandleSetting.rangeType()");
        check(cs.avgPeriod() == 10, "CandleSetting.avgPeriod()");
        check(cs.factor() == 0.1, "CandleSetting.factor()");

        // Structural, not behavioural: every field must be final, so no reference
        // that escapes into a Core can ever be reconfigured behind its back.
        boolean allFinal = true;
        for (java.lang.reflect.Field f : CandleSetting.class.getDeclaredFields()) {
            if (!java.lang.reflect.Modifier.isFinal(f.getModifiers())) {
                allFinal = false;
            }
        }
        check(allFinal, "every CandleSetting field is final");
    }

    static void coreFieldsAreFinal() {
        boolean allFinal = true;
        for (java.lang.reflect.Field f : Core.class.getDeclaredFields()) {
            if (java.lang.reflect.Modifier.isStatic(f.getModifiers())) {
                continue;
            }
            if (!java.lang.reflect.Modifier.isFinal(f.getModifiers())) {
                allFinal = false;
                System.out.println("  (non-final Core field: " + f.getName() + ")");
            }
        }
        check(allFinal, "every Core instance field is final (JLS 17.5 safe publication)");
    }

    /** No compatibility knob survives on the Java surface. */
    static void compatibilityIsGone() {
        List<String> found = new ArrayList<String>();
        for (java.lang.reflect.Field f : Core.class.getDeclaredFields()) {
            if (f.getName().toLowerCase().contains("compat")) {
                found.add("field " + f.getName());
            }
        }
        for (java.lang.reflect.Method m : Core.class.getMethods()) {
            if (m.getName().toLowerCase().contains("compatibility")) {
                found.add("method " + m.getName());
            }
        }
        check(found.isEmpty(), "Core has no compatibility surface " + found);
    }

    static void misuseThrows() {
        checkThrows(NullPointerException.class,
            () -> Core.builder().unstablePeriod(null, 1), "null FuncUnstId -> NPE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().unstablePeriod(FuncUnstId.Rsi, -1), "negative period -> IAE");
        checkThrows(NullPointerException.class,
            () -> Core.builder().candleSetting(null, RangeType.HighLow, 1, 1.0),
            "null CandleSettingType -> NPE");
        checkThrows(NullPointerException.class,
            () -> Core.builder().candleSetting(CandleSettingType.BodyDoji, null, 1, 1.0),
            "null RangeType -> NPE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().candleSetting(
                CandleSettingType.AllCandleSettings, RangeType.HighLow, 1, 1.0),
            "AllCandleSettings as a single-setting target -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().candleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, -1, 1.0),
            "negative avgPeriod -> IAE");
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.getUnstablePeriod(null), "null id on the getter -> NPE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.getUnstablePeriod(FuncUnstId.All),
            "FuncUnstId.All has no single value to read -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.getUnstablePeriod(FuncUnstId.None),
            "FuncUnstId.None is not a function -> IAE");
    }

    /**
     * The headline guarantee: one immutable {@code Core} shared by many threads,
     * with no synchronization, produces the same values as a single-threaded run.
     */
    static void sharedAcrossThreads() throws Exception {
        final double[] in = closes(500);
        final Core shared = Core.builder().unstablePeriod(FuncUnstId.Rsi, 4).build();

        final double[] reference = new double[in.length];
        final OutRange refRange = shared.rsi(0, in.length - 1, in, 14, reference);

        final int threads = 8;
        final CountDownLatch start = new CountDownLatch(1);
        final CountDownLatch done = new CountDownLatch(threads);
        final List<String> problems = java.util.Collections.synchronizedList(new ArrayList<String>());

        for (int t = 0; t < threads; t++) {
            new Thread(() -> {
                try {
                    start.await();
                    for (int rep = 0; rep < 50; rep++) {
                        double[] out = new double[in.length];
                        OutRange r = shared.rsi(0, in.length - 1, in, 14, out);
                        if (!r.equals(refRange)) {
                            problems.add("range diverged: " + r + " != " + refRange);
                            return;
                        }
                        for (int i = 0; i < r.count(); i++) {
                            // Bit-exact: same inputs, same code, no shared mutable state.
                            if (Double.doubleToRawLongBits(out[i])
                                != Double.doubleToRawLongBits(reference[i])) {
                                problems.add("value diverged at " + i);
                                return;
                            }
                        }
                    }
                } catch (Throwable e) {
                    problems.add(e.toString());
                } finally {
                    done.countDown();
                }
            }).start();
        }
        start.countDown();
        done.await();
        check(!refRange.isEmpty(), "the threaded check computed something (not vacuous)");
        check(problems.isEmpty(), "8 threads sharing one Core agree bitwise " + problems);
    }

    public static void main(String[] args) throws Exception {
        defaultsAreDefaults();
        builderSetsOnePeriod();
        builderAllIsSetAll();
        unstablePeriodReachesTheIndicator();
        candleSettingReachesTheIndicator();
        restoreCandleDefaultUndoesAnOverride();
        builtCoreIsIsolatedFromTheBuilder();
        toBuilderRoundTripsAndDoesNotAlias();
        candleSettingIsImmutable();
        coreFieldsAreFinal();
        compatibilityIsGone();
        misuseThrows();
        sharedAcrossThreads();

        if (failures == 0) {
            System.out.println("CoreApiTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("CoreApiTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
