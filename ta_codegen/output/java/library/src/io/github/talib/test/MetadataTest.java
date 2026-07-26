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
 *  072626 MF,CC  First Version — shipped registry + call-by-name.
 */

package io.github.talib.test;

import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;
import io.github.talib.metadata.FuncFlags;
import io.github.talib.metadata.FunctionInfo;
import io.github.talib.metadata.Functions;
import io.github.talib.metadata.InputFlags;
import io.github.talib.metadata.InputInfo;
import io.github.talib.metadata.InputType;
import io.github.talib.metadata.OptInputInfo;
import io.github.talib.metadata.OptInputType;
import io.github.talib.metadata.OutputInfo;
import io.github.talib.metadata.OutputFlags;
import io.github.talib.metadata.OutputType;
import io.github.talib.metadata.ParamHolder;

/**
 * The shipped introspection registry and its call-by-name path.
 *
 * <p>The headline check is {@link #callByNameMatchesTheTypedApi()}: every one of
 * the 168 functions is driven <i>both</i> ways — through the typed method and
 * through {@code Functions.byName(...).newCall()} — and the outputs must agree
 * <b>bit for bit</b>. That is what makes the generated dispatch trustworthy: an
 * argument mis-ordering in the generated switch cannot hide, because it would
 * have to produce identical doubles by accident.
 */
public class MetadataTest {

    private static int failures = 0;
    private static int checks = 0;

    private static void check(boolean condition, String what) {
        checks++;
        if (!condition) {
            failures++;
            System.out.println("  FAIL: " + what);
        }
    }

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

    /* ------------------------------------------------------------ test data */

    private static final int N = 160;

    private static double[] series(double base, double amp, double phase) {
        double[] out = new double[N];
        for (int i = 0; i < N; i++) {
            out[i] = base + amp * Math.sin(i / 9.0 + phase) + 0.4 * Math.cos(i / 3.0 + phase);
        }
        return out;
    }

    private static final double[] CLOSE = series(100.0, 8.0, 0.0);
    private static final double[] OPEN = series(100.0, 8.0, 0.35);
    private static final double[] HIGH = high();
    private static final double[] LOW = low();
    private static final double[] VOLUME = series(1.0e6, 2.0e5, 1.1);
    private static final double[] OPENINT = series(5.0e5, 1.0e5, 0.7);

    private static double[] high() {
        double[] o = new double[N];
        for (int i = 0; i < N; i++) {
            o[i] = Math.max(CLOSE[i], OPEN[i]) + 1.0 + Math.abs(Math.sin(i * 1.7));
        }
        return o;
    }

    private static double[] low() {
        double[] o = new double[N];
        for (int i = 0; i < N; i++) {
            o[i] = Math.min(CLOSE[i], OPEN[i]) - 1.0 - Math.abs(Math.cos(i * 1.3));
        }
        return o;
    }

    /* ------------------------------------------------------------- registry */

    static void registryIsComplete() {
        check(Functions.all().size() >= 160,
              "registry lists every function (" + Functions.all().size() + ")");
        check(Functions.byName("SMA") != null, "byName(SMA)");
        check(Functions.byName("NOSUCHFUNC") == null, "byName of an unknown name is null");
        check(!Functions.groups().isEmpty(), "groups() is populated");

        // Every row must be internally coherent.
        for (FunctionInfo f : Functions.all()) {
            check(!f.name().isEmpty() && !f.group().isEmpty() && !f.javaMethodName().isEmpty(),
                  f.name() + ": name/group/camelCase populated");
            check(!f.outputs().isEmpty(), f.name() + ": has at least one output");
            check(Functions.byName(f.name()) == f, f.name() + ": byName round-trips");
        }
    }

    /** The gap the retired hand-written island never closed. */
    static void hintsArePopulated() {
        long withHint = Functions.all().stream().filter(f -> !f.hint().isEmpty()).count();
        check(withHint == Functions.all().size(),
              "every function carries a hint (" + withHint + "/" + Functions.all().size()
              + ") — the old meta/ island populated none");

        long optTotal = Functions.all().stream().mapToLong(f -> f.optInputs().size()).sum();
        long optWithHint = Functions.all().stream().flatMap(f -> f.optInputs().stream())
                             .filter(o -> !o.hint().isEmpty()).count();
        check(optTotal > 0 && optWithHint == optTotal,
              "every optional parameter carries a hint (" + optWithHint + "/" + optTotal
              + ") — the old island did not even declare the field");
    }

