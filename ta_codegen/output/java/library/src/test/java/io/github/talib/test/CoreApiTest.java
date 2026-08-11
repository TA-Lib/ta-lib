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
                if (id == FuncUnstId.ALL) {
                    continue;   // All / None are sentinels, not functions
                }
                if (core.unstablePeriod(id) != 0) {
                    allZero = false;
                }
            }
            check(allZero, "a default Core has every unstable period at 0");
        }
    }

    static void builderSetsOnePeriod() {
        Core core = Core.builder().unstablePeriod(FuncUnstId.RSI, 10).build();
        check(core.unstablePeriod(FuncUnstId.RSI) == 10, "builder sets the named period");
        check(core.unstablePeriod(FuncUnstId.EMA) == 0, "builder leaves other periods alone");
        check(Core.DEFAULT.unstablePeriod(FuncUnstId.RSI) == 0,
              "building does not disturb Core.DEFAULT");
    }

    static void builderAllIsSetAll() {
        Core core = Core.builder().unstablePeriod(FuncUnstId.ALL, 7).build();
        boolean everySlot = true;
        for (FuncUnstId id : FuncUnstId.values()) {
            if (id != FuncUnstId.ALL && core.unstablePeriod(id) != 7) {
                everySlot = false;
            }
        }
        check(everySlot, "FuncUnstId.ALL sets every unstable period (C TA_FUNC_UNST_ALL)");
    }

    /** The unstable period must actually move the first valid output index. */
    static void unstablePeriodReachesTheIndicator() {
        double[] in = closes(120);
        double[] out0 = new double[in.length], out9 = new double[in.length];

        Core plain = Core.DEFAULT;
        Core tuned = Core.builder().unstablePeriod(FuncUnstId.RSI, 9).build();

        OutRange r0 = plain.RSI(0, in.length - 1, in, 14, out0);
        OutRange r9 = tuned.RSI(0, in.length - 1, in, 14, out9);
        check(!r0.isEmpty() && !r9.isEmpty(), "both rsi calls produced values");
        check(r9.begIdx() == r0.begIdx() + 9,
              "unstable period shifts begIdx by exactly that many bars ("
              + r0.begIdx() + " -> " + r9.begIdx() + ")");
        check(plain.RSI_Lookback(14) + 9 == tuned.RSI_Lookback(14),
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

        OutRange rD = Core.DEFAULT.CDLDOJI(0, n - 1, open, high, low, close, outD);
        OutRange rT = tuned.CDLDOJI(0, n - 1, open, high, low, close, outT);
        check(!rD.isEmpty() && !rT.isEmpty(), "cdlDoji produced output on both cores");
        check(outD[rD.count() - 1] == 0, "default core: this candle is not a doji");
        check(outT[rT.count() - 1] == 100, "tuned core: a huge BodyDoji factor calls it a doji");
    }

    /**
     * The property the {@code avgPeriod} bounds exist to preserve (#185), stated
     * over the two tiers rather than over the setter: for every setting the
     * builder accepts, the lookback is a real index count and the call's reported
     * range agrees with it. An out-of-range {@code avgPeriod} broke exactly this
     * — a value near {@code Integer.MAX_VALUE} wrapped the
     * {@code Math.max(...) + N} lookbacks negative.
     */
    static void acceptedCandleSettingsKeepTheLookbackAndTheCallInStep() {
        int n = 40;
        double[] open = new double[n], high = new double[n], low = new double[n], close = new double[n];
        for (int i = 0; i < n; i++) {
            open[i] = 100.0; close[i] = 104.0; high[i] = 105.0; low[i] = 99.0;
        }

        for (int avgPeriod : new int[] { 0, 1, 5, n - 1, n, 100, Core.MAX_INDEX }) {
            Core core = Core.builder()
                .candleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, avgPeriod, 0.1)
                .build();
            int lookback = core.CDLDOJI_Lookback();
            check(lookback >= 0 && lookback <= Core.MAX_INDEX,
                  "avgPeriod " + avgPeriod + ": lookback " + lookback + " is a real index count");

            int[] out = new int[n];
            OutRange r = core.CDLDOJI(0, n - 1, open, high, low, close, out);
            if (lookback > n - 1) {
                check(r.isEmpty(),
                      "avgPeriod " + avgPeriod + ": a lookback past the series produces nothing");
            } else {
                check(r.begIdx() == lookback && r.count() == n - lookback,
                      "avgPeriod " + avgPeriod + ": begIdx " + r.begIdx() + " / count "
                      + r.count() + " agree with lookback " + lookback);
            }
        }
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

        OutRange q1 = overridden.CDLDOJI(0, n - 1, open, high, low, close, o1);
        OutRange q2 = restored.CDLDOJI(0, n - 1, open, high, low, close, o2);
        OutRange q3 = Core.DEFAULT.CDLDOJI(0, n - 1, open, high, low, close, o3);

        check(o1[q1.count() - 1] != o3[q3.count() - 1],
              "the override changed the verdict (so the restore below is not vacuous)");
        check(o2[q2.count() - 1] == o3[q3.count() - 1],
              "restoreCandleDefault returns the setting to TA-Lib's default");
    }

    /** A built Core must not observe later builder mutations. */
    static void builtCoreIsIsolatedFromTheBuilder() {
        CoreBuilder b = Core.builder().unstablePeriod(FuncUnstId.RSI, 3);
        Core first = b.build();
        b.unstablePeriod(FuncUnstId.RSI, 99);
        Core second = b.build();
        check(first.unstablePeriod(FuncUnstId.RSI) == 3,
              "reusing the builder does not mutate an already-built Core");
        check(second.unstablePeriod(FuncUnstId.RSI) == 99, "the second build sees the new value");
    }

    static void toBuilderRoundTripsAndDoesNotAlias() {
        Core original = Core.builder()
            .unstablePeriod(FuncUnstId.RSI, 5)
            .candleSetting(CandleSettingType.BodyLong, RangeType.HighLow, 4, 2.0)
            .build();
        Core derived = original.toBuilder().unstablePeriod(FuncUnstId.EMA, 8).build();

        check(derived.unstablePeriod(FuncUnstId.RSI) == 5, "toBuilder carries settings over");
        check(derived.unstablePeriod(FuncUnstId.EMA) == 8, "the derived Core has the new setting");
        check(original.unstablePeriod(FuncUnstId.EMA) == 0,
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

    /**
     * The other half of the safe-publication promise: final fields guarantee
     * nothing if a subclass can add a non-final one. {@code coreFieldsAreFinal}
     * reflects over fields only, so this is the class-level check it is not.
     */
    static void configClassesAreFinal() {
        check(java.lang.reflect.Modifier.isFinal(Core.class.getModifiers()),
              "Core is final (a subclass could void JLS 17.5 safe publication)");
        check(java.lang.reflect.Modifier.isFinal(CoreBuilder.class.getModifiers()),
              "CoreBuilder is final");
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
            () -> Core.builder().unstablePeriod(FuncUnstId.RSI, -1), "negative period -> IAE");
        // The period is added to a lookback that is then used as an index, so an
        // unbounded one overflows that lookback negative and the function indexes
        // past its input. C rejects anything above TA_MAX_INDEX
        // (src/ta_func/ta_utility.c) and Java must agree, on the single-id path
        // and on the set-all wildcard alike.
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().unstablePeriod(FuncUnstId.RSI, Core.MAX_INDEX + 1),
            "period above MAX_INDEX -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().unstablePeriod(FuncUnstId.RSI, Integer.MAX_VALUE),
            "Integer.MAX_VALUE period -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().unstablePeriod(FuncUnstId.ALL, Core.MAX_INDEX + 1),
            "wildcard period above MAX_INDEX -> IAE");
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
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().candleSetting(
                CandleSettingType.BodyDoji, RangeType.HighLow, Core.MAX_INDEX + 1, 1.0),
            "avgPeriod above MAX_INDEX -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().candleSetting(
                CandleSettingType.BodyDoji, RangeType.HighLow, Integer.MAX_VALUE, 1.0),
            "avgPeriod at Integer.MAX_VALUE -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.builder().candleSetting(
                CandleSettingType.BodyDoji, RangeType.HighLow, 10, Double.NaN),
            "NaN factor -> IAE");
        // A negative factor is legal: it scales a threshold nothing can fall
        // below, so the pattern simply never matches — a plausible thing to ask
        // for, unlike NaN.
        check(Core.builder().candleSetting(
                  CandleSettingType.BodyDoji, RangeType.HighLow, 10, -1.0) != null,
              "a negative factor is accepted");
        // Core.unstablePeriod(id) reads; CoreBuilder.unstablePeriod(id, period)
        // writes. Same name, different class and arity — the immutable Core has
        // no writer for a `get` prefix to disambiguate against.
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.unstablePeriod(null), "null id on the reader -> NPE");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.unstablePeriod(FuncUnstId.ALL),
            "FuncUnstId.ALL has no single value to read -> IAE");
        // The writer accepts All (set-all wildcard) but must reject None the same
        // way the reader does, rather than indexing off the end of the array.
        check(FuncUnstId.ALL.value() == 65535 && FuncUnstId.COUNT == FuncUnstId.values().length - 1,
            "FuncUnstId.ALL is pinned at 65535 and COUNT covers every function id");
    }

    /**
     * The headline guarantee: one immutable {@code Core} shared by many threads,
     * with no synchronization, produces the same values as a single-threaded run.
     */
    static void sharedAcrossThreads() throws Exception {
        final double[] in = closes(500);
        final Core shared = Core.builder().unstablePeriod(FuncUnstId.RSI, 4).build();

        final double[] reference = new double[in.length];
        final OutRange refRange = shared.RSI(0, in.length - 1, in, 14, reference);

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
                        OutRange r = shared.RSI(0, in.length - 1, in, 14, out);
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

    /**
     * The accepting side of the period bound, and the rule that makes the bound
     * worth having: a rejected call must leave the builder exactly as it was.
     * Asserting only that bad input throws would pass just as well against an
     * implementation that threw <em>after</em> writing.
     */
    static void unstablePeriodBoundIsABoundNotAnOffByOne() {
        // MAX_INDEX itself is legal — C accepts it and rejects MAX_INDEX + 1.
        final Core ceiling = Core.builder().unstablePeriod(FuncUnstId.RSI, Core.MAX_INDEX).build();
        check(ceiling.unstablePeriod(FuncUnstId.RSI) == Core.MAX_INDEX,
            "the MAX_INDEX ceiling is accepted, not rejected");

        // A rejected call writes nothing: set a good value, have the next call be
        // refused, and the good value must survive untouched.
        final CoreBuilder b = Core.builder().unstablePeriod(FuncUnstId.EMA, 7);
        checkThrows(IllegalArgumentException.class,
            () -> b.unstablePeriod(FuncUnstId.EMA, Core.MAX_INDEX + 1),
            "the rejected overwrite still throws");
        check(b.build().unstablePeriod(FuncUnstId.EMA) == 7,
            "a rejected unstablePeriod leaves the previous value in place");

        // The wildcard path writes 24 slots, so a rejection there must not have
        // filled any of them before noticing.
        final CoreBuilder w = Core.builder().unstablePeriod(FuncUnstId.ALL, 3);
        checkThrows(IllegalArgumentException.class,
            () -> w.unstablePeriod(FuncUnstId.ALL, Integer.MAX_VALUE),
            "the rejected wildcard still throws");
        final Core after = w.build();
        boolean allIntact = true;
        for (FuncUnstId id : FuncUnstId.values()) {
            if (id != FuncUnstId.ALL && after.unstablePeriod(id) != 3) {
                allIntact = false;
            }
        }
        check(allIntact, "a rejected wildcard leaves all 24 slots at their previous value");
    }

    public static void main(String[] args) throws Exception {
        defaultsAreDefaults();
        builderSetsOnePeriod();
        builderAllIsSetAll();
        unstablePeriodReachesTheIndicator();
        candleSettingReachesTheIndicator();
        acceptedCandleSettingsKeepTheLookbackAndTheCallInStep();
        restoreCandleDefaultUndoesAnOverride();
        builtCoreIsIsolatedFromTheBuilder();
        toBuilderRoundTripsAndDoesNotAlias();
        candleSettingIsImmutable();
        coreFieldsAreFinal();
        configClassesAreFinal();
        compatibilityIsGone();
        misuseThrows();
        unstablePeriodBoundIsABoundNotAnOffByOne();
        sharedAcrossThreads();

        if (failures == 0) {
            System.out.println("CoreApiTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("CoreApiTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
