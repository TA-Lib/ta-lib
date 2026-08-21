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
 *  081826 MF,CC  First Version — the zero-length no-I/O probe (#172 C2).
 *  081826 MF,CC  Exact-extent, unread-leg and openAndFill sweeps (#235).
 */

package io.github.talib;

import io.github.talib.metadata.FunctionInfo;
import io.github.talib.metadata.Functions;
import io.github.talib.metadata.InputFlags;
import io.github.talib.metadata.InputInfo;
import io.github.talib.metadata.InputType;
import io.github.talib.metadata.OptInputInfo;

import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.TreeSet;

/**
 * Negative-space coverage: array sizes chosen so that any access the contract
 * forbids is an {@code ArrayIndexOutOfBoundsException} rather than a comment.
 *
 * <p>The value gates ({@code ta_regtest}, {@code --xlang-hash}, {@code --fuzz-064})
 * can only see work that reaches an output. Work a function does and then discards
 * — a read past {@code endIdx}, a write past the count it reported, a leg it
 * touches on a range where it promised to touch nothing — leaves no trace in any
 * output, so nothing there can see it. C cannot host the check at all (it has no
 * array sizes); Java bounds-checks unconditionally, on every array of every
 * function, for free. Because all four backends are generated from one source
 * under {@code ta_codegen/input/}, a Java-side probe catches C, Rust, Java and C#
 * at once.
 *
 * <p>Four sweeps, each an array-sizing trick:
 *
 * <ol>
 * <li><b>{@link #subLookbackSweep}</b> — a range strictly shorter than the
 *     lookback, with <b>zero-length</b> arrays everywhere. That call is a
 *     documented success with no values and must touch nothing; with no elements,
 *     every index is out of bounds. It caught APO, PPO and PVO computing their
 *     <b>fast</b> MA over the whole requested range before discovering the range
 *     was too short for their own — reading the caller's input to produce a
 *     result the empty slow MA then threw away.</li>
 * <li><b>{@link #exactExtentSweep}</b> — a range that <i>does</i> produce values,
 *     with each input sized to exactly {@code endIdx + 1} and each output to
 *     exactly the count the call reported. A read past {@code endIdx} or a write
 *     past the reported count is then out of bounds. This is the sweep that
 *     reaches the 30 cores with a lookback of 0, for which no sub-lookback range
 *     exists, and it runs at several parameter vectors rather than only the
 *     all-defaults one.</li>
 * <li><b>{@link #unreadLegSweep}</b> — one input leg at a time given a
 *     zero-length array while the rest stay correctly sized. Whether the call
 *     throws is a direct readout of whether that leg is read at all, which pins
 *     two things nothing else does: every function reads at least one of its
 *     declared legs, and the {@code double[]} and {@code float[]} transcriptions
 *     — independently generated bodies — read the same set.</li>
 * <li><b>{@link #openAndFillSweep}</b> — the streaming tier's one array-shaped
 *     promise: {@code NAME_OpenAndFill} fills exactly {@code historyLen - lookback}
 *     values. Sized to exactly that, a writer running past it is out of bounds.
 *     ({@code update} and {@code peek} take scalars, so there is no array to size;
 *     {@code Open} derives its range from the history's own length, so it cannot
 *     over-read one.)</li>
 * </ol>
 *
 * <p>The sweeps drive {@code NAME_Impl} — the numerics, which validate
 * parameters and the index range but <i>not</i> array lengths — on purpose: the
 * public wrapper's length validation would reject an undersized array before
 * the body could touch it, which is a different (and already tested) property.
 * Named by the check rather than called "unguarded", because that word once
 * contrasted with a retired {@code Unguarded} tier and now reads as the
 * opposite of CLAUDE.md's "guarded" for the same tier in Rust.
 *
 * <p>They drove {@code NAME_Internal} until #236 step 5 deleted it. That tier
 * was a catch-and-convert shim over this same body, so the retarget is a rename:
 * {@code NAME_Impl} has the identical C-shaped signature, is package-private for
 * the same reason, and is the tier the shim was calling all along.
 *
 * <p>The C# suite of the same name carries sweeps 1 and 2. Running both is not
 * duplication: a bug in the shared {@code ta_codegen/input/} C shows up in each,
 * but an emitter bug that changes what one backend <i>touches</i> without changing
 * what it <i>produces</i> shows up only in that backend, and is invisible to
 * {@code --xlang-hash} by construction.
 *
 * <p><b>Rust now carries sweeps 1 and 3 too</b>, in the generated
 * {@code ta_codegen/output/rust/library/tests/no_phantom_io.rs} (issue #235).
 * Two things it has that the sweeps below do not. It reaches every core: since
 * #236 step 3 routed cross-calls through the public callee, ten composed cores
 * are out of reach here and are named in {@code CROSS_CALL_GUARDED}, while
 * Rust's cross-calls still target {@code NAME_Impl} and so probe all 174.
 * And it covers the <i>Rust</i> emitter — an emitter bug of the kind described
 * above is invisible to every other backend's probe, by the same argument.
 *
 * <p>Rust can host it because it has no guarded/unguarded split to pick
 * between: {@code pub fn SMA} is a thin {@code Result} mapper over the body,
 * whose bounds check is the {@code assert!} preamble and the indexing itself,
 * so the public API reaches the body directly.
 *
 * <p>Lives in {@code io.github.talib} rather than the sibling {@code .test} package
 * because the cores and {@link MInteger} are package-private; from here they are
 * reachable without {@code setAccessible}, which would be a second thing that could
 * silently stop working.
 *
 * <p>Junit-free, {@code main()}-driven, like the other suites.
 */
public class NoPhantomIoTest {

    private static int failures = 0;
    private static int checks = 0;

    private static void check(boolean condition, String what) {
        checks++;
        if (!condition) {
            failures++;
            System.out.println("  FAIL: " + what);
        }
    }

    /** One violation: counted as a failed check, printed with its sweep's name. */
    private static void violation(String what) {
        checks++;
        failures++;
        System.out.println("  FAIL: " + what);
    }

    /* ------------------------------------------------------------------ model */

    /**
     * One core's call shape, resolved once and shared by every sweep.
     *
     * <p>The positional split is the emitted signature's own order:
     * {@code (startIdx, endIdx, <input arrays>, <optional params>, outBegIdx,
     * outNBElement, <output arrays>)}. Reading it positionally rather than by
     * parameter name is deliberate — the jar is compiled without
     * {@code -parameters}, so names are not there to read.
     */
    private static final class Sig {
        final String name;
        final Method core;
        final Method lookback;
        final FunctionInfo info;
        /** The settings this signature's lookbacks and calls are resolved against. */
        final Core on;
        /** Parameter index of each input array, in call order. */
        final int[] inputPos;
        /** What each input array carries: "open", "high", ..., "inReal", "inPeriods". */
        final String[] legName;
        /** Parameter index of each optional parameter, in call order. */
        final int[] optPos;
        final int begPos;
        final int nbPos;
        /** Parameter index of each output array, in call order. */
        final int[] outputPos;
        /** The parameter vectors to probe at; see {@link #vectors}. Never empty. */
        List<Vector> vectors;

        Sig(String name, Method core, Method lookback, FunctionInfo info, Core on) {
            this.name = name;
            this.core = core;
            this.lookback = lookback;
            this.info = info;
            this.on = on;

            Class<?>[] pt = core.getParameterTypes();
            List<Integer> in = new ArrayList<>();
            List<Integer> opt = new ArrayList<>();
            List<Integer> out = new ArrayList<>();
            int i = 2;
            while (i < pt.length && pt[i].isArray()) {
                in.add(i++);
            }
            while (i < pt.length && pt[i] != MInteger.class) {
                opt.add(i++);
            }
            this.begPos = i++;
            this.nbPos = i++;
            while (i < pt.length) {
                out.add(i++);
            }
            this.inputPos = in.stream().mapToInt(Integer::intValue).toArray();
            this.optPos = opt.stream().mapToInt(Integer::intValue).toArray();
            this.outputPos = out.stream().mapToInt(Integer::intValue).toArray();
            this.legName = legNames(info);
        }
    }

    /**
     * The OHLCV components each declared input carries, flattened into one leg
     * per emitted array.
     *
     * <p>A {@link InputType#PRICE} input is one metadata row but several arrays —
     * {@code inPriceHLC} is three. The order is C's, low bit first, which is the
     * order {@code ta_abstract} and every backend emit them in.
     */
    private static String[] legNames(FunctionInfo info) {
        List<String> legs = new ArrayList<>();
        for (InputInfo in : info.inputs()) {
            if (in.type() != InputType.PRICE) {
                legs.add(in.paramName());
                continue;
            }
            int f = in.flags();
            if ((f & InputFlags.PRICE_OPEN) != 0) {
                legs.add("open");
            }
            if ((f & InputFlags.PRICE_HIGH) != 0) {
                legs.add("high");
            }
            if ((f & InputFlags.PRICE_LOW) != 0) {
                legs.add("low");
            }
            if ((f & InputFlags.PRICE_CLOSE) != 0) {
                legs.add("close");
            }
            if ((f & InputFlags.PRICE_VOLUME) != 0) {
                legs.add("volume");
            }
            if ((f & InputFlags.PRICE_OPENINTEREST) != 0) {
                legs.add("openinterest");
            }
        }
        return legs.toArray(new String[0]);
    }

    /**
     * Every core of one width, paired with its lookback and its metadata row.
     *
     * @param wantFloat resolve the {@code float[]} overload rather than the
     *        {@code double[]} one — the two carry independent transcriptions of
     *        the same body, so a fix applied to one only would pass here otherwise.
     */
    private static Map<String, Sig> discover(boolean wantFloat, Core on) {
        Map<String, Method> cores = new TreeMap<>();
        Map<String, Method> lookbacks = new HashMap<>();
        for (Method m : Core.class.getDeclaredMethods()) {
            String n = m.getName();
            if (n.endsWith("_Impl")) {
                boolean isFloat = m.getParameterTypes()[2] == float[].class;
                if (isFloat == wantFloat) {
                    cores.put(n.substring(0, n.length() - "_Impl".length()), m);
                }
            } else if (n.endsWith("_Lookback")) {
                lookbacks.put(n.substring(0, n.length() - "_Lookback".length()), m);
            }
        }
        Map<String, Sig> out = new TreeMap<>();
        for (Map.Entry<String, Method> e : cores.entrySet()) {
            String name = e.getKey();
            Method lb = lookbacks.get(name);
            check(lb != null, name + " has a lookback method");
            FunctionInfo info = Functions.byName(name);
            check(info != null, name + " has a metadata row");
            if (lb == null || info == null) {
                continue;
            }
            Sig sig = new Sig(name, e.getValue(), lb, info, on);
            // The metadata registry and the emitted signature are generated from
            // the same input/ directory by different emitters. If they disagree on
            // how many arrays a function takes, every leg name below is off by one
            // and the sweeps would silently probe the wrong thing.
            check(sig.legName.length == sig.inputPos.length,
                  name + " metadata legs match the emitted signature ("
                  + sig.legName.length + " vs " + sig.inputPos.length + ")");
            check(sig.optPos.length == info.optInputs().size(),
                  name + " metadata optional parameters match the emitted signature ("
                  + info.optInputs().size() + " vs " + sig.optPos.length + ")");
            check(sig.outputPos.length == info.outputs().size(),
                  name + " metadata outputs match the emitted signature ("
                  + info.outputs().size() + " vs " + sig.outputPos.length + ")");
            sig.vectors = vectors(sig);
            check(!sig.vectors.isEmpty(),
                  name + " has at least one parameter vector its own lookback accepts");
            // The enum axis, pinned against the enum's own constant count. Every
            // constant either yields a vector or is rejected by the function's own
            // lookback, so a function that stops being swept over its choice list
            // -- the axis that found the BBANDS bug -- is red rather than quiet.
            int enumConstants = 0;
            for (int p : sig.optPos) {
                Class<?> t = sig.core.getParameterTypes()[p];
                if (t.isEnum()) {
                    enumConstants += t.getEnumConstants().length;
                }
            }
            int enumVectors = 0;
            for (Vector v : sig.vectors) {
                if (v.label.contains("=")) {
                    enumVectors++;
                }
            }
            check(enumConstants == 0 || enumVectors > 0,
                  name + " is swept over its choice list (" + enumVectors
                  + " vector(s) from " + enumConstants + " constant(s))");
            out.put(name, sig);
        }
        return out;
    }

    /* --------------------------------------------------------------- fixtures */

    /**
     * Bar {@code i} of one leg of a deterministic series.
     *
     * <p>Ordered so the OHLC relations hold ({@code low <= open,close <= high}),
     * because a pattern that reads a bar it should not is the thing being caught,
     * not a pattern that takes a different branch on nonsense input.
     */
    private static double bar(String leg, int i) {
        double base = 100.0 + 10.0 * Math.sin(i / 7.0) + 3.0 * Math.cos(i / 3.0);
        switch (leg) {
            case "open":   return base - 0.5;
            case "high":   return base + 2.0;
            case "low":    return base - 2.0;
            case "close":  return base + 0.5;
            case "volume": return 1000.0 + i;
            // MAVP's per-bar period. It is the one leg whose value sets how far
            // back the function reads, so the fixture has to reach the deepest
            // legal bar or the exact-extent sweep leaves slack below the window
            // and asserts nothing for it: every third bar asks for more than any
            // documented maximum, which mavp clamps to optInMaxPeriod -- the bar
            // its own lookback is computed from. The rest stay short so the
            // period buckets are not all one value.
            case "inPeriods": return (i % 3 == 0) ? 1.0e5 : 5.0 + (i % 7);
            default:       return base;
        }
    }

    /** {@code len} bars of {@code leg}, as the array type the overload wants. */
    private static Object series(Class<?> type, String leg, int len) {
        if (type == float[].class) {
            float[] a = new float[len];
            for (int i = 0; i < len; i++) {
                a[i] = (float) bar(leg, i);
            }
            return a;
        }
        double[] a = new double[len];
        for (int i = 0; i < len; i++) {
            a[i] = bar(leg, i);
        }
        return a;
    }

    /** An empty array of the declared type. */
    private static Object zeroArray(Class<?> t) {
        return Array.newInstance(t.getComponentType(), 0);
    }

    /** The all-defaults value for one optional parameter, by declared type. */
    private static Object defaultFor(Class<?> t) {
        if (t == int.class) {
            return Integer.MIN_VALUE;
        }
        if (t == double.class) {
            return Core.REAL_DEFAULT;
        }
        if (t.isEnum()) {
            for (Object c : t.getEnumConstants()) {
                if (c.toString().equals("DEFAULT")) {
                    return c;
                }
            }
            return t.getEnumConstants()[0];
        }
        throw new IllegalStateException("unhandled optional parameter type " + t);
    }

    /* ------------------------------------------------- parameter vectors */

    /** One candidate parameter vector, with a label for the failure message. */
    private static final class Vector {
        final String label;
        final Object[] values;
        final int lookback;

        Vector(String label, Object[] values, int lookback) {
            this.label = label;
            this.values = values;
            this.lookback = lookback;
        }
    }

    /**
     * The parameter vectors to probe {@code sig} at.
     *
     * <p>The all-defaults vector alone probes a function whose I/O extent depends
     * on a parameter at exactly one point. Added to it: every parameter at its
     * documented minimum (which is where the period-1 identity fast paths live),
     * and each enum constant in turn (which is where a {@code MAType} selects an
     * entirely different sub-indicator, {@code DISABLED} included).
     *
     * <p>Validity is not judged here: a vector is kept only if
     * {@code NAME_Lookback} accepts it, which it signals by returning a lookback
     * of {@code -1} for any out-of-range parameter. So an impossible combination
     * drops out by the library's own rule rather than by a list kept in step
     * by hand.
     *
     * <p>Deliberately a <b>star</b> around the defaults, not a product: one axis
     * moves per vector. So a body reachable only by two parameters at once — the
     * all-EMA delegation MACDEXT takes when both of its MA types are EMA, say — is
     * not entered. The product of every enum against every other is where the cost
     * stops being worth it; this shape already found the one live bug the enum axis
     * had (BBANDS at {@code MAType.DISABLED}).
     */
    private static List<Vector> vectors(Sig sig) {
        Class<?>[] pt = sig.core.getParameterTypes();
        int n = sig.optPos.length;

        Object[] defaults = new Object[n];
        for (int k = 0; k < n; k++) {
            defaults[k] = defaultFor(pt[sig.optPos[k]]);
        }

        Map<String, Object[]> candidates = new LinkedHashMap<>();
        candidates.put("defaults", defaults);

        if (n > 0) {
            Object[] mins = new Object[n];
            for (int k = 0; k < n; k++) {
                Class<?> t = pt[sig.optPos[k]];
                OptInputInfo oi = sig.info.optInputs().get(k);
                mins[k] = t == int.class ? (Object) oi.intMin()
                        : t == double.class ? (Object) oi.min()
                        : defaults[k];
            }
            candidates.put("minimums", mins);
        }

        // Each enum constant on its own, and again with every integer parameter
        // raised. Raised, because for a composed function the two axes interact:
        // BBANDS bails early only when the moving average's lookback is BELOW the
        // deviation's, which at MAType.MAMA (a constant lookback of 32) needs a
        // period above 33 -- a bar the default of 20 never reaches. The enum alone
        // pinned the DISABLED half of that guard and left the MAMA half unpinned.
        Object[] raised = defaults.clone();
        boolean anyRaised = false;
        for (int k = 0; k < n; k++) {
            if (pt[sig.optPos[k]] != int.class) {
                continue;
            }
            OptInputInfo oi = sig.info.optInputs().get(k);
            int doubled = (int) Math.min(2.0 * oi.defaultValue(), (double) oi.intMax());
            if (doubled > (int) oi.defaultValue()) {
                raised[k] = doubled;
                anyRaised = true;
            }
        }
        for (int k = 0; k < n; k++) {
            Class<?> t = pt[sig.optPos[k]];
            if (!t.isEnum()) {
                continue;
            }
            String param = sig.info.optInputs().get(k).paramName();
            for (Object c : t.getEnumConstants()) {
                Object[] v = defaults.clone();
                v[k] = c;
                candidates.put(param + "=" + c, v);
                if (anyRaised) {
                    Object[] w = raised.clone();
                    w[k] = c;
                    candidates.put(param + "=" + c + ", periods doubled", w);
                }
            }
        }

        List<Vector> out = new ArrayList<>();
        for (Map.Entry<String, Object[]> e : candidates.entrySet()) {
            int lookback;
            try {
                lookback = (Integer) sig.lookback.invoke(sig.on, e.getValue());
            } catch (ReflectiveOperationException ex) {
                violation(sig.name + "_Lookback(" + e.getKey() + ") threw " + ex.getCause());
                continue;
            }
            if (lookback >= 0) {
                out.add(new Vector(e.getKey(), e.getValue(), lookback));
            }
        }
        return out;
    }

    /**
     * The one core whose sub-lookback probe is out of reach, and why it is one.
     *
     * <p>The sweep works by handing a core ZERO-LENGTH arrays and reading what
     * happens: silence means no I/O, a fault means the detector is live. Since
     * #236 step 3 a transcribed body calls its callee's PUBLIC entry point, and
     * the callee's input bound (rule B-5a) requires {@code endIdx + 1} elements —
     * deliberately without the sub-lookback escape the OUTPUT bound takes. A core
     * that forwards on a range shorter than its own lookback therefore answers
     * before touching an array, and the probe cannot tell "read nothing" from
     * "never ran".
     *
     * <p>Nothing about the PUBLIC API moved: reached through the caller's own
     * wrapper the callee's check is provably redundant, same {@code endIdx}, same
     * array.
     *
     * <p><b>The fix for this is an early return, and eight of the original ten
     * already have it.</b> {@code apo}, {@code bbands}, {@code ppo}, {@code pvo}
     * and now {@code stddev} return {@code 0,0} before forwarding when the range
     * is shorter than their lookback; the rest never forwarded on such a range to
     * begin with. {@code ma} is the holdout, and not for want of trying: it is a
     * DISPATCH function, and the generator admits only decls, comments, the
     * identity path, one switch and a final return at the top level of a dispatch
     * body — the shape the stream planner is built on. A guard there is a
     * generator change, not a one-line edit to {@code ma.c}.
     *
     * <p>The other way out is #236 deciding the input bound does not keep its
     * stricter reading. Either route empties this list.
     *
     * <p>An explicit list, not a symptom test: a core that starts answering
     * {@code BadParam} here for any other reason is still a hard failure. The
     * size is asserted, so the debt can be paid down but not quietly grown.
     *
     * <p><b>Paying it down has a second edge.</b> The mechanism that withholds
     * {@code ma} — a composed body cross-calling its callee's public tier — is
     * the only thing that makes C#'s {@code FunctionCall.TryInvoke} catch
     * reachable, since its thunks call the body, which answers a code. Route
     * cross-calls to {@code _Impl} and this list empties, but that catch goes
     * dead and {@code TryInvoke} silently stops converting anything. The two
     * move together; change them together.
     */
    private static final java.util.Set<String> CROSS_CALL_GUARDED = java.util.Set.of("MA");

    /* -------------------------------------------------- sweep 1: sub-lookback */

    /**
     * Every guarded core, over a range strictly shorter than its lookback, called
     * with zero-length arrays for every input and every output.
     *
     * <p>Such a call is a documented success with no values: it must return
     * {@link RetCode#Success}, report a count of 0, and touch neither array.
     *
     * <p>Each core also gets its own control arm before the quiet call: the same
     * call one bar longer produces exactly one value, so it must index an array,
     * so with zero-length arrays it must throw. That turns "this core was silent"
     * into "this core was silent <i>and</i> the detector was working on it",
     * which a single shared control (on SMA alone) cannot say.
     */
    private static void subLookbackSweep(boolean wantFloat, Map<String, Sig> cores) {
        String tier = tier(wantFloat, cores);
        int probed = 0;
        int noSubLookbackRange = 0;
        int violations = 0;
        List<String> live = new ArrayList<>();
        List<String> withheld = new ArrayList<>();

        for (Sig sig : cores.values()) {
            // The per-function control arm. One bar longer than the quiet range is
            // a call that produces exactly one value, so it MUST index an array --
            // and with zero-length arrays that is a throw. A core that stops
            // throwing here has stopped computing, and its silence in the sweep
            // below would otherwise read as compliance. This is also what covers
            // the 30 cores whose lookback is 0: they have no quiet range, but they
            // do have this one.
            Vector defaults = sig.vectors.get(0);
            if (CROSS_CALL_GUARDED.contains(sig.name)) {
                withheld.add(sig.name);
                continue;
            }
            Object[] one = args(sig, defaults, 0, defaults.lookback);
            for (int p : sig.inputPos) {
                one[p] = zeroArray(sig.core.getParameterTypes()[p]);
            }
            for (int p : sig.outputPos) {
                one[p] = zeroArray(sig.core.getParameterTypes()[p]);
            }
            try {
                Object rc = sig.core.invoke(sig.on, one);
                violation(tier + " " + sig.name + " at endIdx == lookback ("
                    + defaults.lookback + ") returned " + rc + " without touching an "
                    + "array; a call that produces a value must index one, so this "
                    + "sweep could not detect I/O for it");
                violations++;
            } catch (InvocationTargetException ite) {
                // Two kinds count, and the KIND is the whole signal -- the same
                // discipline the Rust port applies to panics.
                //
                // An IndexOutOfBoundsException is this core indexing a
                // zero-length array: it computed.
                //
                // A TaLibArgumentException is a LENGTH check, and the core under
                // test is NAME_Impl, which has none. So it can only have come from
                // a callee's public tier, which this core reached by cross-calling
                // it -- which is equally a proof that the core still computes, and
                // is the only proof available for a composed function since #236
                // step 3 routed cross-calls through the public callee. Anything
                // else is a core that stopped computing, and its silence in the
                // sweep below would read as compliance.
                Throwable cause = ite.getCause();
                if (cause instanceof IndexOutOfBoundsException
                        || cause instanceof TaLibArgumentException) {
                    live.add(sig.name);
                } else {
                    violation(tier + " " + sig.name + " at endIdx == lookback threw "
                        + cause);
                    violations++;
                }
            } catch (ReflectiveOperationException ex) {
                violation(tier + " " + sig.name + " could not be invoked: " + ex);
                violations++;
            }

            for (Vector v : sig.vectors) {
                // A lookback of 0 means every bar produces a value, so no range is
                // short enough to expect no I/O from. Counted, not silently dropped;
                // exactExtentSweep is what reaches these.
                if (v.lookback < 1) {
                    if (v.label.equals("defaults")) {
                        noSubLookbackRange++;
                    }
                    continue;
                }
                if (v.label.equals("defaults")) {
                    probed++;
                }

                Object[] args = args(sig, v, 0, v.lookback - 1);
                for (int p : sig.inputPos) {
                    args[p] = zeroArray(sig.core.getParameterTypes()[p]);
                }
                for (int p : sig.outputPos) {
                    args[p] = zeroArray(sig.core.getParameterTypes()[p]);
                }
                MInteger nb = (MInteger) args[sig.nbPos];

                String where = tier + " " + sig.name + "[" + v.label + "] (lookback "
                    + v.lookback + ", endIdx " + (v.lookback - 1) + ")";
                try {
                    Object rc = sig.core.invoke(sig.on, args);
                    if (rc != RetCode.Success) {
                        violation(where + " returned " + rc + ", expected Success");
                        violations++;
                    } else if (nb.value != 0) {
                        violation(where + " reported " + nb.value
                                  + " values on a sub-lookback range");
                        violations++;
                    }
                } catch (InvocationTargetException ite) {
                    Throwable t = ite.getCause();
                    violation(where + " touched an array: " + t.getClass().getSimpleName()
                              + ": " + t.getMessage());
                    violations++;
                } catch (ReflectiveOperationException ex) {
                    violation(where + " could not be invoked: " + ex);
                    violations++;
                }
            }
        }

        // Every discovered core is accounted for at the defaults vector: probed, or
        // explicitly counted as having no sub-lookback range. Nothing may fall out
        // silently in between.
        check(probed + noSubLookbackRange + withheld.size() == cores.size(),
              tier + " sub-lookback: every core is probed, counted or withheld ("
              + probed + " + " + noSubLookbackRange + " + " + withheld.size()
              + " vs " + cores.size() + ")");
        check(probed > 0 && noSubLookbackRange > 0,
              tier + " sub-lookback: both outcomes occur, so neither branch is dead ("
              + probed + " probed, " + noSubLookbackRange + " skipped)");
        check(live.size() + withheld.size() == cores.size(),
              tier + " sub-lookback: the detector is proved live for every core that "
              + "is not withheld (" + live.size() + " + " + withheld.size() + " of "
              + cores.size() + "; not proved "
              + missing(missing(new ArrayList<>(cores.keySet()), live), withheld) + ")");
        // The debt cannot grow silently: the list is what it is, and a core that
        // leaves it has to leave this number too.
        check(withheld.size() == CROSS_CALL_GUARDED.size(),
              tier + " sub-lookback: every withheld core is one of the "
              + CROSS_CALL_GUARDED.size() + " named in CROSS_CALL_GUARDED (got "
              + withheld.size() + ": " + withheld + ")");
        System.out.println("  " + tier + " sub-lookback: " + probed + " cores probed, "
            + violations + " violation(s), " + noSubLookbackRange
            + " skipped (lookback 0, no sub-lookback range exists); "
            + live.size() + " detector control(s) fired; " + withheld.size()
            + " WITHHELD, out of this sweep's reach since #236 step 3 -> " + withheld);
    }

    /* ------------------------------------------------- sweep 2: exact extent */

    /**
     * The ranges the exact-extent sweep uses, as offsets from the lookback.
     *
     * <p>{@code {0, 0}} is the tightest output there is — one value, so an output
     * array of length 1, which catches a writer that stores two and reports one.
     *
     * <p>The first two start at {@code startIdx == lookback}, which is what makes
     * the input bound <b>two-sided</b>: the legal read window is then exactly
     * {@code [0..endIdx]}, so a read below it is a negative index and a read above
     * it is past the end — both out of bounds. {@code {3, 7}} gives that up (its
     * legal window starts at 3, and 0..2 are inside the array) in exchange for the
     * one path the others never take: {@code startIdx > lookback}, so the body's
     * {@code startIdx = lookbackTotal} clamp does <i>not</i> fire.
     */
    private static final int[][] RANGES = { {0, 0}, {0, 4}, {3, 7} };

    /**
     * Every core, over ranges that <i>do</i> produce values, with each input sized
     * to exactly {@code endIdx + 1} and each output to exactly the count the call
     * reported.
     *
     * <p>Two passes. The first uses generously padded arrays and reads off what
     * the call says it produced; the second re-runs it at exactly that size. The
     * report is not taken on trust in between: it is held to the documented
     * {@code begIdx == startIdx && count == endIdx - startIdx + 1} <i>first</i>,
     * and only then used as the bound. That ordering is the point — sizing to the
     * report alone is fail-open, since a body that writes N+1 values and reports
     * N+1 satisfies it while a caller who allocated by the published formula
     * overflows.
     */
    private static void exactExtentSweep(boolean wantFloat, Map<String, Sig> cores) {
        String tier = tier(wantFloat, cores);
        final int PAD = 16;
        int probes = 0;
        int violations = 0;
        List<String> reached = new ArrayList<>();

        for (Sig sig : cores.values()) {
            boolean reachedAtDefaults = false;
            for (Vector v : sig.vectors) {
                for (int[] r : RANGES) {
                    int startIdx = v.lookback + r[0];
                    int endIdx = v.lookback + r[1];
                    if (endIdx < startIdx) {
                        continue;
                    }

                    // Pass 1: padded, to learn the count. A throw here is not an
                    // over-read of one element -- it is a read far outside the
                    // range, and worth its own message.
                    Object[] loose = args(sig, v, startIdx, endIdx);
                    fill(sig, loose, endIdx + 1 + PAD, endIdx - startIdx + 1 + PAD);
                    Object rc;
                    try {
                        rc = sig.core.invoke(sig.on, loose);
                    } catch (InvocationTargetException ite) {
                        violation(tier + " " + sig.name + "[" + v.label + "] "
                            + range(startIdx, endIdx) + " threw on PADDED arrays ("
                            + PAD + " spare elements): "
                            + ite.getCause().getClass().getSimpleName() + ": "
                            + ite.getCause().getMessage());
                        violations++;
                        continue;
                    } catch (ReflectiveOperationException ex) {
                        violation(tier + " " + sig.name + " could not be invoked: " + ex);
                        violations++;
                        continue;
                    }
                    if (rc != RetCode.Success) {
                        // An out-of-range parameter combination the lookback let
                        // through. Not this sweep's business; it simply is not a
                        // call, so there is nothing to hold to a bound.
                        continue;
                    }
                    int count = ((MInteger) loose[sig.nbPos]).value;
                    int begIdx = ((MInteger) loose[sig.begPos]).value;

                    // Sizing pass 2 to the reported count alone would be fail-open:
                    // a body that writes N+1 values AND reports N+1 satisfies it,
                    // while a caller who allocated by the published formula
                    // overflows. So hold the report to the formula first, and only
                    // then use it as the bound.
                    if (begIdx != startIdx || count != endIdx - startIdx + 1) {
                        violation(tier + " " + sig.name + "[" + v.label + "] "
                            + range(startIdx, endIdx) + " reported begIdx " + begIdx
                            + " and count " + count + ", not the documented "
                            + startIdx + " and " + (endIdx - startIdx + 1));
                        violations++;
                        continue;
                    }

                    // Pass 2: exactly what the contract allows the call to touch.
                    Object[] tight = args(sig, v, startIdx, endIdx);
                    fill(sig, tight, endIdx + 1, count);
                    probes++;
                    if (v.label.equals("defaults")) {
                        reachedAtDefaults = true;
                    }
                    try {
                        sig.core.invoke(sig.on, tight);
                    } catch (InvocationTargetException ite) {
                        Throwable t = ite.getCause();
                        violation(tier + " " + sig.name + "[" + v.label + "] "
                            + range(startIdx, endIdx) + " with inputs of " + (endIdx + 1)
                            + " and outputs of " + count + " (the count it reported) "
                            + "touched an element outside them: "
                            + t.getClass().getSimpleName() + ": " + t.getMessage());
                        violations++;
                    } catch (ReflectiveOperationException ex) {
                        violation(tier + " " + sig.name + " could not be invoked: " + ex);
                        violations++;
                    }
                }
            }
            if (reachedAtDefaults) {
                reached.add(sig.name);
            }
        }

        // Fail-closed coverage. A floor ("at least 140 probed") passes in exactly
        // the case that matters -- add ten functions, silently lose ten from
        // discovery, and the floor is still met. Every core must instead be reached
        // at its own defaults vector, on a range built from its own lookback, so a
        // function that stops being reachable is red rather than quiet.
        check(reached.size() == cores.size(),
              tier + " exact extent: every core is reached at its defaults ("
              + reached.size() + " of " + cores.size() + "; missing "
              + missing(new ArrayList<>(cores.keySet()), reached) + ")");
        System.out.println("  " + tier + " exact extent: " + probes + " probes over "
            + cores.size() + " cores, " + violations + " violation(s)");
    }

    /* -------------------------------------------------- sweep 3: unread legs */

    /**
     * One input leg at a time given a zero-length array, on a range that produces
     * values and with every other array correctly sized.
     *
     * <p>The result is a direct readout of which legs a function actually reads —
     * some candlestick patterns declare an OHLC bundle and index only part of it.
     * Note that for a candlestick this is a property of the <b>candle settings</b>,
     * not of the pattern: the emitted body selects the range type at run time
     * ({@code (rangeType == 0) ? |close-open| : (rangeType == 1) ? high-low : ...}),
     * so the untaken arms' legs are never evaluated. The sweep runs against
     * {@link Core#DEFAULT}, and the list it prints is "unread at the default candle
     * settings" — a different {@link CandleSetting} would move legs between the two
     * groups without any generated code changing. That is why the list is printed
     * rather than pinned, and why neither assertion below depends on its contents.
     *
     * <p>Two things are assertable, and neither is circular (nothing here asks the
     * generator what it thinks it emitted):
     *
     * <ul>
     * <li>every function reads at least one of its declared legs — a function that
     *     reads none of its inputs has stopped being an indicator;</li>
     * <li>the {@code double[]} and {@code float[]} bodies read the <i>same</i>
     *     legs. They are independent transcriptions, so this is the one place a
     *     change applied to one and not the other shows up as a shape difference
     *     rather than a value difference no fixture happens to cover.</li>
     * </ul>
     *
     * @return leg names read, by function, for the caller to compare across widths
     */
    private static Map<String, TreeSet<String>> unreadLegSweep(boolean wantFloat,
                                                               Map<String, Sig> cores) {
        String tier = tier(wantFloat, cores);
        Map<String, TreeSet<String>> readLegs = new TreeMap<>();
        List<String> unread = new ArrayList<>();

        int withheld = 0;
        for (Sig sig : cores.values()) {
            // Same reach problem as the sub-lookback sweep, third shape: this one
            // infers "read" from a THROW on a zero-length leg, and since #236 step
            // 3 these ten answer BadParam from a callee's public input bound
            // instead — a return, not a throw. See CROSS_CALL_GUARDED.
            if (CROSS_CALL_GUARDED.contains(sig.name)) {
                withheld++;
                continue;
            }
            Vector v = sig.vectors.get(0);   // defaults; always present
            int startIdx = v.lookback;
            int endIdx = v.lookback + 4;
            TreeSet<String> read = new TreeSet<>();

            for (int leg = 0; leg < sig.inputPos.length; leg++) {
                Object[] args = args(sig, v, startIdx, endIdx);
                fill(sig, args, endIdx + 1, endIdx - startIdx + 1);
                args[sig.inputPos[leg]] =
                    zeroArray(sig.core.getParameterTypes()[sig.inputPos[leg]]);
                try {
                    Object rc = sig.core.invoke(sig.on, args);
                    if (rc != RetCode.Success) {
                        continue;
                    }
                    unread.add(sig.name + "." + sig.legName[leg]);
                } catch (InvocationTargetException ite) {
                    read.add(sig.legName[leg]);
                } catch (ReflectiveOperationException ex) {
                    violation(tier + " " + sig.name + " could not be invoked: " + ex);
                }
            }
            check(!read.isEmpty(),
                  tier + " " + sig.name + " reads at least one of its declared legs "
                  + Arrays.toString(sig.legName));
            readLegs.put(sig.name, read);
        }

        check(withheld == CROSS_CALL_GUARDED.size(),
              tier + " unread legs: every withheld core is one of the "
              + CROSS_CALL_GUARDED.size() + " named in CROSS_CALL_GUARDED (got "
              + withheld + ")");
        System.out.println("  " + tier + " unread legs: " + unread.size()
            + " leg(s) declared but never indexed, at the default candle settings"
            + (unread.isEmpty() ? "" : " -> " + unread)
            + "; " + withheld + " core(s) WITHHELD (#236 step 3)");
        return readLegs;
    }

    /* ---------------------------------------------- sweep 4: openAndFill tier */

    /**
     * {@code NAME_OpenAndFill} over a history of {@code lookback + 5} bars, with
     * each output sized to exactly the {@code fillRange} the handle reports.
     *
     * <p>This is the streaming tier's only array-shaped promise. {@code update}
     * and {@code peek} take one scalar per leg, so there is no array to size, and
     * {@code Open} derives its range from the history array's own length, so
     * reading "past the end" is not a thing it can express. What is left —
     * "fills exactly {@code historyLen - lookback} values" — is a documented
     * claim with, until now, nothing holding it.
     *
     * <p>Public methods and public handles, so no reflection into internals: the
     * fill count is read off {@code fillRange()} exactly as a caller would.
     */
    private static void openAndFillSweep(Map<String, Sig> cores) {
        final int PAD = 16;
        int probed = 0;
        int refusals = 0;
        int violations = 0;
        List<String> reached = new ArrayList<>();

        Map<String, Method> fills = new TreeMap<>();
        for (Method m : Core.class.getMethods()) {
            if (m.getName().endsWith("_OpenAndFill")
                    && m.getParameterTypes()[0] == double[].class) {
                fills.put(m.getName().substring(0, m.getName().length()
                                                - "_OpenAndFill".length()), m);
            }
        }
        // Discovery, not a list: the streaming tier reaches every function, so a
        // core without an openAndFill is a generator regression rather than an
        // exemption. Checked before the sweep so an empty discovery cannot pass.
        check(fills.keySet().equals(cores.keySet()),
              "every core has a double[] openAndFill (" + fills.size() + " of "
              + cores.size() + "; missing "
              + missing(new ArrayList<>(cores.keySet()), new ArrayList<>(fills.keySet()))
              + ")");

        for (Map.Entry<String, Method> e : fills.entrySet()) {
            Sig sig = cores.get(e.getKey());
            if (sig == null) {
                continue;
            }
            Method fill = e.getValue();

            // (legs..., optIns..., outputs...) -- the batch signature without the
            // index pair and without the MIntegers, which the handle replaces.
            Class<?>[] pt = fill.getParameterTypes();
            int nLegs = sig.inputPos.length;
            int nOpts = sig.optPos.length;
            int nOuts = sig.outputPos.length;
            if (pt.length != nLegs + nOpts + nOuts) {
                violation(sig.name + "_OpenAndFill has " + pt.length + " parameters, "
                          + "expected " + nLegs + " legs + " + nOpts + " params + "
                          + nOuts + " outputs");
                violations++;
                continue;
            }

            for (Vector v : sig.vectors) {
                int historyLen = v.lookback + 5;
                Object[] args = new Object[pt.length];
                for (int k = 0; k < nLegs; k++) {
                    args[k] = series(pt[k], sig.legName[k], historyLen);
                }
                for (int k = 0; k < nOpts; k++) {
                    args[nLegs + k] = v.values[k];
                }

                // The documented failure path, first: a history one bar short of
                // lookback + 1 must be refused, and refused BEFORE anything is
                // written -- so with zero-length outputs the refusal must still be
                // the documented exception and never an out-of-bounds. (At a
                // lookback of 0 the short history is empty, which the core rejects
                // one line earlier as a bad parameter; either refusal proves the
                // same thing.)
                Object[] shortArgs = args.clone();
                for (int k = 0; k < nLegs; k++) {
                    shortArgs[k] = series(pt[k], sig.legName[k], v.lookback);
                }
                for (int k = 0; k < nOuts; k++) {
                    shortArgs[nLegs + nOpts + k] = zeroArray(pt[nLegs + nOpts + k]);
                }
                try {
                    fill.invoke(sig.on, shortArgs);
                    violation(sig.name + "_OpenAndFill[" + v.label + "] accepted a "
                        + v.lookback + "-bar history (lookback " + v.lookback
                        + "), which is one short of the documented minimum");
                    violations++;
                } catch (InvocationTargetException ite) {
                    Throwable t = ite.getCause();
                    if (t instanceof IndexOutOfBoundsException) {
                        violation(sig.name + "_OpenAndFill[" + v.label + "] wrote to a "
                            + "zero-length output before refusing a " + v.lookback
                            + "-bar history: " + t);
                        violations++;
                    } else {
                        refusals++;
                    }
                } catch (ReflectiveOperationException ex) {
                    violation(sig.name + "_OpenAndFill could not be invoked: " + ex);
                    violations++;
                }

                for (int k = 0; k < nOuts; k++) {
                    args[nLegs + nOpts + k] =
                        Array.newInstance(pt[nLegs + nOpts + k].getComponentType(),
                                          historyLen + PAD);
                }
                OutRange filled;
                try {
                    Object handle = fill.invoke(sig.on, args);
                    filled = (OutRange) handle.getClass().getMethod("fillRange")
                                              .invoke(handle);
                } catch (InvocationTargetException ite) {
                    violation(sig.name + "_OpenAndFill[" + v.label + "] threw on a "
                              + historyLen + "-bar history (lookback " + v.lookback
                              + ") with padded outputs: " + ite.getCause());
                    violations++;
                    continue;
                } catch (ReflectiveOperationException ex) {
                    violation(sig.name + "_OpenAndFill could not be invoked: " + ex);
                    violations++;
                    continue;
                }
                // Same reason the batch sweep does not take its count on trust.
                if (filled.count() != historyLen - v.lookback) {
                    violation(sig.name + "_OpenAndFill[" + v.label + "] over "
                        + historyLen + " bars reports a fill of " + filled.count()
                        + ", not the documented historyLen - lookback ("
                        + (historyLen - v.lookback) + ")");
                    violations++;
                    continue;
                }

                for (int k = 0; k < nOuts; k++) {
                    args[nLegs + nOpts + k] =
                        Array.newInstance(pt[nLegs + nOpts + k].getComponentType(),
                                          filled.count());
                }
                probed++;
                if (v.label.equals("defaults")) {
                    reached.add(sig.name);
                }
                try {
                    fill.invoke(sig.on, args);
                } catch (InvocationTargetException ite) {
                    Throwable t = ite.getCause();
                    violation(sig.name + "_OpenAndFill[" + v.label + "] over "
                        + historyLen + " bars wrote outside the " + filled.count()
                        + " values its fillRange reports: "
                        + t.getClass().getSimpleName() + ": " + t.getMessage());
                    violations++;
                } catch (ReflectiveOperationException ex) {
                    violation(sig.name + "_OpenAndFill could not be invoked: " + ex);
                    violations++;
                }
            }
        }

        check(reached.size() == fills.size(),
              "openAndFill: every handle is probed (" + reached.size() + " of "
              + fills.size() + "; missing "
              + missing(new ArrayList<>(fills.keySet()), reached) + ")");
        System.out.println("  openAndFill: " + probed + " handles probed, "
            + refusals + " short-history refusal(s), " + violations + " violation(s)");
    }

    /* ------------------------------------------------------------- plumbing */

    /** A fresh argument array with the range, the parameters and both MIntegers set. */
    private static Object[] args(Sig sig, Vector v, int startIdx, int endIdx) {
        Object[] args = new Object[sig.core.getParameterCount()];
        args[0] = startIdx;
        args[1] = endIdx;
        for (int k = 0; k < sig.optPos.length; k++) {
            args[sig.optPos[k]] = v.values[k];
        }
        args[sig.begPos] = new MInteger();
        args[sig.nbPos] = new MInteger();
        return args;
    }

    /** Sizes every input to {@code inLen} bars and every output to {@code outLen}. */
    private static void fill(Sig sig, Object[] args, int inLen, int outLen) {
        Class<?>[] pt = sig.core.getParameterTypes();
        for (int k = 0; k < sig.inputPos.length; k++) {
            args[sig.inputPos[k]] = series(pt[sig.inputPos[k]], sig.legName[k], inLen);
        }
        for (int p : sig.outputPos) {
            args[p] = Array.newInstance(pt[p].getComponentType(), outLen);
        }
    }

    private static String range(int startIdx, int endIdx) {
        return "[" + startIdx + ".." + endIdx + "]";
    }

    /** The width and settings a sweep ran under, for its messages. */
    private static String tier(boolean wantFloat, Map<String, Sig> cores) {
        String width = wantFloat ? "float[]" : "double[]";
        if (cores.isEmpty() || cores.values().iterator().next().on == Core.DEFAULT) {
            return width;
        }
        return width + " unstable=3";
    }

    /** Elements of {@code a} that {@code b} does not have. */
    private static List<String> missing(List<String> a, List<String> b) {
        List<String> out = new ArrayList<>(a);
        out.removeAll(b);
        return out;
    }

    /* -------------------------------------------------- proving the detectors */

    /**
     * Each sweep's detector, proved before the sweep is trusted.
     *
     * <p>Every one of these is a control arm: a call that <b>must</b> throw. If the
     * bounds check the whole file rests on ever stopped firing — a caught-too-broadly
     * exception, an array quietly resized, an argument built wrong — the sweeps
     * would all go green, and only these would go red.
     */
    static void theProbesCanFail() {
        Core core = Core.DEFAULT;
        int lookback = core.SMA_Lookback(30);
        MInteger b = new MInteger();
        MInteger n = new MInteger();

        // 1. sub-lookback: the quiet case, then one bar longer.
        RetCode quiet = core.SMA_Impl(0, lookback - 1, new double[0], 30, b, n,
                                          new double[0]);
        check(quiet == RetCode.Success && n.value == 0,
              "a sub-lookback range with zero-length arrays is a silent success");
        check(throwsOob(() -> core.SMA_Impl(0, lookback, new double[0], 30, b, n,
                                                new double[0])),
              "one bar longer DOES touch the arrays, so sweep 1 can detect I/O");

        // 2. exact extent: the exact sizes pass, one element short in either
        //    direction throws. Proves BOTH bounds, not just the output one.
        int endIdx = lookback + 4;
        int count = endIdx - lookback + 1;
        double[] in = new double[endIdx + 1];
        for (int i = 0; i < in.length; i++) {
            in[i] = bar("inReal", i);
        }
        check(core.SMA_Impl(0, endIdx, in, 30, b, n, new double[count])
                  == RetCode.Success && n.value == count,
              "exactly-sized input and output are enough for SMA");
        check(throwsOob(() -> core.SMA_Impl(0, endIdx, in, 30, b, n,
                                                new double[count - 1])),
              "an output one short of the count throws, so sweep 2 sees over-writes");
        check(throwsOob(() -> core.SMA_Impl(0, endIdx, Arrays.copyOf(in, endIdx),
                                                30, b, n, new double[count])),
              "an input one short of endIdx+1 throws, so sweep 2 sees over-reads");

        // 3. unread legs: a leg that IS read, given nothing, must throw. Without
        //    this the sweep could report every leg unread and still pass.
        double[] bars = new double[endIdx + 1];
        for (int i = 0; i < bars.length; i++) {
            bars[i] = bar("close", i);
        }
        check(throwsOob(() -> core.MEDPRICE_Impl(0, endIdx, new double[0], bars,
                                                     b, n, new double[endIdx + 1])),
              "a leg the function reads, given zero length, throws");

        // 4. openAndFill: the reported fill count is exactly enough, one less is not.
        double[] history = new double[lookback + 5];
        for (int i = 0; i < history.length; i++) {
            history[i] = bar("inReal", i);
        }
        int fillCount = core.SMA_OpenAndFill(history, 30, new double[history.length])
                            .fillRange().count();
        check(fillCount == history.length - lookback,
              "openAndFill fills historyLen - lookback (" + fillCount + " of "
              + (history.length - lookback) + ")");
        check(throwsOob(() -> core.SMA_OpenAndFill(history, 30, new double[fillCount - 1])),
              "an openAndFill output one short of fillRange throws, so sweep 4 can fail");
    }

    private static boolean throwsOob(Runnable body) {
        try {
            body.run();
            return false;
        } catch (IndexOutOfBoundsException ex) {
            return true;
        }
    }

    public static void main(String[] args) {
        theProbesCanFail();

        Map<String, TreeSet<String>> legsByWidth = null;
        Map<String, Sig> cores0 = null;
        for (boolean wantFloat : new boolean[] { false, true }) {
            Map<String, Sig> cores = discover(wantFloat, Core.DEFAULT);

            // Non-vacuity, tied to a GENERATED source of truth rather than a literal.
            //
            // A floor ("at least 140") fails open in exactly the case that matters:
            // add ten functions, silently lose ten from discovery, and the floor
            // still passes. So compare against the metadata registry, which
            // ta_codegen regenerates from the same input/ directory the cores come
            // from. A new indicator lands in both or in neither; if it lands in one,
            // this is red.
            List<String> registry = new ArrayList<>();
            for (FunctionInfo f : Functions.all()) {
                registry.add(f.name());
            }
            java.util.Collections.sort(registry);
            List<String> discovered = new ArrayList<>(cores.keySet());
            String tier = wantFloat ? "float[]" : "double[]";
            check(discovered.equals(registry),
                  tier + " sweep covers every registered function (" + discovered.size()
                  + " cores vs " + registry.size() + " registered)"
                  + (discovered.equals(registry) ? "" : "; missing "
                     + missing(registry, discovered) + ", unexpected "
                     + missing(discovered, registry)));

            if (!wantFloat) {
                cores0 = cores;
            }
            subLookbackSweep(wantFloat, cores);
            exactExtentSweep(wantFloat, cores);
            Map<String, TreeSet<String>> legs = unreadLegSweep(wantFloat, cores);
            if (legsByWidth == null) {
                legsByWidth = legs;
            } else {
                check(legs.equals(legsByWidth),
                      "the double[] and float[] bodies read the same input legs");
            }
            if (!wantFloat) {
                openAndFillSweep(cores);
            }
        }

        // Once more against a non-zero unstable period. Twelve cores carry an
        // explicit `/* Skip the unstable period */ i = unstablePeriod[...]; while(
        // i-- > 0 )` warm-up loop that reads the input on every trip -- and at the
        // default of 0 that loop runs ZERO times, so those reads have never
        // executed under a bounds check at all. The setting also raises the
        // published lookback of every function that carries it, and of every
        // composite that calls one, which is precisely the lookback-versus-what-
        // the-body-reads coupling all four bugs found so far have lived in.
        //
        // The batch sweeps only, at one width: the streaming and unread-leg sweeps
        // assert shapes the unstable period does not move.
        Core unstable = Core.DEFAULT.toBuilder().unstablePeriod(FuncUnstId.ALL, 3).build();
        Map<String, Sig> shifted = discover(false, unstable);
        check(shifted.keySet().equals(cores0.keySet()),
              "the unstable-period pass sees the same cores (" + shifted.size() + " of "
              + cores0.size() + ")");
        // The pass is only worth running if it actually moves something. Tied to
        // the flag the metadata publishes, not to a count: if no function's
        // lookback changed, this pass is a duplicate of the one above and the
        // reader should be told rather than left to assume it added coverage.
        int moved = 0;
        for (Map.Entry<String, Sig> e : shifted.entrySet()) {
            if (e.getValue().vectors.get(0).lookback
                    != cores0.get(e.getKey()).vectors.get(0).lookback) {
                moved++;
            }
        }
        check(moved > 0, "a non-zero unstable period moves at least one lookback ("
              + moved + " of " + shifted.size() + ")");
        System.out.println("  unstable period 3: " + moved + " of " + shifted.size()
            + " lookbacks move");
        subLookbackSweep(false, shifted);
        exactExtentSweep(false, shifted);

        if (failures == 0) {
            System.out.println("NoPhantomIoTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("NoPhantomIoTest: " + failures + " of " + checks
                + " checks FAILED");
            System.exit(1);
        }
    }
}