    /** Flag vocabularies the island left unnamed, so consumers hardcoded the bits. */
    static void flagVocabularyIsComplete() {
        FunctionInfo mama = Functions.byName("MAMA");
        OutputInfo fama = mama.outputs().get(1);
        check((fama.flags() & OutputFlags.NULLABLE) != 0,
              "MAMA outFAMA carries the named NULLABLE bit (was the bare literal 8196)");

        FunctionInfo bb = Functions.byName("BBANDS");
        check((bb.outputs().get(0).flags() & OutputFlags.UPPER_LIMIT) != 0,
              "BBANDS upper band carries UPPER_LIMIT");
        check((bb.outputs().get(2).flags() & OutputFlags.LOWER_LIMIT) != 0,
              "BBANDS lower band carries LOWER_LIMIT");

        check(Functions.byName("RSI").hasFlags(FuncFlags.UNSTABLE_PERIOD),
              "RSI is flagged unstable-period");
        check(!Functions.byName("SMA").hasFlags(FuncFlags.UNSTABLE_PERIOD),
              "SMA is not (so the check above is not vacuous)");
        // VOLUME_USED is defined in the C flag vocabulary but set by NO function --
        // in the YAML, in the C tables, and therefore here. AD and OBV consume
        // volume and sit in the "Volume Indicators" group without carrying the bit.
        // Pinned rather than asserted away: if a definition ever sets it, this
        // tells you, instead of the fact quietly changing under consumers.
        long volumeFlagged = Functions.all().stream()
            .filter(f -> f.hasFlags(FuncFlags.VOLUME_USED)).count();
        check(volumeFlagged == 0,
              "no function sets VOLUME_USED (C parity; got " + volumeFlagged + ")");
        check(Functions.all().stream().anyMatch(f -> f.group().equals("Volume Indicators")),
              "the Volume Indicators group exists regardless");
        check(Functions.byName("CDLDOJI").hasFlags(FuncFlags.CANDLESTICK), "CDLDOJI is a candlestick");

        FunctionInfo stoch = Functions.byName("STOCH");
        InputInfo price = stoch.inputs().get(0);
        check(price.type() == InputType.PRICE, "STOCH takes a price bundle");
        int hlc = InputFlags.PRICE_HIGH | InputFlags.PRICE_LOW | InputFlags.PRICE_CLOSE;
        check((price.flags() & hlc) == hlc, "STOCH's bundle declares H+L+C");
        check((price.flags() & InputFlags.PRICE_VOLUME) == 0, "STOCH's bundle excludes volume");

        OptInputInfo maType = Functions.byName("MA").optInputs().get(1);
        check(maType.type() == OptInputType.INTEGER_LIST, "MA's matype is an enumerated list");
        check(maType.valueList() != null && maType.valueList().contains("0=SMA"),
              "MA's matype enumerates its values");
    }

    /* --------------------------------------------------- call-by-name parity */

    /** Binds every declared parameter of `f` onto a fresh holder. */
    private static ParamHolder bind(FunctionInfo f, double[][] outs, int[][] iouts) {
        ParamHolder h = f.newCall();
        for (int i = 0; i < f.inputs().size(); i++) {
            InputInfo in = f.inputs().get(i);
            switch (in.type()) {
                case PRICE -> h.setPriceInput(i, OPEN, HIGH, LOW, CLOSE, VOLUME, OPENINT);
                case REAL -> h.setInput(i, i == 0 ? CLOSE : HIGH);
                case INTEGER -> {
                    int[] ints = new int[N];
                    for (int k = 0; k < N; k++) {
                        ints[k] = k;
                    }
                    h.setInput(i, ints);
                }
            }
        }
        for (int i = 0; i < f.outputs().size(); i++) {
            if (f.outputs().get(i).type() == OutputType.REAL) {
                h.setOutput(i, outs[i]);
            } else {
                h.setOutput(i, iouts[i]);
            }
        }
        return h;
    }

