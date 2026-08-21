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
 *  072626 MF,CC  First Version — the OutRange batch contract. Absorbs the
 *                non-vacuous cases of the retired junit CoreTest.
 *  081826 MF,CC  Array-argument checks (#172 C2).
 */

package io.github.talib.test;

import io.github.talib.Core;
import io.github.talib.InsufficientHistoryException;
import io.github.talib.MAType;
import io.github.talib.OutRange;
import io.github.talib.RetCode;
import io.github.talib.TaLibFailure;

import java.util.Arrays;
import java.util.EnumSet;
import java.util.Set;

/**
 * The batch API's contract: what {@link OutRange} means, and which misuses
 * throw what.
 *
 * <p>Junit-free by design (see {@link CoreApiTest}). Numerical correctness is
 * not this file's job — {@code ta_regtest --codegen} and {@code --xlang-hash}
 * prove that against the C reference for all 168 functions. What is tested here
 * is the surface a Java caller touches, which no cross-language harness sees.
 *
 * <p>It absorbs the two non-vacuous cases of the retired junit {@code CoreTest}
 * (a MAX call with known outputs, and the CMO {@code FLT_EPSILON} case that
 * pins the untouched tail of the output buffer). {@code CoreTest}'s other
 * methods — {@code testMFI}, {@code testHT}, {@code testMA_MAMA},
 * {@code test_MACD} — asserted nothing whatsoever (the last carried a literal
 * {@code // TODO Add tests of outputs}) and were dropped rather than ported.
 */
public class BatchApiTest {

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

    /**
     * Same, plus the message has to name the things a caller needs to fix it.
     * A check that throws the right type with a useless message is half a check:
     * the length rejections all share one exception class, so the message is the
     * only thing that says <i>which</i> array and by how much.
     */
    private static void checkThrows(Class<? extends RuntimeException> expected,
                                    Runnable body, String what, String... needles) {
        checks++;
        try {
            body.run();
            failures++;
            System.out.println("  FAIL: " + what + " (no exception thrown)");
            return;
        } catch (RuntimeException e) {
            if (!expected.isInstance(e)) {
                failures++;
                System.out.println("  FAIL: " + what + " (threw " + e.getClass().getName()
                    + ": " + e.getMessage() + ")");
                return;
            }
            String msg = String.valueOf(e.getMessage());
            for (String needle : needles) {
                if (!msg.contains(needle)) {
                    failures++;
                    System.out.println("  FAIL: " + what + " (message \"" + msg
                        + "\" omits \"" + needle + "\")");
                    return;
                }
            }
        }
    }

    private static double[] closes(int n) {
        double[] out = new double[n];
        for (int i = 0; i < n; i++) {
            out[i] = 100.0 + 10.0 * Math.sin(i / 7.0) + 3.0 * Math.cos(i / 3.0);
        }
        return out;
    }

    /* ---------------------------------------------------- ported from CoreTest */

    /** MAX over three bars with a period of 2 — known begIdx, count and values. */
    static void maxWithKnownOutputs() {
        double[] input = { 2.0, 1.2, 1.5 };
        double[] output = new double[3];

        OutRange r = Core.DEFAULT.MAX(0, 2, input, 2, output);

        check(r.begIdx() == 1, "MAX begIdx == 1 (got " + r.begIdx() + ")");
        check(r.count() == 2, "MAX count == 2 (got " + r.count() + ")");
        check(output[0] == 2.0, "MAX[0] == 2.0");
        check(output[1] == 1.5, "MAX[1] == 1.5");
        check(!r.isEmpty(), "MAX range is not empty");
    }

    /** The first output lands exactly at the lookback. */
    static void begIdxEqualsLookback() {
        double[] in = closes(200);
        double[] out = new double[in.length];

        OutRange r = Core.DEFAULT.MA(0, in.length - 1, in, 10, MAType.SMA, out);
        check(r.begIdx() == Core.DEFAULT.MA_Lookback(10, MAType.SMA),
              "SMA begIdx == lookback");
        check(r.count() == in.length - r.begIdx(), "SMA count fills to the end");
    }

    /**
     * CMO over ±FLT_EPSILON data: the output starts at the lookback and nothing
     * past {@code count} is written. The untouched tail is the real assertion —
     * it catches a writer running past the range it reported.
     */
    static void cmoLeavesTheTailUntouched() {
        final double FLT_EPSILON = 1.192092896e-07;
        final double SENTINEL = -3e37;

        double[] in = new double[100];
        for (int i = 0; i < in.length; i++) {
            in[i] = ((i % 2) == 0 ? 1.0 : -1.0) * FLT_EPSILON;
        }
        double[] out = new double[100];
        java.util.Arrays.fill(out, SENTINEL);

        int lookback = Core.DEFAULT.CMO_Lookback(Integer.MIN_VALUE);
        OutRange r = Core.DEFAULT.CMO(0, in.length - 1, in, Integer.MIN_VALUE, out);

        check(r.begIdx() == lookback, "CMO begIdx == lookback");
        check(r.count() > 0, "CMO produced values (so the tail check is not vacuous)");
        boolean tailUntouched = true;
        for (int i = r.count(); i < out.length; i++) {
            if (out[i] != SENTINEL) {
                tailUntouched = false;
            }
        }
        check(tailUntouched, "CMO wrote nothing past count");
    }

    /* -------------------------------------------------- the OutRange contract */

    /**
     * The invariant this whole error model hangs on: a valid range shorter than
     * the lookback is a SUCCESS with no values, never an exception. Matches C's
     * {@code TA_SUCCESS} + {@code outNBElement == 0}.
     */
    static void shortRangeIsAnEmptySuccessNotAnException() {
        double[] in = closes(10);
        double[] out = new double[10];

        check(Core.DEFAULT.SMA_Lookback(30) > 9,
              "the 30-period lookback really does exceed this 10-bar range");

        OutRange r = Core.DEFAULT.SMA(0, in.length - 1, in, 30, out);
        check(r.count() == 0, "too-short range yields count == 0");
        check(r.isEmpty(), "too-short range is isEmpty()");
        check(r.begIdx() == 0, "empty range reports begIdx 0");
    }

    /** Misuse mapping. Note what is absent: no code path returns a RetCode. */
    static void misuseThrowsTheDocumentedException() {
        final double[] in = closes(100);
        final double[] out = new double[100];

        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(-1, 50, in, 10, out), "negative startIdx -> IndexOutOfBounds");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(0, -1, in, 10, out), "negative endIdx -> IndexOutOfBounds");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(50, 10, in, 10, out), "endIdx < startIdx -> IndexOutOfBounds");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 50, in, 0, out), "period below range -> IllegalArgument");
        // The cast is required, not incidental: `null` alone is ambiguous between
        // the double[] and float[] overloads. Real callers pass a typed array.
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.SMA(0, 50, (double[]) null, 10, out),
            "null input -> NullPointerException");

        // Two outputs sharing one array has no correct answer (issue #108).
        final double[] shared = new double[100];
        final double[] third = new double[100];
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.BBANDS(0, 50, in, 20, 2.0, 2.0, MAType.SMA, shared, shared, third),
            "aliased output arrays -> IllegalArgument");
    }

    /* ------------------------------------------- array-argument checks (#172 C2) */

    /*
     * The four misuses in issue #172's C2 table. Each one used to be an
     * ArrayIndexOutOfBoundsException (or a JVM helpful-NPE) raised from deep
     * inside the algorithm, after the output buffer was already partly written
     * and with no OutRange to say how far the call had got. C cannot do better —
     * it is handed bare pointers and has no sizes — but Java arrays carry their
     * length.
     *
     * The bound is the one the Rust backend asserts and the cross-language
     * harness verifies: an input must reach endIdx, and an output must hold the
     * values actually PRODUCED — endIdx - max(startIdx, lookback) + 1, which for
     * a range starting below the lookback is shorter than the range. The exact
     * boundary is pinned in bothSidesOfTheOutputBound() below; without that pair
     * every assertion here would still pass against a bound that was merely
     * "some number", which is the vacuity this file's whole method is against.
     */

    /** Requesting more than the input holds names the input, not a bar index. */
    static void undersizedInputIsRejected() {
        final double[] in = closes(200);
        final double[] out = new double[501];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 500, in, 10, out),
            "endIdx past the input end -> IllegalArgument",
            "SMA", "inReal", "200", "501");
    }

    /** Two input series of different lengths: the short one is named. */
    static void mismatchedInputLengthsAreRejected() {
        final double[] longer = closes(200);
        final double[] shorter = closes(50);
        final double[] out = new double[200];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.ADD(0, 199, longer, shorter, out),
            "mismatched input lengths -> IllegalArgument",
            "ADD", "inReal1", "50", "200");
        // Same call with the legs the right way round is the control: nothing
        // about ADD's shape makes it throw.
        check(Core.DEFAULT.ADD(0, 49, longer, shorter, out).count() == 50,
              "ADD over the range both legs cover succeeds");
    }

    /** An undersized output names the output and both sizes. */
    static void undersizedOutputIsRejected() {
        final double[] in = closes(200);
        final double[] out = new double[3];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, out),
            "undersized output -> IllegalArgument",
            "SMA", "outReal", "3", "191");
    }

    /**
     * A null array is named where it was passed, not where it was first read.
     * The casts are required, not incidental: {@code null} alone is ambiguous
     * between the double[] and float[] overloads.
     */
    static void nullArraysAreNamed() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.SMA(0, 199, (double[]) null, 10, out),
            "null input -> NullPointerException naming it", "SMA", "inReal");
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, (double[]) null),
            "null output -> NullPointerException naming it", "SMA", "outReal");
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.SMA(0, 199, (float[]) null, 10, out),
            "null float input -> NullPointerException naming it", "SMA", "inReal");
        // An argument that does not exist is a bug however little of it would
        // have been read: the null check outlives the lookback short-circuit
        // that switches the LENGTH check off.
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.SMA(0, 9, in, 30, (double[]) null),
            "null output on a range that produces nothing -> NullPointerException",
            "SMA", "outReal");
    }

    /**
     * The exact boundary, from both sides. This is what makes every other length
     * assertion in this section mean something: an emitter that required
     * {@code endIdx - startIdx + 1} instead of the produced count would still
     * reject all the calls above, and would fail here.
     */
    static void bothSidesOfTheOutputBound() {
        double[] in = closes(200);
        int lookback = Core.DEFAULT.SMA_Lookback(10);
        int produced = 199 - lookback + 1;

        check(produced == 191, "the produced count really is 191 (got " + produced + ")");
        check(produced < 200, "the produced count is shorter than the requested range");

        OutRange r = Core.DEFAULT.SMA(0, 199, in, 10, new double[produced]);
        check(r.count() == produced, "an exactly-sized output is accepted and filled");

        final double[] oneShort = new double[produced - 1];
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, oneShort),
            "one element short of the produced count -> IllegalArgument",
            "outReal", String.valueOf(produced - 1), String.valueOf(produced));
    }

    /**
     * The complaint C2 actually makes: not that the call fails, but that it
     * fails halfway through, having already scribbled on the caller's buffer.
     */
    static void aRejectedCallWritesNothing() {
        final double SENTINEL = -3e37;
        double[] in = closes(200);
        double[] out = new double[3];
        Arrays.fill(out, SENTINEL);

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, out), "undersized output throws");

        boolean untouched = true;
        for (double v : out) {
            if (v != SENTINEL) {
                untouched = false;
            }
        }
        check(untouched, "a rejected call left the output buffer untouched");
        // Non-vacuity: the same buffer IS writable by a call that is accepted, so
        // the assertion above is about the rejection and not about the sentinel.
        Core.DEFAULT.SMA(0, 2, in, 1, out);
        check(out[0] != SENTINEL, "the sentinel is overwritten by a call that runs");
    }

    /**
     * The checks do not pre-empt the core's own RetCode mapping. Every case here
     * is BOTH a bad argument and an undersized buffer; the core owns the
     * diagnosis, so its exception has to be the one that comes out.
     */
    static void theCoreStillOwnsItsOwnDiagnoses() {
        final double[] in = closes(200);
        final double[] tiny = new double[3];

        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(50, 10, in, 10, tiny),
            "endIdx < startIdx still -> IndexOutOfBounds", "endIdx");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(-1, 199, in, 10, tiny),
            "negative startIdx still -> IndexOutOfBounds", "startIdx");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 0, tiny),
            "out-of-range period still -> the parameter message", "bad parameter");
        // The MAX_INDEX clause of clampedStart is the only one the three cases above
        // cannot reach: endIdx < startIdx and a negative startIdx both re-derive their
        // own rejection from the clamp, so dropping this clause is otherwise silent.
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(0, Core.MAX_INDEX + 1, in, 10, tiny),
            "endIdx above MAX_INDEX still -> IndexOutOfBounds", "endIdx");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(Core.MAX_INDEX + 5, Core.MAX_INDEX + 9, in, 10, tiny),
            "startIdx above MAX_INDEX still -> IndexOutOfBounds", "startIdx");
    }

    /**
     * A valid range shorter than the lookback produces nothing, so it reads
     * nothing and any output length is fine — including none. This is the
     * {@code _assertStart > endIdx ||} short-circuit in front of the Rust
     * asserts; without it the length check would throw exactly where
     * {@link #shortRangeIsAnEmptySuccessNotAnException} promises a success.
     * It applies to the OUTPUT bound only — see
     * {@link #anEndIdxPastTheInputIsRejectedEvenProducingNothing}.
     */
    static void aRangeThatProducesNothingChecksNoLength() {
        double[] in = closes(10);

        check(Core.DEFAULT.SMA_Lookback(30) > 9, "the 30-period lookback exceeds this range");
        OutRange r = Core.DEFAULT.SMA(0, 9, in, 30, new double[0]);
        check(r.count() == 0, "a zero-length output is fine when nothing is produced");
    }

    /**
     * The INPUT bound survives the short-range escape that switches the output
     * bound off: an {@code endIdx} past the end of the supplied series is a caller
     * bug in every range, including one that produces no values.
     *
     * <p>C answers this call with {@code TA_SUCCESS} and a zero count — but only
     * because it has no array size to check against, and an empty {@link OutRange}
     * reads as "not enough data yet" rather than "your endIdx is past the end of
     * your own array". This is the one bound where Java checks more than C and
     * Rust; it is a diagnostic, not a safety net.
     *
     * <p>These three functions are here for a reason: APO, PPO and PVO used to run
     * their fast MA — whose lookback is smaller than their own — over the whole
     * requested range before discovering the range was too short, reading the
     * caller's input to build a result the empty slow MA then discarded. They now
     * return before touching anything, and {@code NoPhantomIoTest} pins that for
     * every core. This test pins the wrapper's half of it.
     */
    static void anEndIdxPastTheInputIsRejectedEvenProducingNothing() {
        final double[] in = closes(24);
        final double[] wide = closes(25);
        final double[] vol = new double[24];
        java.util.Arrays.fill(vol, 1000.0);
        final double[] volWide = new double[25];
        java.util.Arrays.fill(volWide, 1000.0);

        check(Core.DEFAULT.APO_Lookback(12, 26, MAType.EMA) > 24,
              "APO's lookback really does exceed this range (so nothing is produced)");

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.APO(0, 24, in, 12, 26, MAType.EMA, new double[0]),
            "APO: endIdx past the input, producing nothing", "APO", "inReal", "24", "25");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.PPO(0, 24, in, 12, 26, MAType.EMA, new double[0]),
            "PPO likewise", "PPO", "inReal", "24", "25");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.PVO(0, 24, vol, 12, 26, MAType.EMA, new double[0]),
            "PVO likewise", "PVO", "inVolume", "24", "25");

        // The controls. An input that DOES reach endIdx is an empty success again,
        // with a zero-length output — so the fix tightened the input bound only, and
        // did not turn the documented empty success into an error.
        check(Core.DEFAULT.APO(0, 24, wide, 12, 26, MAType.EMA, new double[0]).count() == 0,
              "APO with an input reaching endIdx is an empty success, zero-length output");
        check(Core.DEFAULT.PVO(0, 24, volWide, 12, 26, MAType.EMA, new double[0]).count() == 0,
              "PVO likewise");
        check(Core.DEFAULT.SMA(0, 24, wide, 26, new double[0]).count() == 0,
              "and a function that reads nothing is unaffected");
    }

    /** Every output is checked on its own, and named on its own. */
    static void eachOutputIsCheckedSeparately() {
        final double[] in = closes(200);
        int produced = 199 - Core.DEFAULT.MACD_Lookback(12, 26, 9) + 1;
        check(produced > 0, "MACD produces values over this range");

        final double[] big1 = new double[200];
        final double[] big2 = new double[200];
        final double[] small = new double[produced - 1];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, small, big1, big2),
            "short first output is named", "outMACD", String.valueOf(produced));
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, big1, small, big2),
            "short second output is named", "outMACDSignal", String.valueOf(produced));
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, big1, big2, small),
            "short third output is named", "outMACDHist", String.valueOf(produced));
        check(Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, big1, big2,
                                new double[produced]).count() == produced,
              "three exactly-sized outputs are accepted");
    }

    /** The {@code int[]} outputs the candlestick patterns write are checked too. */
    static void integerOutputsAreChecked() {
        final double[] o = closes(200);
        final double[] h = closes(200);
        final double[] l = closes(200);
        final double[] c = closes(200);

        // Both sizes, not just the one allocated: the int[] and float[] overloads of
        // requireLength are two-line delegates, and forwarding a WRONG required count
        // is the only mutation of this guard the double[] assertions cannot see.
        int produced = 199 - Core.DEFAULT.CDLDOJI_Lookback() + 1;
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.CDLDOJI(0, 199, o, h, l, c, new int[3]),
            "short int[] output -> IllegalArgument",
            "CDLDOJI", "outInteger", "3", String.valueOf(produced));
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.CDLDOJI(0, 199, o, h, l, c, (int[]) null),
            "null int[] output -> NullPointerException", "outInteger");
    }

    /** The float overload carries the identical checks, on its own array type. */
    static void floatOverloadIsCheckedToo() {
        double[] in = closes(200);
        final float[] inF = new float[200];
        for (int i = 0; i < in.length; i++) {
            inF[i] = (float) in[i];
        }
        final double[] tiny = new double[3];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, inF, 10, tiny),
            "float overload: undersized output", "SMA", "outReal", "191");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 500, inF, 10, new double[501]),
            "float overload: endIdx past the input end", "SMA", "inReal", "200", "501");
        check(Core.DEFAULT.SMA(0, 199, inF, 10, new double[191]).count() == 191,
              "float overload accepts an exactly-sized output");
    }

    /**
     * A leg the algorithm never indexes is not checked. Four candlestick patterns
     * declare an OHLC input they do not read — CDL3OUTSIDE's high and low among
     * them — and the generated Rust asserts skip exactly those, so rejecting them
     * here would make the same call a success in one language and a throw in the
     * other. The control is the leg next to it, which IS read.
     */
    static void anUnreadLegIsNotChecked() {
        final double[] real = closes(200);
        final double[] empty = new double[0];
        final int[] out = new int[200];

        OutRange r = Core.DEFAULT.CDL3OUTSIDE(0, 199, real, empty, empty, real, out);
        check(r.count() > 0, "CDL3OUTSIDE runs with empty high/low legs it never reads");

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.CDL3OUTSIDE(0, 199, empty, real, real, real, out),
            "the open leg, which IS read, is still checked", "inOpen", "0", "200");
    }

    /**
     * The metadata path reaches the same guard: {@link io.github.talib.metadata.Dispatch}
     * calls the public wrappers, so an undersized buffer handed to a ParamHolder
     * fails the same way rather than crashing inside the algorithm.
     */
    static void theMetadataPathIsGuardedToo() {
        double[] in = closes(200);
        io.github.talib.metadata.ParamHolder h =
            io.github.talib.metadata.Functions.byName("SMA").newCall(Core.DEFAULT);
        h.setInput(0, in).setOptInput(0, 10).setOutput(0, new double[3]);

        checkThrows(IllegalArgumentException.class,
            () -> h.call(0, 199), "ParamHolder.call is guarded too", "SMA", "outReal");

        // Control: the same holder with a big enough output runs.
        h.setOutput(0, new double[200]);
        check(h.call(0, 199).count() == 191, "the same call with a sized output succeeds");
    }

    /**
     * The public surface exposes no unguarded tier. Pinned by reflection over the
     * shipped Core rather than by a string assertion on generator output.
     */
    static void noUnguardedTierOnThePublicSurface() {
        int leaked = 0;
        for (java.lang.reflect.Method m : Core.class.getMethods()) {
            if (m.getName().endsWith("Unguarded") || m.getName().endsWith("UnguardedInternal")) {
                leaked++;
            }
        }
        check(leaked == 0, "no Unguarded method survives on the public Core surface");
        // Non-vacuity: the reflection actually sees the surface it is asserting over.
        int sma = 0;
        for (java.lang.reflect.Method m : Core.class.getMethods()) {
            if (m.getName().equals("SMA")) {
                sma++;
            }
        }
        check(sma >= 2, "reflection sees the SMA overloads it is filtering over");
    }

    /** The float overload adopts the identical shape (C's TA_S_* parity). */
    static void floatOverloadHasTheSameShape() {
        double[] in = closes(100);
        float[] inF = new float[100];
        for (int i = 0; i < in.length; i++) {
            inF[i] = (float) in[i];
        }
        double[] outD = new double[100];
        double[] outF = new double[100];

        OutRange rd = Core.DEFAULT.SMA(0, in.length - 1, in, 10, outD);
        OutRange rf = Core.DEFAULT.SMA(0, inF.length - 1, inF, 10, outF);

        check(rd.equals(rf), "float overload reports the same OutRange");
        check(rf.count() > 0, "float overload produced values");
    }

    /** OutRange is a value type: equality by components, usable as a key. */
    static void outRangeValueSemantics() {
        check(new OutRange(3, 7).equals(new OutRange(3, 7)), "OutRange equals by value");
        check(!new OutRange(3, 7).equals(new OutRange(3, 8)), "OutRange distinguishes count");
        check(new OutRange(3, 7).hashCode() == new OutRange(3, 7).hashCode(),
              "OutRange hashCode agrees with equals");
        check(OutRange.EMPTY.isEmpty() && OutRange.EMPTY.count() == 0, "OutRange.EMPTY");
        check(new OutRange(3, 7).toString().contains("begIdx"), "OutRange toString names components");
    }

    /**
     * Every failure carries the code C would have returned, and the mapping back
     * is TOTAL and LOSSLESS.
     *
     * <p>Total: every exception the public API raises implements
     * {@link TaLibFailure}, including the two conditions C cannot detect (an
     * absent argument, a buffer too short) and the raw JVM types those used to
     * be. Anything not covered leaves a caller with a thrown object it cannot
     * classify, which is the state this replaced.
     *
     * <p>Lossless: no two codes share one thrown representation. That is the
     * half the exception TYPES cannot carry —
     * {@link IndexOutOfBoundsException} serves both out-of-range index codes and
     * {@link IllegalStateException} serves both library-side ones — so a check
     * on the type alone would pass with the two arms of {@code failure()}
     * swapped.
     */
    static void everyFailureCarriesItsCode() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        // Lossless, the pair the type cannot separate.
        checkCode(RetCode.OutOfRangeStartIndex,
            () -> Core.DEFAULT.SMA(-1, 50, in, 10, out), "negative startIdx carries OutOfRangeStartIndex");
        checkCode(RetCode.OutOfRangeEndIndex,
            () -> Core.DEFAULT.SMA(50, 10, in, 10, out), "endIdx < startIdx carries OutOfRangeEndIndex");

        // The rest of the batch tier's vocabulary.
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.SMA(0, 50, in, 0, out), "an out-of-range period carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, out, out, new double[200]),
            "two outputs sharing one array carries BadParam");

        // The two conditions C has no code for. They report the code C answers
        // for an absent argument it CAN detect, so the mapping stays total.
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.SMA(0, 199, (double[]) null, 10, out), "a null input carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, new double[3]), "a short output carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.MA(0, 199, in, 10, null, out), "a null enum carries BadParam");

        // Streaming's one recoverable condition, which is why it has a code.
        checkCode(RetCode.InsufficientHistory,
            () -> Core.DEFAULT.SMA_Open(Arrays.copyOf(in, Core.DEFAULT.SMA_Lookback(30)), 30),
            "a short history carries InsufficientHistory");

        // ...and the REST of the streaming tier, which is a separate reject
        // ladder from the batch one. Totality is a property of every failure the
        // library raises, not of the tier someone happened to convert first.
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.SMA_Open(new double[0], 30),
            "an empty history carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.SMA_Open(in, 0),
            "an out-of-range period on a stream open carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.BBANDS_OpenAndFill(in, 20, 2.0, 2.0, MAType.SMA, out, out, new double[200]),
            "aliased OpenAndFill outputs carry BadParam");

        // ...and it is still an InsufficientHistoryException, so an existing
        // catch keeps working.
        checkThrows(InsufficientHistoryException.class,
            () -> Core.DEFAULT.SMA_Open(Arrays.copyOf(in, Core.DEFAULT.SMA_Lookback(30)), 30),
            "a short history is still typed");

        // The numbers the cross-language harness compares. Hardcoded, because
        // asking the enum for its own value would prove nothing.
        check(RetCode.Success.asCInt() == 0, "Success is 0");
        check(RetCode.BadParam.asCInt() == 2, "BadParam is 2");
        check(RetCode.AllocErr.asCInt() == 3, "AllocErr is 3");
        check(RetCode.OutOfRangeStartIndex.asCInt() == 12, "OutOfRangeStartIndex is 12");
        check(RetCode.OutOfRangeEndIndex.asCInt() == 13, "OutOfRangeEndIndex is 13");
        check(RetCode.InsufficientHistory.asCInt() == 17, "InsufficientHistory is 17");
        check(RetCode.InternalError.asCInt() == 5000, "InternalError is 5000");

        // Non-vacuity: the cases above have to REACH every code the batch and
        // streaming tiers can produce, or a member could stop being emitted
        // anywhere and nothing here would move. AllocErr and InternalError are
        // the two exceptions -- one is unreachable in Java (#178) and the other
        // needs a corrupted CIRCBUF size -- so they are named rather than
        // silently excluded.
        Set<RetCode> expected = EnumSet.of(
            RetCode.OutOfRangeStartIndex, RetCode.OutOfRangeEndIndex,
            RetCode.BadParam, RetCode.InsufficientHistory);
        check(seenCodes.equals(expected),
              "the probes reached exactly " + expected + " (got " + seenCodes + ")");
    }

    private static final Set<RetCode> seenCodes = EnumSet.noneOf(RetCode.class);

    /** The call must throw, the throw must carry a code, and it must be this one. */
    private static void checkCode(RetCode expected, Runnable body, String what) {
        checks++;
        try {
            body.run();
            failures++;
            System.out.println("  FAIL: " + what + " (no exception thrown)");
        } catch (RuntimeException e) {
            if (!(e instanceof TaLibFailure)) {
                failures++;
                System.out.println("  FAIL: " + what + " (" + e.getClass().getName()
                    + " carries no RetCode)");
                return;
            }
            RetCode got = ((TaLibFailure) e).retCode();
            if (got != expected) {
                failures++;
                System.out.println("  FAIL: " + what + " (carried " + got + ")");
                return;
            }
            seenCodes.add(got);
        }
    }

    /**
     * The index rules are evaluated BEFORE the presence check.
     *
     * <p>The specification lists B-1 and B-2 ahead of B-3, and this wrapper used
     * to run the presence check first, so a negative {@code startIdx} with a null
     * input reported the null and said nothing about the index
     * ({@code docs/error-handling-spec.md}, open item 3). Every case here is
     * BOTH faults at once; only the order decides which is reported.
     */
    static void anIndexFaultOutranksAnAbsentArgument() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(-1, 50, (double[]) null, 10, out),
            "a negative startIdx outranks a null input", "startIdx");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(50, 10, in, 10, (double[]) null),
            "endIdx < startIdx outranks a null output", "endIdx");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(0, Core.MAX_INDEX + 1, (double[]) null, 10, out),
            "an endIdx above MAX_INDEX outranks a null input", "endIdx");

        // The control, and what makes the three above about ORDER rather than
        // about the null check having been deleted: with the indices valid, the
        // null IS the diagnosis.
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.SMA(0, 199, (double[]) null, 10, out),
            "a valid range still reports the null", "SMA", "inReal");
    }

    /**
     * A null enum parameter is rejected as the absent argument it is, naming the
     * function and the parameter.
     *
     * <p>It used to reach the {@code switch} inside the function's own
     * {@code _Lookback} and surface as a bare {@link NullPointerException} naming
     * neither ({@code docs/error-handling-spec.md}, open item 4). Java is the
     * only backend where this is expressible at all.
     */
    static void aNullEnumIsNamed() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.MA(0, 199, in, 10, null, out),
            "a null enum names the function and the parameter", "MA", "optInMAType");
        // ...and it does not outrank the index rules either.
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.MA(-1, 199, in, 10, null, out),
            "a negative startIdx outranks a null enum", "startIdx");
    }

    public static void main(String[] args) {
        maxWithKnownOutputs();
        begIdxEqualsLookback();
        cmoLeavesTheTailUntouched();
        shortRangeIsAnEmptySuccessNotAnException();
        misuseThrowsTheDocumentedException();
        undersizedInputIsRejected();
        mismatchedInputLengthsAreRejected();
        undersizedOutputIsRejected();
        nullArraysAreNamed();
        bothSidesOfTheOutputBound();
        aRejectedCallWritesNothing();
        theCoreStillOwnsItsOwnDiagnoses();
        aRangeThatProducesNothingChecksNoLength();
        anEndIdxPastTheInputIsRejectedEvenProducingNothing();
        eachOutputIsCheckedSeparately();
        integerOutputsAreChecked();
        floatOverloadIsCheckedToo();
        anUnreadLegIsNotChecked();
        theMetadataPathIsGuardedToo();
        noUnguardedTierOnThePublicSurface();
        floatOverloadHasTheSameShape();
        outRangeValueSemantics();
        everyFailureCarriesItsCode();
        anIndexFaultOutranksAnAbsentArgument();
        aNullEnumIsNamed();

        if (failures == 0) {
            System.out.println("BatchApiTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("BatchApiTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
