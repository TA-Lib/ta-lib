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
import io.github.talib.RetCode;
import io.github.talib.TaLibFailure;
import io.github.talib.metadata.FuncFlags;
import io.github.talib.metadata.FunctionDescription;
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
        // No `>= 160` threshold here: against 168 it would let eight functions
        // disappear without a word. The EXACT set is pinned against C by
        // abstract_for_each_func in test_abstract.c, which fails naming the
        // missing function; what this suite owes is internal coherence, and
        // every assertion below is per-row, so it cannot be satisfied by a
        // shortened registry the way a floor can. (#164)
        check(!Functions.all().isEmpty(), "registry is populated");
        check(Functions.byName("SMA") != null, "byName(SMA)");
        check(Functions.byName("NOSUCHFUNC") == null, "byName of an unknown name is null");
        // Case-insensitivity is a contract, not an accident (issue #278): once
        // each backend spells the streaming API in its own idiom, "SMA" is the
        // only spelling a caller can rely on across all four, so the registry
        // folds ASCII case (Locale.ROOT) the way C's TA_GetFuncHandle now does too.
        check(Functions.byName("sma") == Functions.byName("SMA"),
              "byName is case-insensitive and still reports the canonical instance");
        check(!Functions.groups().isEmpty(), "groups() is populated");

        // groups() and the rows are two views of one thing; they cannot drift
        // apart without this failing. Counting per group and summing also means
        // a row whose group() is absent from groups() is unreachable from the
        // group view and shows up as a shortfall.
        int inGroups = 0;
        for (String g : Functions.groups()) {
            int n = 0;
            for (FunctionInfo f : Functions.all()) {
                if (f.group().equals(g)) {
                    n++;
                }
            }
            check(n > 0, "group '" + g + "' is not empty");
            inGroups += n;
        }
        check(inGroups == Functions.all().size(),
              "every function's group is listed by groups() (" + inGroups + "/"
              + Functions.all().size() + ")");

        // Every row must be internally coherent.
        for (FunctionInfo f : Functions.all()) {
            check(!f.name().isEmpty() && !f.group().isEmpty(),
                  f.name() + ": name/group populated");
            check(!f.outputs().isEmpty(), f.name() + ": has at least one output");
            check(Functions.byName(f.name()) == f, f.name() + ": byName round-trips");
        }
    }

    /**
     * byName folds ASCII case; the name the registry stores does not.
     *
     * <p>Swept over the corpus rather than spot-checked on {@code "sma"}: the
     * long names ({@code CDL3STARSINSOUTH}) and the ones carrying a digit or an
     * underscore ({@code HT_DCPERIOD}) are the ones a partial fold gets wrong,
     * and no single case stands in for them.
     */
    static void byNameFoldsAsciiCase() {
        int canonical = 0;
        for (FunctionInfo f : Functions.all()) {
            FunctionInfo lower = Functions.byName(asciiLower(f.name()));
            FunctionInfo mixed = Functions.byName(alternating(f.name()));
            check(lower == f, f.name() + ": lower-case lookup finds it");
            check(mixed == f, f.name() + ": mixed-case lookup finds it");

            // "Canonical" has to name something for a fold to fold onto it.
            // Both spellings probed above are derived from the stored one, so
            // a row stored in lower case folds onto itself just as happily and
            // neither lookup notices; this is the line that does. Same
            // assertion C (test_abstract.c, nameFoldCb) and Rust
            // (abstract_api.rs, registry_tests) carry.
            //
            // It is not the line that used to stand here. That one asserted
            // lower.name().equals(f.name()), "the name reported back stays
            // canonical", which cannot fail: `lower == f` above pins the two
            // to one object, so it compared a name to itself.
            //
            // A lower-cased row is not invisible to Java without this -- and
            // that is the reason to say it rather than leave it. BY_NAME is
            // keyed on the STORED spelling while byName folds upward, so the
            // row becomes unreachable by its own name and four checks fail
            // (byName(SMA), byName round-trips, and both lookups above),
            // none of them naming the defect. This one names it.
            check(isStoredUpperCase(f.name()),
                  f.name() + ": is stored in its canonical upper case");
            canonical++;
        }

        // The sweep's own non-vacuity guard, the counterpart of C's
        // `nbCanonical == 0` check. It is insurance, not a second
        // discriminator: registryIsComplete() already fails on an empty
        // registry, so the only way here is if all() stopped agreeing with
        // itself between two calls.
        check(canonical > 0 && canonical == Functions.all().size(),
              "the canonical-spelling check ran on every row (" + canonical + "/"
              + Functions.all().size() + ")");

        // Caught rather than chained: a regression here throws, and the suite
        // has to report that as one failed check instead of a stack trace that
        // stops every probe below from running.
        boolean nullIsNull;
        try {
            nullIsNull = Functions.byName(null) == null;
        } catch (RuntimeException e) {
            nullIsNull = false;
        }
        check(nullIsNull, "byName(null) answers null, as it did before the fold");

        // The fold is ASCII-only and it is only a fold: it must not start
        // resolving names no function has. These are the spellings a
        // toUpperCase-based fold widens onto real functions -- Locale.ROOT maps
        // the dotless 'i' (U+0131) onto 'I' and the long 's' (U+017F) onto 'S',
        // so "s\u0131n" reaches SIN and "\u017Fma" reaches SMA. U+0130 is the
        // same trap in the other direction, under a tr_TR default locale.
        check(Functions.byName("S\u0130N") == null, "U+0130 does not fold onto SIN");
        check(Functions.byName("s\u0131n") == null, "U+0131 does not fold onto SIN");
        check(Functions.byName("\u017Fma") == null, "U+017F does not fold onto SMA");
        check(Functions.byName("sma ") == null, "a trailing space is still part of the name");
        check(Functions.byName("ht-dcperiod") == null, "a separator is still part of the name");
        check(Functions.byName("") == null, "the empty name resolves to nothing");
    }

    /**
     * True if the stored spelling carries no ASCII lower case.
     *
     * <p>Spelled as "no lower-case letter" rather than {@code s.equals(upper(s))}
     * so it stays an ASCII claim: a name is upper case here in the same sense
     * {@code asciiLower} is a fold, and neither borrows a locale's opinion.
     */
    private static boolean isStoredUpperCase(String s) {
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c >= 'a' && c <= 'z') {
                return false;
            }
        }
        return true;
    }

    /** ASCII-only lower fold, so the probe cannot inherit the bug it looks for. */
    private static String asciiLower(String s) {
        StringBuilder b = new StringBuilder(s.length());
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            b.append(c >= 'A' && c <= 'Z' ? (char) (c + ('a' - 'A')) : c);
        }
        return b.toString();
    }

    /** Every letter position lands in both cases across the two probes. */
    private static String alternating(String s) {
        StringBuilder b = new StringBuilder(s.length());
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            boolean up = i % 2 != 0;
            if (c >= 'A' && c <= 'Z' && !up) {
                c = (char) (c + ('a' - 'A'));
            } else if (c >= 'a' && c <= 'z' && up) {
                c = (char) (c - ('a' - 'A'));
            }
            b.append(c);
        }
        return b.toString();
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
        // Exact, not a floor: `>= 160` against 168 let eight functions drop out
        // of the comparison silently, and a function whose typed overload cannot
        // be reached is exactly the defect this test exists to find (#164).
        check(compared == Functions.all().size(),
              "compared every function (" + compared + "/" + Functions.all().size() + ")");
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
            Core.class.getMethod(f.name(), types.toArray(new Class<?>[0]));
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

        /* A leg bound to a buffer SHORTER than the requested range -- absent is
           covered above, too short was covered nowhere until #265. Java answers
           because Dispatch calls the PUBLIC tier, so the wrapper's own length
           checks apply; Rust and C# reached the numerics instead and faulted
           inside them. All three answer BadParam now. C is excluded by its own
           API, not by oversight: its setters take a bare pointer and carry no
           length. */
        double[] shortLeg = new double[N / 2];
        checkRetCode(RetCode.BadParam,
            () -> sma.newCall().setInput(0, shortLeg).setOptInput(0, 30)
                     .setOutput(0, new double[N]).call(0, N - 1),
            "an input shorter than the range -> BadParam");
        checkRetCode(RetCode.BadParam,
            () -> sma.newCall().setInput(0, CLOSE).setOptInput(0, 30)
                     .setOutput(0, new double[4]).call(0, N - 1),
            "an output shorter than the produced count -> BadParam");
        // Control: sized to the count actually produced, which is B5's bound --
        // not the width of the requested range.
        int lookback = sma.newCall().setOptInput(0, 30).lookback();
        OutRange exact = sma.newCall().setInput(0, CLOSE).setOptInput(0, 30)
                            .setOutput(0, new double[N - lookback]).call(0, N - 1);
        check(exact.count() == N - lookback,
            "an output sized to the produced count is accepted (" + exact.count() + ")");
    }

    /**
     * A rejected SETTER leaves the holder as it found it — the other half of the
     * rule, the call tier's half having landed with #265.
     *
     * <p>The sharp case is a RE-bind. On a fresh holder a partial write is masked
     * by the unbound-component report; over a bundle that already works, a setter
     * that checks and writes one component at a time commits the ones ahead of
     * the offending one and leaves the rest holding the previous bundle, so the
     * next {@code call} succeeds over a mixture of the two. Java already
     * validates all six before committing (issue #266 names it as the shape the
     * other three copy); this is what pins that it keeps doing so.
     */
    static void aRejectedSetterLeavesTheHolderAsItFoundIt() {
        // WILLR consumes High|Low|Close, so close is the last required component
        // and the natural place to trip the setter.
        FunctionInfo willr = Functions.byName("WILLR");
        // A different PHASE, not a shift: WILLR is (hh - c) / (hh - ll), which a
        // uniform offset leaves unchanged -- the control below would then pass on
        // a setter that did nothing at all.
        double[] high2 = series(100.0, 8.0, 1.3);
        double[] low2 = new double[N];
        double[] close2 = new double[N];
        for (int i = 0; i < N; i++) {
            low2[i] = high2[i] - 4.0;
            close2[i] = high2[i] - 2.0;
        }

        double[] reference = new double[N];
        OutRange want = willr.newCall()
            .setPriceInput(0, null, HIGH, LOW, CLOSE, null, null)
            .setOptInput(0, 14).setOutput(0, reference).call(0, N - 1);
        check(want.count() > 0, "the reference call produced values");

        ParamHolder h = willr.newCall()
            .setPriceInput(0, null, HIGH, LOW, CLOSE, null, null)
            .setOptInput(0, 14).setOutput(0, new double[N]);
        h.call(0, N - 1);
        double[] afterReject = new double[N];
        h.setOutput(0, afterReject);
        checkThrows(IllegalArgumentException.class,
            () -> h.setPriceInput(0, null, high2, low2, null, null, null),
            "close is consumed and was not supplied -> IAE");
        OutRange got = h.call(0, N - 1);
        check(got.begIdx() == want.begIdx() && got.count() == want.count(),
            "the holder still reports the same range after a rejected setter");
        boolean same = true;
        for (int i = 0; i < want.count(); i++) {
            same &= Double.doubleToRawLongBits(afterReject[i])
                 == Double.doubleToRawLongBits(reference[i]);
        }
        check(same, "a rejected setter did not change what the holder computes");

        // Control: a CORRECT rebind must reach the output, or the check above
        // passes for a setter that stopped working altogether.
        double[] afterRebind = new double[N];
        ParamHolder h2 = willr.newCall()
            .setPriceInput(0, null, HIGH, LOW, CLOSE, null, null)
            .setOptInput(0, 14).setOutput(0, afterRebind);
        h2.call(0, N - 1);
        h2.setPriceInput(0, null, high2, low2, close2, null, null);
        h2.call(0, N - 1);
        boolean moved = false;
        for (int i = 0; i < want.count(); i++) {
            moved |= Double.doubleToRawLongBits(afterRebind[i])
                  != Double.doubleToRawLongBits(reference[i]);
        }
        check(moved, "a correct rebind reaches the output");
    }

    /** Runs {@code body} and checks it failed carrying {@code expected}. */
    private static void checkRetCode(RetCode expected, Runnable body, String what) {
        checks++;
        try {
            body.run();
            failures++;
            System.out.println("  FAIL: " + what + " (no exception thrown)");
        } catch (RuntimeException e) {
            if (!(e instanceof TaLibFailure f) || f.retCode() != expected) {
                failures++;
                System.out.println("  FAIL: " + what + " (threw " + e.getClass().getName() + ")");
            }
        }
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
        OutRange rc = Core.DEFAULT.SMA(0, N - 1, CLOSE, 5, c);
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

    /* ------------------------------------------- the choice-list default (#164) */

    /**
     * Setting a parameter to its documented default THROUGH the abstract interface
     * is part of the ABI: C's {@code TA_SetOptInputParamInteger} takes
     * {@code TA_INTEGER_DEFAULT} and the function substitutes the declared default.
     *
     * <p>Java cannot carry the sentinel past the holder -- {@code Core} takes a real
     * {@code MAType} -- so it must resolve there, and must land on exactly what an
     * UNSET parameter lands on. Those two disagreed: unset resolved to the declared
     * default while an explicit sentinel threw "not a valid MAType ordinal".
     *
     * <p>Asserted on outputs, range AND lookback, for every choice-list parameter
     * of every function.
     */
    private static void choiceListSentinelMatchesTheDefault() {
        int covered = 0;
        for (FunctionInfo f : Functions.all()) {
            for (int p = 0; p < f.optInputs().size(); p++) {
                if (f.optInputs().get(p).type() != OptInputType.INTEGER_LIST) {
                    continue;
                }
                int declared = (int) f.optInputs().get(p).defaultValue();

                double[][] oU = newReal(f); int[][] iU = newInt(f);
                double[][] oS = newReal(f); int[][] iS = newInt(f);
                double[][] oE = newReal(f); int[][] iE = newInt(f);

                ParamHolder unset = bind(f, oU, iU);
                ParamHolder sent  = bind(f, oS, iS);
                ParamHolder expl  = bind(f, oE, iE);
                sent.setOptInput(p, Core.INTEGER_DEFAULT);
                expl.setOptInput(p, declared);

                OutRange rU = unset.call(0, N - 1);
                OutRange rS = sent.call(0, N - 1);
                OutRange rE = expl.call(0, N - 1);
                covered++;

                check(rS.equals(rE) && rU.equals(rE),
                    f.name() + "." + f.optInputs().get(p).paramName()
                    + ": sentinel and unset both give the declared default's range");
                check(sent.lookback() == expl.lookback() && unset.lookback() == expl.lookback(),
                    f.name() + "." + f.optInputs().get(p).paramName()
                    + ": and the same lookback (" + sent.lookback() + "/" + expl.lookback() + ")");

                boolean same = true;
                for (int k = 0; k < f.outputs().size(); k++) {
                    for (int j = 0; j < rE.count(); j++) {
                        same &= f.outputs().get(k).type() == OutputType.REAL
                            ? Double.doubleToRawLongBits(oS[k][j]) == Double.doubleToRawLongBits(oE[k][j])
                              && Double.doubleToRawLongBits(oU[k][j]) == Double.doubleToRawLongBits(oE[k][j])
                            : iS[k][j] == iE[k][j] && iU[k][j] == iE[k][j];
                    }
                }
                check(same, f.name() + "." + f.optInputs().get(p).paramName()
                    + ": and bit-identical output values");
            }
        }
        check(covered >= 13, "covered every choice-list parameter (" + covered + ")");
    }

    /**
     * The holder's lookback must agree with what the call actually produces.
     *
     * <p>{@code Dispatch.lookback} is a SECOND, independent copy of the
     * opt-slot-to-argument mapping across all 168 functions, and most of those pass
     * two or more same-typed arguments — so a swapped or duplicated slot still
     * compiles. This is the only gate on it, which is why the parameters are bound
     * to DISTINCT non-default values: with every slot carrying the same number, a
     * transposition is undetectable. {@code Dispatch.call} carries its own separate
     * arg list, so comparing the lookback against the range the call reports pits
     * the two mappings against each other.
     */
    private static void holderLookbackMatchesTheTypedApi() {
        int compared = 0;
        int withDistinct = 0;
        for (FunctionInfo f : Functions.all()) {
            ParamHolder h = bind(f, newReal(f), newInt(f));

            /* Distinct, in-range, non-default where the domain allows it. */
            boolean distinct = false;
            for (int p = 0; p < f.optInputs().size(); p++) {
                OptInputInfo o = f.optInputs().get(p);
                switch (o.type()) {
                    case INTEGER_RANGE -> {
                        int v = Math.min(o.intMin() + 2 + p, o.intMax());
                        h.setOptInput(p, v);
                        distinct = true;
                    }
                    /* MAType.DISABLED short-circuits to a zero lookback and would
                       mask a mis-mapped slot, so stay inside the real algorithms. */
                    case INTEGER_LIST -> h.setOptInput(p, MAType.values()[p % 3]);
                    default -> { }
                }
            }
            if (distinct) {
                withDistinct++;
            }

            int viaHolder = h.lookback();
            OutRange r = h.call(0, N - 1);
            /* `==`, not `>=`. The inequality reads safer and is worthless here: a
               duplicated slot makes the lookback too SMALL, which `>=` accepts.
               Sabotage-proven — emitting adOscLookback(intOpt(0), intOpt(0))
               passes under `>=` and fails under `==` with "lookback 3 == outBegIdx
               4". If a function ever legitimately reports outBegIdx > lookback
               (issue #99 raised that for BBANDS), carve THAT function out by name
               rather than relaxing the operator for all 168. */
            check(viaHolder >= 0 && r.begIdx() == viaHolder,
                f.name() + ": holder lookback " + viaHolder + " == outBegIdx " + r.begIdx());
            compared++;
        }
        check(compared == Functions.all().size(), "checked every function (" + compared + ")");
        /* 69 of the 168 declare an INTEGER_RANGE parameter; the rest take only
           reals, only a choice list, or nothing, and cannot carry a distinct
           period. Floor it so the discriminating half cannot quietly shrink. */
        check(withDistinct >= 65,
            "the functions with a period were driven with distinct non-default ones ("
            + withDistinct + ")");
    }

    /**
     * The shipped XML describes every registered function.
     *
     * <p>test_abstract.c compares its length and byte-sum against C's, which
     * catches corruption but says nothing about what is inside. This says the
     * document actually mentions each function, so a well-formed XML missing a
     * whole entry fails here rather than only when its checksum happens to move.
     */
    static void functionDescriptionXmlDescribesEveryFunction() {
        String xml = FunctionDescription.xml();
        check(xml.startsWith("<?xml"), "the XML description is an XML document");
        check(xml.contains("</FinancialFunctions>"), "the XML description is complete");
        int found = 0;
        for (FunctionInfo f : Functions.all()) {
            if (xml.contains("<Abbreviation>" + f.name() + "</Abbreviation>")) {
                found++;
            } else {
                check(false, "XML describes " + f.name());
            }
        }
        check(found == Functions.all().size(),
              "XML describes every function (" + found + "/" + Functions.all().size() + ")");
    }

    private static double[][] newReal(FunctionInfo f) {
        double[][] a = new double[f.outputs().size()][];
        for (int i = 0; i < a.length; i++) {
            a[i] = new double[N];
        }
        return a;
    }

    private static int[][] newInt(FunctionInfo f) {
        int[][] a = new int[f.outputs().size()][];
        for (int i = 0; i < a.length; i++) {
            a[i] = new int[N];
        }
        return a;
    }

    /**
     * {@code FunctionInfo.newCall(Core)} must route through the {@code Core} it
     * was handed, not {@link Core#DEFAULT}.
     *
     * <p>It is public API with zero callers anywhere — every other metadata test
     * uses the no-arg {@code newCall()}, which hardcodes {@code Core.DEFAULT} —
     * so a binder that dropped the argument on the floor would look perfectly
     * healthy. The oracle is the unstable period, because it is the one setting
     * that visibly moves a lookback: {@code CoreApiTest} already pins
     * {@code plain.rsiLookback(14) + 9 == tuned.rsiLookback(14)} for the typed
     * API, and this asserts the metadata path reaches the same place.
     */
    static void newCallCarriesTheGivenCore() {
        Core tuned = Core.builder().unstablePeriod(io.github.talib.FuncUnstId.RSI, 9).build();
        FunctionInfo rsi = Functions.byName("RSI");

        int viaDefault = rsi.newCall().setOptInput(0, 14).lookback();
        int viaTuned = rsi.newCall(tuned).setOptInput(0, 14).lookback();

        check(viaDefault == Core.DEFAULT.RSI_Lookback(14),
              "newCall() uses Core.DEFAULT (" + viaDefault + ")");
        check(viaTuned == tuned.RSI_Lookback(14),
              "newCall(core) uses the given Core (" + viaTuned + " vs " + tuned.RSI_Lookback(14) + ")");
        check(viaTuned == viaDefault + 9,
              "the unstable period reaches the binder: " + viaDefault + " + 9 == " + viaTuned);

        /* lookback() is not call(). Dispatch reads `h.core()` in two places --
         * `Dispatch.lookback` (above) and `Dispatch.call` -- and only the first
         * of them was pinned. Measured on this tree: rewriting the one line in
         * `Dispatch.call` to `Core core = Core.DEFAULT;`, so that every
         * call-by-name silently ignores the Core it was handed, leaves all six
         * Java suites green (BatchApiTest 181, CoreApiTest 66, DivZeroTest 91,
         * MetadataTest 1540, SMathOverflowTest 4, StreamSmokeTest 3859). The
         * blanket rewrite of BOTH lines fails 2 of 1540 -- the three checks
         * above -- which is exactly the half that was already covered.
         *
         * The unstable period is the oracle again, and it discriminates on the
         * range rather than on the values: it does not change how RSI is
         * computed, it withholds the first 9 bars it computes. So a call that
         * honours `tuned` starts 9 bars later and produces 9 fewer values.
         */
        double[] outDefault = new double[N];
        OutRange rDefault = rsi.newCall()
            .setInput(0, CLOSE).setOptInput(0, 14).setOutput(0, outDefault).call(0, N - 1);
        double[] outTuned = new double[N];
        OutRange rTuned = rsi.newCall(tuned)
            .setInput(0, CLOSE).setOptInput(0, 14).setOutput(0, outTuned).call(0, N - 1);

        check(rTuned.begIdx() == rDefault.begIdx() + 9,
              "call(core) starts where that Core's lookback says: "
              + rDefault.begIdx() + " + 9 == " + rTuned.begIdx());
        check(rTuned.count() == rDefault.count() - 9,
              "call(core) withholds the 9 unstable bars: "
              + rDefault.count() + " - 9 == " + rTuned.count());

        /* And the binder's answer is the typed call's answer on that same Core,
         * bit for bit -- the same standard callByNameMatchesTheTypedApi() holds
         * the 176 functions to on Core.DEFAULT, now on a Core that is not it.
         */
        double[] direct = new double[N];
        OutRange rDirect = tuned.RSI(0, N - 1, CLOSE, 14, direct);
        check(rDirect.begIdx() == rTuned.begIdx() && rDirect.count() == rTuned.count(),
              "the binder and the typed call agree on the range for the tuned Core");
        boolean sameBits = true;
        for (int i = 0; i < rTuned.count(); i++) {
            sameBits &= Double.doubleToRawLongBits(direct[i])
                     == Double.doubleToRawLongBits(outTuned[i]);
        }
        check(sameBits, "the binder and the typed call agree bit for bit on the tuned Core");
        /* Non-vacuity: the bit compare above had something to compare, and the
         * two Cores really do answer differently -- without this a binder that
         * produced nothing at all would satisfy every check above.
         */
        check(rTuned.count() > 0 && rDefault.count() > rTuned.count(),
              "both calls produced values, and the tuned Core produced fewer ("
              + rDefault.count() + " vs " + rTuned.count() + ")");
    }

    public static void main(String[] args) throws Exception {
        registryIsComplete();
        byNameFoldsAsciiCase();
        hintsArePopulated();
        flagVocabularyIsComplete();
        callByNameMatchesTheTypedApi();
        holderRejectsMisuse();
        aRejectedSetterLeavesTheHolderAsItFoundIt();
        explicitParametersReachTheFunction();
        choiceListSentinelMatchesTheDefault();
        holderLookbackMatchesTheTypedApi();
        newCallCarriesTheGivenCore();
        functionDescriptionXmlDescribesEveryFunction();
        registryIsImmutable();

        if (failures == 0) {
            System.out.println("MetadataTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("MetadataTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