    /**
     * Every function, both ways, bit-for-bit. Optional parameters are left unset
     * so both paths take the documented defaults — which also exercises the
     * holder's default-filling.
     */
    static void callByNameMatchesTheTypedApi() throws Exception {
        int compared = 0;
        int nonEmpty = 0;
        for (FunctionInfo f : Functions.all()) {
            int nout = f.outputs().size();
            double[][] outsA = new double[nout][N];
            int[][] ioutsA = new int[nout][N];
            double[][] outsB = new double[nout][N];
            int[][] ioutsB = new int[nout][N];

            // Path A: call-by-name.
            OutRange a;
            try {
                a = bind(f, outsA, ioutsA).call(0, N - 1);
            } catch (RuntimeException e) {
                failures++;
                checks++;
                System.out.println("  FAIL: " + f.name() + " call-by-name threw " + e);
                continue;
            }

            // Path B: the typed method, reached generically only to keep this test
            // one screen long — the dispatch under test is the generated switch,
            // not this reflection.
            OutRange b = typedCall(f, outsB, ioutsB);
            if (b == null) {
                continue;   // no comparable typed overload found; reported below
            }

            compared++;
            if (a.count() > 0) {
                nonEmpty++;
            }
            check(a.equals(b), f.name() + ": ranges agree (" + a + " vs " + b + ")");
            for (int k = 0; k < nout; k++) {
                boolean same = true;
                if (f.outputs().get(k).type() == OutputType.REAL) {
                    for (int i = 0; i < a.count(); i++) {
                        if (Double.doubleToRawLongBits(outsA[k][i])
                            != Double.doubleToRawLongBits(outsB[k][i])) {
                            same = false;
                        }
                    }
                } else {
                    for (int i = 0; i < a.count(); i++) {
                        if (ioutsA[k][i] != ioutsB[k][i]) {
                            same = false;
                        }
                    }
                }
                check(same, f.name() + " output " + k + ": call-by-name is bit-identical");
            }
        }
        check(compared >= 160, "compared nearly every function (" + compared + ")");
        check(nonEmpty >= compared - 5,
              "almost every comparison produced values (" + nonEmpty + "/" + compared
              + ") — an all-empty run would compare nothing");
    }

    /**
     * Invokes the typed public wrapper reflectively, purely as an independent
     * second path for the comparison above. (The library itself uses no
     * reflection; this is test scaffolding.)
     */
    private static OutRange typedCall(FunctionInfo f, double[][] outs, int[][] iouts)
            throws Exception {
        java.util.List<Object> args = new java.util.ArrayList<>();
        java.util.List<Class<?>> types = new java.util.ArrayList<>();
        args.add(0);
        types.add(int.class);
        args.add(N - 1);
        types.add(int.class);

        for (int i = 0; i < f.inputs().size(); i++) {
            InputInfo in = f.inputs().get(i);
            switch (in.type()) {
                case PRICE -> {
                    int[] bits = { InputFlags.PRICE_OPEN, InputFlags.PRICE_HIGH, InputFlags.PRICE_LOW,
                                   InputFlags.PRICE_CLOSE, InputFlags.PRICE_VOLUME,
                                   InputFlags.PRICE_OPENINTEREST };
                    double[][] comp = { OPEN, HIGH, LOW, CLOSE, VOLUME, OPENINT };
                    for (int k = 0; k < bits.length; k++) {
                        if ((in.flags() & bits[k]) != 0) {
                            args.add(comp[k]);
                            types.add(double[].class);
                        }
                    }
                }
                case REAL -> {
                    args.add(i == 0 ? CLOSE : HIGH);
                    types.add(double[].class);
                }
                case INTEGER -> {
                    int[] ints = new int[N];
                    for (int k = 0; k < N; k++) {
                        ints[k] = k;
                    }
                    args.add(ints);
                    types.add(int[].class);
                }
            }
        }
        for (OptInputInfo o : f.optInputs()) {
            switch (o.type()) {
                case REAL_RANGE, REAL_LIST -> {
                    args.add(-4e37);
                    types.add(double.class);
                }
                case INTEGER_RANGE -> {
                    args.add(Integer.MIN_VALUE);
                    types.add(int.class);
                }
                case INTEGER_LIST -> {
                    args.add(MAType.values()[(int) o.defaultValue()]);
                    types.add(MAType.class);
                }
            }
        }
        for (int i = 0; i < f.outputs().size(); i++) {
            if (f.outputs().get(i).type() == OutputType.REAL) {
                args.add(outs[i]);
                types.add(double[].class);
            } else {
                args.add(iouts[i]);
                types.add(int[].class);
            }
        }

        java.lang.reflect.Method m =
            Core.class.getMethod(f.javaMethodName(), types.toArray(new Class<?>[0]));
        return (OutRange) m.invoke(Core.DEFAULT, args.toArray());
    }

    /* --------------------------------------------------------- holder misuse */

    static void holderRejectsMisuse() {
        FunctionInfo sma = Functions.byName("SMA");
        double[] out = new double[N];

        checkThrows(IllegalArgumentException.class,
            () -> sma.newCall().setInput(5, CLOSE), "input index out of range -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> sma.newCall().setOptInput(9, 30), "optInput index out of range -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> sma.newCall().setOutput(9, out), "output index out of range -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> sma.newCall().setOutput(0, out).call(0, N - 1), "unset input -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> sma.newCall().setInput(0, CLOSE).call(0, N - 1), "unset output -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> sma.newCall().setOptInput(0, 1.5), "wrong optInput type -> IAE");

        // A price-typed input must not accept a bare real series, and vice versa.
        FunctionInfo stoch = Functions.byName("STOCH");
        checkThrows(IllegalArgumentException.class,
            () -> stoch.newCall().setInput(0, CLOSE), "real setter on a PRICE input -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> sma.newCall().setPriceInput(0, OPEN, HIGH, LOW, CLOSE, VOLUME, OPENINT),
            "price setter on a REAL input -> IAE");
        checkThrows(IllegalArgumentException.class,
            () -> stoch.newCall().setPriceInput(0, OPEN, null, LOW, CLOSE, VOLUME, OPENINT),
            "missing a required price component -> IAE");

        // An integer output cannot be bound with a double[] array.
        FunctionInfo doji = Functions.byName("CDLDOJI");
        checkThrows(IllegalArgumentException.class,
            () -> doji.newCall().setOutput(0, out), "double[] on an INTEGER output -> IAE");
    }

    /** Explicitly-set parameters must actually reach the function. */
    static void explicitParametersReachTheFunction() {
        double[] a = new double[N];
        double[] b = new double[N];
        OutRange ra = Functions.byName("SMA").newCall()
            .setInput(0, CLOSE).setOptInput(0, 5).setOutput(0, a).call(0, N - 1);
        OutRange rb = Functions.byName("SMA").newCall()
            .setInput(0, CLOSE).setOptInput(0, 50).setOutput(0, b).call(0, N - 1);

        check(ra.begIdx() == 4 && rb.begIdx() == 49,
              "the bound period reaches the function (begIdx " + ra.begIdx() + " vs " + rb.begIdx() + ")");
        check(ra.count() != rb.count(), "different periods give different counts");

        // And against the typed call, bit for bit.
        double[] c = new double[N];
        OutRange rc = Core.DEFAULT.sma(0, N - 1, CLOSE, 5, c);
        check(rc.equals(ra), "explicit-parameter range matches the typed call");
        boolean same = true;
        for (int i = 0; i < rc.count(); i++) {
            if (Double.doubleToRawLongBits(a[i]) != Double.doubleToRawLongBits(c[i])) {
                same = false;
            }
        }
        check(same, "explicit-parameter values match the typed call bit-for-bit");
    }

    /** The registry must not expose mutable state. */
    static void registryIsImmutable() {
        checkThrows(UnsupportedOperationException.class,
            () -> Functions.all().add(null), "all() is unmodifiable");
        checkThrows(UnsupportedOperationException.class,
            () -> Functions.byName("SMA").outputs().add(null), "outputs() is unmodifiable");
    }

    public static void main(String[] args) throws Exception {
        registryIsComplete();
        hintsArePopulated();
        flagVocabularyIsComplete();
        callByNameMatchesTheTypedApi();
        holderRejectsMisuse();
        explicitParametersReachTheFunction();
        registryIsImmutable();

        if (failures == 0) {
            System.out.println("MetadataTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("MetadataTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
